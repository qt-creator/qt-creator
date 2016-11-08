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

#include "../nimconstants.h"
#include "../nimplugin.h"

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
    setDisplayName(tr(Nim::Constants::C_EDITOR_DISPLAY_NAME));
    addMimeType(QLatin1String(Nim::Constants::C_NIM_MIMETYPE));
    addMimeType(QLatin1String(Nim::Constants::C_NIM_SCRIPT_MIMETYPE));

    setEditorActionHandlers(TextEditorActionHandler::Format
                            | TextEditorActionHandler::UnCommentSelection
                            | TextEditorActionHandler::UnCollapseAll);

    setEditorWidgetCreator([]{
        auto result = new TextEditorWidget();
        result->setLanguageSettingsId(Nim::Constants::C_NIMLANGUAGE_ID);
        return result;
    });
    setDocumentCreator([]() {
        return new TextDocument(Constants::C_NIMEDITOR_ID);
    });
    setIndenterCreator([]() {
        return new NimIndenter;
    });
    setSyntaxHighlighterCreator([]() {
        return new NimHighlighter;
    });
    setCommentStyle(CommentDefinition::HashStyle);
    setParenthesesMatchingEnabled(true);
    setMarksVisible(false);
    setCodeFoldingSupported(true);
    setMarksVisible(true);
}

Core::IEditor *NimEditorFactory::createEditor()
{
    return TextEditorFactory::createEditor();
}

}
