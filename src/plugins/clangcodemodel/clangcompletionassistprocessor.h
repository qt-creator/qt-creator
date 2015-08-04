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

#ifndef CLANGCODEMODEL_INTERNAL_CLANGCOMPLETIONASSISTPROCESSOR_H
#define CLANGCODEMODEL_INTERNAL_CLANGCOMPLETIONASSISTPROCESSOR_H

#include "clangcompletionassistinterface.h"

#include <cpptools/cppcompletionassistprocessor.h>
#include <texteditor/texteditor.h>

#include <clangbackendipc/codecompletion.h>

#include <QCoreApplication>

namespace ClangCodeModel {
namespace Internal {

using ClangBackEnd::CodeCompletions;

class ClangCompletionAssistProcessor : public CppTools::CppCompletionAssistProcessor
{
    Q_DECLARE_TR_FUNCTIONS(ClangCodeModel::Internal::ClangCompletionAssistProcessor)

public:
    ClangCompletionAssistProcessor();
    ~ClangCompletionAssistProcessor();

    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) override;

    bool handleAvailableAsyncCompletions(const CodeCompletions &completions);

    const TextEditor::TextEditorWidget *textEditorWidget() const;

private:
    TextEditor::IAssistProposal *startCompletionHelper();
    int startOfOperator(int pos, unsigned *kind, bool wantFunctionCall) const;
    int findStartOfName(int pos = -1) const;
    bool accepts() const;

    TextEditor::IAssistProposal *createProposal() const;

    bool completeInclude(const QTextCursor &cursor);
    bool completeInclude(int position);
    void completeIncludePath(const QString &realPath, const QStringList &suffixes);
    bool completePreprocessorDirectives();
    bool completeDoxygenKeywords();
    void addCompletionItem(const QString &text,
                           const QIcon &icon = QIcon(),
                           int order = 0,
                           const QVariant &data = QVariant());

    struct UnsavedFileContentInfo {
        QByteArray unsavedContent;
        bool isDocumentModified = false;
    };
    UnsavedFileContentInfo unsavedFileContent(const QByteArray &customFileContent) const;

    void sendFileContent(const QString &projectPartId, const QByteArray &customFileContent);
    void sendCompletionRequest(int position, const QByteArray &customFileContent);

    void handleAvailableCompletions(const CodeCompletions &completions);
    bool handleAvailableFunctionHintCompletions(const CodeCompletions &completions);

private:
    QScopedPointer<const ClangCompletionAssistInterface> m_interface;
    unsigned m_completionOperator;
    enum CompletionRequestType { NormalCompletion, FunctionHintCompletion } m_sentRequestType;
    QString m_functionName;     // For type == Type::FunctionHintCompletion
    bool m_addSnippets = false; // For type == Type::NormalCompletion
};

} // namespace Internal
} // namespace ClangCodeModel

#endif // CLANGCODEMODEL_INTERNAL_CLANGCOMPLETIONASSISTPROCESSOR_H
