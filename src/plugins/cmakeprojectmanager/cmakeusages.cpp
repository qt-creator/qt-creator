// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeusages.h"

#include "cmakebuildsystem.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakeutils.h"
#include "fileapidataextractor.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>

#include <projectexplorer/buildsystem.h>

#include <texteditor/basefilefind.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/searchresultitem.h>
#include <utils/textutils.h>

#include <QTextBlock>

#ifdef WITH_TESTS
#include <QTest>
#include <QTextDocument>
#endif

using namespace CMakeLang;
using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace CMakeProjectManager::Internal {

// A place a file names the symbol.
class Occurrence
{
public:
    FilePath filePath;
    QString lineText;
    Text::Position position;
    int length = 0;
};

static QString lineTextAt(QStringView source, int position)
{
    int start = position;
    while (start > 0 && source.at(start - 1) != u'\n')
        --start;

    int end = position;
    while (end < source.size() && source.at(end) != u'\n')
        ++end;
    if (end > start && source.at(end - 1) == u'\r')
        --end;

    return source.mid(start, end - start).toString();
}

static Occurrence occurrenceAt(const FilePath &filePath,
                               QStringView source,
                               const Text::Position &position,
                               int offset,
                               int length)
{
    return {filePath, lineTextAt(source, offset), position, length};
}

// Whether what is at this offset is written inside a ${}, $ENV{} or $<>
// expansion, which names what it expands wherever it is written.
static bool isInsideExpansion(QStringView spelling, qsizetype at)
{
    int depth = 0;
    for (qsizetype i = 0; i < at; ++i) {
        const QChar character = spelling.at(i);
        if (character == u'{' || character == u'<') {
            qsizetype start = i;
            while (start > 0 && isCMakeIdentifierChar(spelling.at(start - 1)))
                --start;
            if (start > 0 && spelling.at(start - 1) == u'$')
                ++depth;
        } else if ((character == u'}' || character == u'>') && depth > 0) {
            --depth;
        }
    }
    return depth > 0;
}

// Wherever the token spells out the name on its own: an argument that is the
// name, quoted or not, and a name an expansion in it names.
static void appendTokenOccurrences(const FilePath &filePath,
                                   QStringView source,
                                   const Token &token,
                                   const QString &name,
                                   bool quoted,
                                   QList<Occurrence> *occurrences)
{
    const QStringView spelling = token.spelling;
    // What the argument writes, the quotes that delimit a quoted one aside.
    const qsizetype contentStart = quoted ? 1 : 0;
    const qsizetype contentSize = quoted ? spelling.size() - 2 : spelling.size();
    for (qsizetype at = spelling.indexOf(name); at >= 0; at = spelling.indexOf(name, at + 1)) {
        if (at > 0 && isCMakeIdentifierChar(spelling.at(at - 1)))
            continue;
        const qsizetype behind = at + name.size();
        if (behind < spelling.size() && isCMakeIdentifierChar(spelling.at(behind)))
            continue;

        // A name the argument carries other text along with -- a file name
        // that has it as its stem, a path it is a directory of -- is a name
        // of its own. Only an expansion names the symbol wherever it is
        // written.
        if ((at != contentStart || name.size() != contentSize)
            && !isInsideExpansion(spelling, at)) {
            continue;
        }

        // A quoted argument can span lines, and the name can sit on any of
        // them.
        int line = token.line;
        qsizetype lineStart = -1;
        for (qsizetype i = 0; i < at; ++i) {
            if (spelling.at(i) == u'\n') {
                ++line;
                lineStart = i + 1;
            }
        }
        const int column = lineStart < 0 ? token.column - 1 + int(at) : int(at - lineStart);

        occurrences->append(occurrenceAt(filePath,
                                         source,
                                         {line, column},
                                         token.position + int(at),
                                         name.size()));
    }
}

static void appendArgumentOccurrences(const FilePath &filePath,
                                      QStringView source,
                                      ArgumentAST *argument,
                                      const QString &name,
                                      QList<Occurrence> *occurrences)
{
    if (ParenGroupArgumentAST *group = argument->asParenGroupArgument()) {
        for (ArgumentAST *nested : group->arguments())
            appendArgumentOccurrences(filePath, source, nested, name, occurrences);
        return;
    }
    // A bracket argument expands nothing, so what is written in it is text.
    if (argument->asBracketArgument())
        return;

    appendTokenOccurrences(filePath,
                           source,
                           argument->token,
                           name,
                           argument->asQuotedArgument() != nullptr,
                           occurrences);
}

static QList<Occurrence> occurrencesIn(const FilePath &filePath,
                                       const DocumentPtr &document,
                                       const QString &name)
{
    QList<Occurrence> occurrences;
    const QStringView source = document->source();

    for (CommandAST *command : document->commands()) {
        // A call names the command whatever case it spells it in.
        if (command->isNamed(name)) {
            const Token &token = command->name;
            occurrences.append(occurrenceAt(filePath,
                                            source,
                                            {token.line, token.column - 1},
                                            token.position,
                                            token.length));
        }
        for (ArgumentAST *argument : command->arguments())
            appendArgumentOccurrences(filePath, source, argument, name, &occurrences);
    }
    return occurrences;
}

// Whether the file itself names the symbol as its own: a function or macro it
// defines, a variable it sets, an option it declares.
static bool definesSymbol(const DocumentPtr &document, const QString &name)
{
    for (CommandAST *command : document->commands()) {
        if (!command->isNamed("function") && !command->isNamed("macro")
            && !command->isNamed("set") && !command->isNamed("option")) {
            continue;
        }
        ArgumentAST *argument = command->arguments().first();
        if (argument && argument->value() == name)
            return true;
    }
    return false;
}

// Whether the project defines the symbol itself, which is what makes renaming
// it the project's business rather than one file's. What a Qt or CMake module
// brings along is not the project's to rename.
static bool definedInProject(CMakeBuildSystem *buildSystem, const QString &name)
{
    const QHash<QString, Link> &symbols = buildSystem->cmakeSymbolsHash();
    const auto it = symbols.constFind(name);
    return it != symbols.constEnd()
           && it->targetFilePath.isChildOf(buildSystem->projectDirectory());
}

// The name the cursor is on, delimited the way the CMake editor delimits one
// when following it, so that what F2 resolves is what is looked up here.
static QString nameUnderCursor(const QTextCursor &cursor)
{
    const QString text = cursor.block().text();
    const int position = cursor.positionInBlock();

    int start = position;
    while (start > 0 && isCMakeIdentifierChar(text.at(start - 1)))
        --start;
    int end = position;
    while (end < text.size() && isCMakeIdentifierChar(text.at(end)))
        ++end;

    return text.mid(start, end - start);
}

// The file being edited is read from the editor, so that what is offered
// matches what is on screen; the other files come from what the project
// parsed.
static QList<Occurrence> occurrencesUnderCursor(TextEditorWidget *editorWidget, QString *name)
{
    *name = nameUnderCursor(editorWidget->textCursor());
    if (name->isEmpty())
        return {};

    const FilePath currentFile = editorWidget->textDocument()->filePath();
    const DocumentPtr currentDocument = Document::fromSource(
        editorWidget->textDocument()->plainText());
    if (!currentDocument->isValid())
        return {};

    QList<Occurrence> occurrences = occurrencesIn(currentFile, currentDocument, *name);

    auto buildSystem = qobject_cast<CMakeBuildSystem *>(
        ProjectExplorer::activeBuildSystemForCurrentProject());
    if (!buildSystem || !definedInProject(buildSystem, *name))
        return definesSymbol(currentDocument, *name) ? occurrences : QList<Occurrence>();

    const FilePath projectDirectory = buildSystem->projectDirectory();
    for (const CMakeFileInfo &cmakeFile : buildSystem->cmakeFiles()) {
        if (cmakeFile.path == currentFile || cmakeFile.isExternal || cmakeFile.isGenerated)
            continue;
        if (!cmakeFile.document || !cmakeFile.document->isValid()
            || !cmakeFile.path.isChildOf(projectDirectory)) {
            continue;
        }
        occurrences += occurrencesIn(cmakeFile.path, cmakeFile.document, *name);
    }
    return occurrences;
}

static void showOccurrences(const QString &name,
                            const QList<Occurrence> &occurrences,
                            SearchResultWindow::SearchMode mode)
{
    SearchResult *search = SearchResultWindow::instance()->startNewSearch(
        Tr::tr("CMake Usages:"), {}, name, mode, SearchResultWindow::PreserveCaseDisabled);

    if (mode == SearchResultWindow::SearchAndReplace) {
        search->setTextToReplace(name);
        QObject::connect(search,
                         &SearchResult::replaceButtonClicked,
                         search,
                         [](const QString &text,
                            const SearchResultItems &items,
                            bool preserveCase) {
                             BaseFileFind::replaceAll(text, items, preserveCase);
                             SearchResultWindow::instance()->hide();
                         });
    }
    QObject::connect(search, &SearchResult::activated, search, [](const SearchResultItem &item) {
        EditorManager::openEditorAtSearchResult(item);
    });

    SearchResultItems items;
    for (const Occurrence &occurrence : occurrences) {
        SearchResultItem item;
        item.setFilePath(occurrence.filePath);
        item.setLineText(occurrence.lineText);
        item.setMainRange(occurrence.position.line, occurrence.position.column, occurrence.length);
        item.setUseTextEditorFont(true);
        items.append(item);
    }
    search->addResults(items, SearchResult::AddSortedByPosition);
    search->finishSearch(false);

    SearchResultWindow::instance()->popup(
        IOutputPane::Flags(IOutputPane::ModeSwitch | IOutputPane::WithFocus));
}

static void reportNoSymbol()
{
    EditorManager::showEditorStatusBar(Constants::CMAKE_EDITOR_ID,
                                       Tr::tr("No symbol of this project under the cursor."));
}

void findUsagesUnderCursor(TextEditorWidget *editorWidget)
{
    QString name;
    const QList<Occurrence> occurrences = occurrencesUnderCursor(editorWidget, &name);
    if (occurrences.isEmpty()) {
        reportNoSymbol();
        return;
    }
    showOccurrences(name, occurrences, SearchResultWindow::SearchOnly);
}

void renameSymbolUnderCursor(TextEditorWidget *editorWidget)
{
    QString name;
    const QList<Occurrence> occurrences = occurrencesUnderCursor(editorWidget, &name);
    if (occurrences.isEmpty()) {
        reportNoSymbol();
        return;
    }
    showOccurrences(name, occurrences, SearchResultWindow::SearchAndReplace);
}

#ifdef WITH_TESTS

static QString dumpOccurrences(const QString &source, const QString &name)
{
    const DocumentPtr document = Document::fromSource(source);
    if (!document->isValid())
        return "<error>";

    QStringList dumped;
    for (const Occurrence &occurrence :
         occurrencesIn(FilePath::fromString("CMakeLists.txt"), document, name)) {
        dumped << QString("%1:%2+%3")
                      .arg(occurrence.position.line)
                      .arg(occurrence.position.column)
                      .arg(occurrence.length);
    }
    return dumped.join(u' ');
}

class CMakeUsagesTest final : public QObject
{
    Q_OBJECT

private slots:
    void testOccurrences_data()
    {
        QTest::addColumn<QString>("source");
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("expected");

        // The definition names it, and so does every call, whatever case the
        // call spells the command in.
        QTest::newRow("definition and calls") << "function(my_func)\n"
                                                 "endfunction()\n"
                                                 "my_func(a)\n"
                                                 "MY_FUNC(b)\n"
                                              << "my_func" << "1:9+7 3:0+7 4:0+7";

        // A variable is named where it is set and wherever it is expanded,
        // quotes and a path around it included.
        QTest::newRow("variable expansions") << "set(SOURCES a.cpp)\n"
                                                "target_sources(app PRIVATE ${SOURCES})\n"
                                                "add_library(lib \"${SOURCES}/x\")\n"
                                             << "SOURCES" << "1:4+7 2:29+7 3:19+7";

        // A longer name that has the one looked for inside it is not it.
        QTest::newRow("whole words only") << "set(SOURCES a.cpp)\n"
                                             "message(${SOURCESX})\n"
                                             "set(MY_SOURCES b.cpp)\n"
                                          << "SOURCES" << "1:4+7";

        QTest::newRow("target and lookalikes") << "add_executable(app main.cpp)\n"
                                                  "add_executable(myapp x.cpp)\n"
                                                  "set(app_extra 1)\n"
                                               << "app" << "1:15+3";

        // A source file that carries the name as its stem is a name of its
        // own, not a use of the name.
        QTest::newRow("file stem") << "add_library(mylib mylib.cpp)\n"
                                      "target_sources(mylib PRIVATE mylib_extra.cpp)\n"
                                   << "mylib" << "1:12+5 2:15+5";

        // Quotes around the text change nothing about that: a file name and
        // a path spell out a name of their own there as well.
        QTest::newRow("quoted file stem") << "add_library(mylib mylib.cpp)\n"
                                             "target_sources(mylib PRIVATE \"mylib.cpp\")\n"
                                             "install(FILES \"docs/mylib/readme.md\")\n"
                                          << "mylib" << "1:12+5 2:15+5";

        // A quoted argument that is the name is the name.
        QTest::newRow("quoted name")
            << "add_library(mylib a.cpp)\n"
               "set_target_properties(mylib PROPERTIES OUTPUT_NAME \"mylib\")\n"
            << "mylib" << "1:12+5 2:22+5 2:52+5";

        // What an expansion expands is named wherever the expansion is
        // written, whatever the argument carries around it.
        QTest::newRow("reference in a path") << "include(${DIR}/x.cmake)\n"
                                                "add_subdirectory($ENV{DIR})\n"
                                             << "DIR" << "1:10+3 2:22+3";

        // A generator expression names the target it asks about, and it names
        // it the same whether or not the argument is quoted.
        QTest::newRow("generator expression")
            << "add_custom_command(COMMAND $<TARGET_FILE:mytool>)\n"
               "add_custom_command(COMMAND \"$<TARGET_FILE:mytool>\")\n"
            << "mytool" << "1:41+6 2:42+6";

        // A bracket argument expands nothing, so what it carries is text.
        QTest::newRow("bracket argument") << "set(FOO 1)\n"
                                             "message([[${FOO}]])\n"
                                          << "FOO" << "1:4+3";

        // A name can carry a hyphen, which a target name commonly does, and a
        // longer name that has it inside is still not it.
        QTest::newRow("hyphenated name") << "add_library(my-lib main.cpp)\n"
                                            "target_link_libraries(app PRIVATE my-lib)\n"
                                            "message(\"my-lib-extra\")\n"
                                         << "my-lib" << "1:12+6 2:34+6";

        // A condition can nest its arguments in parentheses.
        QTest::newRow("nested condition") << "if (NOT (FOO OR BAR))\n"
                                             "endif()\n"
                                          << "FOO" << "1:9+3";

        // A quoted argument can span lines, and the name can sit on any of
        // them.
        QTest::newRow("across lines") << "set(X \"a\n"
                                         "${FOO} b\")\n"
                                      << "FOO" << "2:2+3";
    }

    void testOccurrences()
    {
        QFETCH(QString, source);
        QFETCH(QString, name);
        QFETCH(QString, expected);

        QCOMPARE(dumpOccurrences(source, name), expected);
    }

    void testNameUnderCursor_data()
    {
        QTest::addColumn<int>("position");
        QTest::addColumn<QString>("expected");

        // A hyphenated name is one name wherever in it the cursor sits, the
        // way following it with F2 reads it.
        QTest::newRow("at the start") << 12 << "my-lib";
        QTest::newRow("before the hyphen") << 14 << "my-lib";
        QTest::newRow("after the hyphen") << 15 << "my-lib";
        QTest::newRow("at the end") << 18 << "my-lib";

        QTest::newRow("the command") << 3 << "add_library";
        QTest::newRow("the next argument") << 20 << "main";
    }

    void testNameUnderCursor()
    {
        QFETCH(int, position);
        QFETCH(QString, expected);

        QTextDocument document("add_library(my-lib main.cpp)");
        QTextCursor cursor(&document);
        cursor.setPosition(position);

        QCOMPARE(nameUnderCursor(cursor), expected);
    }
};

QObject *createCMakeUsagesTest()
{
    return new CMakeUsagesTest;
}

#endif // WITH_TESTS

} // CMakeProjectManager::Internal

#ifdef WITH_TESTS
#include "cmakeusages.moc"
#endif
