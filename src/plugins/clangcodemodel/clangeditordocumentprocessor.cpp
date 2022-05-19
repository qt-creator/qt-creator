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

#include "clangeditordocumentprocessor.h"

#include "clangdiagnostictooltipwidget.h"
#include "clangfixitoperation.h"
#include "clangmodelmanagersupport.h"
#include "clangutils.h"

#include <cppeditor/builtincursorinfo.h>
#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/compileroptionsbuilder.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/cppworkingcopy.h>
#include <cppeditor/editordocumenthandle.h>

#include <texteditor/fontsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <cplusplus/CppDocument.h>

#include <utils/algorithm.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QTextBlock>
#include <QVBoxLayout>
#include <QWidget>

namespace ClangCodeModel {
namespace Internal {

ClangEditorDocumentProcessor::ClangEditorDocumentProcessor(TextEditor::TextDocument *document)
    : BuiltinEditorDocumentProcessor(document), m_document(*document)
{
    connect(parser().data(), &CppEditor::BaseEditorDocumentParser::projectPartInfoUpdated,
            this, &BaseEditorDocumentProcessor::projectPartInfoUpdated);
    setSemanticHighlightingChecker([this] {
        return !ClangModelManagerSupport::instance()->clientForFile(m_document.filePath());
    });
}

void ClangEditorDocumentProcessor::semanticRehighlight()
{
    const auto matchesEditor = [this](const Core::IEditor *editor) {
        return editor->document()->filePath() == m_document.filePath();
    };
    if (!Utils::contains(Core::EditorManager::visibleEditors(), matchesEditor))
        return;
    if (ClangModelManagerSupport::instance()->clientForFile(m_document.filePath()))
        return;
    BuiltinEditorDocumentProcessor::semanticRehighlight();
}

bool ClangEditorDocumentProcessor::hasProjectPart() const
{
    return !m_projectPart.isNull();
}

CppEditor::ProjectPart::ConstPtr ClangEditorDocumentProcessor::projectPart() const
{
    return m_projectPart;
}

void ClangEditorDocumentProcessor::clearProjectPart()
{
    m_projectPart.clear();
}

::Utils::Id ClangEditorDocumentProcessor::diagnosticConfigId() const
{
    return m_diagnosticConfigId;
}

void ClangEditorDocumentProcessor::setParserConfig(
        const CppEditor::BaseEditorDocumentParser::Configuration &config)
{
    CppEditor::BuiltinEditorDocumentProcessor::setParserConfig(config);
    emit parserConfigChanged(Utils::FilePath::fromString(filePath()), config);
}

CppEditor::BaseEditorDocumentParser::Configuration ClangEditorDocumentProcessor::parserConfig()
{
    return parser()->configuration();
}

ClangEditorDocumentProcessor *ClangEditorDocumentProcessor::get(const QString &filePath)
{
    return qobject_cast<ClangEditorDocumentProcessor*>(
                CppEditor::CppModelManager::cppEditorDocumentProcessor(filePath));
}

} // namespace Internal
} // namespace ClangCodeModel
