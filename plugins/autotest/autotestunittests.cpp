/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
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
#include <QTime>

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


    QTest::newRow("plainAutoTest")
            << QString(m_tmpDir->path() + QLatin1String("/plain/plain.pro"))
            << 1 << 0 << 0;
    QTest::newRow("mixedAutoTestAndQuickTests")
            << QString(m_tmpDir->path() + QLatin1String("/mixed_atp/mixed_atp.pro"))
            << 3 << 5 << 3;
}

} // namespace Internal
} // namespace Autotest
