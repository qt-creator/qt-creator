// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "tasktree.h"

#include "guard.h"
#include "qtcassert.h"

namespace Utils {
namespace Tasking {

bool TreeStorageBase::isValid() const
{
    return m_storageData && m_storageData->m_constructor && m_storageData->m_destructor;
}

size_t TreeStorageBase::hash(uint seed) const
{
    return size_t(m_storageData.get()) ^ seed;
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

int TreeStorageBase::createStorage()
{
    QTC_ASSERT(m_storageData->m_constructor, return 0); // TODO: add isValid()?
    QTC_ASSERT(m_storageData->m_destructor, return 0);
    QTC_ASSERT(m_storageData->m_activeStorage == 0, return 0); // TODO: should be allowed?
    const int newId = ++m_storageData->m_storageCounter;
    m_storageData->m_storageHash.insert(newId, m_storageData->m_constructor());
    return newId;
}

void TreeStorageBase::deleteStorage(int id)
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
void TreeStorageBase::activateStorage(int id)
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

Execute sequential(ExecuteMode::Sequential);
Execute parallel(ExecuteMode::Parallel);
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
        case Type::Mode:
            QTC_ASSERT(m_type == Type::Group,
                       qWarning("Mode may only be a child of Group, skipping..."); return);
            m_executeMode = child.m_executeMode; // TODO: Assert on redefinition?
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
            QTC_ASSERT(!child.m_groupHandler.m_dynamicSetupHandler
                       || !m_groupHandler.m_dynamicSetupHandler,
                       qWarning("Dynamic Setup Handler redefinition, overriding..."));
            if (child.m_groupHandler.m_setupHandler)
                m_groupHandler.m_setupHandler = child.m_groupHandler.m_setupHandler;
            if (child.m_groupHandler.m_doneHandler)
                m_groupHandler.m_doneHandler = child.m_groupHandler.m_doneHandler;
            if (child.m_groupHandler.m_errorHandler)
                m_groupHandler.m_errorHandler = child.m_groupHandler.m_errorHandler;
            if (child.m_groupHandler.m_dynamicSetupHandler)
                m_groupHandler.m_dynamicSetupHandler = child.m_groupHandler.m_dynamicSetupHandler;
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
    TaskContainer(TaskTreePrivate *taskTreePrivate, TaskContainer *parentContainer,
                  const TaskItem &task);
    ~TaskContainer();
    void start();
    int selectChildren(); // returns the skipped child count
    void stop();
    bool isRunning() const;
    int taskCount() const;
    void childDone(bool success);
    void invokeEndHandler(bool success, bool propagateToParent = true);
    void resetSuccessBit();
    void updateSuccessBit(bool success);

    void createStorages();
    void deleteStorages();
    void activateStorages();
    void deactivateStorages();

    TaskTreePrivate *m_taskTreePrivate = nullptr;
    TaskContainer *m_parentContainer = nullptr;
    const ExecuteMode m_executeMode = ExecuteMode::Parallel;
    WorkflowPolicy m_workflowPolicy = WorkflowPolicy::StopOnError;
    const TaskItem::GroupHandler m_groupHandler;
    QList<TreeStorageBase> m_storageList;
    QList<int> m_storageIdList;
    int m_taskCount = 0;
    GroupConfig m_groupConfig;
    QList<TaskNode *> m_children;
    QList<TaskNode *> m_selectedChildren;
    int m_currentIndex = -1;
    bool m_successBit = true;
};

class StorageActivator
{
public:
    StorageActivator(TaskContainer &container)
        : m_container(container)
    {
        m_container.activateStorages();
    }
    ~StorageActivator()
    {
        m_container.deactivateStorages();
    }
private:
    TaskContainer &m_container;
};

class TaskNode : public QObject
{
public:
    TaskNode(TaskTreePrivate *taskTreePrivate, TaskContainer *parentContainer,
             const TaskItem &task)
        : m_taskHandler(task.taskHandler())
        , m_container(taskTreePrivate, parentContainer, task)
    {
    }

    bool start();
    void stop();
    bool isRunning() const;
    bool isTask() const;
    int taskCount() const;

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

    TaskTree *q = nullptr;
    std::unique_ptr<TaskNode> m_root = nullptr;
    Guard m_guard;
    int m_progressValue = 0;
};

TaskContainer::TaskContainer(TaskTreePrivate *taskTreePrivate, TaskContainer *parentContainer,
                             const TaskItem &task)
    : m_taskTreePrivate(taskTreePrivate)
    , m_parentContainer(parentContainer)
    , m_executeMode(task.executeMode())
    , m_workflowPolicy(task.workflowPolicy())
    , m_groupHandler(task.groupHandler())
    , m_storageList(task.storageList())
{
    const QList<TaskItem> &children = task.children();
    for (const TaskItem &child : children) {
        TaskNode *node = new TaskNode(m_taskTreePrivate, this, child);
        m_children.append(node);
        m_taskCount += node->taskCount();
    }
}

TaskContainer::~TaskContainer()
{
    qDeleteAll(m_children);
    deleteStorages();
}

void TaskContainer::start()
{
    m_currentIndex = 0;
    m_groupConfig = {};
    m_selectedChildren.clear();

    createStorages();
    {
        StorageActivator activator(*this);
        if (m_groupHandler.m_setupHandler) {
            GuardLocker locker(m_taskTreePrivate->m_guard);
            m_groupHandler.m_setupHandler();
        }

        if (m_groupHandler.m_dynamicSetupHandler) {
            GuardLocker locker(m_taskTreePrivate->m_guard);
            m_groupConfig = m_groupHandler.m_dynamicSetupHandler();
        }
    }

    if (m_groupConfig.action == GroupAction::StopWithDone || m_groupConfig.action == GroupAction::StopWithError) {
        const bool success = m_groupConfig.action == GroupAction::StopWithDone;
        const int skippedTaskCount = taskCount();
        m_taskTreePrivate->advanceProgress(skippedTaskCount);
        invokeEndHandler(success);
        return;
    }

    const int skippedTaskCount = selectChildren();
    m_taskTreePrivate->advanceProgress(skippedTaskCount);

    if (m_selectedChildren.isEmpty()) {
        invokeEndHandler(true);
        return;
    }

    m_currentIndex = 0;
    resetSuccessBit();

    if (m_executeMode == ExecuteMode::Sequential) {
        m_selectedChildren.at(m_currentIndex)->start();
        return;
    }

    // Parallel case
    for (TaskNode *child : std::as_const(m_selectedChildren)) {
        if (!child->start())
            return;
    }
}

int TaskContainer::selectChildren()
{
    if (m_groupConfig.action != GroupAction::ContinueSelected) {
        m_selectedChildren = m_children;
        return 0;
    }
    m_selectedChildren.clear();
    int skippedTaskCount = 0;
    for (int i = 0; i < m_children.size(); ++i) {
        if (m_groupConfig.childrenToRun.contains(i))
            m_selectedChildren.append(m_children.at(i));
        else
            skippedTaskCount += m_children.at(i)->taskCount();
    }
    return skippedTaskCount;
}

void TaskContainer::stop()
{
    if (!isRunning())
        return;

    if (m_executeMode == ExecuteMode::Sequential) {
        int skippedTaskCount = 0;
        for (int i = m_currentIndex + 1; i < m_selectedChildren.size(); ++i)
            skippedTaskCount += m_selectedChildren.at(i)->taskCount();
        m_taskTreePrivate->advanceProgress(skippedTaskCount);
    }

    m_currentIndex = -1;
    for (TaskNode *child : std::as_const(m_selectedChildren))
        child->stop();
}

bool TaskContainer::isRunning() const
{
    return m_currentIndex >= 0;
}

int TaskContainer::taskCount() const
{
    return m_taskCount;
}

void TaskContainer::childDone(bool success)
{
    if ((m_workflowPolicy == WorkflowPolicy::StopOnDone && success)
            || (m_workflowPolicy == WorkflowPolicy::StopOnError && !success)) {
        stop();
        invokeEndHandler(success);
        return;
    }

    ++m_currentIndex;
    updateSuccessBit(success);

    if (m_currentIndex == m_selectedChildren.size()) {
        invokeEndHandler(m_successBit);
        return;
    }

    if (m_executeMode == ExecuteMode::Sequential)
        m_selectedChildren.at(m_currentIndex)->start();
}

void TaskContainer::invokeEndHandler(bool success, bool propagateToParent)
{
    m_currentIndex = -1;
    m_successBit = success;
    {
        StorageActivator activator(*this);
        if (success && m_groupHandler.m_doneHandler) {
            GuardLocker locker(m_taskTreePrivate->m_guard);
            m_groupHandler.m_doneHandler();
        } else if (!success && m_groupHandler.m_errorHandler) {
            GuardLocker locker(m_taskTreePrivate->m_guard);
            m_groupHandler.m_errorHandler();
        }
    }
    deleteStorages();

    if (!propagateToParent)
        return;

    if (m_parentContainer) {
        m_parentContainer->childDone(success);
        return;
    }
    if (success)
        m_taskTreePrivate->emitDone();
    else
        m_taskTreePrivate->emitError();
}

void TaskContainer::resetSuccessBit()
{
    if (m_selectedChildren.isEmpty())
        m_successBit = true;

    if (m_workflowPolicy == WorkflowPolicy::StopOnDone
            || m_workflowPolicy == WorkflowPolicy::ContinueOnDone) {
        m_successBit = false;
    } else {
        m_successBit = true;
    }
}

void TaskContainer::updateSuccessBit(bool success)
{
    if (m_workflowPolicy == WorkflowPolicy::Optional)
        return;
    if (m_workflowPolicy == WorkflowPolicy::StopOnDone
            || m_workflowPolicy == WorkflowPolicy::ContinueOnDone) {
        m_successBit = m_successBit || success;
    } else {
        m_successBit = m_successBit && success;
    }
}

void TaskContainer::createStorages()
{
    // TODO: Don't create new storage for already created storages with the same shared pointer.
    QTC_CHECK(m_storageIdList.isEmpty());

    for (int i = 0; i < m_storageList.size(); ++i)
        m_storageIdList << m_storageList[i].createStorage();
}

void TaskContainer::deleteStorages()
{
    // TODO: Do the opposite

    for (int i = 0; i < m_storageIdList.size(); ++i) // iterate in reverse order?
        m_storageList[i].deleteStorage(m_storageIdList.value(i));

    m_storageIdList.clear();
}

void TaskContainer::activateStorages()
{
    // TODO: check if the same shared storage was already activated. Don't activate recursively.

    if (m_parentContainer)
        m_parentContainer->activateStorages();

    for (int i = 0; i < m_storageList.size(); ++i)
        m_storageList[i].activateStorage(m_storageIdList.value(i));
}

void TaskContainer::deactivateStorages()
{
    // TODO: Do the opposite

    for (int i = 0; i < m_storageList.size(); ++i) // iterate in reverse order?
        m_storageList[i].activateStorage(0);

    if (m_parentContainer)
        m_parentContainer->deactivateStorages();
}

bool TaskNode::start()
{
    if (!isTask()) {
        m_container.start();
        return true;
    }
    m_task.reset(m_taskHandler.m_createHandler());
    {
        StorageActivator activator(m_container);
        GuardLocker locker(m_container.m_taskTreePrivate->m_guard);
        m_taskHandler.m_setupHandler(*m_task.get());
    }
    connect(m_task.get(), &TaskInterface::done, this, [this](bool success) {
        {
            StorageActivator activator(m_container);
            if (success && m_taskHandler.m_doneHandler) {
                GuardLocker locker(m_container.m_taskTreePrivate->m_guard);
                m_taskHandler.m_doneHandler(*m_task.get());
            } else if (!success && m_taskHandler.m_errorHandler) {
                GuardLocker locker(m_container.m_taskTreePrivate->m_guard);
                m_taskHandler.m_errorHandler(*m_task.get());
            }
        }
        m_container.m_taskTreePrivate->advanceProgress(1);

        m_task.release()->deleteLater();

        QTC_CHECK(m_container.m_parentContainer);
        m_container.m_parentContainer->childDone(success);
    });

    m_task->start();
    return m_task.get(); // In case of failed to start, done handler already released process
}

void TaskNode::stop()
{
    if (!isRunning())
        return;

    if (!m_task) {
        m_container.stop();
        m_container.invokeEndHandler(false, false);
        return;
    }

    // TODO: cancelHandler?
    // TODO: call TaskInterface::stop() ?
    if (m_taskHandler.m_errorHandler) {
        StorageActivator activator(m_container);
        GuardLocker locker(m_container.m_taskTreePrivate->m_guard);
        m_taskHandler.m_errorHandler(*m_task.get());
    }
    m_container.m_taskTreePrivate->advanceProgress(1);
    m_task.reset();
}

bool TaskNode::isRunning() const
{
    return m_task || m_container.isRunning();
}

bool TaskNode::isTask() const
{
    return m_taskHandler.m_createHandler && m_taskHandler.m_setupHandler;
}

int TaskNode::taskCount() const
{
    return isTask() ? 1 : m_container.taskCount();
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
    d->m_root.reset(new TaskNode(d, nullptr, root));
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
