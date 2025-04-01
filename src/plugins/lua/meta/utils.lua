---@meta Utils

local utils = {}

---The Process ID of Qt Creator.
utils.pid = 0

---Suspends the current coroutine for the given amount of milliseconds. Call `a.wait` on the returned value to get the result.
---@param ms number The amount of milliseconds to wait.
function utils.waitms(ms) end

---Calls the callback after the given amount of milliseconds.
---@param ms number The amount of milliseconds to wait.
---@param callback function The callback to call.
function utils.waitms_cb(ms, callback) end

---Creates a UUID.
---@return QString Arbitrary UUID string.
function utils.createUuid() end

---Converts a given shortcut string into its native representation for the current platform.
---@param shortcut string The shortcut string (e.g., "Ctrl+Shift+A").
---@return QString The native representation of the shortcut.
function utils.getNativeShortcut(shortcut) end

---@class Id
utils.Id = {}

---@class FilePath
utils.FilePath = {}

---@param path string The path to convert.
---@return FilePath The converted path.
---Convert and clean a path, returning a FilePath object.
function utils.FilePath.fromUserInput(path) end

---@return FilePath The new absolute path.
---Searches for the path inside the PATH environment variable. Call `a.wait` on the returned value to get the result.
function utils.FilePath:searchInPath() end

---@class (exact) DirEntriesOptions
---@field nameFilters? string[] The name filters to use (e.g. "*.lua"), defaults to all files.
---@field fileFilters? integer The filters to use (combination of QDir.Filters.*), defaults to QDir.Filters.NoFilter.
---@field flags? integer The iterator flags (combination of QDirIterator.Flags.*), defaults to QDirIterator.Flags.NoIteratorFlags.

---Returns all entries in the directory. Call `a.wait` on the returned value to get the result.
---@param options DirEntriesOptions
---@return FilePath[]
function utils.FilePath:dirEntries(options) end

---Returns the FilePath as it should be displayed to the user.
---@return string
function utils.FilePath:toUserOutput() end

---Returns whether the target exists.
---@return boolean
function utils.FilePath:exists() end

---Returns whether the target is a file and executable.
---@return boolean
function utils.FilePath:isExecutableFile() end

---Returns the path portion of FilePath as a string in the hosts native format.
---@return string
function utils.FilePath:nativePath() end

---Returns the last part of the path.
---@return string
function utils.FilePath:fileName() end

---Returns the current working path of Qt Creator.
---@return FilePath
function utils.FilePath.currentWorkingPath() end

---Returns a new FilePath with the given tail appended.
---@param tail string|FilePath The tail to append.
---@return FilePath
function utils.FilePath:resolvePath(tail) end

---Returns the parent directory of the path.
---@return FilePath
function utils.FilePath:parentDir() end

---If the path targets a symlink, this function returns the target of the symlink.
---@return FilePath resolvedPath The resolved path.
function utils.FilePath:resolveSymlinks() end

---Returns the suffix of the paths (e.g. "test.ui.qml" -> ".qml").
---@return string
function utils.FilePath:suffix() end

---Returns the complete suffix of the paths (e.g. "test.ui.qml" -> "ui.qml").
---@return string
function utils.FilePath:completeSuffix() end

---Returns whether the path is absolute.
---@return boolean
function utils.FilePath:isAbsolutePath() end

---Returns the complete OR-ed together combination of permissions for the file.
---@return Permission
function utils.FilePath:permissions() end

---Sets permissions for the file.
---@param permissions Permission The complete OR-ed together combination of permissions for the file.
function utils.FilePath:setPermissions() end

---Returns the list of paths for the given standard location.
---@param location StandardLocation The standard location to get paths for.
---@return [FilePath] The list of paths for the given standard location.
function utils.standardLocations(location) end

---Returns the first available paths for the given standard location.
---@param location StandardLocation The standard location to get the path for.
---@return FilePath|nil The first available paths for the given standard location or nil if no location is available.
function utils.standardLocation(location) end

---Returns the writable paths for the given standard location.
---@param location StandardLocation The standard location to get path for.
---@return FilePath|nil The writable paths for the given standard location or nil if no writable location is available.
function utils.writableLocation(location) end

---@class CommandLine
---@field command FilePath The command to execute.
---@field arguments string[] The arguments to pass to the command.
utils.CommandLine = {}

---Add an argument to the command line.
---@param argument string The argument to add.
function utils.CommandLine:addArgument(argument) end

---@class ProcessRunData
---@field commandLine CommandLine The command line to execute.
---@field workingDirectory FilePath The working directory for the process.
---@field environment Environment The environment variables for the process.
utils.ProcessRunData = {}

---@class HostOsInfo
---@field os "mac"|"windows"|"linux" The current host operating system.
---@field architecture "unknown"|"x86"|"x86_64"|"itanium"|"arm"|"arm64" The current host architecture.
utils.HostOsInfo = {}

---Returns whether the host operating system is windows.
---@return boolean
function utils.HostOsInfo.isWindowsHost() end

---Returns whether the host operating system is mac.
---@return boolean
function utils.HostOsInfo.isMacHost() end

---Returns whether the host operating system is linux.
---@return boolean
function utils.HostOsInfo.isLinuxHost() end


---@class Timer
utils.Timer = {}

---@param timeoutMs integer The timeout in milliseconds.
---@param singleShot boolean Whether the timer should only fire once.
---@param callback function The callback to call when the timeout is reached.
---@return Timer timer The created timer.
function utils.Timer.create(timeoutMs, singleShot, callback) end

--- Starts the timer. Calling start on a running timer restarts the timer.
function utils.Timer:start() end

--- Stops the timer.
function utils.Timer:stop() end

---Opens the given URL in the default browser.
---@param url string The URL to open.
function utils.openExternalUrl(url) end

---Converts a string to a base64 URL encoding. Instead of using "+" and "/" characters, it uses "-" and "_".
---@param text string The text to convert.
---@return string The base64 URL encoded string.
function utils.stringToBase64Url(text) end

---Converts a base64 URL encoded string back to a normal string.
---@param text string The base64 URL encoded string.
---@return string The decoded string.
function utils.base64UrlToString(text) end

---Converts a string to a base64 encoding.
---@param text string The text to convert.
---@return string The base64 encoded string.
function utils.stringToBase64(text) end

---Converts a base64 encoded string back to a normal string.
---@param text string The base64 encoded string.
---@return string The decoded string.
function utils.base64ToString(text) end
---@class Icon
utils.Icon = {}

---@enum IconStyleOption
utils.IconStyleOption = {
    None = 0,
    Tint = 0,
    DropShadow = 0,
    PunchEdges = 0,
    ToolBarStyle = 0,
    MenuTintedStyle = 0,
}

---@enum ThemeColor
utils.ThemeColor = {
    Token_Basic_Black = 0,
    Token_Basic_White = 0,
    Token_Accent_Default = 0,
    Token_Accent_Muted = 0,
    Token_Accent_Subtle = 0,
    Token_Background_Default = 0,
    Token_Background_Muted = 0,
    Token_Background_Subtle = 0,
    Token_Foreground_Default = 0,
    Token_Foreground_Muted = 0,
    Token_Foreground_Subtle = 0,
    Token_Text_Default = 0,
    Token_Text_Muted = 0,
    Token_Text_Subtle = 0,
    Token_Text_Accent = 0,
    Token_Stroke_Strong = 0,
    Token_Stroke_Muted = 0,
    Token_Stroke_Subtle = 0,
    Token_Notification_Alert_Default = 0,
    Token_Notification_Alert_Muted = 0,
    Token_Notification_Alert_Subtle = 0,
    Token_Notification_Success_Default = 0,
    Token_Notification_Success_Muted = 0,
    Token_Notification_Success_Subtle = 0,
    Token_Notification_Neutral_Default = 0,
    Token_Notification_Neutral_Muted = 0,
    Token_Notification_Neutral_Subtle = 0,
    Token_Notification_Danger_Default = 0,
    Token_Notification_Danger_Muted = 0,
    Token_Notification_Danger_Subtle = 0,
    Token_Gradient01_Start = 0,
    Token_Gradient01_End = 0,
    Token_Gradient02_Start = 0,
    Token_Gradient02_End = 0
}

---@class MaskAndColor
---@field mask string|FilePath The mask to use.
---@field color ThemeColor The color to use.
utils.MaskAndColor = {}

---Creates an icon from the given file path.
---@param maskAndColor MaskAndColor[] An array of MaskAndColor.
---@param style IconStyleOption The style to use.
---@return Icon icon The created icon.
---@overload fun(path: FilePath): Icon
---@overload fun(maskAndColor: [MaskAndColor], style: IconStyleOption)
function utils.Icon.create(maskAndColor, style) end
return utils
