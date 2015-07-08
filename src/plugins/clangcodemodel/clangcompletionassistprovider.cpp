/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangcompletionassistprovider.h"

#include "clangcompletionassistprocessor.h"
#include "clangutils.h"
#include "pchmanager.h"

#include <cplusplus/Token.h>
#include <cpptools/cppcompletionassistprocessor.h>
#include <cpptools/cppprojects.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistprovider.h>
#include <texteditor/texteditor.h>

#include <utils/qtcassert.h>

namespace ClangCodeModel {
namespace Internal {

ClangCompletionAssistProvider::ClangCompletionAssistProvider(IpcCommunicator &ipcCommunicator)
    : m_ipcCommunicator(ipcCommunicator)
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
        const QString &filePath,
        const TextEditor::TextEditorWidget *textEditorWidget,
        const CPlusPlus::LanguageFeatures &/*languageFeatures*/,
        int position,
        TextEditor::AssistReason reason) const
{
    const CppTools::ProjectPart::Ptr projectPart = Utils::projectPartForFile(filePath);
    QTC_ASSERT(!projectPart.isNull(), return 0);

    const PchInfo::Ptr pchInfo = PchManager::instance()->pchInfo(projectPart);
    return new ClangCompletionAssistInterface(m_ipcCommunicator,
                                              textEditorWidget,
                                              position,
                                              filePath,
                                              reason,
                                              projectPart->headerPaths,
                                              pchInfo,
                                              projectPart->languageFeatures);
}

} // namespace Internal
} // namespace ClangCodeModel
