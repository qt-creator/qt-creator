// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QHash>
#include <QObject>
#include <QSharedPointer>

namespace Utils {

class StorageActivator;
class TaskContainer;
class TaskTreePrivate;

namespace Tasking {

class QTCREATOR_UTILS_EXPORT TaskInterface : public QObject
{
    Q_OBJECT

public:
    TaskInterface() = default;
    virtual void start() = 0;

signals:
    void done(bool success);
};

class QTCREATOR_UTILS_EXPORT TreeStorageBase
{
public:
    bool isValid() const;

protected:
    using StorageConstructor = std::function<void *(void)>;
    using StorageDestructor = std::function<void(void *)>;

    TreeStorageBase(StorageConstructor ctor, StorageDestructor dtor);
    void *activeStorageVoid() const;

private:
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
    friend TaskContainer;
    friend TaskTreePrivate;
    friend StorageActivator;
};

template <typename StorageStruct>
class TreeStorage : public TreeStorageBase
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
//    a) Report error on first error and stop executing other children (including their subtree)
//    b) On first error - continue executing all children and report error afterwards
// 2. When all children finished with error -> report error, otherwise:
//    a) Report done on first done and stop executing other children (including their subtree)
//    b) On first done - continue executing all children and report done afterwards
// 3. Always run all children, ignore their result and report done afterwards

enum class WorkflowPolicy {
    StopOnError,     // 1a - Reports error on first child error, otherwise done (if all children were done)
    ContinueOnError, // 1b - The same, but children execution continues. When no children it reports done.
    StopOnDone,      // 2a - Reports done on first child done, otherwise error (if all children were error)
    ContinueOnDone,  // 2b - The same, but children execution continues. When no children it reports done. (?)
    Optional         // 3 - Always reports done after all children finished
};

enum class TaskAction
{
    Continue,
    StopWithDone,
    StopWithError
};

class QTCREATOR_UTILS_EXPORT TaskItem
{
public:
    // Internal, provided by QTC_DECLARE_CUSTOM_TASK
    using TaskCreateHandler = std::function<TaskInterface *(void)>;
    // Called prior to task start, just after createHandler
    using TaskSetupHandler = std::function<TaskAction(TaskInterface &)>;
    // Called on task done / error
    using TaskEndHandler = std::function<void(const TaskInterface &)>;
    // Called when group entered
    using GroupSetupHandler = std::function<TaskAction()>;
    // Called when group done / error
    using GroupEndHandler = std::function<void()>;

    struct TaskHandler {
        TaskCreateHandler m_createHandler;
        TaskSetupHandler m_setupHandler;
        TaskEndHandler m_doneHandler;
        TaskEndHandler m_errorHandler;
    };

    struct GroupHandler {
        GroupSetupHandler m_setupHandler;
        GroupEndHandler m_doneHandler = {};
        GroupEndHandler m_errorHandler = {};
    };

    int parallelLimit() const { return m_parallelLimit; }
    WorkflowPolicy workflowPolicy() const { return m_workflowPolicy; }
    TaskHandler taskHandler() const { return m_taskHandler; }
    GroupHandler groupHandler() const { return m_groupHandler; }
    QList<TaskItem> children() const { return m_children; }
    QList<TreeStorageBase> storageList() const { return m_storageList; }

protected:
    enum class Type {
        Group,
        Storage,
        Limit,
        Policy,
        TaskHandler,
        GroupHandler
    };

    TaskItem() = default;
    TaskItem(int parallelLimit)
        : m_type(Type::Limit)
        , m_parallelLimit(parallelLimit) {}
    TaskItem(WorkflowPolicy policy)
        : m_type(Type::Policy)
        , m_workflowPolicy(policy) {}
    TaskItem(const TaskHandler &handler)
        : m_type(Type::TaskHandler)
        , m_taskHandler(handler) {}
    TaskItem(const GroupHandler &handler)
        : m_type(Type::GroupHandler)
        , m_groupHandler(handler) {}
    TaskItem(const TreeStorageBase &storage)
        : m_type(Type::Storage)
        , m_storageList{storage} {}
    void addChildren(const QList<TaskItem> &children);

private:
    Type m_type = Type::Group;
    int m_parallelLimit = 1; // 0 means unlimited
    WorkflowPolicy m_workflowPolicy = WorkflowPolicy::StopOnError;
    TaskHandler m_taskHandler;
    GroupHandler m_groupHandler;
    QList<TreeStorageBase> m_storageList;
    QList<TaskItem> m_children;
};

class QTCREATOR_UTILS_EXPORT Group : public TaskItem
{
public:
    Group(const QList<TaskItem> &children) { addChildren(children); }
    Group(std::initializer_list<TaskItem> children) { addChildren(children); }
};

class QTCREATOR_UTILS_EXPORT Storage : public TaskItem
{
public:
    Storage(const TreeStorageBase &storage) : TaskItem(storage) { }
};

class QTCREATOR_UTILS_EXPORT ParallelLimit : public TaskItem
{
public:
    ParallelLimit(int parallelLimit) : TaskItem(qMax(parallelLimit, 0)) {}
};

class QTCREATOR_UTILS_EXPORT Workflow : public TaskItem
{
public:
    Workflow(WorkflowPolicy policy) : TaskItem(policy) {}
};

class QTCREATOR_UTILS_EXPORT OnGroupSetup : public TaskItem
{
public:
    template <typename SetupFunction>
    OnGroupSetup(SetupFunction &&function)
        : TaskItem({wrapSetup(std::forward<SetupFunction>(function))}) {}

private:
    template<typename SetupFunction>
    static TaskItem::GroupSetupHandler wrapSetup(SetupFunction &&function) {
        static constexpr bool isDynamic = std::is_same_v<TaskAction,
                std::invoke_result_t<std::decay_t<SetupFunction>>>;
        constexpr bool isVoid = std::is_same_v<void,
                std::invoke_result_t<std::decay_t<SetupFunction>>>;
        static_assert(isDynamic || isVoid,
                "Group setup handler needs to take no arguments and has to return "
                "void or TaskAction. The passed handler doesn't fulfill these requirements.");
        return [=] {
            if constexpr (isDynamic)
                return std::invoke(function);
            std::invoke(function);
            return TaskAction::Continue;
        };
    };
};

class QTCREATOR_UTILS_EXPORT OnGroupDone : public TaskItem
{
public:
    OnGroupDone(const GroupEndHandler &handler) : TaskItem({{}, handler}) {}
};

class QTCREATOR_UTILS_EXPORT OnGroupError : public TaskItem
{
public:
    OnGroupError(const GroupEndHandler &handler) : TaskItem({{}, {}, handler}) {}
};

// Synchronous invocation. Similarly to Group - isn't counted as a task inside taskCount()
class QTCREATOR_UTILS_EXPORT Sync : public Group
{
public:
    using SynchronousMethod = std::function<bool()>;
    Sync(const SynchronousMethod &sync)
        : Group({OnGroupSetup([sync] { return sync() ? TaskAction::StopWithDone
                                                     : TaskAction::StopWithError; })}) {}
};

QTCREATOR_UTILS_EXPORT extern ParallelLimit sequential;
QTCREATOR_UTILS_EXPORT extern ParallelLimit parallel;
QTCREATOR_UTILS_EXPORT extern Workflow stopOnError;
QTCREATOR_UTILS_EXPORT extern Workflow continueOnError;
QTCREATOR_UTILS_EXPORT extern Workflow stopOnDone;
QTCREATOR_UTILS_EXPORT extern Workflow continueOnDone;
QTCREATOR_UTILS_EXPORT extern Workflow optional;

template <typename Task>
class TaskAdapter : public TaskInterface
{
public:
    using Type = Task;
    TaskAdapter() = default;
    Task *task() { return &m_task; }
    const Task *task() const { return &m_task; }
private:
    Task m_task;
};

template <typename Adapter>
class CustomTask : public TaskItem
{
public:
    using Task = typename Adapter::Type;
    using EndHandler = std::function<void(const Task &)>;
    static Adapter *createAdapter() { return new Adapter; }
    template <typename SetupFunction>
    CustomTask(SetupFunction &&function, const EndHandler &done = {}, const EndHandler &error = {})
        : TaskItem({&createAdapter, wrapSetup(std::forward<SetupFunction>(function)),
                    wrapEnd(done), wrapEnd(error)}) {}

private:
    template<typename SetupFunction>
    static TaskItem::TaskSetupHandler wrapSetup(SetupFunction &&function) {
        static constexpr bool isDynamic = std::is_same_v<TaskAction,
                std::invoke_result_t<std::decay_t<SetupFunction>, typename Adapter::Type &>>;
        constexpr bool isVoid = std::is_same_v<void,
                std::invoke_result_t<std::decay_t<SetupFunction>, typename Adapter::Type &>>;
        static_assert(isDynamic || isVoid,
                "Task setup handler needs to take (Task &) as an argument and has to return "
                "void or TaskAction. The passed handler doesn't fulfill these requirements.");
        return [=](TaskInterface &taskInterface) {
            Adapter &adapter = static_cast<Adapter &>(taskInterface);
            if constexpr (isDynamic)
                return std::invoke(function, *adapter.task());
            std::invoke(function, *adapter.task());
            return TaskAction::Continue;
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

} // namespace Tasking

class TaskTreePrivate;

class QTCREATOR_UTILS_EXPORT TaskTree : public QObject
{
    Q_OBJECT

public:
    TaskTree();
    TaskTree(const Tasking::Group &root);
    ~TaskTree();

    void setupRoot(const Tasking::Group &root);

    void start();
    void stop();
    bool isRunning() const;

    int taskCount() const;
    int progressMaximum() const { return taskCount(); }
    int progressValue() const; // all finished / skipped / stopped tasks, groups itself excluded

    template <typename StorageStruct, typename StorageHandler>
    void onStorageSetup(const Tasking::TreeStorage<StorageStruct> &storage,
                        StorageHandler &&handler) {
        setupStorageHandler(storage,
                            wrapHandler<StorageStruct>(std::forward<StorageHandler>(handler)), {});
    }
    template <typename StorageStruct, typename StorageHandler>
    void onStorageDone(const Tasking::TreeStorage<StorageStruct> &storage,
                       StorageHandler &&handler) {
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
    void setupStorageHandler(const Tasking::TreeStorageBase &storage,
                             StorageVoidHandler setupHandler,
                             StorageVoidHandler doneHandler);
    template <typename StorageStruct, typename StorageHandler>
    StorageVoidHandler wrapHandler(StorageHandler &&handler) {
        return [=](void *voidStruct) {
            StorageStruct *storageStruct = static_cast<StorageStruct *>(voidStruct);
            std::invoke(handler, storageStruct);
        };
    }

    friend class TaskTreePrivate;
    TaskTreePrivate *d;
};

class QTCREATOR_UTILS_EXPORT TaskTreeAdapter : public Tasking::TaskAdapter<TaskTree>
{
public:
    TaskTreeAdapter();
    void start() final;
};

} // namespace Utils

#define QTC_DECLARE_CUSTOM_TASK(CustomTaskName, TaskAdapterClass)\
namespace Utils::Tasking { using CustomTaskName = CustomTask<TaskAdapterClass>; }

#define QTC_DECLARE_CUSTOM_TEMPLATE_TASK(CustomTaskName, TaskAdapterClass)\
namespace Utils::Tasking {\
template <typename ...Args>\
using CustomTaskName = CustomTask<TaskAdapterClass<Args...>>;\
} // namespace Utils::Tasking

QTC_DECLARE_CUSTOM_TASK(Tree, Utils::TaskTreeAdapter);
