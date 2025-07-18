---@meta Menu
local menu = {}

---@class ContainerAndGroup
---@field containerId string The id of the container.
---@field groupId string The id of the group.
menu.ContainerAndGroup = {}

---@class MenuOptions
---@field title string The title of the menu.
---@field icon? IconFilePathOrString The icon of the menu.
---@field containers? string[]|ContainerAndGroup[] The containers into which the menu should be added.
menu.MenuOptions = {}

---Creates a new menu.
---@param id string The id of the menu.
---@param options MenuOptions The options for the menu.
function menu.create(id, options) end

---Adds a separator to the menu.
---@param id string The id of the separator.
function menu.addSeparator(id) end

return menu
