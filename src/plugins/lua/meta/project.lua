---@meta Project
local project = {}

---@module 'Utils'

---@enum RunMode
project.RunMode {
    Normal = "RunConfiguration.NormalRunMode",
    Debug = "RunConfiguration.DebugRunMode",
}

---@enum Platforms
project.Platforms {
    Desktop = 0,
}

---@class Kit
project.Kit = {}

---Returns the list of supported platforms (device types) for this kit.
---@return [Id] The list of supported platforms (device types) for this kit.
function project.Kit:supportedPlatforms() end

---@class RunConfiguration
---@field runnable ProcessRunData
---@field kit Kit
project.RunConfiguration = {}

---@class Project
---@field displayName string The display name of the project.
---@field directory FilePath The directory of the project.
project.Project = {}

---Returns the active run configuration of the project.
---@return RunConfiguration|nil The active run configuration of the project, or nil if there is no active run configuration.
function project.Project:activeRunConfiguration() end

---Returns the startup project.
---@return Project|nil The startup project, or nil if there is no startup project.
function project.startupProject() end

---Test whether the current startup project can be started using the specified run mode.
---@param runMode RunMode The run mode to test.
---@return boolean True if the current startup project can be started using the specified run mode; otherwise, false.
---@return string|nil If the project cannot be started, a message explaining why; otherwise, nil.
function project.canRunStartupProject(runMode) end

---Starts the active run configuration of the current startup project. It will be build first if necessary.
---@param runnable? ProcessRunData Override the run configuration with the specified runnable.
---@param displayName? string Override the run configuration display name with the provided name.
function project.runStartupProject(runnable, displayName) end

---Stops any configuration with display names equal to the provided name.
---@param displayName string The name for projects to stop.
---@param force? boolean Whether to close project forcefully (false assumeed if not provided).
---@return int Number of stopped configurations.
function project.stopRunConfigurationsByName(displayName, force) end

return project
