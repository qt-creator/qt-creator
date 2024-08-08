---@meta TextEditor
local textEditor = {}

---@class TextCursor
---@field position integer The position of the cursor.
---@field blockNumber integer The block (line) number of the cursor.
---@field columnNumber integer The column number of the cursor.
local TextCursor = {}

---@class MultiTextCursor
---@field mainCursor TextCursor The main cursor.
---@field cursors TextCursor[] The cursors.
local MultiTextCursor = {}

---@class Suggestion
local Suggestion = {}

---@param startLine integer Start position line where to apply the suggestion.
---@param startCharacter integer Start position character where to apply the suggestion.
---@param endLine integer  End position line where to apply the suggestion.
---@param endCharacter integer End position character where to apply the suggestion.
---@param text string Suggestions text.
---@return Suggestion suggestion The created suggestion.
function Suggestion:create(startLine, startCharacter, endLine, endCharacter, text) end

---@class CyclicSuggestion
local CyclicSuggestion = {}

---@return boolean True if the suggestion is locked, false otherwise.
---Suggestion is locked when the user selects it and already started aplying it partially.
function CyclicSuggestion:isLocked() end

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

--- Sets the suggestions for the document and enables tooltip on the mouse cursor hover.
---@param suggestions Suggestion[] A list of possible suggestions to display
function TextDocument:setSuggestions(suggestions) end

---@class TextEditor
local TextEditor = {}

---Returns the document of the editor.
---@return TextDocument document The document of the editor.
function TextEditor:document() end

---Returns the cursor of the editor.
---@return MultiTextCursor cursor The cursor of the editor.
function TextEditor:cursor() end

---Returns the current editor or nil.
---@return TextEditor|nil editor The currently active editor or nil if there is none.
function textEditor.currentEditor() end

---@return CyclicSuggestion|nil The current suggestion if available. Otherwise nil.
function textEditor.currentSuggestion() end

return textEditor
