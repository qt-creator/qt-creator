---@meta Action

local action = {}

---@enum CommandAttributes
action.CommandAttribute = {
    ---Hide the command from the menu.
    CA_Hide = 1,
    ---Update the text of the command.
    CA_UpdateText = 2,
    ---Update the icon of the command.
    CA_UpdateIcon = 4,
    ---The command cannot be configured.
    CA_NonConfigurable = 8,
}

---@class ActionOptions
---@field context? string The context in which the action is available.
---@field text? string The text to display for the action.
---@field iconText? string The icon text to display for the action.
---@field toolTip? string The tooltip to display for the action.
---@field onTrigger? function The callback to call when the action is triggered.
---@field commandAttributes? CommandAttributes The attributes of the action.
---@field commandDescription? string The description of the command.
---@field defaultKeySequence? string The default key sequence for the action.
---@field defaultKeySequences? string[] The default key sequences for the action.
local ActionOptions = {}

---Creates a new Action.
---@param id string The id of the action.
---@param options ActionOptions
function action.create(id, options) end

return action
