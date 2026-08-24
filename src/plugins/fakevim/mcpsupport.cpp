// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "fakevimactions.h"
#include "fakevimhandler.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <mcp/server/toolregistry.h>

#include <utils/result.h>

#include <QJsonObject>
#include <QPointer>
#include <QTextCursor>

using namespace Core;
using namespace Utils;

namespace FakeVim::Internal {

void registerMcpTools()
{
    using namespace Mcp::Schema;
    using Mcp::ToolRegistry;

    ToolRegistry::registerTool(
        Tool{}
            .name("fakevim_keys")
            .title("Send keystrokes to FakeVim")
            .description(
                "Funnels a string of keystrokes into the FakeVim handler of the current "
                "editor, exactly as if typed in Vim. The string uses Vim key notation: "
                "printable characters stand for themselves and special keys are angle-bracket "
                "escapes, so \"ihello<Esc>\" inserts \"hello\" and returns to normal mode, "
                "\"3j\" moves down three lines, \"dd\" deletes a line and \"<C-v>\" is Ctrl-V. "
                "FakeVim must be enabled (Edit > Preferences > FakeVim, or the Use FakeVim "
                "action). Returns the 1-based cursor line and column after the keys were "
                "processed, so a scenario can assert the effect of a motion or edit.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "keys",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Keystrokes in Vim notation, e.g. \"ihello<Esc>\" or \"3j\"."}})
                    .addRequired("keys"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("keys", QJsonObject{{"type", "string"}})
                    .addProperty("line", QJsonObject{{"type", "integer"}})
                    .addProperty("column", QJsonObject{{"type", "integer"}})),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QString keys = params.argumentsAsObject().value("keys").toString();
            if (keys.isEmpty())
                return ResultError(QString("No keys given."));

            // Every text editor has a handler, whether or not FakeVim is
            // enabled, so the setting has to be checked on its own.
            if (!settings().useFakeVim()) {
                return ResultError(QString(
                    "FakeVim is disabled. Enable it in Edit > Preferences > FakeVim, or with "
                    "the \"Use FakeVim\" action."));
            }

            FakeVimHandler *handler = handlerForEditor(EditorManager::currentEditor());
            if (!handler) {
                return ResultError(QString(
                    "No text editor to send keys to. Open a file in a text editor first."));
            }

            // The keys may close the editor - a <C-w> window command removes
            // the split synchronously - so nothing of it may be touched after
            // handleInput() without checking that the widget survived.
            const QPointer<QWidget> widget = handler->widget();
            handler->handleInput(keys);

            QJsonObject result{{"keys", keys}};
            if (widget) {
                const QTextCursor tc = handler->textCursor();
                result.insert("line", tc.blockNumber() + 1);
                result.insert("column", tc.positionInBlock() + 1);
            }
            return CallToolResult{}.isError(false).structuredContent(result);
        });
}

} // namespace FakeVim::Internal
