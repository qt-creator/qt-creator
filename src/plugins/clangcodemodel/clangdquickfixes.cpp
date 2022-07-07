/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "clangdquickfixes.h"

#include "clangdclient.h"
#include "clangmodelmanagersupport.h"

#include <texteditor/codeassist/genericproposal.h>

using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace TextEditor;

namespace ClangCodeModel {
namespace Internal {

ClangdQuickFixFactory::ClangdQuickFixFactory() = default;

void ClangdQuickFixFactory::match(const CppEditor::Internal::CppQuickFixInterface &interface,
                                  QuickFixOperations &result)
{
    const auto client = ClangModelManagerSupport::instance()->clientForFile(interface.filePath());
    if (!client)
        return;

    const auto uri = DocumentUri::fromFilePath(interface.filePath());
    QTextCursor cursor(interface.textDocument());
    cursor.setPosition(interface.position());
    cursor.select(QTextCursor::LineUnderCursor);
    const QList<Diagnostic> &diagnostics = client->diagnosticsAt(uri, cursor);
    for (const Diagnostic &diagnostic : diagnostics) {
        ClangdDiagnostic clangdDiagnostic(diagnostic);
        if (const auto actions = clangdDiagnostic.codeActions()) {
            for (const CodeAction &action : *actions)
                result << new LanguageClient::CodeActionQuickFixOperation(action, client);
        }
    }
}

class ClangdQuickFixProcessor : public LanguageClientQuickFixAssistProcessor
{
public:
    ClangdQuickFixProcessor(LanguageClient::Client *client)
        : LanguageClientQuickFixAssistProcessor(client)
    {
    }

private:
    IAssistProposal *perform(const AssistInterface *interface) override
    {
        m_interface = interface;

        // Step 1: Collect clangd code actions asynchronously
        LanguageClientQuickFixAssistProcessor::perform(interface);

        // Step 2: Collect built-in quickfixes synchronously
        m_builtinOps = CppEditor::quickFixOperations(interface);

        return nullptr;
    }

    TextEditor::GenericProposal *handleCodeActionResult(const CodeActionResult &result) override
    {
        auto toOperation =
            [=](const Utils::variant<Command, CodeAction> &item) -> QuickFixOperation * {
            if (auto action = Utils::get_if<CodeAction>(&item)) {
                const Utils::optional<QList<Diagnostic>> diagnostics = action->diagnostics();
                if (!diagnostics.has_value() || diagnostics->isEmpty())
                    return new CodeActionQuickFixOperation(*action, client());
            }
            if (auto command = Utils::get_if<Command>(&item))
                return new CommandQuickFixOperation(*command, client());
            return nullptr;
        };

        if (auto list = Utils::get_if<QList<Utils::variant<Command, CodeAction>>>(&result)) {
            QuickFixOperations ops;
            for (const Utils::variant<Command, CodeAction> &item : *list) {
                if (QuickFixOperation *op = toOperation(item)) {
                    op->setDescription("clangd: " + op->description());
                    ops << op;
                }
            }
            return GenericProposal::createProposal(m_interface, ops + m_builtinOps);
        }
        return nullptr;
    }

    QuickFixOperations m_builtinOps;
    const AssistInterface *m_interface = nullptr;
};

ClangdQuickFixProvider::ClangdQuickFixProvider(ClangdClient *client)
    : LanguageClientQuickFixProvider(client) {}

IAssistProcessor *ClangdQuickFixProvider::createProcessor(const AssistInterface *) const
{
    return new ClangdQuickFixProcessor(client());
}

} // namespace Internal
} // namespace ClangCodeModel
