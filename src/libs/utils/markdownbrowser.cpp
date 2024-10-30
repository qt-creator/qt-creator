// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdownbrowser.h"

#include "algorithm.h"
#include "async.h"
#include "mimeutils.h"
#include "networkaccessmanager.h"
#include "stylehelper.h"
#include "textutils.h"
#include "theme/theme.h"
#include "utilsicons.h"

#include <solutions/tasking/networkquery.h>
#include <solutions/tasking/tasktree.h>
#include <solutions/tasking/tasktreerunner.h>

#include <QBuffer>
#include <QCache>
#include <QMovie>
#include <QPainter>
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

static void highlightCodeBlock(QTextDocument *document, QTextBlock &block, const QString &language)
{
    const int position = block.position();
    // Find the end of the code block ...
    for (block = block.next(); block.isValid(); block = block.next()) {
        if (!block.blockFormat().hasProperty(QTextFormat::BlockCodeLanguage))
            break;
        if (language != block.blockFormat().stringProperty(QTextFormat::BlockCodeLanguage))
            break;
    }
    const int end = (block.isValid() ? block.position() : document->characterCount()) - 1;
    // Get the text of the code block and erase it
    QTextCursor eraseCursor(document);
    eraseCursor.setPosition(position);
    eraseCursor.setPosition(end, QTextCursor::KeepAnchor);

    const QString code = eraseCursor.selectedText();
    eraseCursor.removeSelectedText();

    // Create a new Frame and insert the highlighted code ...
    block = document->findBlock(position);

    QTextCursor cursor(block);
    QTextFrameFormat format;
    format.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    format.setBackground(creatorColor(Theme::Token_Background_Muted));
    format.setPadding(SpacingTokens::ExPaddingGapM);
    format.setLeftMargin(SpacingTokens::VGapM);
    format.setRightMargin(SpacingTokens::VGapM);

    QTextFrame *frame = cursor.insertFrame(format);
    QTextCursor frameCursor(frame);

    std::unique_ptr<QTextDocument> codeDocument(highlightText(code, language));
    bool first = true;

    for (auto block = codeDocument->begin(); block != codeDocument->end(); block = block.next()) {
        if (!first)
            frameCursor.insertBlock();

        QTextCharFormat charFormat = block.charFormat();
        charFormat.setFontFamilies(defaultCodeFontFamilies());
        frameCursor.setCharFormat(charFormat);

        first = false;
        auto formats = block.layout()->formats();
        frameCursor.insertText(block.text());
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
        using Pointer = std::unique_ptr<Entry>;

        Entry(const QByteArray &data)
        {
            if (data.isEmpty())
                return;

            buffer.setData(data);
            movie.setDevice(&buffer);
        }

        QBuffer buffer;
        QMovie movie;
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

    virtual QSizeF intrinsicSize(
        QTextDocument *doc, int posInDocument, const QTextFormat &format) override
    {
        Q_UNUSED(doc);
        Q_UNUSED(posInDocument);
        QString name = format.toImageFormat().name();

        Entry *entry = m_entries.object(name);

        if (entry && entry->movie.isValid()) {
            if (!entry->movie.frameRect().isValid())
                entry->movie.jumpToFrame(0);
            return entry->movie.frameRect().size();
        } else if (!entry) {
            m_scheduleLoad(name);
        }

        return Utils::Icons::UNKNOWN_FILE.icon().actualSize(QSize(16, 16));
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

        Entry *entry = m_entries.object(format.toImageFormat().name());

        if (entry) {
            if (entry->movie.isValid())
                painter->drawImage(rect, entry->movie.currentImage());
            else
                painter->drawPixmap(rect.toRect(), m_brokenImage.pixmap(rect.size().toSize()));
            return;
        }

        painter->drawPixmap(
            rect.toRect(), Utils::Icons::UNKNOWN_FILE.icon().pixmap(rect.size().toSize()));
    }

    void set(const QString &name, QByteArray data)
    {
        if (data.size() > m_entries.maxCost())
            data.clear();

        set(name, std::make_unique<Entry>(data));
    }

    void set(const QString &name, std::unique_ptr<Entry> entry)
    {
        if (entry->movie.frameCount() > 1) {
            connect(&entry->movie, &QMovie::frameChanged, this, [this]() { m_redraw(); });
            entry->movie.start();
        }
        const qint64 size = qMax(1, entry->buffer.size());
        if (m_entries.insert(name, entry.release(), size))
            m_redraw();
    }

private:
    std::function<void()> m_redraw;
    std::function<void(const QString &)> m_scheduleLoad;
    QCache<QString, Entry> m_entries;

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

            QList<QUrl> remoteUrls = Utils::filtered(m_urlsToLoad, isRemoteUrl);
            QList<QUrl> localUrls = Utils::filtered(m_urlsToLoad, std::not_fn(isRemoteUrl));

            if (m_basePath.isEmpty())
                localUrls.clear();

            if (!m_loadRemoteImages)
                remoteUrls.clear();

            const LoopList remoteIterator(remoteUrls);
            const LoopList localIterator(localUrls);

            auto onQuerySetup = [remoteIterator, base = m_basePath.toUrl()](NetworkQuery &query) {
                QUrl url = *remoteIterator;
                if (url.isRelative())
                    url = base.resolved(url);

                query.setRequest(QNetworkRequest(*remoteIterator));
                query.setNetworkAccessManager(NetworkAccessManager::instance());
            };

            auto onQueryDone = [this](const NetworkQuery &query, DoneWith result) {
                if (result == DoneWith::Cancel)
                    return;
                m_urlsToLoad.removeOne(query.reply()->url());

                if (result == DoneWith::Success)
                    m_imageHandler.set(query.reply()->url().toString(), query.reply()->readAll());
                else
                    m_imageHandler.set(query.reply()->url().toString(), QByteArray{});

                markContentsDirty(0, this->characterCount());
            };

            using EntryPointer = AnimatedImageHandler::Entry::Pointer;

            auto onLocalSetup = [localIterator, basePath = m_basePath](Async<EntryPointer> &async) {
                const FilePath u = basePath.resolvePath(localIterator->path());
                async.setConcurrentCallData(
                    [](FilePath f) -> EntryPointer {
                        auto data = f.fileContents();
                        if (!data)
                            return nullptr;

                        return std::make_unique<AnimatedImageHandler::Entry>(*data);
                    },
                    u);
            };

            auto onLocalDone = [localIterator, this](const Async<EntryPointer> &async) {
                EntryPointer result = async.takeResult();
                if (result)
                    m_imageHandler.set(localIterator->toString(), std::move(result));
            };

            // clang-format off
            Group group {
                parallelLimit(2),
                For(remoteIterator) >> Do {
                    NetworkQueryTask{onQuerySetup, onQueryDone} || successItem,
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
        m_urlsToLoad.append(url);
        m_needsToRestartLoading = true;
    }

    void setBasePath(const FilePath &filePath) { m_basePath = filePath; }
    void setAllowRemoteImages(bool allow) { m_loadRemoteImages = allow; }

private:
    AnimatedImageHandler m_imageHandler;
    QList<QUrl> m_urlsToLoad;
    bool m_needsToRestartLoading = false;
    bool m_loadRemoteImages = false;
    Tasking::TaskTreeRunner m_imageLoaderTree;
    FilePath m_basePath;
};

MarkdownBrowser::MarkdownBrowser(QWidget *parent)
    : QTextBrowser(parent)
{
    setDocument(new AnimatedDocument(this));
}

void MarkdownBrowser::setAllowRemoteImages(bool allow)
{
    static_cast<AnimatedDocument *>(document())->setAllowRemoteImages(allow);
}

void MarkdownBrowser::setBasePath(const FilePath &filePath)
{
    static_cast<AnimatedDocument *>(document())->setBasePath(filePath);
}

void MarkdownBrowser::setMarkdown(const QString &markdown)
{
    document()->setMarkdown(markdown);
    document()->setDefaultFont(Utils::font(contentTF));

    for (QTextBlock block = document()->begin(); block != document()->end(); block = block.next()) {
        QTextBlockFormat blockFormat = block.blockFormat();
        // Leave images as they are.
        if (block.text().contains(QChar::ObjectReplacementCharacter))
            continue;

        if (blockFormat.hasProperty(QTextFormat::HeadingLevel)) {
            blockFormat.setTopMargin(SpacingTokens::ExVPaddingGapXl);
            blockFormat.setBottomMargin(SpacingTokens::VGapL);

            // Add anchors to headings. This should actually be done by Qt QTBUG-120518
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
        } else
            blockFormat.setLineHeight(lineHeight(contentTF), QTextBlockFormat::FixedHeight);

        QTextCursor cursor(block);
        if (blockFormat.hasProperty(QTextFormat::BlockCodeLanguage)) {
            QString language = blockFormat.stringProperty(QTextFormat::BlockCodeLanguage);
            highlightCodeBlock(document(), block, language);
            continue;
        }

        cursor.mergeBlockFormat(blockFormat);
        const TextFormat &headingTf
            = markdownHeadingFormats[qBound(0, blockFormat.headingLevel() - 1, 5)];

        const QFont headingFont = Utils::font(headingTf);
        for (auto it = block.begin(); !(it.atEnd()); ++it) {
            QTextFragment fragment = it.fragment();
            if (fragment.isValid()) {
                QTextCharFormat charFormat = fragment.charFormat();
                cursor.setPosition(fragment.position());
                cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
                if (blockFormat.hasProperty(QTextFormat::HeadingLevel)) {
                    // Change font size adjustment to resemble the heading font size
                    // (We want the font size to be relative to the default font size,
                    // otherwise the heading size won't respect the font scale factor)
                    charFormat.setProperty(
                        QTextFormat::FontSizeAdjustment,
                        headingFont.pointSizeF() / document()->defaultFont().pointSizeF());

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

} // namespace Utils

#include "markdownbrowser.moc"
