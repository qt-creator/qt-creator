-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
return {
    Id = "lualanguageserver",
    Name = "Lua Language Server",
    Version = "1.0.0",
    CompatVersion = "1.0.0",
    VendorId = "theqtcompany",
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
        { Id = "lua",               Version = "15.0.0" },
        { Id = "lualanguageclient", Version = "15.0.0" }
    },
    setup = function()
        require 'init'.setup()
    end,
} --[[@as QtcPlugin]]
