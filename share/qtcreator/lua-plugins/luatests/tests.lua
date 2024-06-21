-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
local Utils = require("Utils")
local Action = require("Action")
local a = require("async")


local function script_path()
    local str = debug.getinfo(2, "S").source:sub(2)
    return str
end

local function printResults(results)
    print("Passed:", results.passed)
    print("Failed:", results.failed)
    for index, value in ipairs(results.failedTests) do
        print("Failed test:", value.name, value.error)
    end
end

local function runTest(testFile, results)
    local testScript, err = loadfile(testFile:nativePath())
    if not testScript then
        print("Failed to load test:", testFile, err)
        return
    end

    local ok, testFunctions = pcall(testScript)
    if not ok then
        print("Failed to run test:", testFile, testFunctions)
        return
    end

    for k, v in pairs(testFunctions) do
        print("* " .. testFile:fileName() .. " : " .. k)
        local ok, res_or_error = pcall(v)

        if ok then
            results.passed = results.passed + 1
        else
            results.failed = results.failed + 1
            table.insert(results.failedTests, { name = testFile:fileName() .. ":" .. k, error = res_or_error })
        end
    end
end

local function runTests()
    local results = {
        passed = 0,
        failed = 0,
        failedTests = {}
    }

    local testDir = Utils.FilePath.fromUserInput(script_path()):parentDir()
    local tests = a.wait(testDir:dirEntries({ nameFilters = { "tst_*.lua" } }))
    for _, testFile in ipairs(tests) do
        runTest(testFile, results)
    end
    printResults(results)
end

local function setup()
    Action.create("LuaTests.run", {
        text = "Run lua tests",
        onTrigger = function() a.sync(runTests)() end,
    })
    Action.create("LuaTests.layoutDemo", {
        text = "Lua Layout Demo",
        onTrigger = function()
            local script, err = loadfile(Utils.FilePath.fromUserInput(script_path()):parentDir():resolvePath(
            "guidemo.lua"):nativePath())
            if not script then
                print("Failed to load demo:", err)
                return
            end

            script()()
        end,
    })
end

return { setup = setup }
