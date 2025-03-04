---@meta TaskHub

TaskHub = {}

---@class TaskCategory
---@field id string A unique identifier for the category.
---@field displayName string The name of the category.
---@field description string A description of the category.
---@field visible boolean Whether the category is visible or not.
---@field priority integer The priority of the category.
TaskCategory = {}


---@enum TaskType
TaskHub.TaskType = {
    Unknown = 0,
    Error = 1,
    Warning = 2
}

---@enum Option
TaskHub.Option = {
    NoOptions = 0,
    AddTextMark = 1,
    FlashWorthy = 2
}

---@enum DescriptionTag
TaskHub.DescriptionTag = {
    WithSummary = 0,
    WithLinks = 1
}

---@class Task
---@field id string The unique identifier of the task.
---@field type TaskType The type of the task.
---@field options Option The options of the task. An integer with bitwise AND of `TaskHub::Option`.
---@field summary string The summary of the task.
---@field details string The details of the task.
---@field file FilePath The file referenced by the task.
---@field fileCandidates FilePath[] The list of file candidates referenced by the task.
---@field line integer The line inside `file` referenced by the task.
---@field movedLine integer The line inside `file` referenced by the task after the file was modified.
---@field column integer The column inside `file` referenced by the task.
---@field category string The id of the category of the task.
---@field description string The description of the task.
Task = {}


---@class TaskCreateParameters
---@field type TaskType
---@field description string
---@field file FilePath|string
---@field line integer
---@field category string
---@field icon? Icon|FilePath|string
---@field options? Option

---Create a new Task from the supplied parameters.
---@param parameters TaskCreateParameters
---@return Task task The created task.
function TaskHub.Task.create(parameters) end

---Add a new task to the TaskHub.
---@param type TaskType The type of the task.
---@param description string The description of the task.
---@param categoryId string The id of the category of the task.
function TaskHub.addTask(type, description, categoryId) end

---Add a new task to the TaskHub.
---@param task Task The task to add.
function TaskHub.addTask(task) end

---Clear all tasks from the TaskHub.
---@param categoryId string The id of the category to clear.
function TaskHub.clearTasks(categoryId) end

---Remove a task from the TaskHub.
---@param task Task The task to remove.
function TaskHub.removeTask(task) end

---@class TaskCategoryParameters
---@field id string A unique identifier for the category.
---@field displayName string The name of the category.
---@field description string A description of the category.
---@field priority? integer The priority of the category.
---@field visible? boolean Whether the category is visible or not.


---Register a new category to the TaskHub. If the Id is already in use the call will be ignored.
---@param parameters TaskCategoryParameters The category to add.
function TaskHub.addCategory(parameters) end

---Set the visibility of a category.
---@param categoryId string The id of the category to set visibility.
---@param visible boolean The visibility of the category.
function TaskHub.setCategoryVisibility(categoryId, visible) end

---Request the TaskHub to show a popup.
function TaskHub.requestPopup() end

return TaskHub
