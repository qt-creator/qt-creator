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
---@field asModeAction? integer Register the action as a mode action with the given priority.
ActionOptions = {}

---Creates a new Action.
---@param id string The id of the action.
---@param options ActionOptions
---@return Command command The created command.
function action.create(id, options) end

---@class Command
---@field enabled boolean Whether the command is enabled or not.
---@field text string The text of the command. Make sure to specify `commandAttributes = CommandAttribute.CA_UpdateText` in the options.
---@field tooltip string The tooltip of the command. Make sure to specify `commandAttributes = CommandAttribute.CA_UpdateText` in the options.
Command = {}

---return the module
return action
