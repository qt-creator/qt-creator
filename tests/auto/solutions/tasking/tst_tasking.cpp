// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <solutions/tasking/barrier.h>

#include <utils/async.h>

#include <QtTest>

#include <QDeadlineTimer>

using namespace std::literals::chrono_literals;

using namespace Utils;
using namespace Tasking;

using TestTask = Async<void>;
using Test = AsyncTask<void>;

enum class Handler {
    Setup,
    Done,
    Error,
    GroupSetup,
    GroupDone,
    GroupError,
    Sync,
    BarrierAdvance,
};

using Log = QList<QPair<int, Handler>>;

struct CustomStorage
{
    CustomStorage() { ++s_count; }
    ~CustomStorage() { --s_count; }
    Log m_log;
    static int instanceCount() { return s_count; }
private:
    static int s_count;
};

int CustomStorage::s_count = 0;
static const char s_taskIdProperty[] = "__taskId";

static FutureSynchronizer *s_futureSynchronizer = nullptr;

enum class OnDone { Success, Failure };

struct TestData {
    TreeStorage<CustomStorage> storage;
    Group root;
    Log expectedLog;
    int taskCount = 0;
    OnDone onDone = OnDone::Success;
};

class tst_Tasking : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void validConstructs(); // compile test
    void testTree_data();
    void testTree();
    void storageOperators();
    void storageDestructor();

    void cleanupTestCase();
};

void tst_Tasking::initTestCase()
{
    s_futureSynchronizer = new FutureSynchronizer;
}

void tst_Tasking::cleanupTestCase()
{
    delete s_futureSynchronizer;
    s_futureSynchronizer = nullptr;
}

void tst_Tasking::validConstructs()
{
    const Group task {
        parallel,
        Test([](TestTask &) {}, [](const TestTask &) {}),
        Test([](TestTask &) {}, [](const TestTask &) {}),
        Test([](TestTask &) {}, [](const TestTask &) {})
    };

    const Group group1 {
        task
    };

    const Group group2 {
        parallel,
        Group {
            parallel,
            Test([](TestTask &) {}, [](const TestTask &) {}),
            Group {
                parallel,
                Test([](TestTask &) {}, [](const TestTask &) {}),
                Group {
                    parallel,
                    Test([](TestTask &) {}, [](const TestTask &) {})
                }
            },
            Group {
                parallel,
                Test([](TestTask &) {}, [](const TestTask &) {}),
                onGroupDone([] {})
            }
        },
        task,
        onGroupDone([] {}),
        onGroupError([] {})
    };

    const auto setupHandler = [](TestTask &) {};
    const auto doneHandler = [](const TestTask &) {};
    const auto errorHandler = [](const TestTask &) {};

    // Not fluent interface

    const Group task2 {
        parallel,
        Test(setupHandler),
        Test(setupHandler, doneHandler),
        Test(setupHandler, doneHandler, errorHandler),
        // need to explicitly pass empty handler for done
        Test(setupHandler, {}, errorHandler)
    };

    // Fluent interface

    const Group fluent {
        parallel,
        Test().onSetup(setupHandler),
        Test().onSetup(setupHandler).onDone(doneHandler),
        Test().onSetup(setupHandler).onDone(doneHandler).onError(errorHandler),
        // possible to skip the empty done
        Test().onSetup(setupHandler).onError(errorHandler),
        // possible to set handlers in a different order
        Test().onError(errorHandler).onDone(doneHandler).onSetup(setupHandler),
    };


    // When turning each of below blocks on, you should see the specific compiler error message.

#if 0
    {
        // "Sync element: The synchronous function has to return void or bool."
        const auto setupSync = [] { return 3; };
        const Sync sync(setupSync);
    }
#endif

#if 0
    {
        // "Sync element: The synchronous function can't take any arguments."
        const auto setupSync = [](int) { };
        const Sync sync(setupSync);
    }
#endif

#if 0
    {
        // "Sync element: The synchronous function can't take any arguments."
        const auto setupSync = [](int) { return true; };
        const Sync sync(setupSync);
    }
#endif
}

static void runTask(QPromise<void> &promise, bool success, std::chrono::milliseconds sleep)
{
    QDeadlineTimer deadline(sleep);
    while (!deadline.hasExpired()) {
        QThread::msleep(1);
        if (promise.isCanceled())
            return;
    }
    if (!success)
        promise.future().cancel();
}

static void reportAndSleep(QPromise<bool> &promise)
{
    promise.addResult(false);
    QThread::msleep(5);
};

template <typename SharedBarrierType>
auto setupBarrierAdvance(const TreeStorage<CustomStorage> &storage,
                         const SharedBarrierType &barrier, int taskId)
{
    return [storage, barrier, taskId](Async<bool> &async) {
        async.setFutureSynchronizer(s_futureSynchronizer);
        async.setConcurrentCallData(reportAndSleep);
        async.setProperty(s_taskIdProperty, taskId);
        storage->m_log.append({taskId, Handler::Setup});

        CustomStorage *currentStorage = storage.activeStorage();
        Barrier *sharedBarrier = barrier->barrier();
        QObject::connect(&async, &TestTask::resultReadyAt, sharedBarrier,
                         [currentStorage, sharedBarrier, taskId](int index) {
            Q_UNUSED(index)
            currentStorage->m_log.append({taskId, Handler::BarrierAdvance});
            sharedBarrier->advance();
        });
    };
}

void tst_Tasking::testTree_data()
{
    QTest::addColumn<TestData>("testData");

    TreeStorage<CustomStorage> storage;

    const auto setupTaskHelper = [storage](TestTask &task, int taskId, bool success = true,
                                           std::chrono::milliseconds sleep = 0ms) {
        task.setFutureSynchronizer(s_futureSynchronizer);
        task.setConcurrentCallData(runTask, success, sleep);
        task.setProperty(s_taskIdProperty, taskId);
        storage->m_log.append({taskId, Handler::Setup});
    };
    const auto setupTask = [setupTaskHelper](int taskId) {
        return [=](TestTask &task) { setupTaskHelper(task, taskId); };
    };
    const auto setupFailingTask = [setupTaskHelper](int taskId) {
        return [=](TestTask &task) { setupTaskHelper(task, taskId, false); };
    };
    const auto setupSleepingTask = [setupTaskHelper](int taskId, bool success,
                                                     std::chrono::milliseconds sleep) {
        return [=](TestTask &task) { setupTaskHelper(task, taskId, success, sleep); };
    };
    const auto setupDynamicTask = [setupTaskHelper](int taskId, TaskAction action) {
        return [=](TestTask &task) {
            setupTaskHelper(task, taskId);
            return action;
        };
    };
    const auto logDone = [storage](const TestTask &task) {
        storage->m_log.append({task.property(s_taskIdProperty).toInt(), Handler::Done});
    };
    const auto logError = [storage](const TestTask &task) {
        storage->m_log.append({task.property(s_taskIdProperty).toInt(), Handler::Error});
    };
    const auto groupSetup = [storage](int taskId) {
        return [=] { storage->m_log.append({taskId, Handler::GroupSetup}); };
    };
    const auto groupDone = [storage](int taskId) {
        return [=] { storage->m_log.append({taskId, Handler::GroupDone}); };
    };
    const auto groupError = [storage](int taskId) {
        return [=] { storage->m_log.append({taskId, Handler::GroupError}); };
    };
    const auto setupSync = [storage](int taskId) {
        return [=] { storage->m_log.append({taskId, Handler::Sync}); };
    };
    const auto setupSyncWithReturn = [storage](int taskId, bool success) {
        return [=] { storage->m_log.append({taskId, Handler::Sync}); return success; };
    };

    const auto constructSimpleSequence = [=](WorkflowPolicy policy) {
        return Group {
            Storage(storage),
            workflowPolicy(policy),
            Test(setupTask(1), logDone),
            Test(setupFailingTask(2), logDone, logError),
            Test(setupTask(3), logDone),
            onGroupDone(groupDone(0)),
            onGroupError(groupError(0))
        };
    };
    const auto constructDynamicHierarchy = [=](TaskAction taskAction) {
        return Group {
            Storage(storage),
            Group {
                Test(setupTask(1), logDone)
            },
            Group {
                onGroupSetup([=] { return taskAction; }),
                Test(setupTask(2), logDone),
                Test(setupTask(3), logDone),
                Test(setupTask(4), logDone)
            },
            onGroupDone(groupDone(0)),
            onGroupError(groupError(0))
        };
    };

    {
        const Group root1 {
            Storage(storage),
            onGroupDone(groupDone(0)),
            onGroupError(groupError(0))
        };
        const Group root2 {
            Storage(storage),
            onGroupSetup([] { return TaskAction::Continue; }),
            onGroupDone(groupDone(0)),
            onGroupError(groupError(0))
        };
        const Group root3 {
            Storage(storage),
            onGroupSetup([] { return TaskAction::StopWithDone; }),
            onGroupDone(groupDone(0)),
            onGroupError(groupError(0))
        };
        const Group root4 {
            Storage(storage),
            onGroupSetup([] { return TaskAction::StopWithError; }),
            onGroupDone(groupDone(0)),
            onGroupError(groupError(0))
        };
        const Log logDone {{0, Handler::GroupDone}};
        const Log logError {{0, Handler::GroupError}};
        QTest::newRow("Empty") << TestData{storage, root1, logDone, 0, OnDone::Success};
        QTest::newRow("EmptyContinue") << TestData{storage, root2, logDone, 0, OnDone::Success};
        QTest::newRow("EmptyDone") << TestData{storage, root3, logDone, 0, OnDone::Success};
        QTest::newRow("EmptyError") << TestData{storage, root4, logError, 0, OnDone::Failure};
    }

    {
        const Group root {
            Storage(storage),
            Test(setupDynamicTask(1, TaskAction::StopWithDone), logDone, logError),
            Test(setupDynamicTask(2, TaskAction::StopWithDone), logDone, logError)
        };
        const Log log {{1, Handler::Setup}, {2, Handler::Setup}};
        QTest::newRow("DynamicTaskDone") << TestData{storage, root, log, 2, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            Test(setupDynamicTask(1, TaskAction::StopWithError), logDone, logError),
            Test(setupDynamicTask(2, TaskAction::StopWithError), logDone, logError)
        };
        const Log log {{1, Handler::Setup}};
        QTest::newRow("DynamicTaskError") << TestData{storage, root, log, 2, OnDone::Failure};
    }

    {
        const Group root {
            Storage(storage),
            Test(setupDynamicTask(1, TaskAction::Continue), logDone, logError),
            Test(setupDynamicTask(2, TaskAction::Continue), logDone, logError),
            Test(setupDynamicTask(3, TaskAction::StopWithError), logDone, logError),
            Test(setupDynamicTask(4, TaskAction::Continue), logDone, logError)
        };
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::Setup}
        };
        QTest::newRow("DynamicMixed") << TestData{storage, root, log, 4, OnDone::Failure};
    }

    {
        const Group root {
            parallel,
            Storage(storage),
            Test(setupDynamicTask(1, TaskAction::Continue), logDone, logError),
            Test(setupDynamicTask(2, TaskAction::Continue), logDone, logError),
            Test(setupDynamicTask(3, TaskAction::StopWithError), logDone, logError),
            Test(setupDynamicTask(4, TaskAction::Continue), logDone, logError)
        };
        const Log log {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Error},
            {2, Handler::Error}
        };
        QTest::newRow("DynamicParallel") << TestData{storage, root, log, 4, OnDone::Failure};
    }

    {
        const Group root {
            parallel,
            Storage(storage),
            Test(setupDynamicTask(1, TaskAction::Continue), logDone, logError),
            Test(setupDynamicTask(2, TaskAction::Continue), logDone, logError),
            Group {
                Test(setupDynamicTask(3, TaskAction::StopWithError), logDone, logError)
            },
            Test(setupDynamicTask(4, TaskAction::Continue), logDone, logError)
        };
        const Log log {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Error},
            {2, Handler::Error}
        };
        QTest::newRow("DynamicParallelGroup") << TestData{storage, root, log, 4, OnDone::Failure};
    }

    {
        const Group root {
            parallel,
            Storage(storage),
            Test(setupDynamicTask(1, TaskAction::Continue), logDone, logError),
            Test(setupDynamicTask(2, TaskAction::Continue), logDone, logError),
            Group {
                onGroupSetup([storage] {
                    storage->m_log.append({0, Handler::GroupSetup});
                    return TaskAction::StopWithError;
                }),
                Test(setupDynamicTask(3, TaskAction::Continue), logDone, logError)
            },
            Test(setupDynamicTask(4, TaskAction::Continue), logDone, logError)
        };
        const Log log {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {0, Handler::GroupSetup},
            {1, Handler::Error},
            {2, Handler::Error}
        };
        QTest::newRow("DynamicParallelGroupSetup")
            << TestData{storage, root, log, 4, OnDone::Failure};
    }

    {
        const Group root {
            Storage(storage),
            Group {
                Group {
                    Group {
                        Group {
                            Group {
                                Test(setupTask(5), logDone, logError),
                                onGroupSetup(groupSetup(5)),
                                onGroupDone(groupDone(5))
                            },
                            onGroupSetup(groupSetup(4)),
                            onGroupDone(groupDone(4))
                        },
                        onGroupSetup(groupSetup(3)),
                        onGroupDone(groupDone(3))
                    },
                    onGroupSetup(groupSetup(2)),
                    onGroupDone(groupDone(2))
                },
                onGroupSetup(groupSetup(1)),
                onGroupDone(groupDone(1))
            },
            onGroupDone(groupDone(0))
        };
        const Log log {
            {1, Handler::GroupSetup},
            {2, Handler::GroupSetup},
            {3, Handler::GroupSetup},
            {4, Handler::GroupSetup},
            {5, Handler::GroupSetup},
            {5, Handler::Setup},
            {5, Handler::Done},
            {5, Handler::GroupDone},
            {4, Handler::GroupDone},
            {3, Handler::GroupDone},
            {2, Handler::GroupDone},
            {1, Handler::GroupDone},
            {0, Handler::GroupDone}
        };
        QTest::newRow("Nested") << TestData{storage, root, log, 1, OnDone::Success};
    }

    {
        const auto logDoneAnonymously = [=](const TestTask &) {
            storage->m_log.append({0, Handler::Done});
        };
        const Group root {
            Storage(storage),
            parallel,
            Test(setupTask(1), logDoneAnonymously),
            Test(setupTask(2), logDoneAnonymously),
            Test(setupTask(3), logDoneAnonymously),
            Test(setupTask(4), logDoneAnonymously),
            Test(setupTask(5), logDoneAnonymously),
            onGroupDone(groupDone(0))
        };
        const Log log {
            {1, Handler::Setup}, // Setup order is determined in parallel mode
            {2, Handler::Setup},
            {3, Handler::Setup},
            {4, Handler::Setup},
            {5, Handler::Setup},
            {0, Handler::Done}, // Done order isn't determined in parallel mode
            {0, Handler::Done},
            {0, Handler::Done},
            {0, Handler::Done},
            {0, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("Parallel") << TestData{storage, root, log, 5, OnDone::Success};
    }

    {
        auto setupSubTree = [=](TaskTree &taskTree) {
            const Group nestedRoot {
                Storage(storage),
                Test(setupTask(2), logDone),
                Test(setupTask(3), logDone),
                Test(setupTask(4), logDone)
            };
            taskTree.setupRoot(nestedRoot);
            CustomStorage *activeStorage = storage.activeStorage();
            auto collectSubLog = [activeStorage](CustomStorage *subTreeStorage){
                activeStorage->m_log += subTreeStorage->m_log;
            };
            taskTree.onStorageDone(storage, collectSubLog);
        };
        const Group root1 {
            Storage(storage),
            Test(setupTask(1), logDone),
            Test(setupTask(2), logDone),
            Test(setupTask(3), logDone),
            Test(setupTask(4), logDone),
            Test(setupTask(5), logDone),
            onGroupDone(groupDone(0))
        };
        const Group root2 {
            Storage(storage),
            Group { Test(setupTask(1), logDone) },
            Group { Test(setupTask(2), logDone) },
            Group { Test(setupTask(3), logDone) },
            Group { Test(setupTask(4), logDone) },
            Group { Test(setupTask(5), logDone) },
            onGroupDone(groupDone(0))
        };
        const Group root3 {
            Storage(storage),
            Test(setupTask(1), logDone),
            TaskTreeTask(setupSubTree),
            Test(setupTask(5), logDone),
            onGroupDone(groupDone(0))
        };
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::Setup},
            {3, Handler::Done},
            {4, Handler::Setup},
            {4, Handler::Done},
            {5, Handler::Setup},
            {5, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("Sequential") << TestData{storage, root1, log, 5, OnDone::Success};
        QTest::newRow("SequentialEncapsulated") << TestData{storage, root2, log, 5, OnDone::Success};
        // We don't inspect subtrees, so taskCount is 3, not 5.
        QTest::newRow("SequentialSubTree") << TestData{storage, root3, log, 3, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            Group {
                Test(setupTask(1), logDone),
                Group {
                    Test(setupTask(2), logDone),
                    Group {
                        Test(setupTask(3), logDone),
                        Group {
                            Test(setupTask(4), logDone),
                            Group {
                                Test(setupTask(5), logDone),
                                onGroupDone(groupDone(5))
                            },
                            onGroupDone(groupDone(4))
                        },
                        onGroupDone(groupDone(3))
                    },
                    onGroupDone(groupDone(2))
                },
                onGroupDone(groupDone(1))
            },
            onGroupDone(groupDone(0))
        };
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::Setup},
            {3, Handler::Done},
            {4, Handler::Setup},
            {4, Handler::Done},
            {5, Handler::Setup},
            {5, Handler::Done},
            {5, Handler::GroupDone},
            {4, Handler::GroupDone},
            {3, Handler::GroupDone},
            {2, Handler::GroupDone},
            {1, Handler::GroupDone},
            {0, Handler::GroupDone}
        };
        QTest::newRow("SequentialNested") << TestData{storage, root, log, 5, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            Test(setupTask(1), logDone),
            Test(setupTask(2), logDone),
            Test(setupFailingTask(3), logDone, logError),
            Test(setupTask(4), logDone),
            Test(setupTask(5), logDone),
            onGroupDone(groupDone(0)),
            onGroupError(groupError(0))
        };
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::Setup},
            {3, Handler::Error},
            {0, Handler::GroupError}
        };
        QTest::newRow("SequentialError") << TestData{storage, root, log, 5, OnDone::Failure};
    }

    {
        const auto constructEmptyWorkflow = [=](WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                workflowPolicy(policy),
                onGroupDone(groupDone(0)),
                onGroupError(groupError(0))
            };
        };

        const Log log = {{0, Handler::GroupDone}};

        const Group root1 = constructEmptyWorkflow(WorkflowPolicy::StopOnError);
        QTest::newRow("EmptyStopOnError") << TestData{storage, root1, log, 0, OnDone::Success};

        const Group root2 = constructEmptyWorkflow(WorkflowPolicy::ContinueOnError);
        QTest::newRow("EmptyContinueOnError") << TestData{storage, root2, log, 0, OnDone::Success};

        const Group root3 = constructEmptyWorkflow(WorkflowPolicy::StopOnDone);
        QTest::newRow("EmptyStopOnDone") << TestData{storage, root3, log, 0, OnDone::Success};

        const Group root4 = constructEmptyWorkflow(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("EmptyContinueOnDone") << TestData{storage, root4, log, 0, OnDone::Success};

        const Group root5 = constructEmptyWorkflow(WorkflowPolicy::StopOnFinished);
        QTest::newRow("EmptyStopOnFinished") << TestData{storage, root5, log, 0, OnDone::Success};

        const Group root6 = constructEmptyWorkflow(WorkflowPolicy::Optional);
        QTest::newRow("EmptyOptional") << TestData{storage, root6, log, 0, OnDone::Success};
    }

    {
        const auto constructDoneWorkflow = [=](WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                workflowPolicy(policy),
                Test(setupTask(1), logDone, logError),
                onGroupDone(groupDone(0)),
                onGroupError(groupError(0))
            };
        };

        const Log log = {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupDone}
        };

        const Group root1 = constructDoneWorkflow(WorkflowPolicy::StopOnError);
        QTest::newRow("DoneStopOnError") << TestData{storage, root1, log, 1, OnDone::Success};

        const Group root2 = constructDoneWorkflow(WorkflowPolicy::ContinueOnError);
        QTest::newRow("DoneContinueOnError") << TestData{storage, root2, log, 1, OnDone::Success};

        const Group root3 = constructDoneWorkflow(WorkflowPolicy::StopOnDone);
        QTest::newRow("DoneStopOnDone") << TestData{storage, root3, log, 1, OnDone::Success};

        const Group root4 = constructDoneWorkflow(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("DoneContinueOnDone") << TestData{storage, root4, log, 1, OnDone::Success};

        const Group root5 = constructDoneWorkflow(WorkflowPolicy::StopOnFinished);
        QTest::newRow("DoneStopOnFinished") << TestData{storage, root5, log, 1, OnDone::Success};

        const Group root6 = constructDoneWorkflow(WorkflowPolicy::Optional);
        QTest::newRow("DoneOptional") << TestData{storage, root6, log, 1, OnDone::Success};
    }

    {
        const auto constructErrorWorkflow = [=](WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                workflowPolicy(policy),
                Test(setupFailingTask(1), logDone, logError),
                onGroupDone(groupDone(0)),
                onGroupError(groupError(0))
            };
        };

        const Log log = {
            {1, Handler::Setup},
            {1, Handler::Error},
            {0, Handler::GroupError}
        };

        const Log optionalLog = {
            {1, Handler::Setup},
            {1, Handler::Error},
            {0, Handler::GroupDone}
        };

        const Group root1 = constructErrorWorkflow(WorkflowPolicy::StopOnError);
        QTest::newRow("ErrorStopOnError") << TestData{storage, root1, log, 1, OnDone::Failure};

        const Group root2 = constructErrorWorkflow(WorkflowPolicy::ContinueOnError);
        QTest::newRow("ErrorContinueOnError") << TestData{storage, root2, log, 1, OnDone::Failure};

        const Group root3 = constructErrorWorkflow(WorkflowPolicy::StopOnDone);
        QTest::newRow("ErrorStopOnDone") << TestData{storage, root3, log, 1, OnDone::Failure};

        const Group root4 = constructErrorWorkflow(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("ErrorContinueOnDone") << TestData{storage, root4, log, 1, OnDone::Failure};

        const Group root5 = constructErrorWorkflow(WorkflowPolicy::StopOnFinished);
        QTest::newRow("ErrorStopOnFinished") << TestData{storage, root5, log, 1, OnDone::Failure};

        const Group root6 = constructErrorWorkflow(WorkflowPolicy::Optional);
        QTest::newRow("ErrorOptional") << TestData{storage, root6, optionalLog, 1, OnDone::Success};
    }

    {
        const Group root = constructSimpleSequence(WorkflowPolicy::StopOnError);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Error},
            {0, Handler::GroupError}
        };
        QTest::newRow("StopOnError") << TestData{storage, root, log, 3, OnDone::Failure};
    }

    {
        const Group root = constructSimpleSequence(WorkflowPolicy::ContinueOnError);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Error},
            {3, Handler::Setup},
            {3, Handler::Done},
            {0, Handler::GroupError}
        };
        QTest::newRow("ContinueOnError") << TestData{storage, root, log, 3, OnDone::Failure};
    }

    {
        const Group root = constructSimpleSequence(WorkflowPolicy::StopOnDone);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("StopOnDone") << TestData{storage, root, log, 3, OnDone::Success};
    }

    {
        const Group root = constructSimpleSequence(WorkflowPolicy::ContinueOnDone);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Error},
            {3, Handler::Setup},
            {3, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("ContinueOnDone") << TestData{storage, root, log, 3, OnDone::Success};
    }

    {
        const Group root = constructSimpleSequence(WorkflowPolicy::StopOnFinished);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("StopOnFinished") << TestData{storage, root, log, 3, OnDone::Success};
    }

    {
        const auto setupRoot = [=](bool firstSuccess, bool secondSuccess) {
            return Group {
                parallel,
                stopOnFinished,
                Storage(storage),
                Test(setupSleepingTask(1, firstSuccess, 1000ms), logDone, logError),
                Test(setupSleepingTask(2, secondSuccess, 5ms), logDone, logError),
                onGroupDone(groupDone(0)),
                onGroupError(groupError(0))
            };
        };

        const Group root1 = setupRoot(true, true);
        const Group root2 = setupRoot(true, false);
        const Group root3 = setupRoot(false, true);
        const Group root4 = setupRoot(false, false);

        const Log success {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {2, Handler::Done},
            {1, Handler::Error},
            {0, Handler::GroupDone}
        };
        const Log failure {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {2, Handler::Error},
            {1, Handler::Error},
            {0, Handler::GroupError}
        };

        QTest::newRow("StopOnFinished1") << TestData{storage, root1, success, 2, OnDone::Success};
        QTest::newRow("StopOnFinished2") << TestData{storage, root2, failure, 2, OnDone::Failure};
        QTest::newRow("StopOnFinished3") << TestData{storage, root3, success, 2, OnDone::Success};
        QTest::newRow("StopOnFinished4") << TestData{storage, root4, failure, 2, OnDone::Failure};
    }

    {
        const Group root {
            Storage(storage),
            optional,
            Test(setupFailingTask(1), logDone, logError),
            Test(setupFailingTask(2), logDone, logError),
            onGroupDone(groupDone(0)),
            onGroupError(groupError(0))
        };
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Error},
            {2, Handler::Setup},
            {2, Handler::Error},
            {0, Handler::GroupDone}
        };
        QTest::newRow("Optional") << TestData{storage, root, log, 2, OnDone::Success};
    }

    {
        const Group root = constructDynamicHierarchy(TaskAction::StopWithDone);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("DynamicSetupDone") << TestData{storage, root, log, 4, OnDone::Success};
    }

    {
        const Group root = constructDynamicHierarchy(TaskAction::StopWithError);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupError}
        };
        QTest::newRow("DynamicSetupError") << TestData{storage, root, log, 4, OnDone::Failure};
    }

    {
        const Group root = constructDynamicHierarchy(TaskAction::Continue);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::Setup},
            {3, Handler::Done},
            {4, Handler::Setup},
            {4, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("DynamicSetupContinue") << TestData{storage, root, log, 4, OnDone::Success};
    }

    {
        const Group root {
            parallelLimit(2),
            Storage(storage),
            Group {
                onGroupSetup(groupSetup(1)),
                Test(setupTask(1))
            },
            Group {
                onGroupSetup(groupSetup(2)),
                Test(setupTask(2))
            },
            Group {
                onGroupSetup(groupSetup(3)),
                Test(setupTask(3))
            },
            Group {
                onGroupSetup(groupSetup(4)),
                Test(setupTask(4))
            }
        };
        const Log log {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {3, Handler::GroupSetup},
            {3, Handler::Setup},
            {4, Handler::GroupSetup},
            {4, Handler::Setup}
        };
        QTest::newRow("NestedParallel") << TestData{storage, root, log, 4, OnDone::Success};
    }

    {
        const Group root {
            parallelLimit(2),
            Storage(storage),
            Group {
                onGroupSetup(groupSetup(1)),
                Test(setupTask(1))
            },
            Group {
                onGroupSetup(groupSetup(2)),
                Test(setupTask(2))
            },
            Group {
                onGroupSetup(groupSetup(3)),
                Test(setupDynamicTask(3, TaskAction::StopWithDone))
            },
            Group {
                onGroupSetup(groupSetup(4)),
                Test(setupTask(4))
            },
            Group {
                onGroupSetup(groupSetup(5)),
                Test(setupTask(5))
            }
        };
        const Log log {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {3, Handler::GroupSetup},
            {3, Handler::Setup},
            {4, Handler::GroupSetup},
            {4, Handler::Setup},
            {5, Handler::GroupSetup},
            {5, Handler::Setup}
        };
        QTest::newRow("NestedParallelDone") << TestData{storage, root, log, 5, OnDone::Success};
    }

    {
        const Group root1 {
            parallelLimit(2),
            Storage(storage),
            Group {
                onGroupSetup(groupSetup(1)),
                Test(setupTask(1))
            },
            Group {
                onGroupSetup(groupSetup(2)),
                Test(setupTask(2))
            },
            Group {
                onGroupSetup(groupSetup(3)),
                Test(setupDynamicTask(3, TaskAction::StopWithError))
            },
            Group {
                onGroupSetup(groupSetup(4)),
                Test(setupTask(4))
            },
            Group {
                onGroupSetup(groupSetup(5)),
                Test(setupTask(5))
            }
        };

        // Inside this test the task 2 should finish first, then synchonously:
        // - task 3 should exit setup with error
        // - task 1 should be stopped as a consequence of the error inside the group
        // - tasks 4 and 5 should be skipped
        const Group root2 {
            parallelLimit(2),
            Storage(storage),
            Group {
                onGroupSetup(groupSetup(1)),
                Test(setupSleepingTask(1, true, 10ms))
            },
            Group {
                onGroupSetup(groupSetup(2)),
                Test(setupTask(2))
            },
            Group {
                onGroupSetup(groupSetup(3)),
                Test(setupDynamicTask(3, TaskAction::StopWithError))
            },
            Group {
                onGroupSetup(groupSetup(4)),
                Test(setupTask(4))
            },
            Group {
                onGroupSetup(groupSetup(5)),
                Test(setupTask(5))
            }
        };

        // This test ensures that the task 1 doesn't invoke its done handler,
        // being ready while sleeping in the task's 2 done handler.
        // Inside this test the task 2 should finish first, then synchonously:
        // - task 3 should exit setup with error
        // - task 1 should be stopped as a consequence of error inside the group
        // - task 4 should be skipped
        // - the first child group of root should finish with error
        // - task 5 should be started (because of root's continueOnError policy)
        const Group root3 {
            continueOnError,
            Storage(storage),
            Group {
                parallelLimit(2),
                Group {
                    onGroupSetup(groupSetup(1)),
                    Test(setupSleepingTask(1, true, 20ms))
                },
                Group {
                    onGroupSetup(groupSetup(2)),
                    Test(setupSleepingTask(2, true, 10ms))
                },
                Group {
                    onGroupSetup(groupSetup(3)),
                    Test(setupDynamicTask(3, TaskAction::StopWithError))
                },
                Group {
                    onGroupSetup(groupSetup(4)),
                    Test(setupTask(4))
                }
            },
            Group {
                onGroupSetup(groupSetup(5)),
                Test(setupTask(5))
            }
        };
        const Log shortLog {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {3, Handler::GroupSetup},
            {3, Handler::Setup}
        };
        const Log longLog = shortLog + Log {{5, Handler::GroupSetup}, {5, Handler::Setup}};
        QTest::newRow("NestedParallelError1")
            << TestData{storage, root1, shortLog, 5, OnDone::Failure};
        QTest::newRow("NestedParallelError2")
            << TestData{storage, root2, shortLog, 5, OnDone::Failure};
        QTest::newRow("NestedParallelError3")
            << TestData{storage, root3, longLog, 5, OnDone::Failure};
    }

    {
        const Group root {
            parallelLimit(2),
            Storage(storage),
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(1)),
                Group {
                    parallel,
                    Test(setupTask(1))
                }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(2)),
                Group {
                    parallel,
                    Test(setupTask(2))
                }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(3)),
                Group {
                    parallel,
                    Test(setupTask(3))
                }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(4)),
                Group {
                    parallel,
                    Test(setupTask(4))
                }
            }
        };
        const Log log {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {3, Handler::GroupSetup},
            {3, Handler::Setup},
            {4, Handler::GroupSetup},
            {4, Handler::Setup}
        };
        QTest::newRow("DeeplyNestedParallel") << TestData{storage, root, log, 4, OnDone::Success};
    }

    {
        const Group root {
            parallelLimit(2),
            Storage(storage),
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(1)),
                Group { Test(setupTask(1)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(2)),
                Group { Test(setupTask(2)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(3)),
                Group { Test(setupDynamicTask(3, TaskAction::StopWithDone)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(4)),
                Group { Test(setupTask(4)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(5)),
                Group { Test(setupTask(5)) }
            }
        };
        const Log log {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {3, Handler::GroupSetup},
            {3, Handler::Setup},
            {4, Handler::GroupSetup},
            {4, Handler::Setup},
            {5, Handler::GroupSetup},
            {5, Handler::Setup}
        };
        QTest::newRow("DeeplyNestedParallelDone")
            << TestData{storage, root, log, 5, OnDone::Success};
    }

    {
        const Group root {
            parallelLimit(2),
            Storage(storage),
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(1)),
                Group { Test(setupTask(1)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(2)),
                Group { Test(setupTask(2)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(3)),
                Group { Test(setupDynamicTask(3, TaskAction::StopWithError)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(4)),
                Group { Test(setupTask(4)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                onGroupSetup(groupSetup(5)),
                Group { Test(setupTask(5)) }
            }
        };
        const Log log {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {3, Handler::GroupSetup},
            {3, Handler::Setup}
        };
        QTest::newRow("DeeplyNestedParallelError")
            << TestData{storage, root, log, 5, OnDone::Failure};
    }

    {
        const Group root {
            Storage(storage),
            Sync(setupSync(1)),
            Sync(setupSync(2)),
            Sync(setupSync(3)),
            Sync(setupSync(4)),
            Sync(setupSync(5))
        };
        const Log log {
            {1, Handler::Sync},
            {2, Handler::Sync},
            {3, Handler::Sync},
            {4, Handler::Sync},
            {5, Handler::Sync}
        };
        QTest::newRow("SyncSequential") << TestData{storage, root, log, 0, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            Sync(setupSyncWithReturn(1, true)),
            Sync(setupSyncWithReturn(2, true)),
            Sync(setupSyncWithReturn(3, true)),
            Sync(setupSyncWithReturn(4, true)),
            Sync(setupSyncWithReturn(5, true))
        };
        const Log log {
            {1, Handler::Sync},
            {2, Handler::Sync},
            {3, Handler::Sync},
            {4, Handler::Sync},
            {5, Handler::Sync}
        };
        QTest::newRow("SyncWithReturn") << TestData{storage, root, log, 0, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            parallel,
            Sync(setupSync(1)),
            Sync(setupSync(2)),
            Sync(setupSync(3)),
            Sync(setupSync(4)),
            Sync(setupSync(5))
        };
        const Log log {
            {1, Handler::Sync},
            {2, Handler::Sync},
            {3, Handler::Sync},
            {4, Handler::Sync},
            {5, Handler::Sync}
        };
        QTest::newRow("SyncParallel") << TestData{storage, root, log, 0, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            parallel,
            Sync(setupSync(1)),
            Sync(setupSync(2)),
            Sync(setupSyncWithReturn(3, false)),
            Sync(setupSync(4)),
            Sync(setupSync(5))
        };
        const Log log {
            {1, Handler::Sync},
            {2, Handler::Sync},
            {3, Handler::Sync}
        };
        QTest::newRow("SyncError") << TestData{storage, root, log, 0, OnDone::Failure};
    }

    {
        const Group root {
            Storage(storage),
            Sync(setupSync(1)),
            Test(setupTask(2)),
            Sync(setupSync(3)),
            Test(setupTask(4)),
            Sync(setupSync(5)),
            onGroupDone(groupDone(0))
        };
        const Log log {
            {1, Handler::Sync},
            {2, Handler::Setup},
            {3, Handler::Sync},
            {4, Handler::Setup},
            {5, Handler::Sync},
            {0, Handler::GroupDone}
        };
        QTest::newRow("SyncAndAsync") << TestData{storage, root, log, 2, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            Sync(setupSync(1)),
            Test(setupTask(2)),
            Sync(setupSyncWithReturn(3, false)),
            Test(setupTask(4)),
            Sync(setupSync(5)),
            onGroupError(groupError(0))
        };
        const Log log {
            {1, Handler::Sync},
            {2, Handler::Setup},
            {3, Handler::Sync},
            {0, Handler::GroupError}
        };
        QTest::newRow("SyncAndAsyncError") << TestData{storage, root, log, 2, OnDone::Failure};
    }

    {
        SingleBarrier barrier;

        // Test that barrier advance, triggered from inside the task described by
        // setupBarrierAdvance, placed BEFORE the group containing the waitFor() element
        // in the tree order, works OK in SEQUENTIAL mode.
        const Group root1 {
            Storage(storage),
            Storage(barrier),
            sequential,
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 1)),
            Group {
                onGroupSetup(groupSetup(2)),
                WaitForBarrierTask(barrier),
                Test(setupTask(2)),
                Test(setupTask(3))
            }
        };
        const Log log1 {
            {1, Handler::Setup},
            {1, Handler::BarrierAdvance},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {3, Handler::Setup}
        };

        // Test that barrier advance, triggered from inside the task described by
        // setupTaskWithCondition, placed BEFORE the group containing the waitFor() element
        // in the tree order, works OK in PARALLEL mode.
        const Group root2 {
            Storage(storage),
            Storage(barrier),
            parallel,
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 1)),
            Group {
                onGroupSetup(groupSetup(2)),
                WaitForBarrierTask(barrier),
                Test(setupTask(2)),
                Test(setupTask(3))
            }
        };
        const Log log2 {
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {1, Handler::BarrierAdvance},
            {2, Handler::Setup},
            {3, Handler::Setup}
        };

        // Test that barrier advance, triggered from inside the task described by
        // setupTaskWithCondition, placed AFTER the group containing the waitFor() element
        // in the tree order, works OK in PARALLEL mode.
        //
        // Notice: This won't work in SEQUENTIAL mode, since the advancing barrier, placed after the
        // group containing the WaitFor element, has no chance to be started in SEQUENTIAL mode,
        // as in SEQUENTIAL mode the next task may only be started after the previous one finished.
        // In this case, the previous task (Group element) awaits for the barrier's advance to
        // come from the not yet started next task, causing a deadlock.
        // The minimal requirement for this scenario to succeed is to set parallelLimit(2) or more.
        const Group root3 {
            Storage(storage),
            Storage(barrier),
            parallel,
            Group {
                onGroupSetup(groupSetup(2)),
                WaitForBarrierTask(barrier),
                Test(setupTask(2)),
                Test(setupTask(3))
            },
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 1))
        };
        const Log log3 {
            {2, Handler::GroupSetup},
            {1, Handler::Setup},
            {1, Handler::BarrierAdvance},
            {2, Handler::Setup},
            {3, Handler::Setup}
        };

        // Test that barrier advance, triggered from inside the task described by
        // setupBarrierAdvance, placed BEFORE the groups containing the waitFor() element
        // in the tree order, wakes both waitFor tasks.
        const Group root4 {
            Storage(storage),
            Storage(barrier),
            parallel,
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 1)),
            Group {
                onGroupSetup(groupSetup(2)),
                WaitForBarrierTask(barrier),
                Test(setupTask(4))
            },
            Group {
                onGroupSetup(groupSetup(3)),
                WaitForBarrierTask(barrier),
                Test(setupTask(5))
            }
        };
        const Log log4 {
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {3, Handler::GroupSetup},
            {1, Handler::BarrierAdvance},
            {4, Handler::Setup},
            {5, Handler::Setup}
        };

        // Test two separate single barriers.

        SingleBarrier barrier2;

        const Group root5 {
            Storage(storage),
            Storage(barrier),
            Storage(barrier2),
            parallel,
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 0)),
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier2, 0)),
            Group {
                Group {
                    parallel,
                    onGroupSetup(groupSetup(1)),
                    WaitForBarrierTask(barrier),
                    WaitForBarrierTask(barrier2)
                },
                Test(setupTask(2))
            },
        };
        const Log log5 {
            {0, Handler::Setup},
            {0, Handler::Setup},
            {1, Handler::GroupSetup},
            {0, Handler::BarrierAdvance},
            {0, Handler::BarrierAdvance},
            {2, Handler::Setup}
        };

        // Notice the different log order for each scenario.
        QTest::newRow("BarrierSequential")
            << TestData{storage, root1, log1, 4, OnDone::Success};
        QTest::newRow("BarrierParallelAdvanceFirst")
            << TestData{storage, root2, log2, 4, OnDone::Success};
        QTest::newRow("BarrierParallelWaitForFirst")
            << TestData{storage, root3, log3, 4, OnDone::Success};
        QTest::newRow("BarrierParallelMultiWaitFor")
            << TestData{storage, root4, log4, 5, OnDone::Success};
        QTest::newRow("BarrierParallelTwoSingleBarriers")
            << TestData{storage, root5, log5, 5, OnDone::Success};
    }

    {
        MultiBarrier<2> barrier;

        // Test that multi barrier advance, triggered from inside the tasks described by
        // setupBarrierAdvance, placed BEFORE the group containing the waitFor() element
        // in the tree order, works OK in SEQUENTIAL mode.
        const Group root1 {
            Storage(storage),
            Storage(barrier),
            sequential,
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 1)),
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 2)),
            Group {
                onGroupSetup(groupSetup(2)),
                WaitForBarrierTask(barrier),
                Test(setupTask(2)),
                Test(setupTask(3))
            }
        };
        const Log log1 {
            {1, Handler::Setup},
            {1, Handler::BarrierAdvance},
            {2, Handler::Setup},
            {2, Handler::BarrierAdvance},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {3, Handler::Setup}
        };

        // Test that multi barrier advance, triggered from inside the tasks described by
        // setupBarrierAdvance, placed BEFORE the group containing the waitFor() element
        // in the tree order, works OK in PARALLEL mode.
        const Group root2 {
            Storage(storage),
            Storage(barrier),
            parallel,
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 0)),
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 0)),
            Group {
                onGroupSetup(groupSetup(2)),
                WaitForBarrierTask(barrier),
                Test(setupTask(2)),
                Test(setupTask(3))
            }
        };
        const Log log2 {
            {0, Handler::Setup},
            {0, Handler::Setup},
            {2, Handler::GroupSetup},
            {0, Handler::BarrierAdvance}, // Barrier advances may come in different order in
            {0, Handler::BarrierAdvance}, // parallel mode, that's why id = 0 (same for both).
            {2, Handler::Setup},
            {3, Handler::Setup}
        };

        // Test that multi barrier advance, triggered from inside the tasks described by
        // setupBarrierAdvance, placed AFTER the group containing the waitFor() element
        // in the tree order, works OK in PARALLEL mode.
        //
        // Notice: This won't work in SEQUENTIAL mode, since the advancing barriers, placed after
        // the group containing the WaitFor element, has no chance to be started in SEQUENTIAL mode,
        // as in SEQUENTIAL mode the next task may only be started after the previous one finished.
        // In this case, the previous task (Group element) awaits for the barrier's advance to
        // come from the not yet started next task, causing a deadlock.
        // The minimal requirement for this scenario to succeed is to set parallelLimit(2) or more.
        const Group root3 {
            Storage(storage),
            Storage(barrier),
            parallel,
            Group {
                onGroupSetup(groupSetup(2)),
                WaitForBarrierTask(barrier),
                Test(setupTask(2)),
                Test(setupTask(3))
            },
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 0)),
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 0))
        };
        const Log log3 {
            {2, Handler::GroupSetup},
            {0, Handler::Setup},
            {0, Handler::Setup},
            {0, Handler::BarrierAdvance}, // Barrier advances may come in different order in
            {0, Handler::BarrierAdvance}, // parallel mode, that's why id = 0 (same for both).
            {2, Handler::Setup},
            {3, Handler::Setup}
        };

        // Test that multi barrier advance, triggered from inside the task described by
        // setupBarrierAdvance, placed BEFORE the groups containing the waitFor() element
        // in the tree order, wakes both waitFor tasks.
        const Group root4 {
            Storage(storage),
            Storage(barrier),
            parallel,
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 0)),
            AsyncTask<bool>(setupBarrierAdvance(storage, barrier, 0)),
            Group {
                onGroupSetup(groupSetup(2)),
                WaitForBarrierTask(barrier),
                Test(setupTask(4))
            },
            Group {
                onGroupSetup(groupSetup(3)),
                WaitForBarrierTask(barrier),
                Test(setupTask(5))
            }
        };
        const Log log4 {
            {0, Handler::Setup},
            {0, Handler::Setup},
            {2, Handler::GroupSetup},
            {3, Handler::GroupSetup},
            {0, Handler::BarrierAdvance},
            {0, Handler::BarrierAdvance},
            {4, Handler::Setup},
            {5, Handler::Setup}
        };

        // Notice the different log order for each scenario.
        QTest::newRow("MultiBarrierSequential")
            << TestData{storage, root1, log1, 5, OnDone::Success};
        QTest::newRow("MultiBarrierParallelAdvanceFirst")
            << TestData{storage, root2, log2, 5, OnDone::Success};
        QTest::newRow("MultiBarrierParallelWaitForFirst")
            << TestData{storage, root3, log3, 5, OnDone::Success};
        QTest::newRow("MultiBarrierParallelMultiWaitFor")
            << TestData{storage, root4, log4, 6, OnDone::Success};
    }
}

void tst_Tasking::testTree()
{
    QFETCH(TestData, testData);

    TaskTree taskTree(testData.root);
    QCOMPARE(taskTree.taskCount(), testData.taskCount);
    Log actualLog;
    const auto collectLog = [&actualLog](CustomStorage *storage) { actualLog = storage->m_log; };
    taskTree.onStorageDone(testData.storage, collectLog);
    const int result = taskTree.runBlocking(2000);
    QCOMPARE(taskTree.isRunning(), false);

    QCOMPARE(taskTree.progressValue(), testData.taskCount);
    QCOMPARE(actualLog, testData.expectedLog);
    QCOMPARE(CustomStorage::instanceCount(), 0);

    const bool expectSuccess = testData.onDone == OnDone::Success;
    QCOMPARE(result, expectSuccess);
}

void tst_Tasking::storageOperators()
{
    TreeStorageBase storage1 = TreeStorage<CustomStorage>();
    TreeStorageBase storage2 = TreeStorage<CustomStorage>();
    TreeStorageBase storage3 = storage1;

    QVERIFY(storage1 == storage3);
    QVERIFY(storage1 != storage2);
    QVERIFY(storage2 != storage3);
}

// This test checks whether a running task tree may be safely destructed.
// It also checks whether the destructor of a task tree deletes properly the storage created
// while starting the task tree. When running task tree is destructed, the storage done
// handler shouldn't be invoked.
void tst_Tasking::storageDestructor()
{
    bool setupCalled = false;
    const auto setupHandler = [&setupCalled](CustomStorage *) {
        setupCalled = true;
    };
    bool doneCalled = false;
    const auto doneHandler = [&doneCalled](CustomStorage *) {
        doneCalled = true;
    };
    QCOMPARE(CustomStorage::instanceCount(), 0);
    {
        TreeStorage<CustomStorage> storage;
        const auto setupSleepingTask = [](TestTask &task) {
            task.setFutureSynchronizer(s_futureSynchronizer);
            task.setConcurrentCallData(runTask, true, 1000ms);
        };
        const Group root {
            Storage(storage),
            Test(setupSleepingTask)
        };

        TaskTree taskTree(root);
        QCOMPARE(CustomStorage::instanceCount(), 0);
        taskTree.onStorageSetup(storage, setupHandler);
        taskTree.onStorageDone(storage, doneHandler);
        taskTree.start();
        QCOMPARE(CustomStorage::instanceCount(), 1);
        QThread::msleep(5); // Give the sleeping task a change to start
    }
    QCOMPARE(CustomStorage::instanceCount(), 0);
    QVERIFY(setupCalled);
    QVERIFY(!doneCalled);
}

QTEST_GUILESS_MAIN(tst_Tasking)

#include "tst_tasking.moc"
