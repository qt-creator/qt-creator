/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <languageserverprotocol/completion.h>
#include <texteditor/codeassist/completionassistprovider.h>

#include <utils/optional.h>

#include <functional>

namespace TextEditor {
class IAssistProposal;
class TextDocumentManipulatorInterface;
}

namespace LanguageClient {

class Client;

using CompletionItemsTransformer = std::function<QList<LanguageServerProtocol::CompletionItem>(
        const Utils::FilePath &, const QString &, int,
        const QList<LanguageServerProtocol::CompletionItem> &)>;
using CompletionApplyHelper = std::function<void(
        const LanguageServerProtocol::CompletionItem &,
        TextEditor::TextDocumentManipulatorInterface &, QChar)>;
using ProposalHandler = std::function<void(TextEditor::IAssistProposal *)>;

class LanguageClientCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    LanguageClientCompletionAssistProvider(Client *client);

    TextEditor::IAssistProcessor *createProcessor() const override;
    RunType runType() const override;

    int activationCharSequenceLength() const override;
    bool isActivationCharSequence(const QString &sequence) const override;
    bool isContinuationChar(const QChar &) const override { return true; }

    void setTriggerCharacters(const Utils::optional<QList<QString>> triggerChars);

    void setItemsTransformer(const CompletionItemsTransformer &transformer);
    void setApplyHelper(const CompletionApplyHelper &applyHelper);
    void setProposalHandler(const ProposalHandler &handler) { m_proposalHandler = handler; }
    void setSnippetsGroup(const QString &group) { m_snippetsGroup = group; }

private:
    QList<QString> m_triggerChars;
    CompletionItemsTransformer m_itemsTransformer;
    CompletionApplyHelper m_applyHelper;
    ProposalHandler m_proposalHandler;
    QString m_snippetsGroup;
    int m_activationCharSequenceLength = 0;
    Client *m_client = nullptr; // not owned
};

} // namespace LanguageClient
