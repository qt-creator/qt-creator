TaskHub = require 'TaskHub'


return {
    categoryAdded = function(category)
        print("TaskHub category added:", category)
    end,
    taskAdded = function(task)
        print("TaskHub task added:", task)
    end
}
