// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QTASKTREE_QTASKTREE_H
#define QTASKTREE_QTASKTREE_H

#include <QtTaskTree/qttasktreeglobal.h>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSharedData>

#include <memory>
#include <QtCore/qxptype_traits.h>


QT_BEGIN_NAMESPACE

#if QT_CONFIG(future)
template <class T>
class QFuture;
#endif

namespace QtTaskTree {

class Do;
class DoPrivate;
class For;
class ForPrivate;
class Group;
class GroupItem;
class GroupItemPrivate;
using GroupItems = QList<GroupItem>;
class IteratorPrivate;
class QTaskInterface;
class QTaskTree;
class QTaskTreePrivate;
class StorageBasePrivate;
class When;

}

#define QT_TASKTREE_DECLARE_SMFS(Class, ExportMacro) \
    ExportMacro ~Class(); \
    ExportMacro Class(const Class &other); \
    ExportMacro Class &operator=(const Class &other); \
    Class(Class &&other) = default; \
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(Class) \
    void swap(Class &other) noexcept { d.swap(other.d); } \
    /* end */

#define QT_TASKTREE_DEFINE_SMF(Class) \
    Class::~Class() = default; \
    Class::Class(const Class &other) = default; \
    Class &Class::operator=(const Class &other) = default; \
    /* end */

QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::DoPrivate)
QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::ForPrivate)
QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::GroupItemPrivate)
QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::IteratorPrivate)
QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::StorageBasePrivate)

namespace QtTaskTree {

Q_NAMESPACE_EXPORT(Q_TASKTREE_EXPORT)

enum class WorkflowPolicy
{
    StopOnError,
    ContinueOnError,
    StopOnSuccess,
    ContinueOnSuccess,
    StopOnSuccessOrError,
    FinishAllAndSuccess,
    FinishAllAndError,
};
Q_ENUM_NS(WorkflowPolicy)

enum class SetupResult
{
    Continue,
    StopWithSuccess,
    StopWithError,
};
Q_ENUM_NS(SetupResult)

enum class DoneResult
{
    Success,
    Error,
};
Q_ENUM_NS(DoneResult)

enum class DoneWith
{
    Success,
    Error,
    Cancel
};
Q_ENUM_NS(DoneWith)

enum class CallDoneFlag
{
    Never = 0,
    OnSuccess = 1 << 0,
    OnError   = 1 << 1,
    OnCancel  = 1 << 2,
    Always = OnSuccess | OnError | OnCancel,
};
Q_DECLARE_FLAGS(CallDone, CallDoneFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(CallDone)
Q_FLAG_NS(CallDone)

Q_TASKTREE_EXPORT DoneResult toDoneResult(bool success);
Q_TASKTREE_EXPORT bool shouldCallDone(CallDone callDone, DoneWith result);

// Checks if Function may be invoked with Args and if Function's return type is Result.
template <typename Result, typename Function, typename ...Args,
          typename DecayedFunction = std::decay_t<Function>>
constexpr bool isInvocable()
{
    // Note, that std::is_invocable_r_v doesn't check Result type properly.
    if constexpr (std::is_invocable_r_v<Result, DecayedFunction, Args...>)
        return std::is_same_v<Result, std::invoke_result_t<DecayedFunction, Args...>>;
    return false;
}

class Iterator
{
public:
    using Condition = std::function<bool(qsizetype)>; // Takes iteration, called prior to each iteration.
    using ValueGetter = std::function<const void *(qsizetype)>; // Takes iteration, returns ptr to ref.

    QT_TASKTREE_DECLARE_SMFS(Iterator, Q_TASKTREE_EXPORT)

    Q_TASKTREE_EXPORT qsizetype iteration() const;

private:
    Q_TASKTREE_EXPORT Iterator(); // LoopForever
    Q_TASKTREE_EXPORT Iterator(qsizetype count, const ValueGetter &valueGetter = {}); // LoopRepeat, LoopList
    Q_TASKTREE_EXPORT Iterator(const Condition &condition); // LoopUntil

    Q_TASKTREE_EXPORT const void *valuePtr() const;

    friend class ExecutionContextActivator;
    friend class QTaskTreePrivate;
    friend class ForeverIterator;
    friend class RepeatIterator;
    friend class UntilIterator;
    template <typename T> friend class ListIterator;
    QExplicitlySharedDataPointer<IteratorPrivate> d;
};

class ForeverIterator final : public Iterator
{
public:
    Q_TASKTREE_EXPORT ForeverIterator();
};

class RepeatIterator final : public Iterator
{
public:
    Q_TASKTREE_EXPORT explicit RepeatIterator(qsizetype count);
};

class UntilIterator final : public Iterator
{
public:
    Q_TASKTREE_EXPORT explicit UntilIterator(const Condition &condition);
};

template <typename T>
class ListIterator final : public Iterator
{
public:
    explicit ListIterator(const QList<T> &list)
        : Iterator(list.size(), [list](qsizetype i) { return std::addressof(list.at(i)); }) {}
    const T *operator->() const { return static_cast<const T *>(valuePtr()); }
    const T &operator*() const { return *static_cast<const T *>(valuePtr()); }
};

class StorageBase
{
public:
    QT_TASKTREE_DECLARE_SMFS(StorageBase, Q_TASKTREE_EXPORT)

private:
    using StorageConstructor = std::function<void *(void)>;
    using StorageDestructor = std::function<void(void *)>;
    using StorageHandler = std::function<void(void *)>;

    Q_TASKTREE_EXPORT StorageBase(const StorageConstructor &ctor, const StorageDestructor &dtor);

    Q_TASKTREE_EXPORT void *activeStorageVoid() const;

    // TODO: de-inline?
    friend bool operator==(const StorageBase &first, const StorageBase &second)
    { return first.d == second.d; }
    friend bool operator!=(const StorageBase &first, const StorageBase &second)
    { return first.d != second.d; }

    friend size_t qHash(const StorageBase &storage, size_t seed = 0) noexcept
    { return size_t(storage.d.get()) ^ seed; }

    QExplicitlySharedDataPointer<StorageBasePrivate> d;

    template <typename StorageStruct> friend class Storage;
    friend class ExecutionContextActivator;
    friend class StorageBasePrivate;
    friend class RuntimeContainer;
    friend class QTaskTree;
    friend class QTaskTreePrivate;
};

template <typename StorageStruct>
class Storage final : public StorageBase
{
public:
    Storage() : StorageBase(Storage::ctor(), Storage::dtor()) {}
    template <typename FirstArg, typename ...Args,
             std::enable_if_t<!std::is_same_v<q20::remove_cvref_t<FirstArg>, Storage<StorageStruct>>, bool> = true>
    explicit Storage(const FirstArg &firstArg, const Args &...args)
        : StorageBase([=] { return new StorageStruct{firstArg, args...}; }, Storage::dtor()) {}

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

class GroupItem
{
public:
    // Called when group entered, after group's storages are created
    using GroupSetupHandler = std::function<SetupResult()>;
    // Called when group done, before group's storages are deleted
    using GroupDoneHandler = std::function<DoneResult(DoneWith)>;

    template <typename StorageStruct>
    GroupItem(const Storage<StorageStruct> &storage) : GroupItem(static_cast<StorageBase>(storage)) {}

    // TODO: Add tests.
    Q_TASKTREE_EXPORT GroupItem(const GroupItems &children);
    Q_TASKTREE_EXPORT GroupItem(std::initializer_list<GroupItem> children);

    QT_TASKTREE_DECLARE_SMFS(GroupItem, Q_TASKTREE_EXPORT)

protected:
    Q_TASKTREE_EXPORT GroupItem(const Iterator &loop);

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
        CallDone m_callDoneFlags = CallDoneFlag::Always;
    };

    struct GroupHandler { // TODO: Make it a pimpled value type?
        GroupSetupHandler m_setupHandler;
        GroupDoneHandler m_doneHandler = {};
        CallDone m_callDoneFlags = CallDoneFlag::Always;
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
        TaskHandler,
    };

    Q_TASKTREE_EXPORT GroupItem();
    Q_TASKTREE_EXPORT GroupItem(Type type);
    Q_TASKTREE_EXPORT GroupItem(const GroupData &data);
    Q_TASKTREE_EXPORT GroupItem(const TaskHandler &handler);
    Q_TASKTREE_EXPORT void addChildren(const GroupItems &children);

    Q_TASKTREE_EXPORT static GroupItem groupHandler(const GroupHandler &handler);

    Q_TASKTREE_EXPORT TaskHandler taskHandler() const;

private:
    Q_TASKTREE_EXPORT GroupItem(const StorageBase &storage);

    Q_TASKTREE_EXPORT friend Group operator>>(const For &forItem, const Do &doItem);
    friend class ContainerNode;
    friend class GroupItemPrivate;
    friend class TaskInterfaceAdapter;
    friend class TaskNode;
    friend class QTaskTreePrivate;

    QExplicitlySharedDataPointer<GroupItemPrivate> d;
};

template <typename Signal>
struct ObjectSignal
{
    typename QtPrivate::FunctionPointer<Signal>::Object *object;
    Signal signal;
};

template <typename Signal>
ObjectSignal<std::decay_t<Signal>> makeObjectSignal(
    typename QtPrivate::FunctionPointer<Signal>::Object *object, Signal &&signal)
{
    return ObjectSignal<std::decay_t<Signal>>{object, std::forward<Signal>(signal)};
}

class ExecutableItem : public GroupItem
{
public:
    Q_TASKTREE_EXPORT Group withTimeout(std::chrono::milliseconds timeout,
                                        const std::function<void()> &handler = {}) const;
    Q_TASKTREE_EXPORT Group withLog(const QString &logName) const;
    template <typename ObjectSignalGetter>
    Group withCancel(ObjectSignalGetter &&getter,
                     std::initializer_list<GroupItem> postCancelRecipe = {}) const;
    template <typename ObjectSignalGetter>
    Group withAccept(ObjectSignalGetter &&getter) const;

protected:
    Q_TASKTREE_EXPORT ExecutableItem();
    Q_TASKTREE_EXPORT ExecutableItem(const TaskHandler &handler);

private:
    Q_TASKTREE_EXPORT friend Group operator!(const ExecutableItem &item);
    Q_TASKTREE_EXPORT friend Group operator&&(const ExecutableItem &first,
                                              const ExecutableItem &second);
    Q_TASKTREE_EXPORT friend Group operator||(const ExecutableItem &first,
                                              const ExecutableItem &second);
    Q_TASKTREE_EXPORT friend Group operator&&(const ExecutableItem &item, DoneResult result);
    Q_TASKTREE_EXPORT friend Group operator||(const ExecutableItem &item, DoneResult result);

    using ConnectWrapper = std::function<void(QObject *, const std::function<void()> &)>;

    Q_TASKTREE_EXPORT Group withCancelImpl(const ConnectWrapper &connectWrapper,
                                           const GroupItems &postCancelRecipe) const;
    Q_TASKTREE_EXPORT Group withAcceptImpl(const ConnectWrapper &connectWrapper) const;

    template <typename ObjectSignalGetter>
    static ConnectWrapper connectWrapper(ObjectSignalGetter &&getter)
    {
        return [getter = std::forward<ObjectSignalGetter>(getter)](
                   QObject *guard, const std::function<void()> &trigger) {
            const auto objectSignal = getter();
            QObject::connect(objectSignal.object, objectSignal.signal, guard, trigger,
                             static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
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
    static GroupItem onGroupDone(Handler &&handler, CallDone callDone = CallDoneFlag::Always) {
        return groupHandler({{}, wrapGroupDone(std::forward<Handler>(handler)), callDone});
    }

private:
    template <typename Handler>
    static GroupSetupHandler wrapGroupSetup(Handler &&handler)
    {
        return [handler = std::forward<Handler>(handler)] {
            // R, V stands for: Setup[R]esult, [V]oid
            constexpr bool isR = isInvocable<SetupResult, Handler>();
            constexpr bool isV = isInvocable<void, Handler>();
            static_assert(isR || isV,
                          "Group setup handler needs to take no arguments and has to return void or SetupResult. "
                          "The passed handler doesn't fulfill these requirements.");
            if constexpr (isR)
                return std::invoke(handler);
            std::invoke(handler);
            return SetupResult::Continue;
        };
    }
    template <typename Handler>
    static GroupDoneHandler wrapGroupDone(Handler &&handler)
    {
        return [handler = std::forward<Handler>(handler)](DoneWith result) {
            constexpr bool isDoneResultType = std::is_same_v<std::decay_t<Handler>, DoneResult>;
            // R, B, V, D stands for: Done[R]esult, [B]ool, [V]oid, [D]oneWith
            constexpr bool isRD = isInvocable<DoneResult, Handler, DoneWith>();
            constexpr bool isR = isInvocable<DoneResult, Handler>();
            constexpr bool isBD = isInvocable<bool, Handler, DoneWith>();
            constexpr bool isB = isInvocable<bool, Handler>();
            constexpr bool isVD = isInvocable<void, Handler, DoneWith>();
            constexpr bool isV = isInvocable<void, Handler>();
            static_assert(isDoneResultType || isRD || isR || isBD || isB || isVD || isV,
                          "Group done handler should be a function taking (DoneWith) or (void) as an argument "
                          "and returning void, bool or DoneResult. "
                          "Alternatively, 'handler' may be an instance of DoneResult. "
                          "The passed handler doesn't fulfill these requirements.");
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

template <typename ObjectSignalGetter>
Group ExecutableItem::withCancel(ObjectSignalGetter &&getter,
                                 std::initializer_list<GroupItem> postCancelRecipe) const
{
    return withCancelImpl(connectWrapper(std::forward<ObjectSignalGetter>(getter)),
                          postCancelRecipe);
}

template <typename ObjectSignalGetter>
Group ExecutableItem::withAccept(ObjectSignalGetter &&getter) const
{
    return withAcceptImpl(connectWrapper(std::forward<ObjectSignalGetter>(getter)));
}

template <typename Handler>
GroupItem onGroupSetup(Handler &&handler)
{
    return Group::onGroupSetup(std::forward<Handler>(handler));
}

template <typename Handler>
GroupItem onGroupDone(Handler &&handler, CallDone callDone = CallDoneFlag::Always)
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

    QT_TASKTREE_DECLARE_SMFS(For, Q_TASKTREE_EXPORT)

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

    QT_TASKTREE_DECLARE_SMFS(Do, Q_TASKTREE_EXPORT)

private:
    Q_TASKTREE_EXPORT friend Group operator>>(const For &forItem, const Do &doItem);
    Q_TASKTREE_EXPORT friend Group operator>>(const When &whenItem, const Do &doItem);

    GroupItem children() const;

    QExplicitlySharedDataPointer<DoPrivate> d;
};

class Forever final : public ExecutableItem
{
public:
    Q_TASKTREE_EXPORT explicit Forever(const GroupItems &children);
    Q_TASKTREE_EXPORT explicit Forever(std::initializer_list<GroupItem> children);
};

Q_TASKTREE_EXPORT ExecutableItem timeoutTask(const std::chrono::milliseconds &timeout,
                                             DoneResult result = DoneResult::Error);

class QSyncTask final : public ExecutableItem
{
public:
    template <typename Handler,
              std::enable_if_t<!std::is_same_v<q20::remove_cvref_t<Handler>, QSyncTask>, bool> = true>
    explicit QSyncTask(Handler &&handler) {
        addChildren({ onGroupDone(wrapHandler(std::forward<Handler>(handler))) });
    }

private:
    template <typename Handler>
    static auto wrapHandler(Handler &&handler) {
        // R, B, V stands for: Done[R]esult, [B]ool, [V]oid
        constexpr bool isR = isInvocable<DoneResult, Handler>();
        constexpr bool isB = isInvocable<bool, Handler>();
        constexpr bool isV = isInvocable<void, Handler>();
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
    QTaskInterface() : QTaskInterface(nullptr) {}
    explicit QTaskInterface(QObject *parent);
    void reportDone(DoneResult result);

Q_SIGNALS:
    void done(QtTaskTree::DoneResult result, QPrivateSignal);

protected:
    bool event(QEvent *event) override;
};

// A convenient default helper, when:
// 1. Task is derived from QObject.
// 2. Task::start() method starts the task.
// 3. Task::done(DoneResult) or Task::done(bool) signal is emitted when the task is finished.
template <typename Task>
class QDefaultTaskAdapter
{
public:
    void operator()(Task *task, QTaskInterface *iface) const {
        if constexpr (is_done_compatible_v<WithDoneResult>) {
            QObject::connect(task, &Task::done, iface, &QTaskInterface::reportDone,
                             Qt::SingleShotConnection);
        } else if constexpr (is_done_compatible_v<WithBool>) {
            QObject::connect(task, &Task::done, iface, [iface](bool result) {
                iface->reportDone(toDoneResult(result));
            }, Qt::SingleShotConnection);
        }
        task->start();
    }

private:
    static_assert(std::is_base_of_v<QObject, Task>,
                  "QDefaultTaskAdapter<Task>: The Task type needs to be derived from QObject.");

    template <typename T>
    using HasStartTest = decltype(
        std::declval<T>().start()
    );
    static_assert(qxp::is_detected_v<HasStartTest, Task>,
                  "QDefaultTaskAdapter<Task>: The Task type needs to specify public start() method.");

#define DoneError "QDefaultTaskAdapter<Task>: The Task type needs to specify " \
                  "public done(DoneResult) or done(bool) signal."

    template <typename T>
    using HasDoneTest = decltype(
        &T::done
    );
    static_assert(qxp::is_detected_v<HasDoneTest, Task>, DoneError);

    using WithDoneResult = std::function<void(DoneResult)>;
    using WithBool = std::function<void(bool)>;
    template <typename Arg, typename T = decltype(&Task::done)>
    static inline constexpr bool is_done_compatible_v = QtPrivate::AreFunctionsCompatible<T, Arg>::value;
    static_assert(is_done_compatible_v<WithDoneResult> || is_done_compatible_v<WithBool>, DoneError);

#undef DoneError
};

// TODO: Allow Task = void?
template <typename Task, typename Adapter = QDefaultTaskAdapter<Task>,
          typename Deleter = std::default_delete<Task>>
class QCustomTask final : public ExecutableItem
{
public:
    using TaskSetupHandler = std::function<SetupResult(Task &)>;
    using TaskDoneHandler = std::function<DoneResult(const Task &, DoneWith)>;

    template <typename SetupHandler = TaskSetupHandler, typename DoneHandler = TaskDoneHandler,
              std::enable_if_t<!std::is_same_v<q20::remove_cvref_t<SetupHandler>, QCustomTask<Task, Adapter, Deleter>>, bool> = true>
    explicit QCustomTask(SetupHandler &&setup = TaskSetupHandler(),
                         DoneHandler &&done = TaskDoneHandler(),
                         CallDone callDone = CallDoneFlag::Always)
        : ExecutableItem(TaskHandler{&taskAdapterConstructor, &taskAdapterDestructor, &taskAdapterStarter,
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

    friend class When;

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
        if constexpr (std::is_same_v<std::decay_t<Handler>, TaskSetupHandler>) {
            if (!handler)
                return {}; // User passed {} for the setup handler.
        }
        return [handler = std::forward<Handler>(handler)](TaskAdapterPtr voidAdapter) {
            // R, V stands for: Setup[R]esult, [V]oid
            constexpr bool isR = isInvocable<SetupResult, Handler, Task &>();
            constexpr bool isV = isInvocable<void, Handler, Task &>();
            static_assert(isR || isV,
                          "Task setup handler needs to take (Task &) as an argument and has to return void or "
                          "SetupResult. The passed handler doesn't fulfill these requirements.");
            Task *task = static_cast<TaskAdapter *>(voidAdapter)->task.get();
            if constexpr (isR)
                return std::invoke(handler, *task);
            std::invoke(handler, *task);
            return SetupResult::Continue;
        };
    }

    template <typename Handler>
    static TaskAdapterDoneHandler wrapDone(Handler &&handler) {
        if constexpr (std::is_same_v<std::decay_t<Handler>, TaskDoneHandler>) {
            if (!handler)
                return {}; // User passed {} for the done handler.
        }
        return [handler = std::forward<Handler>(handler)](TaskAdapterPtr voidAdapter,
                                                          DoneWith result) {
            constexpr bool isDoneResultType = std::is_same_v<std::decay_t<Handler>, DoneResult>;
            // R, B, V, T, D stands for: Done[R]esult, [B]ool, [V]oid, [T]ask, [D]oneWith
            constexpr bool isRTD = isInvocable<DoneResult, Handler, const Task &, DoneWith>();
            constexpr bool isRT = isInvocable<DoneResult, Handler, const Task &>();
            constexpr bool isRD = isInvocable<DoneResult, Handler, DoneWith>();
            constexpr bool isR = isInvocable<DoneResult, Handler>();
            constexpr bool isBTD = isInvocable<bool, Handler, const Task &, DoneWith>();
            constexpr bool isBT = isInvocable<bool, Handler, const Task &>();
            constexpr bool isBD = isInvocable<bool, Handler, DoneWith>();
            constexpr bool isB = isInvocable<bool, Handler>();
            constexpr bool isVTD = isInvocable<void, Handler, const Task &, DoneWith>();
            constexpr bool isVT = isInvocable<void, Handler, const Task &>();
            constexpr bool isVD = isInvocable<void, Handler, DoneWith>();
            constexpr bool isV = isInvocable<void, Handler>();
            static_assert(isDoneResultType || isRTD || isRT || isRD || isR
                              || isBTD || isBT || isBD || isB
                              || isVTD || isVT || isVD || isV,
                          "Task done handler should be a function taking (const Task &, DoneWith), "
                          "(const Task &), (DoneWith) or (void) as arguments and returning void, bool or "
                          "DoneResult. Alternatively, 'handler' may be an instance of DoneResult. "
                          "The passed handler doesn't fulfill these requirements.");
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
                return toDoneResult(std::invoke(handler, *task, result));
            if constexpr (isBT)
                return toDoneResult(std::invoke(handler, *task));
            if constexpr (isBD)
                return toDoneResult(std::invoke(handler, result));
            if constexpr (isB)
                return toDoneResult(std::invoke(handler));
            if constexpr (isVTD)
                std::invoke(handler, *task, result);
            else if constexpr (isVT)
                std::invoke(handler, *task);
            else if constexpr (isVD)
                std::invoke(handler, result);
            else if constexpr (isV)
                std::invoke(handler);
            return toDoneResult(result == DoneWith::Success);
        };
    }
};

class Q_TASKTREE_EXPORT QTaskTree : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QTaskTree)

public:
    QTaskTree() : QTaskTree(nullptr) {}
    explicit QTaskTree(QObject *parent);
    explicit QTaskTree(const Group &recipe, QObject *parent = nullptr);
    ~QTaskTree() override;

    void setRecipe(const Group &recipe);

    void start();
    void cancel();
    bool isRunning() const;

    // Helper methods. They execute a local event loop with ExcludeUserInputEvents.
    // The passed future is used for listening to the cancel event.
    // Don't use it in main thread. To be used in non-main threads or in auto tests.
    DoneWith runBlocking();
    static DoneWith runBlocking(const Group &recipe);
#if QT_CONFIG(future)
    DoneWith runBlocking(const QFuture<void> &future);
    static DoneWith runBlocking(const Group &recipe, const QFuture<void> &future);
#endif

    qsizetype asyncCount() const;
    qsizetype taskCount() const;
    qsizetype progressMaximum() const { return taskCount(); }
    qsizetype progressValue() const; // all finished / skipped / stopped tasks, groups itself excluded

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
    void done(QtTaskTree::DoneWith result);
    void asyncCountChanged(qsizetype count);
    void progressValueChanged(qsizetype value); // updated whenever task finished / skipped / stopped

protected:
    bool event(QEvent *event) override;

private:
    void setupStorageHandler(const StorageBase &storage,
                             const StorageBase::StorageHandler &setupHandler,
                             const StorageBase::StorageHandler &doneHandler);
    template <typename StorageStruct, typename Handler>
    StorageBase::StorageHandler wrapHandler(Handler &&handler) {
        return [handler](void *voidStruct) {
            auto *storageStruct = static_cast<StorageStruct *>(voidStruct);
            std::invoke(handler, *storageStruct);
        };
    }
};

class QTaskTreeTaskAdapter
{
public:
    Q_TASKTREE_EXPORT void operator()(QTaskTree *task, QTaskInterface *iface) const;
};

class QTimeoutTaskAdapter final
{
public:
    QTimeoutTaskAdapter() = default;
    Q_TASKTREE_EXPORT ~QTimeoutTaskAdapter();
    Q_TASKTREE_EXPORT void operator()(std::chrono::milliseconds *task, QTaskInterface *iface);

private:
    Q_DISABLE_COPY_MOVE(QTimeoutTaskAdapter)
    std::optional<size_t> m_timerId = std::nullopt;
};

using QTaskTreeTask = QCustomTask<QTaskTree, QTaskTreeTaskAdapter>;
using QTimeoutTask = QCustomTask<std::chrono::milliseconds, QTimeoutTaskAdapter>;

} // namespace QtTaskTree

QT_END_NAMESPACE

#endif // QTASKTREE_QTASKTREE_H
