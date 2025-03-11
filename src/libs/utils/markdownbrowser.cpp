// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdownbrowser.h"

#include "algorithm.h"
#include "async.h"
#include "mimeutils.h"
#include "movie.h"
#include "networkaccessmanager.h"
#include "stringutils.h"
#include "stylehelper.h"
#include "textutils.h"
#include "theme/theme.h"
#include "utilsicons.h"

#include <solutions/tasking/networkquery.h>
#include <solutions/tasking/tasktree.h>
#include <solutions/tasking/tasktreerunner.h>

#include <QBuffer>
#include <QCache>
#include <QClipboard>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextObjectInterface>

namespace Utils {

using namespace StyleHelper;

using TextFormat = std::pair<Theme::Color, UiElement>;

static constexpr TextFormat contentTF{Theme::Token_Text_Default, UiElement::UiElementBody2};

static constexpr std::array<TextFormat, 6> markdownHeadingFormats{
    TextFormat{contentTF.first, UiElement::UiElementH4},
    TextFormat{contentTF.first, UiElement::UiElementH5},
    TextFormat{contentTF.first, UiElement::UiElementH6Capital},
    TextFormat{contentTF.first, UiElement::UiElementH6Capital},
    TextFormat{contentTF.first, UiElement::UiElementH6Capital},
    TextFormat{contentTF.first, UiElement::UiElementH6Capital},
};

static constexpr int MinimumSizeBlocks = 5;

static QFont font(TextFormat format, bool underlined = false)
{
    QFont result = Utils::StyleHelper::uiFont(format.second);
    result.setUnderline(underlined);
    return result;
}
static int lineHeight(TextFormat format)
{
    return Utils::StyleHelper::uiFontLineHeight(format.second);
}

static QColor color(TextFormat format)
{
    return Utils::creatorColor(format.first);
}

static QTextDocument *highlightText(const QString &code, const QString &language)
{
    auto mimeTypes = mimeTypesForFileName("file." + language);

    QString mimeType = mimeTypes.isEmpty() ? "text/" + language : mimeTypes.first().name();

    auto document = Utils::Text::highlightCode(code, mimeType);
    if (document.isFinished())
        return document.result();
    document.cancel();
    QTextDocument *doc = new QTextDocument;
    doc->setPlainText(code);
    return doc;
}

static QStringList defaultCodeFontFamilies()
{
    return {"Menlo", "Source Code Pro", "Monospace", "Courier"};
}

static int registerSnippet(QTextDocument *document, const QString &code);

static void highlightCodeBlock(
    QTextDocument *document, QTextBlock &block, const QString &language, bool enableCopy)
{
    const int startPos = block.position();
    // Find the end of the code block ...
    for (block = block.next(); block.isValid(); block = block.next()) {
        if (!block.blockFormat().hasProperty(QTextFormat::BlockCodeLanguage))
            break;
        if (language != block.blockFormat().stringProperty(QTextFormat::BlockCodeLanguage))
            break;
    }
    const int endPos = (block.isValid() ? block.position() : document->characterCount()) - 1;

    // Get the text of the code block and erase it
    QTextCursor eraseCursor(document);
    eraseCursor.setPosition(startPos);
    eraseCursor.setPosition(endPos, QTextCursor::KeepAnchor);
    const QString code = eraseCursor.selectedText();
    eraseCursor.removeSelectedText();

    // Reposition the main cursor to startPos, to insert new content
    block = document->findBlock(startPos);
    QTextCursor cursor(block);

    QTextFrameFormat frameFormat;
    frameFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    frameFormat.setBackground(creatorColor(Theme::Token_Background_Muted));
    frameFormat.setPadding(SpacingTokens::ExPaddingGapM);
    frameFormat.setLeftMargin(SpacingTokens::VGapM);
    frameFormat.setRightMargin(SpacingTokens::VGapM);

    QTextFrame *frame = cursor.insertFrame(frameFormat);
    QTextCursor frameCursor(frame);

    if (enableCopy) {
        QTextBlockFormat linkBlockFmt;
        linkBlockFmt.setAlignment(Qt::AlignRight);
        frameCursor.insertBlock(linkBlockFmt);

        const int snippetId = registerSnippet(document, code);
        const QString copy_id = QString("copy:%1").arg(snippetId);

        // Insert copy icon
        QTextImageFormat imageFormat;
        imageFormat.setName("qrc:/markdownbrowser/images/code_copy_square.png");
        imageFormat.setAnchor(true);
        imageFormat.setAnchorHref(copy_id);
        imageFormat.setWidth(16);
        imageFormat.setHeight(16);
        frameCursor.insertImage(imageFormat);

        // Create a clickable anchor for the "Copy" text
        QTextCharFormat anchorFormat;
        anchorFormat.setAnchor(true);
        anchorFormat.setAnchorHref(copy_id);
        anchorFormat.setForeground(QColor("#888"));
        anchorFormat.setFontPointSize(10);
        frameCursor.setCharFormat(anchorFormat);
        frameCursor.insertText(" Copy");

        // Insert a new left-aligned block to start the first line of code
        QTextBlockFormat codeBlockFmt;
        codeBlockFmt.setAlignment(Qt::AlignLeft);
        frameCursor.insertBlock(codeBlockFmt);
    }

    std::unique_ptr<QTextDocument> codeDoc(highlightText(code, language));

    // Iterate each line in codeDoc and copy it out
    bool firstLine = true;
    for (auto tempBlock = codeDoc->begin(); tempBlock != codeDoc->end();
         tempBlock = tempBlock.next()) {
        // For each subsequent line, insert another block
        if (!firstLine) {
            QTextBlockFormat codeBlockFmt;
            codeBlockFmt.setAlignment(Qt::AlignLeft);
            frameCursor.insertBlock(codeBlockFmt);
        }
        firstLine = false;

        QTextCharFormat lineFormat = tempBlock.charFormat();
        lineFormat.setFontFamilies(defaultCodeFontFamilies());
        frameCursor.setCharFormat(lineFormat);

        auto formats = tempBlock.layout()->formats();
        frameCursor.insertText(tempBlock.text());
        frameCursor.block().layout()->setFormats(formats);
    }

    // Leave the frame
    QTextCursor next = frame->lastCursorPosition();
    block = next.block();
}

class AnimatedImageHandler : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    class Entry
    {
    public:
        using Pointer = std::shared_ptr<Entry>;

        Entry(const QByteArray &data)
        {
            if (data.isEmpty())
                return;

            buffer.setData(data);
            movie.setDevice(&buffer);
            if (movie.isValid()) {
                if (!movie.frameRect().isValid())
                    movie.jumpToFrame(0);
            }

            moveToThread(nullptr);
        }

        void moveToThread(QThread *thread)
        {
            buffer.moveToThread(thread);
            movie.moveToThread(thread);
        }

        QBuffer buffer;
        QtcMovie movie;
    };

public:
    AnimatedImageHandler(
        QObject *parent,
        std::function<void()> redraw,
        std::function<void(const QString &name)> scheduleLoad)
        : QObject(parent)
        , m_redraw(redraw)
        , m_scheduleLoad(scheduleLoad)
        , m_entries(1024 * 1024 * 10) // 10 MB max image cache size
    {}

    static Entry::Pointer makeEntry(const QByteArray &data, qsizetype maxSize)
    {
        // If the image is larger than what we allow in our cache,
        // we still want to create an entry, but one with an empty image.
        // So we clear it here, but still create the entry, so the painter can
        // correctly show the "broken image" placeholder instead.
        if (data.size() > maxSize)
            return std::make_shared<Entry>(QByteArray());

        return std::make_shared<Entry>(data);
    }

    virtual QSizeF intrinsicSize(
        QTextDocument *doc, int posInDocument, const QTextFormat &format) override
    {
        Q_UNUSED(doc);
        Q_UNUSED(posInDocument);
        QSize result = Utils::Icons::UNKNOWN_FILE.icon().actualSize(QSize(16, 16));
        QString name = format.toImageFormat().name();

        Entry::Pointer *entryPtr = m_entries.object(name);
        if (!entryPtr) {
            m_scheduleLoad(name);
            return result;
        }

        Entry::Pointer entry = *entryPtr;

        if (entry->movie.isValid()) {
            if (!entry->movie.frameRect().isValid())
                entry->movie.jumpToFrame(0);
            result = entry->movie.frameRect().size();
        }

        return result;
    }

    void drawObject(
        QPainter *painter,
        const QRectF &rect,
        QTextDocument *document,
        int posInDocument,
        const QTextFormat &format) override
    {
        Q_UNUSED(document);
        Q_UNUSED(posInDocument);

        Entry::Pointer *entryPtr = m_entries.object(format.toImageFormat().name());

        if (!entryPtr)
            painter->drawPixmap(
                rect.toRect(), Utils::Icons::UNKNOWN_FILE.icon().pixmap(rect.size().toSize()));
        else if (!(*entryPtr)->movie.isValid())
            painter->drawPixmap(rect.toRect(), m_brokenImage.pixmap(rect.size().toSize()));
        else
            painter->drawImage(rect, (*entryPtr)->movie.currentImage());
    }

    void set(const QString &name, const QByteArray &data)
    {
        set(name, makeEntry(data, m_entries.maxCost()));
    }

    void set(const QString &name, const Entry::Pointer &entry)
    {
        entry->moveToThread(thread());

        if (entry->movie.frameCount() > 1) {
            connect(&entry->movie, &QtcMovie::frameChanged, this, [this]() { m_redraw(); });
            entry->movie.start();
        }
        const qint64 size = qMax(1, entry->buffer.size());

        if (size > m_entries.maxCost()) {
            return;
        }

        Entry::Pointer *entryPtr = new Entry::Pointer(entry);
        if (m_entries.insert(name, entryPtr, size))
            m_redraw();
    }

    void setMaximumCacheSize(qsizetype maxSize) { m_entries.setMaxCost(maxSize); }
    qsizetype maximumCacheSize() const { return m_entries.maxCost(); }

private:
    std::function<void()> m_redraw;
    std::function<void(const QString &)> m_scheduleLoad;
    QCache<QString, Entry::Pointer> m_entries;

    const Icon ErrorCloseIcon = Utils::Icon({{":/utils/images/close.png", Theme::IconsErrorColor}});

    const QIcon m_brokenImage = Icon::combinedIcon(
        {Utils::Icons::UNKNOWN_FILE.icon(), ErrorCloseIcon.icon()});
};

class AnimatedDocument : public QTextDocument
{
public:
    AnimatedDocument(QObject *parent = nullptr)
        : QTextDocument(parent)
        , m_imageHandler(
              this,
              [this]() { documentLayout()->update(); },
              [this](const QString &name) { scheduleLoad(QUrl(name)); })
    {
        connect(this, &QTextDocument::documentLayoutChanged, this, [this]() {
            documentLayout()->registerHandler(QTextFormat::ImageObject, &m_imageHandler);
        });

        connect(this, &QTextDocument::contentsChanged, this, [this]() {
            if (m_urlsToLoad.isEmpty())
                return;

            if (m_imageLoaderTree.isRunning()) {
                if (!m_needsToRestartLoading)
                    return;
                m_imageLoaderTree.cancel();
            }

            m_needsToRestartLoading = false;

            using namespace Tasking;

            const bool isBaseHttp = m_basePath.scheme() == QStringLiteral("http")
                                    || m_basePath.scheme() == QStringLiteral("https");

            const auto isRemoteUrl = [isBaseHttp](const QUrl &url) {
                return url.scheme() == "http" || url.scheme() == "https"
                       || (url.isRelative() && isBaseHttp);
            };

            const auto isLocalUrl = [this, isRemoteUrl](const QUrl &url) {
                if (url.scheme() == "qrc")
                    return true;

                if (!m_basePath.isEmpty() && !isRemoteUrl(url))
                    return true;

                return false;
            };

            QSet<QUrl> remoteUrls = Utils::filtered(m_urlsToLoad, isRemoteUrl);
            QSet<QUrl> localUrls = Utils::filtered(m_urlsToLoad, isLocalUrl);

            if (!m_loadRemoteImages)
                remoteUrls.clear();

            struct RemoteData
            {
                QUrl url;
                QByteArray data;
            };

            Storage<RemoteData> remoteData;

            const LoopList remoteIterator(Utils::toList(remoteUrls));
            const LoopList localIterator(Utils::toList(localUrls));

            auto onQuerySetup =
                [remoteData, this, remoteIterator, base = m_basePath.toUrl()](NetworkQuery &query) {
                    QUrl url = *remoteIterator;
                    if (url.isRelative())
                        url = base.resolved(url);

                    QNetworkRequest request(url);
                    if (m_requestHook)
                        m_requestHook(&request);

                    query.setRequest(request);
                    query.setNetworkAccessManager(m_networkAccessManager);
                    remoteData->url = *remoteIterator;
                };

            auto onQueryDone = [this, remoteData](const NetworkQuery &query, DoneWith result) {
                if (result == DoneWith::Cancel)
                    return;
                m_urlsToLoad.remove(query.reply()->url());

                if (result == DoneWith::Success) {
                    remoteData->data = query.reply()->readAll();
                } else {
                    m_imageHandler.set(remoteData->url.toString(), QByteArray{});
                    markContentsDirty(0, this->characterCount());
                }
            };

            using EntryPointer = AnimatedImageHandler::Entry::Pointer;

            auto onMakeEntrySetup = [remoteData, maxSize = m_imageHandler.maximumCacheSize()](
                                        Async<EntryPointer> &async) {
                async.setConcurrentCallData(
                    [](const QByteArray &data, qsizetype maxSize) {
                        return AnimatedImageHandler::makeEntry(data, maxSize);
                    },
                    remoteData->data,
                    maxSize);
            };

            auto onMakeEntryDone =
                [this, remoteIterator, remoteData](const Async<EntryPointer> &async) {
                    EntryPointer result = async.result();
                    if (result) {
                        m_imageHandler.set(remoteData->url.toString(), result);
                        markContentsDirty(0, this->characterCount());
                    }
                };

            auto onLocalSetup =
                [localIterator, basePath = m_basePath, maxSize = m_imageHandler.maximumCacheSize()](
                    Async<EntryPointer> &async) {
                    const QUrl url = *localIterator;
                    async.setConcurrentCallData(
                        [](QPromise<EntryPointer> &promise,
                           const FilePath &basePath,
                           const QUrl &url,
                           qsizetype maxSize) {
                            if (url.scheme() == "qrc") {
                                QFile f(":" + url.path());
                                if (!f.open(QIODevice::ReadOnly))
                                    return;

                                promise.addResult(
                                    AnimatedImageHandler::makeEntry(f.readAll(), maxSize));
                                return;
                            }

                            const FilePath path = basePath.resolvePath(url.path());
                            auto data = path.fileContents();
                            if (!data || promise.isCanceled())
                                return;

                            promise.addResult(AnimatedImageHandler::makeEntry(*data, maxSize));
                        },
                        basePath,
                        url,
                        maxSize);
                };

            auto onLocalDone = [localIterator, this](const Async<EntryPointer> &async) {
                EntryPointer result = async.result();
                if (result)
                    m_imageHandler.set(localIterator->toString(), std::move(result));
            };

            // clang-format off
            Group group {
                parallelLimit(2),
                For(remoteIterator) >> Do {
                    remoteData,
                    Group {
                        NetworkQueryTask{onQuerySetup, onQueryDone},
                        AsyncTask<EntryPointer>(onMakeEntrySetup, onMakeEntryDone),
                    } || successItem,
                },
                For(localIterator) >> Do {
                    AsyncTask<EntryPointer>(onLocalSetup, onLocalDone) || successItem,
                },
            };
            // clang-format on

            m_imageLoaderTree.start(group);
        });
    }

    int registerSnippet(const QString &code)
    {
        const int id = m_nextSnippetId++;
        m_snippetMap.insert(id, code);
        return id;
    }

    QString snippetById(int id) const { return m_snippetMap.value(id); }

    void clearSnippets()
    {
        m_snippetMap.clear();
        m_nextSnippetId = 0;
    }

    void scheduleLoad(const QUrl &url)
    {
        m_urlsToLoad.insert(url);
        m_needsToRestartLoading = true;
    }

    void setBasePath(const FilePath &filePath) { m_basePath = filePath; }
    void setAllowRemoteImages(bool allow) { m_loadRemoteImages = allow; }

    void setNetworkAccessManager(QNetworkAccessManager *nam) { m_networkAccessManager = nam; }
    void setRequestHook(const MarkdownBrowser::RequestHook &hook) { m_requestHook = hook; }
    void setMaximumCacheSize(qsizetype maxSize) { m_imageHandler.setMaximumCacheSize(maxSize); }

private:
    AnimatedImageHandler m_imageHandler;
    QSet<QUrl> m_urlsToLoad;
    bool m_needsToRestartLoading = false;
    bool m_loadRemoteImages = false;
    Tasking::TaskTreeRunner m_imageLoaderTree;
    FilePath m_basePath;
    std::function<void(QNetworkRequest *)> m_requestHook;
    QNetworkAccessManager *m_networkAccessManager = NetworkAccessManager::instance();
    QMap<int, QString> m_snippetMap;
    int m_nextSnippetId = 0;
};

static int registerSnippet(QTextDocument *document, const QString &code)
{
    auto *animDoc = static_cast<AnimatedDocument *>(document);
    return animDoc->registerSnippet(code);
}

MarkdownBrowser::MarkdownBrowser(QWidget *parent)
    : QTextBrowser(parent)
    , m_enableCodeCopyButton(false)
{
    setOpenLinks(false);

    connect(this, &QTextBrowser::anchorClicked, this, &MarkdownBrowser::handleAnchorClicked);

    setDocument(new AnimatedDocument(this));
}

QSize MarkdownBrowser::sizeHint() const
{
    return document()->size().toSize();
}
QSize MarkdownBrowser::minimumSizeHint() const
{
    //Lets use the size of the first few blocks as minimum size hint
    QTextBlock block = document()->begin();
    QRectF boundingRect;
    for (int i = 0; i < MinimumSizeBlocks && block.isValid(); ++i, block = block.next()) {
        QTextLayout *layout = block.layout();
        QRectF blockRect = layout->boundingRect();
        boundingRect.adjust(0, 0, 0, blockRect.height());
        boundingRect.setWidth(qMax(boundingRect.width(), blockRect.width()));
    }
    return boundingRect.size().toSize() + QTextBrowser::minimumSizeHint();
}

void MarkdownBrowser::setMargins(const QMargins &margins)
{
    setViewportMargins(margins);
}

void MarkdownBrowser::setEnableCodeCopyButton(bool enable)
{
    m_enableCodeCopyButton = enable;
}

void MarkdownBrowser::setAllowRemoteImages(bool allow)
{
    static_cast<AnimatedDocument *>(document())->setAllowRemoteImages(allow);
}

void MarkdownBrowser::setNetworkAccessManager(QNetworkAccessManager *nam)
{
    static_cast<AnimatedDocument *>(document())->setNetworkAccessManager(nam);
}

void MarkdownBrowser::setRequestHook(const RequestHook &hook)
{
    static_cast<AnimatedDocument *>(document())->setRequestHook(hook);
}

void MarkdownBrowser::setMaximumCacheSize(qsizetype maxSize)
{
    static_cast<AnimatedDocument *>(document())->setMaximumCacheSize(maxSize);
}

void MarkdownBrowser::handleAnchorClicked(const QUrl &link)
{
    if (link.scheme() != QLatin1String("copy")) {
        if (link.scheme() == "http" || link.scheme() == "https")
            QDesktopServices::openUrl(link);

        if (link.hasFragment() && link.path().isEmpty() && link.scheme().isEmpty()) {
            // local anchor
            scrollToAnchor(link.fragment(QUrl::FullyEncoded));
        }

        return;
    }

    bool ok = false;
    const int snippetId = link.path().toInt(&ok);
    if (!ok)
        return;

    auto *animDoc = static_cast<AnimatedDocument *>(document());
    const QString snippet = animDoc->snippetById(snippetId).replace(QChar::ParagraphSeparator, '\n');
    if (snippet.isEmpty())
        return;

    Utils::setClipboardAndSelection(snippet);
}

void MarkdownBrowser::setBasePath(const FilePath &filePath)
{
    static_cast<AnimatedDocument *>(document())->setBasePath(filePath);
}

void MarkdownBrowser::setMarkdown(const QString &markdown)
{
    QScrollBar *sb = verticalScrollBar();
    int oldValue = sb->value();

    auto *animDoc = static_cast<AnimatedDocument *>(document());
    animDoc->clearSnippets();
    document()->setMarkdown(markdown);
    postProcessDocument(true);

    QTimer::singleShot(0, this, [sb, oldValue] { sb->setValue(oldValue); });

    // Reset cursor to start of the document, so that "show" does not
    // scroll to the end of the document.
    setTextCursor(QTextCursor(document()));
}

QString MarkdownBrowser::toMarkdown() const
{
    return document()->toMarkdown();
}

void MarkdownBrowser::postProcessDocument(bool firstTime) const
{
    const QFont contentFont = Utils::font(contentTF);
    const float fontScale = font().pointSizeF() / qGuiApp->font().pointSizeF();
    const auto scaledFont = [fontScale](QFont f) {
        f.setPointSizeF(f.pointSizeF() * fontScale);
        return f;
    };
    document()->setDefaultFont(scaledFont(contentFont));

    for (QTextBlock block = document()->begin(); block != document()->end(); block = block.next()) {
        if (firstTime) {
            const QTextBlockFormat blockFormat = block.blockFormat();
            // Leave images as they are.
            if (block.text().contains(QChar::ObjectReplacementCharacter))
                continue;

            // Convert code blocks to highlighted frames
            if (blockFormat.hasProperty(QTextFormat::BlockCodeLanguage)) {
                const QString language = blockFormat.stringProperty(QTextFormat::BlockCodeLanguage);
                highlightCodeBlock(document(), block, language, m_enableCodeCopyButton);
                continue;
            }

            // Add anchors to headings. This should actually be done by Qt QTBUG-120518
            if (blockFormat.hasProperty(QTextFormat::HeadingLevel)) {
                QTextCharFormat cFormat = block.charFormat();
                QString anchor;
                const QString text = block.text();
                for (const QChar &c : text) {
                    if (c == ' ')
                        anchor.append('-');
                    else if (c == '_' || c == '-' || c.isDigit() || c.isLetter())
                        anchor.append(c.toLower());
                }
                cFormat.setAnchor(true);
                cFormat.setAnchorNames({anchor});
                QTextCursor cursor(block);
                cursor.setBlockCharFormat(cFormat);
            }
        }

        // Update fonts
        QTextCursor cursor(block);
        auto blockFormat = block.blockFormat();

        const auto scaledFont = [fontScale](QFont f) {
            f.setPointSizeF(f.pointSizeF() * fontScale);
            return f;
        };

        if (blockFormat.hasProperty(QTextFormat::HeadingLevel)) {
            blockFormat.setTopMargin(SpacingTokens::ExVPaddingGapXl * fontScale);
            blockFormat.setBottomMargin(SpacingTokens::VGapL * fontScale);
        } else {
            blockFormat
                .setLineHeight(lineHeight(contentTF) * fontScale, QTextBlockFormat::FixedHeight);
        }

        cursor.mergeBlockFormat(blockFormat);

        const TextFormat &headingTf
            = markdownHeadingFormats[qBound(0, blockFormat.headingLevel() - 1, 5)];

        const QFont headingFont = scaledFont(Utils::font(headingTf));
        for (auto it = block.begin(); !(it.atEnd()); ++it) {
            const QTextFragment fragment = it.fragment();
            if (fragment.isValid()) {
                QTextCharFormat charFormat = fragment.charFormat();
                cursor.setPosition(fragment.position());
                cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
                if (blockFormat.hasProperty(QTextFormat::HeadingLevel)) {
                    // We don't use font size adjustment for headings
                    charFormat.clearProperty(QTextFormat::FontSizeAdjustment);
                    charFormat.setFontPointSize(headingFont.pointSizeF());
                    charFormat.setFontCapitalization(headingFont.capitalization());
                    charFormat.setFontFamilies(headingFont.families());
                    charFormat.setFontWeight(headingFont.weight());
                    charFormat.setForeground(color(headingTf));
                } else if (charFormat.isAnchor()) {
                    charFormat.setForeground(creatorColor(Theme::Token_Text_Accent));
                } else {
                    charFormat.setForeground(color(contentTF));
                }
                cursor.setCharFormat(charFormat);
            }
        }
    }
}

void MarkdownBrowser::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange)
        postProcessDocument(false);
    QTextBrowser::changeEvent(event);
}

} // namespace Utils

#include "markdownbrowser.moc"
