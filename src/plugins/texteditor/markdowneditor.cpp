// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdowneditor.h"

#include "textdocument.h"
#include "texteditor.h"
#include "texteditortr.h"

#include <aggregation/aggregate.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>

#include <utils/action.h>
#include <utils/ranges.h>
#include <utils/qtcsettings.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

#include <optional>

using namespace Core;
using namespace Utils;

namespace TextEditor::Internal {

const char MARKDOWNVIEWER_ID[] = "Editors.MarkdownViewer";
const char MARKDOWNVIEWER_TEXT_CONTEXT[] = "Editors.MarkdownViewer.Text";
const char MARKDOWNVIEWER_MIME_TYPE[] = "text/markdown";
const char MARKDOWNVIEWER_TEXTEDITOR_RIGHT[] = "Markdown.TextEditorRight";
const char MARKDOWNVIEWER_SHOW_EDITOR[] = "Markdown.ShowEditor";
const char MARKDOWNVIEWER_SHOW_PREVIEW[] = "Markdown.ShowPreview";
const bool kTextEditorRightDefault = false;
const bool kShowEditorDefault = true;
const bool kShowPreviewDefault = true;
const char EMPHASIS_ACTION[] = "Markdown.Emphasis";
const char STRONG_ACTION[] = "Markdown.Strong";
const char INLINECODE_ACTION[] = "Markdown.InlineCode";
const char LINK_ACTION[] = "Markdown.Link";
const char TOGGLEEDITOR_ACTION[] = "Markdown.ToggleEditor";
const char TOGGLEPREVIEW_ACTION[] = "Markdown.TogglePreview";
const char SWAPVIEWS_ACTION[] = "Markdown.SwapViews";

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

class MarkdownEditor : public IEditor
{
    Q_OBJECT
public:
    MarkdownEditor()
        : m_document(new TextDocument(MARKDOWNVIEWER_ID))
    {
        m_document->setMimeType(MARKDOWNVIEWER_MIME_TYPE);

        QtcSettings *s = ICore::settings();
        const bool textEditorRight
            = s->value(MARKDOWNVIEWER_TEXTEDITOR_RIGHT, kTextEditorRightDefault).toBool();
        const bool showPreview = s->value(MARKDOWNVIEWER_SHOW_PREVIEW, kShowPreviewDefault).toBool();
        const bool showEditor = s->value(MARKDOWNVIEWER_SHOW_EDITOR, kShowEditorDefault).toBool()
                                || !showPreview; // ensure at least one is visible

        m_splitter = new MiniSplitter;

        // preview
        m_previewWidget = new QTextBrowser();
        m_previewWidget->setOpenLinks(false); // we want to open files in QtC, not the browser
        m_previewWidget->setFrameShape(QFrame::NoFrame);
        new Utils::MarkdownHighlighter(m_previewWidget->document());
        connect(m_previewWidget, &QTextBrowser::anchorClicked, this, [this](const QUrl &link) {
            if (link.hasFragment() && link.path().isEmpty() && link.scheme().isEmpty()) {
                // local anchor
                m_previewWidget->scrollToAnchor(link.fragment(QUrl::FullyEncoded));
            } else if (link.isLocalFile() || link.scheme().isEmpty()) {
                // absolute path or relative (to the document)
                // open in Qt Creator
                EditorManager::openEditor(
                    document()->filePath().parentDir().resolvePath(link.path()));
            } else {
                QDesktopServices::openUrl(link);
            }
        });

        // editor
        m_textEditorWidget = new MarkdownEditorWidget;
        m_textEditorWidget->setOptionalActions(OptionalActions::FollowSymbolUnderCursor);
        m_textEditorWidget->setTextDocument(m_document);
        m_textEditorWidget->setupGenericHighlighter();
        m_textEditorWidget->setMarksVisible(false);
        QObject::connect(
            m_textEditorWidget,
            &TextEditorWidget::saveCurrentStateForNavigationHistory,
            this,
            &MarkdownEditor::saveCurrentStateForNavigationHistory);
        QObject::connect(
            m_textEditorWidget,
            &TextEditorWidget::addSavedStateToNavigationHistory,
            this,
            &MarkdownEditor::addSavedStateToNavigationHistory);
        QObject::connect(
            m_textEditorWidget,
            &TextEditorWidget::addCurrentStateToNavigationHistory,
            this,
            &MarkdownEditor::addCurrentStateToNavigationHistory);

        IContext::attach(m_textEditorWidget, Context(MARKDOWNVIEWER_TEXT_CONTEXT));

        m_splitter->addWidget(m_textEditorWidget); // sets splitter->focusWidget() on non-Windows
        m_splitter->addWidget(m_previewWidget);

        setContext(Context(MARKDOWNVIEWER_ID));

        auto widget = new QWidget;
        auto layout = new QVBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        widget->setLayout(layout);
        layout->addWidget(m_splitter);
        setWidget(widget);
        m_widget->installEventFilter(this);
        using namespace Aggregation;
        Aggregate *agg = Aggregate::parentAggregate(m_textEditorWidget);
        if (!agg) {
            agg = new Aggregate;
            agg->add(m_textEditorWidget);
        }
        agg->add(m_widget.get());

        m_togglePreviewVisible = Command::createToolButtonWithShortcutToolTip(TOGGLEPREVIEW_ACTION);
        m_togglePreviewVisible->defaultAction()->setCheckable(true);
        m_togglePreviewVisible->setChecked(showPreview);
        m_previewWidget->setVisible(showPreview);

        m_toggleEditorVisible = Command::createToolButtonWithShortcutToolTip(TOGGLEEDITOR_ACTION);
        m_toggleEditorVisible->defaultAction()->setCheckable(true);
        m_toggleEditorVisible->setChecked(showEditor);
        m_textEditorWidget->setVisible(showEditor);

        auto button = Command::createToolButtonWithShortcutToolTip(EMPHASIS_ACTION);
        button->defaultAction()->setIconText("i");
        button->setFont([button]{ auto f = button->font(); f.setItalic(true); return f; }());
        connect(button, &QToolButton::clicked, this, &MarkdownEditor::triggerEmphasis);
        m_markDownButtons.append(button);
        button = Command::createToolButtonWithShortcutToolTip(STRONG_ACTION);
        button->defaultAction()->setIconText("b");
        button->setFont([button]{ auto f = button->font(); f.setBold(true); return f; }());
        connect(button, &QToolButton::clicked, this, &MarkdownEditor::triggerStrong);
        m_markDownButtons.append(button);
        button = Command::createToolButtonWithShortcutToolTip(INLINECODE_ACTION);
        button->defaultAction()->setIconText("`");
        connect(button, &QToolButton::clicked, this, &MarkdownEditor::triggerInlineCode);
        m_markDownButtons.append(button);
        button = Command::createToolButtonWithShortcutToolTip(LINK_ACTION);
        button->setIcon(Utils::Icons::LINK_TOOLBAR.icon());
        connect(button, &QToolButton::clicked, this, &MarkdownEditor::triggerLink);
        m_markDownButtons.append(button);
        for (auto button : m_markDownButtons) {
            // do not call setVisible(true) at this point, this destroys the hover effect on macOS
            if (!showEditor)
                button->setVisible(false);
        }

        for (auto button : m_markDownButtons | Utils::views::reverse)
            m_textEditorWidget->insertExtraToolBarWidget(TextEditorWidget::Left, button);

        m_swapViews = Command::createToolButtonWithShortcutToolTip(SWAPVIEWS_ACTION);
        m_swapViews->setEnabled(showEditor && showPreview);

        m_swapViewsAction = m_textEditorWidget->insertExtraToolBarWidget(TextEditorWidget::Right, m_swapViews);
        m_toggleEditorVisibleAction = m_textEditorWidget->insertExtraToolBarWidget(TextEditorWidget::Right, m_toggleEditorVisible);
        m_togglePreviewVisibleAction = m_textEditorWidget->insertExtraToolBarWidget(TextEditorWidget::Right, m_togglePreviewVisible);
        setWidgetOrder(textEditorRight);

        connect(m_document.data(),
                &TextDocument::mimeTypeChanged,
                m_document.data(),
                &TextDocument::changed);

        const auto updatePreview = [this] {
            // save scroll positions
            const QPoint positions = m_previewRestoreScrollPosition
                                         ? *m_previewRestoreScrollPosition
                                         : QPoint(m_previewWidget->horizontalScrollBar()->value(),
                                                  m_previewWidget->verticalScrollBar()->value());
            m_previewRestoreScrollPosition.reset();

            m_previewWidget->setMarkdown(m_document->plainText());
            // Add anchors to headings. This should actually be done by Qt QTBUG-120518
            for (QTextBlock block = m_previewWidget->document()->begin(); block.isValid();
                 block = block.next()) {
                QTextBlockFormat fmt = block.blockFormat();
                if (fmt.hasProperty(QTextFormat::HeadingLevel)) {
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

            m_previewWidget->horizontalScrollBar()->setValue(positions.x());
            m_previewWidget->verticalScrollBar()->setValue(positions.y());
        };

        const auto viewToggled =
            [this](QWidget *view, bool visible, QWidget *otherView, QToolButton *otherButton) {
                if (view->isVisible() == visible)
                    return;
                view->setVisible(visible);
                if (visible) {
                    view->setFocus();
                } else if (otherView->isVisible()) {
                    otherView->setFocus();
                } else {
                    // make sure at least one view is visible
                    otherButton->toggle();
                }
                m_swapViews->setEnabled(view->isVisible() && otherView->isVisible());
            };
        const auto saveViewSettings = [this] {
            Utils::QtcSettings *s = ICore::settings();
            s->setValueWithDefault(MARKDOWNVIEWER_SHOW_PREVIEW,
                                   m_togglePreviewVisible->isChecked(),
                                   kShowPreviewDefault);
            s->setValueWithDefault(MARKDOWNVIEWER_SHOW_EDITOR,
                                   m_toggleEditorVisible->isChecked(),
                                   kShowEditorDefault);
        };

        connect(m_toggleEditorVisible,
                &QToolButton::toggled,
                this,
                [this, viewToggled, saveViewSettings](bool visible) {
                    viewToggled(m_textEditorWidget,
                                visible,
                                m_previewWidget,
                                m_togglePreviewVisible);
                    for (auto button : m_markDownButtons)
                        button->setVisible(visible);
                    saveViewSettings();
                });
        connect(m_togglePreviewVisible,
                &QToolButton::toggled,
                this,
                [this, viewToggled, updatePreview, saveViewSettings](bool visible) {
                    viewToggled(m_previewWidget, visible, m_textEditorWidget, m_toggleEditorVisible);
                    if (visible && m_performDelayedUpdate) {
                        m_performDelayedUpdate = false;
                        updatePreview();
                    }
                    saveViewSettings();
                });

        connect(m_swapViews, &QToolButton::clicked, m_textEditorWidget, [this] {
            const bool textEditorRight = isTextEditorRight();
            setWidgetOrder(!textEditorRight);
            // save settings
            Utils::QtcSettings *s = ICore::settings();
            s->setValueWithDefault(MARKDOWNVIEWER_TEXTEDITOR_RIGHT,
                                   !textEditorRight,
                                   kTextEditorRightDefault);
        });

        // TODO directly update when we build with Qt 6.5.2
        m_previewTimer.setInterval(500);
        m_previewTimer.setSingleShot(true);
        connect(&m_previewTimer, &QTimer::timeout, this, [this, updatePreview] {
            if (m_togglePreviewVisible->isChecked())
                updatePreview();
            else
                m_performDelayedUpdate = true;
        });

        connect(m_document->document(), &QTextDocument::contentsChanged, &m_previewTimer, [this] {
            m_previewTimer.start();
        });
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

    void toggleEditor() { m_toggleEditorVisible->toggle(); }
    void togglePreview() { m_togglePreviewVisible->toggle(); }
    void swapViews() { m_swapViews->click(); }

    bool isTextEditorRight() const { return m_splitter->widget(0) == m_previewWidget; }

    void setWidgetOrder(bool textEditorRight)
    {
        QTC_ASSERT(m_splitter->count() > 1, return);
        QWidget *left = textEditorRight ? static_cast<QWidget *>(m_previewWidget)
                                        : m_textEditorWidget;
        QWidget *right = textEditorRight ? static_cast<QWidget *>(m_textEditorWidget)
                                         : m_previewWidget;
        m_splitter->insertWidget(0, left);
        m_splitter->insertWidget(1, right);
        // buttons
        const auto leftAction = textEditorRight ? m_togglePreviewVisibleAction : m_toggleEditorVisibleAction;
        const auto rightAction = textEditorRight ? m_toggleEditorVisibleAction : m_togglePreviewVisibleAction;
        m_textEditorWidget->toolBar()->insertAction(m_swapViewsAction, leftAction);
        m_textEditorWidget->toolBar()->insertAction(m_swapViewsAction, rightAction);
    }

    QWidget *toolBar() override { return m_textEditorWidget->toolBarWidget(); }

    IDocument *document() const override { return m_document.data(); }
    TextEditorWidget *textEditorWidget() const { return m_textEditorWidget; }
    int currentLine() const override { return textEditorWidget()->textCursor().blockNumber() + 1; };
    int currentColumn() const override
    {
        QTextCursor cursor = textEditorWidget()->textCursor();
        return cursor.position() - cursor.block().position() + 1;
    }
    void gotoLine(int line, int column, bool centerLine) override
    {
        if (!m_toggleEditorVisible->isChecked())
            m_toggleEditorVisible->toggle();
        textEditorWidget()->gotoLine(line, column, centerLine);
    }

    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (obj == m_widget && ev->type() == QEvent::FocusIn) {
            if (m_splitter->focusWidget())
                m_splitter->focusWidget()->setFocus();
            else if (m_textEditorWidget->isVisible())
                m_textEditorWidget->setFocus();
            else
                m_splitter->widget(0)->setFocus();
            return true;
        }
        return IEditor::eventFilter(obj, ev);
    }

    QByteArray saveState() const override
    {
        QByteArray state;
        QDataStream stream(&state, QIODevice::WriteOnly);
        stream << 1; // version number
        stream << m_textEditorWidget->saveState();
        stream << m_previewWidget->horizontalScrollBar()->value();
        stream << m_previewWidget->verticalScrollBar()->value();
        stream << isTextEditorRight();
        stream << m_togglePreviewVisible->isChecked();
        stream << m_toggleEditorVisible->isChecked();
        stream << m_splitter->saveState();
        return state;
    }

    void restoreState(const QByteArray &state) override
    {
        if (state.isEmpty())
            return;
        int version;
        QByteArray editorState;
        int previewHV;
        int previewVV;
        bool textEditorRight;
        bool previewShown;
        bool textEditorShown;
        QByteArray splitterState;
        QDataStream stream(state);
        stream >> version;
        stream >> editorState;
        stream >> previewHV;
        stream >> previewVV;
        stream >> textEditorRight;
        stream >> previewShown;
        stream >> textEditorShown;
        stream >> splitterState;
        m_textEditorWidget->restoreState(editorState);
        m_previewRestoreScrollPosition.emplace(previewHV, previewVV);
        setWidgetOrder(textEditorRight);
        m_splitter->restoreState(splitterState);
        m_togglePreviewVisible->setChecked(previewShown);
        // ensure at least one is shown
        m_toggleEditorVisible->setChecked(textEditorShown || !previewShown);
    }

private:
    void triggerFormatingAction(std::function<void(QString *selectedText, int *cursorOffset)> action)
    {
        auto formattedText = m_textEditorWidget->selectedText();
        int cursorOffset = 0;
        action(&formattedText, &cursorOffset);
        format(formattedText, cursorOffset);
    }
    void triggerFormatingAction(std::function<void(QString *selectedText, int *cursorOffset, int *selectionLength)> action)
    {
        auto formattedText = m_textEditorWidget->selectedText();
        int cursorOffset = 0;
        int selectionLength = 0;
        action(&formattedText, &cursorOffset, &selectionLength);
        format(formattedText, cursorOffset, selectionLength);
    }
    void format(const QString &formattedText, int cursorOffset = 0, int selectionLength = 0)
    {
        auto cursor = m_textEditorWidget->textCursor();
        int start = cursor.selectionStart();
        int end = cursor.selectionEnd();
        cursor.setPosition(start, QTextCursor::MoveAnchor);
        cursor.setPosition(end, QTextCursor::KeepAnchor);

        cursor.insertText(formattedText);
        if (cursorOffset != 0) {
            auto pos = cursor.position();
            cursor.setPosition(pos + cursorOffset);
            m_textEditorWidget->setTextCursor(cursor);
        }

        if (selectionLength != 0) {
            cursor.setPosition(cursor.position(), QTextCursor::MoveAnchor);
            cursor.setPosition(cursor.position() + selectionLength, QTextCursor::KeepAnchor);
            m_textEditorWidget->setTextCursor(cursor);
        }
    }

    void saveCurrentStateForNavigationHistory() { m_savedNavigationState = saveState(); }

    void addSavedStateToNavigationHistory()
    {
        EditorManager::addCurrentPositionToNavigationHistory(m_savedNavigationState);
    }

    void addCurrentStateToNavigationHistory()
    {
        EditorManager::addCurrentPositionToNavigationHistory();
    }

private:
    QTimer m_previewTimer;
    bool m_performDelayedUpdate = false;
    MiniSplitter *m_splitter;
    QTextBrowser *m_previewWidget;
    TextEditorWidget *m_textEditorWidget;
    TextDocumentPtr m_document;
    QList<QToolButton *> m_markDownButtons;
    QToolButton *m_toggleEditorVisible;
    QToolButton *m_togglePreviewVisible;
    QToolButton *m_swapViews;
    QAction *m_toggleEditorVisibleAction;
    QAction *m_togglePreviewVisibleAction;
    QAction *m_swapViewsAction;
    std::optional<QPoint> m_previewRestoreScrollPosition;
    QByteArray m_savedNavigationState;
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
    Action m_toggleEditorAction;
    Action m_togglePreviewAction;
    Action m_swapAction;
};

MarkdownEditorFactory::MarkdownEditorFactory()
{
    setId(MARKDOWNVIEWER_ID);
    setDisplayName(::Core::Tr::tr("Markdown Editor"));
    addMimeType(MARKDOWNVIEWER_MIME_TYPE);
    setEditorCreator([] { return new MarkdownEditor; });

    const auto textContext = Context(MARKDOWNVIEWER_TEXT_CONTEXT);
    const auto context = Context(MARKDOWNVIEWER_ID);

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

    ActionBuilder(nullptr, TOGGLEEDITOR_ACTION)
        .adopt(&m_toggleEditorAction)
        .setText(Tr::tr("Show Editor"))
        .setContext(context)
        .addOnTriggered(EditorManager::instance(), [] {
            auto editor = qobject_cast<MarkdownEditor *>(EditorManager::currentEditor());
            if (editor)
                editor->toggleEditor();
        });

    ActionBuilder(nullptr, TOGGLEPREVIEW_ACTION)
        .adopt(&m_togglePreviewAction)
        .setText(Tr::tr("Show Preview"))
        .setContext(context)
        .addOnTriggered(EditorManager::instance(), [] {
            auto editor = qobject_cast<MarkdownEditor *>(EditorManager::currentEditor());
            if (editor)
                editor->togglePreview();
        });

    ActionBuilder(nullptr, SWAPVIEWS_ACTION)
        .adopt(&m_swapAction)
        .setText(Tr::tr("Swap Views"))
        .setContext(context)
        .addOnTriggered(EditorManager::instance(), [] {
            auto editor = qobject_cast<MarkdownEditor *>(EditorManager::currentEditor());
            if (editor)
                editor->swapViews();
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

            const QUrl url = link.toString();
            const auto linkedPath = [this, url] {
                if (url.isRelative())
                    return textDocument()->filePath().parentDir().resolvePath(url.path());
                else if (url.isLocalFile())
                    return FilePath::fromString(url.toLocalFile());
                else if (!url.scheme().isEmpty())
                    return FilePath::fromString(url.toString());
                else
                    return FilePath{};
            }();

            if (!linkedPath.isFile() && url.scheme().isEmpty())
                continue;

            Link result = linkedPath;
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
