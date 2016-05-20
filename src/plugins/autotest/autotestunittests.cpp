/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "autotestunittests.h"
#include "testcodeparser.h"
#include "testsettings.h"
#include "testtreemodel.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolstestcase.h>
#include <cpptools/projectinfo.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchain.h>

#include <QSignalSpy>
#include <QTest>

#include <qtsupport/qtkitinformation.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Autotest {
namespace Internal {

AutoTestUnitTests::AutoTestUnitTests(TestTreeModel *model, QObject *parent)
    : QObject(parent),
      m_model(model),
      m_tmpDir(0),
      m_isQt4(false)
{
}

void AutoTestUnitTests::initTestCase()
{
    const QList<Kit *> allKits = KitManager::kits();
    if (allKits.count() != 1)
        QSKIP("This test requires exactly one kit to be present");
    if (auto qtVersion = QtSupport::QtKitInformation::qtVersion(allKits.first()))
        m_isQt4 = qtVersion->qtVersionString().startsWith(QLatin1Char('4'));
    else
        QSKIP("Could not figure out which Qt version is used for default kit.");
    const ToolChain * const toolchain = ToolChainKitInformation::toolChain(allKits.first());
    if (!toolchain)
        QSKIP("This test requires that there is a kit with a toolchain.");

    m_tmpDir = new CppTools::Tests::TemporaryCopiedDir(QLatin1String(":/unit_test"));

    m_originalAlwaysParse = AutotestPlugin::instance()->settings()->alwaysParse;
    if (!m_originalAlwaysParse) {
        AutotestPlugin::instance()->settings()->alwaysParse = true;
        TestTreeModel::instance()->enableParsingFromSettings();
    }
}

void AutoTestUnitTests::cleanupTestCase()
{
    AutotestPlugin::instance()->settings()->alwaysParse = m_originalAlwaysParse;
    delete m_tmpDir;
}

void AutoTestUnitTests::testCodeParser()
{
    QFETCH(QString, projectFilePath);
    QFETCH(int, expectedAutoTestsCount);
    QFETCH(int, expectedNamedQuickTestsCount);
    QFETCH(int, expectedUnnamedQuickTestsCount);
    QFETCH(int, expectedDataTagsCount);

    CppTools::Tests::ProjectOpenerAndCloser projectManager;
    const CppTools::ProjectInfo projectInfo = projectManager.open(projectFilePath, true);
    QVERIFY(projectInfo.isValid());

    QSignalSpy parserSpy(m_model->parser(), SIGNAL(parsingFinished()));
    QSignalSpy modelUpdateSpy(m_model, SIGNAL(sweepingDone()));
    QVERIFY(parserSpy.wait(20000));
    QVERIFY(modelUpdateSpy.wait());

    if (m_isQt4)
        expectedNamedQuickTestsCount = expectedUnnamedQuickTestsCount = 0;

    QCOMPARE(m_model->autoTestsCount(), expectedAutoTestsCount);
    QCOMPARE(m_model->namedQuickTestsCount(), expectedNamedQuickTestsCount);
    QCOMPARE(m_model->unnamedQuickTestsCount(), expectedUnnamedQuickTestsCount);
    QCOMPARE(m_model->dataTagsCount(), expectedDataTagsCount);
}

void AutoTestUnitTests::testCodeParser_data()
{
    QTest::addColumn<QString>("projectFilePath");
    QTest::addColumn<int>("expectedAutoTestsCount");
    QTest::addColumn<int>("expectedNamedQuickTestsCount");
    QTest::addColumn<int>("expectedUnnamedQuickTestsCount");
    QTest::addColumn<int>("expectedDataTagsCount");


    QTest::newRow("plainAutoTest")
            << QString(m_tmpDir->path() + QLatin1String("/plain/plain.pro"))
            << 1 << 0 << 0 << 0;
    QTest::newRow("mixedAutoTestAndQuickTests")
            << QString(m_tmpDir->path() + QLatin1String("/mixed_atp/mixed_atp.pro"))
            << 3 << 5 << 3 << 8;
    QTest::newRow("plainAutoTestQbs")
            << QString(m_tmpDir->path() + QLatin1String("/plain/plain.qbs"))
            << 1 << 0 << 0 << 0;
    QTest::newRow("mixedAutoTestAndQuickTestsQbs")
            << QString(m_tmpDir->path() + QLatin1String("/mixed_atp/mixed_atp.qbs"))
            << 3 << 5 << 3 << 8;
}

void AutoTestUnitTests::testCodeParserSwitchStartup()
{
    QFETCH(QStringList, projectFilePaths);
    QFETCH(QList<int>, expectedAutoTestsCount);
    QFETCH(QList<int>, expectedNamedQuickTestsCount);
    QFETCH(QList<int>, expectedUnnamedQuickTestsCount);
    QFETCH(QList<int>, expectedDataTagsCount);

    CppTools::Tests::ProjectOpenerAndCloser projectManager;
    for (int i = 0; i < projectFilePaths.size(); ++i) {
        qDebug() << "Opening project" << projectFilePaths.at(i);
        CppTools::ProjectInfo projectInfo = projectManager.open(projectFilePaths.at(i), true);
        QVERIFY(projectInfo.isValid());

        QSignalSpy parserSpy(m_model->parser(), SIGNAL(parsingFinished()));
        QSignalSpy modelUpdateSpy(m_model, SIGNAL(sweepingDone()));
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

void AutoTestUnitTests::testCodeParserSwitchStartup_data()
{
    QTest::addColumn<QStringList>("projectFilePaths");
    QTest::addColumn<QList<int> >("expectedAutoTestsCount");
    QTest::addColumn<QList<int> >("expectedNamedQuickTestsCount");
    QTest::addColumn<QList<int> >("expectedUnnamedQuickTestsCount");
    QTest::addColumn<QList<int> >("expectedDataTagsCount");

    QStringList projects = QStringList()
            << QString(m_tmpDir->path() + QLatin1String("/plain/plain.pro"))
            << QString(m_tmpDir->path() + QLatin1String("/mixed_atp/mixed_atp.pro"))
            << QString(m_tmpDir->path() + QLatin1String("/plain/plain.qbs"))
            << QString(m_tmpDir->path() + QLatin1String("/mixed_atp/mixed_atp.qbs"));

    QList<int> expectedAutoTests = QList<int>()         << 1 << 3 << 1 << 3;
    QList<int> expectedNamedQuickTests = QList<int>()   << 0 << 5 << 0 << 5;
    QList<int> expectedUnnamedQuickTests = QList<int>() << 0 << 3 << 0 << 3;
    QList<int> expectedDataTagsCount = QList<int>()     << 0 << 8 << 0 << 8;

    QTest::newRow("loadMultipleProjects")
            << projects << expectedAutoTests << expectedNamedQuickTests
            << expectedUnnamedQuickTests << expectedDataTagsCount;
}

void AutoTestUnitTests::testCodeParserGTest()
{
    if (qgetenv("GOOGLETEST_DIR").isEmpty())
        QSKIP("This test needs googletest - set GOOGLETEST_DIR (point to googletest repository)");

    QFETCH(QString, projectFilePath);
    CppTools::Tests::ProjectOpenerAndCloser projectManager;
    CppTools::ProjectInfo projectInfo = projectManager.open(projectFilePath, true);
    QVERIFY(projectInfo.isValid());

    QSignalSpy parserSpy(m_model->parser(), SIGNAL(parsingFinished()));
    QSignalSpy modelUpdateSpy(m_model, SIGNAL(sweepingDone()));
    QVERIFY(parserSpy.wait(20000));
    QVERIFY(modelUpdateSpy.wait());

    QCOMPARE(m_model->gtestNamesCount(), 6);

    QMultiMap<QString, int> expectedNamesAndSets;
    expectedNamesAndSets.insert(QStringLiteral("FactorialTest"), 3);
    expectedNamesAndSets.insert(QStringLiteral("FactorialTest_Iterative"), 2);
    expectedNamesAndSets.insert(QStringLiteral("Sum"), 2);
    expectedNamesAndSets.insert(QStringLiteral("QueueTest"), 2);
    expectedNamesAndSets.insert(QStringLiteral("DummyTest"), 1); // used as parameterized test
    expectedNamesAndSets.insert(QStringLiteral("DummyTest"), 1); // used as 'normal' test

    QMultiMap<QString, int> foundNamesAndSets = m_model->gtestNamesAndSets();
    QCOMPARE(expectedNamesAndSets.size(), foundNamesAndSets.size());
    foreach (const QString &name, expectedNamesAndSets.keys())
        QCOMPARE(expectedNamesAndSets.values(name), foundNamesAndSets.values(name));

    // check also that no Qt related tests have been found
    QCOMPARE(m_model->autoTestsCount(), 0);
    QCOMPARE(m_model->namedQuickTestsCount(), 0);
    QCOMPARE(m_model->unnamedQuickTestsCount(), 0);
    QCOMPARE(m_model->dataTagsCount(), 0);
}

void AutoTestUnitTests::testCodeParserGTest_data()
{
    QTest::addColumn<QString>("projectFilePath");
    QTest::newRow("simpleGoogletest")
        << QString(m_tmpDir->path() + QLatin1String("/simple_gt/simple_gt.pro"));
    QTest::newRow("simpleGoogletestQbs")
        << QString(m_tmpDir->path() + QLatin1String("/simple_gt/simple_gt.qbs"));
}

} // namespace Internal
} // namespace Autotest
