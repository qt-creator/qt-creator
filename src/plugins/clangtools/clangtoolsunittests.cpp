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

#include "clangtool.h"
#include "clangtoolsdiagnostic.h"
#include "clangtoolssettings.h"
#include "clangtoolsutils.h"

#include <coreplugin/icore.h>
#include <cpptools/clangdiagnosticconfig.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cpptoolstestcase.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/executeondestruction.h>
#include <utils/fileutils.h>

#include <QEventLoop>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

using namespace CppTools;
using namespace ProjectExplorer;
using namespace Utils;

Q_DECLARE_METATYPE(CppTools::ClangDiagnosticConfig)

namespace ClangTools {
namespace Internal {

void ClangToolsUnitTests::initTestCase()
{
    const QList<Kit *> allKits = KitManager::kits();
    if (allKits.count() == 0)
        QSKIP("This test requires at least one kit to be present");

    m_kit = findOr(allKits, nullptr, [](Kit *k) {
            return k->isValid() && QtSupport::QtKitAspect::qtVersion(k) != nullptr;
    });
    if (!m_kit)
        QSKIP("This test requires at least one valid kit with a valid Qt");

    const ToolChain *const toolchain = ToolChainKitAspect::cxxToolChain(m_kit);
    if (!toolchain)
        QSKIP("This test requires that there is a kit with a toolchain.");

    if (Core::ICore::clangExecutable(CLANG_BINDIR).isEmpty())
        QSKIP("No clang suitable for analyzing found");

    m_tmpDir = new Tests::TemporaryCopiedDir(":/clangtools/unit-tests");
    QVERIFY(m_tmpDir->isValid());
}

void ClangToolsUnitTests::cleanupTestCase()
{
    delete m_tmpDir;
}

static ClangDiagnosticConfig configFor(const QString &tidyChecks,
                                                 const QString &clazyChecks)
{
    ClangDiagnosticConfig config;
    config.setId("Test.MyTestConfig");
    config.setDisplayName("Test");
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{QStringLiteral("-Wno-everything")});
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::UseCustomChecks);
    const QString theTidyChecks = tidyChecks.isEmpty() ? tidyChecks : "-*," + tidyChecks;
    config.setClangTidyChecks(theTidyChecks);
    config.setClazyChecks(clazyChecks);
    return config;
}

void ClangToolsUnitTests::testProject()
{
    QFETCH(QString, projectFilePath);
    QFETCH(int, expectedDiagCount);
    QFETCH(ClangDiagnosticConfig, diagnosticConfig);
    if (projectFilePath.contains("mingw")) {
        const auto toolchain = ToolChainKitAspect::cxxToolChain(m_kit);
        if (toolchain->typeId() != ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID)
            QSKIP("This test is mingw specific, does not run for other toolchains");
    }

    // Open project
    Tests::ProjectOpenerAndCloser projectManager;
    const ProjectInfo projectInfo = projectManager.open(projectFilePath, true, m_kit);
    const bool isProjectOpen = projectInfo.isValid();
    QVERIFY(isProjectOpen);

    // Run tool
    ClangTool *tool = ClangTool::instance();
    tool->startTool(ClangTool::FileSelectionType::AllFiles,
                    ClangToolsSettings::instance()->runSettings(),
                    diagnosticConfig);
    QSignalSpy waitForFinishedTool(tool, &ClangTool::finished);
    QVERIFY(waitForFinishedTool.wait(30000));

    // Check for errors
    const QString errorText = waitForFinishedTool.takeFirst().first().toString();
    const bool finishedSuccessfully = errorText.isEmpty();
    if (!finishedSuccessfully)
        qWarning("Error: %s", qPrintable(errorText));
    QVERIFY(finishedSuccessfully);
    QCOMPARE(tool->diagnostics().count(), expectedDiagCount);
}

void ClangToolsUnitTests::testProject_data()
{
    QTest::addColumn<QString>("projectFilePath");
    QTest::addColumn<int>("expectedDiagCount");
    QTest::addColumn<ClangDiagnosticConfig>("diagnosticConfig");

    // Test simple C++ project.
    ClangDiagnosticConfig config = configFor("modernize-use-nullptr", QString());
    addTestRow("simple/simple.qbs", 1, config);
    addTestRow("simple/simple.pro", 1, config);

    // Test simple Qt project.
    config = configFor("readability-static-accessed-through-instance", QString());
    addTestRow("qt-widgets-app/qt-widgets-app.qbs", 1, config);
    addTestRow("qt-widgets-app/qt-widgets-app.pro", 1, config);

    // Test that libraries can be analyzed.
    config = configFor(QString(), QString());
    addTestRow("simple-library/simple-library.qbs", 0, config);
    addTestRow("simple-library/simple-library.pro", 0, config);

    // Test that standard headers can be parsed.
    addTestRow("stdc++11-includes/stdc++11-includes.qbs", 0, config);
    addTestRow("stdc++11-includes/stdc++11-includes.pro", 0, config);

    // Test that qt essential headers can be parsed.
    addTestRow("qt-essential-includes/qt-essential-includes.qbs", 0, config);
    addTestRow("qt-essential-includes/qt-essential-includes.pro", 0, config);

    // Test that mingw includes can be parsed.
    addTestRow("mingw-includes/mingw-includes.qbs", 0, config);
    addTestRow("mingw-includes/mingw-includes.pro", 0, config);

    // Test that tidy and clazy diagnostics are emitted for the same project.
    addTestRow("clangtidy_clazy/clangtidy_clazy.pro",
               1 /*tidy*/ + 1 /*clazy*/,
               configFor("misc-unconventional-assign-operator", "qgetenv"));
}

void ClangToolsUnitTests::addTestRow(const QByteArray &relativeFilePath,
                                     int expectedDiagCount,
                                     const ClangDiagnosticConfig &diagnosticConfig)
{
    const QString absoluteFilePath = m_tmpDir->absolutePath(relativeFilePath);
    const QString fileName = QFileInfo(absoluteFilePath).fileName();

    QTest::newRow(fileName.toUtf8().constData())
        << absoluteFilePath << expectedDiagCount << diagnosticConfig;
}

} // namespace Internal
} // namespace ClangTools
