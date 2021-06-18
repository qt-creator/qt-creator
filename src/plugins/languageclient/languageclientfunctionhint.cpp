/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "languageclientfunctionhint.h"
#include "client.h"

#include <languageserverprotocol/languagefeatures.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/ifunctionhintproposalmodel.h>

using namespace TextEditor;
using namespace LanguageServerProtocol;

namespace LanguageClient {

class FunctionHintProposalModel : public IFunctionHintProposalModel
{
public:
    explicit FunctionHintProposalModel(SignatureHelp signature)
        : m_sigis(signature)
    {}
    void reset() override {}
    int size() const override
    { return m_sigis.signatures().size(); }
    QString text(int index) const override;

    int activeArgument(const QString &/*prefix*/) const override
    { return m_sigis.activeParameter().value_or(0); }

private:
    LanguageServerProtocol::SignatureHelp m_sigis;
};

QString FunctionHintProposalModel::text(int index) const
{
    using Parameters = QList<ParameterInformation>;
    if (index < 0 || m_sigis.signatures().size() <= index)
        return {};
    const SignatureInformation signature = m_sigis.signatures().at(index);
    QString label = signature.label();
    if (index != m_sigis.activeSignature().value_or(-1))
        return label;

    const int parametersIndex = m_sigis.activeParameter().value_or(-1);
    if (parametersIndex < 0)
        return label;

    const QList<QString> parameters = Utils::transform(signature.parameters().value_or(Parameters()),
                                                       &ParameterInformation::label);
    if (parameters.size() <= parametersIndex)
        return label;

    const QString &parameterText = parameters.at(parametersIndex);
    const int start = label.indexOf(parameterText);
    const int end = start + parameterText.length();
    return label.mid(0, start).toHtmlEscaped() + "<b>" + parameterText.toHtmlEscaped() + "</b>"
           + label.mid(end).toHtmlEscaped();
}

class FunctionHintProcessor : public IAssistProcessor
{
public:
    explicit FunctionHintProcessor(Client *client, const ProposalHandler &proposalHandler)
        : m_client(client), m_proposalHandler(proposalHandler) {}
    IAssistProposal *perform(const AssistInterface *interface) override;
    bool running() override { return m_currentRequest.has_value(); }
    bool needsRestart() const override { return true; }
    void cancel() override;

private:
    void handleSignatureResponse(const SignatureHelpRequest::Response &response);
    void processProposal(TextEditor::IAssistProposal *proposal);

    QPointer<Client> m_client;
    const ProposalHandler m_proposalHandler;
    Utils::optional<MessageId> m_currentRequest;
    int m_pos = -1;
};

IAssistProposal *FunctionHintProcessor::perform(const AssistInterface *interface)
{
    QTC_ASSERT(m_client, return nullptr);
    m_pos = interface->position();
    QTextCursor cursor(interface->textDocument());
    cursor.setPosition(m_pos);
    auto uri = DocumentUri::fromFilePath(interface->filePath());
    SignatureHelpRequest request((TextDocumentPositionParams(TextDocumentIdentifier(uri), Position(cursor))));
    request.setResponseCallback([this](auto response) { this->handleSignatureResponse(response); });
    m_client->sendContent(request);
    m_currentRequest = request.id();
    return nullptr;
}

void FunctionHintProcessor::cancel()
{
    if (running()) {
        m_client->cancelRequest(m_currentRequest.value());
        m_client->removeAssistProcessor(this);
        m_currentRequest.reset();
    }
}

void FunctionHintProcessor::handleSignatureResponse(const SignatureHelpRequest::Response &response)
{
    m_currentRequest.reset();
    if (auto error = response.error())
        m_client->log(error.value());
    m_client->removeAssistProcessor(this);
    auto result = response.result().value_or(LanguageClientValue<SignatureHelp>());
    if (result.isNull()) {
        processProposal(nullptr);
        return;
    }
    const SignatureHelp &signatureHelp = result.value();
    if (signatureHelp.signatures().isEmpty()) {
        processProposal(nullptr);
    } else {
        FunctionHintProposalModelPtr model(new FunctionHintProposalModel(signatureHelp));
        processProposal(new FunctionHintProposal(m_pos, model));
    }
}

void FunctionHintProcessor::processProposal(IAssistProposal *proposal)
{
    if (m_proposalHandler)
        m_proposalHandler(proposal);
    else
        setAsyncProposalAvailable(proposal);
}

FunctionHintAssistProvider::FunctionHintAssistProvider(Client *client)
    : CompletionAssistProvider(client)
    , m_client(client)
{}

TextEditor::IAssistProcessor *FunctionHintAssistProvider::createProcessor() const
{
    return new FunctionHintProcessor(m_client, m_proposalHandler);
}

IAssistProvider::RunType FunctionHintAssistProvider::runType() const
{
    return Asynchronous;
}

int FunctionHintAssistProvider::activationCharSequenceLength() const
{
    return m_activationCharSequenceLength;
}

bool FunctionHintAssistProvider::isActivationCharSequence(
    const QString &sequence) const
{
    return Utils::anyOf(m_triggerChars,
                        [sequence](const QString &trigger) { return trigger.endsWith(sequence); });
}

bool FunctionHintAssistProvider::isContinuationChar(const QChar &/*c*/) const
{
    return true;
}

void FunctionHintAssistProvider::setTriggerCharacters(
    const Utils::optional<QList<QString>> &triggerChars)
{
    m_triggerChars = triggerChars.value_or(QList<QString>());
    for (const QString &trigger : qAsConst(m_triggerChars)) {
        if (trigger.length() > m_activationCharSequenceLength)
            m_activationCharSequenceLength = trigger.length();
    }
}

} // namespace LanguageClient
