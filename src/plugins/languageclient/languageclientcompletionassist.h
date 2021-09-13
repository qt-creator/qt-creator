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

#include "languageclient_global.h"

#include <languageserverprotocol/completion.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>

#include <utils/optional.h>

#include <QPointer>

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

class LANGUAGECLIENT_EXPORT LanguageClientCompletionAssistProvider
    : public TextEditor::CompletionAssistProvider
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

    void setProposalHandler(const ProposalHandler &handler) { m_proposalHandler = handler; }
    void setSnippetsGroup(const QString &group) { m_snippetsGroup = group; }

protected:
    void setItemsTransformer(const CompletionItemsTransformer &transformer);
    void setApplyHelper(const CompletionApplyHelper &applyHelper);
    Client *client() const { return m_client; }

private:
    QList<QString> m_triggerChars;
    CompletionItemsTransformer m_itemsTransformer;
    CompletionApplyHelper m_applyHelper;
    ProposalHandler m_proposalHandler;
    QString m_snippetsGroup;
    int m_activationCharSequenceLength = 0;
    Client *m_client = nullptr; // not owned
};

class LANGUAGECLIENT_EXPORT LanguageClientCompletionAssistProcessor
    : public TextEditor::IAssistProcessor
{
public:
    LanguageClientCompletionAssistProcessor(Client *client,
                                            const CompletionItemsTransformer &itemsTransformer,
                                            const CompletionApplyHelper &applyHelper,
                                            const ProposalHandler &proposalHandler,
                                            const QString &snippetsGroup);
    ~LanguageClientCompletionAssistProcessor() override;
    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) override;
    bool running() override;
    bool needsRestart() const override { return true; }
    void cancel() override;

private:
    void handleCompletionResponse(const LanguageServerProtocol::CompletionRequest::Response &response);

    QPointer<QTextDocument> m_document;
    Utils::FilePath m_filePath;
    QPointer<Client> m_client;
    Utils::optional<LanguageServerProtocol::MessageId> m_currentRequest;
    QMetaObject::Connection m_postponedUpdateConnection;
    const CompletionItemsTransformer m_itemsTransformer;
    const CompletionApplyHelper m_applyHelper;
    const ProposalHandler m_proposalHandler;
    const QString m_snippetsGroup;
    int m_pos = -1;
    int m_basePos = -1;
};

} // namespace LanguageClient
