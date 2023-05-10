// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tasktree.h"

#include <QSet>

namespace Tasking {

// That's cut down qtcassert.{c,h} to avoid the dependency.
#define QTC_STRINGIFY_HELPER(x) #x
#define QTC_STRINGIFY(x) QTC_STRINGIFY_HELPER(x)
#define QTC_STRING(cond) qDebug("SOFT ASSERT: \"%s\" in %s: %s", cond,  __FILE__, QTC_STRINGIFY(__LINE__))
#define QTC_ASSERT(cond, action) if (Q_LIKELY(cond)) {} else { QTC_STRING(#cond); action; } do {} while (0)
#define QTC_CHECK(cond) if (cond) {} else { QTC_STRING(#cond); } do {} while (0)

class Guard
{
    Q_DISABLE_COPY(Guard)
public:
    Guard() = default;
    ~Guard() { QTC_CHECK(m_lockCount == 0); }
    bool isLocked() const { return m_lockCount; }
private:
    int m_lockCount = 0;
    friend class GuardLocker;
};

class GuardLocker
{
    Q_DISABLE_COPY(GuardLocker)
public:
    GuardLocker(Guard &guard) : m_guard(guard) { ++m_guard.m_lockCount; }
    ~GuardLocker() { --m_guard.m_lockCount; }
private:
    Guard &m_guard;
};

static TaskAction toTaskAction(bool success)
{
    return success ? TaskAction::StopWithDone : TaskAction::StopWithError;
}

bool TreeStorageBase::isValid() const
{
    return m_storageData && m_storageData->m_constructor && m_storageData->m_destructor;
}

TreeStorageBase::TreeStorageBase(StorageConstructor ctor, StorageDestructor dtor)
    : m_storageData(new StorageData{ctor, dtor}) { }

TreeStorageBase::StorageData::~StorageData()
{
    QTC_CHECK(m_storageHash.isEmpty());
    for (void *ptr : std::as_const(m_storageHash))
        m_destructor(ptr);
}

void *TreeStorageBase::activeStorageVoid() const
{
    QTC_ASSERT(m_storageData->m_activeStorage, qWarning(
        "The referenced storage is not reachable in the running tree. "
        "A nullptr will be returned which might lead to a crash in the calling code. "
        "It is possible that no storage was added to the tree, "
        "or the storage is not reachable from where it is referenced.");
        return nullptr);
    const auto it = m_storageData->m_storageHash.constFind(m_storageData->m_activeStorage);
    QTC_ASSERT(it != m_storageData->m_storageHash.constEnd(), return nullptr);
    return it.value();
}

int TreeStorageBase::createStorage() const
{
    QTC_ASSERT(m_storageData->m_constructor, return 0); // TODO: add isValid()?
    QTC_ASSERT(m_storageData->m_destructor, return 0);
    QTC_ASSERT(m_storageData->m_activeStorage == 0, return 0); // TODO: should be allowed?
    const int newId = ++m_storageData->m_storageCounter;
    m_storageData->m_storageHash.insert(newId, m_storageData->m_constructor());
    return newId;
}

void TreeStorageBase::deleteStorage(int id) const
{
    QTC_ASSERT(m_storageData->m_constructor, return); // TODO: add isValid()?
    QTC_ASSERT(m_storageData->m_destructor, return);
    QTC_ASSERT(m_storageData->m_activeStorage == 0, return); // TODO: should be allowed?
    const auto it = m_storageData->m_storageHash.constFind(id);
    QTC_ASSERT(it != m_storageData->m_storageHash.constEnd(), return);
    m_storageData->m_destructor(it.value());
    m_storageData->m_storageHash.erase(it);
}

// passing 0 deactivates currently active storage
void TreeStorageBase::activateStorage(int id) const
{
    if (id == 0) {
        QTC_ASSERT(m_storageData->m_activeStorage, return);
        m_storageData->m_activeStorage = 0;
        return;
    }
    QTC_ASSERT(m_storageData->m_activeStorage == 0, return);
    const auto it = m_storageData->m_storageHash.find(id);
    QTC_ASSERT(it != m_storageData->m_storageHash.end(), return);
    m_storageData->m_activeStorage = id;
}

ParallelLimit sequential(1);
ParallelLimit parallel(0);
Workflow stopOnError(WorkflowPolicy::StopOnError);
Workflow continueOnError(WorkflowPolicy::ContinueOnError);
Workflow stopOnDone(WorkflowPolicy::StopOnDone);
Workflow continueOnDone(WorkflowPolicy::ContinueOnDone);
Workflow optional(WorkflowPolicy::Optional);

void TaskItem::addChildren(const QList<TaskItem> &children)
{
    QTC_ASSERT(m_type == Type::Group, qWarning("Only Group may have children, skipping...");
               return);
    for (const TaskItem &child : children) {
        switch (child.m_type) {
        case Type::Group:
            m_children.append(child);
            break;
        case Type::Limit:
            QTC_ASSERT(m_type == Type::Group, qWarning("Execution Mode may only be a child of a "
                                                       "Group, skipping..."); return);
            m_parallelLimit = child.m_parallelLimit; // TODO: Assert on redefinition?
            break;
        case Type::Policy:
            QTC_ASSERT(m_type == Type::Group, qWarning("Workflow Policy may only be a child of a "
                                                       "Group, skipping..."); return);
            m_workflowPolicy = child.m_workflowPolicy; // TODO: Assert on redefinition?
            break;
        case Type::TaskHandler:
            QTC_ASSERT(child.m_taskHandler.m_createHandler,
                       qWarning("Task Create Handler can't be null, skipping..."); return);
            m_children.append(child);
            break;
        case Type::GroupHandler:
            QTC_ASSERT(m_type == Type::Group, qWarning("Group Handler may only be a "
                       "child of a Group, skipping..."); break);
            QTC_ASSERT(!child.m_groupHandler.m_setupHandler
                       || !m_groupHandler.m_setupHandler,
                       qWarning("Group Setup Handler redefinition, overriding..."));
            QTC_ASSERT(!child.m_groupHandler.m_doneHandler
                       || !m_groupHandler.m_doneHandler,
                       qWarning("Group Done Handler redefinition, overriding..."));
            QTC_ASSERT(!child.m_groupHandler.m_errorHandler
                       || !m_groupHandler.m_errorHandler,
                       qWarning("Group Error Handler redefinition, overriding..."));
            if (child.m_groupHandler.m_setupHandler)
                m_groupHandler.m_setupHandler = child.m_groupHandler.m_setupHandler;
            if (child.m_groupHandler.m_doneHandler)
                m_groupHandler.m_doneHandler = child.m_groupHandler.m_doneHandler;
            if (child.m_groupHandler.m_errorHandler)
                m_groupHandler.m_errorHandler = child.m_groupHandler.m_errorHandler;
            break;
        case Type::Storage:
            m_storageList.append(child.m_storageList);
            break;
        }
    }
}

void TaskItem::setTaskSetupHandler(const TaskSetupHandler &handler)
{
    if (!handler) {
        qWarning("Setting empty Setup Handler is no-op, skipping...");
        return;
    }
    if (m_taskHandler.m_setupHandler)
        qWarning("Setup Handler redefinition, overriding...");
    m_taskHandler.m_setupHandler = handler;
}

void TaskItem::setTaskDoneHandler(const TaskEndHandler &handler)
{
    if (!handler) {
        qWarning("Setting empty Done Handler is no-op, skipping...");
        return;
    }
    if (m_taskHandler.m_doneHandler)
        qWarning("Done Handler redefinition, overriding...");
    m_taskHandler.m_doneHandler = handler;
}

void TaskItem::setTaskErrorHandler(const TaskEndHandler &handler)
{
    if (!handler) {
        qWarning("Setting empty Error Handler is no-op, skipping...");
        return;
    }
    if (m_taskHandler.m_errorHandler)
        qWarning("Error Handler redefinition, overriding...");
    m_taskHandler.m_errorHandler = handler;
}

class TaskTreePrivate;
class TaskNode;

class TaskTreePrivate
{
public:
    TaskTreePrivate(TaskTree *taskTree)
        : q(taskTree) {}

    void start();
    void stop();
    void advanceProgress(int byValue);
    void emitStartedAndProgress();
    void emitProgress();
    void emitDone();
    void emitError();
    QList<TreeStorageBase> addStorages(const QList<TreeStorageBase> &storages);
    void callSetupHandler(TreeStorageBase storage, int storageId) {
        callStorageHandler(storage, storageId, &StorageHandler::m_setupHandler);
    }
    void callDoneHandler(TreeStorageBase storage, int storageId) {
        callStorageHandler(storage, storageId, &StorageHandler::m_doneHandler);
    }
    struct StorageHandler {
        TaskTree::StorageVoidHandler m_setupHandler = {};
        TaskTree::StorageVoidHandler m_doneHandler = {};
    };
    typedef TaskTree::StorageVoidHandler StorageHandler::*HandlerPtr; // ptr to class member
    void callStorageHandler(TreeStorageBase storage, int storageId, HandlerPtr ptr)
    {
        const auto it = m_storageHandlers.constFind(storage);
        if (it == m_storageHandlers.constEnd())
            return;
        GuardLocker locker(m_guard);
        const StorageHandler storageHandler = *it;
        storage.activateStorage(storageId);
        if (storageHandler.*ptr)
            (storageHandler.*ptr)(storage.activeStorageVoid());
        storage.activateStorage(0);
    }

    TaskTree *q = nullptr;
    Guard m_guard;
    int m_progressValue = 0;
    QSet<TreeStorageBase> m_storages;
    QHash<TreeStorageBase, StorageHandler> m_storageHandlers;
    std::unique_ptr<TaskNode> m_root = nullptr; // Keep me last in order to destruct first
};

class TaskContainer
{
public:
    TaskContainer(TaskTreePrivate *taskTreePrivate, const TaskItem &task,
                  TaskNode *parentNode, TaskContainer *parentContainer)
        : m_constData(taskTreePrivate, task, parentNode, parentContainer, this) {}
    TaskAction start();
    TaskAction continueStart(TaskAction startAction, int nextChild);
    TaskAction startChildren(int nextChild);
    TaskAction childDone(bool success);
    void stop();
    void invokeEndHandler();
    bool isRunning() const { return m_runtimeData.has_value(); }
    bool isStarting() const { return isRunning() && m_runtimeData->m_startGuard.isLocked(); }

    struct ConstData {
        ConstData(TaskTreePrivate *taskTreePrivate, const TaskItem &task, TaskNode *parentNode,
                  TaskContainer *parentContainer, TaskContainer *thisContainer);
        ~ConstData() { qDeleteAll(m_children); }
        TaskTreePrivate * const m_taskTreePrivate = nullptr;
        TaskNode * const m_parentNode = nullptr;
        TaskContainer * const m_parentContainer = nullptr;

        const int m_parallelLimit = 1;
        const WorkflowPolicy m_workflowPolicy = WorkflowPolicy::StopOnError;
        const TaskItem::GroupHandler m_groupHandler;
        const QList<TreeStorageBase> m_storageList;
        const QList<TaskNode *> m_children;
        const int m_taskCount = 0;
    };

    struct RuntimeData {
        RuntimeData(const ConstData &constData);
        ~RuntimeData();

        static QList<int> createStorages(const TaskContainer::ConstData &constData);
        void callStorageDoneHandlers();
        bool updateSuccessBit(bool success);
        int currentLimit() const;

        const ConstData &m_constData;
        const QList<int> m_storageIdList;
        int m_doneCount = 0;
        bool m_successBit = true;
        Guard m_startGuard;
    };

    const ConstData m_constData;
    std::optional<RuntimeData> m_runtimeData;
};

class TaskNode : public QObject
{
public:
    TaskNode(TaskTreePrivate *taskTreePrivate, const TaskItem &task,
             TaskContainer *parentContainer)
        : m_taskHandler(task.taskHandler())
        , m_container(taskTreePrivate, task, this, parentContainer)
    {}

    // If returned value != Continue, childDone() needs to be called in parent container (in caller)
    // in order to unwind properly.
    TaskAction start();
    void stop();
    void invokeEndHandler(bool success);
    bool isRunning() const { return m_task || m_container.isRunning(); }
    bool isTask() const { return bool(m_taskHandler.m_createHandler); }
    int taskCount() const { return isTask() ? 1 : m_container.m_constData.m_taskCount; }
    TaskContainer *parentContainer() const { return m_container.m_constData.m_parentContainer; }

private:
    const TaskItem::TaskHandler m_taskHandler;
    TaskContainer m_container;
    std::unique_ptr<TaskInterface> m_task;
};

void TaskTreePrivate::start()
{
    QTC_ASSERT(m_root, return);
    m_progressValue = 0;
    emitStartedAndProgress();
    // TODO: check storage handlers for not existing storages in tree
    for (auto it = m_storageHandlers.cbegin(); it != m_storageHandlers.cend(); ++it) {
        QTC_ASSERT(m_storages.contains(it.key()), qWarning("The registered storage doesn't "
                   "exist in task tree. Its handlers will never be called."));
    }
    m_root->start();
}

void TaskTreePrivate::stop()
{
    QTC_ASSERT(m_root, return);
    if (!m_root->isRunning())
        return;
    // TODO: should we have canceled flag (passed to handler)?
    // Just one done handler with result flag:
    //   FinishedWithSuccess, FinishedWithError, Canceled, TimedOut.
    // Canceled either directly by user, or by workflow policy - doesn't matter, in both
    // cases canceled from outside.
    m_root->stop();
    emitError();
}

void TaskTreePrivate::advanceProgress(int byValue)
{
    if (byValue == 0)
        return;
    QTC_CHECK(byValue > 0);
    QTC_CHECK(m_progressValue + byValue <= m_root->taskCount());
    m_progressValue += byValue;
    emitProgress();
}

void TaskTreePrivate::emitStartedAndProgress()
{
    GuardLocker locker(m_guard);
    emit q->started();
    emit q->progressValueChanged(m_progressValue);
}

void TaskTreePrivate::emitProgress()
{
    GuardLocker locker(m_guard);
    emit q->progressValueChanged(m_progressValue);
}

void TaskTreePrivate::emitDone()
{
    QTC_CHECK(m_progressValue == m_root->taskCount());
    GuardLocker locker(m_guard);
    emit q->done();
}

void TaskTreePrivate::emitError()
{
    QTC_CHECK(m_progressValue == m_root->taskCount());
    GuardLocker locker(m_guard);
    emit q->errorOccurred();
}

QList<TreeStorageBase> TaskTreePrivate::addStorages(const QList<TreeStorageBase> &storages)
{
    QList<TreeStorageBase> addedStorages;
    for (const TreeStorageBase &storage : storages) {
        QTC_ASSERT(!m_storages.contains(storage), qWarning("Can't add the same storage into "
                                                           "one TaskTree twice, skipping..."); continue);
        addedStorages << storage;
        m_storages << storage;
    }
    return addedStorages;
}

class ExecutionContextActivator
{
public:
    ExecutionContextActivator(TaskContainer *container)
        : m_container(container) { activateContext(m_container); }
    ~ExecutionContextActivator() { deactivateContext(m_container); }

private:
    static void activateContext(TaskContainer *container)
    {
        QTC_ASSERT(container && container->isRunning(), return);
        const TaskContainer::ConstData &constData = container->m_constData;
        if (constData.m_parentContainer)
            activateContext(constData.m_parentContainer);
        for (int i = 0; i < constData.m_storageList.size(); ++i)
            constData.m_storageList[i].activateStorage(container->m_runtimeData->m_storageIdList.value(i));
    }
    static void deactivateContext(TaskContainer *container)
    {
        QTC_ASSERT(container && container->isRunning(), return);
        const TaskContainer::ConstData &constData = container->m_constData;
        for (int i = constData.m_storageList.size() - 1; i >= 0; --i) // iterate in reverse order
            constData.m_storageList[i].activateStorage(0);
        if (constData.m_parentContainer)
            deactivateContext(constData.m_parentContainer);
    }
    TaskContainer *m_container = nullptr;
};

template <typename Handler, typename ...Args,
          typename ReturnType = typename std::invoke_result_t<Handler, Args...>>
ReturnType invokeHandler(TaskContainer *container, Handler &&handler, Args &&...args)
{
    ExecutionContextActivator activator(container);
    GuardLocker locker(container->m_constData.m_taskTreePrivate->m_guard);
    return std::invoke(std::forward<Handler>(handler), std::forward<Args>(args)...);
}

static QList<TaskNode *> createChildren(TaskTreePrivate *taskTreePrivate, TaskContainer *container,
                                        const TaskItem &task)
{
    QList<TaskNode *> result;
    const QList<TaskItem> &children = task.children();
    for (const TaskItem &child : children)
        result.append(new TaskNode(taskTreePrivate, child, container));
    return result;
}

TaskContainer::ConstData::ConstData(TaskTreePrivate *taskTreePrivate, const TaskItem &task,
                                    TaskNode *parentNode, TaskContainer *parentContainer,
                                    TaskContainer *thisContainer)
    : m_taskTreePrivate(taskTreePrivate)
    , m_parentNode(parentNode)
    , m_parentContainer(parentContainer)
    , m_parallelLimit(task.parallelLimit())
    , m_workflowPolicy(task.workflowPolicy())
    , m_groupHandler(task.groupHandler())
    , m_storageList(taskTreePrivate->addStorages(task.storageList()))
    , m_children(createChildren(taskTreePrivate, thisContainer, task))
    , m_taskCount(std::accumulate(m_children.cbegin(), m_children.cend(), 0,
                                  [](int r, TaskNode *n) { return r + n->taskCount(); }))
{}

QList<int> TaskContainer::RuntimeData::createStorages(const TaskContainer::ConstData &constData)
{
    QList<int> storageIdList;
    for (const TreeStorageBase &storage : constData.m_storageList) {
        const int storageId = storage.createStorage();
        storageIdList.append(storageId);
        constData.m_taskTreePrivate->callSetupHandler(storage, storageId);
    }
    return storageIdList;
}

void TaskContainer::RuntimeData::callStorageDoneHandlers()
{
    for (int i = m_constData.m_storageList.size() - 1; i >= 0; --i) { // iterate in reverse order
        const TreeStorageBase storage = m_constData.m_storageList[i];
        const int storageId = m_storageIdList.value(i);
        m_constData.m_taskTreePrivate->callDoneHandler(storage, storageId);
    }
}

TaskContainer::RuntimeData::RuntimeData(const ConstData &constData)
    : m_constData(constData)
    , m_storageIdList(createStorages(constData))
{
    m_successBit = m_constData.m_workflowPolicy != WorkflowPolicy::StopOnDone
                && m_constData.m_workflowPolicy != WorkflowPolicy::ContinueOnDone;
}

TaskContainer::RuntimeData::~RuntimeData()
{
    for (int i = m_constData.m_storageList.size() - 1; i >= 0; --i) { // iterate in reverse order
        const TreeStorageBase storage = m_constData.m_storageList[i];
        const int storageId = m_storageIdList.value(i);
        storage.deleteStorage(storageId);
    }
}

bool TaskContainer::RuntimeData::updateSuccessBit(bool success)
{
    if (m_constData.m_workflowPolicy == WorkflowPolicy::Optional)
        return m_successBit;

    const bool donePolicy = m_constData.m_workflowPolicy == WorkflowPolicy::StopOnDone
                         || m_constData.m_workflowPolicy == WorkflowPolicy::ContinueOnDone;
    m_successBit = donePolicy ? (m_successBit || success) : (m_successBit && success);
    return m_successBit;
}

int TaskContainer::RuntimeData::currentLimit() const
{
    const int childCount = m_constData.m_children.size();
    return m_constData.m_parallelLimit
               ? qMin(m_doneCount + m_constData.m_parallelLimit, childCount) : childCount;
}

TaskAction TaskContainer::start()
{
    QTC_CHECK(!isRunning());
    m_runtimeData.emplace(m_constData);

    TaskAction startAction = TaskAction::Continue;
    if (m_constData.m_groupHandler.m_setupHandler) {
        startAction = invokeHandler(this, m_constData.m_groupHandler.m_setupHandler);
        if (startAction != TaskAction::Continue)
            m_constData.m_taskTreePrivate->advanceProgress(m_constData.m_taskCount);
    }
    if (startAction == TaskAction::Continue) {
        if (m_constData.m_children.isEmpty())
            startAction = TaskAction::StopWithDone;
    }
    return continueStart(startAction, 0);
}

TaskAction TaskContainer::continueStart(TaskAction startAction, int nextChild)
{
    const TaskAction groupAction = startAction == TaskAction::Continue ? startChildren(nextChild)
                                                                       : startAction;
    QTC_CHECK(isRunning()); // TODO: superfluous
    if (groupAction != TaskAction::Continue) {
        const bool success = m_runtimeData->updateSuccessBit(groupAction == TaskAction::StopWithDone);
        invokeEndHandler();
        if (TaskContainer *parentContainer = m_constData.m_parentContainer) {
            QTC_CHECK(parentContainer->isRunning());
            if (!parentContainer->isStarting())
                parentContainer->childDone(success);
        } else if (success) {
            m_constData.m_taskTreePrivate->emitDone();
        } else {
            m_constData.m_taskTreePrivate->emitError();
        }
    }
    return groupAction;
}

TaskAction TaskContainer::startChildren(int nextChild)
{
    QTC_CHECK(isRunning());
    GuardLocker locker(m_runtimeData->m_startGuard);
    for (int i = nextChild; i < m_constData.m_children.size(); ++i) {
        const int limit = m_runtimeData->currentLimit();
        if (i >= limit)
            break;

        const TaskAction startAction = m_constData.m_children.at(i)->start();
        if (startAction == TaskAction::Continue)
            continue;

        const TaskAction finalizeAction = childDone(startAction == TaskAction::StopWithDone);
        if (finalizeAction == TaskAction::Continue)
            continue;

        int skippedTaskCount = 0;
        // Skip scheduled but not run yet. The current (i) was already notified.
        for (int j = i + 1; j < limit; ++j)
            skippedTaskCount += m_constData.m_children.at(j)->taskCount();
        m_constData.m_taskTreePrivate->advanceProgress(skippedTaskCount);
        return finalizeAction;
    }
    return TaskAction::Continue;
}

TaskAction TaskContainer::childDone(bool success)
{
    QTC_CHECK(isRunning());
    const int limit = m_runtimeData->currentLimit(); // Read before bumping m_doneCount and stop()
    const bool shouldStop = (m_constData.m_workflowPolicy == WorkflowPolicy::StopOnDone && success)
                         || (m_constData.m_workflowPolicy == WorkflowPolicy::StopOnError && !success);
    if (shouldStop)
        stop();

    ++m_runtimeData->m_doneCount;
    const bool updatedSuccess = m_runtimeData->updateSuccessBit(success);
    const TaskAction startAction
        = (shouldStop || m_runtimeData->m_doneCount == m_constData.m_children.size())
        ? toTaskAction(updatedSuccess) : TaskAction::Continue;

    if (isStarting())
        return startAction;
    return continueStart(startAction, limit);
}

void TaskContainer::stop()
{
    if (!isRunning())
        return;

    const int limit = m_runtimeData->currentLimit();
    for (int i = 0; i < limit; ++i)
        m_constData.m_children.at(i)->stop();

    int skippedTaskCount = 0;
    for (int i = limit; i < m_constData.m_children.size(); ++i)
        skippedTaskCount += m_constData.m_children.at(i)->taskCount();

    m_constData.m_taskTreePrivate->advanceProgress(skippedTaskCount);
}

void TaskContainer::invokeEndHandler()
{
    const TaskItem::GroupHandler &groupHandler = m_constData.m_groupHandler;
    if (m_runtimeData->m_successBit && groupHandler.m_doneHandler)
        invokeHandler(this, groupHandler.m_doneHandler);
    else if (!m_runtimeData->m_successBit && groupHandler.m_errorHandler)
        invokeHandler(this, groupHandler.m_errorHandler);
    m_runtimeData->callStorageDoneHandlers();
    m_runtimeData.reset();
}

TaskAction TaskNode::start()
{
    QTC_CHECK(!isRunning());
    if (!isTask())
        return m_container.start();

    m_task.reset(m_taskHandler.m_createHandler());
    const TaskAction startAction = m_taskHandler.m_setupHandler
        ? invokeHandler(parentContainer(), m_taskHandler.m_setupHandler, *m_task.get())
        : TaskAction::Continue;
    if (startAction != TaskAction::Continue) {
        m_container.m_constData.m_taskTreePrivate->advanceProgress(1);
        m_task.reset();
        return startAction;
    }
    const std::shared_ptr<TaskAction> unwindAction
        = std::make_shared<TaskAction>(TaskAction::Continue);
    connect(m_task.get(), &TaskInterface::done, this, [=](bool success) {
        invokeEndHandler(success);
        disconnect(m_task.get(), &TaskInterface::done, this, nullptr);
        m_task.release()->deleteLater();
        QTC_ASSERT(parentContainer() && parentContainer()->isRunning(), return);
        if (parentContainer()->isStarting())
            *unwindAction = toTaskAction(success);
        else
            parentContainer()->childDone(success);
    });

    m_task->start();
    return *unwindAction;
}

void TaskNode::stop()
{
    if (!isRunning())
        return;

    if (!m_task) {
        m_container.stop();
        m_container.invokeEndHandler();
        return;
    }

    // TODO: cancelHandler?
    // TODO: call TaskInterface::stop() ?
    invokeEndHandler(false);
    m_task.reset();
}

void TaskNode::invokeEndHandler(bool success)
{
    if (success && m_taskHandler.m_doneHandler)
        invokeHandler(parentContainer(), m_taskHandler.m_doneHandler, *m_task.get());
    else if (!success && m_taskHandler.m_errorHandler)
        invokeHandler(parentContainer(), m_taskHandler.m_errorHandler, *m_task.get());
    m_container.m_constData.m_taskTreePrivate->advanceProgress(1);
}

/*!
    \class TaskTree
    \inheaderfile solutions/tasking/tasktree.h
    \inmodule QtCreator
    \ingroup mainclasses
    \brief The TaskTree class runs an async task tree structure defined in a
           declarative way.

    Use the Tasking namespace to build extensible, declarative task tree
    structures that contain possibly asynchronous tasks, such as Process,
    FileTransfer, or Async<ReturnType>. TaskTree structures enable you
    to create a sophisticated mixture of a parallel or sequential flow of tasks
    in the form of a tree and to run it any time later.

    \section1 Root Element and Tasks

    The TaskTree has a mandatory Group root element, which may contain
    any number of tasks of various types, such as ProcessTask, FileTransferTask,
    or AsyncTask<ReturnType>:

    \code
        using namespace Tasking;

        const Group root {
            ProcessTask(...),
            AsyncTask<int>(...),
            FileTransferTask(...)
        };

        TaskTree *taskTree = new TaskTree(root);
        connect(taskTree, &TaskTree::done, ...);          // a successfully finished handler
        connect(taskTree, &TaskTree::errorOccurred, ...); // an erroneously finished handler
        taskTree->start();
    \endcode

    The task tree above has a top level element of the Group type that contains
    tasks of the type ProcessTask, FileTransferTask, and AsyncTask<int>.
    After taskTree->start() is called, the tasks are run in a chain, starting
    with ProcessTask. When the ProcessTask finishes successfully, the AsyncTask<int> task is
    started. Finally, when the asynchronous task finishes successfully, the
    FileTransferTask task is started.

    When the last running task finishes with success, the task tree is considered
    to have run successfully and the TaskTree::done() signal is emitted.
    When a task finishes with an error, the execution of the task tree is stopped
    and the remaining tasks are skipped. The task tree finishes with an error and
    sends the TaskTree::errorOccurred() signal.

    \section1 Groups

    The parent of the Group sees it as a single task. Like other tasks,
    the group can be started and it can finish with success or an error.
    The Group elements can be nested to create a tree structure:

    \code
        const Group root {
            Group {
                parallel,
                ProcessTask(...),
                AsyncTask<int>(...)
            },
            FileTransferTask(...)
        };
    \endcode

    The example above differs from the first example in that the root element has
    a subgroup that contains the ProcessTask and AsyncTask<int>. The subgroup is a
    sibling element of the FileTransferTask in the root. The subgroup contains an
    additional \e parallel element that instructs its Group to execute its tasks
    in parallel.

    So, when the tree above is started, the ProcessTask and AsyncTask<int> start
    immediately and run in parallel. Since the root group doesn't contain a
    \e parallel element, its direct child tasks are run in sequence. Thus, the
    FileTransferTask starts when the whole subgroup finishes. The group is
    considered as finished when all its tasks have finished. The order in which
    the tasks finish is not relevant.

    So, depending on which task lasts longer (ProcessTask or AsyncTask<int>), the
    following scenarios can take place:

    \table
    \header
        \li Scenario 1
        \li Scenario 2
    \row
        \li Root Group starts
        \li Root Group starts
    \row
        \li Sub Group starts
        \li Sub Group starts
    \row
        \li ProcessTask starts
        \li ProcessTask starts
    \row
        \li AsyncTask<int> starts
        \li AsyncTask<int> starts
    \row
        \li ...
        \li ...
    \row
        \li \b {ProcessTask finishes}
        \li \b {AsyncTask<int> finishes}
    \row
        \li ...
        \li ...
    \row
        \li \b {AsyncTask<int> finishes}
        \li \b {ProcessTask finishes}
    \row
        \li Sub Group finishes
        \li Sub Group finishes
    \row
        \li FileTransferTask starts
        \li FileTransferTask starts
    \row
        \li ...
        \li ...
    \row
        \li FileTransferTask finishes
        \li FileTransferTask finishes
    \row
        \li Root Group finishes
        \li Root Group finishes
    \endtable

    The differences between the scenarios are marked with bold. Three dots mean
    that an unspecified amount of time passes between previous and next events
    (a task or tasks continue to run). No dots between events
    means that they occur synchronously.

    The presented scenarios assume that all tasks run successfully. If a task
    fails during execution, the task tree finishes with an error. In particular,
    when ProcessTask finishes with an error while AsyncTask<int> is still being executed,
    the AsyncTask<int> is automatically stopped, the subgroup finishes with an error,
    the FileTransferTask is skipped, and the tree finishes with an error.

    \section1 Task Types

    Each task type is associated with its corresponding task class that executes
    the task. For example, a ProcessTask inside a task tree is associated with
    the Process class that executes the process. The associated objects are
    automatically created, started, and destructed exclusively by the task tree
    at the appropriate time.

    If a root group consists of five sequential ProcessTask tasks, and the task tree
    executes the group, it creates an instance of Process for the first
    ProcessTask and starts it. If the Process instance finishes successfully,
    the task tree destructs it and creates a new Process instance for the
    second ProcessTask, and so on. If the first task finishes with an error, the task
    tree stops creating Process instances, and the root group finishes with an
    error.

    The following table shows examples of task types and their corresponding task
    classes:

    \table
    \header
        \li Task Type (Tasking Namespace)
        \li Associated Task Class
        \li Brief Description
    \row
        \li ProcessTask
        \li Utils::Process
        \li Starts processes.
    \row
        \li AsyncTask<ReturnType>
        \li Utils::Async<ReturnType>
        \li Starts asynchronous tasks; run in separate thread.
    \row
        \li TaskTreeTask
        \li Utils::TaskTree
        \li Starts a nested task tree.
    \row
        \li FileTransferTask
        \li ProjectExplorer::FileTransfer
        \li Starts file transfer between different devices.
    \endtable

    \section1 Task Handlers

    Use Task handlers to set up a task for execution and to enable reading
    the output data from the task when it finishes with success or an error.

    \section2 Task Start Handler

    When a corresponding task class object is created and before it's started,
    the task tree invokes a mandatory user-provided setup handler. The setup
    handler should always take a \e reference to the associated task class object:

    \code
        const auto onSetup = [](Process &process) {
            process.setCommand({"sleep", {"3"}});
        };
        const Group root {
            ProcessTask(onSetup)
        };
    \endcode

    You can modify the passed Process in the setup handler, so that the task
    tree can start the process according to your configuration.
    You do not need to call \e {process.start();} in the setup handler,
    as the task tree calls it when needed. The setup handler is mandatory
    and must be the first argument of the task's constructor.

    Optionally, the setup handler may return a TaskAction. The returned
    TaskAction influences the further start behavior of a given task. The
    possible values are:

    \table
    \header
        \li TaskAction Value
        \li Brief Description
    \row
        \li Continue
        \li The task is started normally. This is the default behavior when the
            setup handler doesn't return TaskAction (that is, its return type is
            void).
    \row
        \li StopWithDone
        \li The task won't be started and it will report success to its parent.
    \row
        \li StopWithError
        \li The task won't be started and it will report an error to its parent.
    \endtable

    This is useful for running a task only when a condition is met and the data
    needed to evaluate this condition is not known until previously started tasks
    finish. This way, the setup handler dynamically decides whether to start the
    corresponding task normally or skip it and report success or an error.
    For more information about inter-task data exchange, see \l Storage.

    \section2 Task's Done and Error Handlers

    When a running task finishes, the task tree invokes an optionally provided
    done or error handler. Both handlers should always take a \e {const reference}
    to the associated task class object:

    \code
        const auto onSetup = [](Process &process) {
            process.setCommand({"sleep", {"3"}});
        };
        const auto onDone = [](const Process &process) {
            qDebug() << "Success" << process.cleanedStdOut();
        };
        const auto onError = [](const Process &process) {
            qDebug() << "Failure" << process.cleanedStdErr();
        };
        const Group root {
            ProcessTask(onSetup, onDone, onError)
        };
    \endcode

    The done and error handlers may collect output data from Process, and store it
    for further processing or perform additional actions. The done handler is optional.
    When used, it must be the second argument of the task constructor.
    The error handler must always be the third argument.
    You can omit the handlers or substitute the ones that you do not need with curly braces ({}).

    \note If the task setup handler returns StopWithDone or StopWithError,
    neither the done nor error handler is invoked.

    \section1 Group Handlers

    Similarly to task handlers, group handlers enable you to set up a group to
    execute and to apply more actions when the whole group finishes with
    success or an error.

    \section2 Group's Start Handler

    The task tree invokes the group start handler before it starts the child
    tasks. The group handler doesn't take any arguments:

    \code
        const auto onGroupSetup = [] {
            qDebug() << "Entering the group";
        };
        const Group root {
            OnGroupSetup(onGroupSetup),
            ProcessTask(...)
        };
    \endcode

    The group setup handler is optional. To define a group setup handler, add an
    OnGroupSetup element to a group. The argument of OnGroupSetup is a user
    handler. If you add more than one OnGroupSetup element to a group, an assert
    is triggered at runtime that includes an error message.

    Like the task start handler, the group start handler may return TaskAction.
    The returned TaskAction value affects the start behavior of the
    whole group. If you do not specify a group start handler or its return type
    is void, the default group's action is TaskAction::Continue, so that all
    tasks are started normally. Otherwise, when the start handler returns
    TaskAction::StopWithDone or TaskAction::StopWithError, the tasks are not
    started (they are skipped) and the group itself reports success or failure,
    depending on the returned value, respectively.

    \code
        const Group root {
            OnGroupSetup([] { qDebug() << "Root setup"; }),
            Group {
                OnGroupSetup([] { qDebug() << "Group 1 setup"; return TaskAction::Continue; }),
                ProcessTask(...) // Process 1
            },
            Group {
                OnGroupSetup([] { qDebug() << "Group 2 setup"; return TaskAction::StopWithDone; }),
                ProcessTask(...) // Process 2
            },
            Group {
                OnGroupSetup([] { qDebug() << "Group 3 setup"; return TaskAction::StopWithError; }),
                ProcessTask(...) // Process 3
            },
            ProcessTask(...) // Process 4
        };
    \endcode

    In the above example, all subgroups of a root group define their setup handlers.
    The following scenario assumes that all started processes finish with success:

    \table
    \header
        \li Scenario
        \li Comment
    \row
        \li Root Group starts
        \li Doesn't return TaskAction, so its tasks are executed.
    \row
        \li Group 1 starts
        \li Returns Continue, so its tasks are executed.
    \row
        \li Process 1 starts
        \li
    \row
        \li ...
        \li ...
    \row
        \li Process 1 finishes (success)
        \li
    \row
        \li Group 1 finishes (success)
        \li
    \row
        \li Group 2 starts
        \li Returns StopWithDone, so Process 2 is skipped and Group 2 reports
            success.
    \row
        \li Group 2 finishes (success)
        \li
    \row
        \li Group 3 starts
        \li Returns StopWithError, so Process 3 is skipped and Group 3 reports
            an error.
    \row
        \li Group 3 finishes (error)
        \li
    \row
        \li Root Group finishes (error)
        \li Group 3, which is a direct child of the root group, finished with an
            error, so the root group stops executing, skips Process 4, which has
            not started yet, and reports an error.
    \endtable

    \section2 Groups's Done and Error Handlers

    A Group's done or error handler is executed after the successful or failed
    execution of its tasks, respectively. The final value reported by the
    group depends on its \l {Workflow Policy}. The handlers can apply other
    necessary actions. The done and error handlers are defined inside the
    OnGroupDone and OnGroupError elements of a group, respectively. They do not
    take arguments:

    \code
        const Group root {
            OnGroupSetup([] { qDebug() << "Root setup"; }),
            ProcessTask(...),
            OnGroupDone([] { qDebug() << "Root finished with success"; }),
            OnGroupError([] { qDebug() << "Root finished with error"; })
        };
    \endcode

    The group done and error handlers are optional. If you add more than one
    OnGroupDone or OnGroupError each to a group, an assert is triggered at
    runtime that includes an error message.

    \note Even if the group setup handler returns StopWithDone or StopWithError,
    one of the task's done or error handlers is invoked. This behavior differs
    from that of task handlers and might change in the future.

    \section1 Other Group Elements

    A group can contain other elements that describe the processing flow, such as
    the execution mode or workflow policy. It can also contain storage elements
    that are responsible for collecting and sharing custom common data gathered
    during group execution.

    \section2 Execution Mode

    The execution mode element in a Group specifies how the direct child tasks of
    the Group are started.

    \table
    \header
        \li Execution Mode
        \li Description
    \row
        \li sequential
        \li Default. When a Group has no execution mode, it runs in the
            sequential mode. All the direct child tasks of a group are started
            in a chain, so that when one task finishes, the next one starts.
            This enables you to pass the results from the previous task
            as input to the next task before it starts. This mode guarantees
            that the next task is started only after the previous task finishes.
    \row
        \li parallel
        \li All the direct child tasks of a group are started after the group is
            started, without waiting for the previous tasks to finish. In this
            mode, all tasks run simultaneously.
    \row
        \li ParallelLimit(int limit)
        \li In this mode, a limited number of direct child tasks run simultaneously.
            The \e limit defines the maximum number of tasks running in parallel
            in a group. When the group is started, the first batch tasks is
            started (the number of tasks in batch equals to passed limit, at most),
            while the others are kept waiting. When a running task finishes,
            the group starts the next remaining one, so that the \e limit
            of simultaneously running tasks inside a group isn't exceeded.
            This repeats on every child task's finish until all child tasks are started.
            This enables you to limit the maximum number of tasks that
            run simultaneously, for example if running too many processes might
            block the machine for a long time. The value 1 means \e sequential
            execution. The value 0 means unlimited and equals \e parallel.
    \endtable

    In all execution modes, a group starts tasks in the oder in which they appear.

    If a child of a group is also a group (in a nested tree), the child group
    runs its tasks according to its own execution mode.

    \section2 Workflow Policy

    The workflow policy element in a Group specifies how the group should behave
    when its direct child tasks finish:

    \table
    \header
        \li Workflow Policy
        \li Description
    \row
        \li stopOnError
        \li Default. If a task finishes with an error, the group:
        \list 1
            \li Stops the running tasks (if any - for example, in parallel
                mode).
            \li Skips executing tasks it has not started (for example, in the
                sequential mode).
            \li Immediately finishes with an error.
        \endlist
        If all child tasks finish successfully or the group is empty, the group
        finishes with success.
    \row
        \li continueOnError
        \li Similar to stopOnError, but in case any child finishes with
            an error, the execution continues until all tasks finish,
            and the group reports an error afterwards, even when some other
            tasks in group finished with success.
            If a task finishes with an error, the group:
        \list 1
            \li Continues executing the tasks that are running or have not
                started yet.
            \li Finishes with an error when all tasks finish.
        \endlist
        If all tasks finish successfully or the group is empty, the group
        finishes with success.
    \row
        \li stopOnDone
        \li If a task finishes with success, the group:
        \list 1
            \li Stops running tasks and skips those that it has not started.
            \li Immediately finishes with success.
        \endlist
        If all tasks finish with an error or the group is empty, the group
        finishes with an error.
    \row
        \li continueOnDone
        \li Similar to stopOnDone, but in case any child finishes
            successfully, the execution continues until all tasks finish,
            and the group reports success afterwards, even when some other
            tasks in group finished with an error.
            If a task finishes with success, the group:
        \list 1
            \li Continues executing the tasks that are running or have not
                started yet.
            \li Finishes with success when all tasks finish.
        \endlist
        If all tasks finish with an error or the group is empty, the group
        finishes with an error.
    \row
        \li optional
        \li The group executes all tasks and ignores their return state. If all
            tasks finish or the group is empty, the group finishes with success.
    \endtable

    If a child of a group is also a group (in a nested tree), the child group
    runs its tasks according to its own workflow policy.

    \section2 Storage

    Use the Storage element to exchange information between tasks. Especially,
    in the sequential execution mode, when a task needs data from another task
    before it can start. For example, a task tree that copies data by reading
    it from a source and writing it to a destination might look as follows:

    \code
        static QByteArray load(const FilePath &fileName) { ... }
        static void save(const FilePath &fileName, const QByteArray &array) { ... }

        static TaskItem diffRecipe(const FilePath &source, const FilePath &destination)
        {
            struct CopyStorage { // [1] custom inter-task struct
                QByteArray content; // [2] custom inter-task data
            };

            // [3] instance of custom inter-task struct manageable by task tree
            const TreeStorage<CopyStorage> storage;

            const auto onLoaderSetup = [source](Async<QByteArray> &async) {
                async.setConcurrentCallData(&load, source);
            };
            // [4] runtime: task tree activates the instance from [5] before invoking handler
            const auto onLoaderDone = [storage](const Async<QByteArray> &async) {
                storage->content = async.result();
            };

            // [4] runtime: task tree activates the instance from [5] before invoking handler
            const auto onSaverSetup = [storage, destination](Async<void> &async) {
                async.setConcurrentCallData(&save, destination, storage->content);
            };
            const auto onSaverDone = [](const Async<void> &async) {
                qDebug() << "Save done successfully";
            };

            const Group root {
                // [5] runtime: task tree creates an instance of CopyStorage when root is entered
                Storage(storage),
                AsyncTask<QByteArray>(onLoaderSetup, onLoaderDone),
                AsyncTask<void>(onSaverSetup, onSaverDone)
            };
            return root;
        }
    \endcode

    In the example above, the inter-task data consists of a QByteArray content
    variable [2] enclosed in a CopyStorage custom struct [1]. If the loader
    finishes successfully, it stores the data in a CopyStorage::content
    variable. The saver then uses the variable to configure the saving task.

    To enable a task tree to manage the CopyStorage struct, an instance of
    TreeStorage<CopyStorage> is created [3]. If a copy of this object is
    inserted as group's child task [5], an instance of CopyStorage struct is
    created dynamically when the task tree enters this group. When the task
    tree leaves this group, the existing instance of CopyStorage struct is
    destructed as it's no longer needed.

    If several task trees that hold a copy of the common TreeStorage<CopyStorage>
    instance run simultaneously, each task tree contains its own copy of the
    CopyStorage struct.

    You can access CopyStorage from any handler in the group with a storage object.
    This includes all handlers of all descendant tasks of the group with
    a storage object. To access the custom struct in a handler, pass the
    copy of the TreeStorage<CopyStorage> object to the handler (for example, in
    a lambda capture) [4].

    When the task tree invokes a handler in a subtree containing the storage [5],
    the task tree activates its own CopyStorage instance inside the
    TreeStorage<CopyStorage> object. Therefore, the CopyStorage struct may be
    accessed only from within the handler body. To access the currently active
    CopyStorage from within TreeStorage<CopyStorage>, use the TreeStorage::operator->()
    or TreeStorage::activeStorage() method.

    The following list summarizes how to employ a Storage object into the task
    tree:
    \list 1
        \li Define the custom structure MyStorage with custom data [1], [2]
        \li Create an instance of TreeStorage<MyStorage> storage [3]
        \li Pass the TreeStorage<MyStorage> instance to handlers [4]
        \li Insert the TreeStorage<MyStorage> instance into a group [5]
    \endlist

    \note The current implementation assumes that all running task trees
    containing copies of the same TreeStorage run in the same thread. Otherwise,
    the behavior is undefined.

    \section1 TaskTree

    TaskTree executes the tree structure of asynchronous tasks according to the
    recipe described by the Group root element.

    As TaskTree is also an asynchronous task, it can be a part of another TaskTree.
    To place a nested TaskTree inside another TaskTree, insert the TaskTreeTask
    element into other tree's Group element.

    TaskTree reports progress of completed tasks when running. The progress value
    is increased when a task finishes or is skipped or stopped.
    When TaskTree is finished and the TaskTree::done() or TaskTree::errorOccurred()
    signal is emitted, the current value of the progress equals the maximum
    progress value. Maximum progress equals the total number of tasks in a tree.
    A nested TaskTree is counted as a single task, and its child tasks are not
    counted in the top level tree. Groups themselves are not counted as tasks,
    but their tasks are counted.

    To set additional initial data for the running tree, modify the storage
    instances in a tree when it creates them by installing a storage setup
    handler:

    \code
        TreeStorage<CopyStorage> storage;
        Group root = ...; // storage placed inside root's group and inside handlers
        TaskTree taskTree(root);
        auto initStorage = [](CopyStorage *storage){
            storage->content = "initial content";
        };
        taskTree.onStorageSetup(storage, initStorage);
        taskTree.start();
    \endcode

    When the running task tree creates a CopyStorage instance, and before any
    handler inside a tree is called, the task tree calls the initStorage handler,
    to enable setting up initial data of the storage, unique to this particular
    run of taskTree.

    Similarly, to collect some additional result data from the running tree,
    read it from storage instances in the tree when they are about to be
    destroyed. To do this, install a storage done handler:

    \code
        TreeStorage<CopyStorage> storage;
        Group root = ...; // storage placed inside root's group and inside handlers
        TaskTree taskTree(root);
        auto collectStorage = [](CopyStorage *storage){
            qDebug() << "final content" << storage->content;
        };
        taskTree.onStorageDone(storage, collectStorage);
        taskTree.start();
    \endcode

    When the running task tree is about to destroy a CopyStorage instance, the
    task tree calls the collectStorage handler, to enable reading the final data
    from the storage, unique to this particular run of taskTree.

    \section1 Task Adapters

    To extend a TaskTree with new a task type, implement a simple adapter class
    derived from the TaskAdapter class template. The following class is an
    adapter for a single shot timer, which may be considered as a new
    asynchronous task:

    \code
        class TimeoutAdapter : public Tasking::TaskAdapter<QTimer>
        {
        public:
            TimeoutAdapter() {
                task()->setSingleShot(true);
                task()->setInterval(1000);
                connect(task(), &QTimer::timeout, this, [this] { emit done(true); });
            }
            void start() final { task()->start(); }
        };

        QTC_DECLARE_CUSTOM_TASK(Timeout, TimeoutAdapter);
    \endcode

    You must derive the custom adapter from the TaskAdapter class template
    instantiated with a template parameter of the class implementing a running
    task. The code above uses QTimer to run the task. This class appears
    later as an argument to the task's handlers. The instance of this class
    parameter automatically becomes a member of the TaskAdapter template, and is
    accessible through the TaskAdapter::task() method. The constructor
    of TimeoutAdapter initially configures the QTimer object and connects
    to the QTimer::timeout signal. When the signal is triggered, TimeoutAdapter
    emits the done(true) signal to inform the task tree that the task finished
    successfully. If it emits done(false), the task finished with an error.
    The TaskAdapter::start() method starts the timer.

    To make QTimer accessible inside TaskTree under the \e Timeout name,
    register it with QTC_DECLARE_CUSTOM_TASK(Timeout, TimeoutAdapter). Timeout
    becomes a new task type inside Tasking namespace, using TimeoutAdapter.

    The new task type is now registered, and you can use it in TaskTree:

    \code
        const auto onTimeoutSetup = [](QTimer &task) {
            task.setInterval(2000);
        };
        const auto onTimeoutDone = [](const QTimer &task) {
            qDebug() << "timeout triggered";
        };

        const Group root {
            Timeout(onTimeoutSetup, onTimeoutDone)
        };
    \endcode

    When a task tree containing the root from the above example is started, it
    prints a debug message within two seconds and then finishes successfully.

    \note The class implementing the running task should have a default constructor,
    and objects of this class should be freely destructible. It should be allowed
    to destroy a running object, preferably without waiting for the running task
    to finish (that is, safe non-blocking destructor of a running task).
*/

TaskTree::TaskTree()
    : d(new TaskTreePrivate(this))
{
}

TaskTree::TaskTree(const Group &root) : TaskTree()
{
    setupRoot(root);
}

TaskTree::~TaskTree()
{
    QTC_ASSERT(!d->m_guard.isLocked(), qWarning("Deleting TaskTree instance directly from "
               "one of its handlers will lead to crash!"));
    // TODO: delete storages explicitly here?
    delete d;
}

void TaskTree::setupRoot(const Group &root)
{
    QTC_ASSERT(!isRunning(), qWarning("The TaskTree is already running, ignoring..."); return);
    QTC_ASSERT(!d->m_guard.isLocked(), qWarning("The setupRoot() is called from one of the"
                                                "TaskTree handlers, ingoring..."); return);
    d->m_storages.clear();
    d->m_root.reset(new TaskNode(d, root, nullptr));
}

void TaskTree::start()
{
    QTC_ASSERT(!isRunning(), qWarning("The TaskTree is already running, ignoring..."); return);
    QTC_ASSERT(!d->m_guard.isLocked(), qWarning("The start() is called from one of the"
                                                "TaskTree handlers, ingoring..."); return);
    d->start();
}

void TaskTree::stop()
{
    QTC_ASSERT(!d->m_guard.isLocked(), qWarning("The stop() is called from one of the"
                                                "TaskTree handlers, ingoring..."); return);
    d->stop();
}

bool TaskTree::isRunning() const
{
    return d->m_root && d->m_root->isRunning();
}

int TaskTree::taskCount() const
{
    return d->m_root ? d->m_root->taskCount() : 0;
}

int TaskTree::progressValue() const
{
    return d->m_progressValue;
}

void TaskTree::setupStorageHandler(const TreeStorageBase &storage,
                                   StorageVoidHandler setupHandler,
                                   StorageVoidHandler doneHandler)
{
    auto it = d->m_storageHandlers.find(storage);
    if (it == d->m_storageHandlers.end()) {
        d->m_storageHandlers.insert(storage, {setupHandler, doneHandler});
        return;
    }
    if (setupHandler) {
        QTC_ASSERT(!it->m_setupHandler,
                   qWarning("The storage has its setup handler defined, overriding..."));
        it->m_setupHandler = setupHandler;
    }
    if (doneHandler) {
        QTC_ASSERT(!it->m_doneHandler,
                   qWarning("The storage has its done handler defined, overriding..."));
        it->m_doneHandler = doneHandler;
    }
}

TaskTreeTaskAdapter::TaskTreeTaskAdapter()
{
    connect(task(), &TaskTree::done, this, [this] { emit done(true); });
    connect(task(), &TaskTree::errorOccurred, this, [this] { emit done(false); });
}

void TaskTreeTaskAdapter::start()
{
    task()->start();
}

} // namespace Tasking
