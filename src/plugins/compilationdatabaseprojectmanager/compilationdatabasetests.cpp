/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "compilationdatabasetests.h"

#include <coreplugin/icore.h>
#include <cpptools/cpptoolstestcase.h>
#include <cpptools/projectinfo.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <QtTest>

using namespace ProjectExplorer;

namespace CompilationDatabaseProjectManager {

CompilationDatabaseTests::CompilationDatabaseTests(QObject *parent)
    : QObject(parent)
{}

CompilationDatabaseTests::~CompilationDatabaseTests() = default;

void CompilationDatabaseTests::initTestCase()
{
    const QList<Kit *> allKits = KitManager::kits();
    if (allKits.empty())
        QSKIP("This test requires at least one kit to be present.");

    ToolChain *toolchain = ToolChainManager::toolChain([](const ToolChain *tc) {
        return tc->isValid() && tc->language() == Constants::CXX_LANGUAGE_ID;
    });
    if (!toolchain)
        QSKIP("This test requires that there is at least one C++ toolchain present.");

    m_tmpDir = std::make_unique<CppTools::Tests::TemporaryCopiedDir>(":/database_samples");
    QVERIFY(m_tmpDir->isValid());
}

void CompilationDatabaseTests::cleanupTestCase()
{
    m_tmpDir.reset();
}

void CompilationDatabaseTests::testProject()
{
    QFETCH(QString, projectFilePath);

    CppTools::Tests::ProjectOpenerAndCloser projectManager;
    const CppTools::ProjectInfo projectInfo = projectManager.open(projectFilePath, true);
    QVERIFY(projectInfo.isValid());

    projectInfo.projectParts();
    QVector<CppTools::ProjectPart::Ptr> projectParts = projectInfo.projectParts();
    QVERIFY(!projectParts.isEmpty());

    CppTools::ProjectPart &projectPart = *projectParts.first();
    QVERIFY(!projectPart.headerPaths.isEmpty());
    QVERIFY(!projectPart.projectMacros.isEmpty());
    QVERIFY(!projectPart.toolChainMacros.isEmpty());
    QVERIFY(!projectPart.files.isEmpty());
}

void CompilationDatabaseTests::testProject_data()
{
    QTest::addColumn<QString>("projectFilePath");

    addTestRow("qtc/compile_commands.json");
    addTestRow("llvm/compile_commands.json");
}

void CompilationDatabaseTests::addTestRow(const QByteArray &relativeFilePath)
{
    const QString absoluteFilePath = m_tmpDir->absolutePath(relativeFilePath);
    const QString fileName = QFileInfo(absoluteFilePath).fileName();

    QTest::newRow(fileName.toUtf8().constData()) << absoluteFilePath;
}

} // namespace CompilationDatabaseProjectManager
