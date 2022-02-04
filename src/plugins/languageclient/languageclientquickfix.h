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

#include <texteditor/codeassist/iassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/quickfix.h>

#include <languageserverprotocol/languagefeatures.h>

#include <QPointer>

namespace TextEditor { class IAssistProposal; }

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

private:
    void handleCodeActionResponse(const LanguageServerProtocol::CodeActionRequest::Response &response);
    virtual void handleProposalReady(const TextEditor::QuickFixOperations &ops);

    QSharedPointer<const TextEditor::AssistInterface> m_assistInterface;
    Client *m_client = nullptr; // not owned
    Utils::optional<LanguageServerProtocol::MessageId> m_currentRequest;
};

} // namespace LanguageClient
