---@meta TextDocument

---@class TextDocument
local TextDocument = {}

---Returns the file path of the document.
---@return FilePath filePath The file path of the document.
function TextDocument:file() end

---Returns the line and column of the given position.
---@param position integer The position to convert.
---@return integer line The line of the position.
---@return integer column The column of the position.
function TextDocument:lineAndColumn(position) end

return TextDocument
