// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <rstlang/rstast.h>
#include <rstlang/rstastvisitor.h>
#include <rstlang/rstdocument.h>
#include <rstlang/rstlexer.h>
#include <rstlang/rstmarkdown.h>
#include <rstlang/rstparser.h>

#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QTest>

using namespace RstLang;

static QString lex(const QString &source)
{
    Engine engine;
    Lexer lexer(&engine, engine.setSource(source));
    QStringList result;
    Token token;
    while (lexer.yylex(&token) != Parser::EOF_SYMBOL) {
        QString line = QLatin1String(Lexer::name(token.kind));
        if (!token.name.isEmpty())
            line += " {" + token.name.toString() + '}';
        if (!token.text.isEmpty())
            line += ' ' + token.text.toString();
        result << line;
    }
    return result.join(u'\n');
}

namespace {

class Dumper: public Visitor
{
public:
    QString result;

    bool visit(ParagraphAST *ast) override
    {
        add("(p " + ast->text() + ')');
        return false;
    }

    bool visit(LiteralBlockAST *ast) override
    {
        add("(literal " + QString(ast->block()).replace(u'\n', QLatin1String("\\n")) + ')');
        return false;
    }

    bool visit(LineBlockAST *ast) override
    {
        add("(lineblock " + ast->text() + ')');
        return false;
    }

    bool visit(SectionAST *ast) override
    {
        return open("section " + QString::number(ast->level()) + ' ' + ast->title().toString());
    }
    void endVisit(SectionAST *) override { close(); }

    bool visit(BlockQuoteAST *) override { return open("quote"); }
    void endVisit(BlockQuoteAST *) override { close(); }

    bool visit(DefinitionItemAST *ast) override
    {
        open("def " + ast->term());
        AST::accept(ast->blockList, this);
        close();
        return false;
    }

    bool visit(BulletItemAST *ast) override
    {
        return open("item " + ast->marker().toString());
    }
    void endVisit(BulletItemAST *) override { close(); }

    bool visit(EnumeratedItemAST *ast) override
    {
        return open("enum " + ast->marker().toString());
    }
    void endVisit(EnumeratedItemAST *) override { close(); }

    bool visit(FieldAST *ast) override
    {
        return open("field " + ast->fieldName().toString());
    }
    void endVisit(FieldAST *) override { close(); }

    bool visit(DirectiveAST *ast) override
    {
        QString name = "directive " + ast->type().toString();
        if (!ast->argument().isEmpty())
            name += ' ' + ast->argument().toString();
        return open(name);
    }
    void endVisit(DirectiveAST *) override { close(); }

    bool visit(SubstitutionAST *ast) override
    {
        return open("substitution " + ast->substitutionName().toString() + ' '
                    + ast->directiveType().toString() + ' ' + ast->argument().toString());
    }
    void endVisit(SubstitutionAST *) override { close(); }

    bool visit(TargetAST *ast) override
    {
        add("(target " + ast->targetName().toString() + ' ' + ast->link().toString() + ')');
        return false;
    }

    bool visit(CommentAST *) override
    {
        add("(comment)");
        return false;
    }

    bool visit(TransitionAST *) override
    {
        add("(transition)");
        return false;
    }

    bool visit(FootnoteAST *ast) override
    {
        return open("footnote " + ast->label().toString());
    }
    void endVisit(FootnoteAST *) override { close(); }

    bool visit(TableAST *ast) override
    {
        int headerRows = 0;
        const QList<QStringList> rows = ast->rows(&headerRows);
        open("table " + QString::number(headerRows));
        for (const QStringList &row : rows)
            add('(' + row.join(u'|') + ')');
        close();
        return false;
    }

private:
    bool open(const QString &name)
    {
        separate();
        result += '(' + name.trimmed();
        return true;
    }

    void close() { result += ')'; }

    void add(const QString &text)
    {
        separate();
        result += text;
    }

    void separate()
    {
        if (!result.isEmpty() && !result.endsWith(u'('))
            result += u' ';
    }
};

QString dump(const DocumentPtr &document)
{
    Dumper dumper;
    dumper.accept(document->ast());
    return dumper.result;
}

} // namespace

class tst_RstLang: public QObject
{
    Q_OBJECT

private slots:
    void lexer_data();
    void lexer();

    void parser_data();
    void parser();

    void sections();
    void includes();
    void includeCycles();
    void substitutions();
    void diagnostics();

    void markdown_data();
    void markdown();

    void inlineMarkup_data();
    void inlineMarkup();
    void parsedLiterals();

    void cmakeModule();
    void cmakeHelpPages_data();
    void cmakeHelpPages();
    void cmakeHelp();
};

void tst_RstLang::lexer_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << QString() << QString();

    QTest::newRow("paragraph") << "one\ntwo\n"
                               << "text one\n"
                                  "text two\n"
                                  "blank line";

    QTest::newRow("two paragraphs") << "one\n\ntwo\n"
                                    << "text one\n"
                                       "blank line\n"
                                       "text two\n"
                                       "blank line";

    QTest::newRow("title") << "Title\n-----\n\ntext\n"
                           << "section title Title\n"
                              "section start\n"
                              "text text\n"
                              "blank line\n"
                              "section end";

    // What is indented under a paragraph with no blank line in between says
    // what the paragraph names.
    QTest::newRow("definition") << "``FOO``\n  The option.\n"
                                << "text ``FOO``\n"
                                   "indent\n"
                                   "text The option.\n"
                                   "blank line\n"
                                   "dedent";

    // With a blank line in between it is a block quote instead.
    QTest::newRow("block quote") << "text\n\n  quoted\n"
                                 << "text text\n"
                                    "blank line\n"
                                    "indent\n"
                                    "text quoted\n"
                                    "blank line\n"
                                    "dedent";

    QTest::newRow("literal block") << "Example::\n\n   code()\n\nafter\n"
                                   << "text Example::\n"
                                      "blank line\n"
                                      "verbatim text code()\n"
                                      "blank line\n"
                                      "text after\n"
                                      "blank line";

    QTest::newRow("directive") << ".. note:: Careful.\n"
                               << "directive {note} Careful.\n"
                                  "blank line";

    QTest::newRow("directive with body") << ".. note::\n\n   Careful.\n"
                                         << "directive {note}\n"
                                            "indent\n"
                                            "text Careful.\n"
                                            "blank line\n"
                                            "dedent";

    QTest::newRow("code block") << ".. code-block:: cmake\n\n  set(x 1)\n"
                                << "directive {code-block} cmake\n"
                                   "indent\n"
                                   "verbatim text set(x 1)\n"
                                   "blank line\n"
                                   "dedent";

    // The fields that configure a directive come before what it holds, even
    // where the body of it is taken verbatim.
    QTest::newRow("code block with option")
        << ".. code-block:: cmake\n  :caption: Foo.cmake\n\n  set(x 1)\n"
        << "directive {code-block} cmake\n"
           "indent\n"
           "field name {caption} Foo.cmake\n"
           "indent\n"
           "text Foo.cmake\n"
           "blank line\n"
           "dedent\n"
           "verbatim text set(x 1)\n"
           "blank line\n"
           "dedent";

    QTest::newRow("bullet list") << "- one\n- two\n"
                                 << "bullet {-}\n"
                                    "indent\n"
                                    "text one\n"
                                    "blank line\n"
                                    "dedent\n"
                                    "bullet {-}\n"
                                    "indent\n"
                                    "text two\n"
                                    "blank line\n"
                                    "dedent";

    QTest::newRow("substitutions") << ".. |A| replace:: a\n.. |B| replace:: b\n"
                                   << "substitution definition {A} replace:: a\n"
                                      "blank line\n"
                                      "substitution definition {B} replace:: b\n"
                                      "blank line";

    QTest::newRow("target") << ".. _the name: https://example.com\n"
                            << "hyperlink target {the name} https://example.com\n"
                               "blank line";

    // A comment owns the lines indented under it, markup or not.
    QTest::newRow("comment") << ".. a comment\n   .. and more\n\ntext\n"
                             << "comment a comment\n   .. and more\n"
                                "blank line\n"
                                "text text\n"
                                "blank line";

    QTest::newRow("footnote") << ".. [#note] The text\n"
                              << "footnote {#note} The text\n"
                                 "indent\n"
                                 "text The text\n"
                                 "blank line\n"
                                 "dedent";

    // The body of a footnote stands at the column behind the "..", so what is
    // written on the line of the label opens it.
    QTest::newRow("footnote with directive")
        << ".. [#note] .. versionadded:: 3.25\n"
        << "footnote {#note} .. versionadded:: 3.25\n"
           "indent\n"
           "directive {versionadded} 3.25\n"
           "blank line\n"
           "dedent";

    // A table is one token: what stands in its cells is a layout, not a
    // nesting the grammar could read.
    QTest::newRow("simple table") << "==  ==\na   b\n==  ==\n"
                                  << "table\n"
                                     "blank line";

    QTest::newRow("grid table") << "+--+--+\n|a |b |\n+--+--+\n"
                                << "table\n"
                                   "blank line";

    // One column of "=" underlines a title, and draws no table.
    QTest::newRow("title over table") << "Title\n=====\n"
                                      << "section title Title\n"
                                         "section start\n"
                                         "section end";
}

void tst_RstLang::lexer()
{
    QFETCH(QString, source);
    QFETCH(QString, expected);

    QCOMPARE(lex(source), expected);
}

void tst_RstLang::parser_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << QString() << QString();

    QTest::newRow("paragraph") << "one\ntwo\n" << "(p one two)";

    QTest::newRow("two paragraphs") << "one\n\ntwo\n" << "(p one) (p two)";

    QTest::newRow("section") << "Title\n-----\n\ntext\n" << "(section 1 Title (p text))";

    QTest::newRow("definition") << "``FOO``\n  The option.\n"
                                << "(def ``FOO`` (p The option.))";

    QTest::newRow("block quote") << "text\n\n  quoted\n" << "(p text) (quote (p quoted))";

    QTest::newRow("literal block") << "Example::\n\n   a()\n   b()\n"
                                   << "(p Example::) (literal a()\\nb())";

    QTest::newRow("code block") << ".. code-block:: cmake\n\n  set(x 1)\n"
                                << "(directive code-block cmake (literal set(x 1)))";

    QTest::newRow("nested code") << ".. code-block:: cmake\n\n  if(x)\n    set(y 1)\n  endif()\n"
                                 << "(directive code-block cmake "
                                    "(literal if(x)\\n  set(y 1)\\nendif()))";

    QTest::newRow("bullet list") << "- one\n- two\n" << "(item - (p one)) (item - (p two))";

    QTest::newRow("enumerated list") << "1. one\n2. two\n"
                                     << "(enum 1. (p one)) (enum 2. (p two))";

    QTest::newRow("field list") << ":name: value\n" << "(field name (p value))";

    QTest::newRow("transition") << "one\n\n----\n\ntwo\n" << "(p one) (transition) (p two)";

    QTest::newRow("line block") << "| one\n| two\n" << "(lineblock one two)";

    QTest::newRow("target") << ".. _name: link\n" << "(target name link)";

    QTest::newRow("comment") << ".. nothing to see\n" << "(comment)";

    QTest::newRow("footnote") << ".. [#p1] The note\n" << "(footnote #p1 (p The note))";

    // What stands behind the label is read as a line of the body and not as
    // text of its own, so a directive written there is a directive.
    QTest::newRow("footnote with directive")
        << ".. [#p1] .. versionadded:: 3.25\n"
        << "(footnote #p1 (directive versionadded 3.25))";

    // A citation is written the way a footnote is, and what is indented under
    // it belongs to it.
    QTest::newRow("citation") << ".. [CIT] A citation\n\n   More of it\n"
                              << "(footnote CIT (p A citation) (p More of it))";

    // The rule below the heading of a simple table is the second one it
    // draws, so a table of two rules has no heading.
    QTest::newRow("simple table")
        << "=====  =====\nName   Value\n=====  =====\na      1\nb      2\n=====  =====\n"
        << "(table 1 (Name|Value) (a|1) (b|2))";

    QTest::newRow("table without heading") << "==  ==\na   b\n==  ==\n" << "(table 0 (a|b))";

    // A cell that is written over several lines reads as one text.
    QTest::newRow("wrapped cell")
        << "=====  =====\na      one\n       more\n=====  =====\n"
        << "(table 0 (a|one more))";

    QTest::newRow("grid table")
        << "+---+---+\n| a | b |\n+===+===+\n| c | d |\n+---+---+\n"
        << "(table 1 (a|b) (c|d))";

    // A rule that nothing closes opens no table, so its lines stay prose.
    QTest::newRow("unclosed table") << "==  ==\na   b\n" << "(p ==  == a   b)";

    // A directive holds what is indented under it, whatever that is.
    QTest::newRow("nested blocks")
        << ".. command:: foo\n\n  Does a thing.\n\n  ``ARG``\n    An argument.\n"
        << "(directive command foo (p Does a thing.) (def ``ARG`` (p An argument.)))";
}

void tst_RstLang::parser()
{
    QFETCH(QString, source);
    QFETCH(QString, expected);

    const DocumentPtr document = Document::fromSource(source);
    QVERIFY2(document->isValid(), qPrintable(document->errorString()));
    QCOMPARE(dump(document), expected);
}

void tst_RstLang::sections()
{
    // The character a title is underlined with says how deep the section
    // sits: the order the document introduces them in is their order.
    const QString source = R"(Top
===

A
-

B
~

C
-

D
===
)";

    const DocumentPtr document = Document::fromSource(source);
    QVERIFY2(document->isValid(), qPrintable(document->errorString()));
    QCOMPARE(dump(document),
             "(section 1 Top (section 2 A (section 3 B)) (section 2 C)) (section 1 D)");
    QCOMPARE(document->sections().size(), 5);
    QCOMPARE(document->title(), u"Top");
}

void tst_RstLang::includes()
{
    ParseOptions options;
    options.includeDirectives << "cmake-module";
    options.resolveInclude = [](const QString &type,
                                const QString &path) -> std::optional<QString> {
        if (type == "cmake-module" && path == "Module.cmake")
            return QString("From a module");
        if (path == "other.rst")
            return QString("Included\n\n.. include:: nested.rst\n");
        if (path == "nested.rst")
            return QString("Nested");
        return {};
    };

    const DocumentPtr document = Document::fromSource("Before\n\n.. include:: other.rst\n\nAfter\n",
                                                      options);
    QVERIFY2(document->isValid(), qPrintable(document->errorString()));
    QCOMPARE(dump(document), "(p Before) (p Included) (p Nested) (p After)");

    // What an include names inside a literal block is part of the block.
    const DocumentPtr literal
        = Document::fromSource("Example::\n\n   .. include:: other.rst\n", options);
    QCOMPARE(dump(literal), "(p Example::) (literal .. include:: other.rst)");

    // A file that cannot be read leaves the directive standing.
    const DocumentPtr missing = Document::fromSource(".. include:: missing.rst\n", options);
    QCOMPARE(dump(missing), "(directive include missing.rst)");

    // CMake writes the documentation of its modules into the modules
    // themselves and pulls it in with a directive of its own.
    const DocumentPtr module = Document::fromSource(".. cmake-module:: Module.cmake\n", options);
    QCOMPARE(dump(module), "(p From a module)");
}

// A file that is already being read is never read again: the expansion stops
// at the include that closes the ring, and the document says that it did.
void tst_RstLang::includeCycles()
{
    ParseOptions options;
    options.resolveInclude = [](const QString &, const QString &path) -> std::optional<QString> {
        if (path == "a.rst")
            return QString("In A\n\n.. include:: b.rst\n");
        if (path == "b.rst")
            return QString("In B\n\n.. include:: a.rst\n");
        if (path == "self.rst")
            return QString("Self\n\n.. include:: self.rst\n");
        return {};
    };

    const DocumentPtr ring = Document::fromSource(".. include:: a.rst\n", options);
    QCOMPARE(dump(ring), "(p In A) (p In B) (directive include a.rst)");
    QVERIFY(ring->isValid());
    QCOMPARE(ring->diagnostics().size(), 1);
    QCOMPARE(ring->diagnostics().constFirst().message, "Include cycle: a.rst -> b.rst -> a.rst.");

    const DocumentPtr self = Document::fromSource(".. include:: self.rst\n", options);
    QCOMPARE(dump(self), "(p Self) (directive include self.rst)");
    QCOMPARE(self->diagnostics().size(), 1);
    QCOMPARE(self->diagnostics().constFirst().message, "Include cycle: self.rst -> self.rst.");

    // The include that was not followed is where the document says so.
    QCOMPARE(self->diagnostics().constFirst().line, 3);
    QCOMPARE(self->source().sliced(self->diagnostics().constFirst().position),
             u".. include:: self.rst\n");

    // A chain of files that never closes is bounded all the same.
    ParseOptions chain;
    chain.includeLimit = 2;
    chain.resolveInclude = [](const QString &, const QString &path) -> std::optional<QString> {
        const int level = path.first(1).toInt();
        return QString("Level %1\n\n.. include:: %2.rst\n").arg(level).arg(level + 1);
    };
    const DocumentPtr deep = Document::fromSource(".. include:: 1.rst\n", chain);
    QCOMPARE(dump(deep), "(p Level 1) (p Level 2) (directive include 3.rst)");
    QCOMPARE(deep->diagnostics().size(), 1);
    QCOMPARE(deep->diagnostics().constFirst().message, "Includes nest deeper than 2 files.");
}

void tst_RstLang::substitutions()
{
    const QString source = R"(.. |FIND_XXX| replace:: find_file
.. |LONG| replace::
   over two
   lines

Call |FIND_XXX| to do it |LONG|.
)";

    const DocumentPtr document = Document::fromSource(source);
    QVERIFY2(document->isValid(), qPrintable(document->errorString()));
    QCOMPARE(document->substitutions().size(), 2);

    const QHash<QString, QString> texts = document->substitutionTexts();
    QCOMPARE(texts.value("FIND_XXX"), "find_file");
    QCOMPARE(texts.value("LONG"), "over two lines");

    MarkdownOptions options;
    options.substitutions = texts;
    QCOMPARE(inlineToMarkdown(u"Call |FIND_XXX| to do it |LONG|.", options),
             "Call find_file to do it over two lines.");
}

void tst_RstLang::diagnostics()
{
    // Every document of lines is one the grammar takes: the lexer hands out
    // no stream that it cannot read.
    const QStringList sources = {"",
                                 "\n\n\n",
                                 "   indented\n",
                                 "- \n",
                                 "..\n",
                                 ".. \n",
                                 "::\n\n  code\n",
                                 ":field:\n",
                                 "|\n",
                                 "----\n",
                                 "Title\n=====\n=====\n",
                                 "  a\n b\nc\n",
                                 ".. directive::\n.. |x| replace:: y\n"};
    for (const QString &source : sources) {
        const DocumentPtr document = Document::fromSource(source);
        QVERIFY2(document->isValid(),
                 qPrintable(source + " -> " + document->errorString()));
    }
}

void tst_RstLang::markdown_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expected");

    QTest::newRow("paragraph") << "one two\n" << "one two";

    QTest::newRow("section") << "Title\n-----\n\ntext\n" << "### Title\n\ntext";

    QTest::newRow("code block") << ".. code-block:: cmake\n\n  set(x 1)\n"
                                << "```cmake\nset(x 1)\n```";

    QTest::newRow("literal block") << "Example::\n\n   set(x 1)\n"
                                   << "Example:\n\n```\nset(x 1)\n```";

    QTest::newRow("definition") << "``FOO``\n  The option.\n"
                                << "- `FOO`\n  The option.";

    QTest::newRow("note") << ".. note::\n\n  Careful.\n" << "> **Note**\n> Careful.";

    QTest::newRow("versionadded") << ".. versionadded:: 3.20\n" << "> **Added in version 3.20**";

    QTest::newRow("bullets") << "- one\n- two\n" << "- one\n\n- two";

    // What tells the documentation how to build says nothing to the reader.
    QTest::newRow("only") << ".. only:: html\n\n  .. contents::\n\ntext\n" << "text";

    QTest::newRow("command") << ".. command:: foo\n\n  Does a thing.\n"
                             << "#### `foo`\n\nDoes a thing.";

    QTest::newRow("table")
        << "=====  =====\nName   Value\n=====  =====\na      1\n=====  =====\n"
        << "|Name|Value|\n|---|---|\n|a|1|";

    // Markdown asks a table for a heading, so a table that draws none gets an
    // empty one.
    QTest::newRow("table without heading") << "==  ==\na   b\n==  ==\n"
                                           << "|||\n|---|---|\n|a|b|";

    QTest::newRow("footnote") << "Text [#p1]_\n\n.. [#p1] The note\n"
                              << "Text \\[#p1\\]\n\n\\[#p1\\]\nThe note";

    // Three of the four footnotes in the help of CMake have a directive for a
    // body, which reaches the reader only where the label line opens it.
    QTest::newRow("footnote with directive")
        << ".. [#p1] .. versionadded:: 3.25\n"
        << "\\[#p1\\]\n> **Added in version 3.25**";
}

void tst_RstLang::markdown()
{
    QFETCH(QString, source);
    QFETCH(QString, expected);

    const DocumentPtr document = Document::fromSource(source);
    QVERIFY2(document->isValid(), qPrintable(document->errorString()));
    QCOMPARE(toMarkdown(document), expected);
}

void tst_RstLang::inlineMarkup_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expected");

    QTest::newRow("literal") << "the ``code`` here" << "the `code` here";
    QTest::newRow("role") << "see :command:`find_package`" << "see `find_package`";
    QTest::newRow("role with target") << "see :ref:`Find Modules <find-modules>`"
                                      << "see Find Modules";
    QTest::newRow("strong") << "**loud**" << "**loud**";
    QTest::newRow("emphasis") << "*quiet*" << "*quiet*";
    QTest::newRow("reference") << "`the text`_" << "the text";
    // What Markdown reads as markup is written out where the text means it
    // the way it stands.  An underscore inside a word is not markup there.
    QTest::newRow("escaping") << "a_b <c> *d" << "a_b \\<c\\> \\*d";
    QTest::newRow("literal keeps markup") << "``a_b <c>``" << "`a_b <c>`";
}

void tst_RstLang::inlineMarkup()
{
    QFETCH(QString, source);
    QFETCH(QString, expected);

    QCOMPARE(inlineToMarkdown(source), expected);
}

// The way CMake documents the commands its modules define.
void tst_RstLang::cmakeModule()
{
    const QString source = R"(FindPackageMessage
------------------

This module provides a command for printing find result messages.

Commands
^^^^^^^^

.. command:: find_package_message

  Prints a message once for each unique find result:

  .. code-block:: cmake

    find_package_message(<PackageName> <message> <details>)

  ``<PackageName>``
    The name of the package.

  ``<message>``
    The message string to display.
)";

    const DocumentPtr document = Document::fromSource(source);
    QVERIFY2(document->isValid(), qPrintable(document->errorString()));

    QCOMPARE(document->title(), u"FindPackageMessage");
    QCOMPARE(document->sections().size(), 2);

    DirectiveAST *command = document->directive("command", "find_package_message");
    QVERIFY(command);

    // The definition lists of the body say what each argument means.
    QStringList arguments;
    for (BlockAST *block : command->content()) {
        if (DefinitionItemAST *definition = block->asDefinitionItem())
            arguments << definition->term();
    }
    QCOMPARE(arguments, QStringList({"``<PackageName>``", "``<message>``"}));

    // The block of code that comes with the text is the signature of the
    // command, which is what the reader is after.
    QCOMPARE(briefMarkdown(document),
             "### FindPackageMessage\n"
             "\n"
             "This module provides a command for printing find result messages.\n"
             "\n"
             "```cmake\n"
             "find_package_message(<PackageName> <message> <details>)\n"
             "```");

    QCOMPARE(briefMarkdown(command),
             "### find_package_message\n"
             "\n"
             "Prints a message once for each unique find result:\n"
             "\n"
             "```cmake\n"
             "find_package_message(<PackageName> <message> <details>)\n"
             "```");
}

// A parsed literal stands the way it is written, and what it carries is
// markup: this is how CMake writes the synopsis of a command.
void tst_RstLang::parsedLiterals()
{
    const QString source = R"(Synopsis
^^^^^^^^

.. parsed-literal::

  `Decomposition`_
    cmake_path(`GET`_ <path-var> `ROOT_NAME <GET ... ROOT_NAME_>`_ <out-var>)
)";

    const DocumentPtr document = Document::fromSource(source);
    QVERIFY2(document->isValid(), qPrintable(document->errorString()));

    // Nothing of the markup reaches the reader, and the layout of the
    // lines is what the block is written for.
    QCOMPARE(toMarkdown(document),
             "### Synopsis\n"
             "\n"
             "```\n"
             "Decomposition\n"
             "  cmake_path(GET <path-var> ROOT_NAME <out-var>)\n"
             "```");

    // A block of code that is not parsed carries no markup at all.
    const DocumentPtr code
        = Document::fromSource(".. code-block:: cmake\n\n  set(x `a`_)\n");
    QCOMPARE(toMarkdown(code), "```cmake\nset(x `a`_)\n```");

    QCOMPARE(inlineToPlainText(u"see :command:`find_package` and `the text`_"),
             "see find_package and the text");
}

// The shapes of the documentation of CMake that are awkward to read: the
// tables of variable/CMAKE_<LANG>_COMPILER_ID, the footnotes and the grid
// table of command/find_package.  Whether an installation is at hand or not,
// these run wherever the tests do.
void tst_RstLang::cmakeHelpPages_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expected");

    QTest::newRow("compiler id")
        << R"(CMAKE_<LANG>_COMPILER_ID
------------------------

Compiler identification string.

=========================  ==============
Vendor                     Value
=========================  ==============
Absoft Fortran             ``Absoft``
GNU Compiler Collection    ``GNU``
Intel oneAPI Compiler      ``IntelLLVM``
=========================  ==============
)"
        << "### CMAKE\\_\\<LANG\\>\\_COMPILER_ID\n"
           "\n"
           "Compiler identification string.\n"
           "\n"
           "|Vendor|Value|\n"
           "|---|---|\n"
           "|Absoft Fortran|`Absoft`|\n"
           "|GNU Compiler Collection|`GNU`|\n"
           "|Intel oneAPI Compiler|`IntelLLVM`|";

    QTest::newRow("find_package")
        << R"(find_package
------------

Find a package.

+----------------+------------------+
| Mode           | Meaning          |
+================+==================+
| Module [#mod]_ | Reads a find     |
|                | module           |
+----------------+------------------+
| Config [#cfg]_ | Reads a config   |
+----------------+------------------+

.. [#mod] The module mode reads a ``Find<PackageName>.cmake`` file.

.. [#cfg] .. versionadded:: 3.25
)"
        << "### find_package\n"
           "\n"
           "Find a package.\n"
           "\n"
           "|Mode|Meaning|\n"
           "|---|---|\n"
           "|Module \\[#mod\\]|Reads a find module|\n"
           "|Config \\[#cfg\\]|Reads a config|\n"
           "\n"
           "\\[#mod\\]\n"
           "The module mode reads a `Find<PackageName>.cmake` file.\n"
           "\n"
           "\\[#cfg\\]\n"
           "> **Added in version 3.25**";
}

void tst_RstLang::cmakeHelpPages()
{
    QFETCH(QString, source);
    QFETCH(QString, expected);

    const DocumentPtr document = Document::fromSource(source);
    QVERIFY2(document->isValid(), qPrintable(document->errorString()));
    QCOMPARE(toMarkdown(document), expected);
}

// The documentation CMake ships, which is what the editor reads.  Point
// QTC_TEST_CMAKE_HELP_DIR at the "Help" directory of an installation to run
// this over all of it.
void tst_RstLang::cmakeHelp()
{
    const QString root = qEnvironmentVariable("QTC_TEST_CMAKE_HELP_DIR");
    if (root.isEmpty())
        QSKIP("QTC_TEST_CMAKE_HELP_DIR is not set.");

    QDirIterator files(root, {"*.rst"}, QDir::Files, QDirIterator::Subdirectories);
    QStringList paths;
    while (files.hasNext())
        paths.append(files.next());
    QVERIFY(!paths.isEmpty());

    qint64 bytes = 0;
    QElapsedTimer timer;
    timer.start();

    for (const QString &path : paths) {
        // Only the includes are followed here: what a "cmake-module"
        // directive names is a CMake file, and reading one of those is what
        // the CMake language library is for.
        ParseOptions options;
        options.resolveInclude = [&path](const QString &,
                                         const QString &name) -> std::optional<QString> {
            QFile file(QFileInfo(path).dir().filePath(name));
            if (!file.open(QIODevice::ReadOnly))
                return {};
            return QString::fromUtf8(file.readAll());
        };

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QString source = QString::fromUtf8(file.readAll());
        bytes += source.size();

        const DocumentPtr document = Document::fromSource(source, options);
        QVERIFY2(document->isValid(), qPrintable(path + ": " + document->errorString()));

        // Whatever the documentation says, something of it reaches the
        // reader.  What a module says stands in the module itself, so the
        // file that only names one holds nothing of its own.
        QVERIFY2(!briefMarkdown(document).isEmpty() || source.trimmed().isEmpty()
                     || document->directive("cmake-module"),
                 qPrintable(path));
    }

    qInfo("parsed %lld bytes of %lld files in %lld ms",
          bytes, qint64(paths.size()), timer.elapsed());
}

QTEST_GUILESS_MAIN(tst_RstLang)

#include "tst_rstlang.moc"
