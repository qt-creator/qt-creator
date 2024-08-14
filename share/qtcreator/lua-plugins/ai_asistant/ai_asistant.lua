-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
return {
    Name = "AiAssistant",
    Version = "1.0.0",
    CompatVersion = "1.0.0",
    Vendor = "The Qt Company",
    Category = "Language Client",
    Description = "Qt AI Assistant",
    Experimental = true,
    DisabledByDefault = true,
    LongDescription = [[
The Qt AI Assistant extension is your personal coding assistant. The Qt AI Assistant can provide code suggestions triggered by a keyboard shortcut (CTRL-SHIFT-A), on request in an inline chat window, or automatically while you are typing. You can configure the Qt AI Assistant to collect suggestions from different commercial or open-source Large Language Models (LLM).

You also need one of the following valid Qt licenses: Qt for Application Development Enterprise, Qt for Device Creation Professional, Qt for Device Creation Enterprise, Qt for Small Business, Qt Evaluation License, Qt Education License.

You will need access to a LLM for the suggestions. You can use a subscription to a commercial, cloud-hosted LLM, a privately hosted or on-premise LLM, or a Small Language Model running locally on your computer.
    ]],
    Dependencies = {
        { Name = "Lua",               Version = "14.0.0" },
        { Name = "LuaLanguageClient", Version = "14.0.0" }
    },
    hooks = {
        editors = {
            text = {
                contentsChanged = function(document, position, charsRemoved, charsAdded)
                                    require 'init'.Hooks.onDocumentContentsChanged(document, position, charsRemoved, charsAdded)
                                  end,
                currentChanged = function(editor)
                                    require 'init'.Hooks.onCurrentChanged(editor)
                                  end,
            }
        }
    },
    setup = function()
        require 'init'.setup()
    end,
} --[[@as QtcPlugin]]
