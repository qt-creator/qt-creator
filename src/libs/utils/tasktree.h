// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QObject>
#include <QSet>
#include <QSharedPointer>

namespace Utils {

class TaskContainer;

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
    friend bool operator==(const TreeStorageBase &first, const TreeStorageBase &second)
    { return first.m_storageData == second.m_storageData; }
    friend bool operator!=(const TreeStorageBase &first, const TreeStorageBase &second)
    { return first.m_storageData != second.m_storageData; }
    size_t hash(uint seed) const;

protected:
    using StorageConstructor = std::function<void *(void)>;
    using StorageDestructor = std::function<void(void *)>;

    TreeStorageBase(StorageConstructor ctor, StorageDestructor dtor);
    void *activeStorageVoid() const;

private:
    int createStorage();
    void deleteStorage(int id);
    void activateStorage(int id);

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
};

inline size_t qHash(const TreeStorageBase &storage, uint seed = 0)
{
    return storage.hash(seed);
}

template <typename StorageStruct>
class TreeStorage : public TreeStorageBase
{
public:
    TreeStorage() : TreeStorageBase(TreeStorage::ctor(), TreeStorage::dtor()) {}
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

enum class ExecuteMode {
    Sequential, // default
    Parallel
};

// 4 policies:
// 1. When all children finished with done -> report done, otherwise:
//    a) Report error on first error and stop executing other children (including their subtree)
//    b) On first error - wait for all children to be finished and report error afterwards
// 2. When all children finished with error -> report error, otherwise:
//    a) Report done on first done and stop executing other children (including their subtree)
//    b) On first done - wait for all children to be finished and report done afterwards

enum class WorkflowPolicy {
    StopOnError,     // 1a - Will report error on any child error, otherwise done (if all children were done)
    ContinueOnError, // 1b - the same. When no children it reports done.
    StopOnDone,      // 2a - Will report done on any child done, otherwise error (if all children were error)
    ContinueOnDone,  // 2b - the same. When no children it reports done. (?)
    Optional         // Returns always done after all children finished
};

enum class GroupAction
{
    ContinueAll,
    ContinueSelected,
    StopWithDone,
    StopWithError
};

class GroupConfig
{
public:
    GroupAction action = GroupAction::ContinueAll;
    QSet<int> childrenToRun = {};
};

class QTCREATOR_UTILS_EXPORT TaskItem
{
public:
    // Internal, provided by QTC_DECLARE_CUSTOM_TASK
    using TaskCreateHandler = std::function<TaskInterface *(void)>;
    // Called prior to task start, just after createHandler
    using TaskSetupHandler = std::function<void(TaskInterface &)>;
    // Called on task done / error
    using TaskEndHandler = std::function<void(const TaskInterface &)>;
    // Called when group entered / after group ended with success or failure
    using GroupSimpleHandler = std::function<void()>;
    // Called when group entered
    using GroupSetupHandler = std::function<GroupConfig()>;
    // Called after group ended with success or failure, passed set of successful children
//    using GroupEndHandler = std::function<void(const QSet<int> &)>;

    struct TaskHandler {
        TaskCreateHandler m_createHandler;
        TaskSetupHandler m_setupHandler;
        TaskEndHandler m_doneHandler;
        TaskEndHandler m_errorHandler;
    };

    struct GroupHandler {
        GroupSimpleHandler m_setupHandler;
        GroupSimpleHandler m_doneHandler = {};
        GroupSimpleHandler m_errorHandler = {};
        GroupSetupHandler m_dynamicSetupHandler = {};
    };

    ExecuteMode executeMode() const { return m_executeMode; }
    WorkflowPolicy workflowPolicy() const { return m_workflowPolicy; }
    TaskHandler taskHandler() const { return m_taskHandler; }
    GroupHandler groupHandler() const { return m_groupHandler; }
    QList<TaskItem> children() const { return m_children; }
    QList<TreeStorageBase> storageList() const { return m_storageList; }

protected:
    enum class Type {
        Group,
        Storage,
        Mode,
        Policy,
        TaskHandler,
        GroupHandler
    };

    TaskItem() = default;
    TaskItem(ExecuteMode mode)
        : m_type(Type::Mode)
        , m_executeMode(mode) {}
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
    ExecuteMode m_executeMode = ExecuteMode::Sequential;
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

class QTCREATOR_UTILS_EXPORT Execute : public TaskItem
{
public:
    Execute(ExecuteMode mode) : TaskItem(mode) {}
};

class QTCREATOR_UTILS_EXPORT Workflow : public TaskItem
{
public:
    Workflow(WorkflowPolicy policy) : TaskItem(policy) {}
};

class QTCREATOR_UTILS_EXPORT OnGroupSetup : public TaskItem
{
public:
    OnGroupSetup(const GroupSimpleHandler &handler) : TaskItem({handler}) {}
};

class QTCREATOR_UTILS_EXPORT OnGroupDone : public TaskItem
{
public:
    OnGroupDone(const GroupSimpleHandler &handler) : TaskItem({{}, handler}) {}
};

class QTCREATOR_UTILS_EXPORT OnGroupError : public TaskItem
{
public:
    OnGroupError(const GroupSimpleHandler &handler) : TaskItem({{}, {}, handler}) {}
};

class QTCREATOR_UTILS_EXPORT DynamicSetup : public TaskItem
{
public:
    DynamicSetup(const GroupSetupHandler &handler) : TaskItem({{}, {}, {}, handler}) {}
};

QTCREATOR_UTILS_EXPORT extern Execute sequential;
QTCREATOR_UTILS_EXPORT extern Execute parallel;
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
    using SetupHandler = std::function<void(Task &)>;
    using EndHandler = std::function<void(const Task &)>;
    static Adapter *createAdapter() { return new Adapter; }
    CustomTask(const SetupHandler &setup, const EndHandler &done = {}, const EndHandler &error = {})
        : TaskItem({&createAdapter, wrapSetup(setup),
                    wrapEnd(done), wrapEnd(error)}) {}

private:
    static TaskSetupHandler wrapSetup(const SetupHandler &handler) {
        if (!handler)
            return {};
        return [handler](TaskInterface &taskInterface) {
            Adapter &adapter = static_cast<Adapter &>(taskInterface);
            handler(*adapter.task());
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

signals:
    void started();
    void done();
    void errorOccurred();
    void progressValueChanged(int value); // updated whenever task finished / skipped / stopped

private:
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
