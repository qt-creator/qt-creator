-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

---@meta Macro
local macro = {}
---@class MacroExpander
local MacroExpander = {}

---Expands all variables in the given string.
---@param stringWithVariables string The string with variables to expand.
---@return string The expanded string.
function MacroExpander:expand(stringWithVariables) end

---Returns the value of the given variable.
---@param variableName string The name of the variable.
---@return boolean ok Whether the variable was found.
---@return string value The value of the variable.
function MacroExpander:value(variableName) end

---Returns the global Macro expander
---@return MacroExpander
function macro.globalExpander() end

---Returns globalExpander():value(variableName).
---@param variableName string The name of the variable.
---@return boolean ok Whether the variable was found.
---@return string value The value of the variable.
function macro.value(variableName) end

---Returns globalExpander():expand(stringWithVariables).
---@param stringWithVariables string The string with variables to expand.
---@return string The expanded string.
function macro.expand(stringWithVariables) end

return macro
