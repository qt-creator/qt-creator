---@meta Qt

--- The values in enums here do not matter, as they are defined by the C++ code.

local qt = {}

---@enum CompleterCompletionMode
qt.CompleterCompletionMode = {
    PopupCompletion = 0,
    InlineCompletion = 1,
    UnfilteredPopupCompletion = 2,
};

---Creates QCompleter.
---@class QCompleter
---@field completionMode CompleterCompletionMode The completion mode.
local QCompleter = {}

---Returns current completion.
---@return string
function qt.QCompleter:currentCompletion() end

---@param params string list A list of suggestions.
---@return QCompleter Created Completer.
function qt.QCompleter.create(params) end

---@param function The function to be called when user choice is selected from popup.
function qt.QCompleter.onActivated(function) end

---@class QClipboard
--- A Lua wrapper for the Qt `QClipboard` class.

qt.QClipboard() Creates QClipboard object, which is a singleton instance of the system clipboard.
---@field text The text content of the clipboard. Gets or sets the text content of the clipboard.

---@enum TextElideMode
qt.TextElideMode = {
    ElideLeft = 0,
    ElideRight = 0,
    ElideMiddle = 0,
    ElideNone = 0,
}

qt.QDir = {
    ---@enum Filters
    Filters = {
        Dirs = 0,
        Files = 0,
        Drives = 0,
        NoSymLinks = 0,
        AllEntries = 0,
        TypeMask = 0,
        Readable = 0,
        Writable = 0,
        Executable = 0,
        PermissionMask = 0,
        Modified = 0,
        Hidden = 0,
        System = 0,
        AccessMask = 0,
        AllDirs = 0,
        CaseSensitive = 0,
        NoDot = 0,
        NoDotDot = 0,
        NoDotAndDotDot = 0,
        NoFilter = 0,
    },

    ---@enum SortFlags
    SortFlags = {
        Name = 0,
        Time = 0,
        Size = 0,
        Unsorted = 0,
        SortByMask = 0,
        DirsFirst = 0,
        Reversed = 0,
        IgnoreCase = 0,
        DirsLast = 0,
        LocaleAware = 0,
        Type = 0,
        NoSort = 0,
    }
}

qt.QDirIterator = {
    ---@enum IteratorFlag
    IteratorFlag = {
        NoIteratorFlags = 0,
        FollowSymlinks = 0,
        Subdirectories = 0,
    }
}

return qt
