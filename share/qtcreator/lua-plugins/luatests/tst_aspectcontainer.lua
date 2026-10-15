-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
T = require("qtctest")
local S = require('Settings')

local function testAutoApply()
    local container = S.AspectContainer.create({ autoApply = true })

    container.test = S.BoolAspect.create({ defaultValue = true })
    T.compare(container.test.value, true)
    container.test.volatileValue = false
    T.compare(container.test.volatileValue, false)
    T.compare(container.test.value, false)
end

local function testNoAutoApply()
    local container = S.AspectContainer.create({ autoApply = false })

    container.test = S.BoolAspect.create({ defaultValue = true })
    T.compare(container.test.value, true)
    container.test.volatileValue = false
    T.compare(container.test.volatileValue, false)
    T.compare(container.test.value, true)
    container:apply()
    T.compare(container.test.volatileValue, false)
    T.compare(container.test.value, false)
end

local function testIntegerAspectRange()
    local aspect = S.IntegerAspect.create({ range = { 0, 1000 }, defaultValue = 500 })
    T.compare(aspect.value, 500)
end

local function expectIntegerAspectError(options, message)
    local ok, err = pcall(S.IntegerAspect.create, options)
    T.compare(ok, false)
    T.compare(string.find(tostring(err), message, 1, true) ~= nil, true)
end

local function testIntegerAspectInvalidRange()
    expectIntegerAspectError({ range = 5 }, "range must be a table")
    expectIntegerAspectError({ range = "0-10" }, "range must be a table")
    expectIntegerAspectError({ range = { 0 } }, "range must be a table")
    expectIntegerAspectError({ range = { 10, 0 } }, "minimum must not be greater than maximum")
    expectIntegerAspectError({ range = { 0, math.maxinteger } }, "32-bit")
    expectIntegerAspectError({ range = { math.mininteger, 0 } }, "32-bit")
end

return {
    testAutoApply = testAutoApply,
    testNoAutoApply = testNoAutoApply,
    testIntegerAspectRange = testIntegerAspectRange,
    testIntegerAspectInvalidRange = testIntegerAspectInvalidRange,
}
