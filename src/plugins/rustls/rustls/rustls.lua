-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
return {
    Name = "RustLanguageServer",
    Version = "1.0.0",
    CompatVersion = "1.0.0",
    Vendor = "The Qt Company",
    Category = "Language Client",
    Description = "The Rust Language Server",
    Experimental = false,
    DisabledByDefault = false,
    LongDescription = [[
This plugin provides the Rust Language Server.
It will try to install it if it is not found.
    ]],
    Dependencies = {
        { Name = "Lua",               Version = "14.0.0" },
        { Name = "LuaLanguageClient", Version = "14.0.0" }
    },
    setup = function()
        require 'init'.setup()
    end,
} --[[@as QtcPlugin]]
