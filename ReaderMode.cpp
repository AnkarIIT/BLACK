#include "ReaderMode.h"

#include <QWebEngineView>
#include <QStringList>

namespace {

const int kMinFontSize = 12;
const int kMaxFontSize = 32;
const double kMinLineHeight = 1.2;
const double kMaxLineHeight = 2.5;

} // namespace

ReaderMode::ReaderMode(QObject *parent)
    : QObject(parent)
{
    // Placeholder: a real implementation would kick off readability detection
    // on the current page. Nothing is started here.
}

ReaderMode::~ReaderMode() = default;

bool ReaderMode::isInReaderMode(QWebEngineView *view) const
{
    if (!view)
        return false;
    // Placeholder: m_activeViews stays empty because toggleReaderMode() can
    // never add a view (extraction is unavailable).
    return m_activeViews.contains(view);
}

void ReaderMode::toggleReaderMode(QWebEngineView *view)
{
    if (!view)
        return;

    if (m_activeViews.contains(view)) {
        // Defensive: unreachable in this build, but if a view were somehow
        // marked active, clearing it is the honest "leave reader mode" action.
        // No page reload happens because nothing was rewritten in the first place.
        m_activeViews.remove(view);
        emit readerModeToggled(false);
        return;
    }

    // Placeholder: entering reader mode requires an ArticleContent, which
    // requires extraction, which is not implemented. Say so instead of
    // pretending the page was converted.
    emit extractionFailed(
        QStringLiteral("Reader mode article extraction is not implemented in this build."));
}

bool ReaderMode::extract(QWebEngineView *view)
{
    if (!view)
        return false;

    // Placeholder: a real implementation would runJavaScript() a readability
    // bundle over the page's DOM. Nothing is injected and nothing is read.
    emit extractionFailed(
        QStringLiteral("Reader mode article extraction is not implemented in this build."));
    return false;
}

int ReaderMode::fontSize() const
{
    return m_fontSize;
}

void ReaderMode::setFontSize(int size)
{
    if (size < kMinFontSize)
        size = kMinFontSize;
    else if (size > kMaxFontSize)
        size = kMaxFontSize;
    m_fontSize = size;
}

bool ReaderMode::darkMode() const
{
    return m_darkMode;
}

void ReaderMode::setDarkMode(bool dark)
{
    m_darkMode = dark;
}

QString ReaderMode::fontFamily() const
{
    return m_fontFamily;
}

void ReaderMode::setFontFamily(const QString &family)
{
    if (family.isEmpty())
        return;
    m_fontFamily = family;
}

double ReaderMode::lineHeight() const
{
    return m_lineHeight;
}

void ReaderMode::setLineHeight(double height)
{
    if (height < kMinLineHeight)
        height = kMinLineHeight;
    else if (height > kMaxLineHeight)
        height = kMaxLineHeight;
    m_lineHeight = height;
}

QString ReaderMode::readerModeStylesheet(bool dark)
{
    // Real string formatting only: this renders CSS for an already-extracted
    // article. It performs no extraction and touches no page.
    if (dark) {
        return QStringLiteral(
            "html { background-color: #1a1a1a; color: #e8e8e8; }\n"
            "body { font-family: Georgia, serif; color: #e8e8e8;\n"
            "       background-color: #1a1a1a; }\n"
            "a { color: #7aa2f7; }\n"
            "h1, h2, h3 { color: #f2f2f2; }\n"
            ".site-name { color: #9a9a9a; }\n"
            ".byline, .date { color: #9a9a9a; }\n");
    }

    return QStringLiteral(
        "html { background-color: #ffffff; color: #1a1a1a; }\n"
        "body { font-family: Georgia, serif; color: #1a1a1a;\n"
        "       background-color: #ffffff; }\n"
        "a { color: #0b5fd0; }\n"
        "h1, h2, h3 { color: #111111; }\n"
        ".site-name { color: #6b6b6b; }\n"
        ".byline, .date { color: #6b6b6b; }\n");
}

namespace {

// Shared renderer. `extraCss` is appended inside the <style> block of <head>,
// so callers can layer per-instance typography on top of the theme.
QString renderArticleDocument(const ArticleContent &article, bool dark,
                              const QString &extraCss)
{
    const QString dir = (article.dir == QLatin1String("rtl")) ? QStringLiteral("rtl")
                                                              : QStringLiteral("ltr");
    const QString bg = dark ? QStringLiteral("#1a1a1a") : QStringLiteral("#ffffff");
    const QString fg = dark ? QStringLiteral("#e8e8e8") : QStringLiteral("#1a1a1a");
    const QString meta = dark ? QStringLiteral("#9a9a9a") : QStringLiteral("#6b6b6b");

    QStringList headerParts;
    if (!article.siteName.isEmpty())
        headerParts << QStringLiteral("<span class=\"site-name\">%1</span>")
                           .arg(article.siteName.toHtmlEscaped());
    if (!article.byline.isEmpty())
        headerParts << QStringLiteral("<span class=\"byline\">%1</span>")
                           .arg(article.byline.toHtmlEscaped());
    else if (!article.author.isEmpty())
        headerParts << QStringLiteral("<span class=\"byline\">%1</span>")
                           .arg(article.author.toHtmlEscaped());
    if (!article.date.isEmpty())
        headerParts << QStringLiteral("<span class=\"date\">%1</span>")
                           .arg(article.date.toHtmlEscaped());

    QString html;
    html += QStringLiteral("<!DOCTYPE html>\n<html dir=\"%1\">\n<head>\n").arg(dir);
    html += QStringLiteral("<meta charset=\"utf-8\">\n");
    html += QStringLiteral("<title>%1</title>\n").arg(article.title.toHtmlEscaped());
    html += QStringLiteral("<style>\n");
    html += readerModeStylesheet(dark);
    html += QStringLiteral("body { margin: 0 auto; max-width: 42rem; padding: 2rem 1rem; }\n");
    html += QStringLiteral(".article-header { margin-bottom: 2rem; }\n");
    html += QStringLiteral(".article-header .site-name, .article-header .byline,\n"
                           ".article-header .date { margin-right: 0.75rem; font-size: 0.85rem; color: %1; }\n")
                .arg(meta);
    html += QStringLiteral("h1 { font-size: 2rem; line-height: 1.25; margin: 0.5rem 0; }\n");
    html += QStringLiteral(".excerpt { font-style: italic; color: %1; }\n").arg(meta);
    html += extraCss;
    html += QStringLiteral("</style>\n</head>\n");
    html += QStringLiteral("<body style=\"background-color:%1;color:%2\">\n").arg(bg, fg);

    html += QStringLiteral("<header class=\"article-header\">\n");
    if (!headerParts.isEmpty())
        html += QStringLiteral("<div class=\"meta\">%1</div>\n")
                    .arg(headerParts.join(QStringLiteral(" &middot; ")));
    html += QStringLiteral("<h1>%1</h1>\n").arg(article.title.toHtmlEscaped());
    if (!article.excerpt.isEmpty())
        html += QStringLiteral("<p class=\"excerpt\">%1</p>\n").arg(article.excerpt.toHtmlEscaped());
    html += QStringLiteral("</header>\n");

    html += QStringLiteral("<article class=\"content\">\n");
    html += article.content;
    html += QStringLiteral("\n</article>\n");

    html += QStringLiteral("<footer>\n");
    if (article.url.isValid())
        html += QStringLiteral("<p><a href=\"%1\">%1</a></p>\n")
                    .arg(article.url.toString().toHtmlEscaped());
    html += QStringLiteral("</footer>\n");

    html += QStringLiteral("</body>\n</html>\n");
    return html;
}

} // namespace

QString ReaderMode::readerModeTemplate(const ArticleContent &article, bool dark)
{
    // Real string formatting only. The caller must supply an already-populated
    // ArticleContent; this function never produces one.
    return renderArticleDocument(article, dark, QString());
}

QString ReaderMode::buildReaderModeHTML(const ArticleContent &article) const
{
    // Real string formatting only: renders the document using this instance's
    // presentation settings. It still does NOT extract anything.
    QString extraCss;
    if (!m_fontFamily.isEmpty()) {
        extraCss += QStringLiteral("body { font-family: %1; font-size: %2px; line-height: %3; }\n")
                        .arg(m_fontFamily, QString::number(m_fontSize), QString::number(m_lineHeight));
    }
    return renderArticleDocument(article, m_darkMode, extraCss);
}