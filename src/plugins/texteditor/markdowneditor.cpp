// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdowneditor.h"

#include "textdocument.h"
#include "texteditor.h"
#include "texteditortr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <utils/stringutils.h>

#include <QHBoxLayout>
#include <QScrollBar>
#include <QTextBrowser>
#include <QToolButton>

namespace TextEditor::Internal {

const char MARKDOWNVIEWER_ID[] = "Editors.MarkdownViewer";
const char MARKDOWNVIEWER_TEXT_CONTEXT[] = "Editors.MarkdownViewer.Text";
const char MARKDOWNVIEWER_MIME_TYPE[] = "text/markdown";

class MarkdownEditor : public Core::IEditor
{
public:
    MarkdownEditor()
        : m_document(new TextDocument(MARKDOWNVIEWER_ID))
    {
        m_document->setMimeType(MARKDOWNVIEWER_MIME_TYPE);

        // Left side
        auto browser = new QTextBrowser(&m_widget);
        browser->setOpenExternalLinks(true);
        browser->setFrameShape(QFrame::NoFrame);
        new Utils::MarkdownHighlighter(browser->document());

        // Right side (hidable)
        m_textEditorWidget = new TextEditorWidget(&m_widget);
        m_textEditorWidget->setTextDocument(m_document);
        m_textEditorWidget->setupGenericHighlighter();
        m_textEditorWidget->setMarksVisible(false);
        auto context = new Core::IContext(this);
        context->setWidget(m_textEditorWidget);
        context->setContext(Core::Context(MARKDOWNVIEWER_TEXT_CONTEXT));
        Core::ICore::addContextObject(context);

        setContext(Core::Context(MARKDOWNVIEWER_ID));
        setWidget(&m_widget);

        auto toggleEditorVisible = new QToolButton;
        toggleEditorVisible->setText(Tr::tr("Hide Editor"));
        toggleEditorVisible->setCheckable(true);
        toggleEditorVisible->setChecked(true);

        auto layout = new QHBoxLayout(&m_toolbar);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addStretch();
        layout->addWidget(toggleEditorVisible);

        connect(m_document.data(), &TextDocument::mimeTypeChanged,
                m_document.data(), &TextDocument::changed);

        connect(toggleEditorVisible,
                &QToolButton::toggled,
                m_textEditorWidget,
                [this, browser, toggleEditorVisible](bool editorVisible) {
                    if (m_textEditorWidget->isVisible() == editorVisible)
                        return;
                    m_textEditorWidget->setVisible(editorVisible);
                    if (editorVisible)
                        m_textEditorWidget->setFocus();
                    else
                        browser->setFocus();

                    toggleEditorVisible->setText(editorVisible ? Tr::tr("Hide Editor")
                                                               : Tr::tr("Show Editor"));
                });

        connect(m_document->document(), &QTextDocument::contentsChanged, this, [this, browser] {
            QHash<QScrollBar *, int> positions;
            const auto scrollBars = browser->findChildren<QScrollBar *>();

            // save scroll positions
            for (QScrollBar *scrollBar : scrollBars)
                positions.insert(scrollBar, scrollBar->value());

            browser->setMarkdown(m_document->plainText());

            // restore scroll positions
            for (QScrollBar *scrollBar : scrollBars)
                scrollBar->setValue(positions.value(scrollBar));
        });
    }

    QWidget *toolBar() override { return &m_toolbar; }

    Core::IDocument *document() const override { return m_document.data(); }
    TextEditorWidget *textEditorWidget() const { return m_textEditorWidget; }

private:
    Core::MiniSplitter m_widget;
    TextEditorWidget *m_textEditorWidget;
    TextDocumentPtr m_document;
    QWidget m_toolbar;
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

} // TextEditor::Internal
