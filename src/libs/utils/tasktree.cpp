// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tasktree.h"

#include "guard.h"
#include "qtcassert.h"

#include <QSet>

namespace Utils {
namespace Tasking {

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
    QTC_ASSERT(m_storageData->m_activeStorage, return nullptr);
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
    QTC_ASSERT(m_type == Type::Group, qWarning("Only Task may have children, skipping..."); return);
    for (const TaskItem &child : children) {
        switch (child.m_type) {
        case Type::Group:
            m_children.append(child);
            break;
        case Type::Limit:
            QTC_ASSERT(m_type == Type::Group,
                       qWarning("Mode may only be a child of Group, skipping..."); return);
            m_parallelLimit = child.m_parallelLimit; // TODO: Assert on redefinition?
            break;
        case Type::Policy:
            QTC_ASSERT(m_type == Type::Group,
                       qWarning("Workflow Policy may only be a child of Group, skipping..."); return);
            m_workflowPolicy = child.m_workflowPolicy; // TODO: Assert on redefinition?
            break;
        case Type::TaskHandler:
            QTC_ASSERT(child.m_taskHandler.m_createHandler,
                       qWarning("Task Create Handler can't be null, skipping..."); return);
            QTC_ASSERT(child.m_taskHandler.m_setupHandler,
                       qWarning("Task Setup Handler can't be null, skipping..."); return);
            m_children.append(child);
            break;
        case Type::GroupHandler:
            QTC_ASSERT(m_type == Type::Group, qWarning("Group Handler may only be a "
                       "child of Group, skipping..."); break);
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

} // namespace Tasking

using namespace Tasking;

class TaskTreePrivate;
class TaskNode;

class TaskContainer
{
public:
    TaskContainer(TaskTreePrivate *taskTreePrivate, const TaskItem &task,
                  TaskContainer *parentContainer)
        : m_constData(taskTreePrivate, task, parentContainer, this) {}
    TaskAction start();
    TaskAction continueStart(TaskAction startAction, int nextChild);
    TaskAction startChildren(int nextChild);
    TaskAction childDone(bool success);
    void stop();
    void invokeEndHandler();
    bool isRunning() const { return m_runtimeData.has_value(); }
    bool isStarting() const { return isRunning() && m_runtimeData->m_startGuard.isLocked(); }

    struct ConstData {
        ConstData(TaskTreePrivate *taskTreePrivate, const TaskItem &task,
                  TaskContainer *parentContainer, TaskContainer *thisContainer);
        ~ConstData() { qDeleteAll(m_children); }
        TaskTreePrivate * const m_taskTreePrivate = nullptr;
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
        , m_container(taskTreePrivate, task, parentContainer)
    {}

    // If returned value != Continue, childDone() needs to be called in parent container (in caller)
    // in order to unwind properly.
    TaskAction start();
    void stop();
    void invokeEndHandler(bool success);
    bool isRunning() const { return m_task || m_container.isRunning(); }
    bool isTask() const { return m_taskHandler.m_createHandler && m_taskHandler.m_setupHandler; }
    int taskCount() const { return isTask() ? 1 : m_container.m_constData.m_taskCount; }
    TaskContainer *parentContainer() const { return m_container.m_constData.m_parentContainer; }

private:
    const TaskItem::TaskHandler m_taskHandler;
    TaskContainer m_container;
    std::unique_ptr<TaskInterface> m_task;
};

class TaskTreePrivate
{
public:
    TaskTreePrivate(TaskTree *taskTree)
        : q(taskTree) {}

    void start() {
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
    void stop() {
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
    void advanceProgress(int byValue) {
        if (byValue == 0)
            return;
        QTC_CHECK(byValue > 0);
        QTC_CHECK(m_progressValue + byValue <= m_root->taskCount());
        m_progressValue += byValue;
        emitProgress();
    }
    void emitStartedAndProgress() {
        GuardLocker locker(m_guard);
        emit q->started();
        emit q->progressValueChanged(m_progressValue);
    }
    void emitProgress() {
        GuardLocker locker(m_guard);
        emit q->progressValueChanged(m_progressValue);
    }
    void emitDone() {
        QTC_CHECK(m_progressValue == m_root->taskCount());
        GuardLocker locker(m_guard);
        emit q->done();
    }
    void emitError() {
        QTC_CHECK(m_progressValue == m_root->taskCount());
        GuardLocker locker(m_guard);
        emit q->errorOccurred();
    }
    QList<TreeStorageBase> addStorages(const QList<TreeStorageBase> &storages) {
        QList<TreeStorageBase> addedStorages;
        for (const TreeStorageBase &storage : storages) {
            QTC_ASSERT(!m_storages.contains(storage), qWarning("Can't add the same storage into "
                       "one TaskTree twice, skipping..."); continue);
            addedStorages << storage;
            m_storages << storage;
        }
        return addedStorages;
    }
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

class StorageActivator
{
public:
    StorageActivator(TaskContainer *container)
        : m_container(container) { activateStorages(m_container); }
    ~StorageActivator() { deactivateStorages(m_container); }

private:
    static void activateStorages(TaskContainer *container)
    {
        QTC_ASSERT(container && container->isRunning(), return);
        const TaskContainer::ConstData &constData = container->m_constData;
        if (constData.m_parentContainer)
            activateStorages(constData.m_parentContainer);
        for (int i = 0; i < constData.m_storageList.size(); ++i)
            constData.m_storageList[i].activateStorage(container->m_runtimeData->m_storageIdList.value(i));
    }
    static void deactivateStorages(TaskContainer *container)
    {
        QTC_ASSERT(container && container->isRunning(), return);
        const TaskContainer::ConstData &constData = container->m_constData;
        for (int i = constData.m_storageList.size() - 1; i >= 0; --i) // iterate in reverse order
            constData.m_storageList[i].activateStorage(0);
        if (constData.m_parentContainer)
            deactivateStorages(constData.m_parentContainer);
    }
    TaskContainer *m_container = nullptr;
};

template <typename Handler, typename ...Args,
          typename ReturnType = typename std::invoke_result_t<Handler, Args...>>
ReturnType invokeHandler(TaskContainer *container, Handler &&handler, Args &&...args)
{
    StorageActivator activator(container);
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
                                    TaskContainer *parentContainer, TaskContainer *thisContainer)
    : m_taskTreePrivate(taskTreePrivate)
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
        m_constData.m_taskTreePrivate->callDoneHandler(storage, storageId);
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
    if (m_constData.m_children.isEmpty() && startAction == TaskAction::Continue)
        startAction = TaskAction::StopWithDone;
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
    const int childCount = m_constData.m_children.size();
    for (int i = nextChild; i < childCount; ++i) {
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
    const bool shouldStop
        = (m_constData.m_workflowPolicy == WorkflowPolicy::StopOnDone && success)
          | (m_constData.m_workflowPolicy == WorkflowPolicy::StopOnError && !success);
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
    m_runtimeData.reset();
}

TaskAction TaskNode::start()
{
    QTC_CHECK(!isRunning());
    if (!isTask())
        return m_container.start();

    m_task.reset(m_taskHandler.m_createHandler());
    const TaskAction startAction = invokeHandler(parentContainer(), m_taskHandler.m_setupHandler,
                                                 *m_task.get());
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
        QTC_ASSERT(parentContainer()->isRunning(), return);
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
    \class Utils::TaskTree

    \brief The TaskTree class is responsible for running async task tree structure defined in a
           declarative way.

    The Tasking namespace (similar to Layouting) is designer for building declarative task
    tree structure. The examples of tasks that can be used inside TaskTree are e.g. QtcProcess,
    FileTransfer, AsyncTask<>. It's extensible, so any possible asynchronous task may be
    integrated and used inside TaskTree. TaskTree enables to form sophisticated mixtures of
    parallel or sequential flow of tasks in tree form.

    The TaskTree consist of Group root element. The Group can have nested Group elements.
    The Group may also contain any number of tasks, e.g. Process, FileTransfer,
    AsyncTask<ReturnType>.

    Each Group can contain various other elements describing the processing flow.

    The execute mode elements of a Group specify how direct children of a Group will be executed.
    The "sequential" element of a Group means all tasks in a group will be executed in chain,
    so after the previous task finished, the next will be started. This is the default Group
    behavior. The "parallel" element of a Group means that all tasks in a Group will be started
    simultaneously. When having nested Groups hierarchy, we may mix execute modes freely
    and each Group will be executed according to its own execute mode.
    The "sequential" mode may be very useful in cases when result data from one task is need to
    be passed as an input data to the other task - sequential mode guarantees that the next
    task will be started only after the previous task has already finished.

    There are many possible "workflow" behaviors for the Group. E.g. "stopOnError",
    the default Group workflow behavior, means that whenever any direct child of a Group
    finished with error, we immediately stop processing other tasks in this group
    (in parallel case) by canceling them and immediately finish the Group with error.

    The user of TaskTree specifies how to setup his tasks (by providing TaskSetupHandlers)
    and how to collect output data from the finished tasks (by providing TaskEndHandlers).
    The user don't need to create tasks manually - TaskTree will create them when it's needed
    and destroy when they are not used anymore.

    Whenever a Group elemenent is being started, the Group's OnGroupSetup handler is being called.
    Just after the handler finishes, all Group's children are executed (either in parallel or
    in sequence). When all Group's children finished, one of Group's OnGroupDone or OnGroupError
    is being executed, depending on results of children execution and Group's workflow policy.
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
    delete d;
}

void TaskTree::setupRoot(const Tasking::Group &root)
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

void TaskTree::setupStorageHandler(const Tasking::TreeStorageBase &storage,
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

TaskTreeAdapter::TaskTreeAdapter()
{
    connect(task(), &TaskTree::done, this, [this] { emit done(true); });
    connect(task(), &TaskTree::errorOccurred, this, [this] { emit done(false); });
}

void TaskTreeAdapter::start()
{
    task()->start();
}

} // namespace Utils
