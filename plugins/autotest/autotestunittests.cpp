/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "autotestconstants.h"
#include "autotestunittests.h"
#include "testcodeparser.h"
#include "testtreemodel.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolstestcase.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchain.h>

#include <QSignalSpy>
#include <QTest>

#include <coreplugin/navigationwidget.h>

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
}

void AutoTestUnitTests::cleanupTestCase()
{
    delete m_tmpDir;
}

void AutoTestUnitTests::testCodeParser()
{
    QFETCH(QString, projectFilePath);
    QFETCH(int, expectedAutoTestsCount);
    QFETCH(int, expectedNamedQuickTestsCount);
    QFETCH(int, expectedUnnamedQuickTestsCount);
    QFETCH(int, expectedDataTagsCount);

    NavigationWidget *navigation = NavigationWidget::instance();
    navigation->activateSubWidget(Constants::AUTOTEST_ID);

    CppTools::Tests::ProjectOpenerAndCloser projectManager;
    const CppTools::ProjectInfo projectInfo = projectManager.open(projectFilePath, true);
    QVERIFY(projectInfo.isValid());

    QSignalSpy parserSpy(m_model->parser(), SIGNAL(parsingFinished()));
    QVERIFY(parserSpy.wait(20000));

    if (m_isQt4)
        expectedNamedQuickTestsCount = expectedUnnamedQuickTestsCount = 0;

    QCOMPARE(m_model->autoTestsCount(), expectedAutoTestsCount);
    QCOMPARE(m_model->namedQuickTestsCount(), expectedNamedQuickTestsCount);
    QCOMPARE(m_model->unnamedQuickTestsCount(), expectedUnnamedQuickTestsCount);
    QCOMPARE(m_model->dataTagsCount(), expectedDataTagsCount);

    QCOMPARE(m_model->parser()->autoTestsCount(), expectedAutoTestsCount);
    QCOMPARE(m_model->parser()->namedQuickTestsCount(), expectedNamedQuickTestsCount);
    QCOMPARE(m_model->parser()->unnamedQuickTestsCount(), expectedUnnamedQuickTestsCount);

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

    NavigationWidget *navigation = NavigationWidget::instance();
    navigation->activateSubWidget(Constants::AUTOTEST_ID);

    CppTools::Tests::ProjectOpenerAndCloser projectManager;
    for (int i = 0; i < projectFilePaths.size(); ++i) {
        qDebug() << "Opening project" << projectFilePaths.at(i);
        CppTools::ProjectInfo projectInfo = projectManager.open(projectFilePaths.at(i), true);
        QVERIFY(projectInfo.isValid());

        QSignalSpy parserSpy(m_model->parser(), SIGNAL(parsingFinished()));
        QVERIFY(parserSpy.wait(20000));

        QCOMPARE(m_model->autoTestsCount(), expectedAutoTestsCount.at(i));
        QCOMPARE(m_model->namedQuickTestsCount(),
                 m_isQt4 ? 0 : expectedNamedQuickTestsCount.at(i));
        QCOMPARE(m_model->unnamedQuickTestsCount(),
                 m_isQt4 ? 0 : expectedUnnamedQuickTestsCount.at(i));
        QCOMPARE(m_model->dataTagsCount(),
                 expectedDataTagsCount.at(i));

        QCOMPARE(m_model->parser()->autoTestsCount(), expectedAutoTestsCount.at(i));
        QCOMPARE(m_model->parser()->namedQuickTestsCount(),
                 m_isQt4 ? 0 : expectedNamedQuickTestsCount.at(i));
        QCOMPARE(m_model->parser()->unnamedQuickTestsCount(),
                 m_isQt4 ? 0 : expectedUnnamedQuickTestsCount.at(i));
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

} // namespace Internal
} // namespace Autotest
