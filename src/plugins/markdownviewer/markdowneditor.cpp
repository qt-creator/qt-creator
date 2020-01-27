// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdowneditor.h"
#include "markdownviewerconstants.h"
#include "markdowndocument.h"

#include <coreplugin/icore.h>
#include <texteditor/textdocument.h>

namespace Markdown {
namespace Internal {

MarkdownEditor::MarkdownEditor(QWidget *parent)
    : TextEditor::TextEditorWidget(parent)
{
    setTextDocument(TextEditor::TextDocumentPtr(new MarkdownDocument));
    setupGenericHighlighter();
    setMarksVisible(false);

    auto context = new Core::IContext(this);
    context->setWidget(this);
    context->setContext(Core::Context(Constants::MARKDOWNVIEWER_EDITOR_CONTEXT));
    Core::ICore::addContextObject(context);
}

} // namespace Internal
} // namespace Markdown
