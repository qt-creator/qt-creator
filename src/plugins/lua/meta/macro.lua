-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

---@meta Macro

local macro = {}

---Returns globalExpander():value(variableName).
function macro.value(variableName) end

---Returns globalExpander():expand(stringWithVariables).
function macro.expand(stringWithVariables) end

return macro
