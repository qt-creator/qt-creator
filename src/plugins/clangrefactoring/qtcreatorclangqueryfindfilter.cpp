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

#include "qtcreatorclangqueryfindfilter.h"

#include <texteditor/textdocument.h>

#include <cpptools/cppmodelmanager.h>
#include <cpptools/baseeditordocumentparser.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

namespace ClangRefactoring {

QtCreatorClangQueryFindFilter::QtCreatorClangQueryFindFilter(ClangBackEnd::RefactoringServerInterface &server,
                                                             SearchInterface &searchInterface,
                                                             RefactoringClient &refactoringClient)
    : ClangQueryCurrentFileFindFilter(server, searchInterface, refactoringClient)
{
}

namespace  {
CppTools::CppModelManager *cppToolManager()
{
    return CppTools::CppModelManager::instance();
}

bool isCppEditor(Core::IEditor *currentEditor)
{
    return cppToolManager()->isCppEditor(currentEditor);
}

CppTools::ProjectPart::Ptr projectPartForFile(const QString &filePath)
{
    if (const auto parser = CppTools::BaseEditorDocumentParser::get(filePath))
        return parser->projectPart();
    return CppTools::ProjectPart::Ptr();
}

}

void QtCreatorClangQueryFindFilter::findAll(const QString &queryText, Core::FindFlags findFlags)
{
    prepareFind();

    ClangQueryCurrentFileFindFilter::findAll(queryText, findFlags);
}

void QtCreatorClangQueryFindFilter::prepareFind()
{
    Core::IEditor *currentEditor = Core::EditorManager::currentEditor();

    if (isCppEditor(currentEditor)) {
        Core::IDocument *currentDocument = currentEditor->document();
        auto currentTextDocument = static_cast<TextEditor::TextDocument*>(currentDocument);
        const QString filePath = currentDocument->filePath().toString();

        setCurrentDocumentFilePath(filePath);
        setCurrentDocumentRevision(currentTextDocument->document()->revision());
        setProjectPart(projectPartForFile(filePath));

        if (currentTextDocument->isModified())
            setUnsavedDocumentContent(currentTextDocument->document()->toPlainText());
    }
}

} // namespace ClangRefactoring
