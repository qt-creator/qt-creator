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

#include "clangcompletionassistinterface.h"

#include <texteditor/texteditor.h>

namespace ClangCodeModel {
namespace Internal {

ClangCompletionAssistInterface::ClangCompletionAssistInterface(
        BackendCommunicator &communicator,
        const TextEditor::TextEditorWidget *textEditorWidget,
        int position,
        const QString &fileName,
        TextEditor::AssistReason reason,
        const CppTools::ProjectPartHeaderPaths &headerPaths,
        const CPlusPlus::LanguageFeatures &features)
    : AssistInterface(textEditorWidget->document(), position, fileName, reason)
    , m_communicator(communicator)
    , m_headerPaths(headerPaths)
    , m_languageFeatures(features)
    , m_textEditorWidget(textEditorWidget)
{
}

bool ClangCompletionAssistInterface::objcEnabled() const
{
    return true; // TODO:
}

const CppTools::ProjectPartHeaderPaths &ClangCompletionAssistInterface::headerPaths() const
{
    return m_headerPaths;
}

CPlusPlus::LanguageFeatures ClangCompletionAssistInterface::languageFeatures() const
{
    return m_languageFeatures;
}

void ClangCompletionAssistInterface::setHeaderPaths(const CppTools::ProjectPartHeaderPaths &headerPaths)
{
    m_headerPaths = headerPaths;
}

const TextEditor::TextEditorWidget *ClangCompletionAssistInterface::textEditorWidget() const
{
    return m_textEditorWidget;
}

BackendCommunicator &ClangCompletionAssistInterface::communicator() const
{
    return m_communicator;
}

} // namespace Internal
} // namespace ClangCodeModel

