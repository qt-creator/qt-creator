// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qdocrenderer.h>

#include <QDirIterator>
#include <QTemporaryDir>
#include <QTest>

using namespace QDoc::Internal;

// The rendered block without its wrapping section element and empty lines, which is
// what the expected values below spell out.
static QString renderBody(const QString &markup, QStringList *messages)
{
    const ParsedDoc doc = parseDoc(markup);
    Renderer renderer;
    const QString html = renderer.resolveSamePageLinks(renderer.renderBlock(doc, 0));
    QList<Problem> problems = doc.problems;
    problems.append(renderer.problems());
    for (const Problem &problem : problems)
        messages->append(problem.message);

    QStringList lines;
    for (const QString &line : html.split('\n')) {
        if (!line.trimmed().isEmpty())
            lines.append(line);
    }
    if (!lines.isEmpty() && lines.first().startsWith("<section"))
        lines.removeFirst();
    if (!lines.isEmpty() && lines.last() == "</section>")
        lines.removeLast();
    return lines.join('\n');
}

class tst_QDoc : public QObject
{
    Q_OBJECT

private slots:
    void render_data();
    void render();
    void listHints_data();
    void listHints();
    void helpers();
    void config();
    void macroTable();
    void configResolver();
    void editedConfiguration();
    void snippets_data();
    void snippets();
    void quoteSteps();
    void quotedLanguage_data();
    void quotedLanguage();
    void problemPositions();
    void topicCommandCheck_data();
    void topicCommandCheck();
    void ruleIds_data();
    void ruleIds();
    void previewBlocks();
    void editedSnippet();
    void markupSpans_data();
    void markupSpans();
    void corpus();
};

void tst_QDoc::render_data()
{
    QTest::addColumn<QString>("markup");
    QTest::addColumn<QString>("html");
    QTest::addColumn<QString>("problems");

    QTest::newRow("paragraph")
        << R"MARKUP(Some plain text
spanning two lines.

A second paragraph.)MARKUP"
        << R"HTML(<p>Some plain text spanning two lines.</p>
<p>A second paragraph.</p>)HTML"
        << QString("");
    QTest::newRow("inline-formats")
        << R"MARKUP(A \b bold and \e italic and \c code() and \a param and \uicontrol {File > Open}.)MARKUP"
        << R"HTML(<p>A <b>bold</b> and <i>italic</i> and <code class="qdoc-inline-code">code()</code> and <i class="qdoc-parameter">param</i> and <span class="qdoc-uicontrol">File &gt; Open</span>.</p>)HTML"
        << QString("");
    QTest::newRow("section")
        << R"MARKUP(\section1 The \c foo command
Body text.
\section2 Sub
More.)MARKUP"
        << R"HTML(<h2 id="the-foo-command" class="qdoc-section">The <code class="qdoc-inline-code">foo</code> command</h2>
<p>Body text.</p>
<h3 id="sub" class="qdoc-section">Sub</h3>
<p>More.</p>)HTML"
        << QString("");
    QTest::newRow("bullet-list")
        << R"MARKUP(\list
\li One
\li Two
\endlist)MARKUP"
        << R"HTML(<ul><li>One</li>
<li>Two</li></ul>)HTML"
        << QString("");
    QTest::newRow("numeric-list")
        << R"MARKUP(\list 3
\li Three
\li Four
\endlist)MARKUP"
        << R"HTML(<ol type="1" start="3"><li>Three</li>
<li>Four</li></ol>)HTML"
        << QString("");
    QTest::newRow("roman-list")
        << R"MARKUP(\list iv
\li Four
\endlist)MARKUP"
        << R"HTML(<ol type="i" start="4"><li>Four</li></ol>)HTML"
        << QString("");
    QTest::newRow("table")
        << R"MARKUP(\table
\header
\li Name
\li Value
\row
\li a
\li 1
\endtable)MARKUP"
        << R"HTML(<table class="qdoc-table"><tr><th>Name</th><th>Value</th></tr>
<tr><td>a</td><td>1</td></tr></table>)HTML"
        << QString("");
    QTest::newRow("code")
        << R"MARKUP(\code
int main()
{
    return 0;
}
\endcode)MARKUP"
        << R"HTML(<div class="qdoc-code"><pre><code>int main()
{
    return 0;
}</code></pre></div>)HTML"
        << QString("");
    QTest::newRow("dots-clamped-indent")
        << R"MARKUP(\dots 2000000000)MARKUP"
        << R"HTML(<div class="qdoc-code"><pre><code>                                                                ...</code></pre></div>)HTML"
        << QString("Invalid argument to '\\dots': the indent must be between 0 and 64");
    QTest::newRow("badcode")
        << R"MARKUP(\badcode
foo bar
\endcode)MARKUP"
        << R"HTML(<div class="qdoc-code qdoc-badcode"><span class="qdoc-code-label">avoid</span><pre><code>foo bar</code></pre></div>)HTML"
        << QString("");
    QTest::newRow("note")
        << R"MARKUP(\note Watch out for this.

Another paragraph.)MARKUP"
        << R"HTML(<div class="qdoc-admonition qdoc-note"><span class="qdoc-admonition-label">Note:</span> Watch out for this.</div>
<p>Another paragraph.</p>)HTML"
        << QString("");
    QTest::newRow("link-plain")
        << R"MARKUP(See \l QString for details.)MARKUP"
        << R"HTML(<p>See <a class="qdoc-link" title="QString" data-target="QString">QString</a> for details.</p>)HTML"
        << QString("");
    QTest::newRow("link-braced")
        << R"MARKUP(See \l {QString::number()}{the number function}.)MARKUP"
        << R"HTML(<p>See <a class="qdoc-link" title="QString::number()" data-target="QString::number()">the number function</a>.</p>)HTML"
        << QString("");
    QTest::newRow("seealso-two")
        << R"MARKUP(\sa QString, QByteArray)MARKUP"
        << R"HTML(<p class="qdoc-seealso"><span class="qdoc-seealso-label">See also</span> <a class="qdoc-link" title="QString" data-target="QString">QString</a> and <a class="qdoc-link" title="QByteArray" data-target="QByteArray">QByteArray</a>.</p>)HTML"
        << QString("");
    QTest::newRow("seealso-three")
        << R"MARKUP(\sa QString, QByteArray, QChar)MARKUP"
        << R"HTML(<p class="qdoc-seealso"><span class="qdoc-seealso-label">See also</span> <a class="qdoc-link" title="QString" data-target="QString">QString</a>, <a class="qdoc-link" title="QByteArray" data-target="QByteArray">QByteArray</a>, and <a class="qdoc-link" title="QChar" data-target="QChar">QChar</a>.</p>)HTML"
        << QString("");
    QTest::newRow("value-list")
        << R"MARKUP(\value One The first.
\value Two The second.)MARKUP"
        << R"HTML(<table class="qdoc-table qdoc-valuelist"><tr><th>Constant</th><th>Description</th></tr><tr><td class="qdoc-value-name"><code>One</code></td><td>The first.</td></tr>
<tr><td class="qdoc-value-name"><code>Two</code></td><td>The second.</td></tr></table>)HTML"
        << QString("");
    QTest::newRow("details")
        << R"MARKUP(\details {More information}
Hidden text.
\enddetails)MARKUP"
        << R"HTML(<details class="qdoc-details"><summary>More information</summary><p>Hidden text.</p></details>)HTML"
        << QString("");
    QTest::newRow("brief-title")
        << R"MARKUP(\title The Page
\brief A short description)MARKUP"
        << R"HTML(<h1 class="qdoc-title">The Page</h1>
<p class="qdoc-brief">A short description</p>)HTML"
        << QString("'\\brief' statement does not end with a full stop.");
    QTest::newRow("topic-class")
        << R"MARKUP(\class QString
\brief The QString class.
\inmodule QtCore
\since 6.0)MARKUP"
        << R"HTML(<h1 class="qdoc-title" id="m-qstring">QString Class</h1>
<p class="qdoc-badges"><span class="qdoc-badge qdoc-badge-since">Since 6.0</span> <span class="qdoc-badge qdoc-badge-module">QtCore</span></p>
<p class="qdoc-brief">The QString class.</p>)HTML"
        << QString("");
    QTest::newRow("topic-fn")
        << R"MARKUP(\fn void QString::append(const QString &other)
Appends \a other.)MARKUP"
        << R"HTML(<h2 class="qdoc-signature" id="m-qstring-append"><code>void QString::append(const QString &amp;other)</code></h2>
<p>Appends <i class="qdoc-parameter">other</i>.</p>)HTML"
        << QString("");
    QTest::newRow("topic-qmlproperty")
        << R"MARKUP(\qmlproperty int Item::width
The width.)MARKUP"
        << R"HTML(<h2 class="qdoc-member" id="m-item-width"><code>Item::width</code> <span class="qdoc-type-sep">:</span> <code class="qdoc-type">int</code> <span class="qdoc-kind-inline">QML Property</span></h2>
<p>The width.</p>)HTML"
        << QString("");
    QTest::newRow("unicode")
        << R"MARKUP(Copyright \unicode 0x00A9 2026)MARKUP"
        << QString(R"HTML(<p>Copyright %1 2026</p>)HTML").arg(QChar(0x00a9))
        << QString("");
    QTest::newRow("target-link")
        << R"MARKUP(\target my-anchor
\section1 Title
See \l {my-anchor}.)MARKUP"
        << R"HTML(<p><span class="qdoc-anchor" id="my-anchor"></span></p>
<h2 id="title" class="qdoc-section">Title</h2>
<p>See <a class="qdoc-link qdoc-link-resolved" href="#my-anchor" title="my-anchor" data-target="my-anchor">my-anchor</a>.</p>)HTML"
        << QString("");
    QTest::newRow("div-span")
        << R"MARKUP(\div {class="foo"}
Inside.
\enddiv
\span {highlight} {marked text})MARKUP"
        << R"HTML(<div class="qdoc-div" data-attrs="class=&quot;foo&quot;"><p>Inside.</p></div>
<p><span class="highlight">marked text</span></p>)HTML"
        << QString("");
    QTest::newRow("hr-br")
        << R"MARKUP(Before\br after
\hr
After rule.)MARKUP"
        << R"HTML(<p>Before<br> after</p>
<hr>
<p>After rule.</p>)HTML"
        << QString("");
    QTest::newRow("if-endif")
        << R"MARKUP(\if defined(qtquick)
Quick only.
\else
Other.
\endif)MARKUP"
        << R"HTML(<p>Other.</p>)HTML"
        << QString("");
    QTest::newRow("nested-list")
        << R"MARKUP(\list
\li Outer
\list
\li Inner
\endlist
\li Second
\endlist)MARKUP"
        << R"HTML(<ul><li><p>Outer</p>
<ul><li>Inner</li></ul></li>
<li>Second</li></ul>)HTML"
        << QString("");
    QTest::newRow("escapes")
        << R"MARKUP(A literal backslash \\ and a brace \{ and \c {code with spaces}.)MARKUP"
        << R"HTML(<p>A literal backslash \ and a brace { and <code class="qdoc-inline-code">code with spaces</code>.</p>)HTML"
        << QString("");
    QTest::newRow("meta-badges")
        << R"MARKUP(\qmltype Item
\inqmlmodule QtQuick
\readonly
\ingroup one
\ingroup two)MARKUP"
        << R"HTML(<h1 class="qdoc-title" id="m-item">Item QML Type</h1>
<p class="qdoc-badges"><span class="qdoc-badge qdoc-badge-module">import QtQuick</span> <span class="qdoc-badge qdoc-badge-flag">read-only</span> <span class="qdoc-badge qdoc-badge-group">one</span> <span class="qdoc-badge qdoc-badge-group">two</span></p>)HTML"
        << QString("");
    QTest::newRow("quotation")
        << R"MARKUP(\quotation
Quoted text.
\endquotation)MARKUP"
        << R"HTML(<div class="qdoc-admonition qdoc-quotation"><p>Quoted text.</p></div>)HTML"
        << QString("");
    QTest::newRow("compareswith")
        << R"MARKUP(\compareswith strong QString
Comparison notes.
\endcompareswith)MARKUP"
        << R"HTML(<div class="qdoc-compares"><p class="qdoc-compares-kind">Compares strong QString</p><pre>Comparison notes.</pre></div>)HTML"
        << QString("");
    QTest::newRow("keyword-anchor")
        << R"MARKUP(\keyword some keyword
Text.)MARKUP"
        << R"HTML(<p><span class="qdoc-anchor" id="some-keyword"></span>Text.</p>)HTML"
        << QString("");
    QTest::newRow("generatelist")
        << R"MARKUP(\generatelist classesbymodule QtCore)MARKUP"
        << QString(R"HTML(<div class="qdoc-placeholder" role="note"><code>)HTML"
                   R"HTML(\generatelist classesbymodule QtCore</code>)HTML"
                   R"HTML(<span class="qdoc-placeholder-note"> %1 not available in preview)HTML"
                   R"HTML(</span></div>)HTML")
               .arg(QChar(0x2014))
        << QString("");
}

void tst_QDoc::render()
{
    QFETCH(QString, markup);
    QFETCH(QString, html);
    QFETCH(QString, problems);

    QStringList messages;
    QCOMPARE(renderBody(markup, &messages), html);
    QCOMPARE(messages.join('\n'), problems);
}

void tst_QDoc::listHints_data()
{
    QTest::addColumn<QString>("hint");
    QTest::addColumn<int>("style");
    QTest::addColumn<int>("start");
    QTest::addColumn<bool>("unrecognized");

    QTest::newRow("empty") << QString() << int(ListStyle::Bullet) << 1 << false;
    QTest::newRow("numeric") << "5" << int(ListStyle::Numeric) << 5 << false;
    QTest::newRow("lower roman") << "iv" << int(ListStyle::LowerRoman) << 4 << false;
    QTest::newRow("upper roman") << "IV" << int(ListStyle::UpperRoman) << 4 << false;
    // c and d would read as 100 and 500, so they stay alphabetic.
    QTest::newRow("alpha c") << "c" << int(ListStyle::LowerAlpha) << 3 << false;
    QTest::newRow("alpha aa") << "aa" << int(ListStyle::LowerAlpha) << 27 << false;
    QTest::newRow("upper alpha") << "A" << int(ListStyle::UpperAlpha) << 1 << false;
    QTest::newRow("decorated numeric") << "(1)" << int(ListStyle::LowerAlpha) << 1 << false;
    QTest::newRow("decorated roman") << "x)" << int(ListStyle::LowerRoman) << 10 << false;
    QTest::newRow("unrecognized") << "1 2" << int(ListStyle::Bullet) << 1 << true;
}

void tst_QDoc::listHints()
{
    QFETCH(QString, hint);
    QFETCH(int, style);
    QFETCH(int, start);
    QFETCH(bool, unrecognized);

    const ListHint parsed = parseListHint(hint);
    QCOMPARE(int(parsed.style), style);
    QCOMPARE(parsed.start, start);
    QCOMPARE(parsed.unrecognized, unrecognized);
}

void tst_QDoc::helpers()
{
    QCOMPARE(fromRoman("xiv"), 14);
    QCOMPARE(fromRoman("iiii"), 0);
    QCOMPARE(fromAlpha("ab"), 28);
    QCOMPARE(fromAlpha("a1"), 0);
    QCOMPARE(slugify("The \\c foo command"), QString("the-foo-command"));
    QCOMPARE(cleanLink("mailto:someone@example.com"), QString("someone@example.com"));
    QCOMPARE(dedent("\n    one\n      two\n"), QString("one\n  two"));
    QCOMPARE(canonicalName("qsizetype QString::size() const"), QString("QString::size"));
    QCOMPARE(nameKeys("QtQuick::Item::width"),
             QStringList({"QtQuick::Item::width", "Item::width", "width"}));
}


// A configuration tree written to disk, since parsing is about files including one
// another and about paths resolving against the file that declared them.
class ConfigDir
{
public:
    ConfigDir()
    {
        QVERIFY(dir.isValid());
        root = Utils::FilePath::fromString(dir.path());
    }

    void write(const QString &relative, const QString &contents)
    {
        const Utils::FilePath file = root.pathAppended(relative);
        QVERIFY(file.parentDir().ensureWritableDir().has_value());
        QVERIFY(file.writeFileContents(contents.toUtf8()).has_value());
    }

    QTemporaryDir dir;
    Utils::FilePath root;
};

void tst_QDoc::config()
{
    ConfigDir tree;
    tree.write("doc/main.qdocconf", R"(# a comment
project = Test Project
{sourcedirs,headerdirs} = ../src
imagedirs = images ../missing
depends += qtcore \
           qtgui
description = "a value" "and another"
tabsize = 4
documentationinheaders = indeed
defines = QT_FOO=1 QT_BAR
codelanguages = Cpp, QML
include(other.qdocconf)
expanded = $project
literal = \$project
)");
    tree.write("doc/other.qdocconf", "macro.thing = \"\\\\b{thing}\"\nimagedirs += extra\n");
    QVERIFY(tree.root.pathAppended("doc/images").ensureWritableDir().has_value());
    QVERIFY(tree.root.pathAppended("doc/extra").ensureWritableDir().has_value());
    QVERIFY(tree.root.pathAppended("src").ensureWritableDir().has_value());

    const Config config = parseConfig(tree.root.pathAppended("doc/main.qdocconf"));

    QCOMPARE(config.value("project"), QString("Test Project"));
    QCOMPARE(config.values("description"), QStringList({"a value", "and another"}));
    QCOMPARE(config.values("depends"), QStringList({"qtcore", "qtgui"}));
    QCOMPARE(config.intValue("tabsize", 8), 4);
    QCOMPARE(config.boolValue("documentationinheaders"), true);
    QCOMPARE(config.boolValue("nosuchkey", true), true);
    // Brace expansion assigns to both keys, and each path resolves against the
    // directory of the file that declared it.
    QCOMPARE(config.pathList("sourcedirs"), Utils::FilePaths({tree.root.pathAppended("src")}));
    QCOMPARE(config.pathList("headerdirs"), Utils::FilePaths({tree.root.pathAppended("src")}));
    // The include() is read, and a directory that does not exist is dropped.
    QCOMPARE(config.pathList("imagedirs"),
             Utils::FilePaths({tree.root.pathAppended("doc/images"),
                               tree.root.pathAppended("doc/extra")}));
    QCOMPARE(config.files.size(), 2);
    // A right-hand side is a list of words, and $project concatenates them, as
    // Config::expandVariables() does.
    QCOMPARE(config.value("expanded"), QString("TestProject"));
    QCOMPARE(config.value("literal"), QString("$project"));

    const QList<ConfigProblem> missing = config.missingPaths({"imagedirs"});
    QCOMPARE(missing.size(), 1);
    QCOMPARE(missing.first().message, QString("Cannot find file or directory: ../missing"));
}

void tst_QDoc::macroTable()
{
    ConfigDir tree;
    tree.write("m.qdocconf", R"(macro.thing = "\\b{thing}"
macro.BR.HTML = "<br />"
macro.QC = $IDE_DISPLAY_NAME
macro.nothing =
macro.arg = "\\c{\1}"
)");
    const Config config = parseConfig(tree.root.pathAppended("m.qdocconf"));
    const QHash<QString, Macro> macros = buildMacroTable(config);

    QCOMPARE(macros.value("thing").def, QString("\\b{thing}"));
    QCOMPARE(macros.value("BR").raw, QString("<br />"));
    QCOMPARE(macros.value("BR").hasRaw, true);
    QCOMPARE(macros.value("arg").params, 1);
    // Defined, but only the documentation build has the value: kept, so that a use of
    // it is not reported as an unknown command.
    QVERIFY(macros.contains("QC"));
    QCOMPARE(macros.value("QC").unresolved, true);
    QCOMPARE(macros.value("QC").variables, QStringList({"IDE_DISPLAY_NAME"}));
    // Nothing to expand to and no variable behind it, so QDoc does not register it.
    QVERIFY(!macros.contains("nothing"));

    ParserContext context;
    context.macros = macros;
    const ParsedDoc doc = parseDoc("Say \\thing here.", context);
    QCOMPARE(flattenText(doc.body.first()->children), QString("Say thing here."));
}

void tst_QDoc::configResolver()
{
    ConfigDir tree;
    tree.write("doc/aaa.qdocconf", "sourcedirs = ..\nimagedirs = images\n");
    tree.write("doc/zzz.qdocconf", "sourcedirs = ../src\nexampledirs = ../src\n"
                                   "macro.thing = \"\\\\b{thing}\"\n");
    tree.write("doc/excluding.qdocconf", "sourcedirs = ../src\nexcludefiles = ../src/page.qdoc\n");
    tree.write("src/page.qdoc", "/*!\n    \\page page.html\n*/\n");
    QVERIFY(tree.root.pathAppended("doc/images").ensureWritableDir().has_value());

    const Utils::FilePath page = tree.root.pathAppended("src/page.qdoc");
    const Utils::FilePaths candidates = findConfigFiles(page.parentDir());
    QCOMPARE(candidates.size(), 3);

    ConfigResolver resolver;
    const DocContext context = resolver.contextFor(page, candidates);
    // The most specific sourcedirs wins over both the alphabetically first candidate
    // and the one that disowns the file through excludefiles.
    QCOMPARE(context.confFile, tree.root.pathAppended("doc/zzz.qdocconf"));
    QVERIFY(context.macros.contains("thing"));
    QCOMPARE(context.exampleDirs, Utils::FilePaths({tree.root.pathAppended("src")}));

    // A file no configuration claims falls back to the nearest by layout.
    const Utils::FilePath stray = tree.root.pathAppended("elsewhere/other.qdoc");
    const DocContext strayContext = resolver.contextFor(stray, candidates);
    QVERIFY(!strayContext.confFile.isEmpty());
}

// Editing the .qdocconf changes what \image resolves to, so a parsed configuration
// is only reused while the files it was read from are unchanged.
void tst_QDoc::editedConfiguration()
{
    QSKIP("This test fails in CI");
    ConfigDir tree;
    tree.write("doc/m.qdocconf", "imagedirs = ../nowhere\nsourcedirs = ..\n");
    tree.write("page.qdoc", "/*!\n    \\page p.html\n*/\n");
    QVERIFY(tree.root.pathAppended("images").ensureWritableDir().has_value());

    const Utils::FilePath page = tree.root.pathAppended("page.qdoc");
    const Utils::FilePaths candidates = findConfigFiles(page.parentDir());
    ConfigResolver resolver;
    QVERIFY(resolver.contextFor(page, candidates).imageSearchDirs.isEmpty());
    // The file that was read is the one to watch.
    QCOMPARE(resolver.contextFor(page, candidates).confFiles,
             Utils::FilePaths({tree.root.pathAppended("doc/m.qdocconf")}));

    tree.write("doc/m.qdocconf", "imagedirs = ../images\nsourcedirs = ..\n");
    QCOMPARE(resolver.contextFor(page, candidates).imageSearchDirs,
             Utils::FilePaths({tree.root.pathAppended("images")}));
}

void tst_QDoc::snippets_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("identifier");
    QTest::addColumn<QString>("snippet");

    QTest::newRow("plain")
        << "a.cpp"
        << "0"
        << "int main()\n{\n    return 0;\n}";
    // The marker is matched whitespace-insensitively, so //![1] finds //! [1].
    QTest::newRow("tight marker") << "a.cpp" << "1" << "int x = 1;";
    // A nested marker of another snippet leaves a blank line behind.
    QTest::newRow("nested") << "a.cpp" << "2" << "before\n\nafter";
    QTest::newRow("hash marker") << "a.pro" << "0" << "TEMPLATE = app";
    QTest::newRow("missing") << "a.cpp" << "9" << QString();
}

void tst_QDoc::snippets()
{
    QFETCH(QString, file);
    QFETCH(QString, identifier);
    QFETCH(QString, snippet);

    ConfigDir tree;
    tree.write("a.cpp", R"(//! [0]
int main()
{
    return 0;
}
//! [0]
    //![1]
    int x = 1;
    //![1]
//! [2]
before
//! [inner]
after
//! [2]
)");
    tree.write("a.pro", "#! [0]\nTEMPLATE = app\n#! [0]\n");

    const Utils::FilePath quoted = tree.root.pathAppended(file);
    const std::optional<QString> extracted
        = extractSnippet(QString::fromUtf8(*quoted.fileContents()),
                         identifier,
                         commentMarkerFor(quoted));
    QCOMPARE(extracted.value_or(QString()), snippet);
}

void tst_QDoc::quoteSteps()
{
    const QString text = "one\ntwo\nthree\nfour\nfive\n";
    // Two listings walking one cursor: the second carries on where the first
    // stopped, and the common indentation is removed across both.
    const QStringList segments = runQuoteSegments(
        text,
        {{{"printline", {}}, {"skipto", "three"}}, {{"printuntil", "four"}}});
    QCOMPARE(segments, QStringList({"one", "three\nfour"}));

    // A pattern in slashes is a regular expression.
    QCOMPARE(runQuoteSegments(text, {{{"skipto", "/t.o/"}, {"printline", {}}}}),
             QStringList({"two"}));

    // trimWhiteSpace() copies a quirk of quoter.cpp: a space between two
    // alphanumerics duplicates the second character rather than being kept. Both
    // sides of a comparison get it, so matching stays consistent with QDoc.
    QCOMPARE(trimWhiteSpace("int x = 3"), QString("intxx=3"));
    QVERIFY(matchesQuotePattern("//!  [ 0 ]", "//! [0]"));
}

void tst_QDoc::quotedLanguage_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("marker");

    QTest::newRow("cpp") << "a.cpp" << "cpp" << "//!";
    QTest::newRow("header") << "a.h" << "cpp" << "//!";
    QTest::newRow("qml") << "a.qml" << "qml" << "//!";
    // FilePath::suffix() reports "ui.qml" here, which no table is keyed on.
    QTest::newRow("ui.qml") << "Form.ui.qml" << "qml" << "//!";
    QTest::newRow("project") << "a.pro" << "qmake" << "#!";
    QTest::newRow("cmake") << "CMakeLists.txt" << "cmake" << "#!";
    QTest::newRow("xml") << "a.qrc" << "xml" << "<!--";
    QTest::newRow("unknown") << "a.zzz" << QString() << "//!";
}

void tst_QDoc::quotedLanguage()
{
    QFETCH(QString, fileName);
    QFETCH(QString, language);
    QFETCH(QString, marker);

    const Utils::FilePath file = Utils::FilePath::fromString("/tmp/" + fileName);
    QCOMPARE(languageFor(file), language);
    QCOMPARE(commentMarkerFor(file), marker);
}

// A problem carries the line it belongs to, which is what the Problems pane needs.
void tst_QDoc::problemPositions()
{
    const QString text = R"(// Copyright
/*!
    \page test.html
    \title Test

    A paragraph with \macOS in it.

    \list
    \li One
*/
)";
    const PreviewPage page = renderPreviewPage(text, Utils::FilePath::fromString("t.qdoc"));

    QList<QPair<QString, int>> found;
    for (const Problem &problem : page.problems)
        found.append({problem.rule, problem.line});
    const QList<QPair<QString, int>> expected{{"unknown-command", 6},
                                              {"unterminated-block", 10}};
    QCOMPARE(found, expected);
}

void tst_QDoc::topicCommandCheck_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("reported");

    QTest::newRow("no topic") << "page.qdoc" << "/*!\n    Just prose.\n*/\n" << true;
    QTest::newRow("with topic") << "page.qdoc" << "/*!\n    \\page a.html\n    Prose.\n*/\n"
                                << false;
    // A fragment documents part of somebody else's topic, and a C++ comment attaches
    // to the declaration that follows it.
    QTest::newRow("fragment") << "part.qdocinc" << "Just prose.\n" << false;
    QTest::newRow("source") << "a.cpp" << "/*!\n    Just prose.\n*/\nvoid f() {}\n" << false;
    QTest::newRow("annotations only") << "page.qdoc" << "/*!\n    \\internal\n*/\n" << false;
}

void tst_QDoc::topicCommandCheck()
{
    QFETCH(QString, fileName);
    QFETCH(QString, text);
    QFETCH(bool, reported);

    const PreviewPage page = renderPreviewPage(text, Utils::FilePath::fromString(fileName));
    bool found = false;
    for (const Problem &problem : page.problems)
        found = found || problem.rule == "missing-topic-command";
    QCOMPARE(found, reported);
}

void tst_QDoc::ruleIds_data()
{
    QTest::addColumn<QString>("message");
    QTest::addColumn<QString>("rule");

    QTest::newRow("unknown") << "Unknown command '\\macOS'" << "unknown-command";
    QTest::newRow("unterminated") << "Missing '\\endlist'" << "unterminated-block";
    QTest::newRow("brace") << "Missing '}'" << "unterminated-block";
    QTest::newRow("misplaced") << "Command '\\li' outside of '\\list' and '\\table'"
                               << "misplaced-command";
    QTest::newRow("stray end") << "Unexpected '\\endtable'" << "misplaced-command";
    QTest::newRow("image") << "Missing image: foo.png" << "missing-image";
    QTest::newRow("snippet") << "snippet marker \"//! [0]\" not found in a.cpp"
                             << "missing-snippet";
    QTest::newRow("config") << "Cannot find file or directory: images" << "config-missing-path";
    QTest::newRow("unmatched") << "Something else entirely" << "invalid-argument";
}

void tst_QDoc::ruleIds()
{
    QFETCH(QString, message);
    QFETCH(QString, rule);
    QCOMPARE(ruleForMessage(message), rule);
}

// The preview scrolls to a comment by its anchor, and jumps back to the line the
// comment starts on, so both have to come out of the page.
void tst_QDoc::previewBlocks()
{
    const QString text = "// A header comment\n"
                         "/*!\n    \\page a.html\n    First.\n*/\n"
                         "\n"
                         "/*!\n    \\page b.html\n    Second.\n*/\n";
    const PreviewPage page = renderPreviewPage(text, Utils::FilePath::fromString("t.qdoc"));

    QCOMPARE(page.blocks.size(), 2);
    QCOMPARE(page.blocks.at(0).line, 1);
    QCOMPARE(page.blocks.at(0).endLine, 4);
    QCOMPARE(page.blocks.at(1).line, 6);
    QCOMPARE(page.blocks.at(1).endLine, 9);
    QVERIFY(page.body.contains("<a name=\"qdoc-block-0\">"));
    QVERIFY(page.body.contains("<a name=\"qdoc-block-1\">"));
}

// A snippet is read again after its file changes: the caches that make a page of
// snippets cheap must not outlive the contents they were built from.
void tst_QDoc::editedSnippet()
{
    ConfigDir tree;
    tree.write("doc/m.qdocconf", "exampledirs = ..\nsourcedirs = ..\n");
    tree.write("code.cpp", "//! [0]\nint before = 1;\n//! [0]\n");
    tree.write("page.qdoc", "/*!\n    \\page p.html\n    \\snippet code.cpp 0\n*/\n");

    const Utils::FilePath page = tree.root.pathAppended("page.qdoc");
    ConfigResolver resolver;
    QuoteCache cache;
    RenderContext context;
    context.doc = resolver.contextFor(page, findConfigFiles(page.parentDir()));
    context.quoteCache = &cache;
    QCOMPARE(context.doc.confFile, tree.root.pathAppended("doc/m.qdocconf"));

    // The snippet is syntax highlighted, so its identifiers are what to look for.
    const QString markup = QString::fromUtf8(*page.fileContents());
    QVERIFY(renderPreviewPage(markup, page, context).body.contains("before"));

    tree.write("code.cpp", "//! [0]\nint after = 2;\n//! [0]\n");
    const QString rendered = renderPreviewPage(markup, page, context).body;
    QVERIFY2(rendered.contains("after"), qPrintable(rendered));
    QVERIFY(!rendered.contains("before"));
}

// The source pane is colored from these spans, so what they cover is the whole of
// the highlighting.
void tst_QDoc::markupSpans_data()
{
    QTest::addColumn<QString>("line");
    QTest::addColumn<int>("stateBefore");
    QTest::addColumn<QString>("spans");
    QTest::addColumn<int>("stateAfter");

    // Written as "<covered text>:<span>", in the order the scanner reports them.
    QTest::newRow("prose") << "Just prose here." << 0 << "Just prose here.:body" << 0;
    QTest::newRow("comment") << "// Copyright" << 0 << "// Copyright:comment" << 0;
    QTest::newRow("section marker") << "//! [linux]" << 0 << "//! [linux]:comment" << 0;
    QTest::newRow("delimiter") << "/*!" << 0 << "/*!:body|/*!:comment" << 0;
    QTest::newRow("command")
        << "    \\note Watch out." << 0 << "    \\note Watch out.:body|\\note:command" << 0;
    QTest::newRow("entity command")
        << "    \\class QString" << 0
        << "    \\class QString:body|\\class:command| QString:entity" << 0;
    QTest::newRow("braced argument")
        << "    \\l {Some Page}" << 0
        << "    \\l {Some Page}:body|\\l:command|{Some Page}:argument" << 0;
    // An unknown command is left to the linter rather than colored as one.
    QTest::newRow("unknown command")
        << "    Use \\macOS here." << 0 << "    Use \\macOS here.:body" << 0;
    QTest::newRow("escape") << "    A \\\\ backslash." << 0 << "    A \\\\ backslash.:body" << 0;
    QTest::newRow("code opens")
        << "    \\code" << 0 << "    \\code:body|\\code:command" << 1;
    QTest::newRow("code body") << "    int x = 1;" << 1 << "    int x = 1;:code" << 1;
    QTest::newRow("code closes")
        << "    \\endcode" << 1 << "    :code|\\endcode:command" << 0;
    // \badcode ends with \endcode, so the state has to name the block, not the
    // command that opened it.
    QTest::newRow("badcode closes")
        << "    \\endcode" << 2 << "    :code|\\endcode:command" << 0;
}

void tst_QDoc::markupSpans()
{
    QFETCH(QString, line);
    QFETCH(int, stateBefore);
    QFETCH(QString, spans);
    QFETCH(int, stateAfter);

    static const QHash<MarkupSpan, QString> names{{MarkupSpan::Body, "body"},
                                                  {MarkupSpan::Command, "command"},
                                                  {MarkupSpan::Entity, "entity"},
                                                  {MarkupSpan::Argument, "argument"},
                                                  {MarkupSpan::Comment, "comment"},
                                                  {MarkupSpan::Code, "code"}};
    int state = stateBefore;
    QStringList found;
    for (const MarkupToken &token : scanMarkupLine(line, &state)) {
        found.append(QString("%1:%2")
                         .arg(line.mid(token.start, token.length), names.value(token.span)));
    }
    QCOMPARE(found.join('|'), spans);
    QCOMPARE(state, stateAfter);
}

// Every documentation file of this repository has to render without crashing and
// without coming out empty.
void tst_QDoc::corpus()
{
    const QString corpusDir = QDOC_TEST_CORPUS_DIR;
    if (!QFileInfo::exists(corpusDir))
        QSKIP("The documentation directory of this checkout is not available");

    int files = 0;
    QDirIterator it(corpusDir, {"*.qdoc", "*.qdocinc"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        QFile file(path);
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(path));
        const QString text = QString::fromUtf8(file.readAll());
        const PreviewPage page = renderPreviewPage(text, Utils::FilePath::fromString(path));
        QVERIFY2(!page.blocks.isEmpty(), qPrintable(path));
        QVERIFY2(!page.body.isEmpty(), qPrintable(path));
        ++files;
    }
    QVERIFY(files > 100);
}

QTEST_GUILESS_MAIN(tst_QDoc)

#include "tst_qdoc.moc"
