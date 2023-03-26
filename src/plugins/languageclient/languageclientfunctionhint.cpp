// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientfunctionhint.h"
#include "client.h"

#include <languageserverprotocol/languagefeatures.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/ifunctionhintproposalmodel.h>

#include <QScopedPointer>

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
    int parametersIndex = signature.activeParameter().value_or(-1);
    if (parametersIndex < 0) {
        if (index == m_sigis.activeSignature().value_or(-1))
            parametersIndex = m_sigis.activeParameter().value_or(-1);
    }
    QString label = signature.label();
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

FunctionHintProcessor::FunctionHintProcessor(Client *client)
    : m_client(client)
{}

IAssistProposal *FunctionHintProcessor::perform()
{
    QTC_ASSERT(m_client, return nullptr);
    m_pos = interface()->position();
    QTextCursor cursor(interface()->textDocument());
    cursor.setPosition(m_pos);
    auto uri = m_client->hostPathToServerUri(interface()->filePath());
    SignatureHelpRequest request((TextDocumentPositionParams(TextDocumentIdentifier(uri), Position(cursor))));
    request.setResponseCallback([this](auto response) { this->handleSignatureResponse(response); });
    m_client->addAssistProcessor(this);
    m_client->sendMessage(request);
    m_currentRequest = request.id();
    return nullptr;
}

void FunctionHintProcessor::cancel()
{
    QTC_ASSERT(m_client, return);
    if (running()) {
        m_client->cancelRequest(*m_currentRequest);
        m_client->removeAssistProcessor(this);
        m_currentRequest.reset();
    }
}

void FunctionHintProcessor::handleSignatureResponse(const SignatureHelpRequest::Response &response)
{
    QTC_ASSERT(m_client, setAsyncProposalAvailable(nullptr); return);
    m_currentRequest.reset();
    if (auto error = response.error())
        m_client->log(*error);
    m_client->removeAssistProcessor(this);
    auto result = response.result().value_or(LanguageClientValue<SignatureHelp>());
    if (result.isNull()) {
        setAsyncProposalAvailable(nullptr);
        return;
    }
    const SignatureHelp &signatureHelp = result.value();
    if (signatureHelp.signatures().isEmpty()) {
        setAsyncProposalAvailable(nullptr);
    } else {
        FunctionHintProposalModelPtr model(new FunctionHintProposalModel(signatureHelp));
        setAsyncProposalAvailable(new FunctionHintProposal(m_pos, model));
    }
}

FunctionHintAssistProvider::FunctionHintAssistProvider(Client *client)
    : CompletionAssistProvider(client)
    , m_client(client)
{}

IAssistProcessor *FunctionHintAssistProvider::createProcessor(
    const AssistInterface *) const
{
    return new FunctionHintProcessor(m_client);
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
    const std::optional<QList<QString>> &triggerChars)
{
    m_triggerChars = triggerChars.value_or(QList<QString>());
    for (const QString &trigger : std::as_const(m_triggerChars)) {
        if (trigger.length() > m_activationCharSequenceLength)
            m_activationCharSequenceLength = trigger.length();
    }
}

} // namespace LanguageClient
