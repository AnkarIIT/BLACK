#ifndef OAUTHMANAGER_H
#define OAUTHMANAGER_H

#include <QObject>
#include <QTcpServer>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonArray>

// OAuth 2.0 (PKCE where supported) for the first-run onboarding page.
//
// Provider slots:
//   0-2  Sign-in providers: Google, Apple, Microsoft. On success the verified
//        name/email/avatar are passed to Account (stored locally).
//   3-9  Service connections: GitHub, Slack, Discord, Google Drive, Google
//        Calendar, Dropbox, Notion. On success the access/refresh tokens are
//        stored in an encrypted local envelope (services.json, VaultCrypto)
//        so future features can really call the provider APIs.
//
// There is no cloud backend; every token and profile field stays on-device.
//
// Real provider credentials are required before anything can work. They are
// read from <AppDataLocation>/oauth.json; providers without a configured
// client_id report isConfigured() == false and the onboarding page hides their
// button. Nothing is shown as enabled that is not actually wired up.
//
// Dev mock mode:
//   {
//     "dev_mode": true,
//     "google":    { "clientId": "MOCK_GOOGLE_CLIENT_ID" },
//     "apple":     { "clientId": "MOCK_APPLE_CLIENT_ID" },
//     "microsoft": { "clientId": "MOCK_MICROSOFT_CLIENT_ID" }
//   }
// When dev_mode is true, startAuth() emits a deterministic mock profile instead
// of opening a browser. The rest of onboarding can be tested end-to-end.
//
//   {
//     "google":    { "clientId": "....apps.googleusercontent.com" },
//     "microsoft": { "clientId": "...." },
//     "apple":     { "clientId": "....", "clientSecret": "<ES256 JWT>" },
//     "github":    { "clientId": "....", "clientSecret": "...." },
//     "slack":     { "clientId": "....", "port": 9011 },
//     "discord":   { "clientId": "....", "port": 9012 },
//     "drive":     { "clientId": "....apps.googleusercontent.com" },
//     "calendar":  { "clientId": "....apps.googleusercontent.com" },
//     "dropbox":   { "clientId": "....", "port": 9013 },
//     "notion":    { "clientId": "....", "port": 9014 }
//   }
//
// Redirect URI: http://127.0.0.1:<port>/callback. Google/Microsoft ignore the
// loopback port, so they use an ephemeral port. GitHub/Slack/Discord/Dropbox/
// Notion require the exact redirect URI to be pre-registered, so they bind a
// fixed default port (override with "port"). Apple additionally needs a signed
// client_secret (ES256 JWT) minted from your Sign In with Apple key.
class OAuthManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isAuthenticating READ isAuthenticating NOTIFY authenticatingChanged)
    Q_PROPERTY(bool decryptFailed READ decryptFailed NOTIFY decryptFailedChanged)

public:
    enum Provider {
        Google = 0,
        Apple = 1,
        Microsoft = 2,
        GitHub = 3,
        Slack = 4,
        Discord = 5,
        Drive = 6,
        Calendar = 7,
        Dropbox = 8,
        Notion = 9
    };
    Q_ENUM(Provider)

    static constexpr int kProviderCount = 10;
    static constexpr int kFirstService = 3; // indices >= this are "services"

    explicit OAuthManager(QObject *parent = nullptr);

    bool isAuthenticating() const { return m_isAuthenticating; }

    // True when services.json exists but failed to decrypt/parse (tampered, or
    // the master key no longer matches). Distinct from an empty/nonexistent
    // token set: while true, saveConnected() refuses to write so a "failed to
    // decrypt" is never mistaken for "no connections" and overwritten.
    bool decryptFailed() const { return m_decryptFailed; }

    Q_INVOKABLE bool devMode() const { return m_devMode; }

    // The account id whose service connections isConnected()/startAuth() apply
    // to. Empty = the default (global) connection set.
    Q_INVOKABLE void setCurrentAccountId(const QString &accountId) { m_currentAccountId = accountId; }
    Q_INVOKABLE QString currentAccountId() const { return m_currentAccountId; }

    // True when the provider has a configured client_id, i.e. its button may
    // be shown. Keep this honest: no client_id, no button.
    Q_INVOKABLE bool isConfigured(int provider) const;

    // True when the provider has a stored (encrypted) token set from a
    // previous successful authorization, scoped to the current account.
    Q_INVOKABLE bool isConnected(int provider) const;

    // Array of { "provider", "name", "user" } for every service slot that has
    // a stored token set for the current account, so the page can pre-check
    // already-connected services.
    Q_INVOKABLE QJsonArray connectedServicesJson() const;

    // Returns a valid access token for the provider, refreshing if necessary.
    // Returns empty string if no token stored, refresh failed, or not configured.
    Q_INVOKABLE QString getValidAccessToken(int providerEnum);

    // Starts the authorization flow for providerEnum. Opens the provider
    // consent page in the system browser and waits for the loopback redirect.
    // Emits authSuccess(QJsonObject) or authFailed(QString).
    Q_INVOKABLE void startAuth(int providerEnum);
    Q_INVOKABLE void cancelAuth();

signals:
    void authenticatingChanged();
    void authSuccess(const QJsonObject &userProfile);
    void authFailed(const QString &errorMessage);
    void decryptFailedChanged();

private:
    friend class TestOAuthManager;
    struct Config {
        QString clientId;
        QString clientSecret;
        int loopbackPort = 0; // 0 = ephemeral; strict providers need a fixed port
        bool enabled() const { return !clientId.isEmpty(); }
    };

    struct ProviderInfo {
        const char *key;        // oauth.json key and profile "provider" value
        const char *displayName;
        const char *authUrl;
        const char *tokenUrl;
        const char *profileUrl; // nullptr = no profile fetch (Apple, Notion)
        const char *scopes;
        bool needsSecret;
        bool supportsPkce;
        const char *responseMode; // nullptr = GET query; "form_post" for Apple
        int defaultPort;          // 0 = ephemeral (loopback port ignored)
    };

    static const ProviderInfo kProviders[kProviderCount];

    void loadConfig();
    const Config &config(Provider provider) const;
    QString redirectUri() const;
    QString buildAuthUrl(Provider provider) const;
    QString generatePKCEVerifier() const;
    QString generatePKCEChallenge(const QString &verifier) const;
    QString decodeJwtPayload(const QByteArray &jwt) const;

    void setupLocalListener();
    void teardownLocalListener();
    bool openBrowser(const QUrl &url) const;

    void exchangeCodeForToken(const QString &code, Provider provider);
    void fetchUserProfile(const QString &accessToken, Provider provider);
    void finishConnection(const QString &accessToken, const QString &refreshToken,
                          Provider provider, const QJsonObject &user, int expiresIn = 3600);

    void loadConnected();
    void saveConnected() const;

    // The { provider key -> entry } object for the current account.
    QJsonObject connectedForCurrentAccount() const;
    void setConnectedEntry(const QString &providerKey, const QJsonObject &entry);

    QString refreshAccessToken(const QString &refreshToken, Provider provider);

    void fail(const QString &message);
    void finishSuccess(const QJsonObject &profile);

    Config m_configs[kProviderCount];
    // accountId -> { provider key -> {refreshToken, accessToken, scopes, user} }.
    // Empty accountId is the "default" (legacy/global) connection set.
    QJsonObject m_connected;
    QString m_currentAccountId;
    QNetworkAccessManager m_net;
    QTcpServer *m_server = nullptr;

    QString m_codeVerifier;
    QString m_codeChallenge;
    QString m_state;
    QJsonObject m_formPostUser; // Apple form_post "user" payload
    QString m_pendingRefreshToken; // Refresh token from token exchange, used during profile fetch
    Provider m_activeProvider = Google;
    bool m_isAuthenticating = false;
    bool m_devMode = false;
    bool m_decryptFailed = false;
};

#endif // OAUTHMANAGER_H
