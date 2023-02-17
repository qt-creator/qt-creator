// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimeditorfactory.h"
#include "nimindenter.h"
#include "nimhighlighter.h"
#include "nimcompletionassistprovider.h"

#include "../nimconstants.h"
#include "nimtexteditorwidget.h"

#include <coreplugin/coreplugintr.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <utils/qtcassert.h>

using namespace TextEditor;
using namespace Utils;

namespace Nim {

NimEditorFactory::NimEditorFactory()
{
    setId(Constants::C_NIMEDITOR_ID);
    setDisplayName(::Core::Tr::tr("Nim Editor"));
    addMimeType(QLatin1String(Nim::Constants::C_NIM_MIMETYPE));
    addMimeType(QLatin1String(Nim::Constants::C_NIM_SCRIPT_MIMETYPE));

    setEditorActionHandlers(TextEditorActionHandler::Format
                            | TextEditorActionHandler::UnCommentSelection
                            | TextEditorActionHandler::UnCollapseAll
                            | TextEditorActionHandler::FollowSymbolUnderCursor);
    setEditorWidgetCreator([]{
        return new NimTextEditorWidget();
    });
    setDocumentCreator([]() {
        return new TextDocument(Constants::C_NIMEDITOR_ID);
    });
    setIndenterCreator([](QTextDocument *doc) {
        return new NimIndenter(doc);
    });
    setSyntaxHighlighterCreator([]() {
        return new NimHighlighter;
    });
    setCompletionAssistProvider(new NimCompletionAssistProvider());
    setCommentDefinition(CommentDefinition::HashStyle);
    setParenthesesMatchingEnabled(true);
    setCodeFoldingSupported(true);
}

void NimEditorFactory::decorateEditor(TextEditorWidget *editor)
{
    editor->textDocument()->setSyntaxHighlighter(new NimHighlighter());
    editor->textDocument()->setIndenter(new NimIndenter(editor->textDocument()->document()));
}

} // Nim
