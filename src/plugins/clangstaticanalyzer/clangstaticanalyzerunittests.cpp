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

#include "clangstaticanalyzerunittests.h"

#include "clangstaticanalyzerdiagnostic.h"
#include "clangstaticanalyzertool.h"
#include "clangstaticanalyzerutils.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolstestcase.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchain.h>
#include <utils/fileutils.h>

#include <QEventLoop>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace ClangStaticAnalyzer {
namespace Internal {

void ClangStaticAnalyzerUnitTests::initTestCase()
{
    const QList<Kit *> allKits = KitManager::kits();
    if (allKits.count() != 1)
        QSKIP("This test requires exactly one kit to be present");
    const ToolChain * const toolchain = ToolChainKitInformation::toolChain(allKits.first(),
                                                                           Constants::CXX_LANGUAGE_ID);
    if (!toolchain)
        QSKIP("This test requires that there is a kit with a toolchain.");
    bool hasClangExecutable;
    clangExecutableFromSettings(&hasClangExecutable);
    if (!hasClangExecutable)
        QSKIP("No clang suitable for analyzing found");

    m_tmpDir = new CppTools::Tests::TemporaryCopiedDir(QLatin1String(":/unit-tests"));
    QVERIFY(m_tmpDir->isValid());
}

void ClangStaticAnalyzerUnitTests::cleanupTestCase()
{
    delete m_tmpDir;
}

void ClangStaticAnalyzerUnitTests::testProject()
{
    QFETCH(QString, projectFilePath);
    QFETCH(int, expectedDiagCount);
    if (projectFilePath.contains("mingw")) {
        const ToolChain * const toolchain
                = ToolChainKitInformation::toolChain(KitManager::kits().first(),
                                                     Constants::CXX_LANGUAGE_ID);
        if (toolchain->typeId() != ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID)
            QSKIP("This test is mingw specific, does not run for other toolchais");
    }

    CppTools::Tests::ProjectOpenerAndCloser projectManager;
    const CppTools::ProjectInfo projectInfo = projectManager.open(projectFilePath, true);
    QVERIFY(projectInfo.isValid());
    auto tool = ClangStaticAnalyzerTool::instance();
    tool->startTool();
    QSignalSpy waiter(tool, SIGNAL(finished(bool)));
    QVERIFY(waiter.wait(30000));
    const QList<QVariant> arguments = waiter.takeFirst();
    QVERIFY(arguments.first().toBool());
    QCOMPARE(tool->diagnostics().count(), expectedDiagCount);
}

void ClangStaticAnalyzerUnitTests::testProject_data()
{
    QTest::addColumn<QString>("projectFilePath");
    QTest::addColumn<int>("expectedDiagCount");

    addTestRow("simple/simple.qbs", 1);
    addTestRow("simple/simple.pro", 1);

    addTestRow("simple-library/simple-library.qbs", 0);
    addTestRow("simple-library/simple-library.pro", 0);

    addTestRow("stdc++11-includes/stdc++11-includes.qbs", 0);
    addTestRow("stdc++11-includes/stdc++11-includes.pro", 0);

    addTestRow("qt-widgets-app/qt-widgets-app.qbs", 0);
    addTestRow("qt-widgets-app/qt-widgets-app.pro", 0);

    addTestRow("qt-essential-includes/qt-essential-includes.qbs", 0);
    addTestRow("qt-essential-includes/qt-essential-includes.pro", 0);

    addTestRow("mingw-includes/mingw-includes.qbs", 0);
    addTestRow("mingw-includes/mingw-includes.pro", 0);
}

void ClangStaticAnalyzerUnitTests::addTestRow(const QByteArray &relativeFilePath,
                                              int expectedDiagCount)
{
    const QString absoluteFilePath = m_tmpDir->absolutePath(relativeFilePath);
    const QString fileName = QFileInfo(absoluteFilePath).fileName();

    QTest::newRow(fileName.toUtf8().constData()) << absoluteFilePath << expectedDiagCount;
}

} // namespace Internal
} // namespace ClangStaticAnalyzerPlugin
