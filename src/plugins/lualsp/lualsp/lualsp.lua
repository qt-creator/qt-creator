-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
return {
    Name = "LuaLanguageServer",
    Version = "1.0.0",
    CompatVersion = "1.0.0",
    Vendor = "The Qt Company",
    Category = "Language Client",
    Description = "The Lua Language Server",
    Experimental = false,
    DisabledByDefault = false,
    LongDescription = [[
This plugin provides the Lua Language Server.
It will try to install it if it is not found.
    ]],
    Dependencies = {
        { Name = "Core",              Version = "13.0.82", Required = true },
        { Name = "Lua",               Version = "13.0.82", Required = true },
        { Name = "LuaLanguageClient", Version = "13.0.82", Required = true }
    },
    setup = function()
        require 'init'.setup()
    end,
    hooks = {
        editors = {
            documentOpened = function(doc) print("documentOpened", doc) end,
            documentClosed = function(doc) print("documentClosed", doc) end,
        }
    }
} --[[@as QtcPlugin]]
