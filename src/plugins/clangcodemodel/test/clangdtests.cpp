// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdtests.h"

#include "../clangdclient.h"
#include "../clangdfollowsymbol.h"
#include "../clangmodelmanagersupport.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cplusplus/FindUsages.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cppeditorwidget.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/cpptoolstestcase.h>
#include <cppeditor/semantichighlighter.h>
#include <languageclient/languageclientmanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <texteditor/codeassist/assistproposaliteminterface.h>
#include <texteditor/codeassist/textdocumentmanipulatorinterface.h>
#include <texteditor/textmark.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/textutils.h>
#include <qtsupport/qtkitaspect.h>

#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QPair>
#include <QScopedPointer>
#include <QTimer>
#include <QtTest>

#include <tuple>

using namespace CPlusPlus;
using namespace Core;
using namespace CppEditor::Tests;
using namespace LanguageClient;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace ClangCodeModel {
namespace Internal {
namespace Tests {

using Range = std::tuple<int, int, int>;

} // namespace Tests
} // namespace Internal
} // namespace ClangCodeModel

Q_DECLARE_METATYPE(ClangCodeModel::Internal::Tests::Range)
Q_DECLARE_METATYPE(IAssistProposal *)

namespace ClangCodeModel {
namespace Internal {
namespace Tests {

const Usage::Tags Initialization{Usage::Tag::Declaration, Usage::Tag::Write};

static QString qrcPath(const QString &relativeFilePath)
{
    return ":/unittests/ClangCodeModel/" + relativeFilePath;
}

static Q_LOGGING_CATEGORY(debug, "qtc.clangcodemodel.batch", QtWarningMsg);

static int timeOutFromEnvironmentVariable()
{
    bool isConversionOk = false;
    const int intervalAsInt = Utils::qtcEnvironmentVariableIntValue("QTC_CLANG_BATCH_TIMEOUT",
                                                                    &isConversionOk);
    if (!isConversionOk) {
        qCDebug(debug, "Environment variable QTC_CLANG_BATCH_TIMEOUT is not set, assuming 30000.");
        return 30000;
    }

    return intervalAsInt;
}

int timeOutInMs()
{
    static int timeOut = timeOutFromEnvironmentVariable();
    return timeOut;
}

ClangdTest::~ClangdTest()
{
    EditorManager::closeAllEditors(false);
    if (m_project)
        ProjectExplorerPlugin::unloadProject(m_project);
    delete m_projectDir;
}

FilePath ClangdTest::filePath(const QString &fileName) const
{
    return m_projectDir->absolutePath(fileName);
}

void ClangdTest::waitForNewClient(bool withIndex)
{
    // Setting up the project should result in a clangd client being created.
    // Wait until that has happened.
    m_client = nullptr;
    m_client = ClangModelManagerSupport::clientForProject(project());
    if (!m_client) {
        QVERIFY(waitForSignalOrTimeout(LanguageClientManager::instance(),
                                       &LanguageClientManager::clientAdded, timeOutInMs()));
        m_client = ClangModelManagerSupport::clientForProject(project());
    }
    QVERIFY(m_client);
    m_client->enableTesting();

    // Wait until the client is fully initialized, i.e. it's completed the handshake
    // with the server.
    if (!m_client->reachable())
        QVERIFY(waitForSignalOrTimeout(m_client, &ClangdClient::initialized, timeOutInMs()));
    QVERIFY(m_client->reachable());

    if (m_minVersion != -1 && m_client->versionNumber() < QVersionNumber(m_minVersion))
        QSKIP("clangd is too old");

    // Wait for index to build.
    if (withIndex) {
        if (!m_client->isFullyIndexed()) {
            QVERIFY(waitForSignalOrTimeout(m_client, &ClangdClient::indexingFinished,
                                           clangdIndexingTimeout()));
        }
        QVERIFY(m_client->isFullyIndexed());
    }
}

void ClangdTest::initTestCase()
{
    const QString clangdFromEnv = Utils::qtcEnvironmentVariable("QTC_CLANGD");
    if (!clangdFromEnv.isEmpty())
        CppEditor::ClangdSettings::setClangdFilePath(FilePath::fromString(clangdFromEnv));
    const auto clangd = CppEditor::ClangdSettings::instance().clangdFilePath();
    if (clangd.isEmpty() || !clangd.exists())
        QSKIP("clangd binary not found");
    CppEditor::ClangdSettings::setUseClangd(true);

    // Find suitable kit.
    m_kit = Utils::findOr(KitManager::kits(), nullptr, [](const Kit *k) {
        return k->isValid() && QtSupport::QtKitAspect::qtVersion(k);
    });
    if (!m_kit)
        QSKIP("The test requires at least one valid kit with a valid Qt");

    // Copy project out of qrc file, open it, and set up target.
    m_projectDir = new TemporaryCopiedDir(qrcPath(QFileInfo(m_projectFileName).baseName()));
    QVERIFY(m_projectDir->isValid());
    const auto openProjectResult = ProjectExplorerPlugin::openProject(
            m_projectDir->absolutePath(m_projectFileName));
    QVERIFY2(openProjectResult, qPrintable(openProjectResult.errorMessage()));
    m_project = openProjectResult.project();
    m_project->configureAsExampleProject(m_kit);

    waitForNewClient();
    QVERIFY(m_client);

    // Open cpp documents.
    for (const QString &sourceFileName : std::as_const(m_sourceFileNames)) {
        const auto sourceFilePath = m_projectDir->absolutePath(sourceFileName);
        QVERIFY2(sourceFilePath.exists(), qPrintable(sourceFilePath.toUserOutput()));
        IEditor * const editor = EditorManager::openEditor(sourceFilePath);
        QVERIFY(editor);
        const auto doc = qobject_cast<TextEditor::TextDocument *>(editor->document());
        QVERIFY(doc);
        QVERIFY2(m_client->documentForFilePath(sourceFilePath) == doc,
                 qPrintable(sourceFilePath.toUserOutput()));
        doc->setSuspendAllowed(false);
        m_sourceDocuments.insert(sourceFileName, doc);
    }
}

ClangdTestFindReferences::ClangdTestFindReferences()
{
    setProjectFileName("find-usages.pro");
    setSourceFileNames({"defs.h", "main.cpp"});
}

void ClangdTestFindReferences::initTestCase()
{
    ClangdTest::initTestCase();
    CppEditor::codeModelSettings()->setCategorizeFindReferences(true);
    connect(client(), &ClangdClient::foundReferences, this,
            [this](const SearchResultItems &results) {
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
    QTest::addColumn<SearchResultItems>("expectedResults");

    static const auto makeItem = [](int line, int column, Usage::Tags tags) {
        SearchResultItem item;
        item.setMainRange(line, column, 0);
        item.setUserData(tags.toInt());
        return item;
    };

    QTest::newRow("struct member") << "defs.h" << 55 << SearchResultItems{
        makeItem(2, 17, Usage::Tag::Read), makeItem(3, 15, Usage::Tag::Declaration),
        makeItem(6, 17, Usage::Tag::WritableRef), makeItem(8, 11, Usage::Tag::WritableRef),
        makeItem(9, 13, Usage::Tag::WritableRef), makeItem(10, 12, Usage::Tag::WritableRef),
        makeItem(11, 13, Usage::Tag::WritableRef), makeItem(12, 14, Usage::Tag::WritableRef),
        makeItem(13, 26, Usage::Tag::WritableRef), makeItem(14, 23, Usage::Tag::Read),
        makeItem(15, 14, Usage::Tag::Read), makeItem(16, 24, Usage::Tag::WritableRef),
        makeItem(17, 15, Usage::Tag::WritableRef), makeItem(18, 22, Usage::Tag::Read),
        makeItem(19, 12, Usage::Tag::WritableRef), makeItem(20, 12, Usage::Tag::Read),
        makeItem(21, 13, Usage::Tag::WritableRef), makeItem(22, 13, Usage::Tag::Read),
        makeItem(23, 12, Usage::Tag::Read), makeItem(42, 20, Usage::Tag::Read),
        makeItem(44, 15, Usage::Tag::Read), makeItem(47, 15, Usage::Tag::Write),
        makeItem(50, 11, Usage::Tag::Read), makeItem(51, 11, Usage::Tag::Write),
        makeItem(52, 9, Usage::Tag::Write), makeItem(53, 7, Usage::Tag::Write),
        makeItem(56, 7, Usage::Tag::Write), makeItem(56, 25, Usage::Tags()),
        makeItem(58, 13, Usage::Tag::Read), makeItem(58, 25, Usage::Tag::Read),
        makeItem(59, 7, Usage::Tag::Write), makeItem(59, 24, Usage::Tag::Read)};
    QTest::newRow("constructor member initialization") << "defs.h" << 68 << SearchResultItems{
        makeItem(2, 10, Usage::Tag::Write), makeItem(4, 8, Usage::Tag::Declaration)};
    QTest::newRow("direct member initialization") << "defs.h" << 101 << SearchResultItems{
        makeItem(5, 21, Initialization), makeItem(45, 16, Usage::Tag::Read)};

    SearchResultItems pureVirtualRefs{makeItem(17, 17, Usage::Tag::Declaration),
                             makeItem(21, 9, {Usage::Tag::Declaration, Usage::Tag::Override})};
    QTest::newRow("pure virtual declaration") << "defs.h" << 420 << pureVirtualRefs;

    QTest::newRow("pointer variable") << "main.cpp" << 52 << SearchResultItems{
        makeItem(6, 10, Initialization), makeItem(8, 4, Usage::Tag::Write),
        makeItem(10, 4, Usage::Tag::Write), makeItem(24, 5, Usage::Tag::Write),
        makeItem(25, 11, Usage::Tag::WritableRef), makeItem(26, 11, Usage::Tag::Read),
        makeItem(27, 10, Usage::Tag::WritableRef), makeItem(28, 10, Usage::Tag::Read),
        makeItem(29, 11, Usage::Tag::Read), makeItem(30, 15, Usage::Tag::WritableRef),
        makeItem(31, 22, Usage::Tag::Read)};
    QTest::newRow("struct variable") << "main.cpp" << 39 << SearchResultItems{
        makeItem(5, 7, Usage::Tag::Declaration), makeItem(6, 15, Usage::Tag::WritableRef),
        makeItem(8, 9, Usage::Tag::WritableRef), makeItem(9, 11, Usage::Tag::WritableRef),
        makeItem(11, 4, Usage::Tag::Write), makeItem(11, 11, Usage::Tag::WritableRef),
        makeItem(12, 12, Usage::Tag::WritableRef), makeItem(13, 6, Usage::Tag::Write),
        makeItem(14, 21, Usage::Tag::Read), makeItem(15, 4, Usage::Tag::Write),
        makeItem(15, 12, Usage::Tag::Read), makeItem(16, 22, Usage::Tag::WritableRef),
        makeItem(17, 13, Usage::Tag::WritableRef), makeItem(18, 20, Usage::Tag::Read),
        makeItem(19, 10, Usage::Tag::WritableRef), makeItem(20, 10, Usage::Tag::Read),
        makeItem(21, 11, Usage::Tag::WritableRef), makeItem(22, 11, Usage::Tag::Read),
        makeItem(23, 10, Usage::Tag::Read), makeItem(32, 4, Usage::Tag::Write),
        makeItem(33, 23, Usage::Tag::WritableRef), makeItem(34, 23, Usage::Tag::Read),
        makeItem(35, 15, Usage::Tag::WritableRef), makeItem(36, 22, Usage::Tag::WritableRef),
        makeItem(37, 4, Usage::Tag::Read), makeItem(38, 4, Usage::Tag::WritableRef),
        makeItem(39, 6, Usage::Tag::WritableRef), makeItem(40, 4, Usage::Tag::Read),
        makeItem(41, 4, Usage::Tag::WritableRef), makeItem(42, 4, Usage::Tag::Read),
        makeItem(42, 18, Usage::Tag::Read), makeItem(43, 11, Usage::Tag::Write),
        makeItem(54, 4, Usage::Tags()), makeItem(55, 4, Usage::Tags())};

    // Some of these are conceptually questionable, as S is a type and thus we cannot "read from"
    // or "write to" it. But it probably matches the intuitive user expectation.
    QTest::newRow("struct type") << "defs.h" << 7 << SearchResultItems{
        makeItem(1, 7, Usage::Tag::Declaration),
        makeItem(2, 4, (Usage::Tags{Usage::Tag::Declaration, Usage::Tag::ConstructorDestructor})),
        makeItem(20, 19, Usage::Tags()), makeItem(10, 9, Usage::Tag::WritableRef),
        makeItem(12, 4, Usage::Tag::Write), makeItem(44, 12, Usage::Tag::Read),
        makeItem(45, 13, Usage::Tag::Read), makeItem(47, 12, Usage::Tag::Write),
        makeItem(50, 8, Usage::Tag::Read), makeItem(51, 8, Usage::Tag::Write),
        makeItem(52, 6, Usage::Tag::Write), makeItem(53, 4, Usage::Tag::Write),
        makeItem(56, 4, Usage::Tag::Write), makeItem(56, 22, Usage::Tags()),
        makeItem(58, 10, Usage::Tag::Read), makeItem(58, 22, Usage::Tag::Read),
        makeItem(59, 4, Usage::Tag::Write), makeItem(59, 21, Usage::Tag::Read)};

    QTest::newRow("struct type 2") << "defs.h" << 450 << SearchResultItems{
        makeItem(20, 7, Usage::Tag::Declaration), makeItem(5, 4, Usage::Tags()),
        makeItem(13, 21, Usage::Tags()), makeItem(32, 8, Usage::Tags())};

    QTest::newRow("constructor") << "defs.h" << 627 << SearchResultItems{
        makeItem(31, 4, (Usage::Tags{Usage::Tag::Declaration, Usage::Tag::ConstructorDestructor})),
        makeItem(36, 7, Usage::Tag::ConstructorDestructor)};

    QTest::newRow("subclass") << "defs.h" << 450 << SearchResultItems{
        makeItem(20, 7, Usage::Tag::Declaration), makeItem(5, 4, Usage::Tags()),
        makeItem(13, 21, Usage::Tags()), makeItem(32, 8, Usage::Tags())};
    QTest::newRow("array variable") << "main.cpp" << 1134 << SearchResultItems{
        makeItem(57, 8, Usage::Tag::Declaration), makeItem(58, 4, Usage::Tag::Write),
        makeItem(59, 15, Usage::Tag::Read)};
    QTest::newRow("free function") << "defs.h" << 510 << SearchResultItems{
        makeItem(24, 5, Usage::Tag::Declaration), makeItem(19, 4, Usage::Tags()),
        makeItem(25, 4, Usage::Tags()), makeItem(60, 26, Usage::Tag::Read)};
    QTest::newRow("member function") << "defs.h" << 192 << SearchResultItems{
        makeItem(9, 12, Usage::Tag::Declaration), makeItem(40, 8, Usage::Tags())};
}

// The main point here is to test our access type categorization.
// We do not try to stress-test clangd's "Find References" functionality; such tests belong
// into LLVM.
void ClangdTestFindReferences::test()
{
    QFETCH(QString, fileName);
    QFETCH(int, pos);
    QFETCH(SearchResultItems, expectedResults);

    TextEditor::TextDocument * const doc = document(fileName);
    QVERIFY(doc);
    QTextCursor cursor(doc->document());
    cursor.setPosition(pos);
    client()->findUsages(CppEditor::CursorInEditor(cursor, doc->filePath(), nullptr, doc), {}, {});
    QVERIFY(waitForSignalOrTimeout(client(), &ClangdClient::findUsagesDone, timeOutInMs()));

    QCOMPARE(m_actualResults.size(), expectedResults.size());
    for (int i = 0; i < m_actualResults.size(); ++i) {
        const SearchResultItem &curActual = m_actualResults.at(i);
        const SearchResultItem &curExpected = expectedResults.at(i);
        QCOMPARE(curActual.mainRange().begin.line, curExpected.mainRange().begin.line);
        QCOMPARE(curActual.mainRange().begin.column, curExpected.mainRange().begin.column);
        const auto actualTags = Usage::Tags::fromInt(curActual.userData().toInt())
                & ~Usage::Tags(Usage::Tag::Used);
        const auto expectedTags = Usage::Tags::fromInt(curExpected.userData().toInt());
        QCOMPARE(actualTags, expectedTags);
    }
}


ClangdTestFollowSymbol::ClangdTestFollowSymbol()
{
    setProjectFileName("follow-symbol.pro");
    setSourceFileNames({"main.cpp", "header.h"});
}

void ClangdTestFollowSymbol::test_data()
{
    QTest::addColumn<QString>("sourceFile");
    QTest::addColumn<int>("sourceLine");
    QTest::addColumn<int>("sourceColumn");
    QTest::addColumn<QString>("targetFile");
    QTest::addColumn<int>("targetLine");
    QTest::addColumn<int>("targetColumn");
    QTest::addColumn<bool>("goToType");

    QTest::newRow("on namespace") << "main.cpp" << 27 << 1 << "header.h" << 28 << 11 << false;
    QTest::newRow("class ref") << "main.cpp" << 27 << 9 << "header.h" << 34 << 7 << false;
    QTest::newRow("forward decl (same file)") << "header.h" << 32 << 7 << "header.h" << 34 << 7
                                              << false;
    QTest::newRow("forward decl (different file") << "header.h" << 48 << 9 << "main.cpp" << 54 << 7
                                                  << false;
    QTest::newRow("class definition (same file)") << "header.h" << 34 << 7 << "header.h" << 32 << 7
                                                  << false;
    QTest::newRow("class definition (different file)") << "main.cpp" << 54 << 7
                                                       << "header.h" << 48 << 7  << false;
    QTest::newRow("constructor decl") << "header.h" << 36 << 5 << "main.cpp" << 27 << 14 << false;
    QTest::newRow("constructor definition") << "main.cpp" << 27 << 14 << "header.h" << 36 << 5
                                            << false;
    QTest::newRow("member ref") << "main.cpp" << 39 << 10 << "header.h" << 38 << 18 << false;
    QTest::newRow("union member ref") << "main.cpp" << 91 << 20 << "main.cpp" << 86 << 13 << false;
    QTest::newRow("member decl") << "header.h" << 38 << 18 << "header.h" << 38 << 18 << false;
    QTest::newRow("function ref") << "main.cpp" << 66 << 12 << "main.cpp" << 35 << 5 << false;
    QTest::newRow("member function ref") << "main.cpp" << 42 << 12 << "main.cpp" << 49 << 21
                                         << false;
    QTest::newRow("function with no def ref") << "main.cpp" << 43 << 5 << "header.h" << 59 << 5
                                              << false;
    QTest::newRow("function def") << "main.cpp" << 35 << 5 << "header.h" << 52 << 5 << false;
    QTest::newRow("member function def") << "main.cpp" << 49 << 21 << "header.h" << 43 << 9
                                         << false;
    QTest::newRow("member function decl") << "header.h" << 43 << 9 << "main.cpp" << 49 << 21
                                          << false;
    QTest::newRow("include") << "main.cpp" << 25 << 13 << "header.h" << 1 << 1 << false;
    QTest::newRow("local var") << "main.cpp" << 39 << 6 << "main.cpp" << 36 << 9 << false;
    QTest::newRow("alias") << "main.cpp" << 36 << 5 << "main.cpp" << 33 << 7 << false;
    QTest::newRow("static var") << "main.cpp" << 40 << 27 << "header.h" << 30 << 7 << false;
    QTest::newRow("member function ref (other class)") << "main.cpp" << 62 << 39
                                                       << "cursor.cpp" << 104 << 22 << false;
    QTest::newRow("macro ref") << "main.cpp" << 66 << 43 << "header.h" << 27 << 9 << false;
    QTest::newRow("on namespace 2") << "main.cpp" << 27 << 3 << "header.h" << 28 << 11 << false;
    QTest::newRow("after namespace") << "main.cpp" << 27 << 7 << "header.h" << 28 << 11 << false;
    QTest::newRow("operator def") << "main.cpp" << 76 << 13 << "main.cpp" << 72 << 9 << false;
    QTest::newRow("operator def 2") << "main.cpp" << 80 << 15 << "main.cpp" << 73 << 10 << false;
    QTest::newRow("operator decl") << "main.cpp" << 72 << 12 << "main.cpp" << 76 << 10 << false;
    QTest::newRow("operator decl 2") << "main.cpp" << 73 << 12 << "main.cpp" << 80 << 11 << false;

    QTest::newRow("go to typedef") << "main.cpp" << 100 << 19 << "main.cpp" << 33 << 7 << true;
    QTest::newRow("go to type") << "main.cpp" << 101 << 19 << "main.cpp" << 69 << 7 << true;
}

void ClangdTestFollowSymbol::test()
{
    QFETCH(QString, sourceFile);
    QFETCH(int, sourceLine);
    QFETCH(int, sourceColumn);
    QFETCH(QString, targetFile);
    QFETCH(int, targetLine);
    QFETCH(int, targetColumn);
    QFETCH(bool, goToType);

    TextEditor::TextDocument * const doc = document(sourceFile);
    QVERIFY(doc);

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    Link actualLink;
    const auto handler = [&actualLink, &loop](const Link &l) {
        actualLink = l;
        loop.quit();
    };
    QTextCursor cursor(doc->document());
    const int pos = Text::positionInText(doc->document(), sourceLine, sourceColumn);
    cursor.setPosition(pos);
    client()->followSymbol(doc, cursor, nullptr, handler, true,
                           goToType ? FollowTo::SymbolType : FollowTo::SymbolDef, false);
    timer.start(10000);
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();

    QCOMPARE(actualLink.targetFilePath, filePath(targetFile));
    QCOMPARE(actualLink.targetLine, targetLine);
    QCOMPARE(actualLink.targetColumn + 1, targetColumn);
}

// Make sure it is safe to call follow symbol in a follow symbol handler. Since follow symbol
// potentially opens a file that gets loaded in chunks which handles user events inbetween
// the chunks we can potentially call a follow symbol while currently handling another one.
void ClangdTestFollowSymbol::testFollowSymbolInHandler()
{
    TextEditor::TextDocument *const doc = document("header.h");
    QVERIFY(doc);
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    QTextCursor cursor(doc->document());
    const int pos = Text::positionInText(doc->document(), 48, 9);
    cursor.setPosition(pos);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    bool deleted = false;

    const auto handler = [&](const Link &) {
        client()->followSymbol(doc, cursor, nullptr, [&](const Link &) { loop.quit(); }, true,
                               FollowTo::SymbolDef, false);
        QVERIFY(!deleted);
    };

    client()->followSymbol(doc, cursor, nullptr, handler, true, FollowTo::SymbolDef, false);
    QVERIFY(client()->currentFollowSymbolOperation());
    connect(client()->currentFollowSymbolOperation(), &QObject::destroyed, [&] { deleted = true; });
    timer.start(10000);
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
}

ClangdTestLocalReferences::ClangdTestLocalReferences()
{
    setProjectFileName("local-references.pro");
    setSourceFileNames({"references.cpp"});
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
    QTest::newRow("local variable, one use") << 3 << 9 << QList<Range>{{3, 8, 3}};
    QTest::newRow("local variable, two uses") << 10 << 9
                                              << QList<Range>{{10, 8, 3}, {11, 11, 3}};
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
                                                  << QList<Range>{{52, 18, 3}, {53, 4, 3}};
    QTest::newRow("variable in template") << 62 << 13
                                          << QList<Range>{{62, 12, 3}, {63, 10, 3}};
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
                                         << QList<Range>{{122, 14, 3}, {122, 32, 3}};
    QTest::newRow("lambda initializer") << 122 << 19
                                         << QList<Range>{{121, 18, 3}, {122, 18, 3}};
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
            << QList<Range>{{156, 26, 3}, {158, 15, 3}};
    QTest::newRow("overloaded bracket operator argument") << 172 << 7
            << QList<Range>{{171, 6, 1}, {172, 6, 1}, {172, 11, 1},
                     {173, 6, 1}, {173, 9, 1}};
    QTest::newRow("overloaded call operator second argument") << 173 << 10
            << QList<Range>{{171, 6, 1}, {172, 6, 1}, {172, 11, 1},
                     {173, 6, 1}, {173, 9, 1}};
    QTest::newRow("overloaded operators arguments from outside") << 171 << 7
            << QList<Range>{{171, 6, 1}, {172, 6, 1}, {172, 11, 1},
                     {173, 6, 1}, {173, 9, 1}};
    QTest::newRow("documented function parameter") << 181 << 32
            << QList<Range>{{177, 10, 6}, {179, 9, 6}, {181, 31, 6}, {183, 6, 6}, {184, 17, 6}};
}

void ClangdTestLocalReferences::test()
{
    QFETCH(int, sourceLine);
    QFETCH(int, sourceColumn);
    QFETCH(QList<Range>, expectedRanges);

    TextEditor::TextDocument * const doc = document("references.cpp");
    QVERIFY(doc);
    const QList<BaseTextEditor *> editors = BaseTextEditor::textEditorsForDocument(doc);
    QCOMPARE(editors.size(), 1);
    const auto editorWidget = qobject_cast<CppEditor::CppEditorWidget *>(
        editors.first()->editorWidget());
    QVERIFY(editorWidget);

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QList<Range> actualRanges;
    const auto handler = [&actualRanges, &loop](const QString &symbol, const Links &links, int) {
        for (const Link &link : links)
            actualRanges << Range(link.targetLine, link.targetColumn, symbol.length());
        loop.quit();
    };

    QTextCursor cursor(doc->document());
    const int pos = Text::positionInText(doc->document(), sourceLine, sourceColumn);
    cursor.setPosition(pos);
    client()->findLocalUsages(editorWidget, cursor, std::move(handler));
    timer.start(10000);
    loop.exec();
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

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    HelpItem helpItem;
    const auto handler = [&helpItem, &loop](const HelpItem &h) {
        helpItem = h;
        loop.quit();
    };
    connect(client(), &ClangdClient::helpItemGathered, &loop, handler);

    QTextCursor cursor(doc->document());
    const int pos = Text::positionInText(doc->document(), line, column);
    cursor.setPosition(pos);
    editor->editorWidget()->processTooltipRequest(cursor);

    timer.start(10000);
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();

    QEXPECT_FAIL("TypeName_ResolveTemplateTypeAlias", "typedef already resolved in AST", Abort);
    QEXPECT_FAIL("TypeNameIntroducedByUsingDeclarationQualified",
                 "https://github.com/clangd/clangd/issues/989", Abort);
    QCOMPARE(int(helpItem.category()), expectedCategory);
    QEXPECT_FAIL("TemplateClassQualified", "Additional look-up needed?", Abort);
    QCOMPARE(helpItem.helpIds(), expectedIds);
    QCOMPARE(helpItem.docMark(), expectedMark);
}

ClangdTestHighlighting::ClangdTestHighlighting()
{
    setProjectFileName("highlighting.pro");
    setSourceFileNames({"highlighting.cpp"});
}

void ClangdTestHighlighting::initTestCase()
{
    ClangdTest::initTestCase();

    connect(document("highlighting.cpp"), &TextDocument::ifdefedOutBlocksChanged, this,
            [this](const QList<BlockRange> &ranges) { m_ifdefedOutBlocks = ranges; });
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    const auto handler = [this, &loop](const TextEditor::HighlightingResults &results) {
        m_results = results;
        loop.quit();
    };
    connect(client(), &ClangdClient::highlightingResultsReady, &loop, handler);
    timer.start(10000);
    loop.exec();
    QVERIFY(timer.isActive());
    QVERIFY(!m_results.isEmpty());
}

void ClangdTestHighlighting::test_data()
{
    QTest::addColumn<int>("firstLine");
    QTest::addColumn<int>("startColumn");
    QTest::addColumn<int>("lastLine");
    QTest::addColumn<int>("endColumn");
    QTest::addColumn<QList<int>>("expectedStyles");
    QTest::addColumn<int>("expectedKind");

    QTest::newRow("function definition") << 45 << 5 << 45 << 13
        << QList<int>{C_FUNCTION, C_FUNCTION_DEFINITION, C_DECLARATION} << 0;
    QTest::newRow("member function definition") << 52 << 10 << 52 << 24
        << QList<int>{C_FUNCTION, C_FUNCTION_DEFINITION, C_DECLARATION} << 0;
    QTest::newRow("virtual member function definition outside of class body")
        << 586 << 17 << 586 << 32
        << QList<int>{C_VIRTUAL_METHOD, C_FUNCTION_DEFINITION, C_DECLARATION} << 0;
    QTest::newRow("virtual member function definition inside class body")
        << 589 << 16 << 589 << 41
        << QList<int>{C_VIRTUAL_METHOD, C_FUNCTION_DEFINITION, C_DECLARATION} << 0;
    QTest::newRow("function declaration") << 55 << 5 << 55 << 24
        << QList<int>{C_FUNCTION, C_DECLARATION} << 0;
    QTest::newRow("member function declaration") << 59 << 10 << 59 << 24
        << QList<int>{C_FUNCTION, C_DECLARATION} << 0;
    QTest::newRow("member function call") << 104 << 9 << 104 << 32
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("function call") << 64 << 5 << 64 << 13 << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("type conversion function (struct)") << 68 << 14 << 68 << 17
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("type reference") << 74 << 5 << 74 << 8 << QList<int>{C_TYPE} << 0;
    QTest::newRow("local variable declaration") << 79 << 9 << 79 << 12
        << QList<int>{C_LOCAL, C_DECLARATION} << 0;
    QTest::newRow("local variable reference") << 81 << 5 << 81 << 8 << QList<int>{C_LOCAL} << 0;
    QTest::newRow("function parameter declaration") << 84 << 41 << 84 << 44
        << QList<int>{C_PARAMETER, C_DECLARATION} << 0;
    QTest::newRow("function parameter reference") << 86 << 5 << 86 << 8
        << QList<int>{C_PARAMETER} << 0;
    QTest::newRow("member declaration") << 90 << 9 << 90 << 20
        << QList<int>{C_FIELD, C_DECLARATION} << 0;
    QTest::newRow("member reference") << 94 << 9 << 94 << 20 << QList<int>{C_FIELD} << 0;
    QTest::newRow("static member function declaration") << 110 << 10 << 110 << 22
        << QList<int>{C_FUNCTION, C_DECLARATION} << 0;
    QTest::newRow("static member function call") << 114 << 15 << 114 << 27
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("enum declaration") << 118 << 6 << 118 << 17
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("enumerator declaration") << 120 << 5 << 120 << 15
        << QList<int>{C_ENUMERATION, C_DECLARATION} << 0;
    QTest::newRow("enum in variable declaration") << 125 << 5 << 125 << 16
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("enum variable declaration") << 125 << 17 << 125 << 28
        << QList<int>{C_LOCAL, C_DECLARATION} << 0;
    QTest::newRow("enum variable reference") << 127 << 5 << 127 << 16 << QList<int>{C_LOCAL} << 0;
    QTest::newRow("enumerator reference") << 127 << 19 << 127 << 29
        << QList<int>{C_ENUMERATION} << 0;
    QTest::newRow("forward declaration") << 130 << 7 << 130 << 23
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("constructor declaration") << 134 << 5 << 134 << 10
        << QList<int>{C_FUNCTION, C_DECLARATION} << 0;
    QTest::newRow("destructor declaration") << 135 << 6 << 135 << 11
        << QList<int>{C_FUNCTION, C_DECLARATION} << 0;
    QTest::newRow("reference to forward-declared class") << 138 << 1 << 138 << 17
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("class in variable declaration") << 140 << 5 << 140 << 10
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("class variable declaration") << 140 << 11 << 140 << 31
        << QList<int>{C_LOCAL, C_DECLARATION} << 0;
    QTest::newRow("union declaration") << 145 << 7 << 145 << 12
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("union in global variable declaration") << 150 << 1 << 150 << 6
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("global variable declaration") << 150 << 7 << 150 << 32
        << QList<int>{C_GLOBAL, C_DECLARATION} << 0;
    QTest::newRow("struct declaration") << 50 << 8 << 50 << 11
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("namespace declaration") << 160 << 11 << 160 << 20
        << QList<int>{C_NAMESPACE, C_DECLARATION} << 0;
    QTest::newRow("namespace alias declaration") << 164 << 11 << 164 << 25
        << QList<int>{C_NAMESPACE, C_DECLARATION} << 0;
    QTest::newRow("struct in namespaced using declaration") << 165 << 18 << 165 << 35
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("namespace reference") << 166 << 1 << 166 << 10 << QList<int>{C_NAMESPACE} << 0;
    QTest::newRow("namespaced struct in global variable declaration") << 166 << 12 << 166 << 29
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("virtual function declaration") << 170 << 18 << 170 << 33
        << QList<int>{C_VIRTUAL_METHOD, C_DECLARATION} << 0;
    QTest::newRow("virtual function call via pointer") << 192 << 33 << 192 << 48
        << QList<int>{C_VIRTUAL_METHOD} << 0;
    QTest::newRow("final virtual function call via pointer") << 202 << 38 << 202 << 58
        << QList<int>{C_VIRTUAL_METHOD} << 0;
    QTest::newRow("non-final virtual function call via pointer") << 207 << 41 << 207 << 61
        << QList<int>{C_VIRTUAL_METHOD} << 0;
    QTest::newRow("operator+ declaration") << 220 << 18 << 220 << 19
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator+ call") << 224 << 36 << 224 << 37
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator+= call") << 226 << 24 << 226 << 26
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator* member declaration") << 604 << 18 << 604 << 19
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator* non-member declaration") << 607 << 14 << 607 << 15
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator* member call") << 613 << 7 << 613 << 8
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator* non-member call") << 614 << 7 << 614 << 8
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator<<= member declaration") << 618 << 19 << 618 << 22
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator<<= call") << 629 << 12 << 629 << 15
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator(int) member declaration (opening paren") << 619 << 19 << 619 << 20
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator(int) member declaration (closing paren") << 619 << 20 << 619 << 21
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator(int) call (opening parenthesis)") << 632 << 12 << 632 << 13
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator(int) call (closing parenthesis)") << 632 << 14 << 632 << 15
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator[] member declaration (opening bracket") << 620 << 18 << 620 << 19
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator[] member declaration (closing bracket") << 620 << 20 << 620 << 21
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator[] call (opening bracket)") << 633 << 12 << 633 << 13
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator[] call (closing bracket)") << 633 << 14 << 633 << 15
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator new member declaration") << 621 << 20 << 621 << 23
        << QList<int>{C_KEYWORD, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator new member call") << 635 << 22 << 635 << 25
        << QList<int>{C_KEYWORD, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator delete member declaration") << 622 << 19 << 622 << 25
        << QList<int>{C_KEYWORD, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator delete member call") << 636 << 5 << 636 << 11
        << QList<int>{C_KEYWORD, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("var after operator delete member call") << 636 << 12 << 636 << 19
        << QList<int>{C_LOCAL} << 0;
    QTest::newRow("operator new[] member declaration (keyword)") << 623 << 20 << 623 << 23
        << QList<int>{C_KEYWORD, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator new[] member call (keyword") << 637 << 19 << 637 << 22
        << QList<int>{C_KEYWORD, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator new[] member call (type argument)") << 637 << 23 << 637 << 28
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("operator delete[] member declaration (keyword)") << 624 << 19 << 624 << 25
        << QList<int>{C_KEYWORD, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator delete[] member call (keyword") << 638 << 5 << 638 << 11
        << QList<int>{C_KEYWORD, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator new built-in call") << 634 << 14 << 634 << 17
        << QList<int>{C_KEYWORD, C_OPERATOR} << 0;
    QTest::newRow("operator() member declaration (opening paren") << 654 << 20 << 654 << 21
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator() member declaration (closing paren") << 654 << 21 << 654 << 22
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator() call (opening parenthesis)") << 662 << 11 << 662 << 12
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator() call (closing parenthesis)") << 662 << 12 << 662 << 13
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator* member declaration (2)") << 655 << 17 << 655 << 18
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator* member call (2)") << 663 << 5 << 663 << 6
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("operator= member declaration") << 656 << 20 << 656 << 21
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR, C_DECLARATION} << 0;
    QTest::newRow("operator= call") << 664 << 12 << 664 << 13
        << QList<int>{C_PUNCTUATION, C_OPERATOR, C_OVERLOADED_OPERATOR} << 0;
    QTest::newRow("ternary operator (question mark)") << 668 << 18 << 668 << 19
        << QList<int>{C_PUNCTUATION, C_OPERATOR} << int(CppEditor::SemanticHighlighter::TernaryIf);
    QTest::newRow("ternary operator (colon)") << 668 << 23 << 668 << 24
        << QList<int>{C_PUNCTUATION, C_OPERATOR} << int(CppEditor::SemanticHighlighter::TernaryElse);
    QTest::newRow("opening angle bracket in function template declaration")
        << 247 << 10 << 247 << 11
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("closing angle bracket in function template declaration")
        << 247 << 18 << 247 << 19
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("opening angle bracket in class template declaration") << 261 << 10 << 261 << 11
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("closing angle bracket in class template declaration") << 261 << 18 << 261 << 19
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("macro definition") << 231 << 9 << 231 << 31
        << QList<int>{C_MACRO, C_DECLARATION} << 0;
    QTest::newRow("function-like macro definition") << 232 << 9 << 232 << 24
        << QList<int>{C_MACRO, C_DECLARATION} << 0;
    QTest::newRow("function-like macro call") << 236 << 5 << 236 << 20
        << QList<int>{C_MACRO} << 0;
    QTest::newRow("function template call") << 254 << 5 << 254 << 21 << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("template type parameter") << 265 << 17 << 265 << 38
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("template parameter default argument") << 265 << 41 << 265 << 44
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("template non-type parameter") << 265 << 50 << 265 << 74
        << QList<int>{C_PARAMETER, C_DECLARATION} << 0;
    QTest::newRow("template template parameter") << 265 << 103 << 265 << 128
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("template template parameter default argument") << 265 << 131 << 265 << 142
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("outer opening angle bracket in nested template declaration")
        << 265 << 10 << 265 << 11
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("inner opening angle bracket in nested template declaration")
        << 265 << 89 << 265 << 90
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("inner closing angle bracket in nested template declaration")
        << 265 << 95 << 265 << 96
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("outer closing angle bracket in nested template declaration")
        << 265 << 142 << 265 << 143
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("function template declaration") << 266 << 6 << 266 << 22
        << QList<int>{C_FUNCTION, C_FUNCTION_DEFINITION, C_DECLARATION} << 0;
    QTest::newRow("template type parameter reference") << 268 << 5 << 268 << 26
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("local var declaration of template parameter type") << 268 << 27 << 268 << 57
        << QList<int>{C_LOCAL, C_DECLARATION} << 0;
    QTest::newRow("reference to non-type template parameter") << 269 << 46 << 269 << 70
        << QList<int>{C_PARAMETER} << 0;
    QTest::newRow("local var declaration initialized with non-type template parameter")
        << 269 << 10 << 269 << 43
        << QList<int>{C_LOCAL, C_DECLARATION} << 0;
    QTest::newRow("template template parameter reference") << 270 << 5 << 270 << 30
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("template type parameter reference in template instantiation")
        << 270 << 31 << 270 << 52
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("local var declaration of template template parameter type")
        << 270 << 54 << 270 << 88
        << QList<int>{C_LOCAL, C_DECLARATION} << 0;
    QTest::newRow("local variable declaration as argument to function-like macro call")
        << 302 << 18 << 302 << 23
        << QList<int>{C_LOCAL, C_DECLARATION} << 0;
    QTest::newRow("local variable as argument to function-like macro call")
        << 302 << 25 << 302 << 34
        << QList<int>{C_LOCAL} << 0;
    QTest::newRow("class member as argument to function-like macro call")
        << 310 << 29 << 310 << 38
        << QList<int>{C_FIELD} << 0;
    QTest::newRow("enum declaration with underlying type") << 316 << 6 << 316 << 21
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("type in static_cast") << 328 << 23 << 328 << 33
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("opening angle bracket in static_cast") << 328 << 16 << 328 << 17
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("closing angle bracket in static_cast") << 328 << 39 << 328 << 40
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("type in reinterpret_cast") << 329 << 28 << 329 << 38
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("integer alias declaration") << 333 << 7 << 333 << 25
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("integer alias in declaration") << 341 << 5 << 341 << 17
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("recursive integer alias in declaration") << 342 << 5 << 342 << 23
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("integer typedef in declaration") << 343 << 5 << 343 << 19
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("call to function pointer alias") << 344 << 5 << 344 << 13
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("friend class declaration") << 350 << 18 << 350 << 27
        << (client()->versionNumber().majorVersion() >= 16
            ? QList<int>{C_TYPE, C_DECLARATION}: QList<int>{C_TYPE}) << 0;
    QTest::newRow("friend class reference") << 351 << 34 << 351 << 43
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("function parameter of friend class type") << 351 << 45 << 351 << 50
        << QList<int>{C_PARAMETER, C_DECLARATION} << 0;
    QTest::newRow("constructor member initialization") << 358 << 9 << 358 << 15
        << QList<int>{C_FIELD} << 0;
    QTest::newRow("call to function template") << 372 << 5 << 372 << 25
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("class template declaration") << 377 << 7 << 377 << 20
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("class template instantiation (name)") << 384 << 5 << 384 << 18
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("class template instantiation (opening angle bracket)") << 384 << 18 << 384 << 19
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("class template instantiation (closing angle bracket)") << 384 << 22 << 384 << 23
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("static member function decl") << 395 << 17 << 395 << 30
        << QList<int>{C_STATIC_MEMBER, C_DECLARATION, C_FUNCTION} << 0;
    QTest::newRow("static member function call") << 400 << 17 << 400 << 30
        << QList<int>{C_STATIC_MEMBER, C_FUNCTION} << 0;
    QTest::newRow("namespace in declaration") << 413 << 4 << 413 << 26
                                              << QList<int>{C_NAMESPACE} << 0;
    QTest::newRow("namespaced class in declaration") << 413 << 28 << 413 << 41
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("class as template argument in declaration") << 413 << 42 << 413 << 52
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("local variable declaration of template instance type") << 413 << 54 << 413 << 77
        << QList<int>{C_LOCAL, C_DECLARATION} << 0;
    QTest::newRow("local typedef declaration") << 418 << 17 << 418 << 35
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("local typedef in variable declaration") << 419 << 5 << 419 << 23
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("non-const reference argument") << 455 << 31 << 455 << 32
        << QList<int>{C_LOCAL, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("const reference argument") << 464 << 28 << 464 << 29
        << QList<int>{C_LOCAL} << 0;
    QTest::newRow("rvalue reference argument") << 473 << 48 << 473 << 49
        << QList<int>{C_LOCAL} << 0;
    QTest::newRow("non-const pointer argument") << 482 << 29 << 482 << 30
        << QList<int>{C_LOCAL, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("pointer to const argument") << 490 << 28 << 490 << 29
        << QList<int>{C_LOCAL} << 0;
    QTest::newRow("const pointer argument") << 491 << 26 << 491 << 27
        << QList<int>{C_LOCAL, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("value argument") << 501 << 57 << 501 << 58
        << QList<int>{C_LOCAL} << 0;
    QTest::newRow("non-const ref argument as second arg") << 501 << 61 << 501 << 62
        << QList<int>{C_LOCAL, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("non-const ref argument from function parameter") << 506 << 31 << 506 << 40
        << QList<int>{C_PARAMETER, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("non-const pointer argument expression") << 513 << 30 << 513 << 31
        << QList<int>{C_LOCAL, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("non-const ref argument from qualified member (member)") << 525 << 40 << 525 << 46
        << QList<int>{C_FIELD, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("non-const ref argument to constructor") << 540 << 47 << 540 << 55
        << QList<int>{C_LOCAL, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("non-const ref argument to member initialization") << 546 << 15 << 546 << 18
        << QList<int>{C_PARAMETER, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("typedef as underlying type in enum declaration") << 424 << 21 << 424 << 39
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("argument 1 to user-defined subscript operator") << 434 << 5 << 434 << 11
        << QList<int>{C_PARAMETER} << 0;
    QTest::newRow("argument 2 to user-defined subscript operator") << 434 << 12 << 434 << 17
        << QList<int>{C_PARAMETER, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("partial class template specialization") << 553 << 25 << 553 << 28
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("using declaration for function") << 556 << 10 << 556 << 13
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("variable in operator() call") << 566 << 7 << 566 << 10
        << QList<int>{C_PARAMETER} << 0;
    QTest::newRow("using declaration for function template") << 584 << 10 << 584 << 16
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("Q_PROPERTY (macro name)") << 599 << 5 << 599 << 15
        << QList<int>{C_MACRO} << 0;
    QTest::newRow("Q_PROPERTY (property name)") << 599 << 52 << 599 << 56
        << QList<int>{C_FIELD} << 0;
    QTest::newRow("Q_PROPERTY (READ keyword)") << 599 << 57 << 599 << 61
        << QList<int>{C_KEYWORD} << 0;
    QTest::newRow("Q_PROPERTY (getter)") << 599 << 62 << 599 << 69
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("Q_PROPERTY (WRITE keyword)") << 599 << 70 << 599 << 75
        << QList<int>{C_KEYWORD} << 0;
    QTest::newRow("Q_PROPERTY (setter)") << 599 << 76 << 599 << 83
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("Q_PROPERTY (NOTIFY keyword)") << 599 << 84 << 599 << 90
        << QList<int>{C_KEYWORD} << 0;
    QTest::newRow("Q_PROPERTY (notifier)") << 599 << 91 << 599 << 102
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("Q_PROPERTY (SCRIPTABLE keyword)") << 599 << 103 << 599 << 113
        << QList<int>{C_KEYWORD} << 0;
    QTest::newRow("Q_PROPERTY (REVISION keyword)") << 599 << 119 << 599 << 127
        << QList<int>{C_KEYWORD} << 0;
    QTest::newRow("Q_PROPERTY (type)") << 600 << 22 << 600 << 29
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("Q_PROPERTY (REVISION keyword [new])") << 600 << 46 << 600 << 54
        << QList<int>{C_KEYWORD} << 0;
    QTest::newRow("multi-line Q_PROPERTY (macro name)") << 704 << 5 << 704 << 15
        << QList<int>{C_MACRO} << 0;
    QTest::newRow("multi-line Q_PROPERTY (property name)") << 718 << 13 << 718 << 17
        << QList<int>{C_FIELD} << 0;
    QTest::newRow("multi-line Q_PROPERTY (getter)") << 722 << 13 << 722 << 20
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("multi-line Q_PROPERTY (notifier)") << 730 << 13 << 730 << 24
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("old-style signal (macro)") << 672 << 5 << 672 << 11
        << QList<int>{C_MACRO} << 0;
    QTest::newRow("old-style signal (signal)") << 672 << 12 << 672 << 21
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("old-style signal (signal parameter)") << 672 << 22 << 672 << 29
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("old-style slot (macro)") << 673 << 5 << 673 << 9
        << QList<int>{C_MACRO} << 0;
    QTest::newRow("old-style slot (slot)") << 673 << 10 << 673 << 19
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("old-style slot (slot parameter)") << 673 << 20 << 673 << 27
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("old-style signal with complex parameter (macro)") << 674 << 5 << 674 << 11
        << QList<int>{C_MACRO} << 0;
    QTest::newRow("old-style signal with complex parameter (signal)") << 674 << 12 << 674 << 21
        << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("old-style signal with complex parameter (signal parameter part 1)")
        << 674 << 22 << 674 << 29
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("old-style signal with complex parameter (signal parameter part 2)")
        << 674 << 32 << 674 << 37
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("old-style signal with complex parameter (signal parameter part 3)")
        << 674 << 39 << 674 << 46
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("constructor parameter") << 681 << 64 << 681 << 88
        << QList<int>{C_PARAMETER, C_DECLARATION} << 0;
    QTest::newRow("non-const ref argument to constructor (2)") << 686 << 42 << 686 << 45
        << QList<int>{C_LOCAL, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("local variable captured by lambda") << 442 << 24 << 442 << 27
        << QList<int>{C_LOCAL} << 0;
    QTest::newRow("static protected member") << 693 << 16 << 693 << 30
        << QList<int>{C_STATIC_MEMBER, C_DECLARATION} << 0;
    QTest::newRow("static private member") << 696 << 16 << 696 << 28
        << QList<int>{C_STATIC_MEMBER, C_DECLARATION} << 0;
    QTest::newRow("alias template declaration (opening angle bracket)") << 700 << 10 << 700 << 11
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("alias template declaration (closing angle bracket)") << 700 << 16 << 700 << 17
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("alias template declaration (new type)") << 700 << 24 << 700 << 28
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("alias template declaration (base type)") << 700 << 31 << 700 << 32
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("alias template declaration (base type opening angle bracket)")
        << 700 << 32 << 700 << 33
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("alias template declaration (base type closing angle bracket)")
        << 700 << 37 << 700 << 38
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("alias template instantiation (type)") << 701 << 1 << 701 << 5
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("alias template instantiation (opening angle bracket)") << 701 << 5 << 701 << 6
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("alias template instantiation (closing angle bracket)") << 701 << 7 << 701 << 8
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("function template specialization (opening angle bracket 1)")
        << 802 << 9 << 802 << 10
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("function template specialization (closing angle bracket 1)")
        << 802 << 10 << 802 << 11
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("function template specialization (function name)")
        << 802 << 17 << 802 << 29
        << QList<int>{C_FUNCTION, C_FUNCTION_DEFINITION, C_DECLARATION} << 0;
    QTest::newRow("function template specialization (opening angle bracket 2)")
        << 802 << 29 << 802 << 30
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("function template specialization (closing angle bracket 2)")
        << 802 << 33 << 802 << 34
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("class template specialization (opening angle bracket 1)")
        << 804 << 9 << 804 << 10
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("class template specialization (closing angle bracket 1)")
        << 804 << 10 << 804 << 11
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("class template specialization (class name)")
        << 804 << 18 << 804 << 21
        << QList<int>{C_TYPE, C_DECLARATION} << 0;
    QTest::newRow("class template specialization (opening angle bracket 2)")
        << 804 << 21 << 804 << 22
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("class template specialization (closing angle bracket 2)")
        << 804 << 25 << 804 << 26
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("structured binding (var 1)") << 737 << 17 << 737 << 18
        << QList<int>{C_LOCAL, C_DECLARATION} << 0;
    QTest::newRow("structured binding (var 2)") << 737 << 20 << 737 << 21
        << QList<int>{C_LOCAL, C_DECLARATION} << 0;
    QTest::newRow("local var via indirect macro") << 746 << 20 << 746 << 30
        << QList<int>{C_LOCAL} << 0;
    QTest::newRow("global variable in multi-dimensional array") << 752 << 13 << 752 << 23
        << QList<int>{C_GLOBAL} << 0;
    QTest::newRow("reference to global variable") << 764 << 5 << 764 << 14
        << QList<int>{C_GLOBAL} << 0;
    QTest::newRow("nested template instantiation (namespace 1)") << 773 << 8 << 773 << 11
        << QList<int>{C_NAMESPACE} << 0;
    QTest::newRow("nested template instantiation (type 1)") << 773 << 13 << 773 << 19
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("nested template instantiation (opening angle bracket 1)")
        << 773 << 19 << 773 << 20
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("nested template instantiation (namespace 2)") << 773 << 20 << 773 << 23
        << QList<int>{C_NAMESPACE} << 0;
    QTest::newRow("nested template instantiation (type 2)") << 773 << 25 << 773 << 29
        << QList<int>{C_TYPE} << 0;
    QTest::newRow("nested template instantiation (opening angle bracket 2)")
        << 773 << 29 << 773 << 30
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("nested template instantiation (closing angle bracket 1)")
        << 773 << 38 << 773 << 39
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("nested template instantiation (closing angle bracket 2)")
        << 773 << 39 << 773 << 40
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("nested template instantiation (variable)") << 773 << 41 << 773 << 43
        << QList<int>{C_GLOBAL, C_DECLARATION} << 0;
    QTest::newRow("doubly nested template instantiation (opening angle bracket 1)")
        << 806 << 12 << 806 << 13
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("doubly nested template instantiation (opening angle bracket 2)")
        << 806 << 24 << 806 << 25
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("doubly nested template instantiation (opening angle bracket 3)")
        << 806 << 36 << 806 << 37
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("doubly nested template instantiation (closing angle bracket 1)")
        << 806 << 40 << 806 << 41
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("doubly nested template instantiation (closing angle bracket 2)")
        << 806 << 41 << 806 << 42
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("doubly nested template instantiation (closing angle bracket 3)")
        << 806 << 42 << 806 << 43
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("triply nested template instantiation with spacing (opening angle bracket 1)")
        << 808 << 13 << 808 << 14
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("triply nested template instantiation with spacing (opening angle bracket 2)")
        << 808 << 27 << 808 << 28
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("triply nested template instantiation with spacing (opening angle bracket 3)")
        << 808 << 39 << 808 << 40
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("triply nested template instantiation with spacing (opening angle bracket 4)")
        << 809 << 12 << 809 << 13
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketOpen);
    QTest::newRow("triply nested template instantiation with spacing (closing angle bracket 1)")
        << 810 << 1 << 810 << 2
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("triply nested template instantiation with spacing (closing angle bracket 2)")
        << 810 << 2 << 810 << 3
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("triply nested template instantiation with spacing (closing angle bracket 3)")
        << 811 << 2 << 811 << 3
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("triply nested template instantiation with spacing (closing angle bracket 4)")
        << 812 << 3 << 812 << 4
        << QList<int>{C_PUNCTUATION} << int(CppEditor::SemanticHighlighter::AngleBracketClose);
    QTest::newRow("macro in struct") << 795 << 9 << 795 << 14
        << QList<int>{C_MACRO, C_DECLARATION} << 0;
    if (client()->versionNumber() < QVersionNumber(17)) {
        QTest::newRow("#ifdef'ed out code") << 800 << 1 << 800 << 17
                                            << QList<int>{C_DISABLED_CODE} << 0;
    }
    QTest::newRow("static function call (object)") << 819 << 5 << 819 << 6
        << QList<int>{C_LOCAL} << 0;
    QTest::newRow("static function call (argument)") << 819 << 18 << 819 << 19
        << QList<int>{C_LOCAL, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("override attribute") << 186 << 28 << 186 << 36 << QList<int>{C_KEYWORD} << 0;
    QTest::newRow("final attribute") << 187 << 33 << 187 << 38 << QList<int>{C_KEYWORD} << 0;
    QTest::newRow("non-const argument to named lambda") << 827 << 10 << 827 << 13
        << QList<int>{C_LOCAL, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("const argument to named lambda") << 828 << 10 << 828 << 13
        << QList<int>{C_LOCAL} << 0;
    QTest::newRow("non-const argument to unnamed lambda") << 829 << 18 << 829 << 21
        << QList<int>{C_LOCAL, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("const argument to unnamed lambda") << 830 << 16 << 830 << 19
        << QList<int>{C_LOCAL} << 0;
    QTest::newRow("simple assignment") << 835 << 5 << 835 << 6 << QList<int>{C_LOCAL} << 0;
    QTest::newRow("simple return") << 841 << 12 << 841 << 15 << QList<int>{C_LOCAL} << 0;
    QTest::newRow("lambda parameter") << 847 << 49 << 847 << 52
                                      << QList<int>{C_PARAMETER, C_DECLARATION} << 0;
    QTest::newRow("user-defined operator call") << 860 << 7 << 860 << 8
                                      << QList<int>{C_LOCAL} << 0;
    QTest::newRow("const member as function argument") << 868 << 32 << 868 << 43
                                      << QList<int>{C_FIELD} << 0;
    QTest::newRow("lambda call without arguments (const var)") << 887 << 5 << 887 << 12
                                      << QList<int>{C_LOCAL} << 0;
    QTest::newRow("lambda call without arguments (non-const var)") << 889 << 5 << 889 << 12
                                      << QList<int>{C_LOCAL} << 0;
    QTest::newRow("non-const operator()") << 898 << 5 << 898 << 7
                                      << QList<int>{C_LOCAL} << 0;
    QTest::newRow("const operator()") << 903 << 5 << 903 << 7
                                      << QList<int>{C_LOCAL} << 0;
    QTest::newRow("member initialization: member (user-defined type)") << 911 << 14 << 911 << 18
                                      << QList<int>{C_FIELD} << 0;
    QTest::newRow("member initialization: member (built-in type)") << 911 << 23 << 911 << 27
                                      << QList<int>{C_FIELD} << 0;
    QTest::newRow("operator<<") << 934 << 10 << 934 << 14 << QList<int>{C_GLOBAL} << 0;
    QTest::newRow("operator>>") << 936 << 10 << 936 << 13 << QList<int>{C_GLOBAL} << 0;
    QTest::newRow("operator>>") << 936 << 17 << 936 << 18 << QList<int>{C_LOCAL} << 0;
    QTest::newRow("input arg from passed object") << 945 << 17 << 945 << 18
                                                  << QList<int>{C_FIELD} << 0;
    QTest::newRow("output arg") << 945 << 20 << 945 << 23
                                << QList<int>{C_LOCAL, C_OUTPUT_ARGUMENT} << 0;
    QTest::newRow("built-in variable 1") << 950 << 21 << 950 << 29
                                << QList<int>{C_LOCAL} << 0;
    QTest::newRow("built-in variable 2") << 951 << 21 << 951 << 33
                                << QList<int>{C_LOCAL} << 0;
    QTest::newRow("built-in variable 3") << 952 << 21 << 952 << 40
                                << QList<int>{C_LOCAL} << 0;
    QTest::newRow("deref operator (object)") << 960 << 10 << 960 << 11 << QList<int>{C_LOCAL} << 0;
    QTest::newRow("deref operator (member)") << 960 << 12 << 960 << 13 << QList<int>{C_FIELD} << 0;
    QTest::newRow("nested call") << 979 << 20 << 979 << 21 << QList<int>{C_LOCAL} << 0;
    QTest::newRow("member call on dependent (1)") << 996 << 19 << 996 << 22
                                                  << QList<int>{C_FIELD} << 0;
    QTest::newRow("member call on dependent (2)") << 996 << 38 << 996 << 41
                                                  << QList<int>{C_FIELD} << 0;
    QTest::newRow("member call on dependent (3)") << 999 << 9 << 999 << 12
                                                  << QList<int>{C_LOCAL} << 0;
    QTest::newRow("member access via operator->") << 1009 << 7 << 1009 << 21
                                                  << QList<int>{C_FIELD} << 0;
    QTest::newRow("lambda call in member") << 1023 << 9 << 1023 << 15
                                                  << QList<int>{C_LOCAL} << 0;
    QTest::newRow("call on inherited member") << 1024 << 9 << 1024 << 12
                                                  << QList<int>{C_FIELD} << 0;
    QTest::newRow("pass inherited member by value") << 1038 << 21 << 1038 << 26
                                                    << QList<int>{C_FIELD} << 0;
    QTest::newRow("fake operator member declaration") << 1045 << 9 << 1045 << 23
                                                    << QList<int>{C_FIELD, C_DECLARATION} << 0;
    QTest::newRow("fake operator method declaration") << 1046 << 10 << 1046 << 24
                                                    << QList<int>{C_FUNCTION, C_DECLARATION} << 0;
    QTest::newRow("fake operator member access") << 1049 << 8 << 1049 << 22
                                                    << QList<int>{C_FIELD} << 0;
    QTest::newRow("fake operator method call") << 1050 << 8 << 1050 << 22
                                               << QList<int>{C_FUNCTION} << 0;
    QTest::newRow("concept definition") << 1053 << 30 << 1053 << 42
                                               << QList<int>{C_CONCEPT, C_DECLARATION} << 0;
    QTest::newRow("concept use") << 1054 << 29 << 1054 << 41 << QList<int>{C_CONCEPT} << 0;
    QTest::newRow("label declaration") << 242 << 1 << 242 << 11
                                               << QList<int>{C_LABEL, C_DECLARATION} << 0;
    QTest::newRow("label use") << 244 << 10 << 244 << 20 << QList<int>{C_LABEL} << 0;
}

void ClangdTestHighlighting::test()
{
    QFETCH(int, firstLine);
    QFETCH(int, startColumn);
    QFETCH(int, lastLine);
    QFETCH(int, endColumn);
    QFETCH(QList<int>, expectedStyles);
    QFETCH(int, expectedKind);

    const TextEditor::TextDocument * const doc = document("highlighting.cpp");
    QVERIFY(doc);
    const int startPos = Text::positionInText(doc->document(), firstLine, startColumn);
    const int endPos = Text::positionInText(doc->document(), lastLine, endColumn);

    const auto lessThan = [=](const TextEditor::HighlightingResult &r, int) {
        return Text::positionInText(doc->document(), r.line, r.column) < startPos;
    };
    const auto findResults = [=] {
        TextEditor::HighlightingResults results;
        auto it = std::lower_bound(m_results.cbegin(), m_results.cend(), 0, lessThan);
        if (it == m_results.cend())
            return results;
        while (it != m_results.cend()) {
            const int resultEndPos = Text::positionInText(doc->document(), it->line,
                                                                 it->column) + it->length;
            if (resultEndPos > endPos)
                break;
            results << *it++;
        }
        return results;
    };
    const TextEditor::HighlightingResults results = findResults();

    QEXPECT_FAIL("old-style signal (signal)", "check if and how we want to support this", Abort);
    QEXPECT_FAIL("old-style signal (signal parameter)",
                 "check if and how we want to support this", Abort);
    QEXPECT_FAIL("old-style slot (slot)", "check if and how we want to support this", Abort);
    QEXPECT_FAIL("old-style slot (slot parameter)",
                 "check if and how we want to support this", Abort);
    QEXPECT_FAIL("old-style signal with complex parameter (signal)",
                 "check if and how we want to support this", Abort);
    QEXPECT_FAIL("old-style signal with complex parameter (signal parameter part 1)",
                 "check if and how we want to support this", Abort);
    QEXPECT_FAIL("old-style signal with complex parameter (signal parameter part 2)",
                 "check if and how we want to support this", Abort);
    QEXPECT_FAIL("old-style signal with complex parameter (signal parameter part 3)",
                 "check if and how we want to support this", Abort);

    QCOMPARE(results.length(), 1);

    const TextEditor::HighlightingResult result = results.first();
    QCOMPARE(result.line, firstLine);
    QCOMPARE(result.column, startColumn);
    QCOMPARE(result.length, endPos - startPos);
    QList<int> actualStyles;
    if (result.useTextSyles) {
        actualStyles << result.textStyles.mainStyle;
        for (const TextEditor::TextStyle s : result.textStyles.mixinStyles)
            actualStyles << s;
    }

    QCOMPARE(actualStyles, expectedStyles);
    QCOMPARE(result.kind, expectedKind);
}

void ClangdTestHighlighting::testIfdefedOutBlocks()
{
    QCOMPARE(m_ifdefedOutBlocks.size(), 3);
    QCOMPARE(m_ifdefedOutBlocks.at(0).first(), 12056);
    QCOMPARE(m_ifdefedOutBlocks.at(0).last(), 12073);
    QCOMPARE(m_ifdefedOutBlocks.at(1).first(), 13374);
    QCOMPARE(m_ifdefedOutBlocks.at(1).last(), 13387);
    QCOMPARE(m_ifdefedOutBlocks.at(2).first(), 13413);
    QCOMPARE(m_ifdefedOutBlocks.at(2).last(), 13425);
}


class Manipulator : public TextDocumentManipulatorInterface
{
public:
    Manipulator()
    {
        const auto textEditor = static_cast<BaseTextEditor *>(EditorManager::currentEditor());
        QVERIFY(textEditor);
        m_doc = textEditor->textDocument()->document();
        m_cursor = textEditor->editorWidget()->textCursor();
    }

    int currentPosition() const override { return m_cursor.position(); }
    int positionAt(TextPositionOperation) const override { return 0; }
    QChar characterAt(int position) const override { return m_doc->characterAt(position); }

    QString textAt(int position, int length) const override
    {
        return m_doc->toPlainText().mid(position, length);
    }

    QTextCursor textCursorAt(int position) const override
    {
        QTextCursor cursor(m_doc);
        cursor.setPosition(position);
        return cursor;
    }

    void setCursorPosition(int position) override { m_cursor.setPosition(position); }
    void setAutoCompleteSkipPosition(int position) override { m_skipPos = position; }

    bool replace(int position, int length, const QString &text) override
    {
        QTextCursor cursor = textCursorAt(position);
        cursor.setPosition(position + length, QTextCursor::KeepAnchor);
        cursor.insertText(text);
        return true;
    }

    void insertCodeSnippet(int pos, const QString &text, const SnippetParser &parser) override
    {
        const auto parseResult = parser(text);
        if (const auto snippet = std::get_if<ParsedSnippet>(&parseResult)) {
            if (!snippet->parts.isEmpty())
                textCursorAt(pos).insertText(snippet->parts.first().text);
        }
    }

    void paste() override {}
    void encourageApply() override {}
    void autoIndent(int, int) override {}

    QString getLine(int line) const { return m_doc->findBlockByNumber(line - 1).text(); }

    QPair<int, int> cursorPos() const
    {
        const int pos = currentPosition();
        QPair<int, int> lineAndColumn;
        Text::convertPosition(m_doc, pos, &lineAndColumn.first, &lineAndColumn.second);
        return lineAndColumn;
    }

    int skipPos() const { return m_skipPos; }

private:
    QTextDocument *m_doc;
    QTextCursor m_cursor;
    int m_skipPos = -1;
};

ClangdTestCompletion::ClangdTestCompletion()
{
    setProjectFileName("completion.pro");
    setSourceFileNames({"completionWithProject.cpp", "constructorCompletion.cpp",
                        "classAndConstructorCompletion.cpp", "dotToArrowCorrection.cpp",
                        "doxygenKeywordsCompletion.cpp", "functionAddress.cpp",
                        "functionCompletion.cpp", "functionCompletionFiltered2.cpp",
                        "functionCompletionFiltered.cpp", "globalCompletion.cpp",
                        "includeDirectiveCompletion.cpp", "mainwindow.cpp",
                        "memberCompletion.cpp", "membercompletion-friend.cpp",
                        "membercompletion-inside.cpp", "membercompletion-outside.cpp",
                        "noDotToArrowCorrectionForFloats.cpp",
                        "preprocessorKeywordsCompletion.cpp", "preprocessorKeywordsCompletion2.cpp",
                        "preprocessorKeywordsCompletion3.cpp", "privateFuncDefCompletion.cpp",
                        "signalCompletion.cpp"});
}

void ClangdTestCompletion::initTestCase()
{
    ClangdTest::initTestCase();
    startCollectingHighlightingInfo();
}

void ClangdTestCompletion::testCompleteDoxygenKeywords()
{
    ProposalModelPtr proposal;
    getProposal("doxygenKeywordsCompletion.cpp", proposal);

    QVERIFY(proposal);
    QVERIFY(hasItem(proposal, "brief"));
    QVERIFY(hasItem(proposal, "param"));
    QVERIFY(hasItem(proposal, "return"));
    QVERIFY(!hasSnippet(proposal, "class "));
}

void ClangdTestCompletion::testCompletePreprocessorKeywords()
{
    ProposalModelPtr proposal;
    getProposal("preprocessorKeywordsCompletion.cpp", proposal);
    QVERIFY(proposal);
    QVERIFY(hasItem(proposal, "ifdef"));
    QVERIFY(!hasSnippet(proposal, "class "));

    proposal.clear();
    getProposal("preprocessorKeywordsCompletion2.cpp", proposal);
    QVERIFY(proposal);
    QVERIFY(hasItem(proposal, "endif"));
    QVERIFY(!hasSnippet(proposal, "class "));

    proposal.clear();
    getProposal("preprocessorKeywordsCompletion3.cpp", proposal);
    QVERIFY(proposal);
    QVERIFY(hasItem(proposal, "endif"));
    QVERIFY(!hasSnippet(proposal, "class "));
}

void ClangdTestCompletion::testCompleteIncludeDirective()
{
    // Our local include is way down in the result list.
    client()->setCompletionResultsLimit(0);

    ProposalModelPtr proposal;
    getProposal("includeDirectiveCompletion.cpp", proposal);

    QVERIFY(proposal);
    QVERIFY(hasItem(proposal, " file.h>"));
    QVERIFY(hasItem(proposal, " otherFile.h>"));
    QVERIFY(hasItem(proposal, " mylib/"));
    QVERIFY(!hasSnippet(proposal, "class "));
    client()->setCompletionResultsLimit(-1);
}

void ClangdTestCompletion::testCompleteGlobals()
{
    ProposalModelPtr proposal;
    int cursorPos = -1;
    getProposal("globalCompletion.cpp", proposal, {}, &cursorPos);

    QVERIFY(proposal);
    QVERIFY(hasItem(proposal, " globalVariable", "int"));
    QVERIFY(hasItem(proposal, " GlobalClass"));
    QVERIFY(hasItem(proposal, " class"));    // Keyword
    QVERIFY(hasSnippet(proposal, "class ")); // Snippet

    const AssistProposalItemInterface * const item = getItem(proposal, " globalFunction()", "void");
    QVERIFY(item);
    Manipulator manipulator;
    item->apply(manipulator, cursorPos);
    QCOMPARE(manipulator.getLine(7), "   globalFunction() /* COMPLETE HERE */");
    QCOMPARE(manipulator.cursorPos(), qMakePair(7, 19));
    QCOMPARE(manipulator.skipPos(), -1);
}

void ClangdTestCompletion::testCompleteMembers()
{
    ProposalModelPtr proposal;
    int cursorPos = -1;
    getProposal("memberCompletion.cpp", proposal, {}, &cursorPos);

    QVERIFY(proposal);
    QVERIFY(!hasItem(proposal, " globalVariable"));
    QVERIFY(!hasItem(proposal, " class"));    // Keyword
    QVERIFY(!hasSnippet(proposal, "class ")); // Snippet

    const AssistProposalItemInterface * const item = getItem(proposal, " member", "int");
    QVERIFY(item);
    Manipulator manipulator;
    item->apply(manipulator, cursorPos);
    QCOMPARE(manipulator.getLine(7), "    s.member /* COMPLETE HERE */");
    QCOMPARE(manipulator.cursorPos(), qMakePair(7, 12));
    QCOMPARE(manipulator.skipPos(), -1);
}

void ClangdTestCompletion::testCompleteMembersFromInside()
{
    ProposalModelPtr proposal;
    int cursorPos = -1;
    getProposal("membercompletion-inside.cpp", proposal, {}, &cursorPos);

    QVERIFY(proposal);
    QVERIFY(hasItem(proposal, " publicFunc()", "void"));

    const AssistProposalItemInterface * const item = getItem(proposal, " privateFunc()", "void");
    QVERIFY(item);
    Manipulator manipulator;
    item->apply(manipulator, cursorPos);
    QCOMPARE(manipulator.getLine(4), "        privateFunc() /* COMPLETE HERE */");
    QCOMPARE(manipulator.cursorPos(), qMakePair(4, 21));
    QCOMPARE(manipulator.skipPos(), -1);
}

void ClangdTestCompletion::testCompleteMembersFromOutside()
{
    ProposalModelPtr proposal;
    int cursorPos = -1;
    getProposal("membercompletion-outside.cpp", proposal, {}, &cursorPos);

    QVERIFY(proposal);
    QVERIFY(!hasItem(proposal, " privateFunc", "void"));

    const AssistProposalItemInterface * const item = getItem(proposal, " publicFunc()", "void");
    QVERIFY(item);
    Manipulator manipulator;
    item->apply(manipulator, cursorPos);
    QCOMPARE(manipulator.getLine(13), "    c.publicFunc() /* COMPLETE HERE */");
    QCOMPARE(manipulator.cursorPos(), qMakePair(13, 18));
    QCOMPARE(manipulator.skipPos(), -1);
}

void ClangdTestCompletion::testCompleteMembersFromFriend()
{
    ProposalModelPtr proposal;
    int cursorPos = -1;
    getProposal("membercompletion-friend.cpp", proposal, {}, &cursorPos);

    QVERIFY(proposal);
    QVERIFY(hasItem(proposal, " publicFunc()", "void"));

    const AssistProposalItemInterface * const item = getItem(proposal, " privateFunc()", "void");
    QVERIFY(item);
    Manipulator manipulator;
    item->apply(manipulator, cursorPos);
    QCOMPARE(manipulator.getLine(14), "    C().privateFunc() /* COMPLETE HERE */");
    QCOMPARE(manipulator.cursorPos(), qMakePair(14, 21));
    QCOMPARE(manipulator.skipPos(), -1);
}

void ClangdTestCompletion::testFunctionAddress()
{
    ProposalModelPtr proposal;
    int cursorPos = -1;
    getProposal("functionAddress.cpp", proposal, {}, &cursorPos);

    QVERIFY(proposal);

    const AssistProposalItemInterface * const item = getItem(proposal, " memberFunc()", "void");
    QVERIFY(item);
    Manipulator manipulator;
    item->apply(manipulator, cursorPos);
    QCOMPARE(manipulator.getLine(7), "    const auto p = &S::memberFunc /* COMPLETE HERE */;");
    QCOMPARE(manipulator.cursorPos(), qMakePair(7, 33));
    QCOMPARE(manipulator.skipPos(), -1);
}

void ClangdTestCompletion::testFunctionHints()
{
    ProposalModelPtr proposal;
    getProposal("functionCompletion.cpp", proposal);

    QVERIFY(proposal);
    QVERIFY(hasItem(proposal, "f() -> void"));
    QVERIFY(hasItem(proposal, "f(int a) -> void"));
    QVERIFY(hasItem(proposal, "f(const QString &s) -> void"));
    QVERIFY(hasItem(proposal, "f(char c, int optional = 3) -> void"));
    QVERIFY(hasItem(proposal, "f(char c, int optional1 = 3, int optional2 = 3) -> void"));
    QVERIFY(hasItem(proposal, "f(const TType<QString> *t) -> void"));
    QVERIFY(hasItem(proposal, "f(bool) -> TType<QString>"));
}

void ClangdTestCompletion::testFunctionHintsFiltered()
{
    ProposalModelPtr proposal;
    getProposal("functionCompletionFiltered.cpp", proposal);

    QVERIFY(proposal);
    QCOMPARE(proposal->size(), 1);
    QVERIFY(hasItem(proposal, "func(int i, <b>int j</b>) -&gt; void"));

    proposal.clear();
    getProposal("functionCompletionFiltered2.cpp", proposal);

    QVERIFY(proposal);
    QCOMPARE(proposal->size(), 2);
    QVERIFY(hasItem(proposal, "func(const S &amp;s, <b>int j</b>) -&gt; void"));
    QEXPECT_FAIL("", "QTCREATORBUG-26346", Abort);
    QVERIFY(hasItem(proposal, "func(const S &amp;s, <b>int j</b>, int k) -&gt; void"));
}

void ClangdTestCompletion::testFunctionHintConstructor()
{
    ProposalModelPtr proposal;
    getProposal("constructorCompletion.cpp", proposal);

    QVERIFY(proposal);
    QVERIFY(!hasItem(proposal, "globalVariable"));
    QVERIFY(!hasItem(proposal, " class"));
    QVERIFY(hasItem(proposal, "Foo(<b>int</b>)"));
    QEXPECT_FAIL("", "QTCREATORBUG-26346", Abort);
    QVERIFY(hasItem(proposal, "Foo(<b>int</b>, double)"));
}

void ClangdTestCompletion::testCompleteClassAndConstructor()
{
    ProposalModelPtr proposal;
    int cursorPos = -1;
    getProposal("classAndConstructorCompletion.cpp", proposal, {}, &cursorPos);

    QVERIFY(proposal);
    QVERIFY(hasItem(proposal, " Foo"));

    const AssistProposalItemInterface * const item
            = getItem(proposal, QString::fromUtf8(" Foo(…)"), "[2 overloads]");
    QVERIFY(item);
    Manipulator manipulator;
    item->apply(manipulator, cursorPos);
    QCOMPARE(manipulator.getLine(7), "    Foo( /* COMPLETE HERE */");
    QCOMPARE(manipulator.cursorPos(), qMakePair(7, 8));
    QCOMPARE(manipulator.skipPos(), -1);
}

void ClangdTestCompletion::testCompletePrivateFunctionDefinition()
{
    ProposalModelPtr proposal;
    getProposal("privateFuncDefCompletion.cpp", proposal);

    QVERIFY(proposal);
    QEXPECT_FAIL("", "https://github.com/clangd/clangd/issues/880", Abort);
    QCOMPARE(proposal->size(), 1);
    QVERIFY(hasItem(proposal, " theFunc()"));
}

void ClangdTestCompletion::testCompleteWithDotToArrowCorrection()
{
    ProposalModelPtr proposal;
    int cursorPos = -1;
    getProposal("dotToArrowCorrection.cpp", proposal, ".", &cursorPos);

    QVERIFY(proposal);
    const AssistProposalItemInterface * const item = getItem(proposal, " member", "int");
    QVERIFY(item);
    Manipulator manipulator;
    item->apply(manipulator, cursorPos);
    QCOMPARE(manipulator.getLine(4), "    bar->member /* COMPLETE HERE */");
    QCOMPARE(manipulator.cursorPos(), qMakePair(4, 15));
    QCOMPARE(manipulator.skipPos(), -1);
}

void ClangdTestCompletion::testDontCompleteWithDotToArrowCorrectionForFloats()
{
    ProposalModelPtr proposal;
    getProposal("noDotToArrowCorrectionForFloats.cpp", proposal, ".");

    QVERIFY(proposal);
    for (int i = 0; i < proposal->size(); ++i) // Offer only snippets.
        QVERIFY2(hasSnippet(proposal, proposal->text(i)), qPrintable(proposal->text(i)));
}

void ClangdTestCompletion::testCompleteCodeInGeneratedUiFile()
{
    ProposalModelPtr proposal;
    int cursorPos = -1;
    getProposal("mainwindow.cpp", proposal, {}, &cursorPos);

    QVERIFY(proposal);
    QVERIFY(hasItem(proposal, " menuBar"));
    QVERIFY(hasItem(proposal, " statusBar"));
    QVERIFY(hasItem(proposal, " centralWidget"));

    const AssistProposalItemInterface * const item = getItem(
                proposal, " setupUi(QMainWindow *MainWindow)", "void");
    QVERIFY(item);
    Manipulator manipulator;
    item->apply(manipulator, cursorPos);
    QCOMPARE(manipulator.getLine(34), "    ui->setupUi( /* COMPLETE HERE */");
    QCOMPARE(manipulator.cursorPos(), qMakePair(34, 16));
    QCOMPARE(manipulator.skipPos(), -1);
}

void ClangdTestCompletion::testSignalCompletion_data()
{
    QTest::addColumn<QString>("customCode");
    QTest::addColumn<QStringList>("expectedSuggestions");

    QTest::addRow("positive: connect() on QObject class")
            << QString("QObject::connect(dummy, &QObject::")
            << QStringList{"aSignal()", "anotherSignal()"};
    QTest::addRow("positive: connect() on QObject object")
            << QString("QObject o; o.connect(dummy, &QObject::")
            << QStringList{"aSignal()", "anotherSignal()"};
    QTest::addRow("positive: connect() on QObject pointer")
            << QString("QObject *o; o->connect(dummy, &QObject::")
            << QStringList{"aSignal()", "anotherSignal()"};
    QTest::addRow("positive: connect() on QObject rvalue")
            << QString("QObject().connect(dummy, &QObject::")
            << QStringList{"aSignal()", "anotherSignal()"};
    QTest::addRow("positive: connect() on QObject pointer rvalue")
            << QString("(new QObject)->connect(dummy, &QObject::")
            << QStringList{"aSignal()", "anotherSignal()"};
    QTest::addRow("positive: disconnect() on QObject")
            << QString("QObject::disconnect(dummy, &QObject::")
            << QStringList{"aSignal()", "anotherSignal()"};
    QTest::addRow("positive: connect() in member function of derived class")
            << QString("}void DerivedFromQObject::alsoNotASignal() { connect(this, &DerivedFromQObject::")
            << QStringList{"aSignal()", "anotherSignal()", "myOwnSignal()"};

    const QStringList allQObjectFunctions{"aSignal()", "anotherSignal()", "notASignal()",
                                          "connect()", "disconnect()", "QObject", "~QObject()",
                                          "operator=(…)"};
    QTest::addRow("negative: different function name")
            << QString("QObject::notASignal(dummy, &QObject::")
            << allQObjectFunctions;
    QTest::addRow("negative: connect function from other class")
            << QString("NotAQObject::connect(dummy, &QObject::")
            << allQObjectFunctions;
    QTest::addRow("negative: first argument")
            << QString("QObject::connect(QObject::")
            << allQObjectFunctions;
    QTest::addRow("negative: third argument")
            << QString("QObject::connect(dummy1, dummy2, QObject::")
            << allQObjectFunctions;

    QTest::addRow("negative: not a QObject")
            << QString("QObject::connect(dummy, &NotAQObject::")
            << QStringList{"notASignal()", "alsoNotASignal()", "connect()", "NotAQObject",
                           "~NotAQObject()", "operator=(…)"};
}

void ClangdTestCompletion::testSignalCompletion()
{
    QFETCH(QString, customCode);
    QFETCH(QStringList, expectedSuggestions);

    ProposalModelPtr proposal;
    getProposal("signalCompletion.cpp", proposal, customCode);

    QVERIFY(proposal);
    QCOMPARE(proposal->size(), expectedSuggestions.size());
    for (const QString &expectedSuggestion : std::as_const(expectedSuggestions))
        QVERIFY2(hasItem(proposal, ' ' + expectedSuggestion), qPrintable(expectedSuggestion));
}

void ClangdTestCompletion::testCompleteAfterProjectChange()
{
    // No defines set, completion must come from #else branch.
    ProposalModelPtr proposal;
    getProposal("completionWithProject.cpp", proposal);

    QVERIFY(proposal);
    QVERIFY(!hasItem(proposal, " projectConfiguration1"));
    QVERIFY(!hasItem(proposal, " projectConfiguration2"));
    QVERIFY(hasItem(proposal, " noProjectConfigurationDetected"));

    // Set define in project file, completion must come from #if branch.
    const auto proFileEditor = qobject_cast<BaseTextEditor *>(
                EditorManager::openEditor(project()->projectFilePath()));
    QVERIFY(proFileEditor);
    proFileEditor->insert("DEFINES += PROJECT_CONFIGURATION_1\n");
    QString saveError;
    QVERIFY2(proFileEditor->document()->save(&saveError), qPrintable(saveError));
    QVERIFY(waitForSignalOrTimeout(project(), &Project::anyParsingFinished, timeOutInMs()));
    QVERIFY(waitForSignalOrTimeout(LanguageClientManager::instance(),
                                   &LanguageClientManager::clientRemoved,
                                   timeOutInMs()));

    // Waiting for the index will cause highlighting info collection to start too late,
    // so skip it.
    waitForNewClient(false);
    QVERIFY(client());

    startCollectingHighlightingInfo();
    proposal.clear();
    getProposal("completionWithProject.cpp", proposal);

    QVERIFY(proposal);
    QVERIFY(hasItem(proposal, " projectConfiguration1"));
    QVERIFY(!hasItem(proposal, " projectConfiguration2"));
    QVERIFY(!hasItem(proposal, " noProjectConfigurationDetected"));
}

void ClangdTestCompletion::startCollectingHighlightingInfo()
{
    m_documentsWithHighlighting.clear();
    connect(client(), &ClangdClient::highlightingResultsReady, this,
            [this](const HighlightingResults &, const FilePath &file) {
        m_documentsWithHighlighting.insert(file);
    });
}

void ClangdTestCompletion::getProposal(const QString &fileName,
                                       TextEditor::ProposalModelPtr &proposalModel,
                                       const QString &insertString,
                                       int *cursorPos)
{
    const TextDocument * const doc = document(fileName);
    QVERIFY(doc);
    const int pos = doc->document()->toPlainText().indexOf(" /* COMPLETE HERE */");
    QVERIFY(pos != -1);
    if (cursorPos)
        *cursorPos = pos;
    int line, column;
    Text::convertPosition(doc->document(), pos, &line, &column);
    const auto editor = qobject_cast<BaseTextEditor *>(
        EditorManager::openEditorAt({doc->filePath(), line, column}));
    QVERIFY(editor);
    QCOMPARE(EditorManager::currentEditor(), editor);
    QCOMPARE(editor->textDocument(), doc);

    if (!insertString.isEmpty()) {
        m_documentsWithHighlighting.remove(doc->filePath());
        editor->insert(insertString);
        if (cursorPos)
            *cursorPos += insertString.length();
    }

    // Once clangd has sent highlighting information for a file, we know it is also
    // ready for completion.
    QElapsedTimer highlightingTimer;
    highlightingTimer.start();
    while (!m_documentsWithHighlighting.contains(doc->filePath())
           && highlightingTimer.elapsed() < timeOutInMs()) {
        waitForSignalOrTimeout(client(), &ClangdClient::highlightingResultsReady, 1000);
    };
    QVERIFY2(m_documentsWithHighlighting.contains(doc->filePath()), qPrintable(fileName));

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    IAssistProposal *proposal = nullptr;
    connect(client(), &ClangdClient::proposalReady, &loop, [&proposal, &loop](IAssistProposal *p) {
        proposal = p;
        loop.quit();
    }, Qt::QueuedConnection);
    editor->editorWidget()->invokeAssist(Completion, nullptr);
    timer.start(5000);
    loop.exec();
    QVERIFY(timer.isActive());
    QVERIFY(proposal);
    proposalModel = proposal->model();
    delete proposal;

    // The "dot" test files are only used once.
    if (!insertString.isEmpty() && insertString != ".") {
        m_documentsWithHighlighting.remove(doc->filePath());
        editor->editorWidget()->undo();
    }
}

bool ClangdTestCompletion::hasItem(TextEditor::ProposalModelPtr model, const QString &text,
                                   const QString &detail)
{
    for (int i = 0; i < model->size(); ++i) {
        if (model->text(i) == text) {
            if (detail.isEmpty())
                return true;
            const auto genericModel = model.dynamicCast<GenericProposalModel>();
            return genericModel && genericModel->detail(i) == detail;
        }
    }
    return false;
}

bool ClangdTestCompletion::hasSnippet(TextEditor::ProposalModelPtr model, const QString &text)
{
    const auto genericModel = model.dynamicCast<GenericProposalModel>();
    if (!genericModel)
        return false;
    for (int i = 0, size = genericModel->size(); i < size; ++i) {
        const TextEditor::AssistProposalItemInterface * const item = genericModel->proposalItem(i);
        if (item->text() == text)
            return item->isSnippet();
    }
    return false;
}

AssistProposalItemInterface *ClangdTestCompletion::getItem(
        TextEditor::ProposalModelPtr model, const QString &text, const QString &detail)
{
    const auto genericModel = model.dynamicCast<GenericProposalModel>();
    if (!genericModel)
        return nullptr;
    for (int i = 0; i < genericModel->size(); ++i) {
        if (genericModel->text(i) == text &&
                (detail.isEmpty() || detail == genericModel->detail(i))) {
            return genericModel->proposalItem(i);
        }
    }
    return nullptr;
}


ClangdTestExternalChanges::ClangdTestExternalChanges()
{
    setProjectFileName("completion.pro");
    setSourceFileNames({"mainwindow.cpp", "main.cpp"});
}

void ClangdTestExternalChanges::test()
{
    ClangdClient * const oldClient = client();
    QVERIFY(oldClient);

    // Wait until things have settled.
    while (true) {
        if (!waitForSignalOrTimeout(oldClient, &ClangdClient::configChanged, timeOutInMs()))
            break;
    }

    // Break a header file that is used, but not open in Creator.
    // Neither we nor the server should notice, and no diagnostics should be shown for the
    // source file that includes the now-broken header.
    QFile header(project()->projectDirectory().toString() + "/mainwindow.h");
    QVERIFY(header.open(QIODevice::WriteOnly));
    header.write("blubb");
    header.close();
    waitForSignalOrTimeout(LanguageClientManager::instance(),
                           &LanguageClientManager::clientAdded, timeOutInMs());
    QCOMPARE(client(), oldClient);
    QCOMPARE(client(), ClangModelManagerSupport::clientForProject(project()));
    const TextDocument * const curDoc = document("main.cpp");
    QVERIFY(curDoc);
    if (!curDoc->marks().isEmpty())
        for (const auto &m : curDoc->marks())
            qDebug() << m->lineAnnotation();
    QVERIFY(curDoc->marks().isEmpty());

    // Now trigger an external change in an open, but not currently visible file and
    // verify that we get diagnostics in the current editor.
    TextDocument * const docToChange = document("mainwindow.cpp");
    docToChange->setSilentReload();
    QFile otherSource(filePath("mainwindow.cpp").toString());
    QVERIFY(otherSource.open(QIODevice::WriteOnly));
    otherSource.write("blubb");
    otherSource.close();
    if (curDoc->marks().isEmpty())
        QVERIFY(waitForSignalOrTimeout(client(), &ClangdClient::textMarkCreated, timeOutInMs()));
}

ClangdTestIndirectChanges::ClangdTestIndirectChanges()
{
    setProjectFileName("indirect-changes.pro");
    setSourceFileNames({"main.cpp", "directheader.h", "indirectheader.h", "unrelatedheader.h"});
}

void ClangdTestIndirectChanges::test()
{
    // Initially, everything is fine.
    const TextDocument * const src = document("main.cpp");
    QVERIFY(src);
    QVERIFY(src->marks().isEmpty());

    // Write into an indirectly included header file. Our source file should have diagnostics now.
    const TextDocument * const indirectHeader = document("indirectheader.h");
    QVERIFY(indirectHeader);
    QTextCursor cursor(indirectHeader->document());
    cursor.insertText("blubb");
    while (src->marks().isEmpty())
        QVERIFY(waitForSignalOrTimeout(client(), &ClangdClient::textMarkCreated, timeOutInMs()));

    // Remove the inserted text again; the diagnostics should disappear.
    cursor.document()->undo();
    QVERIFY(cursor.document()->toPlainText().isEmpty());
    while (!src->marks().isEmpty()) {
        QVERIFY(waitForSignalOrTimeout(client(), &ClangdClient::highlightingResultsReady,
                                       timeOutInMs()));
    }

    // Now write into a header file that is not included anywhere.
    // We expect diagnostics only for the header itself.
    const TextDocument * const unrelatedHeader = document("unrelatedheader.h");
    QVERIFY(indirectHeader);
    QTextCursor cursor2(unrelatedHeader->document());
    cursor2.insertText("blubb");
    while (waitForSignalOrTimeout(client(), &ClangdClient::textMarkCreated, timeOutInMs()))
        ;
    QVERIFY(!unrelatedHeader->marks().isEmpty());
    QVERIFY(src->marks().isEmpty());
}

} // namespace Tests
} // namespace Internal
} // namespace ClangCodeModel
