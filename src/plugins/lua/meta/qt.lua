---@meta Qt

--- The values in enums here do not matter, as they are defined by the C++ code.

local qt = {}

---@enum CompleterCompletionMode
qt.CompleterCompletionMode = {
    PopupCompletion = 0,
    InlineCompletion = 1,
    UnfilteredPopupCompletion = 2,
};

---@class QCompleter
---@field completionMode CompleterCompletionMode The completion mode.
local QCompleter = {}

---Creates a new Completer.
---@param params string[] The list of suggestions.
---@return QCompleter completer The new Completer.
function qt.QCompleter.create(params) end

---Returns current completion.
---@return string
function qt.QCompleter:currentCompletion() end
---@param callback function The function to be called when user choice is selected from popup.
function qt.QCompleter.onActivated(callback) end

---@class QClipboard A Lua wrapper for the Qt `QClipboard` class.
---@field text string The text content of the clipboard. Gets or sets the text content of the clipboard.
qt.QClipboard = {}

---Returns the global clipboard object.
---@return QClipboard globalClipboard The global clipboard object.
function qt.clipboard() end

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

qt.QFileDevice = {
    ---@enum Permission
    Permission = {
        ReadOwner = 0,
        ReadUser = 0,
        ReadGroup = 0,
        ReadOther = 0,
        WriteOwner = 0,
        WriteUser = 0,
        WriteGroup = 0,
        WriteOther = 0,
        ExeOwner = 0,
        ExeUser = 0,
        ExeGroup = 0,
        ExeOther = 0,
    }
}

qt.QStandardPaths = {
    ---@enum StandardLocation
    StandardLocation = {
        DesktopLocation = 0,
        DocumentsLocation = 0,
        FontsLocation = 0,
        ApplicationsLocation = 0,
        MusicLocation = 0,
        MoviesLocation = 0,
        PicturesLocation = 0,
        TempLocation = 0,
        HomeLocation = 0,
        AppLocalDataLocation = 0,
        CacheLocation = 0,
        GenericDataLocation = 0,
        RuntimeLocation = 0,
        ConfigLocation = 0,
        DownloadLocation = 0,
        GenericCacheLocation = 0,
        GenericConfigLocation = 0,
        AppDataLocation = 0,
        AppConfigLocation = 0,
        PublicShareLocation = 0,
        TemplatesLocation = 0,
    }
}

return qt
