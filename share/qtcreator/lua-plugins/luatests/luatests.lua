-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
return {
    Id = "luatests",
    DisplayName = "Lua Tests",
    Name = "LuaTests",
    Version = "1.0.0",
    CompatVersion = "1.0.0",
    VendorId = "theqtcompany",
    Vendor = "The Qt Company",
    Category = "Tests",
    DisabledByDefault = true,
    Experimental = true,
    Description = "This plugin tests the Lua API.",
    LongDescription = [[
        It has tests for (almost) all functionality exposed by the API.
    ]],
    Dependencies = {
        { Id = "lua", Version = "15.0.0" }
    },
    Type = "Script",
    setup = function() require 'tests'.setup() end,
    printToOutputPane = true,
    hooks = {
        taskHub = {
            categoryAdded = function(category)
                require 'taskhubhooks'.categoryAdded(category)
            end,
            taskAdded = function(task)
                require 'taskhubhooks'.taskAdded(task)
            end
        }
    }
} --[[@as QtcPlugin]]
