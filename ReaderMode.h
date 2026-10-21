#ifndef READERMODE_H
#define READERMODE_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QUrl>

class QWebEngineView;

// The subset of article metadata a readability extractor would produce.
// Populated by extract(); in this build no extraction ever runs, so a caller
// that builds one by hand (tests do) is the only source of these values.
struct ArticleContent
{
    QString title;
    QString byline;
    QString content;
    QString textContent;
    QString excerpt;
    QString dir;        // "ltr" or "rtl"
    QString siteName;
    QUrl url;
    QString date;
    QString author;
    int length = 0;    // Estimated word count.
};

// Placeholder implementation. The readability/Readability article EXTRACTION
// feature is not implemented in this build; extraction calls are no-ops that
// fail explicitly.
//
// What DOES work, because it is pure string formatting and no browser
// interaction:
//   * readerModeStylesheet() / readerModeTemplate() / buildReaderModeHTML()
//   * the font size / dark mode / font family / line height accessors and
//     their clamping
//
// What does NOT work:
//   * extract() never reads a page. It emits extractionFailed() and returns
//     false; it never populates an ArticleContent.
//   * toggleReaderMode() therefore cannot enter reader mode. It only ever emits
//     extractionFailed(). isInReaderMode() is always false and the active-view
//     set is always empty.
//   * No page is ever reloaded, rewritten or mutated.
class ReaderMode : public QObject
{
    Q_OBJECT

public:
    explicit ReaderMode(QObject *parent = nullptr);
    ~ReaderMode() override;

    // Always false in this build: no view is ever actually in reader mode.
    bool isInReaderMode(QWebEngineView *view) const;

    // Placeholder: cannot enter reader mode without extraction, so this emits
    // extractionFailed() and leaves the page untouched. A null view is ignored.
    void toggleReaderMode(QWebEngineView *view);

    // Placeholder: always returns false and emits extractionFailed(). No
    // JavaScript readability probe is ever injected.
    Q_INVOKABLE bool extract(QWebEngineView *view);

    // Real (string-only): renders the reader stylesheet for the given theme.
    // Does not touch any page.
    static QString readerModeStylesheet(bool dark);

    // Real (string-only): renders a complete HTML document around an already
    // populated ArticleContent. Does not extract anything.
    static QString readerModeTemplate(const ArticleContent &article, bool dark);

    // Real (string-only): same as readerModeTemplate() using this instance's
    // current font size, font family, line height and dark-mode setting.
    QString buildReaderModeHTML(const ArticleContent &article) const;

    // Presentation state. Genuinely stored, and used by buildReaderModeHTML().
    int fontSize() const;          // 12..32, default 18
    void setFontSize(int size);    // Clamped to 12..32.

    bool darkMode() const;
    void setDarkMode(bool dark);

    QString fontFamily() const;    // Default "Georgia, serif"
    void setFontFamily(const QString &family);

    double lineHeight() const;     // 1.2..2.5, default 1.6
    void setLineHeight(double height); // Clamped to 1.2..2.5.

signals:
    // Emitted when extraction is attempted and cannot be performed. Never
    // emitted with a success meaning.
    void extractionFailed(const QString &error);

    // Emitted when a view enters or leaves reader mode. Never emitted by this
    // build, because entering is impossible.
    void readerModeToggled(bool enabled);

    // Emitted after a successful extraction. Never emitted by this build.
    void articleExtracted(const QString &title, const QString &html);

private:
    // Views believed to be showing reader-mode content. Always empty here, but
    // kept so the ownership/lookup logic is explicit and correct.
    QSet<QWebEngineView *> m_activeViews;

    int m_fontSize = 18;
    bool m_darkMode = false;
    QString m_fontFamily = QStringLiteral("Georgia, serif");
    double m_lineHeight = 1.6;
};

#endif // READERMODE_H
