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

#ifndef CLANGCODEMODEL_INTERNAL_CLANGCOMPLETIONASSISTINTERFACE_H
#define CLANGCODEMODEL_INTERNAL_CLANGCOMPLETIONASSISTINTERFACE_H

#include "clangbackendipcintegration.h"
#include "pchinfo.h"
#include "utils.h"

#include <cpptools/cppcompletionassistprovider.h>

#include <texteditor/codeassist/assistinterface.h>

namespace ClangCodeModel {
namespace Internal {

class ClangCompletionAssistInterface: public TextEditor::AssistInterface
{
public:
    ClangCompletionAssistInterface(IpcCommunicator &ipcCommunicator,
                                   const TextEditor::TextEditorWidget *textEditorWidget,
                                   int position,
                                   const QString &fileName,
                                   TextEditor::AssistReason reason,
                                   const CppTools::ProjectPart::HeaderPaths &headerPaths,
                                   const PchInfo::Ptr &pchInfo,
                                   const CPlusPlus::LanguageFeatures &features);

    IpcCommunicator &ipcCommunicator() const;
    const UnsavedFiles &unsavedFiles() const;
    bool objcEnabled() const;
    const CppTools::ProjectPart::HeaderPaths &headerPaths() const;
    CPlusPlus::LanguageFeatures languageFeatures() const;
    const TextEditor::TextEditorWidget *textEditorWidget() const;

    void setHeaderPaths(const CppTools::ProjectPart::HeaderPaths &headerPaths); // For tests

private:
    IpcCommunicator &m_ipcCommunicator;
    UnsavedFiles m_unsavedFiles;
    QStringList m_options;
    CppTools::ProjectPart::HeaderPaths m_headerPaths;
    Internal::PchInfo::Ptr m_savedPchPointer;
    CPlusPlus::LanguageFeatures m_languageFeatures;
    const TextEditor::TextEditorWidget *m_textEditorWidget;
};

} // namespace Internal
} // namespace ClangCodeModel

#endif // CLANGCODEMODEL_INTERNAL_CLANGCOMPLETIONASSISTINTERFACE_H
