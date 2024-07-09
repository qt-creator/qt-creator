// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TASKING_TASKTREE_H
#define TASKING_TASKTREE_H

#include "tasking_global.h"

#include <QtCore/QList>
#include <QtCore/QObject>

#include <memory>

QT_BEGIN_NAMESPACE
template <class T>
class QFuture;

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
Q_ENUM_NS(WorkflowPolicy)

enum class SetupResult
{
    Continue,
    StopWithSuccess,
    StopWithError
};
Q_ENUM_NS(SetupResult)

enum class DoneResult
{
    Success,
    Error
};
Q_ENUM_NS(DoneResult)

enum class DoneWith
{
    Success,
    Error,
    Cancel
};
Q_ENUM_NS(DoneWith)

enum class CallDoneIf
{
    SuccessOrError,
    Success,
    Error
};
Q_ENUM_NS(CallDoneIf)

TASKING_EXPORT DoneResult toDoneResult(bool success);

class LoopData;
class StorageData;
class TaskTreePrivate;

class TASKING_EXPORT TaskInterface : public QObject
{
    Q_OBJECT

Q_SIGNALS:
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
    using ValueGetter = std::function<const void *(int)>; // Takes iteration, returns ptr to ref.

    int iteration() const;

protected:
    Loop(); // LoopForever
    Loop(int count, const ValueGetter &valueGetter = {}); // LoopRepeat, LoopList
    Loop(const Condition &condition); // LoopUntil

    const void *valuePtr() const;

private:
    friend class ExecutionContextActivator;
    friend class TaskTreePrivate;
    std::shared_ptr<LoopData> m_loopData;
};

class TASKING_EXPORT LoopForever final : public Loop
{
public:
    LoopForever() : Loop() {}
};

class TASKING_EXPORT LoopRepeat final : public Loop
{
public:
    LoopRepeat(int count) : Loop(count) {}
};

class TASKING_EXPORT LoopUntil final : public Loop
{
public:
    LoopUntil(const Condition &condition) : Loop(condition) {}
};

template <typename T>
class LoopList final : public Loop
{
public:
    LoopList(const QList<T> &list) : Loop(list.size(), [list](int i) { return &list.at(i); }) {}
    const T *operator->() const { return static_cast<const T *>(valuePtr()); }
    const T &operator*() const { return *static_cast<const T *>(valuePtr()); }
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

    std::shared_ptr<StorageData> m_storageData;

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
    static StorageConstructor ctor() { return [] { return new StorageStruct(); }; }
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

    // TODO: Add tests.
    GroupItem(const QList<GroupItem> &children) : m_type(Type::List) { addChildren(children); }
    GroupItem(std::initializer_list<GroupItem> children) : m_type(Type::List) { addChildren(children); }

protected:
    GroupItem(const Loop &loop) : GroupItem(GroupData{{}, {}, {}, loop}) {}
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
    friend class For;
    friend class TaskNode;
    friend class TaskTreePrivate;
    friend class ParallelLimitFunctor;
    friend class WorkflowPolicyFunctor;
    Type m_type = Type::Group;
    QList<GroupItem> m_children;
    GroupData m_groupData;
    QList<StorageBase> m_storageList;
    TaskHandler m_taskHandler;
};

class TASKING_EXPORT ExecutableItem : public GroupItem
{
public:
    ExecutableItem withTimeout(std::chrono::milliseconds timeout,
                               const std::function<void()> &handler = {}) const;
    ExecutableItem withLog(const QString &logName) const;
    template <typename SenderSignalPairGetter>
    ExecutableItem withCancel(SenderSignalPairGetter &&getter) const
    {
        const auto connectWrapper = [getter](QObject *guard, const std::function<void()> &trigger) {
            const auto senderSignalPair = getter();
            QObject::connect(senderSignalPair.first, senderSignalPair.second, guard, [trigger] {
                trigger();
            }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
        };
        return withCancelImpl(connectWrapper);
    }

protected:
    ExecutableItem() = default;
    ExecutableItem(const TaskHandler &handler) : GroupItem(handler) {}

private:
    TASKING_EXPORT friend ExecutableItem operator!(const ExecutableItem &item);
    TASKING_EXPORT friend ExecutableItem operator&&(const ExecutableItem &first,
                                                    const ExecutableItem &second);
    TASKING_EXPORT friend ExecutableItem operator||(const ExecutableItem &first,
                                                    const ExecutableItem &second);
    TASKING_EXPORT friend ExecutableItem operator&&(const ExecutableItem &item, DoneResult result);
    TASKING_EXPORT friend ExecutableItem operator||(const ExecutableItem &item, DoneResult result);

    ExecutableItem withCancelImpl(
        const std::function<void(QObject *, const std::function<void()> &)> &connectWrapper) const;
};

class TASKING_EXPORT Group : public ExecutableItem
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

private:
    template <typename Handler>
    static GroupSetupHandler wrapGroupSetup(Handler &&handler)
    {
        // R, V stands for: Setup[R]esult, [V]oid
        static constexpr bool isR = isInvocable<SetupResult, Handler>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isR || isV,
            "Group setup handler needs to take no arguments and has to return void or SetupResult. "
            "The passed handler doesn't fulfill these requirements.");
        return [handler] {
            if constexpr (isR)
                return std::invoke(handler);
            std::invoke(handler);
            return SetupResult::Continue;
        };
    }
    template <typename Handler>
    static GroupDoneHandler wrapGroupDone(Handler &&handler)
    {
        static constexpr bool isDoneResultType = std::is_same_v<Handler, DoneResult>;
        // R, B, V, D stands for: Done[R]esult, [B]ool, [V]oid, [D]oneWith
        static constexpr bool isRD = isInvocable<DoneResult, Handler, DoneWith>();
        static constexpr bool isR = isInvocable<DoneResult, Handler>();
        static constexpr bool isBD = isInvocable<bool, Handler, DoneWith>();
        static constexpr bool isB = isInvocable<bool, Handler>();
        static constexpr bool isVD = isInvocable<void, Handler, DoneWith>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isDoneResultType || isRD || isR || isBD || isB || isVD || isV,
            "Group done handler needs to take (DoneWith) or (void) as an argument and has to "
            "return void, bool or DoneResult. Alternatively, it may be of DoneResult type. "
            "The passed handler doesn't fulfill these requirements.");
        return [handler](DoneWith result) {
            if constexpr (isDoneResultType)
                return handler;
            if constexpr (isRD)
                return std::invoke(handler, result);
            if constexpr (isR)
                return std::invoke(handler);
            if constexpr (isBD)
                return toDoneResult(std::invoke(handler, result));
            if constexpr (isB)
                return toDoneResult(std::invoke(handler));
            if constexpr (isVD)
                std::invoke(handler, result);
            else if constexpr (isV)
                std::invoke(handler);
            return toDoneResult(result == DoneWith::Success);
        };
    }
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

class TASKING_EXPORT ParallelLimitFunctor
{
public:
    // Default: 1 (sequential). 0 means unlimited (parallel).
    GroupItem operator()(int limit) const;
};

class TASKING_EXPORT WorkflowPolicyFunctor
{
public:
    // Default: WorkflowPolicy::StopOnError.
    GroupItem operator()(WorkflowPolicy policy) const;
};

TASKING_EXPORT extern const ParallelLimitFunctor parallelLimit;
TASKING_EXPORT extern const WorkflowPolicyFunctor workflowPolicy;

TASKING_EXPORT extern const GroupItem sequential;
TASKING_EXPORT extern const GroupItem parallel;
TASKING_EXPORT extern const GroupItem parallelIdealThreadCountLimit;

TASKING_EXPORT extern const GroupItem stopOnError;
TASKING_EXPORT extern const GroupItem continueOnError;
TASKING_EXPORT extern const GroupItem stopOnSuccess;
TASKING_EXPORT extern const GroupItem continueOnSuccess;
TASKING_EXPORT extern const GroupItem stopOnSuccessOrError;
TASKING_EXPORT extern const GroupItem finishAllAndSuccess;
TASKING_EXPORT extern const GroupItem finishAllAndError;

TASKING_EXPORT extern const GroupItem nullItem;
TASKING_EXPORT extern const ExecutableItem successItem;
TASKING_EXPORT extern const ExecutableItem errorItem;

class TASKING_EXPORT For : public Group
{
public:
    template <typename ...Args>
    For(const Loop &loop, const Args &...args)
        : Group(withLoop(loop, args...)) { }

protected:
    For(const Loop &loop, const QList<GroupItem> &children) : Group({loop, children}) {}
    For(const Loop &loop, std::initializer_list<GroupItem> children) : Group({loop, children}) {}

private:
    template <typename ...Args>
    QList<GroupItem> withLoop(const Loop &loop, const Args &...args) {
        QList<GroupItem> children{GroupItem(loop)};
        appendChildren(std::make_tuple(args...), &children);
        return children;
    }

    template <typename Tuple, std::size_t N = 0>
    void appendChildren(const Tuple &tuple, QList<GroupItem> *children) {
        constexpr auto TupleSize = std::tuple_size_v<Tuple>;
        if constexpr (TupleSize > 0) {
            // static_assert(workflowPolicyCount<Tuple>() <= 1, "Too many workflow policies in one group.");
            children->append(std::get<N>(tuple));
            if constexpr (N + 1 < TupleSize)
                appendChildren<Tuple, N + 1>(tuple, children);
        }
    }
};

class TASKING_EXPORT Forever final : public For
{
public:
    Forever(const QList<GroupItem> &children) : For(LoopForever(), children) {}
    Forever(std::initializer_list<GroupItem> children) : For(LoopForever(), children) {}
};

// Synchronous invocation. Similarly to Group - isn't counted as a task inside taskCount()
class TASKING_EXPORT Sync final : public ExecutableItem
{
public:
    template <typename Handler>
    Sync(Handler &&handler) {
        addChildren({ onGroupDone(wrapHandler(std::forward<Handler>(handler))) });
    }

private:
    template <typename Handler>
    static auto wrapHandler(Handler &&handler) {
        // R, B, V stands for: Done[R]esult, [B]ool, [V]oid
        static constexpr bool isR = isInvocable<DoneResult, Handler>();
        static constexpr bool isB = isInvocable<bool, Handler>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isR || isB || isV,
            "Sync handler needs to take no arguments and has to return void, bool or DoneResult. "
            "The passed handler doesn't fulfill these requirements.");
        return handler;
    }
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
class CustomTask final : public ExecutableItem
{
public:
    using Task = typename Adapter::TaskType;
    using Deleter = typename Adapter::DeleterType;
    static_assert(std::is_base_of_v<TaskAdapter<Task, Deleter>, Adapter>,
                  "The Adapter type for the CustomTask<Adapter> needs to be derived from "
                  "TaskAdapter<Task>.");
    using TaskSetupHandler = std::function<SetupResult(Task &)>;
    using TaskDoneHandler = std::function<DoneResult(const Task &, DoneWith)>;

    template <typename SetupHandler = TaskSetupHandler, typename DoneHandler = TaskDoneHandler>
    CustomTask(SetupHandler &&setup = TaskSetupHandler(), DoneHandler &&done = TaskDoneHandler(),
               CallDoneIf callDoneIf = CallDoneIf::SuccessOrError)
        : ExecutableItem({&createAdapter, wrapSetup(std::forward<SetupHandler>(setup)),
                          wrapDone(std::forward<DoneHandler>(done)), callDoneIf})
    {}

private:
    static Adapter *createAdapter() { return new Adapter; }

    template <typename Handler>
    static InterfaceSetupHandler wrapSetup(Handler &&handler) {
        if constexpr (std::is_same_v<Handler, TaskSetupHandler>)
            return {}; // When user passed {} for the setup handler.
        // R, V stands for: Setup[R]esult, [V]oid
        static constexpr bool isR = isInvocable<SetupResult, Handler, Task &>();
        static constexpr bool isV = isInvocable<void, Handler, Task &>();
        static_assert(isR || isV,
            "Task setup handler needs to take (Task &) as an argument and has to return void or "
            "SetupResult. The passed handler doesn't fulfill these requirements.");
        return [handler](TaskInterface &taskInterface) {
            Adapter &adapter = static_cast<Adapter &>(taskInterface);
            if constexpr (isR)
                return std::invoke(handler, *adapter.task());
            std::invoke(handler, *adapter.task());
            return SetupResult::Continue;
        };
    }

    template <typename Handler>
    static InterfaceDoneHandler wrapDone(Handler &&handler) {
        if constexpr (std::is_same_v<Handler, TaskDoneHandler>)
            return {}; // User passed {} for the done handler.
        static constexpr bool isDoneResultType = std::is_same_v<Handler, DoneResult>;
        // R, B, V, T, D stands for: Done[R]esult, [B]ool, [V]oid, [T]ask, [D]oneWith
        static constexpr bool isRTD = isInvocable<DoneResult, Handler, const Task &, DoneWith>();
        static constexpr bool isRT = isInvocable<DoneResult, Handler, const Task &>();
        static constexpr bool isRD = isInvocable<DoneResult, Handler, DoneWith>();
        static constexpr bool isR = isInvocable<DoneResult, Handler>();
        static constexpr bool isBTD = isInvocable<bool, Handler, const Task &, DoneWith>();
        static constexpr bool isBT = isInvocable<bool, Handler, const Task &>();
        static constexpr bool isBD = isInvocable<bool, Handler, DoneWith>();
        static constexpr bool isB = isInvocable<bool, Handler>();
        static constexpr bool isVTD = isInvocable<void, Handler, const Task &, DoneWith>();
        static constexpr bool isVT = isInvocable<void, Handler, const Task &>();
        static constexpr bool isVD = isInvocable<void, Handler, DoneWith>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isDoneResultType || isRTD || isRT || isRD || isR
                                       || isBTD || isBT || isBD || isB
                                       || isVTD || isVT || isVD || isV,
            "Task done handler needs to take (const Task &, DoneWith), (const Task &), "
            "(DoneWith) or (void) as arguments and has to return void, bool or DoneResult. "
            "Alternatively, it may be of DoneResult type. "
            "The passed handler doesn't fulfill these requirements.");
        return [handler](const TaskInterface &taskInterface, DoneWith result) {
            if constexpr (isDoneResultType)
                return handler;
            const Adapter &adapter = static_cast<const Adapter &>(taskInterface);
            if constexpr (isRTD)
                return std::invoke(handler, *adapter.task(), result);
            if constexpr (isRT)
                return std::invoke(handler, *adapter.task());
            if constexpr (isRD)
                return std::invoke(handler, result);
            if constexpr (isR)
                return std::invoke(handler);
            if constexpr (isBTD)
                return toDoneResult(std::invoke(handler, *adapter.task(), result));
            if constexpr (isBT)
                return toDoneResult(std::invoke(handler, *adapter.task()));
            if constexpr (isBD)
                return toDoneResult(std::invoke(handler, result));
            if constexpr (isB)
                return toDoneResult(std::invoke(handler));
            if constexpr (isVTD)
                std::invoke(handler, *adapter.task(), result);
            else if constexpr (isVT)
                std::invoke(handler, *adapter.task());
            else if constexpr (isVD)
                std::invoke(handler, result);
            else if constexpr (isV)
                std::invoke(handler);
            return toDoneResult(result == DoneWith::Success);
        };
    }
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
    void cancel();
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

    int asyncCount() const;
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

Q_SIGNALS:
    void started();
    void done(DoneWith result);
    void asyncCountChanged(int count);
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

QT_END_NAMESPACE

#endif // TASKING_TASKTREE_H
