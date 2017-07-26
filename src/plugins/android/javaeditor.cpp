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

#include "javaeditor.h"
#include "javaindenter.h"
#include "androidconstants.h"

#include <texteditor/codeassist/keywordscompletionassist.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <texteditor/normalindenter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditor.h>

#include <extensionsystem/pluginmanager.h>
#include <utils/fileutils.h>
#include <utils/uncommentselection.h>

#include <QFileInfo>

namespace Android {
namespace Internal {

static TextEditor::TextDocument *createJavaDocument()
{
    auto doc = new TextEditor::TextDocument;
    doc->setId(Constants::JAVA_EDITOR_ID);
    doc->setMimeType(QLatin1String(Constants::JAVA_MIMETYPE));
    doc->setIndenter(new JavaIndenter);
    return doc;
}

//
// JavaEditorFactory
//

JavaEditorFactory::JavaEditorFactory()
{
    static QStringList keywords = {
        "abstract", "assert", "boolean", "break", "byte", "case", "catch", "char", "class", "const",
        "continue", "default", "do", "double", "else", "enum", "extends", "final", "finally",
        "float", "for", "goto", "if", "implements", "import", "instanceof", "int", "interface",
        "long", "native", "new", "package", "private", "protected", "public", "return", "short",
        "static", "strictfp", "super", "switch", "synchronized", "this", "throw", "throws",
        "transient", "try", "void", "volatile", "while"
    };
    setId(Constants::JAVA_EDITOR_ID);
    setDisplayName(tr("Java Editor"));
    addMimeType(Constants::JAVA_MIMETYPE);

    setDocumentCreator(createJavaDocument);
    setUseGenericHighlighter(true);
    setCommentDefinition(Utils::CommentDefinition::CppStyle);
    setEditorActionHandlers(TextEditor::TextEditorActionHandler::UnCommentSelection);
    setMarksVisible(true);
    setCompletionAssistProvider(new TextEditor::KeywordsCompletionAssistProvider(keywords));
}

} // namespace Internal
} // namespace Android
