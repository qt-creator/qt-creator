// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <languageserverprotocol/languagefeatures.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>

#include <QPointer>

#include <optional>

namespace TextEditor { class IAssistProposal; }

namespace LanguageClient {

class Client;

class LANGUAGECLIENT_EXPORT FunctionHintAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    explicit FunctionHintAssistProvider(Client *client);

    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override;

    int activationCharSequenceLength() const override;
    bool isActivationCharSequence(const QString &sequence) const override;
    bool isContinuationChar(const QChar &c) const override;

    void setTriggerCharacters(const std::optional<QList<QString>> &triggerChars);

private:
    QList<QString> m_triggerChars;
    int m_activationCharSequenceLength = 0;
    Client *m_client = nullptr; // not owned
};

class LANGUAGECLIENT_EXPORT FunctionHintProcessor : public TextEditor::IAssistProcessor
{
public:
    explicit FunctionHintProcessor(Client *client);
    TextEditor::IAssistProposal *perform() override;
    bool running() override { return m_currentRequest.has_value(); }
    bool needsRestart() const override { return true; }
    void cancel() override;

private:
    void handleSignatureResponse(
        const LanguageServerProtocol::SignatureHelpRequest::Response &response);

    QPointer<Client> m_client;
    std::optional<LanguageServerProtocol::MessageId> m_currentRequest;
    int m_pos = -1;
};

} // namespace LanguageClient
