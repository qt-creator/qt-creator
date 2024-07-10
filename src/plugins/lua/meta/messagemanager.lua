---@meta MessageManager

local messagemanager = {}

---Writes a message to the Output pane.
---@param ... any
function messagemanager.writeSilently(...) end

---Writes a message to the Output pane and flashes the pane if its not open.
---@param ... any
function messagemanager.writeFlashing(...) end

---Writes a message to the Output pane and opens the pane if its not open.
---@param ... any
function messagemanager.writeDisrupting(...) end

return messagemanager
