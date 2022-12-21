// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    const auto client = ClangModelManagerSupport::clientForFile(interface.filePath());
    if (!client)
        return;

    QTextCursor cursor(interface.textDocument());
    cursor.setPosition(interface.position());
    cursor.select(QTextCursor::LineUnderCursor);
    const QList<Diagnostic> &diagnostics = client->diagnosticsAt(interface.filePath(), cursor);
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
    IAssistProposal *perform() override
    {
        // Step 1: Collect clangd code actions asynchronously
        LanguageClientQuickFixAssistProcessor::perform();

        // Step 2: Collect built-in quickfixes synchronously
        m_builtinOps = CppEditor::quickFixOperations(interface());

        return nullptr;
    }

    TextEditor::GenericProposal *handleCodeActionResult(const CodeActionResult &result) override
    {
        auto toOperation =
            [=](const std::variant<Command, CodeAction> &item) -> QuickFixOperation * {
            if (auto action = std::get_if<CodeAction>(&item)) {
                const std::optional<QList<Diagnostic>> diagnostics = action->diagnostics();
                if (!diagnostics.has_value() || diagnostics->isEmpty())
                    return new CodeActionQuickFixOperation(*action, client());
            }
            if (auto command = std::get_if<Command>(&item))
                return new CommandQuickFixOperation(*command, client());
            return nullptr;
        };

        if (auto list = std::get_if<QList<std::variant<Command, CodeAction>>>(&result)) {
            QuickFixOperations ops;
            for (const std::variant<Command, CodeAction> &item : *list) {
                if (QuickFixOperation *op = toOperation(item)) {
                    op->setDescription("clangd: " + op->description());
                    ops << op;
                }
            }
            return GenericProposal::createProposal(interface(), ops + m_builtinOps);
        }
        return nullptr;
    }

    QuickFixOperations m_builtinOps;
};

ClangdQuickFixProvider::ClangdQuickFixProvider(ClangdClient *client)
    : LanguageClientQuickFixProvider(client) {}

IAssistProcessor *ClangdQuickFixProvider::createProcessor(const AssistInterface *) const
{
    return new ClangdQuickFixProcessor(client());
}

} // namespace Internal
} // namespace ClangCodeModel
