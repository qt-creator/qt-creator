---@meta Process

local process = {}

---@async
---Runs a command in a terminal, has to be called from a coroutine!
---@param cmd string The command to run
---@return number The exit code of the command
function process.runInTerminal(cmd) end

return process
