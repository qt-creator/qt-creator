// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include "utils.h"

#include <projectexplorer/taskhub.h>

#include <string>

using namespace ProjectExplorer;
using namespace std::literals::string_view_literals;
using namespace Utils;

namespace Lua::Internal {

// Template magic to get the arguments from a QObject signal function and forward them to a Lua function
template<typename Func>
struct FPTR;
template<class Obj, typename Ret, typename... Args>
struct FPTR<Ret (Obj::*)(Args...)>
{
    static std::function<void(Args...)> makeCallable(sol::protected_function func)
    {
        return [func](Args... args) {
            Result<> res = void_safe_call(func, args...);
            QTC_CHECK_RESULT(res);
        };
    }
};

template<typename Func1, typename... Args>
static void registerTaskHubHook(const QString &hookName, Func1 signal)
{
    registerHook("taskHub." + hookName, [signal](const sol::protected_function &func, QObject *guard) {
        using F = FPTR<Func1>;
        QObject::connect(&ProjectExplorer::taskHub(), signal, guard, F::makeCallable(func));
    });
};

static QString taskTypeToString(Task::TaskType type)
{
    switch (type) {
    case Task::Unknown:
        return "Unknown";
    case Task::Error:
        return "Error";
    case Task::Warning:
        return "Warning";
    default:
        return "Invalid";
    }
}

void setupTaskHubModule()
{
    registerTaskHubHook("categoryAdded", &TaskHub::categoryAdded);
    registerTaskHubHook("taskAdded", &TaskHub::taskAdded);
    registerTaskHubHook("taskRemoved", &TaskHub::taskRemoved);
    registerTaskHubHook("tasksCleared", &TaskHub::tasksCleared);
    registerTaskHubHook("categoryVisibilityChanged", &TaskHub::categoryVisibilityChanged);

    registerProvider("TaskHub", [](sol::state_view lua) -> sol::object {
        sol::table taskHub = lua.create_table_with(
            "addTask",
            sol::overload(
                [](Task::TaskType type, const QString &description, const QString &categoryId) {
                    TaskHub::addTask(type, description, Id::fromString(categoryId));
                },
                [](Task task) { TaskHub::addTask(task); }),
            "clearTasks",
            [](const QString &categoryId) { TaskHub::clearTasks(Id::fromString(categoryId)); },
            "removeTask",
            &TaskHub::removeTask,
            "addCategory",
            [](sol::table parameter) {
                TaskCategory category;
                category.id = Id::fromString(parameter.get<QString>("id"sv));
                category.displayName = parameter.get<QString>("displayName"sv);
                category.description = parameter.get<QString>("description"sv);
                category.visible = parameter.get_or<bool, std::string_view, bool>("visible"sv, true);
                category.priority = parameter.get_or<int, std::string_view, int>("priority"sv, 0);
                TaskHub::addCategory(category);
            },
            "setCategoryVisibility",
            [](const QString &categoryId, bool visible) {
                TaskHub::setCategoryVisibility(Id::fromString(categoryId), visible);
            },
            "requestPopup",
            &TaskHub::requestPopup);

        // clang-format off
        taskHub.new_enum("TaskType",
            "Unknown", Task::Unknown,
            "Error", Task::Error,
            "Warning", Task::Warning
        );

        taskHub.new_enum("Option",
            "NoOptions",Task::NoOptions,
            "AddTextMark", Task::AddTextMark,
            "FlashWorthy", Task::FlashWorthy
        );

        taskHub.new_enum("DescriptionTag",
            "WithSummary", Task::WithSummary,
            "WithLinks", Task::WithLinks
        );
        // clang-format on

        taskHub.new_usertype<TaskCategory>(
            "TaskCategory",
            sol::no_constructor,
            sol::meta_function::to_string,
            [](const TaskCategory &self) {
                return QString("TaskCategory{id=\"%1\", displayName=\"%2\", description=\"%3\", "
                               "visible=%4, priority=%5}")
                    .arg(self.id.toString())
                    .arg(self.displayName)
                    .arg(self.description)
                    .arg(self.visible ? QString("true") : QString("false"))
                    .arg(self.priority);
            },
            "id",
            sol::property([](TaskCategory &self) -> QString { return self.id.toString(); }),
            "displayName",
            sol::property(&TaskCategory::displayName),
            "description",
            sol::property(&TaskCategory::description),
            "visible",
            sol::property(&TaskCategory::visible),
            "priority",
            sol::property(&TaskCategory::priority));

        taskHub.new_usertype<Task>(
            "Task",
            sol::no_constructor,
            sol::meta_function::to_string,
            [](const Task &self) -> QString {
                return QString("Task{type=%1, category=\"%2\", description=\"%3\"}")
                    .arg(taskTypeToString(self.type))
                    .arg(self.category.toString())
                    .arg(self.description());
            },
            "create",
            [](sol::table parameter) {
                int type = parameter.get<int>("type");
                QString description = parameter.get<QString>("description"sv);
                FilePath file = toFilePath(parameter.get<FilePathOrString>("file"sv));
                int line = parameter.get<int>("line"sv);
                QString category = parameter.get<QString>("category"sv);
                std::optional<IconFilePathOrString> icon
                    = parameter.get<std::optional<IconFilePathOrString>>("icon"sv);
                Task::Options options = parameter.get_or<int, std::string_view, int>(
                    "options"sv, Task::AddTextMark | Task::FlashWorthy);

                QIcon qicon = icon ? toIcon(*icon)->icon() : QIcon();

                QTC_ASSERT(
                    type >= Task::Unknown && type <= Task::Warning,
                    throw sol::error("Type must be one of Task.Type.Unknown, Task.Type.Error, "
                                     "Task.Type.Warning"));

                return Task(
                    static_cast<Task::TaskType>(type),
                    description,
                    file,
                    line,
                    Id::fromString(category),
                    qicon,
                    options);
            },
            "id",
            sol::readonly_property(&Task::taskId),
            "type",
            sol::readonly_property(&Task::type),
            "options",
            sol::readonly_property(&Task::options),
            "summary",
            sol::readonly_property(&Task::summary),
            "details",
            sol::readonly_property(&Task::details),
            "file",
            sol::readonly_property(&Task::file),
            "fileCandidates",
            sol::readonly_property(&Task::fileCandidates),
            "line",
            sol::readonly_property(&Task::line),
            "movedLine",
            sol::readonly_property(&Task::movedLine),
            "column",
            sol::readonly_property(&Task::column),
            "category",
            sol::readonly_property([](Task &self) -> QString { return self.category.toString(); }),
            "description",
            sol::readonly_property([](Task &self) -> QString { return self.description(); }));

        return taskHub;
    });
}

} // namespace Lua::Internal
