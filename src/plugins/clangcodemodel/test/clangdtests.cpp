/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "clangdtests.h"

#include "clangautomationutils.h"
#include "clangbatchfileprocessor.h"
#include "../clangdclient.h"
#include "../clangmodelmanagersupport.h"

#include <cplusplus/FindUsages.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cpptoolstestcase.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultitem.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/algorithm.h>

#include <QEventLoop>
#include <QScopedPointer>
#include <QTimer>
#include <QtTest>

using namespace CPlusPlus;
using namespace Core;
using namespace ProjectExplorer;

namespace ClangCodeModel {
namespace Internal {
namespace Tests {

void ClangdTests::initTestCase()
{
    const auto settings = CppTools::codeModelSettings();
    const QString clangdFromEnv = qEnvironmentVariable("QTC_CLANGD");
    if (!clangdFromEnv.isEmpty())
        settings->setClangdFilePath(Utils::FilePath::fromString(clangdFromEnv));
    const auto clangd = settings->clangdFilePath();
    if (clangd.isEmpty() || !clangd.exists())
        QSKIP("clangd binary not found");
    settings->setUseClangd(true);
}

template <typename Signal> static bool waitForSignalOrTimeout(
        const typename QtPrivate::FunctionPointer<Signal>::Object *sender, Signal signal)
{
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeOutInMs());
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(sender, signal, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();
    return timer.isActive();
}

// The main point here is to test our access type categorization.
// We do not try to stress-test clangd's "Find References" functionality; such tests belong
// into LLVM.
void ClangdTests::testFindReferences()
{
    // Find suitable kit.
    const QList<Kit *> allKits = KitManager::kits();
    if (allKits.isEmpty())
        QSKIP("This test requires at least one kit to be present");
    Kit * const kit = Utils::findOr(allKits, nullptr, [](const Kit *k) {
        return k->isValid() && QtSupport::QtKitAspect::qtVersion(k);
    });
    if (!kit)
        QSKIP("The test requires at least one valid kit with a valid Qt");

    // Copy project out of qrc file, open it, and set up target.
    CppTools::Tests::TemporaryCopiedDir testDir(qrcPath("find-usages"));
    QVERIFY(testDir.isValid());
    const auto openProjectResult = ProjectExplorerPlugin::openProject(
                testDir.absolutePath("find-usages.pro"));
    QVERIFY2(openProjectResult, qPrintable(openProjectResult.errorMessage()));
    openProjectResult.project()->configureAsExampleProject(kit);

    // Setting up the project should result in a clangd client being created.
    // Wait until that has happened.
    const auto modelManagerSupport = ClangModelManagerSupport::instance();
    ClangdClient *client = modelManagerSupport->clientForProject(openProjectResult.project());
    if (!client) {
        QVERIFY(waitForSignalOrTimeout(modelManagerSupport,
                                       &ClangModelManagerSupport::createdClient));
        client = modelManagerSupport->clientForProject(openProjectResult.project());
    }
    QVERIFY(client);
    client->enableTesting();

    // Wait until the client is fully initialized, i.e. it's completed the handshake
    // with the server.
    if (!client->reachable())
        QVERIFY(waitForSignalOrTimeout(client, &ClangdClient::initialized));
    QVERIFY(client->reachable());

    // The kind of AST support we need was introduced in LLVM 13.
    if (client->versionNumber() < QVersionNumber(13))
        QSKIP("Find Usages test needs clang >= 13");

    // Wait for index to build.
    if (!client->isFullyIndexed())
        QVERIFY(waitForSignalOrTimeout(client, &ClangdClient::indexingFinished));
    QVERIFY(client->isFullyIndexed());

    // Open cpp documents.
    struct EditorCloser {
        static void cleanup(IEditor *editor) { EditorManager::closeEditors({editor}); }
    };
    const auto headerPath = Utils::FilePath::fromString(testDir.absolutePath("defs.h"));
    QVERIFY2(headerPath.exists(), qPrintable(headerPath.toUserOutput()));
    QScopedPointer<IEditor, EditorCloser> headerEditor(EditorManager::openEditor(headerPath));
    QVERIFY(headerEditor);
    const auto headerDoc = qobject_cast<TextEditor::TextDocument *>(headerEditor->document());
    QVERIFY(headerDoc);
    QVERIFY(client->documentForFilePath(headerPath) == headerDoc);
    const auto cppFilePath = Utils::FilePath::fromString(testDir.absolutePath("main.cpp"));
    QVERIFY2(cppFilePath.exists(), qPrintable(cppFilePath.toUserOutput()));
    QScopedPointer<IEditor, EditorCloser> cppFileEditor(EditorManager::openEditor(cppFilePath));
    QVERIFY(cppFileEditor);
    const auto cppDoc = qobject_cast<TextEditor::TextDocument *>(cppFileEditor->document());
    QVERIFY(cppDoc);
    QVERIFY(client->documentForFilePath(cppFilePath) == cppDoc);

    // ... and we're ready to go.
    QList<SearchResultItem> searchResults;
    connect(client, &ClangdClient::foundReferences, this,
            [&searchResults](const QList<SearchResultItem> &results) {
        if (results.isEmpty())
            return;
        if (results.first().path().first().endsWith("defs.h"))
            searchResults = results + searchResults; // Guarantee expected file order.
        else
            searchResults += results;
    });

#define FIND_USAGES(doc, pos) do { \
    QTextCursor cursor((doc)->document()); \
    cursor.setPosition((pos)); \
    searchResults.clear(); \
    client->findUsages((doc), cursor, {}); \
    QVERIFY(waitForSignalOrTimeout(client, &ClangdClient::findUsagesDone)); \
} while (false)

#define EXPECT_RESULT(index, lne, col, type) \
    QCOMPARE(searchResults.at(index).mainRange().begin.line, lne); \
    QCOMPARE(searchResults.at(index).mainRange().begin.column, col); \
    QCOMPARE(searchResults.at(index).userData().toInt(), int(type))

    // All kinds of checks involving a struct member.
    FIND_USAGES(headerDoc, 55);
    QCOMPARE(searchResults.size(), 32);
    EXPECT_RESULT(0, 2, 17, Usage::Type::Read);
    EXPECT_RESULT(1, 3, 15, Usage::Type::Declaration);
    EXPECT_RESULT(2, 6, 17, Usage::Type::WritableRef);
    EXPECT_RESULT(3, 8, 11, Usage::Type::WritableRef);
    EXPECT_RESULT(4, 9, 13, Usage::Type::WritableRef);
    EXPECT_RESULT(5, 10, 12, Usage::Type::WritableRef);
    EXPECT_RESULT(6, 11, 13, Usage::Type::WritableRef);
    EXPECT_RESULT(7, 12, 14, Usage::Type::WritableRef);
    EXPECT_RESULT(8, 13, 26, Usage::Type::WritableRef);
    EXPECT_RESULT(9, 14, 23, Usage::Type::Read);
    EXPECT_RESULT(10, 15, 14, Usage::Type::Read);
    EXPECT_RESULT(11, 16, 24, Usage::Type::WritableRef);
    EXPECT_RESULT(12, 17, 15, Usage::Type::WritableRef);
    EXPECT_RESULT(13, 18, 22, Usage::Type::Read);
    EXPECT_RESULT(14, 19, 12, Usage::Type::WritableRef);
    EXPECT_RESULT(15, 20, 12, Usage::Type::Read);
    EXPECT_RESULT(16, 21, 13, Usage::Type::WritableRef);
    EXPECT_RESULT(17, 22, 13, Usage::Type::Read);
    EXPECT_RESULT(18, 23, 12, Usage::Type::Read);
    EXPECT_RESULT(19, 42, 20, Usage::Type::Read);
    EXPECT_RESULT(20, 44, 15, Usage::Type::Read);
    EXPECT_RESULT(21, 47, 15, Usage::Type::Write);
    EXPECT_RESULT(22, 50, 11, Usage::Type::Read);
    EXPECT_RESULT(23, 51, 11, Usage::Type::Write);
    EXPECT_RESULT(24, 52, 9, Usage::Type::Write);
    EXPECT_RESULT(25, 53, 7, Usage::Type::Write);
    EXPECT_RESULT(26, 56, 7, Usage::Type::Write);
    EXPECT_RESULT(27, 56, 25, Usage::Type::Other);
    EXPECT_RESULT(28, 58, 13, Usage::Type::Read);
    EXPECT_RESULT(29, 58, 25, Usage::Type::Read);
    EXPECT_RESULT(30, 59, 7, Usage::Type::Write);
    EXPECT_RESULT(31, 59, 24, Usage::Type::Read);

    // Detect constructor member initialization as a write access.
    FIND_USAGES(headerDoc, 68);
    QCOMPARE(searchResults.size(), 2);
    EXPECT_RESULT(0, 2, 10, Usage::Type::Write);
    EXPECT_RESULT(1, 4, 8, Usage::Type::Declaration);

    // Detect direct member initialization.
    FIND_USAGES(headerDoc, 101);
    QCOMPARE(searchResults.size(), 2);
    EXPECT_RESULT(0, 5, 21, Usage::Type::Initialization);
    EXPECT_RESULT(1, 45, 16, Usage::Type::Read);

    // Make sure that pure virtual declaration is not mistaken for an assignment.
    FIND_USAGES(headerDoc, 420);
    QCOMPARE(searchResults.size(), 3); // FIXME: The override gets reported twice. clang bug?
    EXPECT_RESULT(0, 17, 17, Usage::Type::Declaration);
    EXPECT_RESULT(1, 21, 9, Usage::Type::Declaration);
    EXPECT_RESULT(2, 21, 9, Usage::Type::Declaration);

    // References to pointer variable.
    FIND_USAGES(cppDoc, 52);
    QCOMPARE(searchResults.size(), 11);
    EXPECT_RESULT(0, 6, 10, Usage::Type::Initialization);
    EXPECT_RESULT(1, 8, 4, Usage::Type::Write);
    EXPECT_RESULT(2, 10, 4, Usage::Type::Write);
    EXPECT_RESULT(3, 24, 5, Usage::Type::Write);
    EXPECT_RESULT(4, 25, 11, Usage::Type::WritableRef);
    EXPECT_RESULT(5, 26, 11, Usage::Type::Read);
    EXPECT_RESULT(6, 27, 10, Usage::Type::WritableRef);
    EXPECT_RESULT(7, 28, 10, Usage::Type::Read);
    EXPECT_RESULT(8, 29, 11, Usage::Type::Read);
    EXPECT_RESULT(9, 30, 15, Usage::Type::WritableRef);
    EXPECT_RESULT(10, 31, 22, Usage::Type::Read);

    // References to struct variable, directly and via members.
    FIND_USAGES(cppDoc, 39);
    QCOMPARE(searchResults.size(), 34);
    EXPECT_RESULT(0, 5, 7, Usage::Type::Declaration);
    EXPECT_RESULT(1, 6, 15, Usage::Type::WritableRef);
    EXPECT_RESULT(2, 8, 9, Usage::Type::WritableRef);
    EXPECT_RESULT(3, 9, 11, Usage::Type::WritableRef);
    EXPECT_RESULT(4, 11, 4, Usage::Type::Write);
    EXPECT_RESULT(5, 11, 11, Usage::Type::WritableRef);
    EXPECT_RESULT(6, 12, 12, Usage::Type::WritableRef);
    EXPECT_RESULT(7, 13, 6, Usage::Type::Write);
    EXPECT_RESULT(8, 14, 21, Usage::Type::Read);
    EXPECT_RESULT(9, 15, 4, Usage::Type::Write);
    EXPECT_RESULT(10, 15, 12, Usage::Type::Read);
    EXPECT_RESULT(11, 16, 22, Usage::Type::WritableRef);
    EXPECT_RESULT(12, 17, 13, Usage::Type::WritableRef);
    EXPECT_RESULT(13, 18, 20, Usage::Type::Read);
    EXPECT_RESULT(14, 19, 10, Usage::Type::WritableRef);
    EXPECT_RESULT(15, 20, 10, Usage::Type::Read);
    EXPECT_RESULT(16, 21, 11, Usage::Type::WritableRef);
    EXPECT_RESULT(17, 22, 11, Usage::Type::Read);
    EXPECT_RESULT(18, 23, 10, Usage::Type::Read);
    EXPECT_RESULT(19, 32, 4, Usage::Type::Write);
    EXPECT_RESULT(20, 33, 23, Usage::Type::WritableRef);
    EXPECT_RESULT(21, 34, 23, Usage::Type::Read);
    EXPECT_RESULT(22, 35, 15, Usage::Type::WritableRef);
    EXPECT_RESULT(23, 36, 22, Usage::Type::WritableRef);
    EXPECT_RESULT(24, 37, 4, Usage::Type::Read);
    EXPECT_RESULT(25, 38, 4, Usage::Type::WritableRef);
    EXPECT_RESULT(26, 39, 6, Usage::Type::WritableRef);
    EXPECT_RESULT(27, 40, 4, Usage::Type::Read);
    EXPECT_RESULT(28, 41, 4, Usage::Type::WritableRef);
    EXPECT_RESULT(29, 42, 4, Usage::Type::Read);
    EXPECT_RESULT(30, 42, 18, Usage::Type::Read);
    EXPECT_RESULT(31, 43, 11, Usage::Type::Write);
    EXPECT_RESULT(32, 54, 4, Usage::Type::Other);
    EXPECT_RESULT(33, 55, 4, Usage::Type::Other);

    // References to struct type.
    FIND_USAGES(headerDoc, 7);
    QCOMPARE(searchResults.size(), 18);
    EXPECT_RESULT(0, 1, 7, Usage::Type::Declaration);
    EXPECT_RESULT(1, 2, 4, Usage::Type::Declaration);
    EXPECT_RESULT(2, 20, 19, Usage::Type::Other);

    // These are conceptually questionable, as S is a type and thus we cannot "read from"
    // or "write to" it. But it probably matches the intuitive user expectation.
    EXPECT_RESULT(3, 10, 9, Usage::Type::WritableRef);
    EXPECT_RESULT(4, 12, 4, Usage::Type::Write);
    EXPECT_RESULT(5, 44, 12, Usage::Type::Read);
    EXPECT_RESULT(6, 45, 13, Usage::Type::Read);
    EXPECT_RESULT(7, 47, 12, Usage::Type::Write);
    EXPECT_RESULT(8, 50, 8, Usage::Type::Read);
    EXPECT_RESULT(9, 51, 8, Usage::Type::Write);
    EXPECT_RESULT(10, 52, 6, Usage::Type::Write);
    EXPECT_RESULT(11, 53, 4, Usage::Type::Write);
    EXPECT_RESULT(12, 56, 4, Usage::Type::Write);
    EXPECT_RESULT(13, 56, 22, Usage::Type::Other);
    EXPECT_RESULT(14, 58, 10, Usage::Type::Read);
    EXPECT_RESULT(15, 58, 22, Usage::Type::Read);
    EXPECT_RESULT(16, 59, 4, Usage::Type::Write);
    EXPECT_RESULT(17, 59, 21, Usage::Type::Read);

    // References to subclass type.
    FIND_USAGES(headerDoc, 450);
    QCOMPARE(searchResults.size(), 4);
    EXPECT_RESULT(0, 20, 7, Usage::Type::Declaration);
    EXPECT_RESULT(1, 5, 4, Usage::Type::Other);
    EXPECT_RESULT(2, 13, 21, Usage::Type::Other);
    EXPECT_RESULT(3, 32, 8, Usage::Type::Other);

    // References to array variables.
    FIND_USAGES(cppDoc, 1134);
    QCOMPARE(searchResults.size(), 3);
    EXPECT_RESULT(0, 57, 8, Usage::Type::Declaration);
    EXPECT_RESULT(1, 58, 4, Usage::Type::Write);
    EXPECT_RESULT(2, 59, 15, Usage::Type::Read);
}

} // namespace Tests
} // namespace Internal
} // namespace ClangCodeModel
