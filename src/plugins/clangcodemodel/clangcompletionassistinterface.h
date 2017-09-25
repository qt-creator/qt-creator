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

#pragma once

#include "clangbackendcommunicator.h"
#include "clangutils.h"

#include <texteditor/codeassist/assistinterface.h>

namespace ClangCodeModel {
namespace Internal {

class ClangCompletionAssistInterface: public TextEditor::AssistInterface
{
public:
    ClangCompletionAssistInterface(BackendCommunicator &communicator,
                                   const TextEditor::TextEditorWidget *textEditorWidget,
                                   int position,
                                   const QString &fileName,
                                   TextEditor::AssistReason reason,
                                   const CppTools::ProjectPartHeaderPaths &headerPaths,
                                   const CPlusPlus::LanguageFeatures &features);

    BackendCommunicator &communicator() const;
    bool objcEnabled() const;
    const CppTools::ProjectPartHeaderPaths &headerPaths() const;
    CPlusPlus::LanguageFeatures languageFeatures() const;
    const TextEditor::TextEditorWidget *textEditorWidget() const;

    void setHeaderPaths(const CppTools::ProjectPartHeaderPaths &headerPaths); // For tests

private:
    BackendCommunicator &m_communicator;
    QStringList m_options;
    CppTools::ProjectPartHeaderPaths m_headerPaths;
    CPlusPlus::LanguageFeatures m_languageFeatures;
    const TextEditor::TextEditorWidget *m_textEditorWidget;
};

} // namespace Internal
} // namespace ClangCodeModel
