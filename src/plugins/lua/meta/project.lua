---@meta Project
local project = {}

---@module 'Utils'

---@enum RunMode
project.RunMode {
    Normal = "RunConfiguration.NormalRunMode",
    Debug = "RunConfiguration.DebugRunMode",
}

---@class RunConfiguration
---@field runnable ProcessRunData
project.RunConfiguration = {}

---@class Project
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
function project.runStartupProject(runnable) end

return project
