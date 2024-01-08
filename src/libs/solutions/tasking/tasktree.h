// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tasking_global.h"

#include <QObject>
#include <QSharedPointer>

#include <memory>

QT_BEGIN_NAMESPACE
template <class T>
class QFuture;
QT_END_NAMESPACE

namespace Tasking {

Q_NAMESPACE_EXPORT(TASKING_EXPORT)

// WorkflowPolicy:
// 1. When all children finished with success -> report success, otherwise:
//    a) Report error on first error and stop executing other children (including their subtree).
//    b) On first error - continue executing all children and report error afterwards.
// 2. When all children finished with error -> report error, otherwise:
//    a) Report success on first success and stop executing other children (including their subtree).
//    b) On first success - continue executing all children and report success afterwards.
// 3. Stops on first finished child. In sequential mode it will never run other children then the first one.
//    Useful only in parallel mode.
// 4. Always run all children, let them finish, ignore their results and report success afterwards.
// 5. Always run all children, let them finish, ignore their results and report error afterwards.

enum class WorkflowPolicy
{
    StopOnError,          // 1a - Reports error on first child error, otherwise success (if all children were success).
    ContinueOnError,      // 1b - The same, but children execution continues. Reports success when no children.
    StopOnSuccess,        // 2a - Reports success on first child success, otherwise error (if all children were error).
    ContinueOnSuccess,    // 2b - The same, but children execution continues. Reports error when no children.
    StopOnSuccessOrError, // 3  - Stops on first finished child and report its result.
    FinishAllAndSuccess,  // 4  - Reports success after all children finished.
    FinishAllAndError     // 5  - Reports error after all children finished.
};
Q_ENUM_NS(WorkflowPolicy);

enum class SetupResult
{
    Continue,
    StopWithSuccess,
    StopWithError
};
Q_ENUM_NS(SetupResult);

enum class DoneResult
{
    Success,
    Error
};
Q_ENUM_NS(DoneResult);

enum class DoneWith
{
    Success,
    Error,
    Cancel
};
Q_ENUM_NS(DoneWith);

enum class CallDoneIf
{
    SuccessOrError,
    Success,
    Error
};
Q_ENUM_NS(CallDoneIf);

TASKING_EXPORT DoneResult toDoneResult(bool success);

class LoopData;
class StorageData;
class TaskTreePrivate;

class TASKING_EXPORT TaskInterface : public QObject
{
    Q_OBJECT

signals:
    void done(DoneResult result);

private:
    template <typename Task, typename Deleter> friend class TaskAdapter;
    friend class TaskTreePrivate;
    TaskInterface() = default;
#ifdef Q_QDOC
protected:
#endif
    virtual void start() = 0;
};

class TASKING_EXPORT Loop
{
public:
    using Condition = std::function<bool(int)>; // Takes iteration, called prior to each iteration.

    Loop(const Condition &condition);
    int iteration() const;

private:
    friend class ExecutionContextActivator;
    friend class TaskTreePrivate;
    QSharedPointer<LoopData> m_loopData;
};

class TASKING_EXPORT Forever final : public Loop
{
public:
    Forever() : Loop([](int) { return true; }) {}
};

class TASKING_EXPORT Repeat final : public Loop
{
public:
    Repeat(int count) : Loop([count](int index) { return index < count; }) {}
};

class TASKING_EXPORT StorageBase
{
private:
    using StorageConstructor = std::function<void *(void)>;
    using StorageDestructor = std::function<void(void *)>;
    using StorageHandler = std::function<void(void *)>;

    StorageBase(const StorageConstructor &ctor, const StorageDestructor &dtor);

    void *activeStorageVoid() const;

    friend bool operator==(const StorageBase &first, const StorageBase &second)
    { return first.m_storageData == second.m_storageData; }

    friend bool operator!=(const StorageBase &first, const StorageBase &second)
    { return first.m_storageData != second.m_storageData; }

    friend size_t qHash(const StorageBase &storage, uint seed = 0)
    { return size_t(storage.m_storageData.get()) ^ seed; }

    QSharedPointer<StorageData> m_storageData;

    template <typename StorageStruct> friend class Storage;
    friend class ExecutionContextActivator;
    friend class StorageData;
    friend class RuntimeContainer;
    friend class TaskTree;
    friend class TaskTreePrivate;
};

template <typename StorageStruct>
class Storage final : public StorageBase
{
public:
    Storage() : StorageBase(Storage::ctor(), Storage::dtor()) {}
    StorageStruct &operator*() const noexcept { return *activeStorage(); }
    StorageStruct *operator->() const noexcept { return activeStorage(); }
    StorageStruct *activeStorage() const {
        return static_cast<StorageStruct *>(activeStorageVoid());
    }

private:
    static StorageConstructor ctor() { return [] { return new StorageStruct; }; }
    static StorageDestructor dtor() {
        return [](void *storage) { delete static_cast<StorageStruct *>(storage); };
    }
};

class TASKING_EXPORT GroupItem
{
public:
    // Called when group entered, after group's storages are created
    using GroupSetupHandler = std::function<SetupResult()>;
    // Called when group done, before group's storages are deleted
    using GroupDoneHandler = std::function<DoneResult(DoneWith)>;

    template <typename StorageStruct>
    GroupItem(const Storage<StorageStruct> &storage)
        : m_type(Type::Storage)
        , m_storageList{storage} {}

    GroupItem(const Loop &loop) : GroupItem(GroupData{{}, {}, {}, loop}) {}

    // TODO: Add tests.
    GroupItem(const QList<GroupItem> &children) : m_type(Type::List) { addChildren(children); }
    GroupItem(std::initializer_list<GroupItem> children) : m_type(Type::List) { addChildren(children); }

protected:
    // Internal, provided by CustomTask
    using InterfaceCreateHandler = std::function<TaskInterface *(void)>;
    // Called prior to task start, just after createHandler
    using InterfaceSetupHandler = std::function<SetupResult(TaskInterface &)>;
    // Called on task done, just before deleteLater
    using InterfaceDoneHandler = std::function<DoneResult(const TaskInterface &, DoneWith)>;

    struct TaskHandler {
        InterfaceCreateHandler m_createHandler;
        InterfaceSetupHandler m_setupHandler = {};
        InterfaceDoneHandler m_doneHandler = {};
        CallDoneIf m_callDoneIf = CallDoneIf::SuccessOrError;
    };

    struct GroupHandler {
        GroupSetupHandler m_setupHandler;
        GroupDoneHandler m_doneHandler = {};
        CallDoneIf m_callDoneIf = CallDoneIf::SuccessOrError;
    };

    struct GroupData {
        GroupHandler m_groupHandler = {};
        std::optional<int> m_parallelLimit = {};
        std::optional<WorkflowPolicy> m_workflowPolicy = {};
        std::optional<Loop> m_loop = {};
    };

    enum class Type {
        List,
        Group,
        GroupData,
        Storage,
        TaskHandler
    };

    GroupItem() = default;
    GroupItem(Type type) : m_type(type) { }
    GroupItem(const GroupData &data)
        : m_type(Type::GroupData)
        , m_groupData(data) {}
    GroupItem(const TaskHandler &handler)
        : m_type(Type::TaskHandler)
        , m_taskHandler(handler) {}
    void addChildren(const QList<GroupItem> &children);

    static GroupItem groupHandler(const GroupHandler &handler) { return GroupItem({handler}); }
    static GroupItem parallelLimit(int limit) { return GroupItem({{}, limit}); }
    static GroupItem workflowPolicy(WorkflowPolicy policy) { return GroupItem({{}, {}, policy}); }
    static GroupItem withTimeout(const GroupItem &item, std::chrono::milliseconds timeout,
                                 const std::function<void()> &handler = {});

    // Checks if Function may be invoked with Args and if Function's return type is Result.
    template <typename Result, typename Function, typename ...Args,
              typename DecayedFunction = std::decay_t<Function>>
    static constexpr bool isInvocable()
    {
        // Note, that std::is_invocable_r_v doesn't check Result type properly.
        if constexpr (std::is_invocable_r_v<Result, DecayedFunction, Args...>)
            return std::is_same_v<Result, std::invoke_result_t<DecayedFunction, Args...>>;
        return false;
    }

private:
    friend class ContainerNode;
    friend class TaskNode;
    friend class TaskTreePrivate;
    Type m_type = Type::Group;
    QList<GroupItem> m_children;
    GroupData m_groupData;
    QList<StorageBase> m_storageList;
    TaskHandler m_taskHandler;
};

class TASKING_EXPORT Group final : public GroupItem
{
public:
    Group(const QList<GroupItem> &children) { addChildren(children); }
    Group(std::initializer_list<GroupItem> children) { addChildren(children); }

    // GroupData related:
    template <typename Handler>
    static GroupItem onGroupSetup(Handler &&handler) {
        return groupHandler({wrapGroupSetup(std::forward<Handler>(handler))});
    }
    template <typename Handler>
    static GroupItem onGroupDone(Handler &&handler, CallDoneIf callDoneIf = CallDoneIf::SuccessOrError) {
        return groupHandler({{}, wrapGroupDone(std::forward<Handler>(handler)), callDoneIf});
    }
    using GroupItem::parallelLimit;  // Default: 1 (sequential). 0 means unlimited (parallel).
    using GroupItem::workflowPolicy; // Default: WorkflowPolicy::StopOnError.

    GroupItem withTimeout(std::chrono::milliseconds timeout,
                          const std::function<void()> &handler = {}) const {
        return GroupItem::withTimeout(*this, timeout, handler);
    }

private:
    template <typename Handler>
    static GroupSetupHandler wrapGroupSetup(Handler &&handler)
    {
        // S, V stands for: [S]etupResult, [V]oid
        static constexpr bool isS = isInvocable<SetupResult, Handler>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isS || isV,
            "Group setup handler needs to take no arguments and has to return void or SetupResult. "
            "The passed handler doesn't fulfill these requirements.");
        return [handler] {
            if constexpr (isS)
                return std::invoke(handler);
            std::invoke(handler);
            return SetupResult::Continue;
        };
    };
    template <typename Handler>
    static GroupDoneHandler wrapGroupDone(Handler &&handler)
    {
        // D, V, W stands for: [D]oneResult, [V]oid, Done[W]ith
        static constexpr bool isDW = isInvocable<DoneResult, Handler, DoneWith>();
        static constexpr bool isD = isInvocable<DoneResult, Handler>();
        static constexpr bool isVW = isInvocable<void, Handler, DoneWith>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isDW || isD || isVW || isV,
            "Group done handler needs to take (DoneWith) or (void) as an argument and has to "
            "return void or DoneResult. The passed handler doesn't fulfill these requirements.");
        return [handler](DoneWith result) {
            if constexpr (isDW)
                return std::invoke(handler, result);
            if constexpr (isD)
                return std::invoke(handler);
            if constexpr (isVW)
                std::invoke(handler, result);
            else if constexpr (isV)
                std::invoke(handler);
            return result == DoneWith::Success ? DoneResult::Success : DoneResult::Error;
        };
    };
};

template <typename Handler>
static GroupItem onGroupSetup(Handler &&handler)
{
    return Group::onGroupSetup(std::forward<Handler>(handler));
}

template <typename Handler>
static GroupItem onGroupDone(Handler &&handler, CallDoneIf callDoneIf = CallDoneIf::SuccessOrError)
{
    return Group::onGroupDone(std::forward<Handler>(handler), callDoneIf);
}

TASKING_EXPORT GroupItem parallelLimit(int limit);
TASKING_EXPORT GroupItem workflowPolicy(WorkflowPolicy policy);

TASKING_EXPORT extern const GroupItem sequential;
TASKING_EXPORT extern const GroupItem parallel;

TASKING_EXPORT extern const GroupItem stopOnError;
TASKING_EXPORT extern const GroupItem continueOnError;
TASKING_EXPORT extern const GroupItem stopOnSuccess;
TASKING_EXPORT extern const GroupItem continueOnSuccess;
TASKING_EXPORT extern const GroupItem stopOnSuccessOrError;
TASKING_EXPORT extern const GroupItem finishAllAndSuccess;
TASKING_EXPORT extern const GroupItem finishAllAndError;

// Synchronous invocation. Similarly to Group - isn't counted as a task inside taskCount()
class TASKING_EXPORT Sync final : public GroupItem
{
public:
    template <typename Handler>
    Sync(Handler &&handler) {
        addChildren({ onGroupSetup(wrapHandler(std::forward<Handler>(handler))) });
    }

private:
    template <typename Handler>
    static GroupSetupHandler wrapHandler(Handler &&handler) {
        // D, V stands for: [D]oneResult, [V]oid
        static constexpr bool isB = isInvocable<DoneResult, Handler>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isB || isV,
            "Sync handler needs to take no arguments and has to return void or DoneResult. "
            "The passed handler doesn't fulfill these requirements.");
        return [handler] {
            if constexpr (isB) {
                return std::invoke(handler) == DoneResult::Success ? SetupResult::StopWithSuccess
                                                                   : SetupResult::StopWithError;
            }
            std::invoke(handler);
            return SetupResult::StopWithSuccess;
        };
    };
};

template <typename Task, typename Deleter = std::default_delete<Task>>
class TaskAdapter : public TaskInterface
{
protected:
    TaskAdapter() : m_task(new Task) {}
    Task *task() { return m_task.get(); }
    const Task *task() const { return m_task.get(); }

private:
    using TaskType = Task;
    using DeleterType = Deleter;
    template <typename Adapter> friend class CustomTask;
    std::unique_ptr<Task, Deleter> m_task;
};

template <typename Adapter>
class CustomTask final : public GroupItem
{
public:
    using Task = typename Adapter::TaskType;
    using Deleter = typename Adapter::DeleterType;
    static_assert(std::is_base_of_v<TaskAdapter<Task, Deleter>, Adapter>,
                  "The Adapter type for the CustomTask<Adapter> needs to be derived from "
                  "TaskAdapter<Task>.");
    using TaskSetupHandler = std::function<SetupResult(Task &)>;
    using TaskDoneHandler = std::function<DoneResult(const Task &, DoneWith)>;
    static Adapter *createAdapter() { return new Adapter; }

    template <typename SetupHandler = TaskSetupHandler, typename DoneHandler = TaskDoneHandler>
    CustomTask(SetupHandler &&setup = TaskSetupHandler(), DoneHandler &&done = TaskDoneHandler(),
               CallDoneIf callDoneIf = CallDoneIf::SuccessOrError)
        : GroupItem({&createAdapter, wrapSetup(std::forward<SetupHandler>(setup)),
                     wrapDone(std::forward<DoneHandler>(done)), callDoneIf})
    {}

    GroupItem withTimeout(std::chrono::milliseconds timeout,
                          const std::function<void()> &handler = {}) const
    {
        return GroupItem::withTimeout(*this, timeout, handler);
    }

private:
    template <typename Handler>
    static InterfaceSetupHandler wrapSetup(Handler &&handler) {
        if constexpr (std::is_same_v<Handler, TaskSetupHandler>)
            return {}; // When user passed {} for the setup handler.
        // S, V stands for: [S]etupResult, [V]oid
        static constexpr bool isS = isInvocable<SetupResult, Handler, Task &>();
        static constexpr bool isV = isInvocable<void, Handler, Task &>();
        static_assert(isS || isV,
            "Task setup handler needs to take (Task &) as an argument and has to return void or "
            "SetupResult. The passed handler doesn't fulfill these requirements.");
        return [handler](TaskInterface &taskInterface) {
            Adapter &adapter = static_cast<Adapter &>(taskInterface);
            if constexpr (isS)
                return std::invoke(handler, *adapter.task());
            std::invoke(handler, *adapter.task());
            return SetupResult::Continue;
        };
    };

    template <typename Handler>
    static InterfaceDoneHandler wrapDone(Handler &&handler) {
        if constexpr (std::is_same_v<Handler, TaskDoneHandler>)
            return {}; // When user passed {} for the done handler.
        // D, V, T, W stands for: [D]oneResult, [V]oid, [T]ask, done[W]ith
        static constexpr bool isDTW = isInvocable<DoneResult, Handler, const Task &, DoneWith>();
        static constexpr bool isDT = isInvocable<DoneResult, Handler, const Task &>();
        static constexpr bool isDW = isInvocable<DoneResult, Handler, DoneWith>();
        static constexpr bool isD = isInvocable<DoneResult, Handler>();
        static constexpr bool isVTW = isInvocable<void, Handler, const Task &, DoneWith>();
        static constexpr bool isVT = isInvocable<void, Handler, const Task &>();
        static constexpr bool isVW = isInvocable<void, Handler, DoneWith>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isDTW || isDT || isDW || isD || isVTW || isVT || isVW || isV,
            "Task done handler needs to take (const Task &, DoneWith), (const Task &), "
            "(DoneWith) or (void) as arguments and has to return void or DoneResult. "
            "The passed handler doesn't fulfill these requirements.");
        return [handler](const TaskInterface &taskInterface, DoneWith result) {
            const Adapter &adapter = static_cast<const Adapter &>(taskInterface);
            if constexpr (isDTW)
                return std::invoke(handler, *adapter.task(), result);
            if constexpr (isDT)
                return std::invoke(handler, *adapter.task());
            if constexpr (isDW)
                return std::invoke(handler, result);
            if constexpr (isD)
                return std::invoke(handler);
            if constexpr (isVTW)
                std::invoke(handler, *adapter.task(), result);
            else if constexpr (isVT)
                std::invoke(handler, *adapter.task());
            else if constexpr (isVW)
                std::invoke(handler, result);
            else if constexpr (isV)
                std::invoke(handler);
            return result == DoneWith::Success ? DoneResult::Success : DoneResult::Error;
        };
    };
};

class TASKING_EXPORT TaskTree final : public QObject
{
    Q_OBJECT

public:
    TaskTree();
    TaskTree(const Group &recipe);
    ~TaskTree();

    void setRecipe(const Group &recipe);

    void start();
    void stop();
    bool isRunning() const;

    // Helper methods. They execute a local event loop with ExcludeUserInputEvents.
    // The passed future is used for listening to the cancel event.
    // Don't use it in main thread. To be used in non-main threads or in auto tests.
    DoneWith runBlocking();
    DoneWith runBlocking(const QFuture<void> &future);
    static DoneWith runBlocking(const Group &recipe,
        std::chrono::milliseconds timeout = std::chrono::milliseconds::max());
    static DoneWith runBlocking(const Group &recipe, const QFuture<void> &future,
        std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    int taskCount() const;
    int progressMaximum() const { return taskCount(); }
    int progressValue() const; // all finished / skipped / stopped tasks, groups itself excluded

    template <typename StorageStruct, typename Handler>
    void onStorageSetup(const Storage<StorageStruct> &storage, Handler &&handler) {
        static_assert(std::is_invocable_v<std::decay_t<Handler>, StorageStruct &>,
                      "Storage setup handler needs to take (Storage &) as an argument. "
                      "The passed handler doesn't fulfill this requirement.");
        setupStorageHandler(storage,
                            wrapHandler<StorageStruct>(std::forward<Handler>(handler)), {});
    }
    template <typename StorageStruct, typename Handler>
    void onStorageDone(const Storage<StorageStruct> &storage, Handler &&handler) {
        static_assert(std::is_invocable_v<std::decay_t<Handler>, const StorageStruct &>,
                      "Storage done handler needs to take (const Storage &) as an argument. "
                      "The passed handler doesn't fulfill this requirement.");
        setupStorageHandler(storage, {},
                            wrapHandler<const StorageStruct>(std::forward<Handler>(handler)));
    }

signals:
    void started();
    void done(DoneWith result);
    void progressValueChanged(int value); // updated whenever task finished / skipped / stopped

private:
    void setupStorageHandler(const StorageBase &storage,
                             StorageBase::StorageHandler setupHandler,
                             StorageBase::StorageHandler doneHandler);
    template <typename StorageStruct, typename Handler>
    StorageBase::StorageHandler wrapHandler(Handler &&handler) {
        return [handler](void *voidStruct) {
            auto *storageStruct = static_cast<StorageStruct *>(voidStruct);
            std::invoke(handler, *storageStruct);
        };
    }

    TaskTreePrivate *d;
};

class TASKING_EXPORT TaskTreeTaskAdapter : public TaskAdapter<TaskTree>
{
public:
    TaskTreeTaskAdapter();

private:
    void start() final;
};

class TASKING_EXPORT TimeoutTaskAdapter : public TaskAdapter<std::chrono::milliseconds>
{
public:
    TimeoutTaskAdapter();
    ~TimeoutTaskAdapter();

private:
    void start() final;
    std::optional<int> m_timerId;
};

using TaskTreeTask = CustomTask<TaskTreeTaskAdapter>;
using TimeoutTask = CustomTask<TimeoutTaskAdapter>;

} // namespace Tasking
