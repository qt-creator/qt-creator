// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <cmakelang/cmakeast.h>
#include <cmakelang/cmakedoc.h>
#include <cmakelang/cmakeastvisitor.h>
#include <cmakelang/cmakedocument.h>
#include <cmakelang/cmakeformatter.h>
#include <cmakelang/cmakeindentation.h>
#include <cmakelang/cmakelexer.h>
#include <cmakelang/cmakeparser.h>
#include <cmakelang/cmakerewriter.h>
#include <cmakelang/cmakesignature.h>

#include <utils/algorithm.h>

#include <QDir>
#include <QSet>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QFile>
#include <QTest>

#include <functional>
#include <optional>

using namespace CMakeLang;

static QString lex(const QString &source)
{
    Engine engine;
    Lexer lexer(&engine, engine.setSource(source));
    QStringList result;
    Token token;
    while (lexer.yylex(&token) != Parser::EOF_SYMBOL) {
        result << QString("%1 %2 %3:%4")
                      .arg(QLatin1String(Lexer::name(token.kind)),
                           token.text(),
                           QString::number(token.line),
                           QString::number(token.column));
    }
    return result.join(u'\n');
}

namespace {

class Dumper: public Visitor
{
public:
    QString result;

    bool visit(CommandAST *ast) override
    {
        separate();
        result += "(cmd " + ast->commandName().toLower();
        for (ArgumentAST *argument : ast->arguments())
            result += ' ' + dumpArgument(argument);
        result += ')';
        return false;
    }

    bool visit(IfAST *) override { return open("if"); }
    void endVisit(IfAST *) override { close(); }
    bool visit(ElseIfClauseAST *) override { return open("elseif"); }
    void endVisit(ElseIfClauseAST *) override { close(); }
    bool visit(ElseClauseAST *) override { return open("else"); }
    void endVisit(ElseClauseAST *) override { close(); }
    bool visit(ForEachAST *) override { return open("foreach"); }
    void endVisit(ForEachAST *) override { close(); }
    bool visit(WhileAST *) override { return open("while"); }
    void endVisit(WhileAST *) override { close(); }
    bool visit(FunctionAST *) override { return open("function"); }
    void endVisit(FunctionAST *) override { close(); }
    bool visit(MacroAST *) override { return open("macro"); }
    void endVisit(MacroAST *) override { close(); }
    bool visit(BlockAST *) override { return open("block"); }
    void endVisit(BlockAST *) override { close(); }

private:
    bool open(const QString &name)
    {
        separate();
        result += '(' + name;
        return true;
    }

    void close() { result += ')'; }

    void separate()
    {
        if (!result.isEmpty())
            result += ' ';
    }

    static QString dumpArgument(ArgumentAST *argument)
    {
        if (ParenGroupArgumentAST *group = argument->asParenGroupArgument()) {
            QString inner;
            for (ArgumentAST *nested : group->arguments())
                inner += (inner.isEmpty() ? QString() : QString(u' ')) + dumpArgument(nested);
            return '(' + inner + ')';
        }
        if (argument->asQuotedArgument())
            return '"' + argument->value() + '"';
        if (argument->asBracketArgument())
            return '[' + argument->value() + ']';
        return argument->value();
    }

};

class SpanDumper: public Visitor
{
public:
    explicit SpanDumper(QStringView source)
        : m_source(source)
    {}

    QString result;

    bool visit(SourceFileAST *ast) override { return open("file", ast); }
    void endVisit(SourceFileAST *) override { close(); }
    bool visit(IfAST *ast) override { return open("if", ast); }
    void endVisit(IfAST *) override { close(); }
    bool visit(ElseIfClauseAST *ast) override { return open("elseif", ast); }
    void endVisit(ElseIfClauseAST *) override { close(); }
    bool visit(ElseClauseAST *ast) override { return open("else", ast); }
    void endVisit(ElseClauseAST *) override { close(); }
    bool visit(ForEachAST *ast) override { return open("foreach", ast); }
    void endVisit(ForEachAST *) override { close(); }
    bool visit(CommandAST *) override { return false; }

private:
    bool open(const char *name, AST *ast)
    {
        if (!result.isEmpty())
            result += u' ';
        result += u'(';
        result += QLatin1StringView(name);
        result += u' ';
        result += m_source.mid(ast->position, ast->length).toString().replace(u'\n', u'~');
        return true;
    }

    void close() { result += ')'; }

    QStringView m_source;
};

} // namespace

static QString parse(const QString &source, QString *error = nullptr)
{
    Engine engine;
    Parser parser(&engine, source);
    SourceFileAST *ast = parser.parse();
    if (error) {
        for (const Diagnostic &d : engine.diagnostics())
            *error += d.message;
    }

    QStringList elements;
    for (ElementAST *element : ast->elements()) {
        Dumper one;
        one.accept(element);
        elements << one.result;
    }
    return elements.join(u' ');
}

static QString errorsOf(const QString &source)
{
    QString error;
    parse(source, &error);
    return error;
}

static QString dumpSpans(const QString &source)
{
    Engine engine;
    Parser parser(&engine, source);
    SourceFileAST *ast = parser.parse();
    SpanDumper dumper(source);
    dumper.accept(ast);
    return dumper.result;
}

class tst_CMakeLang: public QObject
{
    Q_OBJECT

private slots:
    void lexer_data();
    void lexer();
    void parser_data();
    void parser();
    void blockBalancing_data();
    void blockBalancing();
    void keywordsAreNotReserved();
    void bracketCommentNeedsNewline();
    void nodeSpans();
    void unterminatedConstructs();
    void errorRecovery();
    void documentCommands();
    void documentScopes();
    void signatures_data();
    void signatures();
    void signatureKeywords();
    void signaturesNeedTheSource();
    void signaturesForwarded();
    void argumentGroups_data();
    void argumentGroups();
    void rewriterReplacesValues();
    void rewriterRemovesArguments();
    void rewriterInsertsValues();
    void indentation_data();
    void indentation();
    void indentationOfHalfWrittenFiles_data();
    void indentationOfHalfWrittenFiles();
    void indentationKeepsMultilineValues();
    void formatting_data();
    void formatting();
    void formattingLeavesFormattedFilesAlone_data();
    void formattingLeavesFormattedFilesAlone();
    void formattingEditsAreMinimal();
    void styleSwitches_data();
    void styleSwitches();
    void formattingKeepsWhatItIsGiven();
    void documentationComments();
    void documentedCommands();
    void documentationOfModules();
    void documentationOfIncludedModules();
    void documentationOfCMakeModules();
    void documentationOfCMakeHelp();
    void documentationIgnoresCase();
    void documentationTellsExamplesApart();
    void documentationOfSeveralArguments();
    void argumentsOfSeveralCommands();
};

void tst_CMakeLang::lexer_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expected");

    QTest::newRow("command") << "set(a 1)\n"
                             << "identifier set 1:1\n"
                                "left paren ( 1:4\n"
                                "identifier a 1:5\n"
                                "space   1:6\n"
                                "unquoted argument 1 1:7\n"
                                "right paren ) 1:8\n"
                                "newline \n 1:9";

    // A bracket argument reports the position of its opening bracket, drops a
    // newline directly after it and keeps everything up to the matching close.
    QTest::newRow("bracket") << "set(a [==[x]=]y]==])\n"
                             << "identifier set 1:1\n"
                                "left paren ( 1:4\n"
                                "identifier a 1:5\n"
                                "space   1:6\n"
                                "bracket argument x]=]y 1:7\n"
                                "right paren ) 1:20\n"
                                "newline \n 1:21";

    QTest::newRow("bracket eats first newline") << "set(a [[\nx]])\n"
                                                << "identifier set 1:1\n"
                                                   "left paren ( 1:4\n"
                                                   "identifier a 1:5\n"
                                                   "space   1:6\n"
                                                   "bracket argument x 1:7\n"
                                                   "right paren ) 2:4\n"
                                                   "newline \n 2:5";

    // Escapes stay in the token text, a backslash-newline continuation does not.
    QTest::newRow("quoted escapes") << "set(a \"x\\\"y\")\n"
                                    << "identifier set 1:1\n"
                                       "left paren ( 1:4\n"
                                       "identifier a 1:5\n"
                                       "space   1:6\n"
                                       "quoted argument x\\\"y 1:7\n"
                                       "right paren ) 1:13\n"
                                       "newline \n 1:14";

    QTest::newRow("quoted continuation") << "set(a \"x\\\ny\")\n"
                                         << "identifier set 1:1\n"
                                            "left paren ( 1:4\n"
                                            "identifier a 1:5\n"
                                            "space   1:6\n"
                                            "quoted argument xy 1:7\n"
                                            "right paren ) 2:3\n"
                                            "newline \n 2:4";

    // The legacy rule swallows quotes and $(MAKEVAR) inside an unquoted
    // argument; the plain unquoted rule would stop at the quote.
    QTest::newRow("legacy quoted") << "set(a -DX=\"b c\")\n"
                                   << "identifier set 1:1\n"
                                      "left paren ( 1:4\n"
                                      "identifier a 1:5\n"
                                      "space   1:6\n"
                                      "unquoted argument -DX=\"b c\" 1:7\n"
                                      "right paren ) 1:16\n"
                                      "newline \n 1:17";

    QTest::newRow("legacy makevar") << "set(a x$(V)y)\n"
                                    << "identifier set 1:1\n"
                                       "left paren ( 1:4\n"
                                       "identifier a 1:5\n"
                                       "space   1:6\n"
                                       "unquoted argument x$(V)y 1:7\n"
                                       "right paren ) 1:13\n"
                                       "newline \n 1:14";

    // A line comment produces no token at all.
    QTest::newRow("line comment") << "# hi\nset(a)\n"
                                  << "newline \n 1:5\n"
                                     "identifier set 2:1\n"
                                     "left paren ( 2:4\n"
                                     "identifier a 2:5\n"
                                     "right paren ) 2:6\n"
                                     "newline \n 2:7";
}

void tst_CMakeLang::lexer()
{
    QFETCH(QString, source);
    QFETCH(QString, expected);
    QCOMPARE(lex(source), expected);
}

void tst_CMakeLang::parser_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << "" << "";
    QTest::newRow("blank lines") << "\n\n\n" << "";

    QTest::newRow("no trailing newline") << "set(a 1)" << "(cmd set a 1)";

    QTest::newRow("two commands") << "set(a)\nset(b)\n" << "(cmd set a) (cmd set b)";

    // Nested parentheses become a group instead of two bare "(" and ")"
    // arguments as in the flat token list.
    QTest::newRow("paren group") << "if(A AND (B OR C))\nendif()\n"
                                 << "(if (cmd if A AND (B OR C)) (cmd endif))";

    QTest::newRow("nested paren groups") << "foo((a) (b (c)))\n"
                                         << "(cmd foo (a) (b (c)))";

    QTest::newRow("if elseif else") << "if(A)\nm(1)\nelseif(B)\nm(2)\nelse()\nm(3)\nendif()\n"
                                    << "(if (cmd if A) (cmd m 1) "
                                       "(elseif (cmd elseif B) (cmd m 2)) "
                                       "(else (cmd else) (cmd m 3)) (cmd endif))";

    QTest::newRow("two elseif") << "if(A)\nelseif(B)\nelseif(C)\nendif()\n"
                                << "(if (cmd if A) (elseif (cmd elseif B)) "
                                   "(elseif (cmd elseif C)) (cmd endif))";

    QTest::newRow("nested if") << "if(A)\nif(B)\nm(1)\nendif()\nendif()\n"
                               << "(if (cmd if A) (if (cmd if B) (cmd m 1) (cmd endif)) "
                                  "(cmd endif))";

    QTest::newRow("foreach") << "foreach(x 1 2)\nm(${x})\nendforeach()\n"
                             << "(foreach (cmd foreach x 1 2) (cmd m ${x}) (cmd endforeach))";

    QTest::newRow("while") << "while(A)\nm()\nendwhile()\n"
                           << "(while (cmd while A) (cmd m) (cmd endwhile))";

    QTest::newRow("function") << "function(f a)\nm(${a})\nendfunction()\n"
                              << "(function (cmd function f a) (cmd m ${a}) (cmd endfunction))";

    QTest::newRow("macro") << "macro(f)\nendmacro()\n"
                           << "(macro (cmd macro f) (cmd endmacro))";

    QTest::newRow("block") << "block()\nm()\nendblock()\n"
                           << "(block (cmd block) (cmd m) (cmd endblock))";

    QTest::newRow("case insensitive") << "IF(A)\nElseIf(B)\nENDIF()\n"
                                      << "(if (cmd if A) (elseif (cmd elseif B)) (cmd endif))";

    QTest::newRow("comments between") << "if(A)\n# c\n#[[b]]\nm()\nendif()\n"
                                      << "(if (cmd if A) (cmd m) (cmd endif))";

    QTest::newRow("newlines inside parens") << "set(a\n  b\n  c\n)\n" << "(cmd set a b c)";

    QTest::newRow("quoted and bracket args") << "set(a \"q\" [[b]])\n"
                                             << "(cmd set a \"q\" [b])";
}

void tst_CMakeLang::parser()
{
    QFETCH(QString, source);
    QFETCH(QString, expected);
    QCOMPARE(parse(source), expected);
}

void tst_CMakeLang::blockBalancing_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expected");

    // An unbalanced file still parses: the keyword that has no partner is
    // demoted back to an ordinary command so that a half-written file keeps
    // producing an AST.
    QTest::newRow("if without endif") << "if(A)\nm()\n" << "(cmd if A) (cmd m)";

    QTest::newRow("endif without if") << "endif()\nm()\n" << "(cmd endif) (cmd m)";

    QTest::newRow("else without if") << "else()\nm()\n" << "(cmd else) (cmd m)";

    // The clauses of a demoted "if" have to be demoted with it.
    QTest::newRow("if with elseif but no endif")
        << "if(A)\nelseif(B)\nelse()\n" << "(cmd if A) (cmd elseif B) (cmd else)";

    QTest::newRow("crossed blocks") << "if(A)\nforeach(x)\nendif()\nendforeach()\n"
                                    << "(cmd if A) (foreach (cmd foreach x) (cmd endif) "
                                       "(cmd endforeach))";

    QTest::newRow("second else") << "if(A)\nelse()\nelse()\nendif()\n"
                                 << "(if (cmd if A) (else (cmd else) (cmd else)) (cmd endif))";

    QTest::newRow("elseif after else") << "if(A)\nelse()\nelseif(B)\nendif()\n"
                                       << "(if (cmd if A) (else (cmd else) (cmd elseif B)) "
                                          "(cmd endif))";

    QTest::newRow("endforeach closing if") << "if(A)\nendforeach()\nendif()\n"
                                           << "(if (cmd if A) (cmd endforeach) (cmd endif))";
}

void tst_CMakeLang::blockBalancing()
{
    QFETCH(QString, source);
    QFETCH(QString, expected);
    QCOMPARE(parse(source), expected);
}

void tst_CMakeLang::keywordsAreNotReserved()
{
    // CMake has no reserved words.  "if" is a valid argument, a valid variable
    // name and a valid name for a command that is not a block command.
    QCOMPARE(parse("set(if 1)\n"), "(cmd set if 1)");
    QCOMPARE(parse("message(if else endif)\n"), "(cmd message if else endif)");
    QCOMPARE(parse("if(if)\nendif()\n"), "(if (cmd if if) (cmd endif))");
    QCOMPARE(parse("set(a ${if})\n"), "(cmd set a ${if})");

    // Only an identifier directly followed by "(" can open a block, and only
    // at file level.
    QCOMPARE(parse("foo(if (a))\n"), "(cmd foo if (a))");
}

void tst_CMakeLang::bracketCommentNeedsNewline()
{
    // CMake counts a bracket comment as something that has to be followed by a
    // newline before a command may start.
    QVERIFY(!errorsOf("#[[c]] set(a)\n").isEmpty());
    QVERIFY(!errorsOf("#[[\nc\n]] set(a)\n").isEmpty());

    QCOMPARE(parse("#[[c]]\nset(a)\n"), "(cmd set a)");
    QCOMPARE(parse("set(a) #[[c]]\nset(b)\n"), "(cmd set a) (cmd set b)");

    // Inside an argument list it is a comment like any other.
    QCOMPARE(parse("set(a #[[c]] b)\n"), "(cmd set a b)");
}

void tst_CMakeLang::nodeSpans()
{
    // Every node covers what it owns: the file covers the whole source, and an
    // else-if or else clause reaches to the end of its last element.
    QCOMPARE(dumpSpans("if(A)\n  m(1)\nelseif(B)\n  m(2)\nelse()\n  m(3)\nendif()\n"),
             "(file if(A)~  m(1)~elseif(B)~  m(2)~else()~  m(3)~endif()~ "
             "(if if(A)~  m(1)~elseif(B)~  m(2)~else()~  m(3)~endif() "
             "(elseif elseif(B)~  m(2)) (else else()~  m(3))))");

    // A clause that owns nothing ends with its own newline.
    QCOMPARE(dumpSpans("if(A)\nelseif(B)\nelse()\nendif()\n"),
             "(file if(A)~elseif(B)~else()~endif()~ "
             "(if if(A)~elseif(B)~else()~endif() (elseif elseif(B)~) (else else()~)))");

    QCOMPARE(dumpSpans("foreach(x)\n  m()\nendforeach()\n"),
             "(file foreach(x)~  m()~endforeach()~ (foreach foreach(x)~  m()~endforeach()))");

    QCOMPARE(dumpSpans(""), "(file )");
}

void tst_CMakeLang::unterminatedConstructs()
{
    QVERIFY(errorsOf("set(a [[unterminated\n").contains("unterminated bracket"));
    QVERIFY(errorsOf("set(a \"unterminated\n").contains("unterminated string"));
    QVERIFY(!errorsOf("set(a\n").isEmpty());

    // Two commands on one line: CMake requires a newline after a command.
    QVERIFY(!errorsOf("set(a) set(b)\n").isEmpty());
}

void tst_CMakeLang::errorRecovery()
{
    // A syntax error costs the element it is in, not the rest of the file: the
    // commands reduced before it are kept and parsing resumes on the next line.
    QCOMPARE(parse("set(a)\n@if x\nset(b)\n"), "(cmd set a) (cmd set b)");
    QCOMPARE(parse("set(a) set(b)\nset(c)\n"), "(cmd set c)");
    QCOMPARE(parse("set(a)\nset(b\n"), "(cmd set a)");

    // A block that fails inside loses the block, not what came before it.
    QCOMPARE(parse("set(a)\nif(A)\n)\nendif()\nset(b)\n"), "(cmd set a) (cmd set b)");
}

static const char scopedSource[] = R"(add_executable(app main.cpp)
if(WIN32)
  target_sources(app PRIVATE win.cpp)
elseif(APPLE)
  target_sources(app PRIVATE mac.cpp)
else()
  foreach(f a b)
    target_sources(app PRIVATE ${f})
  endforeach()
endif()
target_sources(app PRIVATE other.cpp)
)";

void tst_CMakeLang::documentCommands()
{
    // The commands come in source order, the ones that open and close a
    // construct included.
    const DocumentPtr document = Document::fromSource(QString::fromLatin1(scopedSource));
    QVERIFY(document->isValid());

    QStringList names;
    for (CommandAST *command : document->commands())
        names << command->commandName();
    QCOMPARE(names,
             QStringList({"add_executable", "if", "target_sources", "elseif", "target_sources",
                          "else", "foreach", "target_sources", "endforeach", "endif",
                          "target_sources"}));

    const DocumentPtr broken = Document::fromSource("set(a\n");
    QVERIFY(!broken->isValid());
    QVERIFY(!broken->errorString().isEmpty());
    QVERIFY(broken->commands().isEmpty());

    // A document that did not parse cleanly still has what stands around the
    // broken line.
    const DocumentPtr recovered = Document::fromSource("set(a)\n@if x\nset(b)\n");
    QVERIFY(!recovered->isValid());
    QCOMPARE(recovered->commands().size(), 2);
}

void tst_CMakeLang::documentScopes()
{
    const DocumentPtr document = Document::fromSource(QString::fromLatin1(scopedSource));
    QVERIFY(document->isValid());

    const QList<CommandAST *> commands = document->commands();
    CommandAST *addExecutable = commands.at(0);
    CommandAST *ifCommand = commands.at(1);
    CommandAST *inThen = commands.at(2);
    CommandAST *inElseIf = commands.at(4);
    CommandAST *inForEach = commands.at(7);
    CommandAST *atFileLevel = commands.at(10);

    // A condition is evaluated where its construct sits.
    QVERIFY(document->enclosingConstructs(addExecutable).isEmpty());
    QVERIFY(document->enclosingConstructs(ifCommand).isEmpty());
    QCOMPARE(document->enclosingConstructs(inThen).size(), 1);
    QCOMPARE(document->enclosingConstructs(inElseIf).size(), 1);
    QCOMPARE(document->enclosingConstructs(inForEach).size(), 2);

    // What the file does unconditionally runs with everything.
    QVERIFY(document->runsWith(atFileLevel, inThen));
    QVERIFY(document->runsWith(atFileLevel, inForEach));

    // Branches that exclude each other do not run together.
    QVERIFY(!document->runsWith(inThen, inElseIf));
    QVERIFY(!document->runsWith(inElseIf, inThen));
    QVERIFY(!document->runsWith(inThen, atFileLevel));

    // The loop body is reached from the else branch, not the other way around.
    QVERIFY(!document->runsWith(inForEach, atFileLevel));
    QVERIFY(document->runsWith(atFileLevel, inForEach));
}

static const char qmlModuleDefinition[] = R"(function(qt6_add_qml_module target)
    set(args_option STATIC SHARED)
    set(args_single URI VERSION)
    set(args_multi SOURCES QML_FILES RESOURCES)
    cmake_parse_arguments(PARSE_ARGV 1 arg "${args_option}" "${args_single}" "${args_multi}")
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_qml_module)
        qt6_add_qml_module(${ARGV})
        cmake_parse_arguments(PARSE_ARGV 1 arg "" "OUTPUT_TARGETS" "")
    endfunction()
endif()
)";

static const char standardProjectSetupDefinition[] = R"(macro(qt6_standard_project_setup)
    if(NOT QT_NO_STANDARD_PROJECT_SETUP)
        set(__qt_sps_args_option)
        set(__qt_sps_args_single
            REQUIRES
            SUPPORTS_UP_TO
            I18N_SOURCE_LANGUAGE
        )
        set(__qt_sps_args_multi
            I18N_TRANSLATED_LANGUAGES
        )
        cmake_parse_arguments(__qt_sps_arg
            "${__qt_sps_args_option}"
            "${__qt_sps_args_single}"
            "${__qt_sps_args_multi}"
            ${ARGN}
        )
    endif()
endmacro()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    macro(qt_standard_project_setup)
        qt6_standard_project_setup(${ARGV})
    endmacro()
endif()
)";

static QString arityOf(const Signature &signature, const QString &keyword)
{
    const std::optional<Signature::Arity> arity = signature.arity(keyword);
    if (!arity)
        return "none";
    switch (*arity) {
    case Signature::Option:
        return "option";
    case Signature::OneValue:
        return "one";
    case Signature::MultiValue:
        return "multi";
    }
    return {};
}

void tst_CMakeLang::signatures_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("command");
    QTest::addColumn<QString>("keywords");
    QTest::addColumn<QString>("expected");

    QTest::newRow("literal lists")
        << "function(f target)\n"
           "  cmake_parse_arguments(PARSE_ARGV 1 arg \"STATIC\" \"URI\" \"QML_FILES;SOURCES\")\n"
           "endfunction()\n"
        << "f" << "STATIC URI QML_FILES SOURCES NOPE" << "option one multi multi none";

    // Without PARSE_ARGV the lists come after the prefix, and the arguments to
    // parse after them.
    QTest::newRow("macro")
        << "macro(m)\n"
           "  cmake_parse_arguments(arg \"OPT\" \"\" \"FILES\" ${ARGN})\n"
           "endmacro()\n"
        << "m" << "OPT FILES" << "option multi";

    QTest::newRow("variables") << QString::fromLatin1(qmlModuleDefinition)
                               << "qt6_add_qml_module" << "STATIC URI QML_FILES"
                               << "option one multi";

    // The lists are commonly built up over several lines and handed to
    // cmake_parse_arguments() as variables, together with ${ARGN}.
    QTest::newRow("variables without PARSE_ARGV")
        << "function(add_qtc_plugin target_name)\n"
           "  set(opt_args\n"
           "    SKIP_INSTALL\n"
           "    EXPORT\n"
           "  )\n"
           "  set(single_args\n"
           "    VERSION\n"
           "    PLUGIN_NAME\n"
           "  )\n"
           "  set(multi_args\n"
           "    DEPENDS\n"
           "    SOURCES\n"
           "  )\n"
           "  cmake_parse_arguments(_arg \"${opt_args}\" \"${single_args}\" \"${multi_args}\""
           " ${ARGN})\n"
           "endfunction()\n"
        << "add_qtc_plugin" << "EXPORT PLUGIN_NAME SOURCES ARGN" << "option one multi none";

    QTest::newRow("variable naming a variable")
        << "function(f)\n"
           "  set(common URI)\n"
           "  set(args_single ${common} VERSION)\n"
           "  cmake_parse_arguments(PARSE_ARGV 0 arg \"\" \"${args_single}\" \"\")\n"
           "endfunction()\n"
        << "f" << "URI VERSION" << "one one";

    QTest::newRow("appended variable")
        << "function(f)\n"
           "  set(args_multi SOURCES)\n"
           "  list(APPEND args_multi QML_FILES)\n"
           "  cmake_parse_arguments(PARSE_ARGV 0 arg \"\" \"\" \"${args_multi}\")\n"
           "endfunction()\n"
        << "f" << "SOURCES QML_FILES" << "multi multi";

    // A command that hands ${ARGV} on takes what the command it calls takes,
    // its own keywords included.
    QTest::newRow("forwarded keywords")
        << QString::fromLatin1(qmlModuleDefinition) << "qt_add_qml_module"
        << "QML_FILES URI OUTPUT_TARGETS" << "multi one one";

    // The keywords of a versionless command sit behind an if() block, a macro
    // and a forwarded ${ARGV}.
    QTest::newRow("keywords behind a condition")
        << QString::fromLatin1(standardProjectSetupDefinition) << "qt_standard_project_setup"
        << "REQUIRES I18N_SOURCE_LANGUAGE I18N_TRANSLATED_LANGUAGES" << "one one multi";

    QTest::newRow("command names are case insensitive")
        << "FUNCTION(F)\n"
           "  cmake_parse_arguments(PARSE_ARGV 0 arg \"\" \"\" \"FILES\")\n"
           "ENDFUNCTION()\n"
        << "f" << "FILES" << "multi";
}

void tst_CMakeLang::signatures()
{
    QFETCH(QString, source);
    QFETCH(QString, command);
    QFETCH(QString, keywords);
    QFETCH(QString, expected);

    SignatureTable table;
    table.addDocument(Document::fromSource(source));
    const Signature signature = table.signature(command);

    QStringList arities;
    for (const QString &keyword : keywords.split(u' ', Qt::SkipEmptyParts))
        arities << arityOf(signature, keyword);
    QCOMPARE(arities.join(u' '), expected);
}

void tst_CMakeLang::signatureKeywords()
{
    SignatureTable table;
    table.addDocument(Document::fromSource(QString::fromLatin1(standardProjectSetupDefinition)));

    QCOMPARE(table.signature("qt_standard_project_setup").keywords().join(u' '),
             "I18N_SOURCE_LANGUAGE I18N_TRANSLATED_LANGUAGES REQUIRES SUPPORTS_UP_TO");
    QVERIFY(table.signature("qt_add_qml_module").keywords().isEmpty());
}

void tst_CMakeLang::signaturesNeedTheSource()
{
    auto signatureOf = [](const QString &source, const QString &command) {
        SignatureTable table;
        table.addDocument(Document::fromSource(source));
        return table.signature(command);
    };

    // A keyword list that the file does not spell out leaves no signature at
    // all: half of the keywords would group the arguments of a call wrongly.
    QVERIFY(signatureOf("function(f)\n"
                        "  cmake_parse_arguments(PARSE_ARGV 0 arg \"${elsewhere}\" \"\" \"FILES\")\n"
                        "endfunction()\n",
                        "f")
                .isEmpty());

    // What a call outside a function or macro parses belongs to no command.
    QVERIFY(signatureOf("cmake_parse_arguments(PARSE_ARGV 0 arg \"\" \"\" \"FILES\")\n", "f")
                .isEmpty());

    QVERIFY(signatureOf("function(f)\nendfunction()\n", "f").isEmpty());
    QVERIFY(signatureOf("", "f").isEmpty());
}

static QString groupsOf(const QString &definition, const QString &call)
{
    SignatureTable table;
    table.addDocument(Document::fromSource(definition));

    const DocumentPtr document = Document::fromSource(call);
    if (!document->isValid() || document->commands().isEmpty())
        return "<error>";

    CommandAST *command = document->commands().first();
    QStringList dumped;
    for (const KeywordArguments &group :
         groupArguments(command, table.signature(command->commandName()))) {
        QStringList parts{group.keyword ? group.keyword->value() : QString(u'-')};
        for (ArgumentAST *value : group.values)
            parts << value->value();
        dumped << '(' + parts.join(u' ') + ')';
    }
    return dumped.join(u' ');
}

// A command that hands its arguments on takes what the command it hands them
// to says about them, so whoever reads the documentation has to be told where
// they went.
void tst_CMakeLang::signaturesForwarded()
{
    const QString source = R"(function(inner)
  cmake_parse_arguments(_arg "OPTION" "" "SOURCES" ${ARGN})
endfunction()

function(middle target)
  inner(${ARGN})
endfunction()

function(outer target)
  middle(${ARGN})
endfunction()

function(alone target)
  cmake_parse_arguments(_arg "" "" "FILES" ${ARGN})
endfunction()
)";

    SignatureTable signatures;
    signatures.addDocument(Document::fromSource(source));

    // The keywords of the command at the end of the chain are the keywords
    // of every command along it.
    QCOMPARE(signatures.signature("outer").keywords(), QStringList({"OPTION", "SOURCES"}));

    // Where they went, in the order they were handed on.
    QCOMPARE(signatures.forwardsTo("outer"), QStringList({"middle", "inner"}));
    QCOMPARE(signatures.forwardsTo("middle"), QStringList("inner"));
    QCOMPARE(signatures.forwardsTo("inner"), QStringList());
    QCOMPARE(signatures.forwardsTo("alone"), QStringList());

    // A command the documents do not define hands nothing on.
    QCOMPARE(signatures.forwardsTo("nowhere"), QStringList());

    // A name is read the way CMake reads it.
    QCOMPARE(signatures.forwardsTo("OUTER"), QStringList({"middle", "inner"}));
}

void tst_CMakeLang::argumentGroups_data()
{
    QTest::addColumn<QString>("call");
    QTest::addColumn<QString>("expected");

    QTest::newRow("keywords and values")
        << "qt_add_qml_module(app URI My VERSION 1.0 QML_FILES Main.qml Item.qml)\n"
        << "(- app) (URI My) (VERSION 1.0) (QML_FILES Main.qml Item.qml)";

    QTest::newRow("repeated keyword") << "qt_add_qml_module(app QML_FILES a.qml QML_FILES b.qml)\n"
                                      << "(- app) (QML_FILES a.qml) (QML_FILES b.qml)";

    QTest::newRow("keyword without values") << "qt_add_qml_module(app QML_FILES)\n"
                                            << "(- app) (QML_FILES)";

    QTest::newRow("option takes nothing") << "qt_add_qml_module(app STATIC QML_FILES a.qml)\n"
                                          << "(- app) (STATIC) (QML_FILES a.qml)";

    // What no keyword takes stands on its own, the way it ends up in
    // arg_UNPARSED_ARGUMENTS.
    QTest::newRow("value after an option")
        << "qt_add_qml_module(app STATIC extra QML_FILES a.qml)\n"
        << "(- app) (STATIC) (- extra) (QML_FILES a.qml)";

    QTest::newRow("one value keyword takes one") << "qt_add_qml_module(app URI My extra)\n"
                                                 << "(- app) (URI My) (- extra)";

    QTest::newRow("command with no signature") << "add_executable(app URI main.cpp)\n"
                                               << "(- app URI main.cpp)";
}

void tst_CMakeLang::argumentGroups()
{
    QFETCH(QString, call);
    QFETCH(QString, expected);
    QCOMPARE(groupsOf(QString::fromLatin1(qmlModuleDefinition), call), expected);
}

static ArgumentAST *argumentNaming(const DocumentPtr &document, const QString &value)
{
    for (CommandAST *command : document->commands()) {
        for (ArgumentAST *argument : command->arguments()) {
            if (argument->value() == value)
                return argument;
        }
    }
    return nullptr;
}

static CommandAST *commandNamed(const DocumentPtr &document, const QString &name)
{
    for (CommandAST *command : document->commands()) {
        if (command->isNamed(name))
            return command;
    }
    return nullptr;
}

using Change = std::function<void(const DocumentPtr &, Rewriter &)>;

static QString rewritten(const QString &source, const Change &change)
{
    const DocumentPtr document = Document::fromSource(source);
    if (!document->isValid())
        return "<error>";

    Rewriter rewriter(document);
    change(document, rewriter);

    QString result = source;
    const QList<Edit> edits = rewriter.edits();
    for (auto it = edits.crbegin(); it != edits.crend(); ++it)
        result.replace(it->position, it->length, it->text);
    return result;
}

void tst_CMakeLang::rewriterReplacesValues()
{
    // What the line is indented by is the author's, and a new value is no
    // reason to reconsider it.
    QCOMPARE(rewritten("qt_add_qml_module(app\n"
                       "    QML_FILES\n"
                       "        Main.qml\n"
                       ")\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.replaceValue(argumentNaming(document, "Main.qml"),
                                                 "Main2.qml");
                       }),
             "qt_add_qml_module(app\n"
             "    QML_FILES\n"
             "        Main2.qml\n"
             ")\n");

    // A value that was quoted stays quoted, and one that needs quotes gets
    // them.
    QCOMPARE(rewritten("set(FILES \"a b.cpp\")\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.replaceValue(argumentNaming(document, "a b.cpp"), "c.cpp");
                       }),
             "set(FILES \"c.cpp\")\n");

    QCOMPARE(rewritten("set(FILES a.cpp)\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.replaceValue(argumentNaming(document, "a.cpp"), "a b.cpp");
                       }),
             "set(FILES \"a b.cpp\")\n");

    // A quote or a backslash in the value is escaped either way, so it cannot
    // end the argument early.
    QCOMPARE(rewritten("set(FILES \"a b.cpp\")\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.replaceValue(argumentNaming(document, "a b.cpp"),
                                                 "c\\d\"e.cpp");
                       }),
             "set(FILES \"c\\\\d\\\"e.cpp\")\n");

    QCOMPARE(rewritten("set(FILES a.cpp)\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.replaceValue(argumentNaming(document, "a.cpp"),
                                                 "c\\d\"e.cpp");
                       }),
             "set(FILES \"c\\\\d\\\"e.cpp\")\n");
}

void tst_CMakeLang::rewriterRemovesArguments()
{
    // One of several values on a line: the value goes, the line stays.
    QCOMPARE(rewritten("qt_add_qml_module(app\n"
                       "    QML_FILES Main.qml Other.qml\n"
                       ")\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.remove(argumentNaming(document, "Main.qml"));
                       }),
             "qt_add_qml_module(app\n"
             "    QML_FILES Other.qml\n"
             ")\n");

    // The only value on its line: the line goes with it.
    QCOMPARE(rewritten("qt_add_qml_module(app\n"
                       "    QML_FILES\n"
                       "        Main.qml\n"
                       "        Item.qml\n"
                       ")\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.remove(argumentNaming(document, "Main.qml"));
                       }),
             "qt_add_qml_module(app\n"
             "    QML_FILES\n"
             "        Item.qml\n"
             ")\n");

    // A line of a file with CRLF endings goes with its whole ending, and the
    // line before keeps one.
    QCOMPARE(rewritten("qt_add_qml_module(app\r\n"
                       "    QML_FILES\r\n"
                       "        Main.qml\r\n"
                       "        Item.qml\r\n"
                       ")\r\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.remove(argumentNaming(document, "Main.qml"));
                       }),
             "qt_add_qml_module(app\r\n"
             "    QML_FILES\r\n"
             "        Item.qml\r\n"
             ")\r\n");

    // From the keyword to its last value, across the lines they stand on.
    QCOMPARE(rewritten("qt_add_qml_module(app\n"
                       "    QML_FILES\n"
                       "        Main.qml\n"
                       "    SOURCES backend.cpp\n"
                       ")\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.remove(argumentNaming(document, "QML_FILES"),
                                           argumentNaming(document, "Main.qml"));
                       }),
             "qt_add_qml_module(app\n"
             "    SOURCES backend.cpp\n"
             ")\n");
}

void tst_CMakeLang::rewriterInsertsValues()
{
    // A value goes after the one it joins, on a line indented the same way.
    QCOMPARE(rewritten("qt_add_qml_module(app\n"
                       "    QML_FILES Main.qml\n"
                       ")\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.insertAfter(argumentNaming(document, "Main.qml"),
                                                {"Item.qml"});
                       }),
             "qt_add_qml_module(app\n"
             "    QML_FILES Main.qml\n"
             "    Item.qml\n"
             ")\n");

    QCOMPARE(rewritten("qt_add_qml_module(app\n"
                       "    QML_FILES\n"
                       "        Main.qml\n"
                       ")\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.insertAfter(argumentNaming(document, "Main.qml"),
                                                {"Item.qml"});
                       }),
             "qt_add_qml_module(app\n"
             "    QML_FILES\n"
             "        Main.qml\n"
             "        Item.qml\n"
             ")\n");

    // A keyword the call does not have yet goes after its last argument.
    QCOMPARE(rewritten("qt_add_qml_module(app\n"
                       "    URI QuickApp\n"
                       "    VERSION 1.0\n"
                       ")\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.append(commandNamed(document, "qt_add_qml_module"),
                                           {"RESOURCES logo.png", "QML_FILES Item.qml"});
                       }),
             "qt_add_qml_module(app\n"
             "    URI QuickApp\n"
             "    VERSION 1.0\n"
             "    RESOURCES logo.png\n"
             "    QML_FILES Item.qml\n"
             ")\n");

    // Several changes to one call come out in the order the file spells them.
    QCOMPARE(rewritten("qt_add_qml_module(app\n"
                       "    QML_FILES\n"
                       "        Main.qml\n"
                       "    SOURCES\n"
                       "        backend.cpp\n"
                       ")\n",
                       [](const DocumentPtr &document, Rewriter &rewriter) {
                           rewriter.insertAfter(argumentNaming(document, "backend.cpp"),
                                                {"extra.cpp"});
                           rewriter.insertAfter(argumentNaming(document, "Main.qml"),
                                                {"Item.qml"});
                       }),
             "qt_add_qml_module(app\n"
             "    QML_FILES\n"
             "        Main.qml\n"
             "        Item.qml\n"
             "    SOURCES\n"
             "        backend.cpp\n"
             "        extra.cpp\n"
             ")\n");
}

// Stands in for what the CMake documentation and the cmake_parse_arguments()
// calls of a project tell the editor about a command.
static bool namesKeyword(const QString &command, const QString &argument)
{
    static const QHash<QString, QStringList> keywords = {
        {"qt_internal_add_module", {"SOURCES", "LIBRARIES"}},
        {"target_link_libraries", {"PRIVATE", "PUBLIC", "INTERFACE"}},
        {"install", {"TARGETS", "RUNTIME", "DESTINATION", "INCLUDES"}},
    };
    return keywords.value(command.toLower()).contains(argument);
}

static Style testStyle()
{
    Style style;
    style.isKeyword = namesKeyword;
    return style;
}

static QString indented(const QString &source)
{
    const Indentation indentation(source, testStyle());

    QStringList result;
    const QStringList lines = source.split(u'\n');
    for (int line = 0; line < lines.size(); ++line) {
        const int level = indentation.levelAt(line + 1);
        if (level == Indentation::Keep) {
            result << lines.at(line);
            continue;
        }
        const QString text = lines.at(line).trimmed();
        result << (text.isEmpty() ? QString() : QString(level * 4, u' ') + text);
    }
    return result.join(u'\n');
}

void tst_CMakeLang::indentation_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expected");

    QTest::newRow("body of a block") << "if(WIN32)\n"
                                        "add_executable(app main.cpp)\n"
                                        "else()\n"
                                        "add_executable(app other.cpp)\n"
                                        "endif()\n"
                                     << "if(WIN32)\n"
                                        "    add_executable(app main.cpp)\n"
                                        "else()\n"
                                        "    add_executable(app other.cpp)\n"
                                        "endif()\n";

    QTest::newRow("nested blocks") << "foreach(name IN LISTS names)\n"
                                      "while(name)\n"
                                      "message(STATUS ${name})\n"
                                      "endwhile()\n"
                                      "endforeach()\n"
                                   << "foreach(name IN LISTS names)\n"
                                      "    while(name)\n"
                                      "        message(STATUS ${name})\n"
                                      "    endwhile()\n"
                                      "endforeach()\n";

    QTest::newRow("arguments and the closing parenthesis") << "set(FRUITS\n"
                                                              "APPLE\n"
                                                              "BANANA\n"
                                                              ")\n"
                                                           << "set(FRUITS\n"
                                                              "    APPLE\n"
                                                              "    BANANA\n"
                                                              ")\n";

    QTest::newRow("a keyword of its own opens a list") << "qt_internal_add_module(Core\n"
                                                          "SOURCES\n"
                                                          "foo.cpp\n"
                                                          "bar.cpp\n"
                                                          "LIBRARIES\n"
                                                          "Qt::Platform\n"
                                                          ")\n"
                                                       << "qt_internal_add_module(Core\n"
                                                          "    SOURCES\n"
                                                          "        foo.cpp\n"
                                                          "        bar.cpp\n"
                                                          "    LIBRARIES\n"
                                                          "        Qt::Platform\n"
                                                          ")\n";

    QTest::newRow("a keyword next to the command name opens none")
        << "target_link_libraries(app PRIVATE\n"
           "Qt::Core\n"
           "Qt::Gui\n"
           ")\n"
        << "target_link_libraries(app PRIVATE\n"
           "    Qt::Core\n"
           "    Qt::Gui\n"
           ")\n";

    QTest::newRow("a keyword that takes its value along closes the list")
        << "install(TARGETS app\n"
           "RUNTIME DESTINATION bin\n"
           "INCLUDES\n"
           "include\n"
           ")\n"
        << "install(TARGETS app\n"
           "    RUNTIME DESTINATION bin\n"
           "    INCLUDES\n"
           "        include\n"
           ")\n";

    // The table names no keyword of set(), so nothing below it opens a list.
    QTest::newRow("an unknown command lays its arguments out flat")
        << "set(FRUITS\n"
           "APPLE\n"
           "BANANA\n"
           "CHERRY)\n"
        << "set(FRUITS\n"
           "    APPLE\n"
           "    BANANA\n"
           "    CHERRY)\n";

    QTest::newRow("a group of parentheses") << "if(A AND\n"
                                               "(B OR\n"
                                               "C)\n"
                                               ")\n"
                                            << "if(A AND\n"
                                               "    (B OR\n"
                                               "        C)\n"
                                               ")\n";

    QTest::newRow("comments go where the code goes") << "if(WIN32)\n"
                                                        "# what this does\n"
                                                        "add_executable(app main.cpp)\n"
                                                        "endif()\n"
                                                     << "if(WIN32)\n"
                                                        "    # what this does\n"
                                                        "    add_executable(app main.cpp)\n"
                                                        "endif()\n";

    QTest::newRow("a comment does not close a list") << "qt_internal_add_module(Core\n"
                                                        "SOURCES\n"
                                                        "# the sources\n"
                                                        "foo.cpp\n"
                                                        ")\n"
                                                     << "qt_internal_add_module(Core\n"
                                                        "    SOURCES\n"
                                                        "        # the sources\n"
                                                        "        foo.cpp\n"
                                                        ")\n";

    // if() is an ordinary identifier wherever it is not a command, so it opens
    // no block there.
    QTest::newRow("a block command used as a value") << "set(if 1)\n"
                                                        "message(STATUS ${if})\n"
                                                     << "set(if 1)\n"
                                                        "message(STATUS ${if})\n";
}

void tst_CMakeLang::indentation()
{
    QFETCH(QString, source);
    QFETCH(QString, expected);
    QCOMPARE(indented(source), expected);
}

void tst_CMakeLang::indentationOfHalfWrittenFiles_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("expected");

    // QTCREATORBUG-19417: the line below a call that spans lines belongs to
    // the file again, however the arguments of the call were laid out.
    QTest::newRow("below a call that spans lines") << "set(FRUITS APPLE\n"
                                                      "           BANANA\n"
                                                      "           CHERRY)\n"
                                                   << 4 << 0;

    QTest::newRow("inside a call that is not closed yet") << "add_executable(app\n" << 2 << 1;

    QTest::newRow("below a block that is not closed yet") << "if(WIN32)\n" << 2 << 1;

    QTest::newRow("inside a call inside a block") << "if(WIN32)\n"
                                                     "    set(SOURCES\n"
                                                  << 3 << 2;

    QTest::newRow("below a keyword that opened a list") << "qt_internal_add_module(Core\n"
                                                           "    SOURCES\n"
                                                        << 3 << 2;

    QTest::newRow("an endif() that nothing opens") << "endif()\n" << 1 << 0;

    QTest::newRow("a closing parenthesis that nothing opens") << ")\n" << 1 << 0;

    QTest::newRow("a block that the wrong command closes") << "if(WIN32)\n"
                                                              "endforeach()\n"
                                                           << 2 << 1;
}

void tst_CMakeLang::indentationOfHalfWrittenFiles()
{
    QFETCH(QString, source);
    QFETCH(int, line);
    QFETCH(int, expected);

    const Indentation indentation(source, testStyle());
    QCOMPARE(indentation.levelAt(line), expected);
}

void tst_CMakeLang::indentationKeepsMultilineValues()
{
    const QString source = "set(TEXT \"first\n"
                           "  second\")\n"
                           "message(STATUS ${TEXT})\n";
    const Indentation indentation(source);

    QCOMPARE(indentation.levelAt(1), 0);
    QCOMPARE(indentation.levelAt(2), Indentation::Keep);
    QCOMPARE(indentation.levelAt(3), 0);

    // The tail of the value closed the call, so what follows is at file level.
    QCOMPARE(indentation.levelAt(4), 0);
}

static QString formatted(const QString &source, const Style &style = testStyle())
{
    QString result = source;
    const QList<Edit> edits = formattingEdits(source, style);
    for (auto it = edits.crbegin(); it != edits.crend(); ++it)
        result.replace(it->position, it->length, it->text);
    return result;
}

void tst_CMakeLang::formatting_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expected");

    QTest::newRow("the parentheses of a call") << "IF( WIN32 )\n"
                                                  "add_executable( app  main.cpp )\n"
                                                  "ENDIF( WIN32 )\n"
                                               << "IF(WIN32)\n"
                                                  "    add_executable(app main.cpp)\n"
                                                  "ENDIF(WIN32)\n";

    QTest::newRow("whitespace at the end of a line and of the file")
        << "set(a 1)   \n"
           "\n"
           "   \n"
           "set(b 2)"
        << "set(a 1)\n"
           "\n"
           "\n"
           "set(b 2)\n";

    // Nothing is packed onto a line the author did not put it on.
    QTest::newRow("the line breaks stay") << "target_sources(app PRIVATE\n"
                                             "a.cpp\n"
                                             "b.cpp)\n"
                                          << "target_sources(app PRIVATE\n"
                                             "    a.cpp\n"
                                             "    b.cpp)\n";

    QTest::newRow("a group within the arguments stands apart") << "if(NOT (A OR B))\n"
                                                               << "if(NOT (A OR B))\n";

    QTest::newRow("a comment keeps the column it is in") << "set(a 1)    # first\n"
                                                            "set(bb 2)   # second\n"
                                                         << "set(a 1)    # first\n"
                                                            "set(bb 2)   # second\n";

    QTest::newRow("a comment gets a space at least") << "set(a 1)# note\n"
                                                     << "set(a 1) # note\n";

    QTest::newRow("a value that spans lines is untouched") << "set(TEXT \"first\n"
                                                              "  second\")\n"
                                                              "message(  STATUS   ${TEXT} )\n"
                                                           << "set(TEXT \"first\n"
                                                              "  second\")\n"
                                                              "message(STATUS ${TEXT})\n";

    QTest::newRow("a bracket comment is untouched") << "#[[\n"
                                                       "  what this file does\n"
                                                       "]]\n"
                                                       "message( X )\n"
                                                    << "#[[\n"
                                                       "  what this file does\n"
                                                       "]]\n"
                                                       "message(X)\n";

    QTest::newRow("the layout of a keyword list") << "qt_internal_add_module(Core\n"
                                                     "SOURCES\n"
                                                     "foo.cpp\n"
                                                     ")\n"
                                                  << "qt_internal_add_module(Core\n"
                                                     "    SOURCES\n"
                                                     "        foo.cpp\n"
                                                     ")\n";

    QTest::newRow("the name of a call meets its parenthesis") << "message ( STATUS x )\n"
                                                              << "message(STATUS x)\n";
}

void tst_CMakeLang::formatting()
{
    QFETCH(QString, source);
    QFETCH(QString, expected);
    QCOMPARE(formatted(source), expected);
}

void tst_CMakeLang::formattingLeavesFormattedFilesAlone_data()
{
    QTest::addColumn<QString>("source");

    QTest::newRow("a block") << "if(WIN32)\n"
                                "    add_executable(app main.cpp)\n"
                                "else()\n"
                                "    add_executable(app other.cpp)\n"
                                "endif()\n";

    QTest::newRow("a keyword list") << "qt_internal_add_module(Core\n"
                                       "    SOURCES\n"
                                       "        foo.cpp\n"
                                       "    LIBRARIES\n"
                                       "        Qt::Platform\n"
                                       ")\n";

    QTest::newRow("a trailing comment") << "set(a 1) # note\n";

    QTest::newRow("a value that spans lines") << "set(TEXT \"first\n"
                                                 "  second\")\n";

    QTest::newRow("a blank line") << "set(a 1)\n"
                                     "\n"
                                     "set(b 2)\n";
}

void tst_CMakeLang::formattingLeavesFormattedFilesAlone()
{
    QFETCH(QString, source);

    QVERIFY(formattingEdits(source, testStyle()).isEmpty());

    // And what it does to a file it does once: running it again finds nothing.
    QCOMPARE(formatted(formatted(source)), formatted(source));
}

void tst_CMakeLang::formattingEditsAreMinimal()
{
    const QString source = "set(a  1)\nset(b 2)\n";
    const QList<Edit> edits = formattingEdits(source, {});

    QCOMPARE(edits.size(), 1);
    QCOMPARE(edits.first().position, 5);
    QCOMPARE(edits.first().length, 2);
    QCOMPARE(edits.first().text, QString(" "));

    // The editor applies them to the text they were measured against, so they
    // come in order and none of them reaches into the next.
    const QString messy = "IF( WIN32 )\nadd_executable( app  main.cpp )\nENDIF( WIN32 )";
    int reached = 0;
    for (const Edit &edit : formattingEdits(messy, {})) {
        QVERIFY(edit.position >= reached);
        reached = edit.position + edit.length;
    }
    QVERIFY(reached <= messy.size());
}

void tst_CMakeLang::styleSwitches_data()
{
    QTest::addColumn<Style>("style");
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expected");

    Style spaceAfterControl = testStyle();
    spaceAfterControl.spaceBeforeControlParen = true;

    // if() keeps the space, and the call in the body does not get one.
    QTest::newRow("a space after a control keyword") << spaceAfterControl
                                                     << "if (WIN32)\n"
                                                        "message (STATUS x)\n"
                                                        "endif ()\n"
                                                     << "if (WIN32)\n"
                                                        "    message(STATUS x)\n"
                                                        "endif ()\n";

    QTest::newRow("a space after a control keyword is put back")
        << spaceAfterControl << "if(WIN32)\n"
                                "endif()\n"
        << "if (WIN32)\n"
           "endif ()\n";

    Style spaceAfterCommand = testStyle();
    spaceAfterCommand.spaceBeforeCommandParen = true;

    QTest::newRow("a space after a command name") << spaceAfterCommand
                                                  << "if(WIN32)\n"
                                                     "message(STATUS x)\n"
                                                     "endif()\n"
                                                  << "if(WIN32)\n"
                                                     "    message (STATUS x)\n"
                                                     "endif()\n";

    Style flatKeywords = testStyle();
    flatKeywords.indentKeywordValues = false;

    QTest::newRow("a keyword opens no list") << flatKeywords
                                             << "qt_internal_add_module(Core\n"
                                                "SOURCES\n"
                                                "foo.cpp\n"
                                                ")\n"
                                             << "qt_internal_add_module(Core\n"
                                                "    SOURCES\n"
                                                "    foo.cpp\n"
                                                ")\n";

    Style tidyComments = testStyle();
    tidyComments.keepCommentColumn = false;

    QTest::newRow("a comment loses its column") << tidyComments
                                                << "set(a 1)    # first\n"
                                                   "set(bb 2)   # second\n"
                                                << "set(a 1) # first\n"
                                                   "set(bb 2) # second\n";

    Style twoSpaces = testStyle();
    twoSpaces.indentation = [](int level) { return QString(level * 2, u' '); };

    QTest::newRow("two spaces a level") << twoSpaces
                                        << "if(WIN32)\n"
                                           "message(STATUS x)\n"
                                           "endif()\n"
                                        << "if(WIN32)\n"
                                           "  message(STATUS x)\n"
                                           "endif()\n";

    Style tabs = testStyle();
    tabs.indentation = [](int level) { return QString(level, u'\t'); };

    QTest::newRow("tabs") << tabs
                          << "if(WIN32)\n"
                             "message(STATUS x)\n"
                             "endif()\n"
                          << "if(WIN32)\n"
                             "\tmessage(STATUS x)\n"
                             "endif()\n";
}

void tst_CMakeLang::styleSwitches()
{
    QFETCH(Style, style);
    QFETCH(QString, source);
    QFETCH(QString, expected);

    QCOMPARE(formatted(source, style), expected);

    // Whatever the style, running it again finds nothing left to do.
    QVERIFY(formattingEdits(expected, style).isEmpty());
}

void tst_CMakeLang::formattingKeepsWhatItIsGiven()
{
    Style tidy = testStyle();
    tidy.keepCommentColumn = false;

    const QString source = "set(a 1)    # note\n";
    QCOMPARE(formatted(source, testStyle()), source);

    // Keeping the column keeps what stands there, so a column an earlier run
    // took away does not come back. Whatever shows the setting at work has to
    // lay the text out from the source every time rather than from what it
    // last showed.
    const QString tidied = formatted(source, tidy);
    QCOMPARE(tidied, QString("set(a 1) # note\n"));
    QCOMPARE(formatted(tidied, testStyle()), tidied);
}

// The way CMake documents a module, and the way a project documents a
// function of its own: a bracket comment that opens with ".rst:".
void tst_CMakeLang::documentationComments()
{
    const QString source = R"(#[[.rst:
my_helper
---------

Does a thing.
#]]
function(my_helper target source)
endfunction()

#.rst:
# .. command:: other_helper
#
#   Does another thing:
#
#   .. code-block:: cmake
#
#     other_helper(<name>)
#
#   ``<name>``
#     What to call it.
macro(other_helper name)
endmacro()
)";

    const QList<DocComment> comments = CMakeLang::documentationComments(source);
    QCOMPARE(comments.size(), 2);
    QCOMPARE(comments.at(0).line, 2);
    QCOMPARE(comments.at(0).text, "my_helper\n---------\n\nDoes a thing.\n");
    QCOMPARE(comments.at(1).line, 11);

    const DocumentPtr document = Document::fromSource(source);
    QVERIFY(document->isValid());

    const QList<Documentation> documentation = CMakeLang::documentation(document);
    QCOMPARE(documentation.size(), 2);

    // The comment names what it stands in front of, so it documents it.
    QCOMPARE(documentation.at(0).name, "my_helper");
    QCOMPARE(documentation.at(0).kind, Documentation::Command);
    QCOMPARE(documentation.at(0).markdown(), "### my_helper\n\nDoes a thing.");

    QCOMPARE(documentation.at(1).name, "other_helper");
    QCOMPARE(documentation.at(1).kind, Documentation::Command);
    QCOMPARE(documentation.at(1).signatures(), QStringList("other_helper(<name>)"));

    const QList<ArgumentDoc> arguments = documentation.at(1).arguments();
    QCOMPARE(arguments.size(), 1);
    QCOMPARE(arguments.at(0).name, "<name>");
    QCOMPARE(arguments.at(0).documentation, "What to call it.");

    // A definition spells its parameters out whether it is documented or
    // not.
    CommandAST *definition = document->commands().first();
    QCOMPARE(definitionSignature(definition), "my_helper(<target> <source>)");
}

// An index of the commands the modules of CMake provide is scanned out of
// their comments, and what the scan finds is what reading them says.
void tst_CMakeLang::documentedCommands()
{
    const QString source = R"(#[[.rst:
MyModule
--------

.. variable:: MY_VARIABLE

  Not a command.

.. command:: my_command

  Does a thing.

.. macro:: my_macro

.. function:: my_function
#]]

# .. command:: not_documented

macro(my_macro)
endmacro()
)";

    const QStringList scanned = CMakeLang::documentedCommands(source);
    QCOMPARE(scanned, QStringList({"my_command", "my_macro", "my_function"}));

    QStringList read;
    for (const DocComment &comment : CMakeLang::documentationComments(source)) {
        const RstLang::DocumentPtr rst = RstLang::Document::fromSource(comment.text);
        for (const Documentation &documentation : CMakeLang::documentation(rst)) {
            if (documentation.kind == Documentation::Command)
                read.append(documentation.name);
        }
    }
    QCOMPARE(scanned, read);

    // A module of CMake carries the line endings of the platform it was
    // unpacked on, and a carriage return is no part of a name.
    QCOMPARE(CMakeLang::documentedCommands(QString(source).replace('\n', "\r\n")), scanned);

    // A comment that carries no documentation says nothing, however it is
    // written.
    QVERIFY(CMakeLang::documentedCommands("# .. command:: not_documented\n").isEmpty());
}

// A module documents itself and the commands it provides.
void tst_CMakeLang::documentationOfModules()
{
    const QString source = R"(#[=[.rst:
FindPackageMessage
------------------

This module provides a command.

.. command:: find_package_message

  Prints a message:

  .. code-block:: cmake

    find_package_message(<PackageName> <message> <details>)
#]=]

function(find_package_message pkg msg details)
endfunction()
)";

    const DocumentPtr document = Document::fromSource(source);
    QVERIFY(document->isValid());

    const QList<Documentation> documentation = CMakeLang::documentation(document);
    QCOMPARE(documentation.size(), 2);

    QCOMPARE(documentation.at(0).name, "FindPackageMessage");
    QCOMPARE(documentation.at(0).kind, Documentation::Module);

    QCOMPARE(documentation.at(1).name, "find_package_message");
    QCOMPARE(documentation.at(1).kind, Documentation::Command);
    QCOMPARE(documentation.at(1).line, 7);
    QCOMPARE(documentation.at(1).signatures(),
             QStringList("find_package_message(<PackageName> <message> <details>)"));
    QCOMPARE(documentation.at(1).brief(),
             "### find_package_message\n"
             "\n"
             "Prints a message:\n"
             "\n"
             "```cmake\n"
             "find_package_message(<PackageName> <message> <details>)\n"
             "```");
}

// A file of the Help of CMake names a module with a directive of its own,
// and what the ".rst:" comments of that module carry stands in its place.
static RstLang::ParseOptions cmakeHelpOptions(
    const std::function<std::optional<QString>(const QString &)> &read)
{
    RstLang::ParseOptions options;
    options.includeDirectives << "cmake-module";
    options.resolveInclude =
        [read](const QString &type, const QString &included) -> std::optional<QString> {
        const std::optional<QString> source = read(included);
        if (!source || type != "cmake-module")
            return source;

        QStringList blocks;
        for (const DocComment &comment : CMakeLang::documentationComments(*source))
            blocks.append(comment.text);
        return blocks.join(QLatin1Char(10));
    };
    return options;
}

// The page the Help of CMake gives a module is one line that names the
// module file, so what is read of the command a module provides is what the
// module itself spells out.
void tst_CMakeLang::documentationOfIncludedModules()
{
    const QString page = ".. cmake-module:: FindPackageMessage.cmake\n";

    const auto documentationOf = [&page](const QString &module,
                                         const QString &name) -> Documentation {
        const RstLang::ParseOptions options = cmakeHelpOptions(
            [&module](const QString &) { return std::optional<QString>(module); });
        return documentationFor(RstLang::Document::fromSource(page, options),
                                name,
                                Documentation::Command);
    };

    // A module that says what the command it provides is called documents
    // it, and the page of the module carries that documentation.
    const QString declares = R"(#[=[.rst:
FindPackageMessage
------------------

This module provides a command.

.. command:: find_package_message

  Prints a message once for each find result:

  .. code-block:: cmake

    find_package_message(<PackageName> <message> <details>)
#]=]

function(find_package_message pkg msg details)
endfunction()
)";

    const Documentation declared = documentationOf(declares, "find_package_message");
    QVERIFY2(declared.brief().startsWith("### find_package_message"),
             qPrintable(declared.brief()));
    QCOMPARE(declared.signatures(),
             QStringList("find_package_message(<PackageName> <message> <details>)"));

    // A module that only shows the command in use names it nowhere, so
    // there is nothing of the command to read: what comes back is what the
    // module is about.  A call that passes a value is no signature of it.
    const QString shows = R"(#[=[.rst:
FindPackageMessage
------------------

.. code-block:: cmake

  find_package_message(<name> "message for user" "find result details")

This function is intended to be used in FindXXX.cmake module files.

Example:

.. code-block:: cmake

  if(X11_FOUND)
    find_package_message(X11 "Found X11: ${X11_X11_LIB}" "[${X11_X11_LIB}]")
  endif()
#]=]

function(find_package_message pkg msg details)
endfunction()
)";

    const Documentation shown = documentationOf(shows, "find_package_message");
    QVERIFY2(shown.brief().startsWith("### FindPackageMessage"),
             qPrintable(shown.brief()));
    QCOMPARE(shown.signatures(), QStringList());

    // What the page says of the module is the page, not the block that
    // documents the command standing in it.
    const Documentation module = documentationOf(declares, "FindPackageMessage");
    QVERIFY2(module.brief().startsWith("### FindPackageMessage"),
             qPrintable(module.brief()));
    QVERIFY(module.signatures().isEmpty());
}

// The modules CMake ships, which is where the documentation of the commands
// they provide is written.  Point QTC_TEST_CMAKE_MODULES_DIR at the
// "Modules" directory of an installation to run this over all of them.
void tst_CMakeLang::documentationOfCMakeModules()
{
    const QString root = qEnvironmentVariable("QTC_TEST_CMAKE_MODULES_DIR");
    if (root.isEmpty())
        QSKIP("QTC_TEST_CMAKE_MODULES_DIR is not set.");

    const QFileInfoList files = QDir(root).entryInfoList({"*.cmake"}, QDir::Files);
    QVERIFY(!files.isEmpty());

    QSet<QString> names;
    QSet<QString> scanned;
    qint64 bytes = 0;
    QElapsedTimer timer;
    timer.start();

    for (const QFileInfo &info : files) {
        QFile file(info.absoluteFilePath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QString source = QString::fromUtf8(file.readAll());
        bytes += source.size();

        // Whatever the scan of the comments finds is a name that reading
        // them declares.
        const QStringList commands = CMakeLang::documentedCommands(source);
        scanned.unite(Utils::toSet(commands));

        const DocumentPtr document = Document::fromSource(source);
        for (const Documentation &documentation : CMakeLang::documentation(document)) {
            QVERIFY2(!documentation.name.isEmpty(), qPrintable(info.fileName()));
            QVERIFY2(documentation.rst->isValid(),
                     qPrintable(info.fileName() + ": " + documentation.rst->errorString()));
            names.insert(documentation.name);

            // A signature stands for any call of the command, so what a
            // module shows by example is none: an example passes a value
            // where a signature names the argument it stands for.
            for (const QString &signature : documentation.signatures()) {
                QVERIFY2(!signature.contains('"') && !signature.contains("${"),
                         qPrintable(info.fileName() + ": " + signature));
            }
        }
    }

    QVERIFY(!names.isEmpty());
    QVERIFY(!scanned.isEmpty());

    // The names the editor indexes the modules by are scanned out of their
    // comments, and a command CMake documents is documented in one of them.
    QVERIFY(scanned.contains("check_cxx_source_compiles"));
    QVERIFY(scanned.contains("FetchContent_Declare"));
    QVERIFY(scanned.contains("ExternalProject_Add"));

    // The name is the one the documentation spells out.  A module may shout
    // the definition of what it provides, and a name that is written in
    // camel case keeps it.
    QVERIFY(names.contains("check_cxx_source_compiles"));
    QVERIFY(!names.contains("CHECK_CXX_SOURCE_COMPILES"));
    QVERIFY(names.contains("FetchContent_Declare"));
    QVERIFY(names.contains("FetchContent_MakeAvailable"));
    QVERIFY(names.contains("ExternalProject_Add"));

    for (const QString &command : scanned)
        QVERIFY2(names.contains(command), qPrintable(command));

    qInfo("read %lld bytes of %lld files in %lld ms, %lld names documented",
          bytes, qint64(files.size()), timer.elapsed(), qint64(names.size()));
}

// The Help of CMake, read the way the editor reads it: a file of it names
// another with an include, and the documentation of a module stands in the
// module itself.  Point QTC_TEST_CMAKE_HELP_DIR at the "Help" directory of
// an installation to run this.
void tst_CMakeLang::documentationOfCMakeHelp()
{
    const QString root = qEnvironmentVariable("QTC_TEST_CMAKE_HELP_DIR");
    if (root.isEmpty())
        QSKIP("QTC_TEST_CMAKE_HELP_DIR is not set.");

    const auto read = [](const QString &path) -> std::optional<QString> {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return QString::fromUtf8(file.readAll());
    };

    const auto documentationOf = [&](const QString &path,
                                     const QString &name) -> Documentation {
        const QDir directory = QFileInfo(path).dir();
        const RstLang::ParseOptions options = cmakeHelpOptions(
            [&read, directory](const QString &included) {
                return read(directory.filePath(included));
            });

        const std::optional<QString> source = read(path);
        if (!source)
            return {};
        return documentationFor(RstLang::Document::fromSource(*source, options),
                                name,
                                Documentation::Command);
    };

    // A command of CMake itself.  What the Help says of it is rewritten
    // from one version of CMake to the next, so what is asserted here is
    // the shape of what is read rather than the wording of it.
    const Documentation addExecutable
        = documentationOf(root + "/command/add_executable.rst", "add_executable");
    const QString addExecutableBrief = addExecutable.brief();
    QVERIFY2(addExecutableBrief.startsWith("### add_executable"),
             qPrintable(addExecutableBrief));
    // Under the name stands what the command is for, and under that the
    // call it takes.
    QVERIFY2(addExecutableBrief.contains("```cmake"), qPrintable(addExecutableBrief));
    const QStringList addExecutableSignatures = addExecutable.signatures();
    QVERIFY(!addExecutableSignatures.isEmpty());
    QVERIFY2(addExecutableSignatures.first().startsWith("add_executable("),
             qPrintable(addExecutableSignatures.first()));

    // A command whose documentation stands in a file of its own, behind an
    // include, and that names itself through a substitution.
    const Documentation findFile = documentationOf(root + "/command/find_file.rst",
                                                   "find_file");
    QVERIFY(findFile.brief().startsWith("### find_file"));
    const QStringList findFileSignatures = findFile.signatures();
    QVERIFY(!findFileSignatures.isEmpty());
    QVERIFY2(findFileSignatures.first().startsWith("find_file"),
             qPrintable(findFileSignatures.first()));
    QVERIFY(!findFile.arguments().isEmpty());

    // A module, whose documentation stands in the module itself rather than
    // in the page that names it.  Which of the commands a module provides it
    // documents is a thing every version of CMake decides anew, so what
    // such a page gives is asserted in documentationOfIncludedModules().
    const Documentation module = documentationOf(root + "/module/FindPackageMessage.rst",
                                                 "FindPackageMessage");
    QVERIFY2(module.brief().startsWith("### FindPackageMessage"),
             qPrintable(module.brief()));

    // The synopsis of a command is written as a parsed literal: it stands
    // the way it is written, and the markup it carries is markup.
    const Documentation cmakePath = documentationOf(root + "/command/cmake_path.rst",
                                                    "cmake_path");
    const QString synopsis = cmakePath.brief();
    QVERIFY2(synopsis.contains("cmake_path(GET <path-var> ROOT_NAME <out-var>)"),
             qPrintable(synopsis));
    QVERIFY2(!synopsis.contains("`_"), qPrintable(synopsis));

    for (const QString &signature : cmakePath.signatures())
        QVERIFY2(!signature.contains("`_"), qPrintable(signature));

    // A command of many modes documents one signature per mode, and the
    // keyword that opens a mode is an argument of the command.
    const Documentation file = documentationOf(root + "/command/file.rst", "file");
    QVERIFY(file.signatures().size() > 20);

    QHash<QString, QString> modes;
    for (const ArgumentDoc &argument : file.arguments())
        modes.insert(argument.name, argument.documentation);

    QVERIFY2(modes.contains("WRITE"), qPrintable(QStringList(modes.keys()).join(' ')));
    const QString write = modes.value("WRITE");
    QVERIFY2(write.startsWith("```cmake"), qPrintable(write));
    QVERIFY2(write.contains("file(WRITE <filename> <content>...)"), qPrintable(write));
    // What the mode means stands under the call that opens it.
    const QString writeMeans = write.section("```", 2).trimmed();
    QVERIFY2(!writeMeans.isEmpty(), qPrintable(write));

    // Two modes that share what is said about them each get to say it.
    const QString append = modes.value("APPEND");
    QVERIFY2(append.contains("file(APPEND <filename> <content>...)"), qPrintable(append));
    QCOMPARE(append.section("```", 2).trimmed(), writeMeans);

    // What the mode is called is not part of the call it opens.
    QVERIFY(!modes.contains("<HASH>"));

    // A call that is too long for one line carries on over the lines below
    // it, and still is one call.
    QVERIFY2(modes.contains("COPY_FILE"), qPrintable(QStringList(modes.keys()).join(' ')));
    const QStringList copyFile = Utils::filtered(file.signatures(), [](const QString &call) {
        return call.startsWith("file(COPY_FILE");
    });
    QCOMPARE(copyFile.size(), 1);
    QVERIFY2(copyFile.first().contains("[RESULT <result>]"), qPrintable(copyFile.first()));

    // A keyword that takes a value is documented with the value behind it,
    // which is not what the reader writes.
    QVERIFY(Utils::anyOf(modes.keys(), [](const QString &name) {
        return name.startsWith("LIMIT_INPUT ");
    }));
}

// CMake reads the name of a command without regard to its case: a module
// documents "check_cxx_source_compiles" and defines
// "CHECK_CXX_SOURCE_COMPILES", and both name the same macro.
void tst_CMakeLang::documentationIgnoresCase()
{
    const QString source = R"(#[[.rst:
CheckCXXSourceCompiles
----------------------

.. command:: check_cxx_source_compiles

  Check once whether code can be built:

  .. code-block:: cmake

    check_cxx_source_compiles(<code> <resultVar>)
#]]

macro(CHECK_CXX_SOURCE_COMPILES SOURCE VAR)
endmacro()
)";

    const DocumentPtr document = Document::fromSource(source);
    QVERIFY(document->isValid());

    const QList<Documentation> documentation = CMakeLang::documentation(document);
    QCOMPARE(documentation.size(), 2);

    // The name is the one the documentation spells, not the one the
    // definition shouts.
    const Documentation &command = documentation.at(1);
    QCOMPARE(command.name, "check_cxx_source_compiles");
    QCOMPARE(command.kind, Documentation::Command);

    // Whichever way the reader writes it, it is the same command.
    QVERIFY(command.isNamed("check_cxx_source_compiles"));
    QVERIFY(command.isNamed("CHECK_CXX_SOURCE_COMPILES"));
    QVERIFY(command.isNamed("Check_CXX_Source_Compiles"));
    QVERIFY(!command.isNamed("check_c_source_compiles"));
    QVERIFY(!command.isNamed(""));

    // The signature is found however the call in the documentation is
    // written.
    QCOMPARE(command.signatures(), QStringList("check_cxx_source_compiles(<code> <resultVar>)"));

    // A comment that documents what it stands in front of documents it
    // whichever way the definition spells the name.
    const QString shouted = R"(#[[.rst:
my_helper
---------

Does a thing.
#]]
macro(MY_HELPER target)
endmacro()
)";

    const QList<Documentation> helper = CMakeLang::documentation(
        Document::fromSource(shouted));
    QCOMPARE(helper.size(), 1);
    QCOMPARE(helper.at(0).name, "my_helper");
    QCOMPARE(helper.at(0).kind, Documentation::Command);

    // The documentation of a command whose name is written in camel case
    // keeps it, which is the way its module spells it.
    const QString camel = R"(#[[.rst:
.. command:: ExternalProject_Add

  Adds a project:

  .. code-block:: cmake

    ExternalProject_Add(<name> [<option>...])
#]]
function(externalproject_add name)
endfunction()
)";

    const QList<Documentation> external = CMakeLang::documentation(
        Document::fromSource(camel));
    QCOMPARE(external.size(), 1);
    QCOMPARE(external.at(0).name, "ExternalProject_Add");
    QVERIFY(external.at(0).isNamed("externalproject_add"));
    QCOMPARE(external.at(0).signatures(),
             QStringList("ExternalProject_Add(<name> [<option>...])"));

    QVERIFY(isSameCommand("file", "FILE"));
    QVERIFY(!isSameCommand("file", "files"));
}

// What the documentation of a command shows by example is not the way the
// command is called: an example passes arguments rather than standing for
// them, and what stands under a heading or behind a paragraph that
// announces an example shows a use of the command.
void tst_CMakeLang::documentationTellsExamplesApart()
{
    const QString numbered = R"(#[[.rst:
FeatureSummary
--------------

.. command:: feature_summary

  Logs what was found:

  .. code-block:: cmake

    feature_summary(WHAT <what> [FILENAME <file>])

  Example 1, append everything to a log file:

  .. code-block:: cmake

    feature_summary(WHAT ALL FILENAME ${CMAKE_BINARY_DIR}/all.log APPEND)

  Example 2, tell what was found:

  .. code-block:: cmake

    feature_summary(WHAT PACKAGES_FOUND DESCRIPTION "Build tools found:")
#]]
)";

    const QList<Documentation> feature = CMakeLang::documentation(
        Document::fromSource(numbered));
    QCOMPARE(feature.size(), 2);
    QCOMPARE(feature.at(1).name, "feature_summary");
    QCOMPARE(feature.at(1).signatures(),
             QStringList("feature_summary(WHAT <what> [FILENAME <file>])"));

    // A heading that announces an example says of every block under it
    // that it shows one, whatever the call in it looks like.
    const QString heading = R"(#[[.rst:
my_module
---------

Sets a target up:

.. code-block:: cmake

  my_module(<target>)

Example
^^^^^^^

.. code-block:: cmake

  my_module(app)
#]]
)";

    const QList<Documentation> module = CMakeLang::documentation(
        Document::fromSource(heading));
    QCOMPARE(module.size(), 1);
    QCOMPARE(module.at(0).name, "my_module");
    QCOMPARE(module.at(0).signatures(), QStringList("my_module(<target>)"));

    // So does the paragraph that opens the block.
    const QString paragraph = R"(#[[.rst:
my_helper
---------

Does a thing::

  my_helper(<target>)

For example::

  my_helper(app)
#]]
)";

    const QList<Documentation> helper = CMakeLang::documentation(
        Document::fromSource(paragraph));
    QCOMPARE(helper.size(), 1);
    QCOMPARE(helper.at(0).signatures(), QStringList("my_helper(<target>)"));
}

// A term of a definition list may spell out several arguments that mean the
// same, and each of them is one the reader may write.
void tst_CMakeLang::documentationOfSeveralArguments()
{
    const QString source = R"(#[[.rst:
.. command:: copy_it

  Copies a thing:

  .. code-block:: cmake

    copy_it(<from> <to> [PERMISSIONS <permissions>...])

  ``PERMISSIONS`` and ``FILE_PERMISSIONS``
    What the copy may be used for.

  ``NO_SOURCE_PERMISSIONS``
    Nothing of the original carries over.

  :variable:`CMAKE_INSTALL_MODE`
    A term reads as code where a role makes it one, and the name is what
    it says, not the markup around it.

  Whatever is left over
    A term that names no argument of its own names one all the same.
#]]
function(copy_it from to)
endfunction()
)";

    const DocumentPtr document = Document::fromSource(source);
    QVERIFY(document->isValid());

    const QList<Documentation> documentation = CMakeLang::documentation(document);
    QCOMPARE(documentation.size(), 1);

    QStringList names;
    QHash<QString, QString> text;
    for (const ArgumentDoc &argument : documentation.at(0).arguments()) {
        names.append(argument.name);
        text.insert(argument.name, argument.documentation);
    }

    QCOMPARE(names,
             QStringList({"PERMISSIONS",
                          "FILE_PERMISSIONS",
                          "NO_SOURCE_PERMISSIONS",
                          "CMAKE_INSTALL_MODE",
                          "Whatever is left over"}));

    // What is said of the term is said of each argument it names.
    QCOMPARE(text.value("PERMISSIONS"), "What the copy may be used for.");
    QCOMPARE(text.value("FILE_PERMISSIONS"), text.value("PERMISSIONS"));
}

// A command that hands its arguments on to more than one takes what each of
// them says about them, and what it says of them itself comes first.
void tst_CMakeLang::argumentsOfSeveralCommands()
{
    const QString source = R"(#[[.rst:
.. command:: extend_plugin

  ``PLUGIN_DEPENDS``
    The plugins it needs.
#]]
function(extend_plugin)
endfunction()

#[[.rst:
.. command:: extend_target

  ``SOURCES``
    The files to build.

  ``PLUGIN_DEPENDS``
    Said again, by the command the arguments were handed to.
#]]
function(extend_target)
endfunction()

#[[.rst:
.. command:: add_library

  ``DEFINES``
    What to build them with.
#]]
function(add_library)
endfunction()
)";

    const QList<Documentation> documentation = CMakeLang::documentation(
        Document::fromSource(source));
    QCOMPARE(documentation.size(), 3);

    QHash<QString, QList<ArgumentDoc>> arguments;
    for (const Documentation &one : documentation)
        arguments.insert(one.name, one.arguments());

    // Every command along the chain says what it takes, and none of them is
    // dropped for another having spoken.
    QList<ArgumentDoc> merged = arguments.value("extend_plugin");
    merged = mergedArguments(merged, arguments.value("extend_target"));
    merged = mergedArguments(merged, arguments.value("add_library"));

    QStringList names;
    for (const ArgumentDoc &argument : std::as_const(merged))
        names.append(argument.name);
    QCOMPARE(names, QStringList({"PLUGIN_DEPENDS", "SOURCES", "DEFINES"}));

    // What was said first about an argument is what it means.
    QCOMPARE(merged.at(0).documentation, "The plugins it needs.");

    // Nothing to add leaves them as they were, and added to nothing they are
    // all of them.
    QCOMPARE(mergedArguments(merged, {}), merged);
    QCOMPARE(mergedArguments({}, merged), merged);
}

QTEST_GUILESS_MAIN(tst_CMakeLang)

#include "tst_cmakelang.moc"
