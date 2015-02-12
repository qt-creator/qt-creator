/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "clangstaticanalyzerunittests.h"

#include "clangstaticanalyzerdiagnostic.h"
#include "clangstaticanalyzertool.h"
#include "clangstaticanalyzerutils.h"

#include <analyzerbase/analyzermanager.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolstestcase.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchain.h>
#include <utils/fileutils.h>

#include <QEventLoop>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

using namespace Analyzer;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerUnitTests::ClangStaticAnalyzerUnitTests(ClangStaticAnalyzerTool *analyzerTool,
                                                           QObject *parent)
    : QObject(parent)
    , m_analyzerTool(analyzerTool)
    , m_tmpDir(0)
{
}

void ClangStaticAnalyzerUnitTests::initTestCase()
{
    const QList<Kit *> allKits = KitManager::kits();
    if (allKits.count() != 1)
        QSKIP("This test requires exactly one kit to be present");
    const ToolChain * const toolchain = ToolChainKitInformation::toolChain(allKits.first());
    if (!toolchain)
        QSKIP("This test requires that there is a kit with a toolchain.");
    bool hasClangExecutable;
    clangExecutableFromSettings(toolchain->type(), &hasClangExecutable);
    if (!hasClangExecutable)
        QSKIP("No clang suitable for analyzing found");

    m_tmpDir = new CppTools::Tests::TemporaryCopiedDir(QLatin1String(":/unit-tests"));
}

void ClangStaticAnalyzerUnitTests::cleanupTestCase()
{
    delete m_tmpDir;
}

void ClangStaticAnalyzerUnitTests::testProject()
{
    QFETCH(QString, projectFilePath);
    QFETCH(int, expectedDiagCount);

    CppTools::Tests::ProjectOpenerAndCloser projectManager;
    const CppTools::ProjectInfo projectInfo = projectManager.open(projectFilePath, true);
    QVERIFY(projectInfo.isValid());
    AnalyzerManager::selectTool(m_analyzerTool, Analyzer::StartLocal);
    AnalyzerManager::startTool();
    if (m_analyzerTool->isRunning()) {
        QSignalSpy waiter(m_analyzerTool, SIGNAL(finished()));
        QVERIFY(waiter.wait(30000));
    }
    QCOMPARE(m_analyzerTool->diagnostics().count(), expectedDiagCount);
}

void ClangStaticAnalyzerUnitTests::testProject_data()
{
    QTest::addColumn<QString>("projectFilePath");
    QTest::addColumn<int>("expectedDiagCount");
    QTest::newRow("qbs project")
            << QString(m_tmpDir->path() + QLatin1String("/simple/simple.qbs")) << 1;
    QTest::newRow("qmake project")
            << QString(m_tmpDir->path() + QLatin1String("/simple/simple.pro")) << 1;
}

} // namespace Internal
} // namespace ClangStaticAnalyzerPlugin
