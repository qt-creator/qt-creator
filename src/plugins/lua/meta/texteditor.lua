---@meta TextEditor
local textEditor = {}

---@module 'Utils'
local Utils


---@class Position
---@field line integer The line number.
---@field column integer The column number.
local Position = {}

---@class Range
---@field from Position The beginning position of the range.
---@field to Position The end position of the range.
local Range = {}

---@class TextCursor
local TextCursor = {}

---@enum TextCursor.MoveOperation
---Move operations for a TextCursor.
TextCursor.MoveOperation = {
    NoMove = 0,

    Start = 0,
    Up = 0,
    StartOfLine = 0,
    StartOfBlock = 0,
    StartOfWord = 0,
    PreviousBlock = 0,
    PreviousCharacter = 0,
    PreviousWord = 0,
    Left = 0,
    WordLeft = 0,

    End = 0,
    Down = 0,
    EndOfLine = 0,
    EndOfWord = 0,
    EndOfBlock = 0,
    NextBlock = 0,
    NextCharacter = 0,
    NextWord = 0,
    Right = 0,
    WordRight = 0,

    NextCell = 0,
    PreviousCell = 0,
    NextRow = 0,
    PreviousRow = 0
}

---@enum TextCursor.MoveMode
---Specifies how the cursor moves (with or without anchoring the selection).
TextCursor.MoveMode = {
    MoveAnchor = 0,
    KeepAnchor = 0
}

---Creates a new `TextCursor` object.
---
---**Overload 1**: `TextCursor.create()`
---  No parameters. Creates a default-constructed cursor.
---
---**Overload 2**: `TextCursor.create(doc)`
---  - `doc`: A `QTextDocument` (or usertype) from which to create the cursor. The position will be at the start of the document.
---
---**Overload 3**: `TextCursor.create(doc)`
---  - `doc`: A TextDocument for which to create the cursor. The position will be at the start of the document. (since 17.0.0)
---
---**Overload 4**: `TextCursor.create(other)`
---  - `other`: Another `TextCursor` to copy.
---
---@overload fun(): TextCursor
---@overload fun(doc: any): TextCursor
---@overload fun(doc: TextDocument): TextCursor
---@overload fun(other: TextCursor): TextCursor
---@return TextCursor
function TextCursor.create(...) end

---Returns the position of the cursor.
---@return integer position The position of the cursor.
function TextCursor:position() end

---Returns the block (line) and column for the cursor.
---@return integer block The block (line) of the cursor.
function TextCursor:blockNumber() end

---Returns the column for the cursor.
---@return integer column The column of the cursor.
function TextCursor:columnNumber() end

---Returns true if the cursor has a selection, false otherwise.
---@return boolean hasSelection True if the cursor has a selection, false otherwise.
function TextCursor:hasSelection() end

---Returns the selected text of the cursor.
---@return string selectedText The selected text of the cursor.
function TextCursor:selectedText() end

---Returns the range of selected text of the cursor.
---@return Range selectionRange The range of selected text of the cursor.
function TextCursor:selectionRange() end

---Inserts the passed text at the cursors position overwriting any selected text.
---@param text string The text to insert.
function TextCursor:insertText(text) end

---Moves the cursor using a specified operation, and optionally a mode and/or repetition count.
---
---**Overload 1**: `cursor:movePosition(operation)`
---  - `operation`: A `TextCursor.MoveOperation`.
---  - Moves once, using the default mode `MoveAnchor`.
---
---**Overload 2**: `cursor:movePosition(operation, mode)`
---  - `operation`: A `TextCursor.MoveOperation`.
---  - `mode`: A `TextCursor.MoveMode` (e.g. `MoveAnchor` or `KeepAnchor`).
---
---**Overload 3**: `cursor:movePosition(operation, mode, n)`
---  - `operation`: A `TextCursor.MoveOperation`.
---  - `mode`: A `TextCursor.MoveMode`.
---  - `n`: Number of times to repeat the move.
---
---@overload fun(operation: TextCursor.MoveOperation)
---@overload fun(operation: TextCursor.MoveOperation, mode: TextCursor.MoveMode)
---@overload fun(operation: TextCursor.MoveOperation, mode: TextCursor.MoveMode, n: integer)
function TextCursor:movePosition(operation, mode, n) end


---@class MultiTextCursor
local MultiTextCursor = {}

---Returns the main cursor.
---@return TextCursor mainCursor The main cursor.
function MultiTextCursor:mainCursor() end

---Returns the cursors.
---@return TextCursor[] cursors The cursors.
function MultiTextCursor:cursors() end

---Inserts the passed text at all cursor positions overwriting any selected text.
---@param text string The text to insert.
function MultiTextCursor:insertText(text) end

---@class Suggestion
local Suggestion = {}

---@class SuggestionParams
---@field text string The text of the suggestion.
---@field position Position The cursor position where the suggestion should be inserted.
---@field range Range The range of the text preceding the suggestion.
SuggestionParams = {}

---Creates Suggestion.
---@param params SuggestionParams Parameters for creating the suggestion.
---@return Suggestion suggestion The created suggestion.
function Suggestion:create(params) end

---@class TextDocument
local TextDocument = {}

---Returns the file path of the document.
---@return FilePath filePath The file path of the document.
function TextDocument:file() end

---Returns the font of the document.
---@return QFont
function TextDocument:font() end

---Returns the block (line) and column for the given position.
---@param position integer The position to convert.
---@return integer block The block (line) of the position.
---@return integer column The column of the position.
function TextDocument:blockAndColumn(position) end

---Returns the number of blocks (lines) in the document.
---@return integer blockCount The number of blocks in the document.
function TextDocument:blockCount() end

---Sets the suggestions for the document and enables toolTip on the mouse cursor hover.
---@param suggestions Suggestion[] A list of possible suggestions to display.
function TextDocument:setSuggestions(suggestions) end

---@class TextEditor
local TextEditor = {}

---Returns the document of the editor.
---@return TextDocument document The document of the editor.
function TextEditor:document() end

---Returns the cursor of the editor.
---@return MultiTextCursor cursor The cursor of the editor.
function TextEditor:cursor() end

---@class EmbeddedWidget
local EmbeddedWidget = {}

---Closes the floating widget.
function EmbeddedWidget:close() end

---Resizes the floating widget according to its layout.
function EmbeddedWidget:resize() end

---Set the callback to be called when the widget should close. (E.g. if the user presses the escape key)
---@param fn function The function to be called when the embed should close.
function EmbeddedWidget:onShouldClose(fn) end

---Embeds a widget at the specified cursor position in the text editor.
---@param widget Widget|Layout The widget to be added as a floating widget.
---@param position integer|Position The position in the document where the widget should appear.
---@return EmbeddedWidget interface An interface to control the floating widget.
function TextEditor:addEmbeddedWidget(widget, position) end

---Adds an refactor marker in the text editor at given cursor position.
---@param icon Utils.Icon|FilePath|string Icon to be used. If specified icon is invalid the default QtCreator for markers is used.
---@param position integer The position in the document where the marker should appear.
---@param id string The identifier of the marker.
---@param anchorLeft boolean Specifies if the marker should appear at the beginning of the TextCursor block.
---@param callback function A function to be called once the marker is pressed.
function TextEditor:setRefactorMarker(icon, position, id, anchorLeft, callback) end

---Removes the refactor markers with given id.
---param id string The identifier of the marker.
function TextEditor:clearRefactorMarkers(icon, position, id, anchorLeft, callback) end

---Checks if the current suggestion is locked. The suggestion is locked when the user can use it.
---@return boolean True if the suggestion is locked, false otherwise.
function TextEditor:hasLockedSuggestion() end

---Inserts the passed text at all cursor positions overwriting any selected text.
---@param text string The text to insert.
function TextEditor:insertText(text) end

---Indicates if the editor widget has focus.
---@return boolean hasFocus True if the editor widget has focus, false otherwise.
function TextEditor:hasFocus() end

---Returns the current editor or nil.
---@return TextEditor|nil editor The currently active editor or nil if there is none.
function textEditor.currentEditor() end

return textEditor
