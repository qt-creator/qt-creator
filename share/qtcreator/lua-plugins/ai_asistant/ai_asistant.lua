-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
return {
    Name = "AiAssistant",
    Version = "1.0.0",
    CompatVersion = "1.0.0",
    Vendor = "The Qt Company",
    Category = "Language Client",
    Description = "The AI Assistant Server",
    Experimental = true,
    DisabledByDefault = true,
    LongDescription = [[
This plugin provides the AI Assistant Server.
It will try to install it if it is not found.
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
            }
        }
    },
    setup = function()
        require 'init'.setup()
    end,
} --[[@as QtcPlugin]]
