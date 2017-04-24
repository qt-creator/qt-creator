/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include "pythoneditor.h"
#include "pythoneditorconstants.h"
#include "pythoneditorplugin.h"
#include "pythonindenter.h"
#include "pythonhighlighter.h"

#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/textdocument.h>

#include <utils/qtcassert.h>

#include <QCoreApplication>

using namespace TextEditor;

namespace PythonEditor {
namespace Internal {

PythonEditorFactory::PythonEditorFactory()
{
    setId(Constants::C_PYTHONEDITOR_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", Constants::C_EDITOR_DISPLAY_NAME));
    addMimeType(Constants::C_PY_MIMETYPE);

    setEditorActionHandlers(TextEditorActionHandler::Format
                       | TextEditorActionHandler::UnCommentSelection
                       | TextEditorActionHandler::UnCollapseAll);

    setDocumentCreator([] { return new TextDocument(Constants::C_PYTHONEDITOR_ID); });
    setIndenterCreator([] { return new PythonIndenter; });
    setSyntaxHighlighterCreator([] { return new PythonHighlighter; });
    setCommentDefinition(Utils::CommentDefinition::HashStyle);
    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
}

} // namespace Internal
} // namespace PythonEditor
