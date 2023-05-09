// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdowneditor.h"

#include "textdocument.h"
#include "texteditor.h"
#include "texteditortr.h"

#include <aggregation/aggregate.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <utils/stringutils.h>

#include <QHBoxLayout>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>

namespace TextEditor::Internal {

const char MARKDOWNVIEWER_ID[] = "Editors.MarkdownViewer";
const char MARKDOWNVIEWER_TEXT_CONTEXT[] = "Editors.MarkdownViewer.Text";
const char MARKDOWNVIEWER_MIME_TYPE[] = "text/markdown";
const char MARKDOWNVIEWER_TEXTEDITOR_RIGHT[] = "Markdown.TextEditorRight";
const bool kTextEditorRightDefault = true;

class MarkdownEditor : public Core::IEditor
{
public:
    MarkdownEditor()
        : m_document(new TextDocument(MARKDOWNVIEWER_ID))
    {
        m_document->setMimeType(MARKDOWNVIEWER_MIME_TYPE);

        QSettings *s = Core::ICore::settings();
        const bool textEditorRight
            = s->value(MARKDOWNVIEWER_TEXTEDITOR_RIGHT, kTextEditorRightDefault).toBool();

        m_splitter = new Core::MiniSplitter;

        // preview
        auto browser = new QTextBrowser();
        browser->setOpenExternalLinks(true);
        browser->setFrameShape(QFrame::NoFrame);
        new Utils::MarkdownHighlighter(browser->document());

        // editor
        m_textEditorWidget = new TextEditorWidget;
        m_textEditorWidget->setTextDocument(m_document);
        m_textEditorWidget->setupGenericHighlighter();
        m_textEditorWidget->setMarksVisible(false);
        auto context = new Core::IContext(this);
        context->setWidget(m_textEditorWidget);
        context->setContext(Core::Context(MARKDOWNVIEWER_TEXT_CONTEXT));
        Core::ICore::addContextObject(context);

        if (textEditorRight) {
            m_splitter->addWidget(browser);
            m_splitter->addWidget(m_textEditorWidget);
        } else {
            m_splitter->addWidget(m_textEditorWidget);
            m_splitter->addWidget(browser);
        }

        setContext(Core::Context(MARKDOWNVIEWER_ID));

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

        auto togglePreviewVisible = new QToolButton;
        togglePreviewVisible->setText(Tr::tr("Show Preview"));
        togglePreviewVisible->setCheckable(true);
        togglePreviewVisible->setChecked(true);

        m_toggleEditorVisible = new QToolButton;
        m_toggleEditorVisible->setText(Tr::tr("Show Editor"));
        m_toggleEditorVisible->setCheckable(true);
        m_toggleEditorVisible->setChecked(true);

        auto swapViews = new QToolButton;
        swapViews->setText(Tr::tr("Swap Views"));

        auto toolbarLayout = new QHBoxLayout(&m_toolbar);
        toolbarLayout->setSpacing(0);
        toolbarLayout->setContentsMargins(0, 0, 0, 0);
        toolbarLayout->addStretch();
        if (textEditorRight) {
            toolbarLayout->addWidget(togglePreviewVisible);
            toolbarLayout->addWidget(m_toggleEditorVisible);
        } else {
            toolbarLayout->addWidget(m_toggleEditorVisible);
            toolbarLayout->addWidget(togglePreviewVisible);
        }
        toolbarLayout->addWidget(swapViews);

        connect(m_document.data(),
                &TextDocument::mimeTypeChanged,
                m_document.data(),
                &TextDocument::changed);

        const auto updatePreview = [this, browser] {
            QHash<QScrollBar *, int> positions;
            const auto scrollBars = browser->findChildren<QScrollBar *>();

            // save scroll positions
            for (QScrollBar *scrollBar : scrollBars)
                positions.insert(scrollBar, scrollBar->value());

            browser->setMarkdown(m_document->plainText());

            // restore scroll positions
            for (QScrollBar *scrollBar : scrollBars)
                scrollBar->setValue(positions.value(scrollBar));
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

        connect(m_toggleEditorVisible,
                &QToolButton::toggled,
                this,
                [this, browser, togglePreviewVisible, viewToggled](bool visible) {
                    viewToggled(m_textEditorWidget, visible, browser, togglePreviewVisible);
                });
        connect(togglePreviewVisible,
                &QToolButton::toggled,
                this,
                [this, browser, viewToggled, updatePreview](bool visible) {
                    viewToggled(browser, visible, m_textEditorWidget, m_toggleEditorVisible);
                    if (visible && m_performDelayedUpdate) {
                        m_performDelayedUpdate = false;
                        updatePreview();
                    }
                });

        connect(swapViews, &QToolButton::clicked, m_textEditorWidget, [this, toolbarLayout] {
            QTC_ASSERT(m_splitter->count() > 1, return);
            // switch views
            auto placeholder = std::make_unique<QWidget>();
            auto second = m_splitter->replaceWidget(1, placeholder.get());
            auto first = m_splitter->replaceWidget(0, second);
            m_splitter->replaceWidget(1, first);
            // switch buttons
            const int rightIndex = toolbarLayout->count() - 2;
            QLayoutItem *right = toolbarLayout->takeAt(rightIndex);
            toolbarLayout->insertItem(rightIndex - 1, right);
            // save settings
            Utils::QtcSettings *s = Core::ICore::settings();
            s->setValueWithDefault(MARKDOWNVIEWER_TEXTEDITOR_RIGHT,
                                   !s->value(MARKDOWNVIEWER_TEXTEDITOR_RIGHT,
                                             kTextEditorRightDefault)
                                        .toBool(),
                                   kTextEditorRightDefault);
        });

        // TODO directly update when we build with Qt 6.5.2
        m_previewTimer.setInterval(500);
        m_previewTimer.setSingleShot(true);
        connect(&m_previewTimer, &QTimer::timeout, this, [this, togglePreviewVisible, updatePreview] {
            if (togglePreviewVisible->isChecked())
                updatePreview();
            else
                m_performDelayedUpdate = true;
        });

        connect(m_document->document(), &QTextDocument::contentsChanged, &m_previewTimer, [this] {
            m_previewTimer.start();
        });
    }

    QWidget *toolBar() override { return &m_toolbar; }

    Core::IDocument *document() const override { return m_document.data(); }
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
            else
                m_splitter->widget(0)->setFocus();
            return true;
        }
        return Core::IEditor::eventFilter(obj, ev);
    }

private:
    QTimer m_previewTimer;
    bool m_performDelayedUpdate = false;
    Core::MiniSplitter *m_splitter;
    TextEditorWidget *m_textEditorWidget;
    TextDocumentPtr m_document;
    QWidget m_toolbar;
    QToolButton *m_toggleEditorVisible;
};

MarkdownEditorFactory::MarkdownEditorFactory()
    : m_actionHandler(MARKDOWNVIEWER_ID,
                      MARKDOWNVIEWER_TEXT_CONTEXT,
                      TextEditor::TextEditorActionHandler::None,
                      [](Core::IEditor *editor) {
                          return static_cast<MarkdownEditor *>(editor)->textEditorWidget();
                      })
{
    setId(MARKDOWNVIEWER_ID);
    setDisplayName(::Core::Tr::tr("Markdown Viewer"));
    addMimeType(MARKDOWNVIEWER_MIME_TYPE);
    setEditorCreator([] { return new MarkdownEditor; });
}

} // namespace TextEditor::Internal
