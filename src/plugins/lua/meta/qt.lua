---@meta Qt

--- The values in enums here do not matter, as they are defined by the C++ code.

local qt = {}

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
