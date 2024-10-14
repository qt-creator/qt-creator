// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdownbrowser.h"

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
    int startBlockNumner = block.blockNumber();

    // Find the end of the code block ...
    QTextBlock curBlock = block;
    while (curBlock.isValid()) {
        curBlock = curBlock.next();
        const auto valid = curBlock.isValid();
        const auto hasProp = curBlock.blockFormat().hasProperty(QTextFormat::BlockCodeLanguage);
        const auto prop = curBlock.blockFormat().stringProperty(QTextFormat::BlockCodeLanguage);
        if (!valid || !hasProp || prop != language)
            break;
    }
    // Get the text of the code block and erase it
    QTextCursor eraseCursor(document);
    eraseCursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, startBlockNumner);
    eraseCursor.movePosition(
        QTextCursor::NextBlock,
        QTextCursor::KeepAnchor,
        curBlock.blockNumber() - startBlockNumner - 1);
    eraseCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    QString code = eraseCursor.selectedText();
    eraseCursor.deleteChar();

    // Create a new Frame and insert the highlighted code ...
    block = document->findBlockByNumber(startBlockNumner);

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

    class Entry
    {
    public:
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

        std::unique_ptr<Entry> entry = std::make_unique<Entry>(data);

        if (entry->movie.frameCount() > 1) {
            connect(&entry->movie, &QMovie::frameChanged, this, [this]() { m_redraw(); });
            entry->movie.start();
        }
        if (m_entries.insert(name, entry.release(), qMax(1, data.size())))
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

            const LoopList iterator(m_urlsToLoad);

            auto onQuerySetup = [iterator](NetworkQuery &query) {
                query.setRequest(QNetworkRequest(*iterator));
                query.setNetworkAccessManager(NetworkAccessManager::instance());
            };

            auto onQueryDone = [this](const NetworkQuery &query, DoneWith result) {
                if (result == DoneWith::Cancel)
                    return;
                m_urlsToLoad.removeOne(query.reply()->url());

                if (result == DoneWith::Success)
                    m_imageHandler.set(query.reply()->url().toString(), query.reply()->readAll());
                else
                    m_imageHandler.set(query.reply()->url().toString(), {});

                markContentsDirty(0, this->characterCount());
            };

            // clang-format off
            Group group {
                For(iterator) >> Do {
                    continueOnError,
                    NetworkQueryTask{onQuerySetup, onQueryDone},
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

private:
    AnimatedImageHandler m_imageHandler;
    QList<QUrl> m_urlsToLoad;
    bool m_needsToRestartLoading = false;
    Tasking::TaskTreeRunner m_imageLoaderTree;
};

MarkdownBrowser::MarkdownBrowser(QWidget *parent)
    : QTextBrowser(parent)
{
    setDocument(new AnimatedDocument(this));
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
                    // We don't use font size adjustment for headings
                    charFormat.clearProperty(QTextFormat::FontSizeAdjustment);
                    charFormat.setFontCapitalization(headingFont.capitalization());
                    charFormat.setFontFamilies(headingFont.families());
                    charFormat.setFontPointSize(headingFont.pointSizeF());
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
