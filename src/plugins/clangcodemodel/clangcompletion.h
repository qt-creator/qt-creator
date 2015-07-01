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

#ifndef CPPEDITOR_INTERNAL_CLANGCOMPLETION_H
#define CPPEDITOR_INTERNAL_CLANGCOMPLETION_H

#include "clangcompleter.h"
#include "clangbackendipcintegration.h"

#include <cpptools/cppcompletionassistprocessor.h>
#include <cpptools/cppcompletionassistprovider.h>
#include <cpptools/cppmodelmanager.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/ifunctionhintproposalmodel.h>

#include <clangbackendipc/codecompletion.h>

#include <QStringList>
#include <QTextCursor>

namespace ClangCodeModel {
namespace Internal {

using CodeCompletions = QVector<ClangBackEnd::CodeCompletion>;

class ClangAssistProposalModel;

class ClangCompletionAssistProvider : public CppTools::CppCompletionAssistProvider
{
    Q_OBJECT

public:
    ClangCompletionAssistProvider(IpcCommunicator &ipcCommunicator);

    IAssistProvider::RunType runType() const override;

    TextEditor::IAssistProcessor *createProcessor() const override;
    TextEditor::AssistInterface *createAssistInterface(
            const QString &filePath,
            const TextEditor::TextEditorWidget *textEditorWidget,
            const CPlusPlus::LanguageFeatures &languageFeatures,
            int position,
            TextEditor::AssistReason reason) const override;

private:
    IpcCommunicator &m_ipcCommunicator;
};

class ClangAssistProposalItem : public TextEditor::AssistProposalItem
{
public:
    ClangAssistProposalItem() {}

    bool prematurelyApplies(const QChar &c) const override;
    void applyContextualContent(TextEditor::TextEditorWidget *editorWidget, int basePosition) const override;

    void keepCompletionOperator(unsigned compOp) { m_completionOperator = compOp; }

    bool isOverloaded() const;
    void addOverload(const ClangBackEnd::CodeCompletion &ccr);

    ClangBackEnd::CodeCompletion originalItem() const;

    bool isCodeCompletion() const;

private:
    unsigned m_completionOperator;
    mutable QChar m_typedChar;
    QList<ClangBackEnd::CodeCompletion> m_overloads;
};

class ClangFunctionHintModel : public TextEditor::IFunctionHintProposalModel
{
public:
    ClangFunctionHintModel(const CodeCompletions &functionSymbols);

    void reset() override {}
    int size() const override { return m_functionSymbols.size(); }
    QString text(int index) const override;
    int activeArgument(const QString &prefix) const override;

private:
    CodeCompletions m_functionSymbols;
    mutable int m_currentArg;
};

class ClangCompletionAssistInterface: public TextEditor::AssistInterface
{
public:
    ClangCompletionAssistInterface(ClangCodeModel::Internal::IpcCommunicator &ipcCommunicator,
                                   const TextEditor::TextEditorWidget *textEditorWidget,
                                   int position,
                                   const QString &fileName,
                                   TextEditor::AssistReason reason,
                                   const CppTools::ProjectPart::HeaderPaths &headerPaths,
                                   const Internal::PchInfo::Ptr &pchInfo,
                                   const CPlusPlus::LanguageFeatures &features);

    ClangCodeModel::Internal::IpcCommunicator &ipcCommunicator() const;
    const ClangCodeModel::Internal::UnsavedFiles &unsavedFiles() const;
    bool objcEnabled() const;
    const CppTools::ProjectPart::HeaderPaths &headerPaths() const;
    CPlusPlus::LanguageFeatures languageFeatures() const;
    const TextEditor::TextEditorWidget *textEditorWidget() const;

    void setHeaderPaths(const CppTools::ProjectPart::HeaderPaths &headerPaths); // For tests

private:
    ClangCodeModel::Internal::IpcCommunicator &m_ipcCommunicator;
    ClangCodeModel::Internal::UnsavedFiles m_unsavedFiles;
    QStringList m_options;
    CppTools::ProjectPart::HeaderPaths m_headerPaths;
    Internal::PchInfo::Ptr m_savedPchPointer;
    CPlusPlus::LanguageFeatures m_languageFeatures;
    const TextEditor::TextEditorWidget *m_textEditorWidget;
};

class ClangCompletionAssistProcessor : public CppTools::CppCompletionAssistProcessor
{
    Q_DECLARE_TR_FUNCTIONS(ClangCodeModel::Internal::ClangCompletionAssistProcessor)

public:
    ClangCompletionAssistProcessor();
    ~ClangCompletionAssistProcessor();

    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) override;

    void asyncCompletionsAvailable(const CodeCompletions &completions);

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

    void sendFileContent(const QString &projectFilePath, const QByteArray &modifiedFileContent);
    void sendCompletionRequest(int position, const QByteArray &modifiedFileContent);

    void onCompletionsAvailable(const CodeCompletions &completions);
    void onFunctionHintCompletionsAvailable(const CodeCompletions &completions);

private:
    QScopedPointer<const ClangCompletionAssistInterface> m_interface;
    unsigned m_completionOperator;
    enum CompletionRequestType { NormalCompletion, FunctionHintCompletion } m_sentRequestType;
    QString m_functionName;     // For type == Type::FunctionHintCompletion
    bool m_addSnippets = false; // For type == Type::NormalCompletion
};

} // namespace Internal
} // namespace Clang

#endif // CPPEDITOR_INTERNAL_CLANGCOMPLETION_H
