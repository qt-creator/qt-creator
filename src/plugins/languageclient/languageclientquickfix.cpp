// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientquickfix.h"

#include "client.h"
#include "languageclientutils.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/quickfix.h>

#include <languageserverprotocol/lsputils.h>

using namespace TextEditor;

namespace LanguageClient {

CodeActionQuickFixOperation::CodeActionQuickFixOperation(
    const LanguageServerProtocol::CodeAction &action, Client *client)
    : m_action(action)
    , m_client(client)
{
    setDescription(action.title());
}

void CodeActionQuickFixOperation::perform()
{
    if (!m_client)
        return;
    if (const std::optional<LanguageServerProtocol::WorkspaceEdit> &edit = m_action.edit())
        applyWorkspaceEdit(m_client, *edit);
    else if (const std::optional<LanguageServerProtocol::Command> &command = m_action.command())
        m_client->executeCommand(*command);
}

CommandQuickFixOperation::CommandQuickFixOperation(
    const LanguageServerProtocol::Command &command, Client *client)
    : m_command(command)
    , m_client(client)
{ setDescription(command.title()); }


void CommandQuickFixOperation::perform()
{
    if (m_client)
        m_client->executeCommand(m_command);
}

IAssistProposal *LanguageClientQuickFixAssistProcessor::perform()
{
    LanguageServerProtocol::CodeActionParams params;
    QTextCursor cursor = interface()->cursor();
    if (!cursor.hasSelection()) {
        if (cursor.atBlockEnd() || cursor.atBlockStart())
            cursor.select(QTextCursor::LineUnderCursor);
        else
            cursor.select(QTextCursor::WordUnderCursor);
    }
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::LineUnderCursor);
    params.range(LanguageServerProtocol::rangeOf(cursor));
    const Utils::FilePath filePath = interface()->filePath();
    params.textDocument(
        LanguageServerProtocol::TextDocumentIdentifier().uri(m_client->uriFor(filePath)));
    params.context(LanguageServerProtocol::CodeActionContext().diagnostics(
        m_client->diagnosticsAt(filePath, cursor)));

    m_client->addAssistProcessor(this);
    m_currentRequest = m_client->sendRequest<LanguageServerProtocol::CodeActionRequest>(
        params,
        [this](const Utils::Result<LanguageServerProtocol::CodeActionRequestResult> &result) {
            handleCodeActionResponse(result);
        });
    return nullptr;
}

void LanguageClientQuickFixAssistProcessor::cancel()
{
    if (running()) {
        m_client->cancelRequest(*m_currentRequest);
        m_client->removeAssistProcessor(this);
        m_currentRequest.reset();
    }
}

QuickFixOperations LanguageClientQuickFixAssistProcessor::resultToOperations(
    const LanguageServerProtocol::CodeActionRequestResult &result)
{
    const auto list = std::get_if<QList<LanguageServerProtocol::CommandOrCodeAction>>(&result);
    if (!list)
        return {};

    QuickFixOperations ops;
    for (const LanguageServerProtocol::CommandOrCodeAction &item : *list) {
        if (const auto action = std::get_if<LanguageServerProtocol::CodeAction>(&item))
            ops << new CodeActionQuickFixOperation(*action, m_client);
        else if (const auto command = std::get_if<LanguageServerProtocol::Command>(&item))
            ops << new CommandQuickFixOperation(*command, m_client);
    }
    return ops;
}

void LanguageClientQuickFixAssistProcessor::handleCodeActionResponse(
    const Utils::Result<LanguageServerProtocol::CodeActionRequestResult> &result)
{
    m_currentRequest.reset();
    if (!result)
        m_client->log(QtMsgType::QtCriticalMsg, result.error());
    m_client->removeAssistProcessor(this);
    GenericProposal *proposal = result ? handleCodeActionResult(*result) : nullptr;
    setAsyncProposalAvailable(proposal);
}

GenericProposal *LanguageClientQuickFixAssistProcessor::handleCodeActionResult(
    const LanguageServerProtocol::CodeActionRequestResult &result)
{
    return GenericProposal::createProposal(interface(), resultToOperations(result));
}

LanguageClientQuickFixProvider::LanguageClientQuickFixProvider(Client *client)
    : IAssistProvider(client)
    , m_client(client)
{
    QTC_CHECK(client);
}

IAssistProcessor *LanguageClientQuickFixProvider::createProcessor(const AssistInterface *) const
{
    return new LanguageClientQuickFixAssistProcessor(m_client);
}

} // namespace LanguageClient
