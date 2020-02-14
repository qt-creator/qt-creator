/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "nimeditorfactory.h"
#include "nimindenter.h"
#include "nimhighlighter.h"
#include "nimcompletionassistprovider.h"

#include "../nimconstants.h"
#include "../nimplugin.h"
#include "nimtexteditorwidget.h"

#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/textdocument.h>
#include <utils/qtcassert.h>

using namespace TextEditor;
using namespace Utils;

namespace Nim {

NimEditorFactory::NimEditorFactory()
{
    setId(Constants::C_NIMEDITOR_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", "Nim Editor"));
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

}
