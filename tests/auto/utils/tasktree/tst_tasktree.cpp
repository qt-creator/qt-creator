// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include <app/app_version.h>

#include <utils/launcherinterface.h>
#include <utils/qtcprocess.h>
#include <utils/singleton.h>
#include <utils/temporarydirectory.h>

#include <QtTest>

#include <iostream>
#include <fstream>

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

class tst_TaskTree : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void validConstructs(); // compile test
    void processTree_data();
    void processTree();
    void storage_data();
    void storage();
    void storageOperators();
    void storageDestructor();

    void cleanupTestCase();

private:
    Log m_log;
    FilePath m_testAppPath;
};

void tst_TaskTree::initTestCase()
{
    Utils::TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath() + "/"
                             + Core::Constants::IDE_CASED_ID + "-XXXXXX");
    const QString libExecPath(qApp->applicationDirPath() + '/'
                              + QLatin1String(TEST_RELATIVE_LIBEXEC_PATH));
    LauncherInterface::setPathToLauncher(libExecPath);
    m_testAppPath = FilePath::fromString(QLatin1String(TESTAPP_PATH)
                                       + QLatin1String("/testapp")).withExecutableSuffix();
}

void tst_TaskTree::cleanupTestCase()
{
    Utils::Singleton::deleteAll();
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
                OnGroupDone([] {}),
            }
        },
        process,
        OnGroupDone([] {}),
        OnGroupError([] {})
    };
}

static const char s_processIdProperty[] = "__processId";

void tst_TaskTree::processTree_data()
{
    using namespace std::placeholders;

    QTest::addColumn<Group>("root");
    QTest::addColumn<Log>("expectedLog");
    QTest::addColumn<bool>("runningAfterStart");
    QTest::addColumn<bool>("success");
    QTest::addColumn<int>("taskCount");

    const auto setupProcessHelper = [this](QtcProcess &process, const QStringList &args, int processId) {
        process.setCommand(CommandLine(m_testAppPath, args));
        process.setProperty(s_processIdProperty, processId);
        m_log.append({processId, Handler::Setup});
    };
    const auto setupProcess = [setupProcessHelper](QtcProcess &process, int processId) {
        setupProcessHelper(process, {"-return", "0"}, processId);
    };
//    const auto setupErrorProcess = [setupProcessHelper](QtcProcess &process, int processId) {
//        setupProcessHelper(process, {"-return", "1"}, processId);
//    };
    const auto setupCrashProcess = [setupProcessHelper](QtcProcess &process, int processId) {
        setupProcessHelper(process, {"-crash"}, processId);
    };
    const auto readResultAnonymous = [this](const QtcProcess &) {
        m_log.append({-1, Handler::Done});
    };
    const auto readResult = [this](const QtcProcess &process) {
        const int processId = process.property(s_processIdProperty).toInt();
        m_log.append({processId, Handler::Done});
    };
    const auto readError = [this](const QtcProcess &process) {
        const int processId = process.property(s_processIdProperty).toInt();
        m_log.append({processId, Handler::Error});
    };
    const auto groupSetup = [this](int processId) {
        m_log.append({processId, Handler::GroupSetup});
    };
    const auto groupDone = [this](int processId) {
        m_log.append({processId, Handler::GroupDone});
    };
    const auto rootDone = [this] {
        m_log.append({-1, Handler::GroupDone});
    };
    const auto rootError = [this] {
        m_log.append({-1, Handler::GroupError});
    };

    const Group emptyRoot {
        OnGroupDone(rootDone)
    };
    const Log emptyLog{{-1, Handler::GroupDone}};
    QTest::newRow("Empty") << emptyRoot << emptyLog << false << true << 0;

    const Group nestedRoot {
        Group {
            Group {
                Group {
                    Group {
                        Group {
                            Process(std::bind(setupProcess, _1, 5), readResult),
                            OnGroupSetup(std::bind(groupSetup, 5)),
                            OnGroupDone(std::bind(groupDone, 5))
                        },
                        OnGroupSetup(std::bind(groupSetup, 4)),
                        OnGroupDone(std::bind(groupDone, 4))
                    },
                    OnGroupSetup(std::bind(groupSetup, 3)),
                    OnGroupDone(std::bind(groupDone, 3))
                },
                OnGroupSetup(std::bind(groupSetup, 2)),
                OnGroupDone(std::bind(groupDone, 2))
            },
            OnGroupSetup(std::bind(groupSetup, 1)),
            OnGroupDone(std::bind(groupDone, 1))
        },
        OnGroupDone(rootDone)
    };
    const Log nestedLog{{1, Handler::GroupSetup},
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
                        {-1, Handler::GroupDone}};
    QTest::newRow("Nested") << nestedRoot << nestedLog << true << true << 1;

    const Group parallelRoot {
        parallel,
        Process(std::bind(setupProcess, _1, 1), readResultAnonymous),
        Process(std::bind(setupProcess, _1, 2), readResultAnonymous),
        Process(std::bind(setupProcess, _1, 3), readResultAnonymous),
        Process(std::bind(setupProcess, _1, 4), readResultAnonymous),
        Process(std::bind(setupProcess, _1, 5), readResultAnonymous),
        OnGroupDone(rootDone)
    };
    const Log parallelLog{{1, Handler::Setup}, // Setup order is determined in parallel mode
                          {2, Handler::Setup},
                          {3, Handler::Setup},
                          {4, Handler::Setup},
                          {5, Handler::Setup},
                          {-1, Handler::Done}, // Done order isn't determined in parallel mode
                          {-1, Handler::Done},
                          {-1, Handler::Done},
                          {-1, Handler::Done},
                          {-1, Handler::Done},
                          {-1, Handler::GroupDone}}; // Done handlers may come in different order
    QTest::newRow("Parallel") << parallelRoot << parallelLog << true << true << 5;

    const Group sequentialRoot {
        Process(std::bind(setupProcess, _1, 1), readResult),
        Process(std::bind(setupProcess, _1, 2), readResult),
        Process(std::bind(setupProcess, _1, 3), readResult),
        Process(std::bind(setupProcess, _1, 4), readResult),
        Process(std::bind(setupProcess, _1, 5), readResult),
        OnGroupDone(rootDone)
    };
    const Group sequentialEncapsulatedRoot {
        Group {
            Process(std::bind(setupProcess, _1, 1), readResult)
        },
        Group {
            Process(std::bind(setupProcess, _1, 2), readResult)
        },
        Group {
            Process(std::bind(setupProcess, _1, 3), readResult)
        },
        Group {
            Process(std::bind(setupProcess, _1, 4), readResult)
        },
        Group {
            Process(std::bind(setupProcess, _1, 5), readResult)
        },
        OnGroupDone(rootDone)
    };
    auto setupSubTree = [=](TaskTree &taskTree) {
        const Group nestedRoot {
            Process(std::bind(setupProcess, _1, 2), readResult),
            Process(std::bind(setupProcess, _1, 3), readResult),
            Process(std::bind(setupProcess, _1, 4), readResult),
        };
        taskTree.setupRoot(nestedRoot);
    };
    const Group sequentialSubTreeRoot {
        Process(std::bind(setupProcess, _1, 1), readResult),
        Tree(setupSubTree),
        Process(std::bind(setupProcess, _1, 5), readResult),
        OnGroupDone(rootDone)
    };
    const Log sequentialLog{{1, Handler::Setup},
                            {1, Handler::Done},
                            {2, Handler::Setup},
                            {2, Handler::Done},
                            {3, Handler::Setup},
                            {3, Handler::Done},
                            {4, Handler::Setup},
                            {4, Handler::Done},
                            {5, Handler::Setup},
                            {5, Handler::Done},
                            {-1, Handler::GroupDone}};
    QTest::newRow("Sequential") << sequentialRoot << sequentialLog << true << true << 5;
    QTest::newRow("SequentialEncapsulated") << sequentialEncapsulatedRoot << sequentialLog
                                            << true << true << 5;
    QTest::newRow("SequentialSubTree") << sequentialSubTreeRoot << sequentialLog
                                       << true << true << 3; // We don't inspect subtrees

    const Group sequentialNestedRoot {
        Group {
            Process(std::bind(setupProcess, _1, 1), readResult),
            Group {
                Process(std::bind(setupProcess, _1, 2), readResult),
                Group {
                    Process(std::bind(setupProcess, _1, 3), readResult),
                    Group {
                        Process(std::bind(setupProcess, _1, 4), readResult),
                        Group {
                            Process(std::bind(setupProcess, _1, 5), readResult),
                            OnGroupDone(std::bind(groupDone, 5))
                        },
                        OnGroupDone(std::bind(groupDone, 4))
                    },
                    OnGroupDone(std::bind(groupDone, 3))
                },
                OnGroupDone(std::bind(groupDone, 2))
            },
            OnGroupDone(std::bind(groupDone, 1))
        },
        OnGroupDone(rootDone)
    };
    const Log sequentialNestedLog{{1, Handler::Setup},
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
                                  {-1, Handler::GroupDone}};
    QTest::newRow("SequentialNested") << sequentialNestedRoot << sequentialNestedLog
                                      << true << true << 5;

    const Group sequentialErrorRoot {
        Process(std::bind(setupProcess, _1, 1), readResult),
        Process(std::bind(setupProcess, _1, 2), readResult),
        Process(std::bind(setupCrashProcess, _1, 3), readResult, readError),
        Process(std::bind(setupProcess, _1, 4), readResult),
        Process(std::bind(setupProcess, _1, 5), readResult),
        OnGroupDone(rootDone),
        OnGroupError(rootError)
    };
    const Log sequentialErrorLog{{1, Handler::Setup},
                                 {1, Handler::Done},
                                 {2, Handler::Setup},
                                 {2, Handler::Done},
                                 {3, Handler::Setup},
                                 {3, Handler::Error},
                                 {-1, Handler::GroupError}};
    QTest::newRow("SequentialError") << sequentialErrorRoot << sequentialErrorLog
                                     << true << false << 5;

    const auto constructSimpleSequence = [=](const Workflow &policy) {
        return Group {
            policy,
            Process(std::bind(setupProcess, _1, 1), readResult),
            Process(std::bind(setupCrashProcess, _1, 2), readResult, readError),
            Process(std::bind(setupProcess, _1, 3), readResult),
            OnGroupDone(rootDone),
            OnGroupError(rootError)
        };
    };

    const Group stopOnErrorRoot = constructSimpleSequence(stopOnError);
    const Log stopOnErrorLog{{1, Handler::Setup},
                             {1, Handler::Done},
                             {2, Handler::Setup},
                             {2, Handler::Error},
                             {-1, Handler::GroupError}};
    QTest::newRow("StopOnError") << stopOnErrorRoot << stopOnErrorLog << true << false << 3;

    const Group continueOnErrorRoot = constructSimpleSequence(continueOnError);
    const Log continueOnErrorLog{{1, Handler::Setup},
                                 {1, Handler::Done},
                                 {2, Handler::Setup},
                                 {2, Handler::Error},
                                 {3, Handler::Setup},
                                 {3, Handler::Done},
                                 {-1, Handler::GroupError}};
    QTest::newRow("ContinueOnError") << continueOnErrorRoot << continueOnErrorLog
                                     << true << false << 3;

    const Group stopOnDoneRoot = constructSimpleSequence(stopOnDone);
    const Log stopOnDoneLog{{1, Handler::Setup},
                            {1, Handler::Done},
                            {-1, Handler::GroupDone}};
    QTest::newRow("StopOnDone") << stopOnDoneRoot << stopOnDoneLog
                                << true << true << 3;

    const Group continueOnDoneRoot = constructSimpleSequence(continueOnDone);
    const Log continueOnDoneLog{{1, Handler::Setup},
                                {1, Handler::Done},
                                {2, Handler::Setup},
                                {2, Handler::Error},
                                {3, Handler::Setup},
                                {3, Handler::Done},
                                {-1, Handler::GroupDone}};
    QTest::newRow("ContinueOnDone") << continueOnDoneRoot << continueOnDoneLog << true << true << 3;

    const Group optionalRoot {
        optional,
        Process(std::bind(setupCrashProcess, _1, 1), readResult, readError),
        Process(std::bind(setupCrashProcess, _1, 2), readResult, readError),
        OnGroupDone(rootDone),
        OnGroupError(rootError)
    };
    const Log optionalLog{{1, Handler::Setup},
                          {1, Handler::Error},
                          {2, Handler::Setup},
                          {2, Handler::Error},
                          {-1, Handler::GroupDone}};
    QTest::newRow("Optional") << optionalRoot << optionalLog << true << true << 2;

    const auto stopWithDoneSetup = [] { return GroupConfig{GroupAction::StopWithDone}; };
    const auto stopWithErrorSetup = [] { return GroupConfig{GroupAction::StopWithError}; };
    const auto continueAllSetup = [] { return GroupConfig{GroupAction::ContinueAll}; };
    const auto continueSelSetup = [] { return GroupConfig{GroupAction::ContinueSelected, {0, 2}}; };
    const auto constructDynamicSetup = [=](const DynamicSetup &dynamicSetup) {
        return Group {
            Group {
                Process(std::bind(setupProcess, _1, 1), readResult)
            },
            Group {
                dynamicSetup,
                Process(std::bind(setupProcess, _1, 2), readResult),
                Process(std::bind(setupProcess, _1, 3), readResult),
                Process(std::bind(setupProcess, _1, 4), readResult)
            },
            OnGroupDone(rootDone),
            OnGroupError(rootError)
        };
    };
    const Group dynamicSetupDoneRoot = constructDynamicSetup({stopWithDoneSetup});
    const Log dynamicSetupDoneLog{{1, Handler::Setup},
                                  {1, Handler::Done},
                                  {-1, Handler::GroupDone}};
    QTest::newRow("DynamicSetupDone") << dynamicSetupDoneRoot << dynamicSetupDoneLog
                                      << true << true << 4;

    const Group dynamicSetupErrorRoot = constructDynamicSetup({stopWithErrorSetup});
    const Log dynamicSetupErrorLog{{1, Handler::Setup},
                                  {1, Handler::Done},
                                  {-1, Handler::GroupError}};
    QTest::newRow("DynamicSetupError") << dynamicSetupErrorRoot << dynamicSetupErrorLog
                                       << true << false << 4;

    const Group dynamicSetupAllRoot = constructDynamicSetup({continueAllSetup});
    const Log dynamicSetupAllLog{{1, Handler::Setup},
                                 {1, Handler::Done},
                                 {2, Handler::Setup},
                                 {2, Handler::Done},
                                 {3, Handler::Setup},
                                 {3, Handler::Done},
                                 {4, Handler::Setup},
                                 {4, Handler::Done},
                                 {-1, Handler::GroupDone}};
    QTest::newRow("DynamicSetupAll") << dynamicSetupAllRoot << dynamicSetupAllLog
                                     << true << true << 4;

    const Group dynamicSetupSelRoot = constructDynamicSetup({continueSelSetup});
    const Log dynamicSetupSelLog{{1, Handler::Setup},
                                 {1, Handler::Done},
                                 {2, Handler::Setup},
                                 {2, Handler::Done},
                                 {4, Handler::Setup},
                                 {4, Handler::Done},
                                 {-1, Handler::GroupDone}};
    QTest::newRow("DynamicSetupSelected") << dynamicSetupSelRoot << dynamicSetupSelLog
                                          << true << true << 4;

}

void tst_TaskTree::processTree()
{
    m_log = {};

    QFETCH(Group, root);
    QFETCH(Log, expectedLog);
    QFETCH(bool, runningAfterStart);
    QFETCH(bool, success);
    QFETCH(int, taskCount);

    QEventLoop eventLoop;
    TaskTree processTree(root);
    QCOMPARE(processTree.taskCount(), taskCount);
    int doneCount = 0;
    int errorCount = 0;
    connect(&processTree, &TaskTree::done, this, [&doneCount, &eventLoop] { ++doneCount; eventLoop.quit(); });
    connect(&processTree, &TaskTree::errorOccurred, this, [&errorCount, &eventLoop] { ++errorCount; eventLoop.quit(); });
    processTree.start();
    QCOMPARE(processTree.isRunning(), runningAfterStart);

    if (runningAfterStart) {
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
        QCOMPARE(processTree.isRunning(), false);
    }

    QCOMPARE(processTree.progressValue(), taskCount);
    QCOMPARE(m_log, expectedLog);

    const int expectedDoneCount = success ? 1 : 0;
    const int expectedErrorCount = success ? 0 : 1;
    QCOMPARE(doneCount, expectedDoneCount);
    QCOMPARE(errorCount, expectedErrorCount);
}

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
static Log s_log;

void tst_TaskTree::storage_data()
{
    using namespace std::placeholders;

    QTest::addColumn<Group>("root");
    QTest::addColumn<TreeStorage<CustomStorage>>("storageLog");
    QTest::addColumn<Log>("expectedLog");
    QTest::addColumn<bool>("runningAfterStart");
    QTest::addColumn<bool>("success");

    TreeStorage<CustomStorage> storageLog;

    const auto setupProcessHelper = [storageLog, testAppPath = m_testAppPath]
            (QtcProcess &process, const QStringList &args, int processId) {
        process.setCommand(CommandLine(testAppPath, args));
        process.setProperty(s_processIdProperty, processId);
        storageLog->m_log.append({processId, Handler::Setup});
    };
    const auto setupProcess = [setupProcessHelper](QtcProcess &process, int processId) {
        setupProcessHelper(process, {"-return", "0"}, processId);
    };
    const auto readResult = [storageLog](const QtcProcess &process) {
        const int processId = process.property(s_processIdProperty).toInt();
        storageLog->m_log.append({processId, Handler::Done});
    };
    const auto groupSetup = [storageLog](int processId) {
        storageLog->m_log.append({processId, Handler::GroupSetup});
    };
    const auto groupDone = [storageLog](int processId) {
        storageLog->m_log.append({processId, Handler::GroupDone});
    };
    const auto rootDone = [storageLog] {
        storageLog->m_log.append({-1, Handler::GroupDone});
        s_log = storageLog->m_log;
    };

    const Log expectedLog{{1, Handler::GroupSetup},
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
                          {-1, Handler::GroupDone}};

    const Group root {
        Storage(storageLog),
        Group {
            Group {
                Group {
                    Group {
                        Group {
                            Process(std::bind(setupProcess, _1, 5), readResult),
                            OnGroupSetup(std::bind(groupSetup, 5)),
                            OnGroupDone(std::bind(groupDone, 5))
                        },
                        OnGroupSetup(std::bind(groupSetup, 4)),
                        OnGroupDone(std::bind(groupDone, 4))
                    },
                    OnGroupSetup(std::bind(groupSetup, 3)),
                    OnGroupDone(std::bind(groupDone, 3))
                },
                OnGroupSetup(std::bind(groupSetup, 2)),
                OnGroupDone(std::bind(groupDone, 2))
            },
            OnGroupSetup(std::bind(groupSetup, 1)),
            OnGroupDone(std::bind(groupDone, 1))
        },
        OnGroupDone(rootDone)
    };

    QTest::newRow("Storage") << root << storageLog << expectedLog << true << true;
}

void tst_TaskTree::storage()
{
    QFETCH(Group, root);
    QFETCH(TreeStorage<CustomStorage>, storageLog);
    QFETCH(Log, expectedLog);
    QFETCH(bool, runningAfterStart);
    QFETCH(bool, success);

    s_log.clear();

    QVERIFY(storageLog.isValid());
    QCOMPARE(CustomStorage::instanceCount(), 0);

    QEventLoop eventLoop;
    TaskTree processTree(root);
    int doneCount = 0;
    int errorCount = 0;
    connect(&processTree, &TaskTree::done, this, [&doneCount, &eventLoop] { ++doneCount; eventLoop.quit(); });
    connect(&processTree, &TaskTree::errorOccurred, this, [&errorCount, &eventLoop] { ++errorCount; eventLoop.quit(); });
    processTree.start();
    QCOMPARE(CustomStorage::instanceCount(), 1);
    QCOMPARE(processTree.isRunning(), runningAfterStart);

    QTimer timer;
    connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    timer.setInterval(2000);
    timer.setSingleShot(true);
    timer.start();
    eventLoop.exec();

    QVERIFY(!processTree.isRunning());
    QCOMPARE(s_log, expectedLog);
    QCOMPARE(CustomStorage::instanceCount(), 0);

    const int expectedDoneCount = success ? 1 : 0;
    const int expectedErrorCount = success ? 0 : 1;
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

void tst_TaskTree::storageDestructor()
{
    QCOMPARE(CustomStorage::instanceCount(), 0);
    {
        const auto setupProcess = [this](QtcProcess &process) {
            process.setCommand(CommandLine(m_testAppPath, {"-sleep", "1"}));
        };
        const Group root {
            Storage(TreeStorage<CustomStorage>()),
            Process(setupProcess)
        };

        TaskTree processTree(root);
        QCOMPARE(CustomStorage::instanceCount(), 0);
        processTree.start();
        QCOMPARE(CustomStorage::instanceCount(), 1);
    }
    QCOMPARE(CustomStorage::instanceCount(), 0);
}

QTEST_GUILESS_MAIN(tst_TaskTree)

#include "tst_tasktree.moc"
