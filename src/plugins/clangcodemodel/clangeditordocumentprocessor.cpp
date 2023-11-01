// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangeditordocumentprocessor.h"

#include "clangdclient.h"
#include "clangmodelmanagersupport.h"

#include <cppeditor/builtincursorinfo.h>
#include <cppeditor/builtineditordocumentparser.h>
#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/compileroptionsbuilder.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/cppworkingcopy.h>
#include <cppeditor/editordocumenthandle.h>

#include <languageclient/languageclientmanager.h>

#include <texteditor/fontsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <cplusplus/CppDocument.h>

#include <utils/algorithm.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>

#include <QTextBlock>
#include <QWidget>

namespace ClangCodeModel {
namespace Internal {

ClangEditorDocumentProcessor::ClangEditorDocumentProcessor(TextEditor::TextDocument *document)
    : BuiltinEditorDocumentProcessor(document), m_document(*document)
{
    connect(parser().data(), &CppEditor::BaseEditorDocumentParser::projectPartInfoUpdated,
            this, &BaseEditorDocumentProcessor::projectPartInfoUpdated);
    connect(static_cast<CppEditor::BuiltinEditorDocumentParser *>(parser().data()),
            &CppEditor::BuiltinEditorDocumentParser::finished,
            this, [this] {
        emit parserConfigChanged(filePath(), parserConfig());
    });
    setSemanticHighlightingChecker([this] {
        return !ClangModelManagerSupport::clientForFile(m_document.filePath());
    });
}

void ClangEditorDocumentProcessor::semanticRehighlight()
{
    const auto matchesEditor = [this](const Core::IEditor *editor) {
        return editor->document()->filePath() == m_document.filePath();
    };
    if (!Utils::contains(Core::EditorManager::visibleEditors(), matchesEditor))
        return;
    if (ClangModelManagerSupport::clientForFile(m_document.filePath()))
        return;
    BuiltinEditorDocumentProcessor::semanticRehighlight();
}

void ClangEditorDocumentProcessor::setParserConfig(
        const CppEditor::BaseEditorDocumentParser::Configuration &config)
{
    CppEditor::BuiltinEditorDocumentProcessor::setParserConfig(config);
    emit parserConfigChanged(filePath(), config);
}

CppEditor::BaseEditorDocumentParser::Configuration ClangEditorDocumentProcessor::parserConfig()
{
    return parser()->configuration();
}

ClangEditorDocumentProcessor *ClangEditorDocumentProcessor::get(const Utils::FilePath &filePath)
{
    return qobject_cast<ClangEditorDocumentProcessor*>(
                CppEditor::CppModelManager::cppEditorDocumentProcessor(filePath));
}

void ClangEditorDocumentProcessor::forceUpdate(TextEditor::TextDocument *doc)
{
    if (const auto client = qobject_cast<ClangdClient *>(
            LanguageClient::LanguageClientManager::clientForDocument(doc))) {
        client->documentContentsChanged(doc, 0, 0, 0);
    }
}

} // namespace Internal
} // namespace ClangCodeModel
