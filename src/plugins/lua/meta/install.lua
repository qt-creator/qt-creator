---@meta Install

local Install = {}

---@class PackageInfo
---@field name string The name of the package.
---@field version string The version of the package.
---@field path FilePath The path to the package.

local PackageInfo = {}
---@class InstallOptions
---@field name string The name of the package to install.
---@field url string The url to fetch the package from.
---@field version string The version of the package to install.
local InstallOptions = {}

---Install something
---@param msg string The message to display to the user asking for permission to install.
---@param options InstallOptions|[InstallOptions] The options to install.
---@return boolean Result Whether the installation was successful.
---@return string Error The error message if the installation failed.
function Install.install(msg, options) end

---Get the package info
---@param name any The name of the package.
---@return PackageInfo
function Install.packageInfo(name) end

return Install
