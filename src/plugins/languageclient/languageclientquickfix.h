// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <texteditor/codeassist/iassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/quickfix.h>

#include <languageserverprotocol/languagefeatures.h>

#include <QPointer>

namespace TextEditor {
class IAssistProposal;
class GenericProposal;
} // namespace TextEditor

namespace LanguageClient {

class Client;

class LANGUAGECLIENT_EXPORT CodeActionQuickFixOperation : public TextEditor::QuickFixOperation
{
public:
    CodeActionQuickFixOperation(const LanguageServerProtocol::CodeAction &action, Client *client);
    void perform() override;

private:
    LanguageServerProtocol::CodeAction m_action;
    QPointer<Client> m_client;
};

class LANGUAGECLIENT_EXPORT CommandQuickFixOperation : public TextEditor::QuickFixOperation
{
public:
    CommandQuickFixOperation(const LanguageServerProtocol::Command &command, Client *client);
    void perform() override;

private:
    LanguageServerProtocol::Command m_command;
    QPointer<Client> m_client;
};

class LANGUAGECLIENT_EXPORT LanguageClientQuickFixProvider : public TextEditor::IAssistProvider
{
public:
    explicit LanguageClientQuickFixProvider(Client *client);
    IAssistProvider::RunType runType() const override;
    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override;

protected:
    Client *client() const { return m_client; }

private:
    Client *m_client = nullptr; // not owned
};

class LANGUAGECLIENT_EXPORT LanguageClientQuickFixAssistProcessor
        : public TextEditor::IAssistProcessor
{
public:
    explicit LanguageClientQuickFixAssistProcessor(Client *client) : m_client(client) {}
    bool running() override { return m_currentRequest.has_value(); }
    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) override;
    void cancel() override;

protected:
    void setOnlyKinds(const QList<LanguageServerProtocol::CodeActionKind> &only);
    Client *client() { return m_client; }

private:
    void handleCodeActionResponse(
        const LanguageServerProtocol::CodeActionRequest::Response &response);
    virtual TextEditor::GenericProposal *handleCodeActionResult(
        const LanguageServerProtocol::CodeActionResult &result);

    QSharedPointer<const TextEditor::AssistInterface> m_assistInterface;
    Client *m_client = nullptr; // not owned
    Utils::optional<LanguageServerProtocol::MessageId> m_currentRequest;
};

} // namespace LanguageClient
