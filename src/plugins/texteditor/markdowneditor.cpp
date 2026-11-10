// Copyright (C) 2023 Tasuku Suzuki
// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdowneditor.h"

#include "editorwithpreview.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditortr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <utils/action.h>
#include <utils/link.h>
#include <utils/markdownbrowser.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QRegularExpression>
#include <QToolButton>

using namespace Core;
using namespace Utils;

namespace TextEditor::Internal {

const char MARKDOWNVIEWER_ID_PREFIX[] = "Markdown";
const char MARKDOWNVIEWER_MIME_TYPE[] = "text/markdown";
const char EMPHASIS_ACTION[] = "Markdown.Emphasis";
const char STRONG_ACTION[] = "Markdown.Strong";
const char INLINECODE_ACTION[] = "Markdown.InlineCode";
const char LINK_ACTION[] = "Markdown.Link";

class MarkdownEditorWidget : public TextEditorWidget
{
    Q_OBJECT
public:
    using TextEditorWidget::TextEditorWidget;

    void findLinkAt(const QTextCursor &cursor,
                    const Utils::LinkHandler &processLinkCallback,
                    bool resolveTarget = true,
                    bool inNextSplit = false) override;
};

class MarkdownEditor : public EditorWithPreview
{
    Q_OBJECT
public:
    MarkdownEditor()
        : MarkdownEditor(TextDocumentPtr(
            new TextDocument(EditorWithPreview::editorId(MARKDOWNVIEWER_ID_PREFIX))))
    {
        TextDocument *document = textDocument();
        document->setMimeType(MARKDOWNVIEWER_MIME_TYPE);
        connect(document, &TextDocument::mimeTypeChanged, document, &TextDocument::changed);
    }

    explicit MarkdownEditor(const TextDocumentPtr &doc)
        : EditorWithPreview(doc,
                            new MarkdownEditorWidget,
                            new MarkdownBrowser,
                            MARKDOWNVIEWER_ID_PREFIX)
        , m_previewWidget(static_cast<MarkdownBrowser *>(previewWidget()))
    {
        // preview
        m_previewWidget->setWheelZoomEnabled(true);
        m_previewWidget->setFrameShape(QFrame::NoFrame);
        m_previewWidget->setShowRulersForHeadings(true);
        connect(m_previewWidget,
                &MarkdownBrowser::openFileRequested,
                this,
                [this](Link link) {
                    // Absolute path or relative (to the document): open in Qt Creator.
                    link.targetFilePath = document()->filePath().parentDir().resolvePath(
                        link.targetFilePath);
                    EditorManager::openEditorAt(link);
                });

        // editor
        TextEditorWidget *textEditor = textEditorWidget();
        textEditor->setOptionalActions(OptionalActions::FollowSymbolUnderCursor);
        textEditor->setupGenericHighlighter();
        textEditor->setMarksVisible(false);

        QList<QToolButton *> markDownButtons;
        auto button = Command::createToolButtonWithShortcutToolTip(EMPHASIS_ACTION);
        button->defaultAction()->setIconText("i");
        button->setFont([button]{ auto f = button->font(); f.setItalic(true); return f; }());
        connect(button, &QToolButton::clicked, this, &MarkdownEditor::triggerEmphasis);
        markDownButtons.append(button);
        button = Command::createToolButtonWithShortcutToolTip(STRONG_ACTION);
        button->defaultAction()->setIconText("b");
        button->setFont([button]{ auto f = button->font(); f.setBold(true); return f; }());
        connect(button, &QToolButton::clicked, this, &MarkdownEditor::triggerStrong);
        markDownButtons.append(button);
        button = Command::createToolButtonWithShortcutToolTip(INLINECODE_ACTION);
        button->defaultAction()->setIconText("`");
        connect(button, &QToolButton::clicked, this, &MarkdownEditor::triggerInlineCode);
        markDownButtons.append(button);
        button = Command::createToolButtonWithShortcutToolTip(LINK_ACTION);
        button->setIcon(Utils::Icons::LINK_TOOLBAR.icon());
        connect(button, &QToolButton::clicked, this, &MarkdownEditor::triggerLink);
        markDownButtons.append(button);
        for (auto it = markDownButtons.rbegin(); it != markDownButtons.rend(); ++it)
            insertEditorToolBarWidget(TextEditorWidget::Left, *it);
    }

    BaseTextEditor *duplicate() override
    {
        auto other = new MarkdownEditor(textDocumentPtr());
        other->restoreState(saveState());
        emit editorDuplicated(other);
        return other;
    }

    void triggerEmphasis()
    {
        triggerFormatingAction([](QString *selectedText, int *cursorOffset) {
            if (selectedText->isEmpty()) {
                *selectedText = QStringLiteral("**");
                *cursorOffset = -1;
            } else {
                *selectedText = QStringLiteral("*%1*").arg(*selectedText);
            }
        });
    }
    void triggerStrong()
    {
        triggerFormatingAction([](QString *selectedText, int *cursorOffset) {
            if (selectedText->isEmpty()) {
                *selectedText = QStringLiteral("****");
                *cursorOffset = -2;
            } else {
                *selectedText = QStringLiteral("**%1**").arg(*selectedText);
            }
        });
    }
    void triggerInlineCode()
    {
        triggerFormatingAction([](QString *selectedText, int *cursorOffset) {
            if (selectedText->isEmpty()) {
                *selectedText = QStringLiteral("``");
                *cursorOffset = -1;
            } else {
                *selectedText = QStringLiteral("`%1`").arg(*selectedText);
            }
        });
    }
    void triggerLink()
    {
        triggerFormatingAction([](QString *selectedText, int *cursorOffset, int *selectionLength) {
            if (selectedText->isEmpty()) {
                *selectedText = QStringLiteral("[](https://)");
                *cursorOffset = -11; // ](https://) is 11 chars
            } else {
                *selectedText = QStringLiteral("[%1](https://)").arg(*selectedText);
                *cursorOffset = -1;
                *selectionLength = -8; // https:// is 8 chars
            }
        });
    }

    void increasePreviewZoom() override { m_previewWidget->increaseZoom(); }
    void decreasePreviewZoom() override { m_previewWidget->decreaseZoom(); }
    void resetPreviewZoom() override { m_previewWidget->resetZoom(); }

protected:
    void renderPreview() override { m_previewWidget->setMarkdown(textDocument()->plainText()); }

private:
    void triggerFormatingAction(std::function<void(QString *selectedText, int *cursorOffset)> action)
    {
        auto formattedText = textEditorWidget()->selectedText();
        int cursorOffset = 0;
        action(&formattedText, &cursorOffset);
        format(formattedText, cursorOffset);
    }
    void triggerFormatingAction(std::function<void(QString *selectedText, int *cursorOffset, int *selectionLength)> action)
    {
        auto formattedText = textEditorWidget()->selectedText();
        int cursorOffset = 0;
        int selectionLength = 0;
        action(&formattedText, &cursorOffset, &selectionLength);
        format(formattedText, cursorOffset, selectionLength);
    }
    void format(const QString &formattedText, int cursorOffset = 0, int selectionLength = 0)
    {
        TextEditorWidget *textEditor = textEditorWidget();
        auto cursor = textEditor->textCursor();
        int start = cursor.selectionStart();
        int end = cursor.selectionEnd();
        cursor.setPosition(start, QTextCursor::MoveAnchor);
        cursor.setPosition(end, QTextCursor::KeepAnchor);

        cursor.insertText(formattedText);
        if (cursorOffset != 0) {
            auto pos = cursor.position();
            cursor.setPosition(pos + cursorOffset);
            textEditor->setTextCursor(cursor);
        }

        if (selectionLength != 0) {
            cursor.setPosition(cursor.position(), QTextCursor::MoveAnchor);
            cursor.setPosition(cursor.position() + selectionLength, QTextCursor::KeepAnchor);
            textEditor->setTextCursor(cursor);
        }
    }

    Utils::MarkdownBrowser *m_previewWidget;
};

class MarkdownEditorFactory final : public IEditorFactory
{
public:
    MarkdownEditorFactory();

private:
    Action m_emphasisAction;
    Action m_strongAction;
    Action m_inlineCodeAction;
    Action m_linkAction;
    EditorWithPreviewActions m_previewActions{MARKDOWNVIEWER_ID_PREFIX};
};

MarkdownEditorFactory::MarkdownEditorFactory()
{
    setId(EditorWithPreview::editorId(MARKDOWNVIEWER_ID_PREFIX));
    setDisplayName(Tr::tr("Markdown Editor"));
    addMimeType(MARKDOWNVIEWER_MIME_TYPE);
    setEditorCreator([] { return new MarkdownEditor; });

    const auto textContext = Context(EditorWithPreview::textContextId(MARKDOWNVIEWER_ID_PREFIX));

    ActionBuilder(nullptr, EMPHASIS_ACTION)
        .adopt(&m_emphasisAction)
        .setText(Tr::tr("Emphasis"))
        .setContext(textContext)
        .addOnTriggered(EditorManager::instance(), [] {
            auto editor = qobject_cast<MarkdownEditor *>(EditorManager::currentEditor());
            if (editor)
                editor->triggerEmphasis();
        });

    ActionBuilder(nullptr, STRONG_ACTION)
        .adopt(&m_strongAction)
        .setText("Strong")
        .setContext(textContext)
        .addOnTriggered(EditorManager::instance(), [] {
            auto editor = qobject_cast<MarkdownEditor *>(EditorManager::currentEditor());
            if (editor)
                editor->triggerStrong();
        });

    ActionBuilder(nullptr, INLINECODE_ACTION)
        .adopt(&m_inlineCodeAction)
        .setText(Tr::tr("Inline Code"))
        .setContext(textContext)
        .addOnTriggered(EditorManager::instance(), [] {
            auto editor = qobject_cast<MarkdownEditor *>(EditorManager::currentEditor());
            if (editor)
                editor->triggerInlineCode();
        });

    ActionBuilder(nullptr, LINK_ACTION)
        .adopt(&m_linkAction)
        .setText(Tr::tr("Hyperlink"))
        .setContext(textContext)
        .addOnTriggered(EditorManager::instance(), [] {
            auto editor = qobject_cast<MarkdownEditor *>(EditorManager::currentEditor());
            if (editor)
                editor->triggerLink();
        });
}

void MarkdownEditorWidget::findLinkAt(const QTextCursor &cursor,
                                      const LinkHandler &processLinkCallback,
                                      bool /*resolveTarget*/,
                                      bool /*inNextSplit*/)
{
    static const QStringView CAPTURE_GROUP_LINK   = u"link";
    static const QStringView CAPTURE_GROUP_ANCHOR = u"anchor";
    static const QStringView CAPTURE_GROUP_RAWURL = u"rawurl";

    static const QRegularExpression markdownLink{
        R"(\[[^[\]]*\]\((?<link>.+?)\))"
        R"(|(?<anchor>\[\^[^\]]+\])(?:[^:]|$))"
        R"(|(?<rawurl>(?:https?|ftp)\://[^">)\s]+))"
    };

    QTC_ASSERT(markdownLink.isValid(), return);

    const int blockOffset = cursor.block().position();
    const QString &currentBlock = cursor.block().text();

    for (const QRegularExpressionMatch &match : markdownLink.globalMatch(currentBlock)) {
        // Ignore matches outside of the current cursor position.
        if (cursor.positionInBlock() < match.capturedStart())
            break;
        if (cursor.positionInBlock() >= match.capturedEnd())
            continue;

        if (const QStringView link = match.capturedView(CAPTURE_GROUP_LINK); !link.isEmpty()) {
            // Process regular Markdown links of the form `[description](link)`.

            const QUrl url(link.toString());
            Link result = MarkdownBrowser::fileLink(url);
            if (result.hasValidTarget()) {
                result.targetFilePath = textDocument()->filePath().parentDir().resolvePath(
                    result.targetFilePath);
                if (!result.targetFilePath.isFile())
                    continue;
            } else if (!url.scheme().isEmpty()) {
                result.targetFilePath = FilePath::fromString(url.toString());
            } else {
                continue;
            }
            result.linkTextStart = match.capturedStart() + blockOffset;
            result.linkTextEnd = match.capturedEnd() + blockOffset;
            processLinkCallback(result);
            break;
        } else if (const QStringView anchor = match.capturedView(CAPTURE_GROUP_ANCHOR);
                   !anchor.isEmpty()) {
            // Process local anchor links of the form `[^footnote]` that point
            // to anchors in the current document: `[^footnote]: Description`.

            const QTextCursor target = cursor.document()->find(anchor + u':');

            if (target.isNull())
                continue;

            int line = 0;
            int column = 0;

            convertPosition(target.position(), &line, &column);
            Link result{textDocument()->filePath(), line, column};
            result.linkTextStart = match.capturedStart(CAPTURE_GROUP_ANCHOR) + blockOffset;
            result.linkTextEnd = match.capturedEnd(CAPTURE_GROUP_ANCHOR) + blockOffset;
            processLinkCallback(result);
            break;
        } else if (const QStringView rawurl = match.capturedView(CAPTURE_GROUP_RAWURL);
                   !rawurl.isEmpty()) {
            // Process raw links starting with "http://", "https://", or "ftp://".

            Link result{FilePath::fromString(rawurl.toString())};
            result.linkTextStart = match.capturedStart() + blockOffset;
            result.linkTextEnd = match.capturedEnd() + blockOffset;
            processLinkCallback(result);
            break;
        } else {
            QTC_ASSERT_STRING("This line should not be reached unless 'markdownLink' is wrong");
            return;
        }
    }
}

void setupMarkdownEditor()
{
    static MarkdownEditorFactory theMarkdownEditorFactory;
}

} // namespace TextEditor::Internal

#include "markdowneditor.moc"
