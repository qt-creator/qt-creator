---@meta Json

local Json = {}

---Create a json string representing the table.
---@param table table The table to encode.
---@return string json The JSON string.
function Json.encode(table) end

---Decode a json string into a table.
---@param json string The JSON string.
---@return table table The decoded table.
function Json.decode(json) end

return Json
