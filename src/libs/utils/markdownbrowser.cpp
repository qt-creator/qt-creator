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
#include "utilstr.h"

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
#include <QTextDocumentFragment>
#include <QTextDocumentWriter>
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

static QTextFragment copyButtonFragment(const QTextBlock &block);

class CopyButtonHandler : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit CopyButtonHandler(QObject *parent = nullptr)
        : QObject(parent)
    {}

    static constexpr int objectId() { return QTextFormat::UserObject + 1; }
    static constexpr int codePropertyId() { return QTextFormat::UserProperty + 1; }
    static constexpr int isCopiedPropertyId() { return QTextFormat::UserProperty + 2; }

    static QString text(bool isCopied)
    {
        return " " + (isCopied ? Tr::tr("Copied") : Tr::tr("Copy"));
    }
    static QIcon icon(bool isCopied)
    {
        static QIcon clickedIcon = Utils::Icons::OK.icon();
        static QIcon unclickedIcon = Utils::Icons::COPY.icon();
        if (isCopied)
            return clickedIcon;
        return unclickedIcon;
    }

    static void resetOtherCopyButtons(QTextDocument *doc, const QTextFragment &clickedFragment)
    {
        for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
            const QTextFragment otherFragment = copyButtonFragment(block);
            if (otherFragment.isValid() && otherFragment != clickedFragment) {
                QTextCursor resetCursor(block);
                resetCursor.setPosition(otherFragment.position());
                resetCursor.setPosition(
                    otherFragment.position() + otherFragment.length(), QTextCursor::KeepAnchor);
                QTextCharFormat resetFormat = otherFragment.charFormat();
                resetFormat.setProperty(isCopiedPropertyId(), false);
                resetCursor.setCharFormat(resetFormat);
            }
        }
    }

    static void copyCodeAndUpdateButton(QTextCursor &cursor, const QTextCharFormat &format)
    {
        const QString code = format.property(codePropertyId()).value<QString>();
        Utils::setClipboardAndSelection(code);

        QTextCharFormat newFormat = format;
        newFormat.setProperty(isCopiedPropertyId(), true);
        cursor.setCharFormat(newFormat);
    }

    QSizeF intrinsicSize(QTextDocument *doc, int pos, const QTextFormat &format) override
    {
        Q_UNUSED(pos)

        if (!doc || !format.hasProperty(isCopiedPropertyId()))
            return QSizeF(0, 0);

        const QFontMetricsF metrics(getFont(doc));
        const bool isCopied = format.property(isCopiedPropertyId()).value<bool>();

        return QSizeF(metrics.horizontalAdvance(text(isCopied)) + 30, metrics.height() + 10);
    }

    void drawObject(
        QPainter *painter,
        const QRectF &rect,
        QTextDocument *doc,
        int pos,
        const QTextFormat &format) override
    {
        Q_UNUSED(pos)

        if (!doc || !format.hasProperty(isCopiedPropertyId()))
            return;

        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::transparent);
        painter->drawRect(rect);

        const bool isCopied = format.property(isCopiedPropertyId()).value<bool>();

        constexpr int iconSize = 16;
        QRectF iconRect(rect.left(), rect.top() + (rect.height() - iconSize) / 2, iconSize, iconSize);
        icon(isCopied).paint(painter, iconRect.toRect());

        painter->setPen(QColor("#888"));
        painter->setFont(getFont(doc));
        painter
            ->drawText(rect.adjusted(20, 0, -5, 0), Qt::AlignLeft | Qt::AlignVCenter, text(isCopied));
    }

private:
    QFont getFont(QTextDocument *doc) const
    {
        QFont font = doc->defaultFont();
        font.setPointSize(10);
        return font;
    }
};

static QTextFragment copyButtonFragment(const QTextBlock &block)
{
    for (auto it = block.begin(); !it.atEnd(); ++it) {
        QTextFragment fragment = it.fragment();
        if (fragment.charFormat().objectType() == CopyButtonHandler::objectId())
            return fragment;
    }
    return QTextFragment();
}

static QPointF blockBBoxTopLeftPosition(const QTextBlock &block)
{
    const QAbstractTextDocumentLayout *docLayout = block.document()->documentLayout();
    const QRectF blockRect = docLayout->blockBoundingRect(block);
    return blockRect.topLeft();
}

static QRectF calculateFragmentBounds(
    const QTextBlock &block, const QTextFragment &fragment, const QPointF &documentOffset)
{
    QRectF bounds(0, 0, 0, 0);

    if (!block.isValid() || !fragment.isValid())
        return bounds;

    QTextLayout *layout = block.layout();
    if (!layout)
        return bounds;

    int fragmentStart = fragment.position() - block.position();
    QTextLine line = layout->lineForTextPosition(fragmentStart);
    if (!line.isValid())
        return bounds;

    qreal x = line.cursorToX(fragmentStart);
    qreal y = line.y();
    qreal width = line.cursorToX(fragmentStart + fragment.length()) - x;
    qreal height = line.height();

    bounds = QRectF(x, y, width, height);
    bounds.translate(documentOffset);

    return bounds;
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
        Q_UNUSED(doc)
        Q_UNUSED(posInDocument)
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
        Q_UNUSED(document)
        Q_UNUSED(posInDocument)

        const QString name = format.toImageFormat().name();
        Entry::Pointer *entryPtr = m_entries.object(name);

        if (!entryPtr) {
            constexpr QStringView themeScheme(u"theme://");
            constexpr QStringView iconScheme(u"icon://");

            QVariant resource = document->resource(QTextDocument::ImageResource, name);
            if (resource.isValid()) {
                const QImage img = qvariant_cast<QImage>(resource);
                if (!img.isNull()) {
                    painter->drawImage(rect, img);
                    return;
                }
            } else if (name.startsWith(themeScheme)) {
                const QIcon icon = QIcon::fromTheme(name.mid(themeScheme.length()));
                if (!icon.isNull()) {
                    painter->drawPixmap(
                        rect.toRect(),
                        icon.pixmap(rect.size().toSize(), painter->device()->devicePixelRatioF()));
                    return;
                }
            } else if (name.startsWith(iconScheme)) {
                std::optional<Icon> icon = Icons::fromString(name.mid(iconScheme.length()));
                if (icon) {
                    painter->drawPixmap(rect.toRect(), icon->pixmap());
                    return;
                }
            }

            painter->drawPixmap(
                rect.toRect(), Utils::Icons::UNKNOWN_FILE.icon().pixmap(rect.size().toSize()));
        } else if (!(*entryPtr)->movie.isValid())
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
                QVariant res = this->resource(QTextDocument::ImageResource, url);
                if (res.isValid())
                    return false;

                if (url.scheme() == "qrc")
                    return true;

                if (!url.scheme().isEmpty())
                    return false;

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
};

MarkdownBrowser::MarkdownBrowser(QWidget *parent)
    : QTextBrowser(parent)
    , m_enableCodeCopyButton(false)
{
    setOpenLinks(false);
    connect(this, &QTextBrowser::anchorClicked, this, &MarkdownBrowser::handleAnchorClicked);

    setDocument(new AnimatedDocument(this));
    document()
        ->documentLayout()
        ->registerHandler(CopyButtonHandler::objectId(), new CopyButtonHandler(document()));
}

void MarkdownBrowser::highlightCodeBlock(const QString &language, QTextBlock &block)
{
    const int startPos = block.position();
    // Find the end of the code block ...
    for (block = block.next(); block.isValid(); block = block.next()) {
        if (!block.blockFormat().hasProperty(QTextFormat::BlockCodeLanguage))
            break;
        if (language != block.blockFormat().stringProperty(QTextFormat::BlockCodeLanguage))
            break;
    }
    const int endPos = (block.isValid() ? block.position() : document()->characterCount()) - 1;

    // Get the text of the code block and erase it
    QTextCursor eraseCursor(document());
    eraseCursor.setPosition(startPos);
    eraseCursor.setPosition(endPos, QTextCursor::KeepAnchor);
    const QString code = eraseCursor.selectedText();
    eraseCursor.removeSelectedText();

    // Reposition the main cursor to startPos, to insert new content
    block = document()->findBlock(startPos);
    QTextCursor cursor(block);

    QTextFrameFormat frameFormat;
    frameFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    frameFormat.setBackground(creatorColor(Theme::Token_Background_Muted));
    frameFormat.setPadding(SpacingTokens::ExPaddingGapM);
    frameFormat.setLeftMargin(SpacingTokens::VGapM);
    frameFormat.setRightMargin(SpacingTokens::VGapM);

    QTextFrame *frame = cursor.insertFrame(frameFormat);
    QTextCursor frameCursor(frame);

    if (m_enableCodeCopyButton) {
        QTextBlockFormat rightAlignedCopyButton;
        rightAlignedCopyButton.setAlignment(Qt::AlignRight);
        frameCursor.insertBlock(rightAlignedCopyButton);

        QString copiableCode = code;
        copiableCode.replace(QChar::ParagraphSeparator, '\n');

        QTextCharFormat buttonFormat;
        buttonFormat.setObjectType(CopyButtonHandler::objectId());
        buttonFormat.setProperty(CopyButtonHandler::codePropertyId(), copiableCode);
        buttonFormat.setProperty(CopyButtonHandler::isCopiedPropertyId(), false);
        frameCursor.insertText(QString(QChar::ObjectReplacementCharacter), buttonFormat);

        QTextBlockFormat leftAlignedCode;
        leftAlignedCode.setAlignment(Qt::AlignLeft);
        frameCursor.insertBlock(leftAlignedCode);
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
    if (link.scheme() == "http" || link.scheme() == "https")
        QDesktopServices::openUrl(link);

    if (link.hasFragment() && link.path().isEmpty() && link.scheme().isEmpty()) {
        // local anchor
        scrollToAnchor(link.fragment(QUrl::FullyEncoded));
    }
}

void MarkdownBrowser::setBasePath(const FilePath &filePath)
{
    static_cast<AnimatedDocument *>(document())->setBasePath(filePath);
}

void MarkdownBrowser::setMarkdown(const QString &markdown)
{
    QScrollBar *sb = verticalScrollBar();
    const int scrollValue = sb->value();

    document()->setMarkdown(markdown);
    postProcessDocument(true);

    QTimer::singleShot(0, this, [sb, scrollValue] { sb->setValue(scrollValue); });

    // Reset cursor to start of the document, so that "show" does not
    // scroll to the end of the document.
    setTextCursor(QTextCursor(document()));
}

QString MarkdownBrowser::toMarkdown() const
{
    return document()->toMarkdown();
}

void MarkdownBrowser::postProcessDocument(bool firstTime)
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
                highlightCodeBlock(language, block);
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

        QList<QTextCursor> fragmentCursors = [&block]() {
            QList<QTextCursor> result;
            for (auto it = block.begin(); !it.atEnd(); ++it) {
                QTextFragment fragment = it.fragment();
                result.emplaceBack(block);
                result.back().setPosition(fragment.position());
                result.back().setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
            }
            return result;
        }();

        for (QTextCursor &fc : fragmentCursors) {
            QTextCharFormat charFormat = block.charFormat();

            if (blockFormat.hasProperty(QTextFormat::HeadingLevel)) {
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
            fc.setCharFormat(charFormat);
        }
    }
}

void MarkdownBrowser::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange)
        postProcessDocument(false);
    QTextBrowser::changeEvent(event);
}

std::optional<std::pair<QTextFragment, QRectF>> MarkdownBrowser::findCopyButtonFragmentAt(const QPoint& viewportPos)
{
    QTextCursor cursor = cursorForPosition(viewportPos);
    if (cursor.isNull())
        return std::nullopt;

    const QTextCharFormat format = cursor.charFormat();
    if (format.objectType() != CopyButtonHandler::objectId())
        return std::nullopt;

    const QTextBlock block = cursor.block();
    QTextFragment fragment = copyButtonFragment(block);
    if (!fragment.isValid())
        return std::nullopt;

    QPointF blockPos = blockBBoxTopLeftPosition(block);
    QRectF fragmentRect = calculateFragmentBounds(block, fragment, blockPos);
    fragmentRect.translate(-horizontalScrollBar()->value(), -verticalScrollBar()->value());

    if (fragmentRect.contains(viewportPos))
        return std::make_pair(fragment, fragmentRect);
    else
        return std::nullopt;
}

void MarkdownBrowser::mousePressEvent(QMouseEvent *event)
{
    auto result = findCopyButtonFragmentAt(event->pos());
    if (result) {
        QTextFragment fragment = result->first;
        QTextCursor cursor(document());
        cursor.setPosition(fragment.position());
        cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);

        CopyButtonHandler::resetOtherCopyButtons(document(), fragment);
        CopyButtonHandler::copyCodeAndUpdateButton(cursor, fragment.charFormat());

        document()->documentLayout()->update();
        event->accept();
    } else
        QTextBrowser::mousePressEvent(event);
}

void MarkdownBrowser::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint mousePos = event->pos();

    if (m_cachedCopyRect && m_cachedCopyRect->contains(mousePos)) {
        QTextBrowser::mouseMoveEvent(event);
        return;
    }

    m_cachedCopyRect.reset();
    viewport()->unsetCursor();

    auto result = findCopyButtonFragmentAt(mousePos);
    if (result) {
        m_cachedCopyRect = result->second;
        viewport()->setCursor(Qt::PointingHandCursor);
    }

    QTextBrowser::mouseMoveEvent(event);
}

QMimeData *MarkdownBrowser::createMimeDataFromSelection() const
{
    // Basically a copy of QTextEditMimeData::setup, just replacing the object markers.
    QMimeData *mimeData = new QMimeData;
    QTextDocumentFragment fragment(textCursor());

    static const auto removeObjectChar = [](QString &&text) {
        return text.replace(QChar::ObjectReplacementCharacter, "");
    };

    mimeData->setData("text/html", removeObjectChar(fragment.toHtml()).toUtf8());
    mimeData->setData("text/markdown", removeObjectChar(fragment.toMarkdown()).toUtf8());
    {
        QBuffer buffer;
        QTextDocumentWriter writer(&buffer, "ODF");
        if (writer.write(fragment)) {
            buffer.close();
            mimeData->setData("application/vnd.oasis.opendocument.text", buffer.data());
        }
    }
    mimeData->setText(removeObjectChar(fragment.toPlainText()));

    return mimeData;
}

} // namespace Utils

#include "markdownbrowser.moc"
