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

#include "clangtoolsunittests.h"

#include "clangtoolsdiagnostic.h"
#include "clangstaticanalyzertool.h"
#include "clangtidyclazytool.h"
#include "clangtoolsutils.h"

#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolstestcase.h>
#include <cpptools/cpptoolsreuse.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchain.h>

#include <utils/executeondestruction.h>
#include <utils/fileutils.h>

#include <QEventLoop>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace ClangTools {
namespace Internal {

void ClangToolsUnitTests::initTestCase()
{
    const QList<Kit *> allKits = KitManager::kits();
    if (allKits.count() != 1)
        QSKIP("This test requires exactly one kit to be present");
    const ToolChain * const toolchain = ToolChainKitInformation::toolChain(allKits.first(),
                                                                           Constants::CXX_LANGUAGE_ID);
    if (!toolchain)
        QSKIP("This test requires that there is a kit with a toolchain.");

    if (CppTools::clangExecutable(CLANG_BINDIR).isEmpty())
        QSKIP("No clang suitable for analyzing found");

    m_tmpDir = new CppTools::Tests::TemporaryCopiedDir(QLatin1String(":/unit-tests"));
    QVERIFY(m_tmpDir->isValid());
}

void ClangToolsUnitTests::cleanupTestCase()
{
    delete m_tmpDir;
}

static CppTools::ClangDiagnosticConfig createTidyClazyConfig()
{
    CppTools::ClangDiagnosticConfig config;
    config.setId("Test.ClangTidy");
    config.setDisplayName("Test");
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{QStringLiteral("-Wno-everything")});
    config.setClangTidyMode(CppTools::ClangDiagnosticConfig::TidyMode::ChecksString);
    config.setClangTidyChecksString("-*,modernize-*, misc-*");
    config.setClazyChecks("level2");
    return config;
}

void ClangToolsUnitTests::testProject()
{
    QFETCH(Tool, clangtool);
    QFETCH(QString, projectFilePath);
    QFETCH(int, expectedDiagCount);
    if (projectFilePath.contains("mingw")) {
        const ToolChain * const toolchain
                = ToolChainKitInformation::toolChain(KitManager::kits().first(),
                                                     Constants::CXX_LANGUAGE_ID);
        if (toolchain->typeId() != ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID)
            QSKIP("This test is mingw specific, does not run for other toolchains");
    }

    CppTools::Tests::ProjectOpenerAndCloser projectManager;
    const CppTools::ProjectInfo projectInfo = projectManager.open(projectFilePath, true);
    QVERIFY(projectInfo.isValid());
    ClangTool *tool = (clangtool == Tool::ClangStaticAnalyzer)
            ? ClangStaticAnalyzerTool::instance()
            : static_cast<ClangTool *>(ClangTidyClazyTool::instance());

    ExecuteOnDestruction executeOnDestruction;
    if (clangtool == Tool::ClangTidyAndClazy) {
        // Change configs
        QSharedPointer<CppTools::CppCodeModelSettings> settings = CppTools::codeModelSettings();
        const CppTools::ClangDiagnosticConfigs originalConfigs
                = settings->clangCustomDiagnosticConfigs();
        const Core::Id originalId = settings->clangDiagnosticConfigId();

        CppTools::ClangDiagnosticConfigs modifiedConfigs = originalConfigs;

        const CppTools::ClangDiagnosticConfig clangTidyConfig = createTidyClazyConfig();
        modifiedConfigs.push_back(clangTidyConfig);

        executeOnDestruction.reset([=]() {
            // Restore configs
            settings->setClangCustomDiagnosticConfigs(originalConfigs);
            settings->setClangDiagnosticConfigId(originalId);
        });

        settings->setClangCustomDiagnosticConfigs(modifiedConfigs);
        settings->setClangDiagnosticConfigId(clangTidyConfig.id());
    }

    tool->startTool();
    QSignalSpy waiter(tool, SIGNAL(finished(bool)));
    QVERIFY(waiter.wait(30000));

    const QList<QVariant> arguments = waiter.takeFirst();
    QVERIFY(arguments.first().toBool());
    QCOMPARE(tool->diagnostics().count(), expectedDiagCount);
}

void ClangToolsUnitTests::testProject_data()
{
    QTest::addColumn<Tool>("clangtool");
    QTest::addColumn<QString>("projectFilePath");
    QTest::addColumn<int>("expectedDiagCount");

    addTestRow(Tool::ClangStaticAnalyzer, "simple/simple.qbs", 1);
    addTestRow(Tool::ClangStaticAnalyzer, "simple/simple.pro", 1);

    addTestRow(Tool::ClangStaticAnalyzer, "simple-library/simple-library.qbs", 0);
    addTestRow(Tool::ClangStaticAnalyzer, "simple-library/simple-library.pro", 0);

    addTestRow(Tool::ClangStaticAnalyzer, "stdc++11-includes/stdc++11-includes.qbs", 0);
    addTestRow(Tool::ClangStaticAnalyzer, "stdc++11-includes/stdc++11-includes.pro", 0);

    addTestRow(Tool::ClangStaticAnalyzer, "qt-widgets-app/qt-widgets-app.qbs", 0);
    addTestRow(Tool::ClangStaticAnalyzer, "qt-widgets-app/qt-widgets-app.pro", 0);

    addTestRow(Tool::ClangStaticAnalyzer, "qt-essential-includes/qt-essential-includes.qbs", 0);
    addTestRow(Tool::ClangStaticAnalyzer, "qt-essential-includes/qt-essential-includes.pro", 0);

    addTestRow(Tool::ClangStaticAnalyzer, "mingw-includes/mingw-includes.qbs", 0);
    addTestRow(Tool::ClangStaticAnalyzer, "mingw-includes/mingw-includes.pro", 0);

    addTestRow(Tool::ClangTidyAndClazy, "clangtidy_clazy/clangtidy_clazy.pro",
               4 /* ClangTidy: modernize-*,misc-* */
               + 2 /* Clazy: level1 */);
}

void ClangToolsUnitTests::addTestRow(Tool tool, const QByteArray &relativeFilePath,
                                     int expectedDiagCount)
{
    const QString absoluteFilePath = m_tmpDir->absolutePath(relativeFilePath);
    const QString fileName = QFileInfo(absoluteFilePath).fileName();

    QTest::newRow(fileName.toUtf8().constData()) << tool << absoluteFilePath << expectedDiagCount;
}

} // namespace Internal
} // namespace ClangTools
