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
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/algorithm.h>

#include <QEventLoop>
#include <QFileInfo>
#include <QScopedPointer>
#include <QTimer>
#include <QtTest>

using namespace CPlusPlus;
using namespace Core;
using namespace CppTools::Tests;
using namespace ProjectExplorer;

namespace ClangCodeModel {
namespace Internal {
namespace Tests {

ClangdTest::~ClangdTest()
{
    if (m_project)
        ProjectExplorerPlugin::unloadProject(m_project);
    delete m_projectDir;
}

Utils::FilePath ClangdTest::filePath(const QString &fileName) const
{
    return Utils::FilePath::fromString(m_projectDir->absolutePath(fileName.toLocal8Bit()));
}

void ClangdTest::initTestCase()
{
    const auto settings = CppTools::codeModelSettings();
    const QString clangdFromEnv = qEnvironmentVariable("QTC_CLANGD");
    if (!clangdFromEnv.isEmpty())
        settings->setClangdFilePath(Utils::FilePath::fromString(clangdFromEnv));
    const auto clangd = settings->clangdFilePath();
    if (clangd.isEmpty() || !clangd.exists())
        QSKIP("clangd binary not found");
    settings->setUseClangd(true);

    // Find suitable kit.
    m_kit = Utils::findOr(KitManager::kits(), nullptr, [](const Kit *k) {
        return k->isValid() && QtSupport::QtKitAspect::qtVersion(k);
    });
    if (!m_kit)
        QSKIP("The test requires at least one valid kit with a valid Qt");

    // Copy project out of qrc file, open it, and set up target.
    m_projectDir = new TemporaryCopiedDir(qrcPath(QFileInfo(m_projectFileName)
                                                  .baseName().toLocal8Bit()));
    QVERIFY(m_projectDir->isValid());
    const auto openProjectResult = ProjectExplorerPlugin::openProject(
                m_projectDir->absolutePath(m_projectFileName.toUtf8()));
    QVERIFY2(openProjectResult, qPrintable(openProjectResult.errorMessage()));
    m_project = openProjectResult.project();
    m_project->configureAsExampleProject(m_kit);

    // Setting up the project should result in a clangd client being created.
    // Wait until that has happened.
    const auto modelManagerSupport = ClangModelManagerSupport::instance();
    m_client = modelManagerSupport->clientForProject(openProjectResult.project());
    if (!m_client) {
        QVERIFY(waitForSignalOrTimeout(modelManagerSupport,
                                       &ClangModelManagerSupport::createdClient, timeOutInMs()));
        m_client = modelManagerSupport->clientForProject(m_project);
    }
    QVERIFY(m_client);
    m_client->enableTesting();

    // Wait until the client is fully initialized, i.e. it's completed the handshake
    // with the server.
    if (!m_client->reachable())
        QVERIFY(waitForSignalOrTimeout(m_client, &ClangdClient::initialized, timeOutInMs()));
    QVERIFY(m_client->reachable());

    // The kind of AST support we need was introduced in LLVM 13.
    if (m_minVersion != -1 && m_client->versionNumber() < QVersionNumber(m_minVersion))
        QSKIP("clangd is too old");

    // Wait for index to build.
    if (!m_client->isFullyIndexed())
        QVERIFY(waitForSignalOrTimeout(m_client, &ClangdClient::indexingFinished, timeOutInMs()));
    QVERIFY(m_client->isFullyIndexed());

    // Open cpp documents.
    for (const QString &sourceFileName : qAsConst(m_sourceFileNames)) {
        const auto sourceFilePath = Utils::FilePath::fromString(
                    m_projectDir->absolutePath(sourceFileName.toLocal8Bit()));
        QVERIFY2(sourceFilePath.exists(), qPrintable(sourceFilePath.toUserOutput()));
        IEditor * const editor = EditorManager::openEditor(sourceFilePath);
        QVERIFY(editor);
        const auto doc = qobject_cast<TextEditor::TextDocument *>(editor->document());
        QVERIFY(doc);
        QVERIFY(m_client->documentForFilePath(sourceFilePath) == doc);
        m_sourceDocuments.insert(sourceFileName, doc);
    }
}

ClangdTestFindReferences::ClangdTestFindReferences()
{
    setProjectFileName("find-usages.pro");
    setSourceFileNames({"defs.h", "main.cpp"});
    setMinimumVersion(13);
}

void ClangdTestFindReferences::initTestCase()
{
    ClangdTest::initTestCase();
    CppTools::codeModelSettings()->setCategorizeFindReferences(true);
    connect(client(), &ClangdClient::foundReferences, this,
            [this](const QList<SearchResultItem> &results) {
        if (results.isEmpty())
            return;
        if (results.first().path().first().endsWith("defs.h"))
            m_actualResults = results + m_actualResults; // Guarantee expected file order.
        else
            m_actualResults += results;
    });
}

void ClangdTestFindReferences::test_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("pos");
    using ItemList = QList<SearchResultItem>;
    QTest::addColumn<ItemList>("expectedResults");

    static const auto makeItem = [](int line, int column, Usage::Type type) {
        SearchResultItem item;
        item.setMainRange(line, column, 0);
        item.setUserData(int(type));
        return item;
    };

    QTest::newRow("struct member") << "defs.h" << 55 << ItemList{
        makeItem(2, 17, Usage::Type::Read), makeItem(3, 15, Usage::Type::Declaration),
        makeItem(6, 17, Usage::Type::WritableRef), makeItem(8, 11, Usage::Type::WritableRef),
        makeItem(9, 13, Usage::Type::WritableRef), makeItem(10, 12, Usage::Type::WritableRef),
        makeItem(11, 13, Usage::Type::WritableRef), makeItem(12, 14, Usage::Type::WritableRef),
        makeItem(13, 26, Usage::Type::WritableRef), makeItem(14, 23, Usage::Type::Read),
        makeItem(15, 14, Usage::Type::Read), makeItem(16, 24, Usage::Type::WritableRef),
        makeItem(17, 15, Usage::Type::WritableRef), makeItem(18, 22, Usage::Type::Read),
        makeItem(19, 12, Usage::Type::WritableRef), makeItem(20, 12, Usage::Type::Read),
        makeItem(21, 13, Usage::Type::WritableRef), makeItem(22, 13, Usage::Type::Read),
        makeItem(23, 12, Usage::Type::Read), makeItem(42, 20, Usage::Type::Read),
        makeItem(44, 15, Usage::Type::Read), makeItem(47, 15, Usage::Type::Write),
        makeItem(50, 11, Usage::Type::Read), makeItem(51, 11, Usage::Type::Write),
        makeItem(52, 9, Usage::Type::Write), makeItem(53, 7, Usage::Type::Write),
        makeItem(56, 7, Usage::Type::Write), makeItem(56, 25, Usage::Type::Other),
        makeItem(58, 13, Usage::Type::Read), makeItem(58, 25, Usage::Type::Read),
        makeItem(59, 7, Usage::Type::Write), makeItem(59, 24, Usage::Type::Read)};
    QTest::newRow("constructor member initialization") << "defs.h" << 68 << ItemList{
        makeItem(2, 10, Usage::Type::Write), makeItem(4, 8, Usage::Type::Declaration)};
    QTest::newRow("direct member initialization") << "defs.h" << 101 << ItemList{
        makeItem(5, 21, Usage::Type::Initialization), makeItem(45, 16, Usage::Type::Read)};

    // FIXME: The override gets reported twice. clangd bug?
    QTest::newRow("pure virtual declaration") << "defs.h" << 420 << ItemList{
        makeItem(17, 17, Usage::Type::Declaration), makeItem(21, 9, Usage::Type::Declaration),
        makeItem(21, 9, Usage::Type::Declaration)};

    QTest::newRow("pointer variable") << "main.cpp" << 52 << ItemList{
        makeItem(6, 10, Usage::Type::Initialization), makeItem(8, 4, Usage::Type::Write),
        makeItem(10, 4, Usage::Type::Write), makeItem(24, 5, Usage::Type::Write),
        makeItem(25, 11, Usage::Type::WritableRef), makeItem(26, 11, Usage::Type::Read),
        makeItem(27, 10, Usage::Type::WritableRef), makeItem(28, 10, Usage::Type::Read),
        makeItem(29, 11, Usage::Type::Read), makeItem(30, 15, Usage::Type::WritableRef),
        makeItem(31, 22, Usage::Type::Read)};
    QTest::newRow("struct variable") << "main.cpp" << 39 << ItemList{
        makeItem(5, 7, Usage::Type::Declaration), makeItem(6, 15, Usage::Type::WritableRef),
        makeItem(8, 9, Usage::Type::WritableRef), makeItem(9, 11, Usage::Type::WritableRef),
        makeItem(11, 4, Usage::Type::Write), makeItem(11, 11, Usage::Type::WritableRef),
        makeItem(12, 12, Usage::Type::WritableRef), makeItem(13, 6, Usage::Type::Write),
        makeItem(14, 21, Usage::Type::Read), makeItem(15, 4, Usage::Type::Write),
        makeItem(15, 12, Usage::Type::Read), makeItem(16, 22, Usage::Type::WritableRef),
        makeItem(17, 13, Usage::Type::WritableRef), makeItem(18, 20, Usage::Type::Read),
        makeItem(19, 10, Usage::Type::WritableRef), makeItem(20, 10, Usage::Type::Read),
        makeItem(21, 11, Usage::Type::WritableRef), makeItem(22, 11, Usage::Type::Read),
        makeItem(23, 10, Usage::Type::Read), makeItem(32, 4, Usage::Type::Write),
        makeItem(33, 23, Usage::Type::WritableRef), makeItem(34, 23, Usage::Type::Read),
        makeItem(35, 15, Usage::Type::WritableRef), makeItem(36, 22, Usage::Type::WritableRef),
        makeItem(37, 4, Usage::Type::Read), makeItem(38, 4, Usage::Type::WritableRef),
        makeItem(39, 6, Usage::Type::WritableRef), makeItem(40, 4, Usage::Type::Read),
        makeItem(41, 4, Usage::Type::WritableRef), makeItem(42, 4, Usage::Type::Read),
        makeItem(42, 18, Usage::Type::Read), makeItem(43, 11, Usage::Type::Write),
        makeItem(54, 4, Usage::Type::Other), makeItem(55, 4, Usage::Type::Other)};

    // Some of these are conceptually questionable, as S is a type and thus we cannot "read from"
    // or "write to" it. But it probably matches the intuitive user expectation.
    QTest::newRow("struct type") << "defs.h" << 7 << ItemList{
        makeItem(1, 7, Usage::Type::Declaration), makeItem(2, 4, Usage::Type::Declaration),
        makeItem(20, 19, Usage::Type::Other), makeItem(10, 9, Usage::Type::WritableRef),
        makeItem(12, 4, Usage::Type::Write), makeItem(44, 12, Usage::Type::Read),
        makeItem(45, 13, Usage::Type::Read), makeItem(47, 12, Usage::Type::Write),
        makeItem(50, 8, Usage::Type::Read), makeItem(51, 8, Usage::Type::Write),
        makeItem(52, 6, Usage::Type::Write), makeItem(53, 4, Usage::Type::Write),
        makeItem(56, 4, Usage::Type::Write), makeItem(56, 22, Usage::Type::Other),
        makeItem(58, 10, Usage::Type::Read), makeItem(58, 22, Usage::Type::Read),
        makeItem(59, 4, Usage::Type::Write), makeItem(59, 21, Usage::Type::Read)};

    QTest::newRow("subclass") << "defs.h" << 450 << ItemList{
        makeItem(20, 7, Usage::Type::Declaration), makeItem(5, 4, Usage::Type::Other),
        makeItem(13, 21, Usage::Type::Other), makeItem(32, 8, Usage::Type::Other)};
    QTest::newRow("array variable") << "main.cpp" << 1134 << ItemList{
        makeItem(57, 8, Usage::Type::Declaration), makeItem(58, 4, Usage::Type::Write),
        makeItem(59, 15, Usage::Type::Read)};
}

// The main point here is to test our access type categorization.
// We do not try to stress-test clangd's "Find References" functionality; such tests belong
// into LLVM.
void ClangdTestFindReferences::test()
{
    QFETCH(QString, fileName);
    QFETCH(int, pos);
    QFETCH(QList<SearchResultItem>, expectedResults);

    TextEditor::TextDocument * const doc = document(fileName);
    QVERIFY(doc);
    QTextCursor cursor(doc->document());
    cursor.setPosition(pos);
    client()->findUsages(doc, cursor, {});
    QVERIFY(waitForSignalOrTimeout(client(), &ClangdClient::findUsagesDone, timeOutInMs()));

    QCOMPARE(m_actualResults.size(), expectedResults.size());
    for (int i = 0; i < m_actualResults.size(); ++i) {
        const SearchResultItem &curActual = m_actualResults.at(i);
        const SearchResultItem &curExpected = expectedResults.at(i);
        QCOMPARE(curActual.mainRange().begin.line, curExpected.mainRange().begin.line);
        QCOMPARE(curActual.mainRange().begin.column, curExpected.mainRange().begin.column);
        QCOMPARE(curActual.userData(), curExpected.userData());
    }
}


ClangdTestFollowSymbol::ClangdTestFollowSymbol()
{
    setProjectFileName("follow-symbol.pro");
    setSourceFileNames({"main.cpp", "header.h"});
    setMinimumVersion(12);
}

void ClangdTestFollowSymbol::test_data()
{
    QTest::addColumn<QString>("sourceFile");
    QTest::addColumn<int>("sourceLine");
    QTest::addColumn<int>("sourceColumn");
    QTest::addColumn<QString>("targetFile");
    QTest::addColumn<int>("targetLine");
    QTest::addColumn<int>("targetColumn");

    QTest::newRow("on namespace") << "main.cpp" << 27 << 1 << "header.h" << 28 << 11;
    QTest::newRow("class ref") << "main.cpp" << 27 << 9 << "header.h" << 34 << 7;
    QTest::newRow("forward decl (same file)") << "header.h" << 32 << 7 << "header.h" << 34 << 7;
    QTest::newRow("forward decl (different file") << "header.h" << 48 << 9 << "main.cpp" << 54 << 7;
    QTest::newRow("class definition (same file)") << "header.h" << 34 << 7 << "header.h" << 32 << 7;
    QTest::newRow("class definition (different file)") << "main.cpp" << 54 << 7
                                                       << "header.h" << 48 << 7;
    QTest::newRow("constructor decl") << "header.h" << 36 << 5 << "main.cpp" << 27 << 14;
    QTest::newRow("constructor definition") << "main.cpp" << 27 << 14 << "header.h" << 36 << 5;
    QTest::newRow("member ref") << "main.cpp" << 39 << 10 << "header.h" << 38 << 18;
    QTest::newRow("union member ref") << "main.cpp" << 91 << 20 << "main.cpp" << 86 << 13;
    QTest::newRow("member decl") << "header.h" << 38 << 18 << "header.h" << 38 << 18;
    QTest::newRow("function ref") << "main.cpp" << 66 << 12 << "main.cpp" << 35 << 5;
    QTest::newRow("member function ref") << "main.cpp" << 42 << 12 << "main.cpp" << 49 << 21;
    QTest::newRow("function with no def ref") << "main.cpp" << 43 << 5 << "header.h" << 59 << 5;
    QTest::newRow("function def") << "main.cpp" << 35 << 5 << "header.h" << 52 << 5;
    QTest::newRow("member function def") << "main.cpp" << 49 << 21 << "header.h" << 43 << 9;
    QTest::newRow("member function decl") << "header.h" << 43 << 9 << "main.cpp" << 49 << 21;
    QTest::newRow("include") << "main.cpp" << 25 << 13 << "header.h" << 1 << 1;
    QTest::newRow("local var") << "main.cpp" << 39 << 6 << "main.cpp" << 36 << 9;
    QTest::newRow("alias") << "main.cpp" << 36 << 5 << "main.cpp" << 33 << 7;
    QTest::newRow("static var") << "main.cpp" << 40 << 27 << "header.h" << 30 << 7;
    QTest::newRow("member function ref (other class)") << "main.cpp" << 62 << 39
                                                       << "cursor.cpp" << 104 << 22;
    QTest::newRow("macro ref") << "main.cpp" << 66 << 43 << "header.h" << 27 << 9;
    QTest::newRow("on namespace 2") << "main.cpp" << 27 << 3 << "header.h" << 28 << 11;
    QTest::newRow("after namespace") << "main.cpp" << 27 << 7 << "header.h" << 28 << 11;
    QTest::newRow("operator def") << "main.cpp" << 76 << 13 << "main.cpp" << 72 << 9;
    QTest::newRow("operator def 2") << "main.cpp" << 80 << 15 << "main.cpp" << 73 << 10;
    QTest::newRow("operator decl") << "main.cpp" << 72 << 12 << "main.cpp" << 76 << 10;
    QTest::newRow("operator decl 2") << "main.cpp" << 73 << 12 << "main.cpp" << 80 << 11;
}

void ClangdTestFollowSymbol::test()
{
    QFETCH(QString, sourceFile);
    QFETCH(int, sourceLine);
    QFETCH(int, sourceColumn);
    QFETCH(QString, targetFile);
    QFETCH(int, targetLine);
    QFETCH(int, targetColumn);

    TextEditor::TextDocument * const doc = document(sourceFile);
    QVERIFY(doc);

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    Utils::Link actualLink;
    const auto handler = [&actualLink, &loop](const Utils::Link &l) {
        actualLink = l;
        loop.quit();
    };
    QTextCursor cursor(doc->document());
    const int pos = Utils::Text::positionInText(doc->document(), sourceLine, sourceColumn);
    cursor.setPosition(pos);
    client()->followSymbol(doc, cursor, nullptr, handler, true, false);
    timer.start(10000);
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();

    QCOMPARE(actualLink.targetFilePath, filePath(targetFile));
    QEXPECT_FAIL("union member ref", "FIXME: clangd points to union", Abort);
    QCOMPARE(actualLink.targetLine, targetLine);
    QCOMPARE(actualLink.targetColumn + 1, targetColumn);
}

} // namespace Tests
} // namespace Internal
} // namespace ClangCodeModel
