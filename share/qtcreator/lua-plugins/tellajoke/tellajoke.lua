--- Copyright (C) 2024 The Qt Company Ltd.
--- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

local function fetchJoke()
    print("Fetching ...")

    local a = require("async")
    local fetch = require("Fetch").fetch
    local utils = require("Utils")
    local mm = require("MessageManager")

    local r = a.wait(fetch({ url = "https://official-joke-api.appspot.com/random_joke", convertToTable = true }))
    if (type(r) == "table") then
        mm.writeDisrupting(r.setup)
        a.wait(utils.waitms(1000))
        mm.writeSilently(".")
        a.wait(utils.waitms(1000))
        mm.writeSilently(".")
        a.wait(utils.waitms(1000))
        mm.writeSilently(".")
        a.wait(utils.waitms(1000))
        mm.writeDisrupting(r.punchline)
    else
        print("echo Error fetching: " .. r)
    end
end

local function fetchJokeSafe()
    local ok, err = pcall(fetchJoke)
    if not ok then
        print("echo Error fetching: " .. err)
    end
end

local function setup()
    local a = require("async")
    Action = require("Action")
    Action.create("Simple.joke", {
        text = "Tell a joke",
        onTrigger = function() a.sync(fetchJokeSafe)() end,
        defaultKeySequences = { "Meta+Ctrl+Shift+J", "Ctrl+Shift+Alt+J" },
    })
end

return {
    Name = "Tell A Joke",
    Version = "1.0.0",
    CompatVersion = "1.0.0",
    Vendor = "The Qt Company",
    Category = "Fun",
    Description = "This plugin adds an action that tells a joke.",
    Dependencies = {
        { Name = "Lua", Version = "14.0.0" },
    },
    setup = setup,
} --[[@as QtcPlugin]]
