---@meta Process

local process = {}

---@async
---Runs a command in a terminal, has to be called from a coroutine!
---@param cmd string The command to run
---@return number The exit code of the command
function process.runInTerminal(cmd) end

---@async
---Runs a command and returns the output!
---@param cmd string The command to run
---@return string The output of the command
function process.commandOutput(cmd) end

return process
