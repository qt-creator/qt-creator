---@meta Process

local process = {}

---@async
---Runs a command in a terminal, has to be called from a coroutine!
---@param cmd string The command to run.
---@return number exitCode The exit code of the command.
function process.runInTerminal(cmd) end

---@async
---Runs a command and returns the output!
---@param cmd string The command to run.
---@return string output The output of the command.
function process.commandOutput(cmd) end

---@class Process
process.Process = {}

---@class ProcessParameters
---@field command FilePath The command to run.
---@field arguments? string[] The arguments to pass to the command.
---@field workingDirectory? FilePath The working directory for the command.
---@field stdin? string The input to write to stdin.
---@field stdout? function The callback to call when the process writes to stdout.
---@field stderr? function The callback to call when the process writes to stderr.
process.ProcessParameters = {}

---Creates a new process object.
---@param parameters ProcessParameters
---@return Process
function process.create(parameters) end

---Start the process.
---@async
---@return boolean isRunning Whether the process was started successfully.
function process.Process:start() end

---Stop the process.
---@async
---@return boolean didStop Whether the process was stopped successfully.
---@return number exitCode The exit code of the process.
---@return string error The error message if the process could not be stopped.
function process.Process:stop() end

---Returns whether the process is running.
---@return boolean isRunning Whether the process is running.
function process.Process:isRunning() end

---
return process
