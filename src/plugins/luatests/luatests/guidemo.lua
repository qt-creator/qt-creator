-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

local Utils = require("Utils")
local Gui = require("Gui")

local function using(tbl)
    local result = _G
    for k, v in pairs(tbl) do result[k] = v end
    return result
end

local function show()
    --- "using namespace Gui"
    local _ENV = using(Gui)

    Widget {
        size = { 400, 300 },
        Row { "Hello World!" },
    }:show()
end

return show
