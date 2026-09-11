// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakesemantichighlighter.h"

#include "cmakecommandkeywords.h"
#include "cmaketoolmanager.h"

#include <cmakelang/cmakelexer.h>
#include <cmakelang/cmakeparser.h>

#include <projectexplorer/projectmanager.h>

#include <texteditor/fontsettings.h>
#include <texteditor/semantichighlighter.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>

#include <utils/async.h>
#include <utils/futuresynchronizer.h>

#include <QFutureWatcher>
#include <QTimer>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace TextEditor;

namespace CMakeProjectManager::Internal {

// What this highlighter has to say about a word.
enum Kind { KeywordArgument = 1 };

// Reading the file over again for every character that is written would be
// work done for nothing, so the highlighting follows a moment behind.
const int Delay = 200;

// Whether the command takes the argument as one of its keywords.
using IsKeyword = std::function<bool(const QString &command, const QString &argument)>;

// The words of the calls of the source that are keywords of the command they
// stand in.
//
// This is read off the tokens rather than off the syntax tree: a file that is
// being written has a call in it that is not closed yet, which is no command
// of a tree, and the keywords already written in it are to be coloured all
// the same.
static HighlightingResults keywordHighlights(const QString &source, const IsKeyword &isKeyword)
{
    CMakeLang::Engine engine;
    CMakeLang::Lexer lexer(&engine, engine.setSource(source));

    HighlightingResults results;
    QString command;
    int depth = 0;
    CMakeLang::Token previous;
    CMakeLang::Token token;

    while (lexer.yylex(&token) != CMakeLang::Parser::EOF_SYMBOL) {
        switch (token.kind) {
        case CMakeLang::Parser::T_LEFT_PAREN:
            // The name of the command stands in front of the parenthesis
            // that opens its arguments, and a space may stand in between.
            if (depth++ == 0 && previous.is(CMakeLang::Parser::T_IDENTIFIER))
                command = previous.text();
            break;

        case CMakeLang::Parser::T_RIGHT_PAREN:
            if (--depth <= 0) {
                depth = 0;
                command.clear();
            }
            break;

        case CMakeLang::Parser::T_IDENTIFIER:
        case CMakeLang::Parser::T_UNQUOTED_ARGUMENT:
            // A keyword stands the way it is written; what is quoted is a
            // value, whatever it says.
            if (depth > 0 && !command.isEmpty() && isKeyword(command, token.text()))
                results.append({token.line, token.column, token.length, KeywordArgument});
            break;

        default:
            break;
        }

        if (token.isNot(CMakeLang::Parser::T_SPACE)
            && token.isNot(CMakeLang::Parser::T_NEWLINE)) {
            previous = token;
        }
    }
    return results;
}

class SemanticHighlighter: public QObject
{
public:
    explicit SemanticHighlighter(TextDocument *document)
        : QObject(document)
        , m_document(document)
    {
        m_timer.setSingleShot(true);
        m_timer.setInterval(Delay);
        connect(&m_timer, &QTimer::timeout, this, [this] { highlight(); });

        connect(document, &TextDocument::contentsChanged, this, [this] { m_timer.start(); });
        connect(document, &TextDocument::fontSettingsChanged, this, [this] { highlight(); });

        // The keywords of the commands the project defines itself are known
        // once it has been configured.
        connect(ProjectExplorer::ProjectManager::instance(),
                &ProjectExplorer::ProjectManager::parsingFinishedCurrent,
                this,
                [this] { m_timer.start(); });

        // The ones CMake itself brings are read in a thread of its own, and
        // until that is done there is nothing of them to colour.
        connect(CMakeToolManager::instance(),
                &CMakeToolManager::keywordsRead,
                this,
                [this] { m_timer.start(); });

        connect(&m_watcher, &QFutureWatcherBase::finished, this, [this] { apply(); });

        highlight();
    }

private:
    void highlight();
    void apply();

    TextDocument *m_document;
    CommandKeywords m_keywords;
    QTimer m_timer;
    QFutureWatcher<HighlightingResults> m_watcher;
    Utils::FutureSynchronizer m_synchronizer;
    int m_revision = 0;
};

// Reading the file is the work of a thread of its own: the GUI thread hands
// over a copy of the source and of what it knows about the commands, and
// takes the results back in apply().
void SemanticHighlighter::highlight()
{
    if (!m_document->syntaxHighlighter())
        return;

    m_keywords.refresh();

    m_watcher.cancel();
    m_revision = m_document->document()->revision();

    const QFuture<HighlightingResults> results = Utils::asyncRun(
        QThread::LowestPriority,
        [source = m_document->plainText(), keywords = m_keywords]() mutable {
            return keywordHighlights(source,
                                     [&keywords](const QString &command, const QString &argument) {
                                         return keywords.contains(command, argument);
                                     });
        });
    m_watcher.setFuture(results);
    m_synchronizer.addFuture(results);
}

void SemanticHighlighter::apply()
{
    // What was read of a source that has been written in since says nothing
    // about the one that stands there now.
    if (m_watcher.isCanceled() || m_revision != m_document->document()->revision())
        return;

    SyntaxHighlighter *highlighter = m_document->syntaxHighlighter();
    if (!highlighter)
        return;

    // The colour is the one the theme gives an enumerator, which is what a
    // keyword of a call is: one of the words the command knows.
    const QTextCharFormat format = globalFontSettings().data().toTextCharFormat(C_ENUMERATION);
    TextEditor::SemanticHighlighter::setExtraAdditionalFormats(highlighter,
                                                               m_watcher.result(),
                                                               {{KeywordArgument, format}});
}

void setupCMakeSemanticHighlighter(TextDocument *document)
{
    new SemanticHighlighter(document);
}

#ifdef WITH_TESTS

class CMakeSemanticHighlighterTest final : public QObject
{
    Q_OBJECT

private slots:
    // Only the keywords of the command a word stands in are keywords.
    void testKeywordsOfACall()
    {
        const QString source = "add_qtc_plugin(CMakeProjectManager\n"
                               "  PLUGIN_CLASS CMakeProjectPlugin\n"
                               "  SOURCES SOURCES.cpp \"SOURCES\"\n"
                               ")\n"
                               "set(SOURCES foo.cpp)\n";

        const HighlightingResults results
            = keywordHighlights(source, [](const QString &command, const QString &argument) {
                  return command == "add_qtc_plugin"
                         && (argument == "PLUGIN_CLASS" || argument == "SOURCES");
              });

        // The keyword of the second line and the one of the third, and
        // neither the value that reads like one, nor the quoted word, nor
        // the argument of the command that does not take it.
        QCOMPARE(results.size(), 2);

        QCOMPARE(results.at(0).line, 2);
        QCOMPARE(results.at(0).column, 3);
        QCOMPARE(results.at(0).length, 12);
        QCOMPARE(results.at(0).kind, int(KeywordArgument));

        QCOMPARE(results.at(1).line, 3);
        QCOMPARE(results.at(1).column, 3);
        QCOMPARE(results.at(1).length, 7);
    }

    // A file no command of which knows the words is left alone.
    void testWithoutKeywords()
    {
        const HighlightingResults results
            = keywordHighlights("project(Foo)\nadd_subdirectory(bar)\n",
                                [](const QString &, const QString &) { return false; });
        QCOMPARE(results.size(), 0);
    }

    // A file that is being written does not parse, and what stands in it so
    // far is read all the same.
    void testHalfWrittenFile()
    {
        const HighlightingResults results
            = keywordHighlights("add_qtc_plugin(Foo\n  SOURCES\n",
                                [](const QString &, const QString &argument) {
                                    return argument == "SOURCES";
                                });
        QCOMPARE(results.size(), 1);
        QCOMPARE(results.at(0).line, 2);
    }
};

QObject *createCMakeSemanticHighlighterTest()
{
    return new CMakeSemanticHighlighterTest;
}

#endif // WITH_TESTS

} // namespace CMakeProjectManager::Internal

#ifdef WITH_TESTS
#include "cmakesemantichighlighter.moc"
#endif
