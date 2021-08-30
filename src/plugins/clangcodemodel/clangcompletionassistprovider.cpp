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

#include "clangcompletionassistprovider.h"

#include "clangcompletionassistprocessor.h"
#include "clangeditordocumentprocessor.h"
#include "clangutils.h"

#include <cplusplus/Token.h>
#include <cppeditor/cppcompletionassistprocessor.h>
#include <cppeditor/projectpart.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistprovider.h>
#include <texteditor/texteditor.h>

#include <utils/qtcassert.h>

namespace ClangCodeModel {
namespace Internal {

ClangCompletionAssistProvider::ClangCompletionAssistProvider(BackendCommunicator &communicator,
                                                             CompletionType type)
    : m_communicator(communicator), m_type(type)
{
}

TextEditor::IAssistProvider::RunType ClangCompletionAssistProvider::runType() const
{
    return Asynchronous;
}

TextEditor::IAssistProcessor *ClangCompletionAssistProvider::createProcessor() const
{
    return new ClangCompletionAssistProcessor;
}

TextEditor::AssistInterface *ClangCompletionAssistProvider::createAssistInterface(
    const Utils::FilePath &filePath,
    const TextEditor::TextEditorWidget *textEditorWidget,
    const CPlusPlus::LanguageFeatures & /*languageFeatures*/,
    int position,
    TextEditor::AssistReason reason) const
{
    const CppEditor::ProjectPart::ConstPtr projectPart = projectPartForFileBasedOnProcessor(
        filePath.toString());
    if (projectPart) {
        return new ClangCompletionAssistInterface(m_communicator,
                                                  m_type,
                                                  textEditorWidget,
                                                  position,
                                                  filePath,
                                                  reason,
                                                  projectPart->headerPaths,
                                                  projectPart->languageFeatures);
    }

    return nullptr;
}

} // namespace Internal
} // namespace ClangCodeModel
