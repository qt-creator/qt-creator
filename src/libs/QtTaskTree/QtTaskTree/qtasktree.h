// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTASKTREE_H
#define QTASKTREE_H

#include <QtTaskTree/qttasktreeglobal.h>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSharedData>

#include <memory>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(future)
template <class T>
class QFuture;
#endif

class QTaskInterface;
class QTaskTree;

namespace QtTaskTree {

class Do;
class DoPrivate;
class For;
class ForPrivate;
class ForeverPrivate;
class Group;
class GroupItem;
class GroupItemPrivate;
using GroupItems = QList<GroupItem>;
class IteratorData;
class QTaskTreePrivate;
class StorageData;
class When;

Q_NAMESPACE_EXPORT(Q_TASKTREE_EXPORT)

enum class WorkflowPolicy
{
    StopOnError,
    ContinueOnError,
    StopOnSuccess,
    ContinueOnSuccess,
    StopOnSuccessOrError,
    FinishAllAndSuccess,
    FinishAllAndError
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

enum class CallDone
{
    Never = 0,
    OnSuccess = 1 << 0,
    OnError   = 1 << 1,
    OnCancel  = 1 << 2,
    Always = OnSuccess | OnError | OnCancel
};
Q_ENUM_NS(CallDone)
Q_DECLARE_FLAGS(CallDoneFlags, CallDone)
Q_DECLARE_OPERATORS_FOR_FLAGS(CallDoneFlags)

Q_TASKTREE_EXPORT DoneResult toDoneResult(bool success);
Q_TASKTREE_EXPORT bool shouldCallDone(CallDoneFlags callDone, DoneWith result);

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

// TODO: pimpl me?
class Q_TASKTREE_EXPORT Iterator
{
public:
    using Condition = std::function<bool(int)>; // Takes iteration, called prior to each iteration.
    using ValueGetter = std::function<const void *(int)>; // Takes iteration, returns ptr to ref.

    int iteration() const;

protected:
    Iterator(); // LoopForever
    Iterator(int count, const ValueGetter &valueGetter = {}); // LoopRepeat, LoopList
    Iterator(const Condition &condition); // LoopUntil

    const void *valuePtr() const;

private:
    friend class ExecutionContextActivator;
    friend class QTaskTreePrivate;
    std::shared_ptr<IteratorData> m_iteratorData;
};

class ForeverIterator final : public Iterator
{
public:
    Q_TASKTREE_EXPORT ForeverIterator();
};

class RepeatIterator final : public Iterator
{
public:
    Q_TASKTREE_EXPORT RepeatIterator(int count);
};

class UntilIterator final : public Iterator
{
public:
    Q_TASKTREE_EXPORT UntilIterator(const Condition &condition);
};

template <typename T>
class ListIterator final : public Iterator
{
public:
    ListIterator(const QList<T> &list) : Iterator(list.size(), [list](int i) { return &list.at(i); }) {}
    const T *operator->() const { return static_cast<const T *>(valuePtr()); }
    const T &operator*() const { return *static_cast<const T *>(valuePtr()); }
};

class Q_TASKTREE_EXPORT StorageBase
{
private:
    using StorageConstructor = std::function<void *(void)>;
    using StorageDestructor = std::function<void(void *)>;
    using StorageHandler = std::function<void(void *)>;

    StorageBase(const StorageConstructor &ctor, const StorageDestructor &dtor);

    void *activeStorageVoid() const;

    // TODO: de-inline?
    friend bool operator==(const StorageBase &first, const StorageBase &second)
    { return first.m_storageData == second.m_storageData; }

    friend bool operator!=(const StorageBase &first, const StorageBase &second)
    { return first.m_storageData != second.m_storageData; }

    friend size_t qHash(const StorageBase &storage, size_t seed = 0)
    { return size_t(storage.m_storageData.get()) ^ seed; }

    std::shared_ptr<StorageData> m_storageData;

    template <typename StorageStruct> friend class Storage;
    friend class ExecutionContextActivator;
    friend class StorageData;
    friend class RuntimeContainer;
    friend class QT_PREPEND_NAMESPACE(QTaskTree);
    friend class QTaskTreePrivate;
};

template <typename StorageStruct>
class Storage final : public StorageBase
{
public:
    Storage() : StorageBase(Storage::ctor(), Storage::dtor()) {}
#if __cplusplus >= 201803L // C++20: Allow pack expansion in lambda init-capture.
    template <typename ...Args>
    Storage(const Args &...args)
        : StorageBase([...args = args] { return new StorageStruct{args...}; }, Storage::dtor()) {}
#else // C++17
    template <typename ...Args>
    Storage(const Args &...args)
        : StorageBase([argsTuple = std::tuple(args...)] {
            return std::apply([](const Args &...arguments) { return new StorageStruct{arguments...}; }, argsTuple);
        }, Storage::dtor()) {}
#endif
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

class Q_TASKTREE_EXPORT GroupItem
{
public:
    // Called when group entered, after group's storages are created
    using GroupSetupHandler = std::function<SetupResult()>;
    // Called when group done, before group's storages are deleted
    using GroupDoneHandler = std::function<DoneResult(DoneWith)>;

    template <typename StorageStruct>
    GroupItem(const Storage<StorageStruct> &storage) : GroupItem(static_cast<StorageBase>(storage)) {}

    // TODO: Add tests.
    GroupItem(const GroupItems &children);
    GroupItem(std::initializer_list<GroupItem> children);
    ~GroupItem();

    GroupItem(const GroupItem &other);
    GroupItem(GroupItem &&other) noexcept;
    GroupItem &operator=(const GroupItem &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(GroupItem)

    void swap(GroupItem &other) noexcept { d.swap(other.d); }

protected:
    GroupItem(const Iterator &loop);

    using TaskAdapterPtr = void *;
    // Internal, provided by QCustomTask
    using TaskAdapterConstructor = std::function<TaskAdapterPtr(void)>;
    // Internal, provided by QCustomTask
    using TaskAdapterDestructor = std::function<void(TaskAdapterPtr)>;
    //
    using TaskAdapterStarter = std::function<void(TaskAdapterPtr, QTaskInterface *)>;
    // Called prior to task start, just after createHandler
    using TaskAdapterSetupHandler = std::function<SetupResult(TaskAdapterPtr)>;
    // Called on task done, just before deleteLater
    using TaskAdapterDoneHandler = std::function<DoneResult(TaskAdapterPtr, DoneWith)>;

    struct TaskHandler { // TODO: Make it a pimpled value type?
        TaskAdapterConstructor m_taskAdapterConstructor;
        TaskAdapterDestructor m_taskAdapterDestructor;
        TaskAdapterStarter m_taskAdapterStarter;
        TaskAdapterSetupHandler m_taskAdapterSetupHandler = {};
        TaskAdapterDoneHandler m_taskAdapterDoneHandler = {};
        CallDoneFlags m_callDoneFlags = CallDone::Always;
    };

    struct GroupHandler { // TODO: Make it a pimpled value type?
        GroupSetupHandler m_setupHandler;
        GroupDoneHandler m_doneHandler = {};
        CallDoneFlags m_callDoneFlags = CallDone::Always;
    };

    struct GroupData { // TODO: Make it a pimpled value type?
        GroupHandler m_groupHandler = {};
        std::optional<int> m_parallelLimit = std::nullopt;
        std::optional<WorkflowPolicy> m_workflowPolicy = std::nullopt;
        std::optional<Iterator> m_iterator = std::nullopt;
    };

    enum class Type {
        List,
        Group,
        GroupData,
        Storage,
        TaskHandler
    };

    GroupItem();
    GroupItem(Type type);
    GroupItem(const GroupData &data);
    GroupItem(const TaskHandler &handler);
    void addChildren(const GroupItems &children);

    static GroupItem groupHandler(const GroupHandler &handler);

    TaskHandler taskHandler() const;

private:
    GroupItem(const StorageBase &storage);

    Q_TASKTREE_EXPORT friend Group operator>>(const For &forItem, const Do &doItem);
    friend class ContainerNode;
    friend class GroupItemPrivate;
    friend class TaskInterfaceAdapter;
    friend class TaskNode;
    friend class QTaskTreePrivate;

    QExplicitlySharedDataPointer<GroupItemPrivate> d;
};

class Q_TASKTREE_EXPORT ExecutableItem : public GroupItem
{
public:
    Group withTimeout(std::chrono::milliseconds timeout,
                      const std::function<void()> &handler = {}) const;
    Group withLog(const QString &logName) const;
    template <typename SenderSignalPairGetter>
    Group withCancel(SenderSignalPairGetter &&getter,
                     std::initializer_list<GroupItem> postCancelRecipe = {}) const;
    template <typename SenderSignalPairGetter>
    Group withAccept(SenderSignalPairGetter &&getter) const;

protected:
    ExecutableItem();
    ExecutableItem(const TaskHandler &handler);

private:
    Q_TASKTREE_EXPORT friend Group operator!(const ExecutableItem &item);
    Q_TASKTREE_EXPORT friend Group operator&&(const ExecutableItem &first,
                                              const ExecutableItem &second);
    Q_TASKTREE_EXPORT friend Group operator||(const ExecutableItem &first,
                                              const ExecutableItem &second);
    Q_TASKTREE_EXPORT friend Group operator&&(const ExecutableItem &item, DoneResult result);
    Q_TASKTREE_EXPORT friend Group operator||(const ExecutableItem &item, DoneResult result);

    using ConnectWrapper = std::function<void(QObject *, const std::function<void()> &)>;

    Group withCancelImpl(const ConnectWrapper &connectWrapper,
                         const GroupItems &postCancelRecipe) const;
    Group withAcceptImpl(const ConnectWrapper &connectWrapper) const;

    template <typename SenderSignalPairGetter>
    static ConnectWrapper connectWrapper(SenderSignalPairGetter &&getter)
    {
        return [getter = std::forward<SenderSignalPairGetter>(getter)](
                   QObject *guard, const std::function<void()> &trigger) {
            const auto senderSignalPair = getter();
            QObject::connect(senderSignalPair.first, senderSignalPair.second, guard, [trigger] {
                trigger();
            }, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
        };
    }

    friend class When;
};

class Group final : public ExecutableItem
{
public:
    Q_TASKTREE_EXPORT Group(const GroupItems &children);
    Q_TASKTREE_EXPORT Group(std::initializer_list<GroupItem> children);

    // GroupData related:
    template <typename Handler>
    static GroupItem onGroupSetup(Handler &&handler) {
        return groupHandler({wrapGroupSetup(std::forward<Handler>(handler))});
    }
    template <typename Handler>
    static GroupItem onGroupDone(Handler &&handler, CallDoneFlags callDone = CallDone::Always) {
        return groupHandler({{}, wrapGroupDone(std::forward<Handler>(handler)), callDone});
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
        return [handler = std::forward<Handler>(handler)] {
            if constexpr (isR)
                return std::invoke(handler);
            std::invoke(handler);
            return SetupResult::Continue;
        };
    }
    template <typename Handler>
    static GroupDoneHandler wrapGroupDone(Handler &&handler)
    {
        static constexpr bool isDoneResultType = std::is_same_v<std::decay_t<Handler>, DoneResult>;
        // R, B, V, D stands for: Done[R]esult, [B]ool, [V]oid, [D]oneWith
        static constexpr bool isRD = isInvocable<DoneResult, Handler, DoneWith>();
        static constexpr bool isR = isInvocable<DoneResult, Handler>();
        static constexpr bool isBD = isInvocable<bool, Handler, DoneWith>();
        static constexpr bool isB = isInvocable<bool, Handler>();
        static constexpr bool isVD = isInvocable<void, Handler, DoneWith>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isDoneResultType || isRD || isR || isBD || isB || isVD || isV,
            "Group done handler should be a function taking (DoneWith) or (void) as an argument "
            "and returning void, bool or DoneResult. "
            "Alternatively, 'handler' may be an instance of DoneResult. "
            "The passed handler doesn't fulfill these requirements.");
        return [handler = std::forward<Handler>(handler)](DoneWith result) {
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

template <typename SenderSignalPairGetter>
Group ExecutableItem::withCancel(SenderSignalPairGetter &&getter,
                                 std::initializer_list<GroupItem> postCancelRecipe) const
{
    return withCancelImpl(connectWrapper(std::forward<SenderSignalPairGetter>(getter)),
                          postCancelRecipe);
}

template <typename SenderSignalPairGetter>
Group ExecutableItem::withAccept(SenderSignalPairGetter &&getter) const
{
    return withAcceptImpl(connectWrapper(std::forward<SenderSignalPairGetter>(getter)));
}

template <typename Handler>
static GroupItem onGroupSetup(Handler &&handler)
{
    return Group::onGroupSetup(std::forward<Handler>(handler));
}

template <typename Handler>
static GroupItem onGroupDone(Handler &&handler, CallDoneFlags callDone = CallDone::Always)
{
    return Group::onGroupDone(std::forward<Handler>(handler), callDone);
}

class ExecutionMode : public GroupItem
{
private:
    friend class ParallelLimit;
    Q_TASKTREE_EXPORT ExecutionMode(int limit);
};

class ParallelLimit : public ExecutionMode
{
public:
    Q_TASKTREE_EXPORT ParallelLimit(int limit);
};

// Default: WorkflowPolicy::StopOnError.
Q_TASKTREE_EXPORT GroupItem workflowPolicy(WorkflowPolicy policy);

Q_TASKTREE_EXPORT extern const ExecutionMode sequential;
Q_TASKTREE_EXPORT extern const ExecutionMode parallel;
Q_TASKTREE_EXPORT extern const ExecutionMode parallelIdealThreadCountLimit;

Q_TASKTREE_EXPORT extern const GroupItem stopOnError;
Q_TASKTREE_EXPORT extern const GroupItem continueOnError;
Q_TASKTREE_EXPORT extern const GroupItem stopOnSuccess;
Q_TASKTREE_EXPORT extern const GroupItem continueOnSuccess;
Q_TASKTREE_EXPORT extern const GroupItem stopOnSuccessOrError;
Q_TASKTREE_EXPORT extern const GroupItem finishAllAndSuccess;
Q_TASKTREE_EXPORT extern const GroupItem finishAllAndError;

Q_TASKTREE_EXPORT extern const GroupItem nullItem;
Q_TASKTREE_EXPORT extern const ExecutableItem successItem;
Q_TASKTREE_EXPORT extern const ExecutableItem errorItem;

class For final
{
public:
    Q_TASKTREE_EXPORT explicit For(const Iterator &loop);

    Q_TASKTREE_EXPORT ~For();
    Q_TASKTREE_EXPORT For(const For &other);
    Q_TASKTREE_EXPORT For(For &&other) noexcept;
    Q_TASKTREE_EXPORT For &operator=(const For &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(For)
    void swap(For &other) noexcept { d.swap(other.d); }

private:
    Q_TASKTREE_EXPORT friend Group operator>>(const For &forItem, const Do &doItem);

    Iterator iterator() const;

    QExplicitlySharedDataPointer<ForPrivate> d;
};

class Do final
{
public:
    Q_TASKTREE_EXPORT explicit Do(const GroupItems &children);
    Q_TASKTREE_EXPORT explicit Do(std::initializer_list<GroupItem> children);

    Q_TASKTREE_EXPORT ~Do();
    Q_TASKTREE_EXPORT Do(const Do &other);
    Q_TASKTREE_EXPORT Do(Do &&other) noexcept;
    Q_TASKTREE_EXPORT Do &operator=(const Do &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(Do)
    void swap(Do &other) noexcept { d.swap(other.d); }

private:
    Q_TASKTREE_EXPORT friend Group operator>>(const For &forItem, const Do &doItem);
    Q_TASKTREE_EXPORT friend Group operator>>(const When &whenItem, const Do &doItem);

    GroupItem children() const;

    QExplicitlySharedDataPointer<DoPrivate> d;
};

Q_TASKTREE_EXPORT Group operator>>(const For &forItem, const Do &doItem);
Q_TASKTREE_EXPORT Group operator>>(const When &whenItem, const Do &doItem);

class Forever final : public ExecutableItem
{
public:
    Q_TASKTREE_EXPORT explicit Forever(const GroupItems &children);
    Q_TASKTREE_EXPORT explicit Forever(std::initializer_list<GroupItem> children);
};

Q_TASKTREE_EXPORT ExecutableItem timeoutTask(const std::chrono::milliseconds &timeout,
                                             DoneResult result = DoneResult::Error);

} // namespace QtTaskTree

class QSyncTask final : public QtTaskTree::ExecutableItem
{
public:
    template <typename Handler>
    QSyncTask(Handler &&handler) {
        addChildren({ QtTaskTree::onGroupDone(wrapHandler(std::forward<Handler>(handler))) });
    }

private:
    template <typename Handler>
    static auto wrapHandler(Handler &&handler) {
        using QtTaskTree::isInvocable;
        // R, B, V stands for: Done[R]esult, [B]ool, [V]oid
        static constexpr bool isR = isInvocable<QtTaskTree::DoneResult, Handler>();
        static constexpr bool isB = isInvocable<bool, Handler>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isR || isB || isV,
            "QSyncTask handler needs to take no arguments and has to return void, bool or DoneResult. "
            "The passed handler doesn't fulfill these requirements.");
        return handler;
    }
};

class Q_TASKTREE_EXPORT QTaskInterface final : public QObject
{
    Q_OBJECT

public:
    QTaskInterface(QObject *parent = nullptr);
    void reportDone(QtTaskTree::DoneResult result);

Q_SIGNALS:
    void done(QtTaskTree::DoneResult result, QPrivateSignal);
};

// A convenient default helper, when:
// 1. Task is derived from QObject.
// 2. Task::start() method starts the task.
// 3. Task::done(DoneResult) or Task::done(bool) signal is emitted when the task is finished.
template <typename Task>
class QDefaultTaskAdapter
{
public:
    void operator()(Task *task, QTaskInterface *iface) {
//        if constexpr (is_done_compatible_v<WithDoneResult>) {
//            QObject::connect(task, &Task::done, iface, &QTaskInterface::reportDone,
//                             Qt::SingleShotConnection);
//        } else if constexpr (is_done_compatible_v<WithBool>) {
//            QObject::connect(task, &Task::done, iface, [iface](bool result) {
//                iface->reportDone(QtTaskTree::toDoneResult(result));
//            }, Qt::SingleShotConnection);
//        }
        QObject::connect(task, &Task::done, iface, &QTaskInterface::reportDone,
                         Qt::SingleShotConnection);
        task->start();
    }

private:
    static_assert(std::is_base_of_v<QObject, Task>,
                  "QDefaultTaskAdapter<Task>: The Task type needs to be derived from QObject.");

    template <typename, typename = void>
    struct has_start : std::false_type {};
    template <typename T>
    struct has_start<T, std::void_t<decltype(std::declval<T>().start())>> : std::true_type {};
    template <typename T>
    static inline constexpr bool has_start_v = has_start<T>::value;
    static_assert(has_start_v<Task>,
                  "QDefaultTaskAdapter<Task>: The Task type needs to specify public start() method.");

#define DoneError "QDefaultTaskAdapter<Task>: The Task type needs to specify " \
                  "public done(QtTaskTree::DoneResult) or done(bool) signal."

    template <typename, typename = void>
    struct has_done : std::false_type {};
    template <typename T>
    struct has_done<T, std::void_t<decltype(&T::done)>> : std::true_type {};
    template <typename T>
    static inline constexpr bool has_done_v = has_done<T>::value;
    static_assert(has_done_v<Task>, DoneError);

//    using WithDoneResult = std::function<void(QtTaskTree::DoneResult)>;
//    using WithBool = std::function<void(bool)>;
//    template <typename Arg, typename T = decltype(&Task::done)>
//    static inline constexpr bool is_done_compatible_v = QtPrivate::AreFunctionsCompatible<T, Arg>::value;
//    static_assert(is_done_compatible_v<WithDoneResult> || is_done_compatible_v<WithBool>, DoneError);

#undef DoneError
};

// TODO: Allow Task = void?
template <typename Task, typename Adapter = QDefaultTaskAdapter<Task>,
          typename Deleter = std::default_delete<Task>>
class QCustomTask final : public QtTaskTree::ExecutableItem
{
public:
    using TaskSetupHandler = std::function<QtTaskTree::SetupResult(Task &)>;
    using TaskDoneHandler = std::function<QtTaskTree::DoneResult(const Task &, QtTaskTree::DoneWith)>;

    template <typename SetupHandler = TaskSetupHandler, typename DoneHandler = TaskDoneHandler>
    QCustomTask(SetupHandler &&setup = TaskSetupHandler(), DoneHandler &&done = TaskDoneHandler(),
                QtTaskTree::CallDoneFlags callDone = QtTaskTree::CallDone::Always)
        : ExecutableItem({&taskAdapterConstructor, &taskAdapterDestructor, &taskAdapterStarter,
                          wrapSetup(std::forward<SetupHandler>(setup)),
                          wrapDone(std::forward<DoneHandler>(done)), callDone})
    {}

private:
    static_assert(std::is_default_constructible_v<Task>,
                  "QCustomTask<Type, Adapter, Deleter>: The Task type needs to "
                  "be default constructible.");
    static_assert(std::is_default_constructible_v<Adapter>,
                  "QCustomTask<Type, Adapter, Deleter>: The Adapter type needs to "
                  "be default constructible.");
    static_assert(std::is_invocable_v<Adapter, Task *, QTaskInterface *>,
                  "QCustomTask<Type, Adapter, Deleter>: The Adapter type needs to "
                  "implement public \"void operator()(Task *task, QTaskInterface *iface);\" "
                  "method.");

    friend class QtTaskTree::When;

    struct TaskAdapter {
        TaskAdapter() : task(new Task()) {}
        std::unique_ptr<Task, Deleter> task;
        Adapter adapter;
    };

    static TaskAdapter *taskAdapterConstructor() { return new TaskAdapter; }

    static void taskAdapterDestructor(TaskAdapterPtr voidAdapter) {
        delete static_cast<TaskAdapter *>(voidAdapter);
    }

    static void taskAdapterStarter(TaskAdapterPtr voidAdapter, QTaskInterface *iface) {
        TaskAdapter *taskAdapter = static_cast<TaskAdapter *>(voidAdapter);
        std::invoke(taskAdapter->adapter, taskAdapter->task.get(), iface);
    }

    template <typename Handler>
    static TaskAdapterSetupHandler wrapSetup(Handler &&handler) {
        using QtTaskTree::isInvocable;
        if constexpr (std::is_same_v<std::decay_t<Handler>, TaskSetupHandler>) {
            if (!handler)
                return {}; // User passed {} for the setup handler.
        }
        // R, V stands for: Setup[R]esult, [V]oid
        static constexpr bool isR = isInvocable<QtTaskTree::SetupResult, Handler, Task &>();
        static constexpr bool isV = isInvocable<void, Handler, Task &>();
        static_assert(isR || isV,
            "Task setup handler needs to take (Task &) as an argument and has to return void or "
            "SetupResult. The passed handler doesn't fulfill these requirements.");
        return [handler = std::forward<Handler>(handler)](TaskAdapterPtr voidAdapter) {
            Task *task = static_cast<TaskAdapter *>(voidAdapter)->task.get();
            if constexpr (isR)
                return std::invoke(handler, *task);
            std::invoke(handler, *task);
            return QtTaskTree::SetupResult::Continue;
        };
    }

    template <typename Handler>
    static TaskAdapterDoneHandler wrapDone(Handler &&handler) {
        using QtTaskTree::isInvocable;
        if constexpr (std::is_same_v<std::decay_t<Handler>, TaskDoneHandler>) {
            if (!handler)
                return {}; // User passed {} for the done handler.
        }
        static constexpr bool isDoneResultType = std::is_same_v<std::decay_t<Handler>, QtTaskTree::DoneResult>;
        // R, B, V, T, D stands for: Done[R]esult, [B]ool, [V]oid, [T]ask, [D]oneWith
        static constexpr bool isRTD = isInvocable<QtTaskTree::DoneResult, Handler, const Task &, QtTaskTree::DoneWith>();
        static constexpr bool isRT = isInvocable<QtTaskTree::DoneResult, Handler, const Task &>();
        static constexpr bool isRD = isInvocable<QtTaskTree::DoneResult, Handler, QtTaskTree::DoneWith>();
        static constexpr bool isR = isInvocable<QtTaskTree::DoneResult, Handler>();
        static constexpr bool isBTD = isInvocable<bool, Handler, const Task &, QtTaskTree::DoneWith>();
        static constexpr bool isBT = isInvocable<bool, Handler, const Task &>();
        static constexpr bool isBD = isInvocable<bool, Handler, QtTaskTree::DoneWith>();
        static constexpr bool isB = isInvocable<bool, Handler>();
        static constexpr bool isVTD = isInvocable<void, Handler, const Task &, QtTaskTree::DoneWith>();
        static constexpr bool isVT = isInvocable<void, Handler, const Task &>();
        static constexpr bool isVD = isInvocable<void, Handler, QtTaskTree::DoneWith>();
        static constexpr bool isV = isInvocable<void, Handler>();
        static_assert(isDoneResultType || isRTD || isRT || isRD || isR
                                       || isBTD || isBT || isBD || isB
                                       || isVTD || isVT || isVD || isV,
            "Task done handler should be a function taking (const Task &, DoneWith), "
            "(const Task &), (DoneWith) or (void) as arguments and returning void, bool or "
            "DoneResult. Alternatively, 'handler' may be an instance of DoneResult. "
            "The passed handler doesn't fulfill these requirements.");
        return [handler = std::forward<Handler>(handler)](TaskAdapterPtr voidAdapter,
                                                          QtTaskTree::DoneWith result) {
            if constexpr (isDoneResultType)
                return handler;
            [[maybe_unused]] Task *task = static_cast<TaskAdapter *>(voidAdapter)->task.get();
            if constexpr (isRTD)
                return std::invoke(handler, *task, result);
            if constexpr (isRT)
                return std::invoke(handler, *task);
            if constexpr (isRD)
                return std::invoke(handler, result);
            if constexpr (isR)
                return std::invoke(handler);
            if constexpr (isBTD)
                return QtTaskTree::toDoneResult(std::invoke(handler, *task, result));
            if constexpr (isBT)
                return QtTaskTree::toDoneResult(std::invoke(handler, *task));
            if constexpr (isBD)
                return QtTaskTree::toDoneResult(std::invoke(handler, result));
            if constexpr (isB)
                return QtTaskTree::toDoneResult(std::invoke(handler));
            if constexpr (isVTD)
                std::invoke(handler, *task, result);
            else if constexpr (isVT)
                std::invoke(handler, *task);
            else if constexpr (isVD)
                std::invoke(handler, result);
            else if constexpr (isV)
                std::invoke(handler);
            return QtTaskTree::toDoneResult(result == QtTaskTree::DoneWith::Success);
        };
    }
};

class Q_TASKTREE_EXPORT QTaskTree : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QtTaskTree::QTaskTree)

public:
    QTaskTree(QObject *parent = nullptr);
    QTaskTree(const QtTaskTree::Group &recipe, QObject *parent = nullptr);
    ~QTaskTree() override;

    void setRecipe(const QtTaskTree::Group &recipe);

    void start();
    void cancel();
    bool isRunning() const;

    // Helper methods. They execute a local event loop with ExcludeUserInputEvents.
    // The passed future is used for listening to the cancel event.
    // Don't use it in main thread. To be used in non-main threads or in auto tests.
    QtTaskTree::DoneWith runBlocking();
    static QtTaskTree::DoneWith runBlocking(const QtTaskTree::Group &recipe);
#if QT_CONFIG(future)
    QtTaskTree::DoneWith runBlocking(const QFuture<void> &future);
    static QtTaskTree::DoneWith runBlocking(const QtTaskTree::Group &recipe, const QFuture<void> &future);
#endif

    int asyncCount() const;
    int taskCount() const;
    int progressMaximum() const { return taskCount(); }
    int progressValue() const; // all finished / skipped / stopped tasks, groups itself excluded

    template <typename StorageStruct, typename Handler>
    void onStorageSetup(const QtTaskTree::Storage<StorageStruct> &storage, Handler &&handler) {
        static_assert(std::is_invocable_v<std::decay_t<Handler>, StorageStruct &>,
                      "Storage setup handler needs to take (Storage &) as an argument. "
                      "The passed handler doesn't fulfill this requirement.");
        setupStorageHandler(storage,
                            wrapHandler<StorageStruct>(std::forward<Handler>(handler)), {});
    }
    template <typename StorageStruct, typename Handler>
    void onStorageDone(const QtTaskTree::Storage<StorageStruct> &storage, Handler &&handler) {
        static_assert(std::is_invocable_v<std::decay_t<Handler>, const StorageStruct &>,
                      "Storage done handler needs to take (const Storage &) as an argument. "
                      "The passed handler doesn't fulfill this requirement.");
        setupStorageHandler(storage, {},
                            wrapHandler<const StorageStruct>(std::forward<Handler>(handler)));
    }

Q_SIGNALS:
    void started();
    void done(QtTaskTree::DoneWith result);
    void asyncCountChanged(int count);
    void progressValueChanged(int value); // updated whenever task finished / skipped / stopped

private:
    void setupStorageHandler(const QtTaskTree::StorageBase &storage,
                             const QtTaskTree::StorageBase::StorageHandler &setupHandler,
                             const QtTaskTree::StorageBase::StorageHandler &doneHandler);
    template <typename StorageStruct, typename Handler>
    QtTaskTree::StorageBase::StorageHandler wrapHandler(Handler &&handler) {
        return [handler](void *voidStruct) {
            auto *storageStruct = static_cast<StorageStruct *>(voidStruct);
            std::invoke(handler, *storageStruct);
        };
    }
};

class QTaskTreeTaskAdapter final
{
public:
    Q_TASKTREE_EXPORT void operator()(QTaskTree *task, QTaskInterface *iface);
};

class QTimeoutTaskAdapter final
{
public:
    Q_TASKTREE_EXPORT ~QTimeoutTaskAdapter();
    Q_TASKTREE_EXPORT void operator()(std::chrono::milliseconds *task, QTaskInterface *iface);

private:
    std::optional<size_t> m_timerId;
};

using QTaskTreeTask = QCustomTask<QTaskTree, QTaskTreeTaskAdapter>;
using QTimeoutTask = QCustomTask<std::chrono::milliseconds, QTimeoutTaskAdapter>;

QT_END_NAMESPACE

#endif // QTASKTREE_H
