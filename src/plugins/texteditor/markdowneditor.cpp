// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdowneditor.h"

#include "textdocument.h"
#include "texteditor.h"
#include "texteditortr.h"

#include <aggregation/aggregate.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>

#include <utils/qtcsettings.h>
#include <utils/qtcsettings.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QHBoxLayout>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
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
        m_previewWidget->setOpenExternalLinks(true);
        m_previewWidget->setFrameShape(QFrame::NoFrame);
        new Utils::MarkdownHighlighter(m_previewWidget->document());

        // editor
        m_textEditorWidget = new TextEditorWidget;
        m_textEditorWidget->setTextDocument(m_document);
        m_textEditorWidget->setupGenericHighlighter();
        m_textEditorWidget->setMarksVisible(false);
        auto context = new IContext(this);
        context->setWidget(m_textEditorWidget);
        context->setContext(Context(MARKDOWNVIEWER_TEXT_CONTEXT));
        ICore::addContextObject(context);

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

        m_togglePreviewVisible = new QToolButton;
        m_togglePreviewVisible->setText(Tr::tr("Show Preview"));
        m_togglePreviewVisible->setCheckable(true);
        m_togglePreviewVisible->setChecked(showPreview);
        m_previewWidget->setVisible(showPreview);

        m_toggleEditorVisible = new QToolButton;
        m_toggleEditorVisible->setText(Tr::tr("Show Editor"));
        m_toggleEditorVisible->setCheckable(true);
        m_toggleEditorVisible->setChecked(showEditor);
        m_textEditorWidget->setVisible(showEditor);

        auto button = new CommandButton(EMPHASIS_ACTION);
        button->setText("i");
        button->setFont([button]{ auto f = button->font(); f.setItalic(true); return f; }());
        connect(button, &QToolButton::clicked, this, &MarkdownEditor::triggerEmphasis);
        m_markDownButtons.append(button);
        button = new CommandButton(STRONG_ACTION);
        button->setText("b");
        button->setFont([button]{ auto f = button->font(); f.setBold(true); return f; }());
        connect(button, &QToolButton::clicked, this, &MarkdownEditor::triggerStrong);
        m_markDownButtons.append(button);
        button = new CommandButton(INLINECODE_ACTION);
        button->setText("`");
        connect(button, &QToolButton::clicked, this, &MarkdownEditor::triggerInlineCode);
        m_markDownButtons.append(button);
        button = new CommandButton(LINK_ACTION);
        button->setIcon(Utils::Icons::LINK_TOOLBAR.icon());
        connect(button, &QToolButton::clicked, this, &MarkdownEditor::triggerLink);
        m_markDownButtons.append(button);
        for (auto button : m_markDownButtons) {
            // do not call setVisible(true) at this point, this destroys the hover effect on macOS
            if (!showEditor)
                button->setVisible(false);
        }

        auto swapViews = new QToolButton;
        swapViews->setText(Tr::tr("Swap Views"));
        swapViews->setEnabled(showEditor && showPreview);

        m_toolbarLayout = new QHBoxLayout(&m_toolbar);
        m_toolbarLayout->setSpacing(0);
        m_toolbarLayout->setContentsMargins(0, 0, 0, 0);
        for (auto button : m_markDownButtons)
            m_toolbarLayout->addWidget(button);
        m_toolbarLayout->addStretch();
        m_toolbarLayout->addWidget(m_togglePreviewVisible);
        m_toolbarLayout->addWidget(m_toggleEditorVisible);
        m_toolbarLayout->addWidget(swapViews);

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

            m_previewWidget->horizontalScrollBar()->setValue(positions.x());
            m_previewWidget->verticalScrollBar()->setValue(positions.y());
        };

        const auto viewToggled =
            [swapViews](QWidget *view, bool visible, QWidget *otherView, QToolButton *otherButton) {
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
                swapViews->setEnabled(view->isVisible() && otherView->isVisible());
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

        connect(swapViews, &QToolButton::clicked, m_textEditorWidget, [this] {
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
        QWidget *leftButton = textEditorRight ? m_togglePreviewVisible : m_toggleEditorVisible;
        QWidget *rightButton = textEditorRight ? m_toggleEditorVisible : m_togglePreviewVisible;
        const int rightIndex = m_toolbarLayout->count() - 2;
        m_toolbarLayout->insertWidget(rightIndex, leftButton);
        m_toolbarLayout->insertWidget(rightIndex, rightButton);
    }

    QWidget *toolBar() override { return &m_toolbar; }

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

private:
    QTimer m_previewTimer;
    bool m_performDelayedUpdate = false;
    MiniSplitter *m_splitter;
    QTextBrowser *m_previewWidget;
    TextEditorWidget *m_textEditorWidget;
    TextDocumentPtr m_document;
    QWidget m_toolbar;
    QHBoxLayout *m_toolbarLayout;
    QList<QToolButton *> m_markDownButtons;
    QToolButton *m_toggleEditorVisible;
    QToolButton *m_togglePreviewVisible;
    std::optional<QPoint> m_previewRestoreScrollPosition;
};

MarkdownEditorFactory::MarkdownEditorFactory()
    : m_actionHandler(MARKDOWNVIEWER_ID,
                      MARKDOWNVIEWER_TEXT_CONTEXT,
                      TextEditor::TextEditorActionHandler::None,
                      [](IEditor *editor) {
                          return static_cast<MarkdownEditor *>(editor)->textEditorWidget();
                      })
{
    setId(MARKDOWNVIEWER_ID);
    setDisplayName(::Core::Tr::tr("Markdown Editor"));
    addMimeType(MARKDOWNVIEWER_MIME_TYPE);
    setEditorCreator([] { return new MarkdownEditor; });

    const auto textContext = Context(MARKDOWNVIEWER_TEXT_CONTEXT);
    Command *cmd = nullptr;
    cmd = ActionManager::registerAction(&m_emphasisAction, EMPHASIS_ACTION, textContext);
    cmd->setDescription(Tr::tr("Emphasis"));
    QObject::connect(&m_emphasisAction, &QAction::triggered, EditorManager::instance(), [] {
        auto editor = qobject_cast<MarkdownEditor *>(EditorManager::currentEditor());
        if (editor)
            editor->triggerEmphasis();
    });
    cmd = ActionManager::registerAction(&m_strongAction, STRONG_ACTION, textContext);
    cmd->setDescription(Tr::tr("Strong"));
    QObject::connect(&m_strongAction, &QAction::triggered, EditorManager::instance(), [] {
        auto editor = qobject_cast<MarkdownEditor *>(EditorManager::currentEditor());
        if (editor)
            editor->triggerStrong();
    });
    cmd = ActionManager::registerAction(&m_inlineCodeAction, INLINECODE_ACTION, textContext);
    cmd->setDescription(Tr::tr("Inline Code"));
    QObject::connect(&m_inlineCodeAction, &QAction::triggered, EditorManager::instance(), [] {
        auto editor = qobject_cast<MarkdownEditor *>(EditorManager::currentEditor());
        if (editor)
            editor->triggerInlineCode();
    });
    cmd = ActionManager::registerAction(&m_linkAction, LINK_ACTION, textContext);
    cmd->setDescription(Tr::tr("Hyper Link"));
    QObject::connect(&m_linkAction, &QAction::triggered, EditorManager::instance(), [] {
        auto editor = qobject_cast<MarkdownEditor *>(EditorManager::currentEditor());
        if (editor)
            editor->triggerLink();
    });
}

} // namespace TextEditor::Internal

#include "markdowneditor.moc"
