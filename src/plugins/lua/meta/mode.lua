---@meta Mode
local mode = {}

---@class Mode
---@field id string The id of the mode.
local modeHandle = {}

---Activates (switches to) this mode.
function modeHandle:activate() end

---Enables or disables the mode's tab.
---@param enabled boolean
function modeHandle:setEnabled(enabled) end

---@class (exact) ModeOptions
---@field id string The id of the mode (required).
---@field widget Widget The mode's full-window content widget (required).
---@field displayName? string The name shown in the mode selector (defaults to the id).
---@field priority? integer Sort priority in the mode bar; higher is further up.
---@field icon? FilePath Path to the icon shown in the mode selector. It is tinted to the theme's icon color (alpha preserved) so it stays visible on the dark mode bar -- themed by default here, unlike the opt-in icon tinting elsewhere in the Lua API.
mode.ModeOptions = {}

---Adds a mode (a left-side tab with its own full-window widget). The mode is
---removed again when the plugin is unloaded.
---@param options ModeOptions
---@return Mode
function mode.create(options) end

return mode
