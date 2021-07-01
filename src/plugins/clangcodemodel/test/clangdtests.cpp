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

#include <clangsupport/sourcelocationscontainer.h>
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

#include <tuple>

using namespace CPlusPlus;
using namespace Core;
using namespace CppTools::Tests;
using namespace ProjectExplorer;

namespace ClangCodeModel {
namespace Internal {
namespace Tests {

using Range = std::tuple<int, int, int>;

} // namespace Tests
} // namespace Internal
} // namespace ClangCodeModel

Q_DECLARE_METATYPE(ClangCodeModel::Internal::Tests::Range)

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
    const QString clangdFromEnv = qEnvironmentVariable("QTC_CLANGD");
    if (!clangdFromEnv.isEmpty())
        CppTools::ClangdSettings::setClangdFilePath(Utils::FilePath::fromString(clangdFromEnv));
    const auto clangd = CppTools::ClangdSettings::instance().clangdFilePath();
    if (clangd.isEmpty() || !clangd.exists())
        QSKIP("clangd binary not found");
    CppTools::ClangdSettings::setUseClangd(true);

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
    if (!m_client->isFullyIndexed()) {
        QVERIFY(waitForSignalOrTimeout(m_client, &ClangdClient::indexingFinished,
                                       clangdIndexingTimeout()));
    }
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


ClangdTestLocalReferences::ClangdTestLocalReferences()
{
    setProjectFileName("local-references.pro");
    setSourceFileNames({"references.cpp"});
    setMinimumVersion(13);
}

// We currently only support local variables, but if and when clangd implements
// the linkedEditingRange request, we can change the expected values for
// the file-scope test cases from empty ranges to the actual locations.
void ClangdTestLocalReferences::test_data()
{
    QTest::addColumn<int>("sourceLine");
    QTest::addColumn<int>("sourceColumn");
    QTest::addColumn<QList<Range>>("expectedRanges");

    QTest::newRow("cursor not on identifier") << 3 << 5 << QList<Range>();
    QTest::newRow("local variable, one use") << 3 << 9 << QList<Range>{{3, 9, 3}};
    QTest::newRow("local variable, two uses") << 10 << 9
                                              << QList<Range>{{10, 9, 3}, {11, 12, 3}};
    QTest::newRow("class name") << 16 << 7 << QList<Range>()
            /* QList<Range>{{16, 7, 3}, {19, 5, 3}} */;
    QTest::newRow("namespace") << 24 << 11 << QList<Range>()
            /* QList<Range>{{24, 11, 1}, {25, 11, 1}, {26, 1, 1}} */;
    QTest::newRow("class name via using") << 30 << 21 << QList<Range>()
            /* QList<Range>{{30, 21, 3}, {31, 10, 3}} */;
    QTest::newRow("forward-declared class") << 35 << 7 << QList<Range>()
            /* QList<Range>{{35, 7, 3}, {36, 14, 3}} */;
    QTest::newRow("class name and new expression") << 40 << 7 << QList<Range>()
            /* QList<Range>{{40, 7, 3}, {43, 9, 3}} */;
    QTest::newRow("instantiated template object") << 52 << 19
                                                  << QList<Range>{{52, 19, 3}, {53, 5, 3}};
    QTest::newRow("variable in template") << 62 << 13 << QList<Range>()
            /* QList<Range>{{62, 13, 3}, {63, 11, 3}} */;
    QTest::newRow("member in template") << 67 << 7 << QList<Range>()
            /* QList<Range>{{64, 16, 3}, {67, 7, 3}} */;
    QTest::newRow("template type") << 58 << 19 << QList<Range>()
            /* QList<Range>{{58, 19, 1}, {60, 5, 1}, {67, 5, 1}} */;
    QTest::newRow("template parameter member access") << 76 << 9 << QList<Range>();
    QTest::newRow("constructor as type") << 82 << 5 << QList<Range>()
            /* QList<Range>{{81, 8, 3}, {82, 5, 3}, {83, 6, 3}} */;
    QTest::newRow("freestanding overloads") << 88 << 5 << QList<Range>()
            /* QList<Range>{{88, 5, 3}, {89, 5, 3}} */;
    QTest::newRow("member function overloads") << 94 << 9 << QList<Range>()
            /* QList<Range>{{94, 9, 3}, {95, 9, 3}} */;
    QTest::newRow("function and function template") << 100 << 26 << QList<Range>()
            /* QList<Range>{{100, 26, 3}, {101, 5, 3}} */;
    QTest::newRow("function and function template as member") << 106 << 30 << QList<Range>()
            /* QList<Range>{{106, 30, 3}, {107, 9, 3}} */;
    QTest::newRow("enum type") << 112 << 6 << QList<Range>()
            /* QList<Range>{{112, 6, 2}, {113, 8, 2}} */;
    QTest::newRow("captured lambda var") << 122 << 15
                                         << QList<Range>{{122, 15, 3}, {122, 33, 3}};
    QTest::newRow("lambda initializer") << 122 << 19
                                         << QList<Range>{{121, 19, 3}, {122, 19, 3}};
    QTest::newRow("template specialization") << 127 << 25 << QList<Range>()
            /* QList<Range>{{127, 5, 3}, {128, 25, 3}, {129, 18, 3}} */;
    QTest::newRow("dependent name") << 133 << 34 << QList<Range>()
            /* QList<Range>{{133, 34, 3} */;
    QTest::newRow("function call and definition") << 140 << 5 << QList<Range>()
            /* QList<Range>{{140, 5, 3}, {142, 25, 3}} */;
    QTest::newRow("object-like macro") << 147 << 9 << QList<Range>()
            /* QList<Range>{{147, 9, 3}, {150, 12, 3}} */;
    QTest::newRow("function-like macro") << 155 << 9 << QList<Range>()
            /* QList<Range>{{155, 9, 3}, {158, 12, 3}} */;
    QTest::newRow("argument to function-like macro") << 156 << 27
            << QList<Range>{{156, 27, 3}, {158, 16, 3}};
    QTest::newRow("overloaded bracket operator argument") << 172 << 7
            << QList<Range>{{171, 7, 1}, {172, 7, 1}, {172, 12, 1},
                     {173, 7, 1}, {173, 10, 1}};
    QTest::newRow("overloaded call operator second argument") << 173 << 10
            << QList<Range>{{171, 7, 1}, {172, 7, 1}, {172, 12, 1},
                     {173, 7, 1}, {173, 10, 1}};
    QTest::newRow("overloaded operators arguments from outside") << 171 << 7
            << QList<Range>{{171, 7, 1}, {172, 7, 1}, {172, 12, 1},
                     {173, 7, 1}, {173, 10, 1}};
}

void ClangdTestLocalReferences::test()
{
    QFETCH(int, sourceLine);
    QFETCH(int, sourceColumn);
    QFETCH(QList<Range>, expectedRanges);

    TextEditor::TextDocument * const doc = document("references.cpp");
    QVERIFY(doc);

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QList<Range> actualRanges;
    const auto handler = [&actualRanges, &loop](const QString &symbol,
            const ClangBackEnd::SourceLocationsContainer &container, int) {
        for (const ClangBackEnd::SourceLocationContainer &c
             : container.m_sourceLocationContainers) {
            actualRanges << Range(c.line, c.column, symbol.length());
        }
        loop.quit();
    };

    QTextCursor cursor(doc->document());
    const int pos = Utils::Text::positionInText(doc->document(), sourceLine, sourceColumn);
    cursor.setPosition(pos);
    client()->findLocalUsages(doc, cursor, std::move(handler));
    timer.start(10000);
    loop.exec();
    QEXPECT_FAIL("cursor not on identifier", "clangd bug: go to definition does not return", Abort);
    QEXPECT_FAIL("template parameter member access",
                 "clangd bug: go to definition does not return", Abort);
    QVERIFY(timer.isActive());
    timer.stop();

    QCOMPARE(actualRanges, expectedRanges);
}


// This tests our help item construction, not the actual tooltip contents. Those come
// pre-formatted from clangd.
ClangdTestTooltips::ClangdTestTooltips()
{
    setProjectFileName("tooltips.pro");
    setSourceFileNames({"tooltips.cpp"});
    setMinimumVersion(13);
}

void ClangdTestTooltips::test_data()
{
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("column");
    QTest::addColumn<QStringList>("expectedIds");
    QTest::addColumn<QString>("expectedMark");
    QTest::addColumn<int>("expectedCategory");

    QTest::newRow("LocalParameterVariableConstRefCustomType") << 12 << 12
            << QStringList("Foo") << QString("Foo") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("LocalNonParameterVariableConstRefCustomType") << 14 << 5
            << QStringList("Foo") << QString("Foo") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("MemberVariableBuiltinType") << 12 << 16
            << QStringList() << QString() << int(HelpItem::Unknown);
    QTest::newRow("MemberFunctionCall") << 21 << 9
            << QStringList{"Bar::mem", "mem"} << QString("mem()") << int(HelpItem::Function);
    QTest::newRow("TemplateFunctionCall") << 30 << 5
            << QStringList{"t"} << QString("t(int)") << int(HelpItem::Function);
    QTest::newRow("Enum") << 49 << 12
            << QStringList{"EnumType"} << QString("EnumType") << int(HelpItem::Enum);
    QTest::newRow("Enumerator") << 49 << 22
            << QStringList{"Custom"} << QString("EnumType") << int(HelpItem::Enum);
    QTest::newRow("TemplateTypeFromParameter") << 55 << 25
            << QStringList{"Baz"} << QString("Baz") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("TemplateTypeFromNonParameter") << 56 << 19
            << QStringList{"Baz"} << QString("Baz") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("IncludeDirective") << 59 << 11 << QStringList{"tooltipinfo.h"}
            << QString("tooltipinfo.h") << int(HelpItem::Brief);
    QTest::newRow("MacroUse") << 66 << 5 << QStringList{"MACRO_FROM_MAINFILE"}
            << QString("MACRO_FROM_MAINFILE") << int(HelpItem::Macro);
    QTest::newRow("TypeNameIntroducedByUsingDirectiveQualified") << 77 << 5
            << QStringList{"N::Muu", "Muu"} << QString("Muu") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("TypeNameIntroducedByUsingDirectiveResolvedAndQualified") << 82 << 5
            << QStringList{"N::Muu", "Muu"} << QString("Muu") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("TypeNameIntroducedByUsingDeclarationQualified") << 87 << 5
            << QStringList{"N::Muu", "Muu"} << QString("Muu") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("Namespace") << 106 << 11
            << QStringList{"X"} << QString("X") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("NamespaceQualified") << 107 << 11
            << QStringList{"X::Y", "Y"} << QString("Y") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("TypeName_ResolveTypeDef") << 122 << 5 << QStringList{"PtrFromTypeDef"}
            << QString("PtrFromTypeDef") << int(HelpItem::Typedef);
    QTest::newRow("TypeName_ResolveAlias") << 123 << 5 << QStringList{"PtrFromTypeAlias"}
            << QString("PtrFromTypeAlias") << int(HelpItem::Typedef);
    QTest::newRow("TypeName_ResolveTemplateTypeAlias") << 124 << 5
            << QStringList{"PtrFromTemplateTypeAlias"} << QString("PtrFromTemplateTypeAlias")
            << int(HelpItem::Typedef);
    QTest::newRow("TemplateClassReference") << 134 << 5
            << QStringList{"Zii"} << QString("Zii") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("TemplateClassQualified") << 135 << 5
            << QStringList{"U::Yii", "Yii"} << QString("Yii") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("ResolveNamespaceAliasForType") << 144 << 8
            << QStringList{"A::X", "X"} << QString("X") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("ResolveNamespaceAlias") << 144 << 5
            << QStringList{"B"} << QString("B") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("QualificationForTemplateClassInClassInNamespace") << 153 << 16
            << QStringList{"N::Outer::Inner", "Outer::Inner", "Inner"} << QString("Inner")
            << int(HelpItem::ClassOrNamespace);
    QTest::newRow("Function") << 165 << 5 << QStringList{"f"} << QString("f()")
            << int(HelpItem::Function);
    QTest::newRow("Function_QualifiedName") << 166 << 8
            << QStringList{"R::f", "f"} << QString("f()") << int(HelpItem::Function);
    QTest::newRow("FunctionWithParameter") << 167 << 5 << QStringList{"f"} << QString("f(int)")
            << int(HelpItem::Function);
    QTest::newRow("FunctionWithDefaultValue") << 168 << 5
            << QStringList{"z"} << QString("z(int)") << int(HelpItem::Function);
    QTest::newRow("PointerToPointerToClass") << 200 << 12
            << QStringList{"Nuu"} << QString("Nuu") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("AutoTypeEnum") << 177 << 10
            << QStringList{"EnumType"} << QString("EnumType") << int(HelpItem::Enum);
    QTest::newRow("AutoTypeClass") << 178 << 10
            << QStringList{"Bar"} << QString("Bar") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("AutoTypeTemplate") << 179 << 10
            << QStringList{"Zii"} << QString("Zii") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("Function_DefaultConstructor") << 193 << 5
            << QStringList{"Con"} << QString("Con") << int(HelpItem::ClassOrNamespace);
    QTest::newRow("Function_ExplicitDefaultConstructor") << 194 << 5
            << QStringList{"ExplicitCon"} << QString("ExplicitCon")
            << int(HelpItem::ClassOrNamespace);
    QTest::newRow("Function_CustomConstructor") << 195 << 5
            << QStringList{"ExplicitCon"} << QString("ExplicitCon")
            << int(HelpItem::ClassOrNamespace);
}

void ClangdTestTooltips::test()
{
    QFETCH(int, line);
    QFETCH(int, column);
    QFETCH(QStringList, expectedIds);
    QFETCH(QString, expectedMark);
    QFETCH(int, expectedCategory);

    TextEditor::TextDocument * const doc = document("tooltips.cpp");
    QVERIFY(doc);
    const auto editor = qobject_cast<TextEditor::BaseTextEditor *>(EditorManager::currentEditor());
    QVERIFY(editor);
    QCOMPARE(editor->document(), doc);
    QVERIFY(editor->editorWidget());

    if (QLatin1String(QTest::currentDataTag()) == "IncludeDirective")
        QSKIP("FIXME: clangd sends empty or no hover data for includes");

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    HelpItem helpItem;
    const auto handler = [&helpItem, &loop](const HelpItem &h) {
        helpItem = h;
        loop.quit();
    };
    connect(client(), &ClangdClient::helpItemGathered, handler);

    QTextCursor cursor(doc->document());
    const int pos = Utils::Text::positionInText(doc->document(), line, column);
    cursor.setPosition(pos);
    editor->editorWidget()->processTooltipRequest(cursor);

    timer.start(10000);
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();

    QEXPECT_FAIL("TypeName_ResolveTemplateTypeAlias", "typedef already resolved in AST", Abort);
    QCOMPARE(int(helpItem.category()), expectedCategory);
    QEXPECT_FAIL("TemplateClassQualified", "Additional look-up needed?", Abort);
    QEXPECT_FAIL("AutoTypeTemplate", "Additional look-up needed?", Abort);
    QCOMPARE(helpItem.helpIds(), expectedIds);
    QCOMPARE(helpItem.docMark(), expectedMark);
}

} // namespace Tests
} // namespace Internal
} // namespace ClangCodeModel
