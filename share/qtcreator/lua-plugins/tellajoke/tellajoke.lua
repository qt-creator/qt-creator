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
        for i = 1, 3 do
            a.wait(utils.waitms(1000))
            mm.writeSilently(".")
        end
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
    Id = "tellajoke",
    Name = "Tell A Joke",
    Version = "1.0.0",
    CompatVersion = "1.0.0",
    VendorId = "theqtcompany",
    Vendor = "The Qt Company",
    Category = "Fun",
    Description = "This plugin adds an action that tells a joke.",
    Dependencies = {
        { Id = "lua", Version = "15.0.0" },
    },
    setup = setup,
} --[[@as QtcPlugin]]
