-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
local T = require("qtctest")
local a = require('async')
local Utils = require('Utils')
local Qt = require('Qt')

local function testDirEntries()
    local u = require("Utils")
    local d = u.FilePath.currentWorkingPath()
    print("CWD:", d)
    local result = a.wait(d:dirEntries({}))
    print("RESULT:", result, #result)
end

local function testSearchInPath()
    local u = require("Utils")
    local d = u.FilePath.fromUserInput('hostname')
    local result = a.wait(d:searchInPath())
    print("Hostname found at:", result)
end

local function testWaitMs()
    local u = require("Utils")
    print("Starting to wait ...")
    a.wait(u.waitms(1000))
    print("About a second should have elapsed now.")
end

return {
    testDirEntries = testDirEntries,
    testSearchInPath = testSearchInPath,
    testWaitMs = testWaitMs,
}
