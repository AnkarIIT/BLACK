#include "OAuthManager.h"

#include <QUrlQuery>
#include <QUrl>
#include <QDesktopServices>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDateTime>
#include <QEventLoop>

#include "VaultCrypto.h"
#include "OSPaths.h"

namespace {

QString configFilePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + QStringLiteral("oauth.json");
}

QString connectedFilePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + QStringLiteral("services.json");
}

QByteArray successPageHtml()
{
    return QByteArray(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<!doctype html><html><head><meta charset=\"utf-8\">"
        "<title>BLACK</title></head><body style=\"margin:0;background:#0b0b0f;"
        "color:#f5f5f7;font-family:-apple-system,'Segoe UI',sans-serif;"
        "display:flex;align-items:center;justify-content:center;height:100vh;\">"
        "<div style=\"text-align:center\"><h1>You're signed in</h1>"
        "<p style=\"color:#86868b\">Return to BLACK.</p>"
        "<script>setTimeout(function(){window.close()},1200)</script></div>"
        "</body></html>");
}

QByteArray errorPageHtml(const QString &reason)
{
    return QByteArray("HTTP/1.1 400 Bad Request\r\n"
                      "Content-Type: text/html; charset=utf-8\r\n"
                      "Connection: close\r\n\r\n")
        + "<!doctype html><html><body style='background:#0b0b0f;color:#f5f5f7;"
          "font-family:sans-serif;text-align:center;margin-top:20vh'>"
        + "<h2>Sign-in failed</h2><p>" + reason.toHtmlEscaped().toUtf8()
        + "</p><p><small>Return to BLACK.</small></p></body></html>";
}

} // namespace

const OAuthManager::ProviderInfo OAuthManager::kProviders[OAuthManager::kProviderCount] = {
    // key, displayName, authUrl, tokenUrl, profileUrl, scopes, needsSecret, pkce, responseMode, port
    { "google", "Google",
      "https://accounts.google.com/o/oauth2/v2/auth",
      "https://oauth2.googleapis.com/token",
      "https://www.googleapis.com/oauth2/v3/userinfo",
      "openid email profile", false, true, nullptr, 0 },
    { "apple", "Apple",
      "https://appleid.apple.com/auth/authorize",
      "https://appleid.apple.com/auth/token",
      nullptr,
      "name email", true, true, "form_post", 0 },
    { "microsoft", "Microsoft",
      "https://login.microsoftonline.com/common/oauth2/v2.0/authorize",
      "https://login.microsoftonline.com/common/oauth2/v2.0/token",
      "https://graph.microsoft.com/v1.0/me",
      "openid profile email User.Read", false, true, nullptr, 0 },
    { "github", "GitHub",
      "https://github.com/login/oauth/authorize",
      "https://github.com/login/oauth/access_token",
      "https://api.github.com/user",
      "read:user user:email", true, false, nullptr, 9010 },
    { "slack", "Slack",
      "https://slack.com/oauth/v2/authorize",
      "https://slack.com/api/oauth.v2.access",
      "https://slack.com/api/openid.connect.userInfo",
      "openid profile email", false, true, nullptr, 9011 },
    { "discord", "Discord",
      "https://discord.com/oauth2/authorize",
      "https://discord.com/api/oauth2/token",
      "https://discord.com/api/users/@me",
      "identify email", false, true, nullptr, 9012 },
    { "drive", "Google Drive",
      "https://accounts.google.com/o/oauth2/v2/auth",
      "https://oauth2.googleapis.com/token",
      "https://www.googleapis.com/oauth2/v3/userinfo",
      "openid email profile drive.file", false, true, nullptr, 0 },
    { "calendar", "Google Calendar",
      "https://accounts.google.com/o/oauth2/v2/auth",
      "https://oauth2.googleapis.com/token",
      "https://www.googleapis.com/oauth2/v3/userinfo",
      "openid email profile calendar", false, true, nullptr, 0 },
    { "dropbox", "Dropbox",
      "https://www.dropbox.com/oauth2/authorize",
      "https://api.dropboxapi.com/oauth2/token",
      "https://api.dropboxapi.com/2/users/get_current_account",
      "account_info.read", false, true, nullptr, 9013 },
    { "notion", "Notion",
      "https://api.notion.com/v1/oauth/authorize",
      "https://api.notion.com/v1/oauth/token",
      nullptr,
      "", false, true, nullptr, 9014 },
};

OAuthManager::OAuthManager(QObject *parent)
    : QObject(parent)
{
    loadConfig();
    loadConnected();
}

void OAuthManager::loadConfig()
{
    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return;
    const QJsonObject root = doc.object();
    m_devMode = root.value(QStringLiteral("dev_mode")).toBool(false);
    for (int i = 0; i < kProviderCount; ++i) {
        const QJsonObject provider = root.value(QLatin1String(kProviders[i].key)).toObject();
        Config &cfg = m_configs[i];
        cfg.clientId = provider.value(QStringLiteral("clientId")).toString();
        cfg.clientSecret = provider.value(QStringLiteral("clientSecret")).toString();
        cfg.loopbackPort = provider.value(QStringLiteral("port")).toInt(0);
    }
}

const OAuthManager::Config &OAuthManager::config(Provider provider) const
{
    return m_configs[provider];
}

bool OAuthManager::isConfigured(int provider) const
{
    if (provider < Google || provider >= kProviderCount)
        return false;
    if (m_devMode)
        return true;
    return m_configs[provider].enabled();
}

bool OAuthManager::isConnected(int provider) const
{
    if (provider < Google || provider >= kProviderCount)
        return false;
    return connectedForCurrentAccount().contains(QLatin1String(kProviders[provider].key));
}

QJsonArray OAuthManager::connectedServicesJson() const
{
    QJsonArray result;
    const QJsonObject accountSet = connectedForCurrentAccount();
    for (int i = kFirstService; i < kProviderCount; ++i) {
        const QString key = QLatin1String(kProviders[i].key);
        const QJsonObject entry = accountSet.value(key).toObject();
        if (entry.isEmpty())
            continue;
        QJsonObject out;
        out.insert(QStringLiteral("provider"), key);
        out.insert(QStringLiteral("name"), QLatin1String(kProviders[i].displayName));
        out.insert(QStringLiteral("user"), entry.value(QStringLiteral("user")).toObject());
        result.append(out);
    }
    return result;
}

QJsonObject OAuthManager::connectedForCurrentAccount() const
{
    return m_connected.value(m_currentAccountId).toObject();
}

void OAuthManager::setConnectedEntry(const QString &providerKey, const QJsonObject &entry)
{
    QJsonObject accountSet = m_connected.value(m_currentAccountId).toObject();
    accountSet.insert(providerKey, entry);
    m_connected.insert(m_currentAccountId, accountSet);
    saveConnected();
}

QString OAuthManager::generatePKCEVerifier() const
{
    QByteArray bytes;
    bytes.resize(32);
    QRandomGenerator::system()->fillRange(reinterpret_cast<quint32 *>(bytes.data()),
                                          bytes.size() / int(sizeof(quint32)));
    return QString::fromLatin1(bytes.toBase64(QByteArray::Base64UrlEncoding
                                              | QByteArray::OmitTrailingEquals));
}

QString OAuthManager::generatePKCEChallenge(const QString &verifier) const
{
    const QByteArray hash = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toBase64(QByteArray::Base64UrlEncoding
                                             | QByteArray::OmitTrailingEquals));
}

QString OAuthManager::redirectUri() const
{
    if (!m_server)
        return QString();
    return QStringLiteral("http://127.0.0.1:%1/callback").arg(m_server->serverPort());
}

QString OAuthManager::buildAuthUrl(Provider provider) const
{
    const ProviderInfo &info = kProviders[provider];
    const Config &cfg = m_configs[provider];
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("client_id"), cfg.clientId);
    q.addQueryItem(QStringLiteral("redirect_uri"), redirectUri());
    q.addQueryItem(QStringLiteral("state"), m_state);
    q.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    if (info.supportsPkce) {
        q.addQueryItem(QStringLiteral("code_challenge"), m_codeChallenge);
        q.addQueryItem(QStringLiteral("code_challenge_method"), QStringLiteral("S256"));
    }
    if (info.responseMode)
        q.addQueryItem(QStringLiteral("response_mode"), QLatin1String(info.responseMode));
    if (info.scopes && *info.scopes)
        q.addQueryItem(QStringLiteral("scope"), QLatin1String(info.scopes));

    if (provider == Google || provider == Drive || provider == Calendar)
        q.addQueryItem(QStringLiteral("prompt"), QStringLiteral("select_account"));

    return QString::fromLatin1(info.authUrl) + QLatin1Char('?')
        + q.toString(QUrl::FullyEncoded);
}

QString OAuthManager::decodeJwtPayload(const QByteArray &jwt) const
{
    const QList<QByteArray> parts = jwt.split('.');
    if (parts.size() < 2)
        return QString();
    return QString::fromUtf8(QByteArray::fromBase64(
        parts[1], QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

void OAuthManager::setupLocalListener()
{
    const ProviderInfo &info = kProviders[m_activeProvider];
    const Config &cfg = m_configs[m_activeProvider];
    int port = cfg.loopbackPort > 0 ? cfg.loopbackPort : info.defaultPort;

    if (!m_server) {
        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection, this, [this]() {
            while (QTcpSocket *socket = m_server->nextPendingConnection()) {
                QByteArray *buffer = new QByteArray;
                connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer]() {
                    buffer->append(socket->readAll());

                    const int headerEnd = buffer->indexOf("\r\n\r\n");
                    if (headerEnd < 0)
                        return; // wait for the full headers

                    const QByteArray head = buffer->left(headerEnd);
                    QByteArray body = buffer->mid(headerEnd + 4);

                    // Honor Content-Length for POST bodies (Apple form_post).
                    const QByteArray lower = head.toLower();
                    int contentLength = 0;
                    const int clPos = lower.indexOf("content-length:");
                    if (clPos >= 0) {
                        const int valueStart = clPos + int(strlen("content-length:"));
                        const int lineEnd = head.indexOf("\r\n", valueStart);
                        contentLength = head.mid(valueStart, lineEnd - valueStart).trimmed().toInt();
                    }
                    if (contentLength > 0 && body.size() < contentLength)
                        return; // wait for the full body

                    QString authCode;
                    QString state;
                    QString requestPath;
                    const QList<QByteArray> lines = head.split('\n');
                    if (!lines.isEmpty()) {
                        const QByteArray requestLine = lines.first().trimmed();
                        const QList<QByteArray> parts = requestLine.split(' ');
                        if (parts.size() >= 2) {
                            const QUrl url(QString::fromLatin1("http://127.0.0.1")
                                           + QString::fromLatin1(parts[1]));
                            requestPath = url.path();
                            if (parts[0] == "GET") {
                                const QUrlQuery query(url);
                                authCode = query.queryItemValue(QStringLiteral("code"));
                                state = query.queryItemValue(QStringLiteral("state"));
                            } else if (parts[0] == "POST") {
                                const QUrlQuery query(QString::fromUtf8(body));
                                authCode = query.queryItemValue(QStringLiteral("code"));
                                state = query.queryItemValue(QStringLiteral("state"));
                                const QString userJson = query.queryItemValue(QStringLiteral("user"));
                                if (!userJson.isEmpty()) {
                                    const QJsonObject user = QJsonDocument::fromJson(userJson.toUtf8()).object();
                                    if (!user.isEmpty())
                                        m_formPostUser = user;
                                }
                            }
                        }
                    }

                    // M13: the loopback port is shared and predictable, so a
                    // callback is only accepted when it targets the exact
                    // /callback path AND carries a state token that matches the
                    // active flow. Missing, wrong-path, or stale callbacks are
                    // rejected without tearing the active flow down.
                    if (requestPath != QStringLiteral("/callback")
                        || state.isEmpty()
                        || state != m_state) {
                        socket->write(errorPageHtml(QStringLiteral("Invalid OAuth callback.")));
                        socket->flush();
                        socket->disconnectFromHost();
                        delete buffer;
                        return;
                    }
                    if (authCode.isEmpty()) {
                        socket->write(errorPageHtml(QStringLiteral("No authorization code received.")));
                        socket->flush();
                        socket->disconnectFromHost();
                        fail(QStringLiteral("Provider did not return an authorization code."));
                        delete buffer;
                        return;
                    }

                    socket->write(successPageHtml());
                    socket->flush();
                    socket->disconnectFromHost();
                    delete buffer;

                    exchangeCodeForToken(authCode, m_activeProvider);
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }
    if (m_server->isListening()) {
        m_server->close();
        // Never call m_server->disconnect() here: it severs the newConnection
        // handler installed above, so the NEXT startAuth() re-entry finds a
        // listener that accepts connections but never processes them. close()
        // already clears the pending-connection queue, and any in-flight
        // socket from a previous flow is gated by the path + state check.
    }
    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        fail(QStringLiteral("Could not start the local OAuth callback listener on port %1 (is it already in use?)")
                 .arg(port));
    }
}

void OAuthManager::teardownLocalListener()
{
    if (m_server && m_server->isListening())
        m_server->close();
    m_formPostUser = QJsonObject();
    m_isAuthenticating = false;
    emit authenticatingChanged();
}

void OAuthManager::startAuth(int providerEnum)
{
    if (providerEnum < Google || providerEnum >= kProviderCount) {
        emit authFailed(QStringLiteral("Unknown provider."));
        return;
    }
    const Provider provider = static_cast<Provider>(providerEnum);

    if (m_devMode) {
        m_activeProvider = provider;
        m_isAuthenticating = true;
        emit authenticatingChanged();
        QJsonObject profile;
        profile.insert(QStringLiteral("provider"), QLatin1String(kProviders[provider].key));
        profile.insert(QStringLiteral("name"), QStringLiteral("Ankar (Dev)"));
        profile.insert(QStringLiteral("email"), QStringLiteral("ankar.dev@blackhole.local"));
        if (provider == Google || provider == Drive || provider == Calendar)
            profile.insert(QStringLiteral("picture"), QString());
        if (provider == Microsoft)
            profile.insert(QStringLiteral("avatar"), QString());
        finishConnection(QString(), QString(), provider, profile, 3600);
        return;
    }

    if (!m_configs[provider].enabled()) {
        emit authFailed(QStringLiteral("%1 is not configured. Add a client_id to oauth.json first.")
                            .arg(QLatin1String(kProviders[provider].displayName)));
        return;
    }

    m_activeProvider = provider;
    m_formPostUser = QJsonObject();
    const ProviderInfo &info = kProviders[provider];
    if (info.supportsPkce) {
        m_codeVerifier = generatePKCEVerifier();
        m_codeChallenge = generatePKCEChallenge(m_codeVerifier);
    } else {
        m_codeVerifier.clear();
        m_codeChallenge.clear();
    }
    m_state = QString::number(QRandomGenerator::system()->generate64(), 16);

    setupLocalListener();
    if (!m_server || !m_server->isListening()) {
        emit authFailed(QStringLiteral("Could not start the local OAuth callback listener."));
        return;
    }

    m_isAuthenticating = true;
    emit authenticatingChanged();

    if (!openBrowser(QUrl(buildAuthUrl(provider)))) {
        teardownLocalListener();
        emit authFailed(QStringLiteral("Could not open the provider sign-in page."));
    }
}

void OAuthManager::cancelAuth()
{
    teardownLocalListener();
}

void OAuthManager::fail(const QString &message)
{
    teardownLocalListener();
    emit authFailed(message);
}

void OAuthManager::finishSuccess(const QJsonObject &profile)
{
    teardownLocalListener();
    emit authSuccess(profile);
}

bool OAuthManager::openBrowser(const QUrl &url) const
{
    return QDesktopServices::openUrl(url);
}

void OAuthManager::exchangeCodeForToken(const QString &code, Provider provider)
{
    const ProviderInfo &info = kProviders[provider];
    const Config &cfg = m_configs[provider];
    const QUrl tokenUrl(QLatin1String(info.tokenUrl));

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
    params.addQueryItem(QStringLiteral("code"), code);
    params.addQueryItem(QStringLiteral("redirect_uri"), redirectUri());
    if (info.supportsPkce)
        params.addQueryItem(QStringLiteral("code_verifier"), m_codeVerifier);

    params.addQueryItem(QStringLiteral("client_id"), cfg.clientId);
    if (info.needsSecret && !cfg.clientSecret.isEmpty())
        params.addQueryItem(QStringLiteral("client_secret"), cfg.clientSecret);
    if (provider == Microsoft)
        params.addQueryItem(QStringLiteral("scope"), QLatin1String(info.scopes));

    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));
    if (provider == GitHub)
        request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_net.post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply, provider, info]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            const QByteArray raw = reply->readAll();
            QString detail;
            const QJsonDocument doc = QJsonDocument::fromJson(raw);
            if (doc.isObject()) {
                const QJsonObject o = doc.object();
                detail = o.value(QStringLiteral("error_description")).toString(
                    o.value(QStringLiteral("error")).toString());
            }
            fail(QStringLiteral("Token exchange failed: %1%2")
                     .arg(reply->errorString(),
                          detail.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(detail)));
            return;
        }
        const QByteArray raw = reply->readAll();
        const QJsonObject json = QJsonDocument::fromJson(raw).object();
        const QString accessToken = json.value(QStringLiteral("access_token")).toString();
        const QString refreshToken = json.value(QStringLiteral("refresh_token")).toString();
        const int expiresIn = json.value(QStringLiteral("expires_in")).toInt(3600);

        // Store the refresh token for use in profile fetch
        m_pendingRefreshToken = refreshToken;

        if (provider == Apple) {
            // Apple has no profile API: name/email live in the id_token and the
            // first-time form_post "user" payload.
            const QJsonObject user = m_formPostUser;
            const QJsonObject nameObj = user.value(QStringLiteral("name")).toObject();
            const QString firstName = nameObj.value(QStringLiteral("firstName")).toString();
            const QString lastName = nameObj.value(QStringLiteral("lastName")).toString();
            const QString userEmail = user.value(QStringLiteral("email")).toString();

            const QString payload = decodeJwtPayload(
                json.value(QStringLiteral("id_token")).toString().toUtf8());
            const QJsonObject claims = QJsonDocument::fromJson(payload.toUtf8()).object();
            const QString claimEmail = claims.value(QStringLiteral("email")).toString();

            QString displayName = QStringLiteral("%1 %2").arg(firstName, lastName).trimmed();
            if (displayName.isEmpty())
                displayName = claimEmail;
            const QString email = userEmail.isEmpty() ? claimEmail : userEmail;

            QJsonObject userInfo;
            userInfo.insert(QStringLiteral("name"), displayName);
            userInfo.insert(QStringLiteral("email"), email);
            finishConnection(accessToken, refreshToken, Apple, userInfo, expiresIn);
            return;
        }

        if (provider == Notion) {
            // Notion has no profile endpoint; the token response carries the
            // workspace identity the user authorized.
            QJsonObject userInfo;
            userInfo.insert(QStringLiteral("name"),
                            json.value(QStringLiteral("workspace_name")).toString());
            finishConnection(accessToken, refreshToken, Notion, userInfo, expiresIn);
            return;
        }

        if (accessToken.isEmpty()) {
            fail(QStringLiteral("No access token in the provider response."));
            return;
        }
        fetchUserProfile(accessToken, refreshToken, provider);
    });
}

void OAuthManager::fetchUserProfile(const QString &accessToken, const QString &refreshToken, Provider provider)
{
    const ProviderInfo &info = kProviders[provider];
    if (!info.profileUrl || !*info.profileUrl) {
        finishConnection(accessToken, refreshToken, provider, QJsonObject(), 3600);
        return;
    }

    const QUrl profileUrl(QLatin1String(info.profileUrl));
    QNetworkRequest request(profileUrl);
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(accessToken).toUtf8());

    if (provider == Dropbox) {
        // Dropbox profile endpoint is an RPC: POST with an empty JSON body.
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        QNetworkReply *reply = m_net.post(request, QByteArrayLiteral("{}"));
        connect(reply, &QNetworkReply::finished, this,
                [this, reply, accessToken, refreshToken, provider]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                fail(QStringLiteral("Profile fetch failed: %1").arg(reply->errorString()));
                return;
            }
            const QJsonObject raw = QJsonDocument::fromJson(reply->readAll()).object();
            const QJsonObject name = raw.value(QStringLiteral("name")).toObject();
            QJsonObject userInfo;
            userInfo.insert(QStringLiteral("name"), name.value(QStringLiteral("display_name")).toString());
            userInfo.insert(QStringLiteral("email"), raw.value(QStringLiteral("email")).toString());
            userInfo.insert(QStringLiteral("login"), name.value(QStringLiteral("abbreviated_name")).toString());
            finishConnection(accessToken, refreshToken, Dropbox, userInfo, 3600);
        });
        return;
    }

    if (provider == GitHub) {
        request.setRawHeader("Accept", "application/vnd.github+json");
        request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
        QNetworkReply *reply = m_net.get(request);
        connect(reply, &QNetworkReply::finished, this,
                [this, reply, accessToken, refreshToken, profileUrl]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                fail(QStringLiteral("Profile fetch failed: %1").arg(reply->errorString()));
                return;
            }
            const QJsonObject raw = QJsonDocument::fromJson(reply->readAll()).object();
            QJsonObject userInfo;
            userInfo.insert(QStringLiteral("name"), raw.value(QStringLiteral("name")).toString());
            userInfo.insert(QStringLiteral("email"), raw.value(QStringLiteral("email")).toString());
            userInfo.insert(QStringLiteral("login"), raw.value(QStringLiteral("login")).toString());
            userInfo.insert(QStringLiteral("avatar"), raw.value(QStringLiteral("avatar_url")).toString());

            // email is null when the address is private; fetch it from /user/emails.
            if (userInfo.value(QStringLiteral("email")).toString().isEmpty()) {
                QNetworkRequest emails(profileUrl.resolved(QUrl(QStringLiteral("emails"))));
                emails.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(accessToken).toUtf8());
                emails.setRawHeader("Accept", "application/vnd.github+json");
                emails.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
                QNetworkReply *emailsReply = m_net.get(emails);
                connect(emailsReply, &QNetworkReply::finished, this,
                        [this, emailsReply, accessToken, refreshToken, userInfo]() {
                    emailsReply->deleteLater();
                    if (emailsReply->error() == QNetworkReply::NoError) {
                        const QJsonArray arr = QJsonDocument::fromJson(emailsReply->readAll()).array();
                        for (const QJsonValue &v : arr) {
                            const QJsonObject e = v.toObject();
                            if (e.value(QStringLiteral("primary")).toBool(false)) {
                                QJsonObject merged = userInfo;
                                merged.insert(QStringLiteral("email"),
                                              e.value(QStringLiteral("email")).toString());
                                finishConnection(accessToken, refreshToken, GitHub, merged, 3600);
                                return;
                            }
                        }
                    }
                    finishConnection(accessToken, refreshToken, GitHub, userInfo, 3600);
                });
                return;
            }
            finishConnection(accessToken, QString(), GitHub, userInfo, 3600);
        });
        return;
    }

    QNetworkReply *reply = m_net.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, accessToken, refreshToken, provider]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            fail(QStringLiteral("Profile fetch failed: %1").arg(reply->errorString()));
            return;
        }
        const QJsonObject raw = QJsonDocument::fromJson(reply->readAll()).object();
        QJsonObject userInfo;
        switch (provider) {
        case Google:
        case Drive:
        case Calendar:
            userInfo.insert(QStringLiteral("name"), raw.value(QStringLiteral("name")).toString());
            userInfo.insert(QStringLiteral("email"), raw.value(QStringLiteral("email")).toString());
            userInfo.insert(QStringLiteral("avatar"), raw.value(QStringLiteral("picture")).toString());
            break;
        case Microsoft:
            userInfo.insert(QStringLiteral("name"), raw.value(QStringLiteral("displayName")).toString());
            userInfo.insert(QStringLiteral("email"), raw.value(QStringLiteral("userPrincipalName")).toString());
            break;
        case Slack:
            userInfo.insert(QStringLiteral("name"), raw.value(QStringLiteral("name")).toString());
            userInfo.insert(QStringLiteral("email"), raw.value(QStringLiteral("email")).toString());
            userInfo.insert(QStringLiteral("avatar"), raw.value(QStringLiteral("picture")).toString());
            break;
        case Discord:
            userInfo.insert(QStringLiteral("name"),
                            QStringLiteral("%1#%2")
                                .arg(raw.value(QStringLiteral("username")).toString(),
                                     raw.value(QStringLiteral("discriminator")).toString()));
            userInfo.insert(QStringLiteral("email"), raw.value(QStringLiteral("email")).toString());
            {
                const QString id = raw.value(QStringLiteral("id")).toString();
                const QString hash = raw.value(QStringLiteral("avatar")).toString();
                if (!id.isEmpty() && !hash.isEmpty())
                    userInfo.insert(QStringLiteral("avatar"),
                                    QStringLiteral("https://cdn.discordapp.com/avatars/%1/%2.png")
                                        .arg(id, hash));
            }
            break;
        default:
            break;
        }
        finishConnection(accessToken, refreshToken, provider, userInfo, 3600);
    });
}

void OAuthManager::finishConnection(const QString &accessToken, const QString &refreshToken,
                                    Provider provider, const QJsonObject &user, int expiresIn)
{
    const ProviderInfo &info = kProviders[provider];
    const QString key = QLatin1String(info.key);

    // Persist the token set (encrypted) so later features can call the APIs.
    QJsonObject entry;
    entry.insert(QStringLiteral("refreshToken"), refreshToken);
    entry.insert(QStringLiteral("accessToken"), accessToken);
    entry.insert(QStringLiteral("scopes"), QLatin1String(info.scopes ? info.scopes : ""));
    entry.insert(QStringLiteral("expiresIn"), expiresIn);
    entry.insert(QStringLiteral("expiresAt"), QDateTime::currentMSecsSinceEpoch() + expiresIn * 1000);
    entry.insert(QStringLiteral("user"), user);
    setConnectedEntry(key, entry);

    QJsonObject profile = user;
    profile.insert(QStringLiteral("provider"), key);
    profile.insert(QStringLiteral("service"), QLatin1String(info.displayName));
    finishSuccess(profile);
}

void OAuthManager::loadConnected()
{
    QFile file(connectedFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QByteArray data = file.readAll();
    QByteArray plain;
    if (VaultCrypto::isEnvelope(data)) {
        plain = VaultCrypto::decrypt(data);
        if (plain.isEmpty()) {
            // The services file exists but failed authentication (tampered, or
            // the master key no longer matches). Fail closed: never let a later
            // save mistake this for an empty connection set and overwrite the
            // real encrypted tokens.
            m_decryptFailed = true;
            emit decryptFailedChanged();
            return;
        }
    } else {
        plain = data; // legacy plaintext (nothing shipped, keep tolerant)
    }

    const QJsonDocument doc = QJsonDocument::fromJson(plain);
    if (doc.isObject()) {
        const QJsonObject root = doc.object();
        // New shape: accountId -> { providerKey -> entry }.
        // Old flat shape: providerKey -> entry. Detect by checking whether any
        // value itself looks like a provider-set object.
        bool flat = false;
        for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
            const QJsonObject v = it.value().toObject();
            if (v.contains(QStringLiteral("refreshToken")) || v.contains(QStringLiteral("accessToken"))) {
                flat = true;
                break;
            }
        }
        if (flat) {
            // Migrate legacy flat layout into the "default" account bucket.
            QJsonObject migrated;
            migrated.insert(QStringLiteral("default"), root);
            m_connected = migrated;
        } else {
            m_connected = root;
        }
    } else if (doc.isArray()) {
        // Tolerant of an array-of-entries legacy shape.
        QJsonObject accountSet;
        for (const QJsonValue &v : doc.array()) {
            const QJsonObject o = v.toObject();
            const QString key = o.value(QStringLiteral("provider")).toString();
            if (!key.isEmpty())
                accountSet.insert(key, o);
        }
        QJsonObject migrated;
        migrated.insert(QStringLiteral("default"), accountSet);
        m_connected = migrated;
    } else {
        // File exists but holds neither an object nor an array. Treat it as
        // unreadable so a later save cannot clobber it with an empty set.
        m_decryptFailed = true;
        emit decryptFailedChanged();
    }
}

void OAuthManager::saveConnected() const
{
    if (m_decryptFailed)
        return; // never overwrite a token set we failed to authenticate
    const QByteArray plain = QJsonDocument(m_connected).toJson(QJsonDocument::Compact);
    const QByteArray blob = VaultCrypto::encrypt(plain);
    if (blob.isEmpty())
        return; // master key unavailable: never overwrite the vault with garbage
    if (!OSPaths::writeFileAtomic(connectedFilePath(), blob)) {
        qWarning() << "Failed to save OAuth connections";
        return;
    }
}

QString OAuthManager::getValidAccessToken(int providerEnum)
{
    if (providerEnum < Google || providerEnum >= kProviderCount)
        return QString();
    const Provider provider = static_cast<Provider>(providerEnum);
    const QString key = QLatin1String(kProviders[provider].key);
    QJsonObject accountSet = connectedForCurrentAccount();
    if (!accountSet.contains(key))
        return QString();
    QJsonObject entry = accountSet.value(key).toObject();

    // Check if current access token is still valid (with 5-minute buffer)
    const qint64 expiresAt = entry.value(QStringLiteral("expiresAt")).toVariant().toLongLong();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 buffer = 5 * 60 * 1000; // 5 minutes
    const QString accessToken = entry.value(QStringLiteral("accessToken")).toString();
    if (!accessToken.isEmpty() && now < (expiresAt - buffer)) {
        return accessToken; // still valid
    }

    // Need to refresh - check if we have a refresh token
    const QString refreshToken = entry.value(QStringLiteral("refreshToken")).toString();
    if (refreshToken.isEmpty())
        return QString(); // no refresh token, cannot refresh

    // Need to refresh
    QString newAccessToken = refreshAccessToken(refreshToken, provider);
    if (newAccessToken.isEmpty())
        return QString();

    // Update the stored token with new access token and expiry
    entry.insert(QStringLiteral("accessToken"), newAccessToken);
    // Use expires_in from provider if available, else default to 1 hour
    const int expiresIn = entry.value(QStringLiteral("expiresIn")).toInt(3600);
    entry.insert(QStringLiteral("expiresAt"), QDateTime::currentMSecsSinceEpoch() + expiresIn * 1000);
    accountSet.insert(key, entry);
    m_connected.insert(m_currentAccountId, accountSet);
    saveConnected();
    return newAccessToken;
}

QString OAuthManager::refreshAccessToken(const QString &refreshToken, Provider provider)
{
    const ProviderInfo &info = kProviders[provider];
    const Config &cfg = m_configs[provider];
    const QUrl tokenUrl(QLatin1String(info.tokenUrl));

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("refresh_token"));
    params.addQueryItem(QStringLiteral("refresh_token"), refreshToken);
    params.addQueryItem(QStringLiteral("client_id"), cfg.clientId);
    if (info.needsSecret && !cfg.clientSecret.isEmpty())
        params.addQueryItem(QStringLiteral("client_secret"), cfg.clientSecret);

    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));
    if (provider == GitHub)
        request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_net.post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return QString();
    }
    const QByteArray raw = reply->readAll();
    reply->deleteLater();
    const QJsonObject json = QJsonDocument::fromJson(raw).object();
    const QString accessToken = json.value(QStringLiteral("access_token")).toString();
    const QString newRefreshToken = json.value(QStringLiteral("refresh_token")).toString();
    const int expiresIn = json.value(QStringLiteral("expires_in")).toInt(3600);

    // Update the entry with new tokens
    QJsonObject accountSet = connectedForCurrentAccount();
    const QString key = QLatin1String(kProviders[provider].key);
    QJsonObject entry = accountSet.value(key).toObject();
    entry.insert(QStringLiteral("accessToken"), accessToken);
    if (!newRefreshToken.isEmpty())
        entry.insert(QStringLiteral("refreshToken"), newRefreshToken);
    entry.insert(QStringLiteral("expiresIn"), expiresIn);
    entry.insert(QStringLiteral("expiresAt"), QDateTime::currentMSecsSinceEpoch() + expiresIn * 1000);
    accountSet.insert(key, entry);
    m_connected.insert(m_currentAccountId, accountSet);
    saveConnected();

    return accessToken;
}
