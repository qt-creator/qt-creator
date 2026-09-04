// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <cmakelang/cmakeast.h>
#include <cmakelang/cmakeastvisitor.h>
#include <cmakelang/cmakedocument.h>
#include <cmakelang/cmakeengine.h>
#include <cmakelang/cmakelexer.h>
#include <cmakelang/cmakeparser.h>
#include <cmakelang/cmakerewriter.h>
#include <cmakelang/cmakesignature.h>

#include <QTest>

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
    void argumentGroups_data();
    void argumentGroups();
    void rewriterReplacesValues();
    void rewriterRemovesArguments();
    void rewriterInsertsValues();
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

QTEST_GUILESS_MAIN(tst_CMakeLang)

#include "tst_cmakelang.moc"
