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

return {
    testAutoApply = testAutoApply,
    testNoAutoApply = testNoAutoApply,
}
