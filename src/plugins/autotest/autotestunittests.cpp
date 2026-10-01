// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "autotestunittests.h"

#include "externaltestrun.h"
#include "testcodeparser.h"
#include "testoutputreader.h"
#include "testrunner.h"
#include "testsettings.h"
#include "testtreemodel.h"

#include "boost/boosttestconfiguration.h"
#include "boost/boosttestframework.h"
#include "catch/catchconfiguration.h"
#include "catch/catchtestframework.h"
#include "gtest/gtestconfiguration.h"
#include "gtest/gtestframework.h"
#include "gtest/gtestoutputreader.h"
#include "qtest/qttest_utils.h"
#include "qtest/qttestconfiguration.h"
#include "qtest/qttestframework.h"
#include "quick/quicktestconfiguration.h"
#include "quick/quicktestframework.h"

#include <cppeditor/cpptoolstestcase.h>
#include <cppeditor/projectinfo.h>

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/environment.h>

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTest>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Autotest::Internal {

class AutotestUnitTests : public QObject
{
    Q_OBJECT

public:
    AutotestUnitTests()
        : m_model(TestTreeModel::instance())
    {}

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCodeParser();
    void testCodeParser_data();
    void testCodeParserSwitchStartup();
    void testCodeParserSwitchStartup_data();
    void testCodeParserGTest();
    void testCodeParserGTest_data();
    void testCodeParserBoostTest();
    void testCodeParserBoostTest_data();

private:
    TestTreeModel *m_model = nullptr;
    CppEditor::Tests::TemporaryCopiedDir *m_tmpDir = nullptr;
    bool m_isQt4 = false;
    bool m_checkBoost = false;
    ProjectExplorer::Kit *m_kit = nullptr;
};


void AutotestUnitTests::initTestCase()
{
    const QList<Kit *> allKits = KitManager::kits();
    if (allKits.count() == 0)
        QSKIP("This test requires at least one kit to be present");

    m_kit = findOr(allKits, nullptr, [](Kit *k) {
            return k->isValid() && QtSupport::QtKitAspect::qtVersion(k) != nullptr;
    });
    if (!m_kit)
        QSKIP("The test requires at least one valid kit with a valid Qt");

    if (auto qtVersion = QtSupport::QtKitAspect::qtVersion(m_kit))
        m_isQt4 = qtVersion->qtVersionString().startsWith('4');
    else
        QSKIP("Could not figure out which Qt version is used for default kit.");
    const Toolchain * const toolchain = ToolchainKitAspect::cxxToolchain(m_kit);
    if (!toolchain)
        QSKIP("This test requires that there is a kit with a toolchain.");

    m_tmpDir = new CppEditor::Tests::TemporaryCopiedDir(":/unit_test");

    if (!qtcEnvironmentVariableIsEmpty("BOOST_INCLUDE_DIR")) {
        m_checkBoost = true;
    } else {
        if (HostOsInfo::isLinuxHost()
                && (QFileInfo::exists("/usr/include/boost/version.hpp")
                    || QFileInfo::exists("/usr/local/include/boost/version.hpp"))) {
            qDebug() << "Found boost at system level - will run boost parser test.";
            m_checkBoost = true;
        }
    }

    // Enable quick check for derived tests
    theQtTestFramework().quickCheckForDerivedTests.setValue(true);
}

void AutotestUnitTests::cleanupTestCase()
{
    delete m_tmpDir;
}

void AutotestUnitTests::testCodeParser()
{
    QFETCH(FilePath, projectFilePath);
    QFETCH(int, expectedAutoTestsCount);
    QFETCH(int, expectedNamedQuickTestsCount);
    QFETCH(int, expectedUnnamedQuickTestsCount);
    QFETCH(int, expectedDataTagsCount);

    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(projectFilePath, m_kit));

    QSignalSpy parserSpy(m_model->parser(), &TestCodeParser::parsingFinished);
    QSignalSpy modelUpdateSpy(m_model, &TestTreeModel::sweepingDone);
    QVERIFY(parserSpy.wait(20000));
    QVERIFY(modelUpdateSpy.wait());

    if (m_isQt4)
        expectedNamedQuickTestsCount = expectedUnnamedQuickTestsCount = 0;

    QCOMPARE(m_model->autoTestsCount(), expectedAutoTestsCount);
    QCOMPARE(m_model->namedQuickTestsCount(), expectedNamedQuickTestsCount);
    QCOMPARE(m_model->unnamedQuickTestsCount(), expectedUnnamedQuickTestsCount);
    QCOMPARE(m_model->dataTagsCount(), expectedDataTagsCount);
}

void AutotestUnitTests::testCodeParser_data()
{
    QTest::addColumn<FilePath>("projectFilePath");
    QTest::addColumn<int>("expectedAutoTestsCount");
    QTest::addColumn<int>("expectedNamedQuickTestsCount");
    QTest::addColumn<int>("expectedUnnamedQuickTestsCount");
    QTest::addColumn<int>("expectedDataTagsCount");

    const FilePath base = m_tmpDir->filePath();
    QTest::newRow("plainAutoTest")
            << base / "plain/plain.pro"
            << 1 << 0 << 0 << 0;
    QTest::newRow("mixedAutoTestAndQuickTests")
            << base / "mixed_atp/mixed_atp.pro"
            << 4 << 10 << 5 << 10;
    // the test is declared in a library the application target links against
    QTest::newRow("libraryAutoTest")
            << base / "lib_atp/lib_atp.pro"
            << 1 << 0 << 0 << 0;
    QTest::newRow("plainAutoTestQbs")
            << base / "plain/plain.qbs"
            << 1 << 0 << 0 << 0;
    QTest::newRow("mixedAutoTestAndQuickTestsQbs")
            << base / "mixed_atp/mixed_atp.qbs"
            << 4 << 10 << 5 << 10;
    QTest::newRow("libraryAutoTestQbs")
            << base / "lib_atp/lib_atp.qbs"
            << 1 << 0 << 0 << 0;
}

void AutotestUnitTests::testCodeParserSwitchStartup()
{
    QFETCH(FilePaths, projectFilePaths);
    QFETCH(QList<int>, expectedAutoTestsCount);
    QFETCH(QList<int>, expectedNamedQuickTestsCount);
    QFETCH(QList<int>, expectedUnnamedQuickTestsCount);
    QFETCH(QList<int>, expectedDataTagsCount);

    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    for (int i = 0; i < projectFilePaths.size(); ++i) {
        qDebug() << "Opening project" << projectFilePaths.at(i);
        QVERIFY(projectManager.open(projectFilePaths.at(i), m_kit));

        QSignalSpy parserSpy(m_model->parser(), &TestCodeParser::parsingFinished);
        QSignalSpy modelUpdateSpy(m_model, &TestTreeModel::sweepingDone);
        QVERIFY(parserSpy.wait(20000));
        QVERIFY(modelUpdateSpy.wait());

        QCOMPARE(m_model->autoTestsCount(), expectedAutoTestsCount.at(i));
        QCOMPARE(m_model->namedQuickTestsCount(),
                 m_isQt4 ? 0 : expectedNamedQuickTestsCount.at(i));
        QCOMPARE(m_model->unnamedQuickTestsCount(),
                 m_isQt4 ? 0 : expectedUnnamedQuickTestsCount.at(i));
        QCOMPARE(m_model->dataTagsCount(),
                 expectedDataTagsCount.at(i));
    }
}

void AutotestUnitTests::testCodeParserSwitchStartup_data()
{
    QTest::addColumn<FilePaths>("projectFilePaths");
    QTest::addColumn<QList<int> >("expectedAutoTestsCount");
    QTest::addColumn<QList<int> >("expectedNamedQuickTestsCount");
    QTest::addColumn<QList<int> >("expectedUnnamedQuickTestsCount");
    QTest::addColumn<QList<int> >("expectedDataTagsCount");

    const FilePath base = m_tmpDir->filePath();
    FilePaths projects {
        base / "plain/plain.pro",
        base / "mixed_atp/mixed_atp.pro",
        base / "plain/plain.qbs",
        base / "mixed_atp/mixed_atp.qbs"
    };

    QList<int> expectedAutoTests = QList<int>()         << 1 << 4 << 1 << 4;
    QList<int> expectedNamedQuickTests = QList<int>()   << 0 << 10 << 0 << 10;
    QList<int> expectedUnnamedQuickTests = QList<int>() << 0 << 5 << 0 << 5;
    QList<int> expectedDataTagsCount = QList<int>()     << 0 << 10 << 0 << 10;

    QTest::newRow("loadMultipleProjects")
            << projects << expectedAutoTests << expectedNamedQuickTests
            << expectedUnnamedQuickTests << expectedDataTagsCount;
}

void AutotestUnitTests::testCodeParserGTest()
{
    if (qtcEnvironmentVariableIsEmpty("GOOGLETEST_DIR")) {
        const QString qcSource = QString(QTCREATORDIR);
        const FilePath gtestSrc = FilePath::fromUserInput(qcSource)
                                      .pathAppended("src/libs/3rdparty/googletest");
        if (gtestSrc.exists()) {
            qDebug() << "Trying to use googletest submodule in" << gtestSrc.toUserOutput() << ".";
            Environment::modifySystemEnvironment({EnvironmentItem{"GOOGLETEST_DIR",
                                                                  gtestSrc.toUserOutput()}});
        } else {
            QSKIP("This test needs googletest - set GOOGLETEST_DIR (point to googletest repository)");
        }
    }

    QFETCH(FilePath, projectFilePath);
    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(projectFilePath, m_kit));

    QSignalSpy parserSpy(m_model->parser(), &TestCodeParser::parsingFinished);
    QSignalSpy modelUpdateSpy(m_model, &TestTreeModel::sweepingDone);
    QVERIFY(parserSpy.wait(20000));
    QVERIFY(modelUpdateSpy.wait());

    QCOMPARE(m_model->gtestNamesCount(), 9);

    QMultiMap<QString, int> expectedNamesAndSets;
    expectedNamesAndSets.insert(QStringLiteral("FactorialTest"), 3);
    expectedNamesAndSets.insert(QStringLiteral("FactorialTest_Iterative"), 2);
    expectedNamesAndSets.insert(QStringLiteral("Sum"), 2);
    expectedNamesAndSets.insert(QStringLiteral("QueueTest"), 2);
    expectedNamesAndSets.insert(QStringLiteral("DummyTest"), 1); // used as parameterized test
    expectedNamesAndSets.insert(QStringLiteral("DummyTest"), 1); // used as 'normal' test
    expectedNamesAndSets.insert(QStringLiteral("NumberAsNameStart"), 1);
    expectedNamesAndSets.insert(QStringLiteral("NamespaceTest"), 1);
    expectedNamesAndSets.insert(QStringLiteral("InLibTest"), 2); // in a linked library

    QMultiMap<QString, int> foundNamesAndSets = m_model->gtestNamesAndSets();
    QCOMPARE(expectedNamesAndSets.size(), foundNamesAndSets.size());
    for (const QString &name : expectedNamesAndSets.keys())
        QCOMPARE(expectedNamesAndSets.values(name), foundNamesAndSets.values(name));

    // check also that no Qt related tests have been found
    QCOMPARE(m_model->autoTestsCount(), 0);
    QCOMPARE(m_model->namedQuickTestsCount(), 0);
    QCOMPARE(m_model->unnamedQuickTestsCount(), 0);
    QCOMPARE(m_model->dataTagsCount(), 0);
    QCOMPARE(m_model->boostTestNamesCount(), 0);
}

void AutotestUnitTests::testCodeParserGTest_data()
{
    QTest::addColumn<FilePath>("projectFilePath");
    QTest::newRow("simpleGoogletest")
        << m_tmpDir->filePath() / "simple_gt/simple_gt.pro";
    QTest::newRow("simpleGoogletestQbs")
        << m_tmpDir->filePath() / "simple_gt/simple_gt.qbs";
}

void AutotestUnitTests::testCodeParserBoostTest()
{
    if (!m_checkBoost)
        QSKIP("This test needs boost - set BOOST_INCLUDE_DIR (or have it installed)");

    QFETCH(FilePath, projectFilePath);
    QFETCH(QString, extension);
    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    const CppEditor::ProjectInfo::ConstPtr projectInfo
            = projectManager.open(projectFilePath, m_kit);
    QVERIFY(projectInfo);

    QSignalSpy parserSpy(m_model->parser(), &TestCodeParser::parsingFinished);
    QSignalSpy modelUpdateSpy(m_model, &TestTreeModel::sweepingDone);
    QVERIFY(parserSpy.wait(20000));
    QVERIFY(modelUpdateSpy.wait());

    QCOMPARE(m_model->boostTestNamesCount(), 5);

    const FilePath basePath = projectInfo->projectRoot();
    QVERIFY(!basePath.isEmpty());

    QMap<QString, int> expectedSuitesAndTests;

    auto pathConstructor = [basePath, extension](const QString &name, const QString &subPath) {
        return QString(name + '|' + basePath.pathAppended(subPath + extension).toUrlishString());
    };
    expectedSuitesAndTests.insert(pathConstructor("Master Test Suite", "tests/deco/deco"), 2); // decorators w/o suite
    expectedSuitesAndTests.insert(pathConstructor("Master Test Suite", "tests/fix/fix"), 2); // fixtures
    expectedSuitesAndTests.insert(pathConstructor("Master Test Suite", "tests/params/params"), 3); // functions
    expectedSuitesAndTests.insert(pathConstructor("Suite1", "tests/deco/deco"), 4);
    expectedSuitesAndTests.insert(pathConstructor("SuiteOuter", "tests/deco/deco"), 5); // 2 sub suites + 3 tests

    QMap<QString, int> foundNamesAndSets = m_model->boostTestSuitesAndTests();
    QCOMPARE(expectedSuitesAndTests.size(), foundNamesAndSets.size());
    for (auto it = expectedSuitesAndTests.cbegin(); it != expectedSuitesAndTests.cend(); ++it)
        QCOMPARE(*it, foundNamesAndSets.value(it.key()));

    // check also that no Qt related tests have been found
    QCOMPARE(m_model->autoTestsCount(), 0);
    QCOMPARE(m_model->namedQuickTestsCount(), 0);
    QCOMPARE(m_model->unnamedQuickTestsCount(), 0);
    QCOMPARE(m_model->dataTagsCount(), 0);
    QCOMPARE(m_model->gtestNamesCount(), 0);
}

void AutotestUnitTests::testCodeParserBoostTest_data()
{
    QTest::addColumn<FilePath>("projectFilePath");
    QTest::addColumn<QString>("extension");
    QTest::newRow("simpleBoostTest")
        << m_tmpDir->filePath() / "simple_boost/simple_boost.pro" << QString(".pro");
    QTest::newRow("simpleBoostTestQbs")
        << m_tmpDir->filePath() / "simple_boost/simple_boost.qbs" << QString(".qbs");
}

class ExternalTestRunTest : public QObject
{
    Q_OBJECT

private slots:
    void testResultNesting();
};

void ExternalTestRunTest::testResultNesting()
{
    QList<TestResult> results;
    const QMetaObject::Connection connection
        = connect(TestRunner::instance(), &TestRunner::testResultReady,
                  this, [&results](const TestResult &result) { results.append(result); });

    {
        ExternalTestRun run("suite");
        if (!run.isRunning()) {
            disconnect(connection);
            QSKIP("Another test run is going on");
        }
        run.reportResult("aTest", ResultType::TestStart);
        run.reportResult("aTest", ResultType::Pass);
    }
    disconnect(connection);

    QCOMPARE(results.size(), 2);
    bool needsIntermediate = false;
    // The start of a test takes what the test says about itself...
    QVERIFY(results.at(0).isDirectParentOf(results.at(1), &needsIntermediate));
    // ...but an outcome takes nothing, or the tree would nest ever deeper.
    QVERIFY(!results.at(1).isDirectParentOf(results.at(0), &needsIntermediate));
}

class QtTestUtilsTest : public QObject
{
    Q_OBJECT

private slots:
    void testFilterInterfering();
    void testFilterInterfering_data();
    void testFilterInterferingWithoutOmitted();
};

void QtTestUtilsTest::testFilterInterfering()
{
    QFETCH(QStringList, provided);
    QFETCH(QStringList, expectedAllowed);
    QFETCH(QStringList, expectedOmitted);

    QStringList omitted;
    const QStringList allowed = QTestUtils::filterInterfering(provided, &omitted, false);
    QCOMPARE(allowed, expectedAllowed);
    QCOMPARE(omitted, expectedOmitted);
}

void QtTestUtilsTest::testFilterInterfering_data()
{
    QTest::addColumn<QStringList>("provided");
    QTest::addColumn<QStringList>("expectedAllowed");
    QTest::addColumn<QStringList>("expectedOmitted");

    QTest::newRow("empty") << QStringList() << QStringList() << QStringList();
    QTest::newRow("allowed with parameter")
        << QStringList{"-iterations", "5"} << QStringList{"-iterations", "5"} << QStringList();
    QTest::newRow("interfering single")
        << QStringList{"-silent"} << QStringList() << QStringList{"-silent"};
    // logging to a file is supported, logging to stdout would collide with the plugin's own
    QTest::newRow("new style logging to a file is allowed")
        << QStringList{"-o", "/tmp/out,xml"} << QStringList{"-o", "/tmp/out,xml"} << QStringList();
    QTest::newRow("new style logging to stdout is interfering")
        << QStringList{"-o", "-,xml"} << QStringList() << QStringList{"-o", "-,xml"};
    QTest::newRow("unknown output format is interfering")
        << QStringList{"-o", "/tmp/out"} << QStringList() << QStringList{"-o", "/tmp/out"};
    QTest::newRow("several loggings to files")
        << QStringList{"-o", "/tmp/a,xml", "-o", "/tmp/b,tap"}
        << QStringList{"-o", "/tmp/a,xml", "-o", "/tmp/b,tap"} << QStringList();
    // a trailing option without its parameter drops that option, not the whole list
    QTest::newRow("trailing output option")
        << QStringList{"-iterations", "5", "-o"}
        << QStringList{"-iterations", "5"} << QStringList{"-o"};
    QTest::newRow("trailing allowed option with parameter")
        << QStringList{"-silent", "-iterations"}
        << QStringList() << QStringList{"-silent", "-iterations"};
    QTest::newRow("trailing interfering option with parameter")
        << QStringList{"-o", "/tmp/out,xml", "-maxwarnings"}
        << QStringList{"-o", "/tmp/out,xml"} << QStringList{"-maxwarnings"};
}

// The parameter of an interfering option has to be consumed even when there is nowhere to
// report it, or it is read as the next option and allowed through for matching nothing.
void QtTestUtilsTest::testFilterInterferingWithoutOmitted()
{
    QCOMPARE(QTestUtils::filterInterfering({"-maxwarnings", "500"}, nullptr, false), QStringList());
    QCOMPARE(QTestUtils::filterInterfering({"-maxwarnings", "500", "-iterations", "5"},
                                           nullptr, false),
             QStringList({"-iterations", "5"}));
}

class TestOutputReaderTest : public QObject
{
    Q_OBJECT

private slots:
    void testColorRemoval();
    void testColorRemoval_data();
    void testBoundedAppend();
    void testGTestDescriptions();
    void testGTestDescriptions_data();
};

void TestOutputReaderTest::testColorRemoval()
{
    QFETCH(QString, original);
    QFETCH(QString, expected);

    QCOMPARE(TestOutputReader::removeCommandlineColors(original), expected);
}

void TestOutputReaderTest::testColorRemoval_data()
{
    QTest::addColumn<QString>("original");
    QTest::addColumn<QString>("expected");

    const QString esc(QChar(0x1B));

    QTest::newRow("empty") << QString() << QString();
    QTest::newRow("nothing to do") << QString("plain text") << QString("plain text");
    QTest::newRow("single") << esc + "[31mFAIL" + esc + "[0m" << QString("FAIL");
    QTest::newRow("reset only") << esc + "[m" << QString();
    QTest::newRow("bare escape") << esc + "text" << esc + "text";
    QTest::newRow("unterminated") << esc + "[31 no end" << esc + "[31 no end";
    QTest::newRow("no terminator before newline")
        << esc + "[31\n" + esc + "[0mx" << esc + "[31\nx";
    // removing a sequence can put a leftover escape next to a following bracket, and the result
    // is a sequence that was not in the input
    QTest::newRow("removal creates a sequence")
        << esc + esc + "[a2m[2m;" << QString(";");
    QTest::newRow("adjacent") << esc + "[1m" + esc + "[31mx" << QString("x");
}

void TestOutputReaderTest::testBoundedAppend()
{
    const QString line(1024, 'x');
    QString accumulated;
    for (int i = 0; i < 4096; ++i)
        TestOutputReader::appendBounded(accumulated, line);

    // bounded, and it says so where a reader of the result will see it
    QVERIFY(accumulated.size() < 4096 * (line.size() + 1));
    QVERIFY(accumulated.contains("truncated"));

    // once truncated it stays put, and clearing starts over
    const QString truncated = accumulated;
    TestOutputReader::appendBounded(accumulated, line);
    QCOMPARE(accumulated, truncated);

    accumulated.clear();
    TestOutputReader::appendBounded(accumulated, "one");
    QCOMPARE(accumulated, QString("one"));

    // check for append without separating
    accumulated.clear();
    TestOutputReader::appendBounded(accumulated, "a");
    TestOutputReader::appendBounded(accumulated, "b", false);
    QCOMPARE(accumulated, "ab");
}

static QString resultTypeName(ResultType type)
{
    switch (type) {
    case ResultType::Pass: return "Pass";
    case ResultType::Fail: return "Fail";
    case ResultType::Skip: return "Skip";
    case ResultType::MessageLocation: return "Location";
    default: return {};
    }
}

void TestOutputReaderTest::testGTestDescriptions()
{
    QFETCH(QStringList, output);
    QFETCH(QStringList, expected);

    GTestOutputReader reader(nullptr, {}, {});
    QStringList reported;
    connect(&reader, &TestOutputReader::newResult, this, [&reported](const TestResult &result) {
        const QString type = resultTypeName(result.result());
        if (!type.isEmpty())
            reported << type + ": " + result.description();
    });
    for (const QString &line : std::as_const(output))
        reader.processStdOutput(line.toLatin1());

    QCOMPARE(reported, expected);
}

void TestOutputReaderTest::testGTestDescriptions_data()
{
    QTest::addColumn<QStringList>("output");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("pass")
        << QStringList{"[ RUN      ] Suite.Test", "some output", "more output",
                       "[       OK ] Suite.Test (0 ms)"}
        << QStringList{"Pass: some output\nmore output"};
    QTest::newRow("fail")
        << QStringList{"[ RUN      ] Suite.Test", "some output", "more output",
                       "[  FAILED  ] Suite.Test (0 ms)"}
        << QStringList{"Fail: some output\nmore output"};
    QTest::newRow("fail with location")
        << QStringList{"[ RUN      ] Suite.Test", "foo.cpp:12: Failure",
                       "Expected equality of these values:", "  a", "    Which is: 1", "  b",
                       "    Which is: 2", "[  FAILED  ] Suite.Test (0 ms)"}
        << QStringList{"Fail: ",
                       "Location: foo.cpp:12: Failure\nExpected equality of these values:\n"
                       "  a\n    Which is: 1\n  b\n    Which is: 2"};
    QTest::newRow("skip")
        << QStringList{"[ RUN      ] Suite.Test", "foo.cpp:12: Skipped", "the reason",
                       "[  SKIPPED ] Suite.Test (0 ms)"}
        << QStringList{"Skip: Suite.Test\nfoo.cpp:12: Skipped\nthe reason"};
}

enum Framework { QtTest, QuickTest, GTest, CatchTest, BoostTest };

static std::unique_ptr<TestConfiguration> createConfiguration(int framework,
                                                              const QStringList &testCases)
{
    std::unique_ptr<TestConfiguration> config;
    switch (framework) {
    case QtTest: config = std::make_unique<QtTestConfiguration>(&theQtTestFramework()); break;
    case QuickTest:
        config = std::make_unique<QuickTestConfiguration>(&theQuickTestFramework());
        break;
    case GTest: config = std::make_unique<GTestConfiguration>(&theGTestFramework()); break;
    case CatchTest: config = std::make_unique<CatchConfiguration>(&theCatchFramework()); break;
    case BoostTest:
        config = std::make_unique<BoostTestConfiguration>(&theBoostTestFramework());
        break;
    }
    if (config)
        config->setTestCases(testCases);
    return config;
}

// Guards the contract that the frameworks hand out raw, unquoted arguments and that
// TestConfiguration::commandLine() is the only place that quotes them. Both halves of that
// contract are load-bearing: a test case name is project data, and an argument that survives
// quoting as several arguments - or as one carrying literal quote characters - reaches the test
// executable, or a shell, as something other than what the tree displayed.
// Catch is the exception: it hands out one test specification, whose quote characters are
// Catch's own syntax, and that has to reach the test executable as it is.
class TestArgumentsTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCaseArgumentsSurviveQuoting();
    void testCaseArgumentsSurviveQuoting_data();
    void testRunConfigurationArguments();
    void testRunConfigurationArguments_data();
    void testRunConfigurationArgumentsOnDevice();
    void testRunConfigurationArgumentsOnDevice_data();
    void testFrameworkRunConfigurationArguments();
    void testFrameworkRunConfigurationArguments_data();

private:
    bool m_processArgs = false;
};

void TestArgumentsTest::initTestCase()
{
    // argumentsForTestRunner() only consults runnable() when this is on, and these
    // configurations deliberately have no project
    m_processArgs = testSettings().processArgs();
    testSettings().processArgs.setValue(false);
}

void TestArgumentsTest::cleanupTestCase()
{
    testSettings().processArgs.setValue(m_processArgs);
}

void TestArgumentsTest::testCaseArgumentsSurviveQuoting()
{
    QFETCH(int, framework);
    QFETCH(QStringList, testCases);
    QFETCH(QStringList, expected);

    const std::unique_ptr<TestConfiguration> config = createConfiguration(framework, testCases);
    QVERIFY(config);

    const CommandLine command = config->commandLine();
    // same unquoting Process applies, including the cmd-level ^ escapes on Windows
    const QStringList arguments = ProcessArgs::splitArgs(command.arguments(),
                                                         command.executable().osType(), true);

    // the expected arguments appear as a contiguous run of whole arguments
    int at = arguments.indexOf(expected.first());
    QVERIFY2(at != -1, qPrintable("not found: " + expected.first() + " in " + arguments.join('|')));
    QCOMPARE(arguments.mid(at, expected.size()), expected);

    if (framework == CatchTest)
        return;
    for (const QString &argument : arguments)
        QVERIFY2(!argument.contains('"'), qPrintable("quote character in: " + argument));
}

void TestArgumentsTest::testCaseArgumentsSurviveQuoting_data()
{
    QTest::addColumn<int>("framework");
    QTest::addColumn<QStringList>("testCases");
    QTest::addColumn<QStringList>("expected");

    const QString tagWithSpace("tst_Foo::parse data:a tag");
    const QString tagWithTab("tst_Foo::parse data:a\ttag&id");
    const QString tagWithMeta("tst_Foo::parse data:$(id);x");

    QTest::newRow("qttest plain")
        << int(QtTest) << QStringList{"tst_Foo::simple"} << QStringList{"tst_Foo::simple"};
    QTest::newRow("qttest space")
        << int(QtTest) << QStringList{tagWithSpace} << QStringList{tagWithSpace};
    QTest::newRow("qttest tab and ampersand")
        << int(QtTest) << QStringList{tagWithTab} << QStringList{tagWithTab};
    QTest::newRow("qttest shell metacharacters")
        << int(QtTest) << QStringList{tagWithMeta} << QStringList{tagWithMeta};
    QTest::newRow("qttest several")
        << int(QtTest) << QStringList{tagWithSpace, tagWithTab}
        << QStringList{tagWithSpace, tagWithTab};
    QTest::newRow("quicktest space")
        << int(QuickTest) << QStringList{tagWithSpace} << QStringList{tagWithSpace};
    QTest::newRow("gtest space")
        << int(GTest) << QStringList{"Suite.a case", "Other.b"}
        << QStringList{"--gtest_filter=Suite.a case:Other.b"};
    QTest::newRow("catch space")
        << int(CatchTest) << QStringList{"a case", "another"}
        << QStringList{"\"a case\", \"another\""};
    QTest::newRow("catch escaped comma")
        << int(CatchTest) << QStringList{"a\\, case"} << QStringList{"\"a\\, case\""};
    QTest::newRow("catch shell metacharacters")
        << int(CatchTest) << QStringList{"a $(id);\tcase"} << QStringList{"\"a $(id);\tcase\""};
    QTest::newRow("boost space")
        << int(BoostTest) << QStringList{"suite/a case"} << QStringList{"-t", "suite/a case"};
    QTest::newRow("boost several")
        << int(BoostTest) << QStringList{"suite/a case", "suite/b"}
        << QStringList{"-t", "suite/a case", "-t", "suite/b"};
}

// Lets a configuration carry run configuration arguments without a project behind it.
template<typename Configuration>
class ArgumentCarryingConfiguration : public Configuration
{
public:
    ArgumentCarryingConfiguration(ITestFramework *framework, const QString &arguments,
                                  const FilePath &executable = FilePath::fromString("/bin/tst_x"))
        : Configuration(framework)
    {
        ProcessRunData &runnable = this->m_runnable;
        runnable.command = CommandLine(executable, arguments, CommandLine::Raw);
        Environment env = Environment::systemEnvironment();
        env.set("TST_LOG_DIR",
                QLatin1String(HostOsInfo::isWindowsHost() ? "C:\\logs" : "/logs"));
        env.set("TST_LOG_DIR_WITH_SPACE",
                QLatin1String(HostOsInfo::isWindowsHost() ? "C:\\logs\\with space"
                                                          : "/logs/with space"));
        runnable.environment = env;
        // does not exist, so the test runs in the executable's directory
        runnable.workingDirectory = FilePath::fromString("/nonexistent/tst_x_dir");
    }
};

// The run configuration's arguments are a command line the user wrote, so they have to be read as
// one. Splitting on spaces cut a quoted path in half and left the quote characters attached, so
// what the filter judged was fragments rather than arguments.
void TestArgumentsTest::testRunConfigurationArguments()
{
    QFETCH(QString, arguments);
    QFETCH(QStringList, expectedAllowed);
    QFETCH(QStringList, expectedOmitted);

    testSettings().processArgs.setValue(true);
    ArgumentCarryingConfiguration<QtTestConfiguration> config(&theQtTestFramework(), arguments);
    QStringList omitted;
    const QStringList result = config.argumentsForTestRunner(&omitted);
    testSettings().processArgs.setValue(false);

    // the framework's own options follow the ones taken from the run configuration
    QCOMPARE(result.mid(0, expectedAllowed.size()), expectedAllowed);
    QCOMPARE(omitted, expectedOmitted);
}

void TestArgumentsTest::testRunConfigurationArguments_data()
{
    QTest::addColumn<QString>("arguments");
    QTest::addColumn<QStringList>("expectedAllowed");
    QTest::addColumn<QStringList>("expectedOmitted");

    QTest::newRow("plain") << "-iterations 5" << QStringList{"-iterations", "5"} << QStringList();
    // a quoted path stays one argument, so the option is judged on its real parameter
    QTest::newRow("quoted path with a space")
        << "-o \"/tmp/my dir/log,xml\""
        << QStringList{"-o", "/tmp/my dir/log,xml"} << QStringList();
    const bool isWin = HostOsInfo::isWindowsHost();
    // passing an environment variable should expand it on command line
    QTest::newRow("env var in option param")
        << (isWin ? "-o %TST_LOG_DIR%\\log,xml" : "-o $TST_LOG_DIR/log,xml")
        << QStringList{"-o",
           QString("%1log,xml").arg(QLatin1String(isWin ? "C:\\logs\\" : "/logs/"))}
        << QStringList{};
    QTest::newRow("env var with space in option param")
        << (isWin ? "-o %TST_LOG_DIR_WITH_SPACE%\\log,xml" : "-o $TST_LOG_DIR_WITH_SPACE/log,xml")
        << QStringList{QLatin1String(isWin ? "space\\log,xml" : "space/log,xml")}
        << QStringList{"-o", QLatin1String(isWin ? "C:\\logs\\with" : "/logs/with")};
    QTest::newRow("quoted env var with space in option param")
        << (isWin ? "-o \"%TST_LOG_DIR_WITH_SPACE%\"\\log,xml"
                  : "-o \"$TST_LOG_DIR_WITH_SPACE\"/log,xml")
        << QStringList{"-o",
           QString("%1log,xml").arg(QLatin1String(isWin ? "C:\\logs\\with space\\"
                                                        : "/logs/with space/"))}
        << QStringList{};

    // quoting an option no longer hides it from the filter
    QTest::newRow("quoted option name")
        << "\"-silent\" -iterations 5"
        << QStringList{"-iterations", "5"} << QStringList{"-silent"};
    QTest::newRow("tab separated")
        << "-silent\t-iterations 5" << QStringList{"-iterations", "5"} << QStringList{"-silent"};
    // nothing in the string can be judged, so none of it is passed on - and it is reported
    QTest::newRow("unbalanced quote")
        << "-o \"/tmp/log,xml" << QStringList() << QStringList{"-o \"/tmp/log,xml"};
    // a glob that matches nothing stays literal in a shell, and no shell is run, so glob
    // characters are passed on literally - unless something else in the string needs the shell
    QTest::newRow("glob")
        << "-o /tmp/run*/log,xml" << QStringList{"-o", "/tmp/run*/log,xml"} << QStringList();
    QTest::newRow("quoted glob")
        << "-o \"/tmp/run*/log,xml\"" << QStringList{"-o", "/tmp/run*/log,xml"} << QStringList();
    QTest::newRow("brackets") << "testCase:[tag]" << QStringList{"testCase:[tag]"} << QStringList();
    QTest::newRow("quoted brackets")
        << "\"testCase:[tag]\"" << QStringList{"testCase:[tag]"} << QStringList();
    // a # starts a comment only at the start of a word
    QTest::newRow("hash inside word")
        << "-o /tmp/build#2/log,xml" << QStringList{"-o", "/tmp/build#2/log,xml"}
        << QStringList();
    QTest::newRow("comment")
        << "-iterations 5 # note"
        << (isWin ? QStringList{"-iterations", "5", "#", "note"} : QStringList())
        << (isWin ? QStringList() : QStringList{"-iterations 5 # note"});
    QTest::newRow("brackets and pipe")
        << "testCase:[tag] | tee x" << QStringList() << QStringList{"testCase:[tag] | tee x"};
    QTest::newRow("command substitution")
        << "-o $(pwd)/log,xml" << QStringList() << QStringList{"-o $(pwd)/log,xml"};
    QTest::newRow("backtick substitution")
        << "-o `pwd`/log,xml"
        << (isWin ? QStringList{"-o", "`pwd`/log,xml"} : QStringList())
        << (isWin ? QStringList() : QStringList{"-o `pwd`/log,xml"});
    QTest::newRow("redirection")
        << "-silent 2>err.log" << QStringList() << QStringList{"-silent 2>err.log"};
    QTest::newRow("pipe")
        << "-o /tmp/log,xml | tee x" << QStringList() << QStringList{"-o /tmp/log,xml | tee x"};
    QTest::newRow("command list")
        << "-iterations 5 && rm x" << QStringList() << QStringList{"-iterations 5 && rm x"};
    QTest::newRow("quoted shell syntax")
        << (isWin ? "\"a|b\"" : "\"a|b\" '$(x)'")
        << (isWin ? QStringList{"a|b"} : QStringList{"a|b", "$(x)"}) << QStringList();
    if (isWin) {
        // cmd's escape character is removed, as a normal run does
        QTest::newRow("caret")
            << "-o C:\\a^&b\\log,txt" << QStringList{"-o", "C:\\a&b\\log,txt"}
            << QStringList();
        QTest::newRow("escaped quote")
            << "-o " + ProcessArgs::quoteArg("C:\\a\"b\\log,txt", OsTypeWindows)
            << QStringList{"-o", "C:\\a\"b\\log,txt"} << QStringList();
    } else {
        // $PWD is the directory the test runs in
        QTest::newRow("pwd")
            << "-o $PWD/log,xml" << QStringList{"-o", "/bin/log,xml"} << QStringList();
        QTest::newRow("tilde")
            << "-o ~/log,xml" << QStringList{"-o", QDir::homePath() + "/log,xml"}
            << QStringList();
        // ~name/ is someone's home directory, which only a shell can resolve
        QTest::newRow("tilde user")
            << "-o ~root/log,xml" << QStringList() << QStringList{"-o ~root/log,xml"};
        // an escape in double quotes reads the same with a glob elsewhere in the string
        QTest::newRow("escaped dollar and glob")
            << "-o \"/tmp/a\\$b/log,xml\" Parser*"
            << QStringList{"-o", "/tmp/a$b/log,xml", "Parser*"} << QStringList();
    }
    QTest::newRow("empty") << QString() << QStringList() << QStringList();
}

// Splitting expands on the host, so what would need the device's values is reported instead.
void TestArgumentsTest::testRunConfigurationArgumentsOnDevice()
{
    QFETCH(QString, arguments);
    QFETCH(QStringList, expectedAllowed);
    QFETCH(QStringList, expectedOmitted);

    testSettings().processArgs.setValue(true);
    ArgumentCarryingConfiguration<QtTestConfiguration> config(
        &theQtTestFramework(), arguments,
        FilePath::fromParts(u"device", u"tst-device", u"/bin/tst_x"));
    QStringList omitted;
    const QStringList result = config.argumentsForTestRunner(&omitted);
    testSettings().processArgs.setValue(false);

    QCOMPARE(result.mid(0, expectedAllowed.size()), expectedAllowed);
    QCOMPARE(omitted, expectedOmitted);
}

void TestArgumentsTest::testRunConfigurationArgumentsOnDevice_data()
{
    QTest::addColumn<QString>("arguments");
    QTest::addColumn<QStringList>("expectedAllowed");
    QTest::addColumn<QStringList>("expectedOmitted");

    QTest::newRow("plain") << "-iterations 5" << QStringList{"-iterations", "5"} << QStringList();
    QTest::newRow("quoted path with a space")
        << "-o \"/tmp/my dir/log,xml\""
        << QStringList{"-o", "/tmp/my dir/log,xml"} << QStringList();
    QTest::newRow("tilde") << "-o ~/log,xml" << QStringList() << QStringList{"-o ~/log,xml"};
    QTest::newRow("env var")
        << "-o $TST_LOG_DIR/log,xml" << QStringList() << QStringList{"-o $TST_LOG_DIR/log,xml"};
    QTest::newRow("pwd") << "-o $PWD/log,xml" << QStringList() << QStringList{"-o $PWD/log,xml"};
}

static std::unique_ptr<TestConfiguration> createArgumentCarryingConfiguration(
    int framework, const QString &arguments)
{
    switch (framework) {
    case CatchTest:
        return std::make_unique<ArgumentCarryingConfiguration<CatchConfiguration>>(
            &theCatchFramework(), arguments);
    case GTest:
        return std::make_unique<ArgumentCarryingConfiguration<GTestConfiguration>>(
            &theGTestFramework(), arguments);
    }
    return {};
}

// Filters by tags, wildcards and globs are passed on literally, as a shell does with a glob that
// matches nothing; the framework's own filter then judges them as usual.
void TestArgumentsTest::testFrameworkRunConfigurationArguments()
{
    QFETCH(int, framework);
    QFETCH(QString, arguments);
    QFETCH(QStringList, expectedAllowed);
    QFETCH(QStringList, expectedOmitted);

    testSettings().processArgs.setValue(true);
    const auto resetProcessArgs = qScopeGuard([] { testSettings().processArgs.setValue(false); });
    // where the framework puts the run configuration's arguments among its own
    const QString marker("tst_marker");
    const std::unique_ptr<TestConfiguration> markerConfig
        = createArgumentCarryingConfiguration(framework, marker);
    QVERIFY(markerConfig);
    QStringList expected = markerConfig->argumentsForTestRunner();
    const qsizetype at = expected.indexOf(marker);
    QVERIFY(at != -1);
    expected.remove(at);
    for (qsizetype i = 0; i < expectedAllowed.size(); ++i)
        expected.insert(at + i, expectedAllowed.at(i));

    const std::unique_ptr<TestConfiguration> config
        = createArgumentCarryingConfiguration(framework, arguments);
    QVERIFY(config);
    QStringList omitted;
    const QStringList result = config->argumentsForTestRunner(&omitted);

    QCOMPARE(result, expected);
    QCOMPARE(omitted, expectedOmitted);
}

void TestArgumentsTest::testFrameworkRunConfigurationArguments_data()
{
    QTest::addColumn<int>("framework");
    QTest::addColumn<QString>("arguments");
    QTest::addColumn<QStringList>("expectedAllowed");
    QTest::addColumn<QStringList>("expectedOmitted");

    QTest::newRow("catch tag")
        << int(CatchTest) << "[fast] --rng-seed 42"
        << QStringList{"[fast]", "--rng-seed", "42"} << QStringList();
    QTest::newRow("catch excluded tag")
        << int(CatchTest) << "~[slow]" << QStringList{"~[slow]"} << QStringList();
    QTest::newRow("catch excluded name")
        << int(CatchTest) << "~notThis" << QStringList{"~notThis"} << QStringList();
    QTest::newRow("catch excluded wildcard")
        << int(CatchTest) << "~*private*" << QStringList{"~*private*"} << QStringList();
    QTest::newRow("catch excluded quoted name")
        << int(CatchTest) << "~\"some name\"" << QStringList{"~some name"} << QStringList();
    QTest::newRow("catch file tag")
        << int(CatchTest) << "[#tst_foo]" << QStringList{"[#tst_foo]"} << QStringList();
    QTest::newRow("catch wildcard")
        << int(CatchTest) << "Parser*" << QStringList{"Parser*"} << QStringList();
    QTest::newRow("catch quoted")
        << int(CatchTest) << "\"[fast]\" \"~[slow]\" \"Parser*\" --rng-seed 42"
        << QStringList{"[fast]", "~[slow]", "Parser*", "--rng-seed", "42"} << QStringList();
    QTest::newRow("catch tag and redirection")
        << int(CatchTest) << "[fast] 2>err.log" << QStringList()
        << QStringList{"[fast] 2>err.log"};
    QTest::newRow("gtest filtered glob")
        << int(GTest) << "--gtest_filter=Suite.* --my-option 3"
        << QStringList{"--my-option", "3"} << QStringList{"--gtest_filter=Suite.*"};
}

QObject *createAutotestUnitTests()
{
    return new AutotestUnitTests;
}

QObject *createExternalTestRunTest()
{
    return new ExternalTestRunTest;
}

QObject *createQtTestUtilsTest()
{
    return new QtTestUtilsTest;
}

QObject *createTestOutputReaderTest()
{
    return new TestOutputReaderTest;
}

QObject *createTestArgumentsTest()
{
    return new TestArgumentsTest;
}

} // namespace Autotest::Internal

#include "autotestunittests.moc"
