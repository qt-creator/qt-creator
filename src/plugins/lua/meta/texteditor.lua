---@meta TextEditor

---@class TextCursor
---@field position integer The position of the cursor.
---@field blockNumber integer The block (line) number of the cursor.
---@field columnNumber integer The column number of the cursor.
local TextCursor = {}

---@class MultiTextCursor
---@field mainCursor TextCursor The main cursor.
---@field cursors TextCursor[] The cursors.
local MultiTextCursor = {}

---@class TextDocument
local TextDocument = {}

---Returns the file path of the document.
---@return FilePath filePath The file path of the document.
function TextDocument:file() end

---Returns the block (line) and column for the given position.
---@param position integer The position to convert.
---@return integer block The block (line) of the position.
---@return integer column The column of the position.
function TextDocument:blockAndColumn(position) end

---Returns the number of blocks (lines) in the document.
---@return integer blockCount The number of blocks in the document.
function TextDocument:blockCount() end

---@class TextEditor
local TextEditor = {}

---Returns the document of the editor.
---@return TextDocument document The document of the editor.
function TextEditor:document() end

---Returns the cursor of the editor.
---@return MultiTextCursor cursor The cursor of the editor.
function TextEditor:cursor() end

return TextDocument
