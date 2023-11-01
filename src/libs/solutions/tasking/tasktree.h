// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tasking_global.h"

#include <QHash>
#include <QObject>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
template <class T>
class QFuture;
QT_END_NAMESPACE

namespace Tasking {

Q_NAMESPACE_EXPORT(TASKING_EXPORT)

class ExecutionContextActivator;
class TaskContainer;
class TaskTreePrivate;

class TASKING_EXPORT TaskInterface : public QObject
{
    Q_OBJECT

signals:
    void done(bool success);

private:
    template <typename Task> friend class TaskAdapter;
    friend class TaskNode;
    TaskInterface() = default;
#ifdef Q_QDOC
protected:
#endif
    virtual void start() = 0;
};

class TASKING_EXPORT TreeStorageBase
{
public:
    bool isValid() const;

private:
    using StorageConstructor = std::function<void *(void)>;
    using StorageDestructor = std::function<void(void *)>;

    TreeStorageBase(StorageConstructor ctor, StorageDestructor dtor);
    void *activeStorageVoid() const;

    int createStorage() const;
    void deleteStorage(int id) const;
    void activateStorage(int id) const;

    friend bool operator==(const TreeStorageBase &first, const TreeStorageBase &second)
    { return first.m_storageData == second.m_storageData; }

    friend bool operator!=(const TreeStorageBase &first, const TreeStorageBase &second)
    { return first.m_storageData != second.m_storageData; }

    friend size_t qHash(const TreeStorageBase &storage, uint seed = 0)
    { return size_t(storage.m_storageData.get()) ^ seed; }

    struct StorageData {
        ~StorageData();
        StorageConstructor m_constructor = {};
        StorageDestructor m_destructor = {};
        QHash<int, void *> m_storageHash = {};
        int m_activeStorage = 0; // 0 means no active storage
        int m_storageCounter = 0;
    };
    QSharedPointer<StorageData> m_storageData;

    template <typename StorageStruct> friend class TreeStorage;
    friend ExecutionContextActivator;
    friend TaskContainer;
    friend TaskTreePrivate;
};

template <typename StorageStruct>
class TreeStorage final : public TreeStorageBase
{
public:
    TreeStorage() : TreeStorageBase(TreeStorage::ctor(), TreeStorage::dtor()) {}
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

// WorkflowPolicy:
// 1. When all children finished with done -> report done, otherwise:
//    a) Report error on first error and stop executing other children (including their subtree).
//    b) On first error - continue executing all children and report error afterwards.
// 2. When all children finished with error -> report error, otherwise:
//    a) Report done on first done and stop executing other children (including their subtree).
//    b) On first done - continue executing all children and report done afterwards.
// 3. Stops on first finished child. In sequential mode it will never run other children then the first one.
//    Useful only in parallel mode.
// 4. Always run all children, let them finish, ignore their results and report done afterwards.
// 5. Always run all children, let them finish, ignore their results and report error afterwards.

enum class WorkflowPolicy {
    StopOnError,      // 1a - Reports error on first child error, otherwise done (if all children were done).
    ContinueOnError,  // 1b - The same, but children execution continues. Reports done when no children.
    StopOnDone,       // 2a - Reports done on first child done, otherwise error (if all children were error).
    ContinueOnDone,   // 2b - The same, but children execution continues. Reports error when no children.
    StopOnFinished,   // 3  - Stops on first finished child and report its result.
    FinishAllAndDone, // 4  - Reports done after all children finished.
    FinishAllAndError // 5  - Reports error after all children finished.
};
Q_ENUM_NS(WorkflowPolicy);

enum class SetupResult
{
    Continue,
    StopWithDone,
    StopWithError
};
Q_ENUM_NS(SetupResult);

class TASKING_EXPORT GroupItem
{
public:
    // Internal, provided by QTC_DECLARE_CUSTOM_TASK
    using TaskCreateHandler = std::function<TaskInterface *(void)>;
    // Called prior to task start, just after createHandler
    using TaskSetupHandler = std::function<SetupResult(TaskInterface &)>;
    // Called on task done / error
    using TaskEndHandler = std::function<void(const TaskInterface &)>;
    // Called when group entered
    using GroupSetupHandler = std::function<SetupResult()>;
    // Called when group done / error
    using GroupEndHandler = std::function<void()>;

    struct TaskHandler {
        TaskCreateHandler m_createHandler;
        TaskSetupHandler m_setupHandler = {};
        TaskEndHandler m_doneHandler = {};
        TaskEndHandler m_errorHandler = {};
    };

    struct GroupHandler {
        GroupSetupHandler m_setupHandler;
        GroupEndHandler m_doneHandler = {};
        GroupEndHandler m_errorHandler = {};
    };

    struct GroupData {
        GroupHandler m_groupHandler = {};
        std::optional<int> m_parallelLimit = {};
        std::optional<WorkflowPolicy> m_workflowPolicy = {};
    };

    QList<GroupItem> children() const { return m_children; }
    GroupData groupData() const { return m_groupData; }
    QList<TreeStorageBase> storageList() const { return m_storageList; }
    TaskHandler taskHandler() const { return m_taskHandler; }

protected:
    enum class Type {
        Group,
        GroupData,
        Storage,
        TaskHandler
    };

    GroupItem() = default;
    GroupItem(const GroupData &data)
        : m_type(Type::GroupData)
        , m_groupData(data) {}
    GroupItem(const TreeStorageBase &storage)
        : m_type(Type::Storage)
        , m_storageList{storage} {}
    GroupItem(const TaskHandler &handler)
        : m_type(Type::TaskHandler)
        , m_taskHandler(handler) {}
    void addChildren(const QList<GroupItem> &children);

    void setTaskSetupHandler(const TaskSetupHandler &handler);
    void setTaskDoneHandler(const TaskEndHandler &handler);
    void setTaskErrorHandler(const TaskEndHandler &handler);
    static GroupItem groupHandler(const GroupHandler &handler) { return GroupItem({handler}); }
    static GroupItem parallelLimit(int limit) { return GroupItem({{}, limit}); }
    static GroupItem workflowPolicy(WorkflowPolicy policy) { return GroupItem({{}, {}, policy}); }
    static GroupItem withTimeout(const GroupItem &item, std::chrono::milliseconds timeout,
                                 const GroupEndHandler &handler = {});

private:
    Type m_type = Type::Group;
    QList<GroupItem> m_children;
    GroupData m_groupData;
    QList<TreeStorageBase> m_storageList;
    TaskHandler m_taskHandler;
};

class TASKING_EXPORT Group final : public GroupItem
{
public:
    Group(const QList<GroupItem> &children) { addChildren(children); }
    Group(std::initializer_list<GroupItem> children) { addChildren(children); }

    // GroupData related:
    template <typename SetupHandler>
    static GroupItem onGroupSetup(SetupHandler &&handler) {
        return groupHandler({wrapGroupSetup(std::forward<SetupHandler>(handler))});
    }
    static GroupItem onGroupDone(const GroupEndHandler &handler) {
        return groupHandler({{}, handler});
    }
    static GroupItem onGroupError(const GroupEndHandler &handler) {
        return groupHandler({{}, {}, handler});
    }
    using GroupItem::parallelLimit;  // Default: 1 (sequential). 0 means unlimited (parallel).
    using GroupItem::workflowPolicy; // Default: WorkflowPolicy::StopOnError.

    GroupItem withTimeout(std::chrono::milliseconds timeout,
                          const GroupEndHandler &handler = {}) const {
        return GroupItem::withTimeout(*this, timeout, handler);
    }

private:
    template<typename SetupHandler>
    static GroupSetupHandler wrapGroupSetup(SetupHandler &&handler)
    {
        static constexpr bool isDynamic
            = std::is_same_v<SetupResult, std::invoke_result_t<std::decay_t<SetupHandler>>>;
        constexpr bool isVoid
            = std::is_same_v<void, std::invoke_result_t<std::decay_t<SetupHandler>>>;
        static_assert(isDynamic || isVoid,
                      "Group setup handler needs to take no arguments and has to return "
                      "void or SetupResult. The passed handler doesn't fulfill these requirements.");
        return [=] {
            if constexpr (isDynamic)
                return std::invoke(handler);
            std::invoke(handler);
            return SetupResult::Continue;
        };
    };
};

template <typename SetupHandler>
static GroupItem onGroupSetup(SetupHandler &&handler)
{
    return Group::onGroupSetup(std::forward<SetupHandler>(handler));
}

TASKING_EXPORT GroupItem onGroupDone(const GroupItem::GroupEndHandler &handler);
TASKING_EXPORT GroupItem onGroupError(const GroupItem::GroupEndHandler &handler);
TASKING_EXPORT GroupItem parallelLimit(int limit);
TASKING_EXPORT GroupItem workflowPolicy(WorkflowPolicy policy);

TASKING_EXPORT extern const GroupItem sequential;
TASKING_EXPORT extern const GroupItem parallel;

TASKING_EXPORT extern const GroupItem stopOnError;
TASKING_EXPORT extern const GroupItem continueOnError;
TASKING_EXPORT extern const GroupItem stopOnDone;
TASKING_EXPORT extern const GroupItem continueOnDone;
TASKING_EXPORT extern const GroupItem stopOnFinished;
TASKING_EXPORT extern const GroupItem finishAllAndDone;
TASKING_EXPORT extern const GroupItem finishAllAndError;

class TASKING_EXPORT Storage final : public GroupItem
{
public:
    Storage(const TreeStorageBase &storage) : GroupItem(storage) { }
};

// Synchronous invocation. Similarly to Group - isn't counted as a task inside taskCount()
class TASKING_EXPORT Sync final : public GroupItem
{
public:
    template<typename Function>
    Sync(Function &&function) { addChildren({init(std::forward<Function>(function))}); }

private:
    template<typename Function>
    static GroupItem init(Function &&function) {
        constexpr bool isInvocable = std::is_invocable_v<std::decay_t<Function>>;
        static_assert(isInvocable,
                      "Sync element: The synchronous function can't take any arguments.");
        constexpr bool isBool = std::is_same_v<bool, std::invoke_result_t<std::decay_t<Function>>>;
        constexpr bool isVoid = std::is_same_v<void, std::invoke_result_t<std::decay_t<Function>>>;
        static_assert(isBool || isVoid,
                      "Sync element: The synchronous function has to return void or bool.");
        if constexpr (isBool) {
            return onGroupSetup([function] { return function() ? SetupResult::StopWithDone
                                                               : SetupResult::StopWithError; });
        }
        return onGroupSetup([function] { function(); return SetupResult::StopWithDone; });
    };
};

template <typename Task>
class TaskAdapter : public TaskInterface
{
protected:
    using Type = Task;
    TaskAdapter() = default;
    Task *task() { return &m_task; }
    const Task *task() const { return &m_task; }

private:
    template <typename Adapter> friend class CustomTask;
    Task m_task;
};

template <typename Adapter>
class CustomTask final : public GroupItem
{
public:
    using Task = typename Adapter::Type;
    static_assert(std::is_base_of_v<TaskAdapter<Task>, Adapter>,
                  "The Adapter type for the CustomTask<Adapter> needs to be derived from "
                  "TaskAdapter<Task>.");
    using EndHandler = std::function<void(const Task &)>;
    static Adapter *createAdapter() { return new Adapter; }
    CustomTask() : GroupItem({&createAdapter}) {}
    template <typename SetupHandler>
    CustomTask(SetupHandler &&setup, const EndHandler &done = {}, const EndHandler &error = {})
        : GroupItem({&createAdapter, wrapSetup(std::forward<SetupHandler>(setup)),
                     wrapEnd(done), wrapEnd(error)}) {}

    template <typename SetupHandler>
    CustomTask &onSetup(SetupHandler &&handler) {
        setTaskSetupHandler(wrapSetup(std::forward<SetupHandler>(handler)));
        return *this;
    }
    CustomTask &onDone(const EndHandler &handler) {
        setTaskDoneHandler(wrapEnd(handler));
        return *this;
    }
    CustomTask &onError(const EndHandler &handler) {
        setTaskErrorHandler(wrapEnd(handler));
        return *this;
    }

    GroupItem withTimeout(std::chrono::milliseconds timeout,
                          const GroupEndHandler &handler = {}) const {
        return GroupItem::withTimeout(*this, timeout, handler);
    }

private:
    template<typename SetupHandler>
    static GroupItem::TaskSetupHandler wrapSetup(SetupHandler &&handler) {
        static constexpr bool isDynamic = std::is_same_v<SetupResult,
                std::invoke_result_t<std::decay_t<SetupHandler>, typename Adapter::Type &>>;
        constexpr bool isVoid = std::is_same_v<void,
                std::invoke_result_t<std::decay_t<SetupHandler>, typename Adapter::Type &>>;
        static_assert(isDynamic || isVoid,
                "Task setup handler needs to take (Task &) as an argument and has to return "
                "void or SetupResult. The passed handler doesn't fulfill these requirements.");
        return [=](TaskInterface &taskInterface) {
            Adapter &adapter = static_cast<Adapter &>(taskInterface);
            if constexpr (isDynamic)
                return std::invoke(handler, *adapter.task());
            std::invoke(handler, *adapter.task());
            return SetupResult::Continue;
        };
    };

    static TaskEndHandler wrapEnd(const EndHandler &handler) {
        if (!handler)
            return {};
        return [handler](const TaskInterface &taskInterface) {
            const Adapter &adapter = static_cast<const Adapter &>(taskInterface);
            handler(*adapter.task());
        };
    };
};

class TaskTreePrivate;

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
    bool runBlocking();
    bool runBlocking(const QFuture<void> &future);
    static bool runBlocking(const Group &recipe,
                            std::chrono::milliseconds timeout = std::chrono::milliseconds::max());
    static bool runBlocking(const Group &recipe, const QFuture<void> &future,
                            std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    int taskCount() const;
    int progressMaximum() const { return taskCount(); }
    int progressValue() const; // all finished / skipped / stopped tasks, groups itself excluded

    template <typename StorageStruct, typename StorageHandler>
    void onStorageSetup(const TreeStorage<StorageStruct> &storage, StorageHandler &&handler) {
        constexpr bool isInvokable = std::is_invocable_v<std::decay_t<StorageHandler>,
                                                         StorageStruct &>;
        static_assert(isInvokable,
                      "Storage setup handler needs to take (Storage &) as an argument. "
                      "The passed handler doesn't fulfill these requirements.");
        setupStorageHandler(storage,
                            wrapHandler<StorageStruct>(std::forward<StorageHandler>(handler)), {});
    }
    template <typename StorageStruct, typename StorageHandler>
    void onStorageDone(const TreeStorage<StorageStruct> &storage, StorageHandler &&handler) {
        constexpr bool isInvokable = std::is_invocable_v<std::decay_t<StorageHandler>,
                                                         const StorageStruct &>;
        static_assert(isInvokable,
                      "Storage done handler needs to take (const Storage &) as an argument. "
                      "The passed handler doesn't fulfill these requirements.");
        setupStorageHandler(storage,
                            {}, wrapHandler<StorageStruct>(std::forward<StorageHandler>(handler)));
    }

signals:
    void started();
    void done();
    void errorOccurred();
    void progressValueChanged(int value); // updated whenever task finished / skipped / stopped

private:
    using StorageVoidHandler = std::function<void(void *)>;
    void setupStorageHandler(const TreeStorageBase &storage,
                             StorageVoidHandler setupHandler,
                             StorageVoidHandler doneHandler);
    template <typename StorageStruct, typename StorageHandler>
    StorageVoidHandler wrapHandler(StorageHandler &&handler) {
        return [=](void *voidStruct) {
            StorageStruct *storageStruct = static_cast<StorageStruct *>(voidStruct);
            std::invoke(handler, *storageStruct);
        };
    }

    friend class TaskTreePrivate;
    TaskTreePrivate *d;
};

class TASKING_EXPORT TaskTreeTaskAdapter : public TaskAdapter<TaskTree>
{
public:
    TaskTreeTaskAdapter();
    void start() final;
};

class TASKING_EXPORT TimeoutTaskAdapter : public TaskAdapter<std::chrono::milliseconds>
{
public:
    TimeoutTaskAdapter();
    ~TimeoutTaskAdapter();
    void start() final;

private:
    std::optional<int> m_timerId;
};

using TaskTreeTask = CustomTask<TaskTreeTaskAdapter>;
using TimeoutTask = CustomTask<TimeoutTaskAdapter>;

} // namespace Tasking
