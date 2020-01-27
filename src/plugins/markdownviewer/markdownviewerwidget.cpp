// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdownviewerwidget.h"
#include "markdowneditor.h"
#include "markdownbrowser.h"

#include <texteditor/textdocument.h>

namespace Markdown {
namespace Internal {

struct MarkdownViewerWidgetPrivate
{
    MarkdownBrowser *browser;
    MarkdownEditor *editor;
};

MarkdownViewerWidget::MarkdownViewerWidget()
    : d(new MarkdownViewerWidgetPrivate)
{
    d->browser = new MarkdownBrowser(this);
    d->browser->setOpenExternalLinks(true);
    d->browser->setFrameShape(QFrame::NoFrame);
    d->editor = new MarkdownEditor(this);

    connect(d->editor->document(), &QTextDocument::contentsChanged,
            this, [this]() {
        const auto plainText = d->editor->textDocument()->plainText();
        d->browser->setMarkdown(plainText);
    });
}

MarkdownViewerWidget::~MarkdownViewerWidget()
{
    delete d;
}

Core::IDocument *MarkdownViewerWidget::document() const
{
    return d->editor->textDocument();
}

bool MarkdownViewerWidget::isEditorVisible() const
{
    return d->editor->isVisible();
}

void MarkdownViewerWidget::setEditorVisible(bool editorVisible)
{
    if (d->editor->isVisible() == editorVisible)
        return;
    d->editor->setVisible(editorVisible);
    emit editorVisibleChanged(editorVisible);
}

} // namespace Internal
} // namespace Markdown
