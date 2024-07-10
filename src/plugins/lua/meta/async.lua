---@meta async

local async = {}


---Wraps the provided function so it can be started in another thread.
---
--- Example:
--- ```lua
--- local a = require("async")
--- local u = require("Utils")
---
--- function asyncFunction()
---   a.wait(u.waitms(500))
--- end
---
--- a.sync(asyncFunction)()
--- ```
---@param func function The function to call from the new thread.
function async.sync(func) end

---@async
---Calls an async function and waits for it to finish. **Must** be called from async.sync().
---
--- Example:
--- ```lua
--- local a = require("async")
--- local u = require("Utils")
---
--- function asyncFunction()
---   a.wait(u.waitms(500))
---   a.wait(u.waitms(1000))
--- end
---
--- a.sync(asyncFunction)()
--- ```
---@param func any The function to call and wait for its result.
---@return any any The result of the function.
function async.wait(func) end

---@async
---Calls multiple async functions and waits for all of them to finish. **Must** be called from async.sync().
---
--- Example:
--- ```lua
--- local a = require("async")
--- local u = require("Utils")
---
--- function asyncFunction()
---   a.wait_all {
---     u.waitms(500),
---     u.waitms(1000),
---   }
--- end
---
--- a.sync(asyncFunction)()
--- ```
---@param funcs table The functions to call and wait for.
---@return table table The result of each of the functions as an array.
function async.wait_all(funcs) end

return async
