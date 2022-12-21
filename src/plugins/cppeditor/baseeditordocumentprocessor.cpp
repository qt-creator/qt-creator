// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baseeditordocumentprocessor.h"

#include "cppcodemodelsettings.h"
#include "cppmodelmanager.h"
#include "cpptoolsreuse.h"
#include "editordocumenthandle.h"

#include <projectexplorer/session.h>
#include <texteditor/quickfix.h>

namespace CppEditor {

/*!
    \class CppEditor::BaseEditorDocumentProcessor

    \brief The BaseEditorDocumentProcessor class controls and executes all
           document relevant actions (reparsing, semantic highlighting, additional
           semantic calculations) after a text document has changed.
*/

BaseEditorDocumentProcessor::BaseEditorDocumentProcessor(QTextDocument *textDocument,
                                                         const Utils::FilePath &filePath)
    : m_filePath(filePath),
      m_textDocument(textDocument)
{
}

BaseEditorDocumentProcessor::~BaseEditorDocumentProcessor() = default;

void BaseEditorDocumentProcessor::run(bool projectsUpdated)
{
    const Utils::Language languagePreference = codeModelSettings()->interpretAmbigiousHeadersAsCHeaders()
            ? Utils::Language::C
            : Utils::Language::Cxx;

    runImpl({CppModelManager::instance()->workingCopy(),
             ProjectExplorer::SessionManager::startupProject(),
             languagePreference,
             projectsUpdated});
}

TextEditor::QuickFixOperations
BaseEditorDocumentProcessor::extraRefactoringOperations(const TextEditor::AssistInterface &)
{
    return TextEditor::QuickFixOperations();
}

void BaseEditorDocumentProcessor::invalidateDiagnostics()
{
}

void BaseEditorDocumentProcessor::setParserConfig(
        const BaseEditorDocumentParser::Configuration &config)
{
    parser()->setConfiguration(config);
}

void BaseEditorDocumentProcessor::runParser(QFutureInterface<void> &future,
                                            BaseEditorDocumentParser::Ptr parser,
                                            BaseEditorDocumentParser::UpdateParams updateParams)
{
    future.setProgressRange(0, 1);
    if (future.isCanceled()) {
        future.setProgressValue(1);
        return;
    }

    parser->update(future, updateParams);
    CppModelManager::instance()->finishedRefreshingSourceFiles({parser->filePath().toString()});

    future.setProgressValue(1);
}

} // namespace CppEditor
