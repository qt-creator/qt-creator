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

#include "clangcompletionassistinterface.h"

#include <cpptools/cppcompletionassistprocessor.h>

#include <clangbackendipc/codecompletion.h>

#include <QCoreApplication>
#include <QTextCursor>

namespace ClangCodeModel {
namespace Internal {

using ClangBackEnd::CodeCompletions;
using ClangBackEnd::CompletionCorrection;

class ClangCompletionAssistProcessor : public CppTools::CppCompletionAssistProcessor
{
    Q_DECLARE_TR_FUNCTIONS(ClangCodeModel::Internal::ClangCompletionAssistProcessor)

public:
    ClangCompletionAssistProcessor();
    ~ClangCompletionAssistProcessor();

    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) override;

    bool handleAvailableAsyncCompletions(const CodeCompletions &completions,
                                         CompletionCorrection neededCorrection);

    const TextEditor::TextEditorWidget *textEditorWidget() const;

private:
    TextEditor::IAssistProposal *startCompletionHelper();
    int startOfOperator(int pos, unsigned *kind, bool wantFunctionCall) const;
    int findStartOfName(int pos = -1) const;
    bool accepts() const;

    TextEditor::IAssistProposal *createProposal(
            CompletionCorrection neededCorrection = CompletionCorrection::NoCorrection) const;

    bool completeInclude(const QTextCursor &cursor);
    bool completeInclude(int position);
    void completeIncludePath(const QString &realPath, const QStringList &suffixes);
    bool completePreprocessorDirectives();
    bool completeDoxygenKeywords();
    void addCompletionItem(const QString &text,
                           const QIcon &icon = QIcon(),
                           int order = 0);

    struct UnsavedFileContentInfo {
        QByteArray unsavedContent;
        bool isDocumentModified = false;
    };
    UnsavedFileContentInfo unsavedFileContent(const QByteArray &customFileContent) const;

    void sendFileContent(const QByteArray &customFileContent);
    bool sendCompletionRequest(int position, const QByteArray &customFileContent);

    void handleAvailableCompletions(const CodeCompletions &completions,
                                    CompletionCorrection neededCorrection);
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
