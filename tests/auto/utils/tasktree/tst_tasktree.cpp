// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <app/app_version.h>

#include <utils/launcherinterface.h>
#include <utils/qtcprocess.h>
#include <utils/singleton.h>
#include <utils/temporarydirectory.h>

#include <QtTest>

using namespace Utils;
using namespace Utils::Tasking;

enum class Handler {
    Setup,
    Done,
    Error,
    GroupSetup,
    GroupDone,
    GroupError
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
static const char s_processIdProperty[] = "__processId";

enum class OnStart { Running, NotRunning };
enum class OnDone { Success, Failure };

struct TestData {
    TreeStorage<CustomStorage> storage;
    Group root;
    Log expectedLog;
    int taskCount = 0;
    OnStart onStart = OnStart::Running;
    OnDone onDone = OnDone::Success;
};

class tst_TaskTree : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void validConstructs(); // compile test
    void processTree_data();
    void processTree();
    void storageOperators();
    void storageDestructor();

    void cleanupTestCase();

private:
    FilePath m_testAppPath;
};

void tst_TaskTree::initTestCase()
{
    TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath() + "/"
                                                    + Core::Constants::IDE_CASED_ID + "-XXXXXX");
    const QString libExecPath(qApp->applicationDirPath() + '/'
                              + QLatin1String(TEST_RELATIVE_LIBEXEC_PATH));
    LauncherInterface::setPathToLauncher(libExecPath);
    m_testAppPath = FilePath::fromString(QLatin1String(TESTAPP_PATH)
                                       + QLatin1String("/testapp")).withExecutableSuffix();
}

void tst_TaskTree::cleanupTestCase()
{
    Singleton::deleteAll();
}

void tst_TaskTree::validConstructs()
{
    const Group process {
        parallel,
        Process([](QtcProcess &) {}, [](const QtcProcess &) {}),
        Process([](QtcProcess &) {}, [](const QtcProcess &) {}),
        Process([](QtcProcess &) {}, [](const QtcProcess &) {})
    };

    const Group group1 {
        process
    };

    const Group group2 {
        parallel,
        Group {
            parallel,
            Process([](QtcProcess &) {}, [](const QtcProcess &) {}),
            Group {
                parallel,
                Process([](QtcProcess &) {}, [](const QtcProcess &) {}),
                Group {
                    parallel,
                    Process([](QtcProcess &) {}, [](const QtcProcess &) {})
                }
            },
            Group {
                parallel,
                Process([](QtcProcess &) {}, [](const QtcProcess &) {}),
                OnGroupDone([] {})
            }
        },
        process,
        OnGroupDone([] {}),
        OnGroupError([] {})
    };
}

void tst_TaskTree::processTree_data()
{
    QTest::addColumn<TestData>("testData");

    TreeStorage<CustomStorage> storage;

    const auto setupProcessHelper = [storage, testAppPath = m_testAppPath]
            (QtcProcess &process, const QStringList &args, int processId) {
        process.setCommand(CommandLine(testAppPath, args));
        process.setProperty(s_processIdProperty, processId);
        storage->m_log.append({processId, Handler::Setup});
    };
    const auto setupProcess = [setupProcessHelper](int processId) {
        return [=](QtcProcess &process) {
            setupProcessHelper(process, {"-return", "0"}, processId);
        };
    };
    const auto setupCrashProcess = [setupProcessHelper](int processId) {
        return [=](QtcProcess &process) {
            setupProcessHelper(process, {"-crash"}, processId);
        };
    };
    const auto setupSleepProcess = [setupProcessHelper](int processId, int msecs) {
        return [=](QtcProcess &process) {
            setupProcessHelper(process, {"-sleep", QString::number(msecs)}, processId);
        };
    };
    const auto setupDynamicProcess = [setupProcessHelper](int processId, TaskAction action) {
        return [=](QtcProcess &process) {
            setupProcessHelper(process, {"-return", "0"}, processId);
            return action;
        };
    };
    const auto readResult = [storage](const QtcProcess &process) {
        const int processId = process.property(s_processIdProperty).toInt();
        storage->m_log.append({processId, Handler::Done});
    };
    const auto readError = [storage](const QtcProcess &process) {
        const int processId = process.property(s_processIdProperty).toInt();
        storage->m_log.append({processId, Handler::Error});
    };
    const auto groupSetup = [storage](int groupId) {
        return [=] { storage->m_log.append({groupId, Handler::GroupSetup}); };
    };
    const auto groupDone = [storage](int groupId) {
        return [=] { storage->m_log.append({groupId, Handler::GroupDone}); };
    };
    const auto groupError = [storage](int groupId) {
        return [=] { storage->m_log.append({groupId, Handler::GroupError}); };
    };

    const auto constructSimpleSequence = [=](const Workflow &policy) {
        return Group {
            Storage(storage),
            policy,
            Process(setupProcess(1), readResult),
            Process(setupCrashProcess(2), readResult, readError),
            Process(setupProcess(3), readResult),
            OnGroupDone(groupDone(0)),
            OnGroupError(groupError(0))
        };
    };
    const auto constructDynamicHierarchy = [=](TaskAction taskAction) {
        return Group {
            Storage(storage),
            Group {
                Process(setupProcess(1), readResult)
            },
            Group {
                OnGroupSetup([=] { return taskAction; }),
                Process(setupProcess(2), readResult),
                Process(setupProcess(3), readResult),
                Process(setupProcess(4), readResult)
            },
            OnGroupDone(groupDone(0)),
            OnGroupError(groupError(0))
        };
    };

    {
        const Group root1 {
            Storage(storage),
            OnGroupDone(groupDone(0)),
            OnGroupError(groupError(0))
        };
        const Group root2 {
            Storage(storage),
            OnGroupSetup([] { return TaskAction::Continue; }),
            OnGroupDone(groupDone(0)),
            OnGroupError(groupError(0))
        };
        const Group root3 {
            Storage(storage),
            OnGroupSetup([] { return TaskAction::StopWithDone; }),
            OnGroupDone(groupDone(0)),
            OnGroupError(groupError(0))
        };
        const Group root4 {
            Storage(storage),
            OnGroupSetup([] { return TaskAction::StopWithError; }),
            OnGroupDone(groupDone(0)),
            OnGroupError(groupError(0))
        };
        const Log logDone {{0, Handler::GroupDone}};
        const Log logError {{0, Handler::GroupError}};
        QTest::newRow("Empty")
            << TestData{storage, root1, logDone, 0, OnStart::NotRunning, OnDone::Success};
        QTest::newRow("EmptyContinue")
            << TestData{storage, root2, logDone, 0, OnStart::NotRunning, OnDone::Success};
        QTest::newRow("EmptyDone")
            << TestData{storage, root3, logDone, 0, OnStart::NotRunning, OnDone::Success};
        QTest::newRow("EmptyError")
            << TestData{storage, root4, logError, 0, OnStart::NotRunning, OnDone::Failure};
    }

    {
        const Group root {
            Storage(storage),
            Process(setupDynamicProcess(1, TaskAction::StopWithDone), readResult, readError),
            Process(setupDynamicProcess(2, TaskAction::StopWithDone), readResult, readError)
        };
        const Log log {{1, Handler::Setup}, {2, Handler::Setup}};
        QTest::newRow("DynamicTaskDone")
                << TestData{storage, root, log, 2, OnStart::NotRunning, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            Process(setupDynamicProcess(1, TaskAction::StopWithError), readResult, readError),
            Process(setupDynamicProcess(2, TaskAction::StopWithError), readResult, readError)
        };
        const Log log {{1, Handler::Setup}};
        QTest::newRow("DynamicTaskError")
                << TestData{storage, root, log, 2, OnStart::NotRunning, OnDone::Failure};
    }

    {
        const Group root {
            Storage(storage),
            Process(setupDynamicProcess(1, TaskAction::Continue), readResult, readError),
            Process(setupDynamicProcess(2, TaskAction::Continue), readResult, readError),
            Process(setupDynamicProcess(3, TaskAction::StopWithError), readResult, readError),
            Process(setupDynamicProcess(4, TaskAction::Continue), readResult, readError)
        };
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Done},
            {3, Handler::Setup}
        };
        QTest::newRow("DynamicMixed")
                << TestData{storage, root, log, 4, OnStart::Running, OnDone::Failure};
    }

    {
        const Group root {
            parallel,
            Storage(storage),
            Process(setupDynamicProcess(1, TaskAction::Continue), readResult, readError),
            Process(setupDynamicProcess(2, TaskAction::Continue), readResult, readError),
            Process(setupDynamicProcess(3, TaskAction::StopWithError), readResult, readError),
            Process(setupDynamicProcess(4, TaskAction::Continue), readResult, readError)
        };
        const Log log {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Error},
            {2, Handler::Error}
        };
        QTest::newRow("DynamicParallel")
                << TestData{storage, root, log, 4, OnStart::NotRunning, OnDone::Failure};
    }

    {
        const Group root {
            parallel,
            Storage(storage),
            Process(setupDynamicProcess(1, TaskAction::Continue), readResult, readError),
            Process(setupDynamicProcess(2, TaskAction::Continue), readResult, readError),
            Group {
                Process(setupDynamicProcess(3, TaskAction::StopWithError), readResult, readError)
            },
            Process(setupDynamicProcess(4, TaskAction::Continue), readResult, readError)
        };
        const Log log {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {3, Handler::Setup},
            {1, Handler::Error},
            {2, Handler::Error}
        };
        QTest::newRow("DynamicParallelGroup")
                << TestData{storage, root, log, 4, OnStart::NotRunning, OnDone::Failure};
    }

    {
        const Group root {
            parallel,
            Storage(storage),
            Process(setupDynamicProcess(1, TaskAction::Continue), readResult, readError),
            Process(setupDynamicProcess(2, TaskAction::Continue), readResult, readError),
            Group {
                OnGroupSetup([storage] {
                    storage->m_log.append({0, Handler::GroupSetup});
                    return TaskAction::StopWithError;
                }),
                Process(setupDynamicProcess(3, TaskAction::Continue), readResult, readError)
            },
            Process(setupDynamicProcess(4, TaskAction::Continue), readResult, readError)
        };
        const Log log {
            {1, Handler::Setup},
            {2, Handler::Setup},
            {0, Handler::GroupSetup},
            {1, Handler::Error},
            {2, Handler::Error}
        };
        QTest::newRow("DynamicParallelGroupSetup")
                << TestData{storage, root, log, 4, OnStart::NotRunning, OnDone::Failure};
    }

    {
        const Group root {
            Storage(storage),
            Group {
                Group {
                    Group {
                        Group {
                            Group {
                                Process(setupProcess(5), readResult, readError),
                                OnGroupSetup(groupSetup(5)),
                                OnGroupDone(groupDone(5))
                            },
                            OnGroupSetup(groupSetup(4)),
                            OnGroupDone(groupDone(4))
                        },
                        OnGroupSetup(groupSetup(3)),
                        OnGroupDone(groupDone(3))
                    },
                    OnGroupSetup(groupSetup(2)),
                    OnGroupDone(groupDone(2))
                },
                OnGroupSetup(groupSetup(1)),
                OnGroupDone(groupDone(1))
            },
            OnGroupDone(groupDone(0))
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
        QTest::newRow("Nested")
                << TestData{storage, root, log, 1, OnStart::Running, OnDone::Success};
    }

    {
        const auto readResultAnonymous = [=](const QtcProcess &) {
            storage->m_log.append({0, Handler::Done});
        };
        const Group root {
            Storage(storage),
            parallel,
            Process(setupProcess(1), readResultAnonymous),
            Process(setupProcess(2), readResultAnonymous),
            Process(setupProcess(3), readResultAnonymous),
            Process(setupProcess(4), readResultAnonymous),
            Process(setupProcess(5), readResultAnonymous),
            OnGroupDone(groupDone(0))
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
        QTest::newRow("Parallel")
                << TestData{storage, root, log, 5, OnStart::Running, OnDone::Success};
    }

    {
        auto setupSubTree = [=](TaskTree &taskTree) {
            const Group nestedRoot {
                Storage(storage),
                Process(setupProcess(2), readResult),
                Process(setupProcess(3), readResult),
                Process(setupProcess(4), readResult)
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
            Process(setupProcess(1), readResult),
            Process(setupProcess(2), readResult),
            Process(setupProcess(3), readResult),
            Process(setupProcess(4), readResult),
            Process(setupProcess(5), readResult),
            OnGroupDone(groupDone(0))
        };
        const Group root2 {
            Storage(storage),
            Group { Process(setupProcess(1), readResult) },
            Group { Process(setupProcess(2), readResult) },
            Group { Process(setupProcess(3), readResult) },
            Group { Process(setupProcess(4), readResult) },
            Group { Process(setupProcess(5), readResult) },
            OnGroupDone(groupDone(0))
        };
        const Group root3 {
            Storage(storage),
            Process(setupProcess(1), readResult),
            Tree(setupSubTree),
            Process(setupProcess(5), readResult),
            OnGroupDone(groupDone(0))
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
        QTest::newRow("Sequential")
                << TestData{storage, root1, log, 5, OnStart::Running, OnDone::Success};
        QTest::newRow("SequentialEncapsulated")
                << TestData{storage, root2, log, 5, OnStart::Running, OnDone::Success};
        QTest::newRow("SequentialSubTree") // We don't inspect subtrees, so taskCount is 3, not 5.
                << TestData{storage, root3, log, 3, OnStart::Running, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            Group {
                Process(setupProcess(1), readResult),
                Group {
                    Process(setupProcess(2), readResult),
                    Group {
                        Process(setupProcess(3), readResult),
                        Group {
                            Process(setupProcess(4), readResult),
                            Group {
                                Process(setupProcess(5), readResult),
                                OnGroupDone(groupDone(5))
                            },
                            OnGroupDone(groupDone(4))
                        },
                        OnGroupDone(groupDone(3))
                    },
                    OnGroupDone(groupDone(2))
                },
                OnGroupDone(groupDone(1))
            },
            OnGroupDone(groupDone(0))
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
        QTest::newRow("SequentialNested")
                << TestData{storage, root, log, 5, OnStart::Running, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            Process(setupProcess(1), readResult),
            Process(setupProcess(2), readResult),
            Process(setupCrashProcess(3), readResult, readError),
            Process(setupProcess(4), readResult),
            Process(setupProcess(5), readResult),
            OnGroupDone(groupDone(0)),
            OnGroupError(groupError(0))
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
        QTest::newRow("SequentialError")
                << TestData{storage, root, log, 5, OnStart::Running, OnDone::Failure};
    }

    {
        const Group root = constructSimpleSequence(stopOnError);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Error},
            {0, Handler::GroupError}
        };
        QTest::newRow("StopOnError")
                << TestData{storage, root, log, 3, OnStart::Running, OnDone::Failure};
    }

    {
        const Group root = constructSimpleSequence(continueOnError);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Error},
            {3, Handler::Setup},
            {3, Handler::Done},
            {0, Handler::GroupError}
        };
        QTest::newRow("ContinueOnError")
                << TestData{storage, root, log, 3, OnStart::Running, OnDone::Failure};
    }

    {
        const Group root = constructSimpleSequence(stopOnDone);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("StopOnDone")
                << TestData{storage, root, log, 3, OnStart::Running, OnDone::Success};
    }

    {
        const Group root = constructSimpleSequence(continueOnDone);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {2, Handler::Setup},
            {2, Handler::Error},
            {3, Handler::Setup},
            {3, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("ContinueOnDone")
                << TestData{storage, root, log, 3, OnStart::Running, OnDone::Success};
    }

    {
        const Group root {
            Storage(storage),
            optional,
            Process(setupCrashProcess(1), readResult, readError),
            Process(setupCrashProcess(2), readResult, readError),
            OnGroupDone(groupDone(0)),
            OnGroupError(groupError(0))
        };
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Error},
            {2, Handler::Setup},
            {2, Handler::Error},
            {0, Handler::GroupDone}
        };
        QTest::newRow("Optional")
                << TestData{storage, root, log, 2, OnStart::Running, OnDone::Success};
    }

    {
        const Group root = constructDynamicHierarchy(TaskAction::StopWithDone);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupDone}
        };
        QTest::newRow("DynamicSetupDone")
                << TestData{storage, root, log, 4, OnStart::Running, OnDone::Success};
    }

    {
        const Group root = constructDynamicHierarchy(TaskAction::StopWithError);
        const Log log {
            {1, Handler::Setup},
            {1, Handler::Done},
            {0, Handler::GroupError}
        };
        QTest::newRow("DynamicSetupError")
                << TestData{storage, root, log, 4, OnStart::Running, OnDone::Failure};
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
        QTest::newRow("DynamicSetupContinue")
                << TestData{storage, root, log, 4, OnStart::Running, OnDone::Success};
    }

    {
        const Group root {
            ParallelLimit(2),
            Storage(storage),
            Group {
                OnGroupSetup(groupSetup(1)),
                Process(setupProcess(1))
            },
            Group {
                OnGroupSetup(groupSetup(2)),
                Process(setupProcess(2))
            },
            Group {
                OnGroupSetup(groupSetup(3)),
                Process(setupProcess(3))
            },
            Group {
                OnGroupSetup(groupSetup(4)),
                Process(setupProcess(4))
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
        QTest::newRow("NestedParallel")
            << TestData{storage, root, log, 4, OnStart::Running, OnDone::Success};
    }

    {
        const Group root {
            ParallelLimit(2),
            Storage(storage),
            Group {
                OnGroupSetup(groupSetup(1)),
                Process(setupProcess(1))
            },
            Group {
                OnGroupSetup(groupSetup(2)),
                Process(setupProcess(2))
            },
            Group {
                OnGroupSetup(groupSetup(3)),
                Process(setupDynamicProcess(3, TaskAction::StopWithDone))
            },
            Group {
                OnGroupSetup(groupSetup(4)),
                Process(setupProcess(4))
            },
            Group {
                OnGroupSetup(groupSetup(5)),
                Process(setupProcess(5))
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
        QTest::newRow("NestedParallelDone")
            << TestData{storage, root, log, 5, OnStart::Running, OnDone::Success};
    }

    {
        const Group root1 {
            ParallelLimit(2),
            Storage(storage),
            Group {
                OnGroupSetup(groupSetup(1)),
                Process(setupProcess(1))
            },
            Group {
                OnGroupSetup(groupSetup(2)),
                Process(setupProcess(2))
            },
            Group {
                OnGroupSetup(groupSetup(3)),
                Process(setupDynamicProcess(3, TaskAction::StopWithError))
            },
            Group {
                OnGroupSetup(groupSetup(4)),
                Process(setupProcess(4))
            },
            Group {
                OnGroupSetup(groupSetup(5)),
                Process(setupProcess(5))
            }
        };

        // Inside this test the process 2 should finish first, then synchonously:
        // - process 3 should exit setup with error
        // - process 1 should be stopped as a consequence of error inside the group
        // - processes 4 and 5 should be skipped
        const Group root2 {
            ParallelLimit(2),
            Storage(storage),
            Group {
                OnGroupSetup(groupSetup(1)),
                Process(setupSleepProcess(1, 100))
            },
            Group {
                OnGroupSetup(groupSetup(2)),
                Process(setupProcess(2))
            },
            Group {
                OnGroupSetup(groupSetup(3)),
                Process(setupDynamicProcess(3, TaskAction::StopWithError))
            },
            Group {
                OnGroupSetup(groupSetup(4)),
                Process(setupProcess(4))
            },
            Group {
                OnGroupSetup(groupSetup(5)),
                Process(setupProcess(5))
            }
        };

        // This test ensures process 1 doesn't invoke its done handler,
        // being ready while sleeping in process 2 done handler.
        // Inside this test the process 2 should finish first, then synchonously:
        // - process 3 should exit setup with error
        // - process 1 should be stopped as a consequence of error inside the group
        // - process 4 should be skipped
        // - the first child group of root should finish with error
        // - process 5 should be started (because of root's continueOnError policy)
        const Group root3 {
            continueOnError,
            Storage(storage),
            Group {
                ParallelLimit(2),
                Group {
                    OnGroupSetup(groupSetup(1)),
                    Process(setupSleepProcess(1, 100))
                },
                Group {
                    OnGroupSetup(groupSetup(2)),
                    Process(setupProcess(2), [](const QtcProcess &) { QThread::msleep(200); })
                },
                Group {
                    OnGroupSetup(groupSetup(3)),
                    Process(setupDynamicProcess(3, TaskAction::StopWithError))
                },
                Group {
                    OnGroupSetup(groupSetup(4)),
                    Process(setupProcess(4))
                }
            },
            Group {
                OnGroupSetup(groupSetup(5)),
                Process(setupProcess(5))
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
            << TestData{storage, root1, shortLog, 5, OnStart::Running, OnDone::Failure};
        QTest::newRow("NestedParallelError2")
            << TestData{storage, root2, shortLog, 5, OnStart::Running, OnDone::Failure};
        QTest::newRow("NestedParallelError3")
            << TestData{storage, root3, longLog, 5, OnStart::Running, OnDone::Failure};
    }

    {
        const Group root {
            ParallelLimit(2),
            Storage(storage),
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(1)),
                Group {
                    parallel,
                    Process(setupProcess(1))
                }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(2)),
                Group {
                    parallel,
                    Process(setupProcess(2))
                }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(3)),
                Group {
                    parallel,
                    Process(setupProcess(3))
                }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(4)),
                Group {
                    parallel,
                    Process(setupProcess(4))
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
        QTest::newRow("DeeplyNestedParallel")
            << TestData{storage, root, log, 4, OnStart::Running, OnDone::Success};
    }

    {
        const Group root {
            ParallelLimit(2),
            Storage(storage),
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(1)),
                Group { Process(setupProcess(1)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(2)),
                Group { Process(setupProcess(2)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(3)),
                Group { Process(setupDynamicProcess(3, TaskAction::StopWithDone)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(4)),
                Group { Process(setupProcess(4)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(5)),
                Group { Process(setupProcess(5)) }
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
            << TestData{storage, root, log, 5, OnStart::Running, OnDone::Success};
    }

    {
        const Group root {
            ParallelLimit(2),
            Storage(storage),
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(1)),
                Group { Process(setupProcess(1)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(2)),
                Group { Process(setupProcess(2)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(3)),
                Group { Process(setupDynamicProcess(3, TaskAction::StopWithError)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(4)),
                Group { Process(setupProcess(4)) }
            },
            Group {
                Storage(TreeStorage<CustomStorage>()),
                OnGroupSetup(groupSetup(5)),
                Group { Process(setupProcess(5)) }
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
            << TestData{storage, root, log, 5, OnStart::Running, OnDone::Failure};
    }
}

void tst_TaskTree::processTree()
{
    QFETCH(TestData, testData);

    QEventLoop eventLoop;
    TaskTree taskTree(testData.root);
    QCOMPARE(taskTree.taskCount(), testData.taskCount);
    int doneCount = 0;
    int errorCount = 0;
    connect(&taskTree, &TaskTree::done, this, [&doneCount, &eventLoop] {
        ++doneCount;
        eventLoop.quit();
    });
    connect(&taskTree, &TaskTree::errorOccurred, this, [&errorCount, &eventLoop] {
        ++errorCount;
        eventLoop.quit();
    });
    Log actualLog;
    auto collectLog = [&actualLog](CustomStorage *storage){
        actualLog = storage->m_log;
    };
    taskTree.onStorageDone(testData.storage, collectLog);
    taskTree.start();
    const bool expectRunning = testData.onStart == OnStart::Running;
    QCOMPARE(taskTree.isRunning(), expectRunning);

    if (expectRunning) {
        QTimer timer;
        bool timedOut = false;
        connect(&timer, &QTimer::timeout, &eventLoop, [&eventLoop, &timedOut] {
            timedOut = true;
            eventLoop.quit();
        });
        timer.setInterval(2000);
        timer.setSingleShot(true);
        timer.start();
        eventLoop.exec();
        QCOMPARE(timedOut, false);
        QCOMPARE(taskTree.isRunning(), false);
    }

    QCOMPARE(taskTree.progressValue(), testData.taskCount);
    QCOMPARE(actualLog, testData.expectedLog);
    QCOMPARE(CustomStorage::instanceCount(), 0);

    const bool expectSuccess = testData.onDone == OnDone::Success;
    const int expectedDoneCount = expectSuccess ? 1 : 0;
    const int expectedErrorCount = expectSuccess ? 0 : 1;
    QCOMPARE(doneCount, expectedDoneCount);
    QCOMPARE(errorCount, expectedErrorCount);
}

void tst_TaskTree::storageOperators()
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
void tst_TaskTree::storageDestructor()
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
        const auto setupProcess = [testAppPath = m_testAppPath](QtcProcess &process) {
            process.setCommand(CommandLine(testAppPath, {"-sleep", "1000"}));
        };
        const Group root {
            Storage(storage),
            Process(setupProcess)
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

QTEST_GUILESS_MAIN(tst_TaskTree)

#include "tst_tasktree.moc"
