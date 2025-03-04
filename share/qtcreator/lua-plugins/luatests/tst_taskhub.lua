local a = require("async")
local U = require("Utils")

local function testTaskHub()
    TaskHub = require 'TaskHub'

    TaskHub.addCategory({ id = "Test.Category", displayName = "TestCat", description = "Test category for tasks", priority = 100 })

    local firstTask = TaskHub.Task.create({
        type = TaskHub.TaskType.Unknown,
        description = ">> Welcome to the Lua TaskHub Demo <<",
        file = "",
        line = -1,
        category = "Test.Category"
    })
    TaskHub.addTask(firstTask)

    a.wait(U.waitms(500))
    TaskHub.addTask(TaskHub.TaskType.Warning,
        "For the full experience, create a file /tmp/test.txt with at least 10 lines of text.", "Test.Category");
    a.wait(U.waitms(500))

    TaskHub.addTask(TaskHub.TaskType.Unknown, "This is an unknown type task", "Test.Category");
    a.wait(U.waitms(500))
    TaskHub.addTask(TaskHub.TaskType.Error, "This is an error type task", "Test.Category");
    a.wait(U.waitms(500))
    TaskHub.addTask(TaskHub.TaskType.Warning, "This is a warning type task", "Test.Category");
    a.wait(U.waitms(500))

    local myTask = TaskHub.Task.create({
        type = TaskHub.TaskType.Warning,
        description = "Manually created task (will be deleted in 5 seconds ...)",
        file = "/tmp/test.txt",
        line = 10,
        category = "Test.Category"
    })

    TaskHub.addTask(myTask)

    a.wait(U.waitms(5000))
    TaskHub.removeTask(myTask)

    TaskHub.addTask(TaskHub.TaskType.Unknown, "All other tasks will be removed in 5 seconds ...", "Test.Category");

    a.wait(U.waitms(5000))
    TaskHub.clearTasks("Test.Category")
end


local function setup()
    Action = require 'Action'
    Action.create("LuaTests.taskHub", {
        text = "Lua TaskHub Demo",
        onTrigger = a.sync(testTaskHub),
    })
end

return {
    setup = setup,
}
