// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tasking/barrier.h>

#include <QtTest>

using namespace Tasking;

using namespace std::chrono;
using namespace std::chrono_literals;

using TaskObject = milliseconds;
using TestTask = TimeoutTask;

namespace PrintableEnums {

Q_NAMESPACE

enum class Handler {
    Setup,
    Done,
    Error,
    GroupSetup,
    GroupDone,
    GroupError,
    Sync,
    BarrierAdvance,
    Timeout
};
Q_ENUM_NS(Handler);

enum class OnDone { Success, Failure };
Q_ENUM_NS(OnDone);

} // namespace PrintableEnums

using namespace PrintableEnums;

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
    void validConstructs(); // compile test
    void testTree_data();
    void testTree();
    void storageIO_data();
    void storageIO();
    void storageOperators();
    void storageDestructor();
};

void tst_Tasking::validConstructs()
{
    const Group task {
        parallel,
        TestTask([](TaskObject &) {}, [](const TaskObject &) {}),
        TestTask([](TaskObject &) {}, [](const TaskObject &) {}),
        TestTask([](TaskObject &) {}, [](const TaskObject &) {})
    };

    const Group group1 {
        task
    };

    const Group group2 {
        parallel,
        Group {
            parallel,
            TestTask([](TaskObject &) {}, [](const TaskObject &) {}),
            Group {
                parallel,
                TestTask([](TaskObject &) {}, [](const TaskObject &) {}),
                Group {
                    parallel,
                    TestTask([](TaskObject &) {}, [](const TaskObject &) {})
                }
            },
            Group {
                parallel,
                TestTask([](TaskObject &) {}, [](const TaskObject &) {}),
                onGroupDone([] {})
            }
        },
        task,
        onGroupDone([] {}),
        onGroupError([] {})
    };

    const auto setupHandler = [](TaskObject &) {};
    const auto doneHandler = [](const TaskObject &) {};
    const auto errorHandler = [](const TaskObject &) {};

    // Not fluent interface

    const Group task2 {
        parallel,
        TestTask(setupHandler),
        TestTask(setupHandler, doneHandler),
        TestTask(setupHandler, doneHandler, errorHandler),
        // need to explicitly pass empty handler for done
        TestTask(setupHandler, {}, errorHandler)
    };

    // Fluent interface

    const Group fluent {
        parallel,
        TestTask().onSetup(setupHandler),
        TestTask().onSetup(setupHandler).onDone(doneHandler),
        TestTask().onSetup(setupHandler).onDone(doneHandler).onError(errorHandler),
        // possible to skip the empty done
        TestTask().onSetup(setupHandler).onError(errorHandler),
        // possible to set handlers in a different order
        TestTask().onError(errorHandler).onDone(doneHandler).onSetup(setupHandler),
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

class TickAndDone : public QObject
{
    Q_OBJECT

public:
    void setInterval(const milliseconds &interval) { m_interval = interval; }
    void start() {
        QTimer::singleShot(0, this, [this] {
            emit tick();
            QTimer::singleShot(m_interval, this, &TickAndDone::done);
        });
    }

signals:
    void tick();
    void done();

private:
    milliseconds m_interval;
};

class TickAndDoneTaskAdapter : public TaskAdapter<TickAndDone>
{
public:
    TickAndDoneTaskAdapter() { connect(task(), &TickAndDone::done, this,
                                       [this] { emit done(true); }); }
    void start() final { task()->start(); }
};

using TickAndDoneTask = CustomTask<TickAndDoneTaskAdapter>;

template <typename SharedBarrierType>
GroupItem createBarrierAdvance(const TreeStorage<CustomStorage> &storage,
                               const SharedBarrierType &barrier, int taskId)
{
    return TickAndDoneTask([storage, barrier, taskId](TickAndDone &tickAndDone) {
        tickAndDone.setInterval(1ms);
        storage->m_log.append({taskId, Handler::Setup});

        CustomStorage *currentStorage = storage.activeStorage();
        Barrier *sharedBarrier = barrier->barrier();
        QObject::connect(&tickAndDone, &TickAndDone::tick, sharedBarrier,
                         [currentStorage, sharedBarrier, taskId] {
            currentStorage->m_log.append({taskId, Handler::BarrierAdvance});
            sharedBarrier->advance();
        });
    });
}

void tst_Tasking::testTree_data()
{
    QTest::addColumn<TestData>("testData");

    TreeStorage<CustomStorage> storage;

    const auto setupTask = [storage](int taskId, milliseconds timeout) {
        return [storage, taskId, timeout](TaskObject &taskObject) {
            taskObject = timeout;
            storage->m_log.append({taskId, Handler::Setup});
        };
    };

    const auto setupDynamicTask = [storage](int taskId, SetupResult action) {
        return [storage, taskId, action](TaskObject &) {
            storage->m_log.append({taskId, Handler::Setup});
            return action;
        };
    };

    const auto setupDone = [storage](int taskId) {
        return [storage, taskId](const TaskObject &) {
            storage->m_log.append({taskId, Handler::Done});
        };
    };

    const auto setupError = [storage](int taskId) {
        return [storage, taskId](const TaskObject &) {
            storage->m_log.append({taskId, Handler::Error});
        };
    };

    const auto setupTimeout = [storage](int taskId) {
        return [storage, taskId] {
            storage->m_log.append({taskId, Handler::Timeout});
        };
    };

    const auto createTask = [storage, setupTask, setupDone, setupError](
            int taskId, bool successTask, milliseconds timeout = 0ms) -> GroupItem {
        if (successTask)
            return TestTask(setupTask(taskId, timeout), setupDone(taskId), setupError(taskId));
        const Group root {
            finishAllAndError,
            TestTask(setupTask(taskId, timeout)),
            onGroupDone([storage, taskId] { storage->m_log.append({taskId, Handler::Done}); }),
            onGroupError([storage, taskId] { storage->m_log.append({taskId, Handler::Error}); })
        };
        return root;
    };

    const auto createSuccessTask = [createTask](int taskId, milliseconds timeout = 0ms) {
        return createTask(taskId, true, timeout);
    };

    const auto createFailingTask = [createTask](int taskId, milliseconds timeout = 0ms) {
        return createTask(taskId, false, timeout);
    };

    const auto createDynamicTask = [storage, setupDynamicTask, setupDone, setupError](
                                       int taskId, SetupResult action) {
        return TestTask(setupDynamicTask(taskId, action), setupDone(taskId), setupError(taskId));
    };

    const auto groupSetup = [storage](int taskId) {
        return onGroupSetup([=] { storage->m_log.append({taskId, Handler::GroupSetup}); });
    };
    const auto groupDone = [storage](int taskId) {
        return onGroupDone([=] { storage->m_log.append({taskId, Handler::GroupDone}); });
    };
    const auto groupError = [storage](int taskId) {
        return onGroupError([=] { storage->m_log.append({taskId, Handler::GroupError}); });
    };
    const auto createSync = [storage](int taskId) {
        return Sync([=] { storage->m_log.append({taskId, Handler::Sync}); });
    };
    const auto createSyncWithReturn = [storage](int taskId, bool success) {
        return Sync([=] { storage->m_log.append({taskId, Handler::Sync}); return success; });
    };

    {
        const Group root1 {
            Storage(storage),
            groupDone(0),
            groupError(0)
        };
        const Group root2 {
            Storage(storage),
            onGroupSetup([] { return SetupResult::Continue; }),
            groupDone(0),
            groupError(0)
        };
        const Group root3 {
            Storage(storage),
            onGroupSetup([] { return SetupResult::StopWithDone; }),
            groupDone(0),
            groupError(0)
        };
        const Group root4 {
            Storage(storage),
            onGroupSetup([] { return SetupResult::StopWithError; }),
            groupDone(0),
            groupError(0)
        };

        const Log logDone {{0, Handler::GroupDone}};
        const Log logError {{0, Handler::GroupError}};

        QTest::newRow("Empty") << TestData{storage, root1, logDone, 0, OnDone::Success};
        QTest::newRow("EmptyContinue") << TestData{storage, root2, logDone, 0, OnDone::Success};
        QTest::newRow("EmptyDone") << TestData{storage, root3, logDone, 0, OnDone::Success};
        QTest::newRow("EmptyError") << TestData{storage, root4, logError, 0, OnDone::Failure};
    }

    {
        const auto setupGroup = [=](SetupResult setupResult, WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                workflowPolicy(policy),
                onGroupSetup([setupResult] { return setupResult; }),
                groupDone(0),
                groupError(0)
            };
        };

        const auto doneData = [storage, setupGroup](WorkflowPolicy policy) {
            return TestData{storage, setupGroup(SetupResult::StopWithDone, policy),
                            Log{{0, Handler::GroupDone}}, 0, OnDone::Success};
        };
        const auto errorData = [storage, setupGroup](WorkflowPolicy policy) {
            return TestData{storage, setupGroup(SetupResult::StopWithError, policy),
                            Log{{0, Handler::GroupError}}, 0, OnDone::Failure};
        };

        QTest::newRow("DoneAndStopOnError") << doneData(WorkflowPolicy::StopOnError);
        QTest::newRow("DoneAndContinueOnError") << doneData(WorkflowPolicy::ContinueOnError);
        QTest::newRow("DoneAndStopOnDone") << doneData(WorkflowPolicy::StopOnDone);
        QTest::newRow("DoneAndContinueOnDone") << doneData(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("DoneAndStopOnFinished") << doneData(WorkflowPolicy::StopOnFinished);
        QTest::newRow("DoneAndFinishAllAndDone") << doneData(WorkflowPolicy::FinishAllAndDone);
        QTest::newRow("DoneAndFinishAllAndError") << doneData(WorkflowPolicy::FinishAllAndError);

        QTest::newRow("ErrorAndStopOnError") << errorData(WorkflowPolicy::StopOnError);
        QTest::newRow("ErrorAndContinueOnError") << errorData(WorkflowPolicy::ContinueOnError);
        QTest::newRow("ErrorAndStopOnDone") << errorData(WorkflowPolicy::StopOnDone);
        QTest::newRow("ErrorAndContinueOnDone") << errorData(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("ErrorAndStopOnFinished") << errorData(WorkflowPolicy::StopOnFinished);
        QTest::newRow("ErrorAndFinishAllAndDone") << errorData(WorkflowPolicy::FinishAllAndDone);
        QTest::newRow("ErrorAndFinishAllAndError") << errorData(WorkflowPolicy::FinishAllAndError);
    }

    {
        const Group root {
            Storage(storage),
            createDynamicTask(1, SetupResult::StopWithDone),
            createDynamicTask(2, SetupResult::StopWithDone)
        };
        const Log log {{1, Handler::Setup}, {2, Handler::Setup}};
        QTest::newRow("DynamicTaskDone") << TestData{storage, root, log, 2, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            createDynamicTask(1, SetupResult::StopWithError),
            createDynamicTask(2, SetupResult::StopWithError)
        };
        const Log log {{1, Handler::Setup}};
        QTest::newRow("DynamicTaskError") << TestData{storage, root, log, 2, OnDone::Failure};
    }

    {
        const Group root {
            Storage(storage),
            createDynamicTask(1, SetupResult::Continue),
            createDynamicTask(2, SetupResult::Continue),
            createDynamicTask(3, SetupResult::StopWithError),
            createDynamicTask(4, SetupResult::Continue)
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
            createDynamicTask(1, SetupResult::Continue),
            createDynamicTask(2, SetupResult::Continue),
            createDynamicTask(3, SetupResult::StopWithError),
            createDynamicTask(4, SetupResult::Continue)
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
            createDynamicTask(1, SetupResult::Continue),
            createDynamicTask(2, SetupResult::Continue),
            Group {
                createDynamicTask(3, SetupResult::StopWithError)
            },
            createDynamicTask(4, SetupResult::Continue)
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
            createDynamicTask(1, SetupResult::Continue),
            createDynamicTask(2, SetupResult::Continue),
            Group {
                onGroupSetup([storage] {
                    storage->m_log.append({0, Handler::GroupSetup});
                    return SetupResult::StopWithError;
                }),
                createDynamicTask(3, SetupResult::Continue)
            },
            createDynamicTask(4, SetupResult::Continue)
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
                                createSuccessTask(5),
                                groupSetup(5),
                                groupDone(5)
                            },
                            groupSetup(4),
                            groupDone(4)
                        },
                        groupSetup(3),
                        groupDone(3)
                    },
                    groupSetup(2),
                    groupDone(2)
                },
                groupSetup(1),
                groupDone(1)
            },
            groupDone(0)
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
        const Group root {
            Storage(storage),
            parallel,
            createSuccessTask(1),
            createSuccessTask(2),
            createSuccessTask(3),
            createSuccessTask(4),
            createSuccessTask(5),
            groupDone(0)
        };
        const Log log {
            {1, Handler::Setup}, // Setup order is determined in parallel mode
            {2, Handler::Setup},
            {3, Handler::Setup},
            {4, Handler::Setup},
            {5, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Done},
            {3, Handler::Done},
            {4, Handler::Done},
            {5, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("Parallel") << TestData{storage, root, log, 5, OnDone::Success};
    }

    {
        auto setupSubTree = [storage, createSuccessTask](TaskTree &taskTree) {
            const Group nestedRoot {
                Storage(storage),
                createSuccessTask(2),
                createSuccessTask(3),
                createSuccessTask(4)
            };
            taskTree.setRecipe(nestedRoot);
            CustomStorage *activeStorage = storage.activeStorage();
            const auto collectSubLog = [activeStorage](const CustomStorage &subTreeStorage){
                activeStorage->m_log += subTreeStorage.m_log;
            };
            taskTree.onStorageDone(storage, collectSubLog);
        };
        const Group root1 {
            Storage(storage),
            createSuccessTask(1),
            createSuccessTask(2),
            createSuccessTask(3),
            createSuccessTask(4),
            createSuccessTask(5),
            groupDone(0)
        };
        const Group root2 {
            Storage(storage),
            Group { createSuccessTask(1) },
            Group { createSuccessTask(2) },
            Group { createSuccessTask(3) },
            Group { createSuccessTask(4) },
            Group { createSuccessTask(5) },
            groupDone(0)
        };
        const Group root3 {
            Storage(storage),
            createSuccessTask(1),
            TaskTreeTask(setupSubTree),
            createSuccessTask(5),
            groupDone(0)
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
                createSuccessTask(1),
                Group {
                    createSuccessTask(2),
                    Group {
                        createSuccessTask(3),
                        Group {
                            createSuccessTask(4),
                            Group {
                                createSuccessTask(5),
                                groupDone(5)
                            },
                            groupDone(4)
                        },
                        groupDone(3)
                    },
                    groupDone(2)
                },
                groupDone(1)
            },
            groupDone(0)
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
            createSuccessTask(1),
            createSuccessTask(2),
            createFailingTask(3),
            createSuccessTask(4),
            createSuccessTask(5),
            groupDone(0),
            groupError(0)
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
        const auto createRoot = [storage, groupDone, groupError](WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                workflowPolicy(policy),
                groupDone(0),
                groupError(0)
            };
        };

        const Log doneLog = {{0, Handler::GroupDone}};
        const Log errorLog = {{0, Handler::GroupError}};

        const Group root1 = createRoot(WorkflowPolicy::StopOnError);
        QTest::newRow("EmptyStopOnError") << TestData{storage, root1, doneLog, 0,
                                                      OnDone::Success};

        const Group root2 = createRoot(WorkflowPolicy::ContinueOnError);
        QTest::newRow("EmptyContinueOnError") << TestData{storage, root2, doneLog, 0,
                                                          OnDone::Success};

        const Group root3 = createRoot(WorkflowPolicy::StopOnDone);
        QTest::newRow("EmptyStopOnDone") << TestData{storage, root3, errorLog, 0,
                                                     OnDone::Failure};

        const Group root4 = createRoot(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("EmptyContinueOnDone") << TestData{storage, root4, errorLog, 0,
                                                         OnDone::Failure};

        const Group root5 = createRoot(WorkflowPolicy::StopOnFinished);
        QTest::newRow("EmptyStopOnFinished") << TestData{storage, root5, errorLog, 0,
                                                         OnDone::Failure};

        const Group root6 = createRoot(WorkflowPolicy::FinishAllAndDone);
        QTest::newRow("EmptyFinishAllAndDone") << TestData{storage, root6, doneLog, 0,
                                                           OnDone::Success};

        const Group root7 = createRoot(WorkflowPolicy::FinishAllAndError);
        QTest::newRow("EmptyFinishAllAndError") << TestData{storage, root7, errorLog, 0,
                                                            OnDone::Failure};
    }

    {
        const auto createRoot = [storage, createSuccessTask, groupDone, groupError](
                                    WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                workflowPolicy(policy),
                createSuccessTask(1),
                groupDone(0),
                groupError(0)
            };
        };

        const Log doneLog = {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupDone}
        };

        const Log errorLog = {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupError}
        };

        const Group root1 = createRoot(WorkflowPolicy::StopOnError);
        QTest::newRow("DoneStopOnError") << TestData{storage, root1, doneLog, 1,
                                                     OnDone::Success};

        const Group root2 = createRoot(WorkflowPolicy::ContinueOnError);
        QTest::newRow("DoneContinueOnError") << TestData{storage, root2, doneLog, 1,
                                                         OnDone::Success};

        const Group root3 = createRoot(WorkflowPolicy::StopOnDone);
        QTest::newRow("DoneStopOnDone") << TestData{storage, root3, doneLog, 1,
                                                    OnDone::Success};

        const Group root4 = createRoot(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("DoneContinueOnDone") << TestData{storage, root4, doneLog, 1,
                                                        OnDone::Success};

        const Group root5 = createRoot(WorkflowPolicy::StopOnFinished);
        QTest::newRow("DoneStopOnFinished") << TestData{storage, root5, doneLog, 1,
                                                        OnDone::Success};

        const Group root6 = createRoot(WorkflowPolicy::FinishAllAndDone);
        QTest::newRow("DoneFinishAllAndDone") << TestData{storage, root6, doneLog, 1,
                                                          OnDone::Success};

        const Group root7 = createRoot(WorkflowPolicy::FinishAllAndError);
        QTest::newRow("DoneFinishAllAndError") << TestData{storage, root7, errorLog, 1,
                                                           OnDone::Failure};
    }

    {
        const auto createRoot = [storage, createFailingTask, groupDone, groupError](
                                    WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                workflowPolicy(policy),
                createFailingTask(1),
                groupDone(0),
                groupError(0)
            };
        };

        const Log doneLog = {
            {1, Handler::Setup},
            {1, Handler::Error},
            {0, Handler::GroupDone}
        };

        const Log errorLog = {
            {1, Handler::Setup},
            {1, Handler::Error},
            {0, Handler::GroupError}
        };

        const Group root1 = createRoot(WorkflowPolicy::StopOnError);
        QTest::newRow("ErrorStopOnError") << TestData{storage, root1, errorLog, 1,
                                                      OnDone::Failure};

        const Group root2 = createRoot(WorkflowPolicy::ContinueOnError);
        QTest::newRow("ErrorContinueOnError") << TestData{storage, root2, errorLog, 1,
                                                          OnDone::Failure};

        const Group root3 = createRoot(WorkflowPolicy::StopOnDone);
        QTest::newRow("ErrorStopOnDone") << TestData{storage, root3, errorLog, 1,
                                                     OnDone::Failure};

        const Group root4 = createRoot(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("ErrorContinueOnDone") << TestData{storage, root4, errorLog, 1,
                                                         OnDone::Failure};

        const Group root5 = createRoot(WorkflowPolicy::StopOnFinished);
        QTest::newRow("ErrorStopOnFinished") << TestData{storage, root5, errorLog, 1,
                                                         OnDone::Failure};

        const Group root6 = createRoot(WorkflowPolicy::FinishAllAndDone);
        QTest::newRow("ErrorFinishAllAndDone") << TestData{storage, root6, doneLog, 1,
                                                           OnDone::Success};

        const Group root7 = createRoot(WorkflowPolicy::FinishAllAndError);
        QTest::newRow("ErrorFinishAllAndError") << TestData{storage, root7, errorLog, 1,
                                                           OnDone::Failure};
    }

    {
        // These tests check whether the proper root's group end handler is called
        // when the group is stopped. Test it with different workflow policies.
        // The root starts one short failing task together with one long task.
        const auto createRoot = [storage, createSuccessTask, createFailingTask, groupDone,
                                 groupError](WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                parallel,
                workflowPolicy(policy),
                createFailingTask(1, 1ms),
                createSuccessTask(2, 1ms),
                groupDone(0),
                groupError(0)
            };
        };

        const Log errorErrorLog = {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {1, Handler::Error},
            {2, Handler::Error},
            {0, Handler::GroupError}
        };

        const Log errorDoneLog = {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {1, Handler::Error},
            {2, Handler::Done},
            {0, Handler::GroupError}
        };

        const Log doneLog = {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {1, Handler::Error},
            {2, Handler::Done},
            {0, Handler::GroupDone}
        };

        const Group root1 = createRoot(WorkflowPolicy::StopOnError);
        QTest::newRow("StopRootWithStopOnError")
            << TestData{storage, root1, errorErrorLog, 2, OnDone::Failure};

        const Group root2 = createRoot(WorkflowPolicy::ContinueOnError);
        QTest::newRow("StopRootWithContinueOnError")
            << TestData{storage, root2, errorDoneLog, 2, OnDone::Failure};

        const Group root3 = createRoot(WorkflowPolicy::StopOnDone);
        QTest::newRow("StopRootWithStopOnDone")
            << TestData{storage, root3, doneLog, 2, OnDone::Success};

        const Group root4 = createRoot(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("StopRootWithContinueOnDone")
            << TestData{storage, root4, doneLog, 2, OnDone::Success};

        const Group root5 = createRoot(WorkflowPolicy::StopOnFinished);
        QTest::newRow("StopRootWithStopOnFinished")
            << TestData{storage, root5, errorErrorLog, 2, OnDone::Failure};

        const Group root6 = createRoot(WorkflowPolicy::FinishAllAndDone);
        QTest::newRow("StopRootWithFinishAllAndDone")
            << TestData{storage, root6, doneLog, 2, OnDone::Success};

        const Group root7 = createRoot(WorkflowPolicy::FinishAllAndError);
        QTest::newRow("StopRootWithFinishAllAndError")
            << TestData{storage, root7, errorDoneLog, 2, OnDone::Failure};
    }

    {
        // These tests check whether the proper root's group end handler is called
        // when the group is stopped. Test it with different workflow policies.
        // The root starts in parallel: one very short successful task, one short failing task
        // and one long task.
        const auto createRoot = [storage, createSuccessTask, createFailingTask, groupDone,
                                 groupError](WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                parallel,
                workflowPolicy(policy),
                createSuccessTask(1),
                createFailingTask(2, 1ms),
                createSuccessTask(3, 1ms),
                groupDone(0),
                groupError(0)
            };
        };

        const Log errorErrorLog = {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Error},
            {3, Handler::Error},
            {0, Handler::GroupError}
        };

        const Log errorDoneLog = {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Error},
            {3, Handler::Done},
            {0, Handler::GroupError}
        };

        const Log doneErrorLog = {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Error},
            {3, Handler::Error},
            {0, Handler::GroupDone}
        };

        const Log doneDoneLog = {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Error},
            {3, Handler::Done},
            {0, Handler::GroupDone}
        };

        const Group root1 = createRoot(WorkflowPolicy::StopOnError);
        QTest::newRow("StopRootAfterDoneWithStopOnError")
            << TestData{storage, root1, errorErrorLog, 3, OnDone::Failure};

        const Group root2 = createRoot(WorkflowPolicy::ContinueOnError);
        QTest::newRow("StopRootAfterDoneWithContinueOnError")
            << TestData{storage, root2, errorDoneLog, 3, OnDone::Failure};

        const Group root3 = createRoot(WorkflowPolicy::StopOnDone);
        QTest::newRow("StopRootAfterDoneWithStopOnDone")
            << TestData{storage, root3, doneErrorLog, 3, OnDone::Success};

        const Group root4 = createRoot(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("StopRootAfterDoneWithContinueOnDone")
            << TestData{storage, root4, doneDoneLog, 3, OnDone::Success};

        const Group root5 = createRoot(WorkflowPolicy::StopOnFinished);
        QTest::newRow("StopRootAfterDoneWithStopOnFinished")
            << TestData{storage, root5, doneErrorLog, 3, OnDone::Success};

        const Group root6 = createRoot(WorkflowPolicy::FinishAllAndDone);
        QTest::newRow("StopRootAfterDoneWithFinishAllAndDone")
            << TestData{storage, root6, doneDoneLog, 3, OnDone::Success};

        const Group root7 = createRoot(WorkflowPolicy::FinishAllAndError);
        QTest::newRow("StopRootAfterDoneWithFinishAllAndError")
            << TestData{storage, root7, errorDoneLog, 3, OnDone::Failure};
    }

    {
        // These tests check whether the proper subgroup's end handler is called
        // when the group is stopped. Test it with different workflow policies.
        // The subgroup starts one long task.
        const auto createRoot = [storage, createSuccessTask, createFailingTask, groupDone,
                                 groupError](WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                parallel,
                Group {
                    workflowPolicy(policy),
                    createSuccessTask(1, 1000ms),
                    groupDone(1),
                    groupError(1)
                },
                createFailingTask(2, 1ms),
                groupDone(2),
                groupError(2)
            };
        };

        const Log errorLog = {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {2, Handler::Error},
            {1, Handler::Error},
            {1, Handler::GroupError},
            {2, Handler::GroupError}
        };

        const Log doneLog = {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {2, Handler::Error},
            {1, Handler::Error},
            {1, Handler::GroupDone},
            {2, Handler::GroupError}
        };

        const Group root1 = createRoot(WorkflowPolicy::StopOnError);
        QTest::newRow("StopGroupWithStopOnError")
            << TestData{storage, root1, errorLog, 2, OnDone::Failure};

        const Group root2 = createRoot(WorkflowPolicy::ContinueOnError);
        QTest::newRow("StopGroupWithContinueOnError")
            << TestData{storage, root2, errorLog, 2, OnDone::Failure};

        const Group root3 = createRoot(WorkflowPolicy::StopOnDone);
        QTest::newRow("StopGroupWithStopOnDone")
            << TestData{storage, root3, errorLog, 2, OnDone::Failure};

        const Group root4 = createRoot(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("StopGroupWithContinueOnDone")
            << TestData{storage, root4, errorLog, 2, OnDone::Failure};

        const Group root5 = createRoot(WorkflowPolicy::StopOnFinished);
        QTest::newRow("StopGroupWithStopOnFinished")
            << TestData{storage, root5, errorLog, 2, OnDone::Failure};

        const Group root6 = createRoot(WorkflowPolicy::FinishAllAndDone);
        QTest::newRow("StopGroupWithFinishAllAndDone")
            << TestData{storage, root6, doneLog, 2, OnDone::Failure};

        const Group root7 = createRoot(WorkflowPolicy::FinishAllAndError);
        QTest::newRow("StopGroupWithFinishAllAndError")
            << TestData{storage, root7, errorLog, 2, OnDone::Failure};
    }

    {
        // These tests check whether the proper subgroup's end handler is called
        // when the group is stopped. Test it with different workflow policies.
        // The sequential subgroup starts one short successful task followed by one long task.
        const auto createRoot = [storage, createSuccessTask, createFailingTask, groupDone,
                                 groupError](WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                parallel,
                Group {
                    workflowPolicy(policy),
                    createSuccessTask(1),
                    createSuccessTask(2, 1000ms),
                    groupDone(1),
                    groupError(1)
                },
                createFailingTask(3, 1ms),
                groupDone(2),
                groupError(2)
            };
        };

        const Log errorLog = {
            {1, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {3, Handler::Error},
            {2, Handler::Error},
            {1, Handler::GroupError},
            {2, Handler::GroupError}
        };

        const Log shortDoneLog = {
            {1, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Done},
            {1, Handler::GroupDone},
            {3, Handler::Error},
            {2, Handler::GroupError}
        };

        const Log longDoneLog = {
            {1, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {3, Handler::Error},
            {2, Handler::Error},
            {1, Handler::GroupDone},
            {2, Handler::GroupError}
        };

        const Group root1 = createRoot(WorkflowPolicy::StopOnError);
        QTest::newRow("StopGroupAfterDoneWithStopOnError")
            << TestData{storage, root1, errorLog, 3, OnDone::Failure};

        const Group root2 = createRoot(WorkflowPolicy::ContinueOnError);
        QTest::newRow("StopGroupAfterDoneWithContinueOnError")
            << TestData{storage, root2, errorLog, 3, OnDone::Failure};

        const Group root3 = createRoot(WorkflowPolicy::StopOnDone);
        QTest::newRow("StopGroupAfterDoneWithStopOnDone")
            << TestData{storage, root3, shortDoneLog, 3, OnDone::Failure};

        const Group root4 = createRoot(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("StopGroupAfterDoneWithContinueOnDone")
            << TestData{storage, root4, longDoneLog, 3, OnDone::Failure};

        const Group root5 = createRoot(WorkflowPolicy::StopOnFinished);
        QTest::newRow("StopGroupAfterDoneWithStopOnFinished")
            << TestData{storage, root5, shortDoneLog, 3, OnDone::Failure};

        const Group root6 = createRoot(WorkflowPolicy::FinishAllAndDone);
        QTest::newRow("StopGroupAfterDoneWithFinishAllAndDone")
            << TestData{storage, root6, longDoneLog, 3, OnDone::Failure};

        const Group root7 = createRoot(WorkflowPolicy::FinishAllAndError);
        QTest::newRow("StopGroupAfterDoneWithFinishAllAndError")
            << TestData{storage, root7, errorLog, 3, OnDone::Failure};
    }

    {
        // These tests check whether the proper subgroup's end handler is called
        // when the group is stopped. Test it with different workflow policies.
        // The sequential subgroup starts one short failing task followed by one long task.
        const auto createRoot = [storage, createSuccessTask, createFailingTask, groupDone,
                                 groupError](WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                parallel,
                Group {
                    workflowPolicy(policy),
                    createFailingTask(1),
                    createSuccessTask(2, 1000ms),
                    groupDone(1),
                    groupError(1)
                },
                createFailingTask(3, 1ms),
                groupDone(2),
                groupError(2)
            };
        };

        const Log shortErrorLog = {
            {1, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Error},
            {1, Handler::GroupError},
            {3, Handler::Error},
            {2, Handler::GroupError}
        };

        const Log longErrorLog = {
            {1, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Error},
            {2, Handler::Setup},
            {3, Handler::Error},
            {2, Handler::Error},
            {1, Handler::GroupError},
            {2, Handler::GroupError}
        };

        const Log doneLog = {
            {1, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Error},
            {2, Handler::Setup},
            {3, Handler::Error},
            {2, Handler::Error},
            {1, Handler::GroupDone},
            {2, Handler::GroupError}
        };

        const Group root1 = createRoot(WorkflowPolicy::StopOnError);
        QTest::newRow("StopGroupAfterErrorWithStopOnError")
            << TestData{storage, root1, shortErrorLog, 3, OnDone::Failure};

        const Group root2 = createRoot(WorkflowPolicy::ContinueOnError);
        QTest::newRow("StopGroupAfterErrorWithContinueOnError")
            << TestData{storage, root2, longErrorLog, 3, OnDone::Failure};

        const Group root3 = createRoot(WorkflowPolicy::StopOnDone);
        QTest::newRow("StopGroupAfterErrorWithStopOnDone")
            << TestData{storage, root3, longErrorLog, 3, OnDone::Failure};

        const Group root4 = createRoot(WorkflowPolicy::ContinueOnDone);
        QTest::newRow("StopGroupAfterErrorWithContinueOnDone")
            << TestData{storage, root4, longErrorLog, 3, OnDone::Failure};

        const Group root5 = createRoot(WorkflowPolicy::StopOnFinished);
        QTest::newRow("StopGroupAfterErrorWithStopOnFinished")
            << TestData{storage, root5, shortErrorLog, 3, OnDone::Failure};

        const Group root6 = createRoot(WorkflowPolicy::FinishAllAndDone);
        QTest::newRow("StopGroupAfterErrorWithFinishAllAndDone")
            << TestData{storage, root6, doneLog, 3, OnDone::Failure};

        const Group root7 = createRoot(WorkflowPolicy::FinishAllAndError);
        QTest::newRow("StopGroupAfterErrorWithFinishAllAndError")
            << TestData{storage, root7, longErrorLog, 3, OnDone::Failure};
    }

    {
        const auto createRoot = [storage, createSuccessTask, createFailingTask, groupDone,
                                 groupError](WorkflowPolicy policy) {
            return Group {
                Storage(storage),
                workflowPolicy(policy),
                createSuccessTask(1),
                createFailingTask(2),
                createSuccessTask(3),
                groupDone(0),
                groupError(0)
            };
        };

        const Group root1 = createRoot(WorkflowPolicy::StopOnError);
        const Log log1 {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Error},
            {0, Handler::GroupError}
        };
        QTest::newRow("StopOnError") << TestData{storage, root1, log1, 3, OnDone::Failure};

        const Group root2 = createRoot(WorkflowPolicy::ContinueOnError);
        const Log errorLog {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Error},
            {3, Handler::Setup},
            {3, Handler::Done},
            {0, Handler::GroupError}
        };
        QTest::newRow("ContinueOnError") << TestData{storage, root2, errorLog, 3, OnDone::Failure};

        const Group root3 = createRoot(WorkflowPolicy::StopOnDone);
        const Log log3 {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("StopOnDone") << TestData{storage, root3, log3, 3, OnDone::Success};

        const Group root4 = createRoot(WorkflowPolicy::ContinueOnDone);
        const Log doneLog {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Error},
            {3, Handler::Setup},
            {3, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("ContinueOnDone") << TestData{storage, root4, doneLog, 3, OnDone::Success};

        const Group root5 = createRoot(WorkflowPolicy::StopOnFinished);
        const Log log5 {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("StopOnFinished") << TestData{storage, root5, log5, 3, OnDone::Success};

        const Group root6 = createRoot(WorkflowPolicy::FinishAllAndDone);
        QTest::newRow("FinishAllAndDone") << TestData{storage, root6, doneLog, 3, OnDone::Success};

        const Group root7 = createRoot(WorkflowPolicy::FinishAllAndError);
        QTest::newRow("FinishAllAndError") << TestData{storage, root7, errorLog, 3, OnDone::Failure};
    }

    {
        const auto createRoot = [storage, createTask, groupDone, groupError](
                                    bool firstSuccess, bool secondSuccess) {
            return Group {
                parallel,
                stopOnFinished,
                Storage(storage),
                createTask(1, firstSuccess, 1000ms),
                createTask(2, secondSuccess, 1ms),
                groupDone(0),
                groupError(0)
            };
        };

        const Group root1 = createRoot(true, true);
        const Group root2 = createRoot(true, false);
        const Group root3 = createRoot(false, true);
        const Group root4 = createRoot(false, false);

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
        const auto createRoot = [storage, createSuccessTask, groupDone, groupError](
                                    SetupResult setupResult) {
            return Group {
                Storage(storage),
                Group {
                    createSuccessTask(1)
                },
                Group {
                    onGroupSetup([=] { return setupResult; }),
                    createSuccessTask(2),
                    createSuccessTask(3),
                    createSuccessTask(4)
                },
                groupDone(0),
                groupError(0)
            };
        };

        const Group root1 = createRoot(SetupResult::StopWithDone);
        const Log log1 {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("DynamicSetupDone") << TestData{storage, root1, log1, 4, OnDone::Success};

        const Group root2 = createRoot(SetupResult::StopWithError);
        const Log log2 {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupError}
        };
        QTest::newRow("DynamicSetupError") << TestData{storage, root2, log2, 4, OnDone::Failure};

        const Group root3 = createRoot(SetupResult::Continue);
        const Log log3 {
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
        QTest::newRow("DynamicSetupContinue") << TestData{storage, root3, log3, 4, OnDone::Success};
    }

    {
        const Group root {
            parallelLimit(2),
            Storage(storage),
            Group {
                groupSetup(1),
                createSuccessTask(1)
            },
            Group {
                groupSetup(2),
                createSuccessTask(2)
            },
            Group {
                groupSetup(3),
                createSuccessTask(3)
            },
            Group {
                groupSetup(4),
                createSuccessTask(4)
            }
        };
        const Log log {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {1, Handler::Done},
            {3, Handler::GroupSetup},
            {3, Handler::Setup},
            {2, Handler::Done},
            {4, Handler::GroupSetup},
            {4, Handler::Setup},
            {3, Handler::Done},
            {4, Handler::Done}
        };
        QTest::newRow("NestedParallel") << TestData{storage, root, log, 4, OnDone::Success};
    }

    {
        const Group root {
            parallelLimit(2),
            Storage(storage),
            Group {
                groupSetup(1),
                createSuccessTask(1)
            },
            Group {
                groupSetup(2),
                createSuccessTask(2)
            },
            Group {
                groupSetup(3),
                createDynamicTask(3, SetupResult::StopWithDone)
            },
            Group {
                groupSetup(4),
                createSuccessTask(4)
            },
            Group {
                groupSetup(5),
                createSuccessTask(5)
            }
        };
        const Log log {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {1, Handler::Done},
            {3, Handler::GroupSetup},
            {3, Handler::Setup},
            {4, Handler::GroupSetup},
            {4, Handler::Setup},
            {2, Handler::Done},
            {5, Handler::GroupSetup},
            {5, Handler::Setup},
            {4, Handler::Done},
            {5, Handler::Done}
        };
        QTest::newRow("NestedParallelDone") << TestData{storage, root, log, 5, OnDone::Success};
    }

    {
        const Group root1 {
            parallelLimit(2),
            Storage(storage),
            Group {
                groupSetup(1),
                createSuccessTask(1)
            },
            Group {
                groupSetup(2),
                createSuccessTask(2)
            },
            Group {
                groupSetup(3),
                createDynamicTask(3, SetupResult::StopWithError)
            },
            Group {
                groupSetup(4),
                createSuccessTask(4)
            },
            Group {
                groupSetup(5),
                createSuccessTask(5)
            }
        };
        const Log log1 {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {1, Handler::Done},
            {3, Handler::GroupSetup},
            {3, Handler::Setup},
            {2, Handler::Error}
        };

        // Inside this test the task 2 should finish first, then synchonously:
        // - task 3 should exit setup with error
        // - task 1 should be stopped as a consequence of the error inside the group
        // - tasks 4 and 5 should be skipped
        const Group root2 {
            parallelLimit(2),
            Storage(storage),
            Group {
                groupSetup(1),
                createSuccessTask(1, 10ms)
            },
            Group {
                groupSetup(2),
                createSuccessTask(2)
            },
            Group {
                groupSetup(3),
                createDynamicTask(3, SetupResult::StopWithError)
            },
            Group {
                groupSetup(4),
                createSuccessTask(4)
            },
            Group {
                groupSetup(5),
                createSuccessTask(5)
            }
        };
        const Log log2 {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::GroupSetup},
            {3, Handler::Setup},
            {1, Handler::Error}
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
                    groupSetup(1),
                    createSuccessTask(1, 10ms)
                },
                Group {
                    groupSetup(2),
                    createSuccessTask(2, 1ms)
                },
                Group {
                    groupSetup(3),
                    createDynamicTask(3, SetupResult::StopWithError)
                },
                Group {
                    groupSetup(4),
                    createSuccessTask(4)
                }
            },
            Group {
                groupSetup(5),
                createSuccessTask(5)
            }
        };
        const Log log3 {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::GroupSetup},
            {3, Handler::Setup},
            {1, Handler::Error},
            {5, Handler::GroupSetup},
            {5, Handler::Setup},
            {5, Handler::Done}
        };
        QTest::newRow("NestedParallelError1")
            << TestData{storage, root1, log1, 5, OnDone::Failure};
        QTest::newRow("NestedParallelError2")
            << TestData{storage, root2, log2, 5, OnDone::Failure};
        QTest::newRow("NestedParallelError3")
            << TestData{storage, root3, log3, 5, OnDone::Failure};
    }

    {
        const Group root {
            parallelLimit(2),
            Storage(storage),
            Group {
                groupSetup(1),
                Group {
                    parallel,
                    createSuccessTask(1)
                }
            },
            Group {
                groupSetup(2),
                Group {
                    parallel,
                    createSuccessTask(2)
                }
            },
            Group {
                groupSetup(3),
                Group {
                    parallel,
                    createSuccessTask(3)
                }
            },
            Group {
                groupSetup(4),
                Group {
                    parallel,
                    createSuccessTask(4)
                }
            }
        };
        const Log log {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {1, Handler::Done},
            {3, Handler::GroupSetup},
            {3, Handler::Setup},
            {2, Handler::Done},
            {4, Handler::GroupSetup},
            {4, Handler::Setup},
            {3, Handler::Done},
            {4, Handler::Done}
        };
        QTest::newRow("DeeplyNestedParallel") << TestData{storage, root, log, 4, OnDone::Success};
    }

    {
        const Group root {
            parallelLimit(2),
            Storage(storage),
            Group {
                groupSetup(1),
                Group { createSuccessTask(1) }
            },
            Group {
                groupSetup(2),
                Group { createSuccessTask(2) }
            },
            Group {
                groupSetup(3),
                Group { createDynamicTask(3, SetupResult::StopWithDone) }
            },
            Group {
                groupSetup(4),
                Group { createSuccessTask(4) }
            },
            Group {
                groupSetup(5),
                Group { createSuccessTask(5) }
            }
        };
        const Log log {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {1, Handler::Done},
            {3, Handler::GroupSetup},
            {3, Handler::Setup},
            {4, Handler::GroupSetup},
            {4, Handler::Setup},
            {2, Handler::Done},
            {5, Handler::GroupSetup},
            {5, Handler::Setup},
            {4, Handler::Done},
            {5, Handler::Done}
        };
        QTest::newRow("DeeplyNestedParallelDone")
            << TestData{storage, root, log, 5, OnDone::Success};
    }

    {
        const Group root {
            parallelLimit(2),
            Storage(storage),
            Group {
                groupSetup(1),
                Group { createSuccessTask(1) }
            },
            Group {
                groupSetup(2),
                Group { createSuccessTask(2) }
            },
            Group {
                groupSetup(3),
                Group { createDynamicTask(3, SetupResult::StopWithError) }
            },
            Group {
                groupSetup(4),
                Group { createSuccessTask(4) }
            },
            Group {
                groupSetup(5),
                Group { createSuccessTask(5) }
            }
        };
        const Log log {
            {1, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {1, Handler::Done},
            {3, Handler::GroupSetup},
            {3, Handler::Setup},
            {2, Handler::Error}
        };
        QTest::newRow("DeeplyNestedParallelError")
            << TestData{storage, root, log, 5, OnDone::Failure};
    }

    {
        const Group root {
            Storage(storage),
            createSync(1),
            createSync(2),
            createSync(3),
            createSync(4),
            createSync(5)
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
            createSyncWithReturn(1, true),
            createSyncWithReturn(2, true),
            createSyncWithReturn(3, true),
            createSyncWithReturn(4, true),
            createSyncWithReturn(5, true)
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
            createSync(1),
            createSync(2),
            createSync(3),
            createSync(4),
            createSync(5)
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
            createSync(1),
            createSync(2),
            createSyncWithReturn(3, false),
            createSync(4),
            createSync(5)
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
            createSync(1),
            createSuccessTask(2),
            createSync(3),
            createSuccessTask(4),
            createSync(5),
            groupDone(0)
        };
        const Log log {
            {1, Handler::Sync},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::Sync},
            {4, Handler::Setup},
            {4, Handler::Done},
            {5, Handler::Sync},
            {0, Handler::GroupDone}
        };
        QTest::newRow("SyncAndAsync") << TestData{storage, root, log, 2, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            createSync(1),
            createSuccessTask(2),
            createSyncWithReturn(3, false),
            createSuccessTask(4),
            createSync(5),
            groupError(0)
        };
        const Log log {
            {1, Handler::Sync},
            {2, Handler::Setup},
            {2, Handler::Done},
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
            createBarrierAdvance(storage, barrier, 1),
            Group {
                groupSetup(2),
                waitForBarrierTask(barrier),
                createSuccessTask(2),
                createSuccessTask(3)
            }
        };
        const Log log1 {
            {1, Handler::Setup},
            {1, Handler::BarrierAdvance},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::Setup},
            {3, Handler::Done}
        };

        // Test that barrier advance, triggered from inside the task described by
        // setupTaskWithCondition, placed BEFORE the group containing the waitFor() element
        // in the tree order, works OK in PARALLEL mode.
        const Group root2 {
            Storage(storage),
            Storage(barrier),
            parallel,
            createBarrierAdvance(storage, barrier, 1),
            Group {
                groupSetup(2),
                waitForBarrierTask(barrier),
                createSuccessTask(2),
                createSuccessTask(3)
            }
        };
        const Log log2 {
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {1, Handler::BarrierAdvance},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::Setup},
            {3, Handler::Done}
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
                groupSetup(2),
                waitForBarrierTask(barrier),
                createSuccessTask(2),
                createSuccessTask(3)
            },
            createBarrierAdvance(storage, barrier, 1)
        };
        const Log log3 {
            {2, Handler::GroupSetup},
            {1, Handler::Setup},
            {1, Handler::BarrierAdvance},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::Setup},
            {3, Handler::Done}
        };

        // Test that barrier advance, triggered from inside the task described by
        // setupBarrierAdvance, placed BEFORE the groups containing the waitFor() element
        // in the tree order, wakes both waitFor tasks.
        const Group root4 {
            Storage(storage),
            Storage(barrier),
            parallel,
            createBarrierAdvance(storage, barrier, 1),
            Group {
                groupSetup(2),
                waitForBarrierTask(barrier),
                createSuccessTask(4)
            },
            Group {
                groupSetup(3),
                waitForBarrierTask(barrier),
                createSuccessTask(5)
            }
        };
        const Log log4 {
            {1, Handler::Setup},
            {2, Handler::GroupSetup},
            {3, Handler::GroupSetup},
            {1, Handler::BarrierAdvance},
            {4, Handler::Setup},
            {5, Handler::Setup},
            {4, Handler::Done},
            {5, Handler::Done}
        };

        // Test two separate single barriers.

        SingleBarrier barrier2;

        const Group root5 {
            Storage(storage),
            Storage(barrier),
            Storage(barrier2),
            parallel,
            createBarrierAdvance(storage, barrier, 1),
            createBarrierAdvance(storage, barrier2, 2),
            Group {
                Group {
                    parallel,
                    groupSetup(1),
                    waitForBarrierTask(barrier),
                    waitForBarrierTask(barrier2)
                },
                createSuccessTask(3)
            },
        };
        const Log log5 {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {1, Handler::GroupSetup},
            {1, Handler::BarrierAdvance},
            {2, Handler::BarrierAdvance},
            {3, Handler::Setup},
            {3, Handler::Done}
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
            createBarrierAdvance(storage, barrier, 1),
            createBarrierAdvance(storage, barrier, 2),
            Group {
                groupSetup(2),
                waitForBarrierTask(barrier),
                createSuccessTask(2),
                createSuccessTask(3)
            }
        };
        const Log log1 {
            {1, Handler::Setup},
            {1, Handler::BarrierAdvance},
            {2, Handler::Setup},
            {2, Handler::BarrierAdvance},
            {2, Handler::GroupSetup},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::Setup},
            {3, Handler::Done}
        };

        // Test that multi barrier advance, triggered from inside the tasks described by
        // setupBarrierAdvance, placed BEFORE the group containing the waitFor() element
        // in the tree order, works OK in PARALLEL mode.
        const Group root2 {
            Storage(storage),
            Storage(barrier),
            parallel,
            createBarrierAdvance(storage, barrier, 1),
            createBarrierAdvance(storage, barrier, 2),
            Group {
                groupSetup(2),
                waitForBarrierTask(barrier),
                createSuccessTask(3),
                createSuccessTask(4)
            }
        };
        const Log log2 {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {2, Handler::GroupSetup},
            {1, Handler::BarrierAdvance},
            {2, Handler::BarrierAdvance},
            {3, Handler::Setup},
            {3, Handler::Done},
            {4, Handler::Setup},
            {4, Handler::Done}
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
                groupSetup(2),
                waitForBarrierTask(barrier),
                createSuccessTask(3),
                createSuccessTask(4)
            },
            createBarrierAdvance(storage, barrier, 1),
            createBarrierAdvance(storage, barrier, 2)
        };
        const Log log3 {
            {2, Handler::GroupSetup},
            {1, Handler::Setup},
            {2, Handler::Setup},
            {1, Handler::BarrierAdvance},
            {2, Handler::BarrierAdvance},
            {3, Handler::Setup},
            {3, Handler::Done},
            {4, Handler::Setup},
            {4, Handler::Done}
        };

        // Test that multi barrier advance, triggered from inside the task described by
        // setupBarrierAdvance, placed BEFORE the groups containing the waitFor() element
        // in the tree order, wakes both waitFor tasks.
        const Group root4 {
            Storage(storage),
            Storage(barrier),
            parallel,
            createBarrierAdvance(storage, barrier, 1),
            createBarrierAdvance(storage, barrier, 2),
            Group {
                groupSetup(2),
                waitForBarrierTask(barrier),
                createSuccessTask(3)
            },
            Group {
                groupSetup(3),
                waitForBarrierTask(barrier),
                createSuccessTask(4)
            }
        };
        const Log log4 {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {2, Handler::GroupSetup},
            {3, Handler::GroupSetup},
            {1, Handler::BarrierAdvance},
            {2, Handler::BarrierAdvance},
            {3, Handler::Setup},
            {4, Handler::Setup},
            {3, Handler::Done},
            {4, Handler::Done}
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

    {
        // Test CustomTask::withTimeout() combinations:
        // 1. When the timeout has triggered or not.
        // 2. With and without timeout handler.
        const Group root1 {
            Storage(storage),
            TestTask(setupTask(1, 1000ms), setupDone(1), setupError(1))
                .withTimeout(1ms)
        };
        const Log log1 {
            {1, Handler::Setup},
            {1, Handler::Error}
        };
        QTest::newRow("TaskErrorWithTimeout") << TestData{storage, root1, log1, 2,
                                                          OnDone::Failure};

        const Group root2 {
            Storage(storage),
            TestTask(setupTask(1, 1000ms), setupDone(1), setupError(1))
                .withTimeout(1ms, setupTimeout(1))
        };
        const Log log2 {
            {1, Handler::Setup},
            {1, Handler::Timeout},
            {1, Handler::Error}
        };
        QTest::newRow("TaskErrorWithTimeoutHandler") << TestData{storage, root2, log2, 2,
                                                                 OnDone::Failure};

        const Group root3 {
            Storage(storage),
            TestTask(setupTask(1, 1ms), setupDone(1), setupError(1))
                .withTimeout(1000ms)
        };
        const Log doneLog {
            {1, Handler::Setup},
            {1, Handler::Done}
        };
        QTest::newRow("TaskDoneWithTimeout") << TestData{storage, root3, doneLog, 2,
                                                         OnDone::Success};

        const Group root4 {
            Storage(storage),
            TestTask(setupTask(1, 1ms), setupDone(1), setupError(1))
                .withTimeout(1000ms, setupTimeout(1))
        };
        QTest::newRow("TaskDoneWithTimeoutHandler") << TestData{storage, root4, doneLog, 2,
                                                                OnDone::Success};
    }

    {
        // Test Group::withTimeout() combinations:
        // 1. When the timeout has triggered or not.
        // 2. With and without timeout handler.
        const Group root1 {
            Storage(storage),
            Group {
                createSuccessTask(1, 1000ms)
            }.withTimeout(1ms)
        };
        const Log log1 {
            {1, Handler::Setup},
            {1, Handler::Error}
        };
        QTest::newRow("GroupErrorWithTimeout") << TestData{storage, root1, log1, 2,
                                                           OnDone::Failure};

        // Test Group::withTimeout(), passing custom handler
        const Group root2 {
            Storage(storage),
            Group {
                createSuccessTask(1, 1000ms)
            }.withTimeout(1ms, setupTimeout(1))
        };
        const Log log2 {
            {1, Handler::Setup},
            {1, Handler::Timeout},
            {1, Handler::Error}
        };
        QTest::newRow("GroupErrorWithTimeoutHandler") << TestData{storage, root2, log2, 2,
                                                                  OnDone::Failure};

        const Group root3 {
            Storage(storage),
            Group {
                createSuccessTask(1, 1ms)
            }.withTimeout(1000ms)
        };
        const Log doneLog {
            {1, Handler::Setup},
            {1, Handler::Done}
        };
        QTest::newRow("GroupDoneWithTimeout") << TestData{storage, root3, doneLog, 2,
                                                          OnDone::Success};

        // Test Group::withTimeout(), passing custom handler
        const Group root4 {
            Storage(storage),
            Group {
                createSuccessTask(1, 1ms)
            }.withTimeout(1000ms, setupTimeout(1))
        };
        QTest::newRow("GroupDoneWithTimeoutHandler") << TestData{storage, root4, doneLog, 2,
                                                                 OnDone::Success};
    }
}

void tst_Tasking::testTree()
{
    QFETCH(TestData, testData);

    TaskTree taskTree({testData.root.withTimeout(1000ms)});
    QCOMPARE(taskTree.taskCount() - 1, testData.taskCount); // -1 for the timeout task above
    Log actualLog;
    const auto collectLog = [&actualLog](const CustomStorage &storage) {
        actualLog = storage.m_log;
    };
    taskTree.onStorageDone(testData.storage, collectLog);
    const OnDone result = taskTree.runBlocking() ? OnDone::Success : OnDone::Failure;
    QCOMPARE(taskTree.isRunning(), false);

    QCOMPARE(taskTree.progressValue(), taskTree.progressMaximum());
    QCOMPARE(actualLog, testData.expectedLog);
    QCOMPARE(CustomStorage::instanceCount(), 0);

    QCOMPARE(result, testData.onDone);
}

struct StorageIO
{
    int value = 0;
};

static Group inputOutputRecipe(const TreeStorage<StorageIO> &storage)
{
    return Group {
        Storage(storage),
        onGroupSetup([storage] { ++storage->value; }),
        onGroupDone([storage] { storage->value *= 2; })
    };
}

void tst_Tasking::storageIO_data()
{
    QTest::addColumn<int>("input");
    QTest::addColumn<int>("output");

    QTest::newRow("-1 -> 0") << -1 << 0;
    QTest::newRow("0 -> 2") <<  0 << 2;
    QTest::newRow("1 -> 4") <<  1 << 4;
    QTest::newRow("2 -> 6") <<  2 << 6;
}

void tst_Tasking::storageIO()
{
    QFETCH(int, input);
    QFETCH(int, output);

    int actualOutput = 0;

    const TreeStorage<StorageIO> storage;
    TaskTree taskTree(inputOutputRecipe(storage));

    const auto setInput = [input](StorageIO &storage) { storage.value = input; };
    const auto getOutput = [&actualOutput](const StorageIO &storage) { actualOutput = storage.value; };

    taskTree.onStorageSetup(storage, setInput);
    taskTree.onStorageDone(storage, getOutput);
    taskTree.runBlocking();

    QCOMPARE(taskTree.isRunning(), false);
    QCOMPARE(actualOutput, output);
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
    const auto setupHandler = [&setupCalled](CustomStorage &) {
        setupCalled = true;
    };
    bool doneCalled = false;
    const auto doneHandler = [&doneCalled](const CustomStorage &) {
        doneCalled = true;
    };
    QCOMPARE(CustomStorage::instanceCount(), 0);
    {
        TreeStorage<CustomStorage> storage;
        const auto setupSleepingTask = [](TaskObject &taskObject) {
            taskObject = 1000ms;
        };
        const Group root {
            Storage(storage),
            TestTask(setupSleepingTask)
        };

        TaskTree taskTree(root);
        QCOMPARE(CustomStorage::instanceCount(), 0);
        taskTree.onStorageSetup(storage, setupHandler);
        taskTree.onStorageDone(storage, doneHandler);
        taskTree.start();
        QCOMPARE(CustomStorage::instanceCount(), 1);
    }
    QCOMPARE(CustomStorage::instanceCount(), 0);
    QVERIFY(setupCalled);
    QVERIFY(!doneCalled);
}

QTEST_GUILESS_MAIN(tst_Tasking)

#include "tst_tasking.moc"
