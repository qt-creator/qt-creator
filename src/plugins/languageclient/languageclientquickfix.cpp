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

#include "languageclientquickfix.h"

#include "client.h"
#include "languageclientutils.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/quickfix.h>


using namespace LanguageServerProtocol;
using namespace TextEditor;

namespace LanguageClient {

class CodeActionQuickFixOperation : public QuickFixOperation
{
public:
    CodeActionQuickFixOperation(const CodeAction &action, Client *client)
        : m_action(action)
        , m_client(client)
    { setDescription(action.title()); }

    void perform() override
    {
        if (Utils::optional<WorkspaceEdit> edit = m_action.edit()) {
            applyWorkspaceEdit(*edit);
        } else if (Utils::optional<Command> command = m_action.command()) {
            if (m_client)
                m_client->executeCommand(*command);
        }
    }

private:
    CodeAction m_action;
    QPointer<Client> m_client;
};

class CommandQuickFixOperation : public QuickFixOperation
{
public:
    CommandQuickFixOperation(const Command &command, Client *client)
        : m_command(command)
        , m_client(client)
    { setDescription(command.title()); }
    void perform() override
    {
        if (m_client)
            m_client->executeCommand(m_command);
    }

private:
    Command m_command;
    QPointer<Client> m_client;
};

class LanguageClientQuickFixAssistProcessor : public IAssistProcessor
{
public:
    explicit LanguageClientQuickFixAssistProcessor(Client *client) : m_client(client) {}
    bool running() override { return m_currentRequest.has_value(); }
    IAssistProposal *perform(const AssistInterface *interface) override;
    void cancel() override;

private:
    void handleCodeActionResponse(const CodeActionRequest::Response &response);

    QSharedPointer<const AssistInterface> m_assistInterface;
    Client *m_client = nullptr; // not owned
    Utils::optional<MessageId> m_currentRequest;
};

IAssistProposal *LanguageClientQuickFixAssistProcessor::perform(const AssistInterface *interface)
{
    m_assistInterface = QSharedPointer<const AssistInterface>(interface);

    CodeActionParams params;
    params.setContext({});
    QTextCursor cursor(interface->textDocument());
    cursor.setPosition(interface->position());
    if (cursor.atBlockEnd() || cursor.atBlockStart())
        cursor.select(QTextCursor::LineUnderCursor);
    else
        cursor.select(QTextCursor::WordUnderCursor);
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::LineUnderCursor);
    Range range(cursor);
    params.setRange(range);
    auto uri = DocumentUri::fromFilePath(Utils::FilePath::fromString(interface->fileName()));
    params.setTextDocument(TextDocumentIdentifier(uri));
    CodeActionParams::CodeActionContext context;
    context.setDiagnostics(m_client->diagnosticsAt(uri, range));
    params.setContext(context);

    CodeActionRequest request(params);
    request.setResponseCallback([this](const CodeActionRequest::Response &response){
        handleCodeActionResponse(response);
    });

    m_client->requestCodeActions(request);
    m_currentRequest = request.id();
    return nullptr;
}

void LanguageClientQuickFixAssistProcessor::cancel()
{
    if (running()) {
        m_client->cancelRequest(m_currentRequest.value());
        m_client->removeAssistProcessor(this);
        m_currentRequest.reset();
    }
}

void LanguageClientQuickFixAssistProcessor::handleCodeActionResponse(
        const CodeActionRequest::Response &response)
{
    m_currentRequest.reset();
    if (const Utils::optional<CodeActionRequest::Response::Error> &error = response.error())
        m_client->log(*error);
    QuickFixOperations ops;
    if (const Utils::optional<CodeActionResult> &_result = response.result()) {
        const CodeActionResult &result = _result.value();
        if (auto list = Utils::get_if<QList<Utils::variant<Command, CodeAction>>>(&result)) {
            for (const Utils::variant<Command, CodeAction> &item : *list) {
                if (auto action = Utils::get_if<CodeAction>(&item))
                    ops << new CodeActionQuickFixOperation(*action, m_client);
                else if (auto command = Utils::get_if<Command>(&item))
                    ops << new CommandQuickFixOperation(*command, m_client);
            }
        }
    }
    m_client->removeAssistProcessor(this);
    setAsyncProposalAvailable(GenericProposal::createProposal(m_assistInterface.data(), ops));
}

LanguageClientQuickFixProvider::LanguageClientQuickFixProvider(Client *client)
    : IAssistProvider(client)
    , m_client(client)
{
    QTC_CHECK(client);
}

IAssistProvider::RunType LanguageClientQuickFixProvider::runType() const
{
    return Asynchronous;
}

IAssistProcessor *LanguageClientQuickFixProvider::createProcessor() const
{
    return new LanguageClientQuickFixAssistProcessor(m_client);
}

} // namespace LanguageClient
