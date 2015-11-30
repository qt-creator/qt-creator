/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "../cplusplus_global.h"
#include <cplusplus/pp.h>
#include <QtTest>
#include <QFile>
#include <QHash>
#include <QSet>

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

typedef QByteArray _;

#define DUMP_OUTPUT(x)     {QByteArray b(x);qDebug("output: [[%s]]", b.replace("\n", "<<\n").constData());}


QByteArray loadSource(const QString &fileName)
{
    QFile inf(QLatin1String(SRCDIR) + QLatin1Char('/') + fileName);
    if (!inf.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug("Cannot open \"%s\"", fileName.toUtf8().constData());
        return QByteArray();
    }

    QTextStream ins(&inf);
    QString source = ins.readAll();
    inf.close();
    return source.toUtf8();
}

void saveData(const QByteArray &data, const QString &fileName)
{
    QFile inf(QLatin1String(SRCDIR) + QLatin1Char('/') + fileName);
    if (!inf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug("Cannot open \"%s\"", fileName.toUtf8().constData());
        return;
    }

    inf.write(data);
    inf.close();
}

static QByteArray stringify(QByteArray code)
{
    QByteArray res = code.replace('"', "\\\"").replace("\n", "\\n\"\n\"");
    if (res.endsWith("\"\n\""))
        res.chop(3);
    return res;
}

static bool compare(const QByteArray &actual, const QByteArray &expected)
{
    if (actual == expected)
        return true;
    qDebug() << "Compared strings are not the same\n"
                "   Actual:\n" << stringify(actual)
             << "   Expected:\n" << stringify(expected);
    return false;
}

struct Include
{
    Include(const QString &fileName, Client::IncludeType type, unsigned line)
        : fileName(fileName), type(type), line(line)
    {}

    QString fileName;
    Client::IncludeType type;
    unsigned line;
};

QDebug &operator<<(QDebug& d, const Include &i)
{
    d << '[' << i.fileName
      << ',' << (i.type == Client::IncludeGlobal ? "Global"
            : (i.type == Client::IncludeLocal ? "Local" : "Unknown"))
      << ',' << i.line
      << ']';
    return d;
}

class MockClient : public Client
{
public:
    struct Block {
        Block() : start(0), end(0) {}
        Block(unsigned start) : start(start), end(0) {}

        unsigned start;
        unsigned end;
    };

public:
    MockClient(Environment *env, QByteArray *output)
        : m_env(env)
        , m_output(output)
        , m_pp(this, env)
        , m_includeDepth(0)
    {}

    virtual ~MockClient() {}

    virtual void macroAdded(const Macro & macro)
    {
        m_definedMacros.append(macro.name());
        m_definedMacrosLine.append(macro.line());
    }

    virtual void passedMacroDefinitionCheck(unsigned /*bytesOffset*/,
                                            unsigned /*utf16charsOffset*/,
                                            unsigned line,
                                            const Macro &macro)
    {
        m_definitionsResolvedFromLines[macro.name()].append(line);
    }

    virtual void failedMacroDefinitionCheck(unsigned /*offset*/,
                                            unsigned /*utf16charsOffset*/,
                                            const ByteArrayRef &name)
    {
        m_unresolvedDefines.insert(name.toByteArray());
    }

    virtual void notifyMacroReference(unsigned bytesOffset, unsigned /*utf16charsOffset*/,
                                      unsigned line, const Macro &macro)
    {
        m_macroUsesLine[macro.name()].append(line);
        m_expandedMacrosOffset.append(bytesOffset);
    }

    virtual void startExpandingMacro(unsigned bytesOffset,
                                     unsigned /*utf16charsOffset*/,
                                     unsigned line,
                                     const Macro &macro,
                                     const QVector<MacroArgumentReference> &actuals
                                            = QVector<MacroArgumentReference>())
    {
        m_expandedMacros.append(macro.name());
        m_expandedMacrosOffset.append(bytesOffset);
        m_macroUsesLine[macro.name()].append(line);
        m_macroArgsCount.append(actuals.size());
        m_usedMacros.insert(macro.name(), actuals);
    }

    virtual void stopExpandingMacro(unsigned /*offset*/, const Macro &/*macro*/) {}

    virtual void startSkippingBlocks(unsigned utf16charsOffset)
    { m_skippedBlocks.append(Block(utf16charsOffset)); }

    virtual void stopSkippingBlocks(unsigned utf16charsOffset)
    { m_skippedBlocks.last().end = utf16charsOffset; }

    virtual void sourceNeeded(unsigned line, const QString &includedFileName, IncludeType mode,
                              const QStringList &initialIncludes = QStringList())
    {
        Q_UNUSED(initialIncludes)
#if 1
        m_recordedIncludes.append(Include(includedFileName, mode, line));
        Q_UNUSED(m_env);
        Q_UNUSED(m_includeDepth);
#else
        Q_UNUSED(line);

        QString resolvedFileName;
        if (mode == IncludeLocal)
            resolvedFileName = resolveLocally(m_env->currentFile, includedFileName);
        else
            resolvedFileName = resolveGlobally(includedFileName);

    //    qDebug("resolved [[%s]] to [[%s]] from [[%s]] (%s)\n",
    //           includedFileName.toUtf8().constData(),
    //           resolvedFileName.toUtf8().constData(),
    //           currentFileName.toUtf8().constData(),
    //           (mode == IncludeLocal) ? "locally" : "globally");

        if (resolvedFileName.isEmpty())
            return;

        ++m_includeDepth;
        //    qDebug("%5d %s %s", m_includeDepth, QByteArray(m_includeDepth, '+').constData(), resolvedFileName.toUtf8().constData());
        sourceNeeded(resolvedFileName);
        --m_includeDepth;
#endif
    }

    QString resolveLocally(const QString &currentFileName,
                           const QString &includedFileName) const
    {
        QDir dir;
        if (currentFileName.isEmpty())
            dir = QDir::current();
        else
            dir = QFileInfo(currentFileName).dir();
        const QFileInfo inc(dir, includedFileName);
        if (inc.exists()) {
            const QString resolved = inc.filePath();
            return resolved.toUtf8().constData();
        } else {
    //        std::cerr<<"Cannot find " << inc.fileName().toUtf8().constData()<<std::endl;
            return QString();
        }
    }

    QString resolveGlobally(const QString &currentFileName) const
    {
        foreach (const QDir &dir, m_includePaths) {
            QFileInfo f(dir, currentFileName);
            if (f.exists())
                return f.filePath();
        }

        return QString();
    }

    void setIncludePaths(const QStringList &includePaths)
    {
        foreach (const QString &path, includePaths) {
            QDir dir(path);
            if (dir.exists())
                m_includePaths.append(dir);
        }
    }

    void sourceNeeded(const QString &fileName, bool nolines)
    {
        QByteArray src = loadSource(fileName);
        QVERIFY(!src.isEmpty());
        *m_output = m_pp.run(fileName, src, nolines, true);
    }

    virtual void markAsIncludeGuard(const QByteArray &macroName)
    { m_includeGuardMacro = macroName; }

    QByteArray includeGuard() const
    { return m_includeGuardMacro; }

    QList<Block> skippedBlocks() const
    { return m_skippedBlocks; }

    QList<Include> recordedIncludes() const
    { return m_recordedIncludes; }

    QList<QByteArray> expandedMacros() const
    { return m_expandedMacros; }

    QList<unsigned> expandedMacrosOffset() const
    { return m_expandedMacrosOffset; }

    QList<QByteArray> definedMacros() const
    { return m_definedMacros; }

    QList<unsigned> definedMacrosLine() const
    { return m_definedMacrosLine; }

    QHash<QByteArray, QList<unsigned> > macroUsesLine() const
    { return m_macroUsesLine; }

    QHash<QByteArray, QList<unsigned> > definitionsResolvedFromLines() const
    { return m_definitionsResolvedFromLines; }

    QSet<QByteArray> unresolvedDefines() const
    { return m_unresolvedDefines; }

    const QList<int> macroArgsCount() const
    { return m_macroArgsCount; }

    const QMap<QByteArray, QVector<MacroArgumentReference >> usedMacros() const
    { return m_usedMacros; }

private:
    Environment *m_env;
    QByteArray *m_output;
    Preprocessor m_pp;
    QList<QDir> m_includePaths;
    unsigned m_includeDepth;
    QByteArray m_includeGuardMacro;
    QList<Block> m_skippedBlocks;
    QList<Include> m_recordedIncludes;
    QList<QByteArray> m_expandedMacros;
    QList<unsigned> m_expandedMacrosOffset;
    QList<QByteArray> m_definedMacros;
    QList<unsigned> m_definedMacrosLine;
    QHash<QByteArray, QList<unsigned> > m_macroUsesLine;
    QHash<QByteArray, QList<unsigned> > m_definitionsResolvedFromLines;
    QSet<QByteArray> m_unresolvedDefines;
    QList<int> m_macroArgsCount;
    QMap<QByteArray, QVector<MacroArgumentReference >> m_usedMacros;
};

QT_BEGIN_NAMESPACE
namespace QTest {
    template<> char *toString(const QList<unsigned> &list)
    {
        QByteArray ba = "QList<unsigned>(";
        foreach (const unsigned& item, list) {
            ba += QTest::toString(item);
            ba += ',';
        }
        if (!list.isEmpty())
            ba[ba.size() - 1] = ')';
        return qstrdup(ba.data());
    }
    template<> char *toString(const QList<QByteArray> &list)
    {
        QByteArray ba = "QList<QByteArray>(";
        foreach (const QByteArray& item, list) {
            ba += QTest::toString(item);
            ba += ',';
        }
        if (!list.isEmpty())
            ba[ba.size() - 1] = ')';
        return qstrdup(ba.data());
    }
}
QT_END_NAMESPACE

QDebug &operator<<(QDebug& d, const MockClient::Block &b)
{
    d << '[' << b.start << ',' << b.end << ']';
    return d;
}

class tst_Preprocessor : public QObject
{
    Q_OBJECT

protected:
    QByteArray preprocess(const QString &fileName, QByteArray * /*errors*/, bool nolines) {
        //### TODO: hook up errors
        QByteArray output;
        Environment env;
        MockClient client(&env, &output);
        client.sourceNeeded("data/" + fileName, nolines);
        return output;
    }
    static QByteArray simplified(const QByteArray &buf);

private:
    void compare_input_output(bool keepComments = false);

private slots:
    void va_args();
    void named_va_args();
    void extra_va_args();
    void defined();
    void defined_data();
    void defined_usage();
    void empty_macro_args();
    void macro_args_count();
    void macro_args_offsets();
    void macro_args_offsets_data();
    void invalid_param_count();
    void objmacro_expanding_as_fnmacro_notification();
    void macro_uses();
    void macro_uses_lines();
    void macro_arguments_notificatin();
    void unfinished_function_like_macro_call();
    void nasty_macro_expansion();
    void glib_attribute();
    void builtin__FILE__();
    void blockSkipping();
    void includes_1();
    void dont_eagerly_expand();
    void dont_eagerly_expand_data();
    void comparisons_data();
    void comparisons();
    void comments_before_args();
    void comments_within();
    void comments_within_data();
    void multitokens_argument();
    void multitokens_argument_data();
    void multiline_strings();
    void multiline_strings_data();
    void skip_unknown_directives();
    void skip_unknown_directives_data();
    void include_guard();
    void include_guard_data();
    void empty_trailing_lines();
    void empty_trailing_lines_data();
    void undef();
    void concat();
    void excessive_nesting();
    void multi_byte_code_point_in_expansion();
};

// Remove all #... lines, and 'simplify' string, to allow easily comparing the result
// Also, remove all unneeded spaces: keep only to ensure identifiers are separated.
// NOTE: may not correctly handle underscore in identifiers
QByteArray tst_Preprocessor::simplified(const QByteArray &buf)
{
    QString out;
    QList<QByteArray> lines = buf.split('\n');
    foreach (const QByteArray &line, lines) {
        if (!line.startsWith('#')) {
            out.append(' ');
            out.append(line);
        }
    }

    out = out.simplified();
    for (int i = 1; i < out.length() - 1; ) {
        if (out.at(i).isSpace()
                && !(out.at(i-1).isLetterOrNumber()
                && out.at(i+1).isLetterOrNumber()))
            out.remove(i,1);
        else
            i++;
    }

    return out.toUtf8();
}

void tst_Preprocessor::va_args()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess.run(QLatin1String("<stdin>"),
                                                "#define foo(...) int f(__VA_ARGS__);\n"
                                                "\nfoo(  )\n"
                                                "\nfoo(int a)\n"
                                                "\nfoo(int a,int b)\n",
                                             true, false);

    preprocessed = preprocessed.simplified();
//    DUMP_OUTPUT(preprocessed);
    QVERIFY(compare(simplified(preprocessed), "int f();int f(int a);int f(int a,int b);"));
}

void tst_Preprocessor::named_va_args()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess.run(QLatin1String("<stdin>"),
                                                "\n#define foo(ARGS...) int f(ARGS);"
                                                "\nfoo(  )\n"
                                                "\nfoo(int a)\n"
                                                "\nfoo(int a,int b)\n",
                                             true, false);

    preprocessed = preprocessed.simplified();
    QVERIFY(compare(simplified(preprocessed), "int f();int f(int a);int f(int a,int b);"));
}

void tst_Preprocessor::extra_va_args()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess.run(QLatin1String("<stdin>"),
                                                "#define foo(ret, ...) ret f(__VA_ARGS__);\n"
                                                "\nfoo(int)\n"
                                                "\nfoo(float,int b)\n"
                                                "\nfoo(long,int b,int c)\n",
                                             true, false);

    preprocessed = preprocessed.simplified();
    QVERIFY(compare(simplified(preprocessed), "int f();float f(int b);long f(int b,int c);"));
}

void tst_Preprocessor::empty_macro_args()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess.run(QLatin1String("<stdin>"),
                                                "\n#define foo(a,b) a int b;"
                                                "\nfoo(const,cVal)\n"
                                                "\nfoo(,Val)\n"
                                                "\nfoo( ,Val2)\n"
                                                "\nfoo(,)\n"
                                                "\nfoo(, )\n",
                                             true, false);

    preprocessed = preprocessed.simplified();
//    DUMP_OUTPUT(preprocessed);
    QVERIFY(compare(simplified(preprocessed), "const int cVal;int Val;int Val2;int;int;"));
}

void tst_Preprocessor::macro_args_count()
{
    Environment env;
    QByteArray output;
    MockClient client(&env, &output);
    Preprocessor preprocess(&client, &env);
    preprocess.run(QLatin1String("<stdin>"),
                   "#define foo(a,b) a int b;\n"
                   "foo(const,cVal)\n"
                   "foo(, i)\n"
                   "foo(,Val)\n"
                   "foo( ,Val2)\n"
                   "foo(,)\n"
                   "foo(, )\n"
                   "#define bar(a)\n"
                   "bar()\n"
                   "bar(i)\n",
                   true, false);

    QCOMPARE(client.macroArgsCount(),
             QList<int>() << 2 // foo(const,cVal)
                          << 2 // foo(, i)
                          << 2 // foo(,Val)
                          << 2 // foo( , Val2)
                          << 2 // foo(,)
                          << 2 // foo(, )
                          << 1 // bar()
                          << 1 // bar(i)
            );

}

void tst_Preprocessor::macro_args_offsets()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, source);
    QFETCH(QByteArray, macroName);
    QFETCH(unsigned, bytesOffset);
    QFETCH(unsigned, bytesLength);
    QFETCH(unsigned, utf16charsOffset);
    QFETCH(unsigned, utf16charsLength);

    Environment env;
    QByteArray output;
    MockClient client(&env, &output);
    Preprocessor preprocess(&client, &env);
    preprocess.run(fileName, source, true, false);

    QMap<QByteArray, QVector<MacroArgumentReference >> usedMacros = client.usedMacros();
    QCOMPARE(usedMacros.size(), 1);
    QVERIFY(usedMacros.contains(macroName));
    MacroArgumentReference argRef = usedMacros.value(macroName).at(0);
    QCOMPARE(argRef.bytesOffset(), bytesOffset);
    QCOMPARE(argRef.bytesLength(), bytesLength);
    QCOMPARE(argRef.utf16charsOffset(), utf16charsOffset);
    QCOMPARE(argRef.utf16charsLength(), utf16charsLength);


}

void tst_Preprocessor::macro_args_offsets_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QByteArray>("macroName");
    QTest::addColumn<unsigned>("bytesOffset");
    QTest::addColumn<unsigned>("bytesLength");
    QTest::addColumn<unsigned>("utf16charsOffset");
    QTest::addColumn<unsigned>("utf16charsLength");

    QString fN = QLatin1String("<stdin>");
    QByteArray src = QByteArray("#define SQR(a) ( a * a )\n"
                                "void f(){\n"
                                "int i = 10;\n"
                                "int j = SQR(10);\n"
                                "}");
    QTest::newRow("ascii_only_before_ref") << fN << src << QByteArray("SQR")
                                           << 59u << 2u << 59u << 2u;
    src.replace("int i", "int äöü");
    QTest::newRow("ascii_with_umlauts_before_ref") << fN << src << QByteArray("SQR")
                                                   << 64u << 2u << 61u << 2u;
    src.clear();
    src.append("#define OUT(format, ...) printf(\"%s %d: \" format, __FILE__, __LINE__)\n"
               "void f(){\n"
               "OUT(\"Hei verden!\\n\");\n"
               "}\n");
    QTest::newRow("arg_ascii_only") << fN << src << QByteArray("OUT")
                                    << 84u << 15u << 84u << 15u;
    src.replace("Hei verden", UC_U00FC);
    QTest::newRow("arg_ascii_with_unicode_00fc") << fN << src << QByteArray("OUT")
                                                 << 84u << 7u << 84u << 6u;
    src.replace(UC_U00FC, UC_U4E8C);
    QTest::newRow("arg_ascii_with_unicode_4e8c") << fN << src << QByteArray("OUT")
                                                 << 84u << 8u << 84u << 6u;
    src.replace(UC_U4E8C, UC_U10302);
    QTest::newRow("arg_ascii_with_unicode_10302") << fN << src << QByteArray("OUT")
                                                  << 84u << 9u << 84u << 7u;
}

void tst_Preprocessor::invalid_param_count()
{
    Environment env;
    QByteArray output;
    MockClient client(&env, &output);
    Preprocessor preprocess(&client, &env);
    // The following are illegal, but shouldn't crash the preprocessor.
    preprocess.run(QLatin1String("<stdin>"),
                   "\n#define foo(a,b) int f(a,b);"
                   "\n#define ARGS(t)  t a,t b"
                   "\nfoo(ARGS(int))"
                   "\nfoo()"
                   "\nfoo(int a, int b, int c)",
                   true, false);

    // Output is not that relevant but check that nothing triggered expansion.
    QCOMPARE(client.macroArgsCount(), QList<int>());
}

void tst_Preprocessor::macro_uses()
{
    QByteArray buffer = QByteArray("\n#define FOO 8"
                                   "\n#define BAR 9"
                                   "\nvoid test(){"
                                   "\n\tint x=FOO;"
                                   "\n\tint y=BAR;"
                                   "\n}");

    QByteArray output;
    Environment env;
    MockClient client(&env, &output);

    Preprocessor preprocess(&client, &env);
    QByteArray preprocessed = preprocess.run(QLatin1String("<stdin>"), buffer);
    QVERIFY(compare(simplified(preprocessed), "void test(){int x=8;int y=9;}"));
    QCOMPARE(client.expandedMacros(), QList<QByteArray>() << QByteArray("FOO") << QByteArray("BAR"));
    QCOMPARE(client.expandedMacrosOffset(), QList<unsigned>() << buffer.indexOf("FOO;") << buffer.indexOf("BAR;"));
    QCOMPARE(client.definedMacros(), QList<QByteArray>() << QByteArray("FOO") << QByteArray("BAR"));
    QCOMPARE(client.definedMacrosLine(), QList<unsigned>() << 2 << 3);
}

void tst_Preprocessor::macro_uses_lines()
{
    QByteArray buffer("#define FOO\n"
                      "FOO\n"
                      "\n"
                      "#define HEADER <test>\n"
                      "#include HEADER\n"
                      "\n"
                      "#define DECLARE(C, V) struct C {}; C V;\n"
                      "#define ABC X\n"
                      "DECLARE(Test, test)\n"
                      "\n"
                      "int abc;\n"
                      "#define NOTHING(C)\n"
                      "NOTHING(abc)\n"
                      "\n"
                      "#define ENABLE(FEATURE) (defined ENABLE_##FEATURE && ENABLE_##FEATURE)\n"
                      "#define ENABLE_COOL 1\n"
                      "void fill();\n"
                      "#if ENABLE(COOL)\n"
                      "class Cool {};\n"
                      "#endif\n"
                      "int cool = ENABLE_COOL;\n"
                      "#define OTHER_ENABLE(FEATURE) ENABLE(FEATURE)\n"
                      "#define MORE(LESS) FOO ENABLE(LESS)\n");

    QByteArray output;
    Environment env;
    MockClient client(&env, &output);
    Preprocessor preprocess(&client, &env);
    preprocess.run(QLatin1String("<stdin>"), buffer);

    QCOMPARE(client.macroUsesLine().value("FOO"), QList<unsigned>() << 2U << 23U);
    QCOMPARE(client.macroUsesLine().value("HEADER"), QList<unsigned>() << 5U);
    QCOMPARE(client.macroUsesLine().value("DECLARE"), QList<unsigned>() << 9U);
    QCOMPARE(client.macroUsesLine().value("NOTHING"), QList<unsigned>() << 13U);
    QCOMPARE(client.macroUsesLine().value("ENABLE"), QList<unsigned>() << 18U << 22U << 23U);
    QCOMPARE(client.macroUsesLine().value("ENABLE_COOL"), QList<unsigned>() << 21U);
    QCOMPARE(client.definitionsResolvedFromLines().value("ENABLE_COOL"), QList<unsigned>() << 18U);
    QCOMPARE(client.expandedMacrosOffset(), QList<unsigned>()
             << buffer.lastIndexOf("FOO\n")
             << buffer.lastIndexOf("HEADER")
             << buffer.lastIndexOf("DECLARE")
             << buffer.lastIndexOf("NOTHING")
             << buffer.lastIndexOf("ENABLE(COOL)")
             << buffer.lastIndexOf("ENABLE_COOL")
             << buffer.lastIndexOf("ENABLE(FEATURE)")
             << buffer.lastIndexOf("FOO ")
             << buffer.lastIndexOf("ENABLE(LESS)"));
}

void tst_Preprocessor::multitokens_argument_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("output");

    QTest::newRow("case 1") << _(
            "#define foo(ARGS) int f(ARGS)\n"
            "foo(int a);\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "# expansion begin 30,3 ~3 2:4 2:8 ~1\n"
            "int f(int a)\n"
            "# expansion end\n"
            "# 2 \"<stdin>\"\n"
            "          ;\n"
    );

    QTest::newRow("case 2") << _(
            "#define foo(ARGS) int f(ARGS)\n"
            "foo(int   \n"
            "    a);\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "# expansion begin 30,3 ~3 2:4 3:4 ~1\n"
            "int f(int a)\n"
            "# expansion end\n"
            "# 3 \"<stdin>\"\n"
            "      ;\n"
    );

    QTest::newRow("case 3") << _(
            "#define foo(ARGS) int f(ARGS)\n"
            "foo(int a = 0);\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "# expansion begin 30,3 ~3 2:4 2:8 2:10 2:12 ~1\n"
            "int f(int a = 0)\n"
            "# expansion end\n"
            "# 2 \"<stdin>\"\n"
            "              ;\n"
    );

    QTest::newRow("case 4") << _(
            "#define foo(X) int f(X = 0)\n"
            "foo(int \n"
            "    a);\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "# expansion begin 28,3 ~3 2:4 3:4 ~3\n"
            "int f(int a = 0)\n"
            "# expansion end\n"
            "# 3 \"<stdin>\"\n"
            "      ;\n"
    );
}

void tst_Preprocessor::multitokens_argument()
{
    compare_input_output();
}

void tst_Preprocessor::objmacro_expanding_as_fnmacro_notification()
{
    QByteArray output;
    Environment env;
    MockClient client(&env, &output);

    Preprocessor preprocess(&client, &env);
    QByteArray preprocessed = preprocess.run(QLatin1String("<stdin>"),
                                         QByteArray("\n#define bar(a,b) a + b"
                                                    "\n#define foo bar"
                                                    "\nfoo(1, 2)\n"));

    QVERIFY(client.expandedMacros() == (QList<QByteArray>() << QByteArray("foo")));
}

void tst_Preprocessor::macro_arguments_notificatin()
{
    QByteArray output;
    Environment env;
    MockClient client(&env, &output);

    Preprocessor preprocess(&client, &env);
    QByteArray preprocessed = preprocess.run(QLatin1String("<stdin>"),
                                         QByteArray("\n#define foo(a,b) a + b"
                                                    "\n#define arg(a) a"
                                                    "\n#define value  2"
                                                    "\nfoo(arg(1), value)\n"));

    QVERIFY(client.expandedMacros() == (QList<QByteArray>() << QByteArray("foo")
                                                            << QByteArray("arg")
                                                            << QByteArray("value")));
}

void tst_Preprocessor::unfinished_function_like_macro_call()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess.run(QLatin1String("<stdin>"),
                                             QByteArray("\n"
                                                        "#define foo(a,b) a + b\n"
                                                        "foo(1, 2\n"));
    QByteArray expected__("# 1 \"<stdin>\"\n"
                          "\n"
                          "\n"
                          "# expansion begin 24,3 3:4 ~1 3:7\n"
                          "1 + 2\n"
                          "# expansion end\n"
                          "# 4 \"<stdin>\"\n");

//    DUMP_OUTPUT(preprocessed);
    QVERIFY(compare(preprocessed, expected__));
}

void tst_Preprocessor::nasty_macro_expansion()
{
    QByteArray input("\n"
                     "#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))\n"
                     "#define is_power_of_two(x)      ( !((x) & ((x)-1)) )\n"
                     "#define low_bit_mask(x)         ( ((x)-1) & ~(x) )\n"
                     "#define is_valid_mask(x)        is_power_of_two(1LU + (x) + low_bit_mask(x))\n"
                     "#define compile_ffs2(__x) \\\n"
                     "        __builtin_choose_expr(((__x) & 0x1), 0, 1)\n"
                     "#define compile_ffs4(__x) \\\n"
                     "        __builtin_choose_expr(((__x) & 0x3), \\\n"
                     "                              (compile_ffs2((__x))), \\\n"
                     "                              (compile_ffs2((__x) >> 2) + 2))\n"
                     "#define compile_ffs8(__x) \\\n"
                     "        __builtin_choose_expr(((__x) & 0xf), \\\n"
                     "                              (compile_ffs4((__x))), \\\n"
                     "                              (compile_ffs4((__x) >> 4) + 4))\n"
                     "#define compile_ffs16(__x) \\\n"
                     "        __builtin_choose_expr(((__x) & 0xff), \\\n"
                     "                              (compile_ffs8((__x))), \\\n"
                     "                              (compile_ffs8((__x) >> 8) + 8))\n"
                     "#define compile_ffs32(__x) \\\n"
                     "        __builtin_choose_expr(((__x) & 0xffff), \\\n"
                     "                              (compile_ffs16((__x))), \\\n"
                     "                              (compile_ffs16((__x) >> 16) + 16))\n"
                     "#define FIELD_CHECK(__mask, __type)                     \\\n"
                     "        BUILD_BUG_ON(!(__mask) ||                       \\\n"
                     "                     !is_valid_mask(__mask) ||          \\\n"
                     "                     (__mask) != (__type)(__mask))      \\\n"
                     "\n"
                     "#define FIELD32(__mask)                         \\\n"
                     "({                                              \\\n"
                     "        FIELD_CHECK(__mask, u32);               \\\n"
                     "        (struct rt2x00_field32) {               \\\n"
                     "                compile_ffs32(__mask), (__mask) \\\n"
                     "        };                                      \\\n"
                     "})\n"
                     "#define BBPCSR                          0x00f0\n"
                     "#define BBPCSR_BUSY                     FIELD32(0x00008000)\n"
                     "#define WAIT_FOR_BBP(__dev, __reg)  \\\n"
                     "        rt2x00pci_regbusy_read((__dev), BBPCSR, BBPCSR_BUSY, (__reg))\n"
                     "if (WAIT_FOR_BBP(rt2x00dev, &reg)) {}\n"
                     );

    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess.run(QLatin1String("<stdin>"), input);

    QVERIFY(!preprocessed.contains("FIELD32"));
}

void tst_Preprocessor::glib_attribute()
{
    Environment env;
    Preprocessor preprocess(0, &env);
    QByteArray preprocessed = preprocess.run(
                QLatin1String("<stdin>"),
                QByteArray("\n"
                           "# define _GLIBCXX_VISIBILITY(V) __attribute__ ((__visibility__ (#V)))\n"
                           "namespace std _GLIBCXX_VISIBILITY(default) {\n"
                           "}\n"
                           ));
    const QByteArray result____ =
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "namespace std\n"
            "# expansion begin 85,19 ~9\n"
            "__attribute__ ((__visibility__ (\"default\")))\n"
            "# expansion end\n"
            "# 3 \"<stdin>\"\n"
            "                                           {\n"
            "}\n";

//    DUMP_OUTPUT(preprocessed);
    QVERIFY(compare(preprocessed, result____));
}

void tst_Preprocessor::builtin__FILE__()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess.run(
                QLatin1String("some-file.c"),
                QByteArray("const char *f = __FILE__\n"
                           ));
    const QByteArray result____ =
            "# 1 \"some-file.c\"\n"
            "const char *f = \"some-file.c\"\n";

    QVERIFY(compare(preprocessed, result____));
}

void tst_Preprocessor::comparisons_data()
{
    QTest::addColumn<QString>("infile");
    QTest::addColumn<QString>("outfile");
    QTest::addColumn<QString>("errorfile");

    QTest::newRow("do nothing") << "noPP.1.cpp" << "noPP.1.cpp" << "";
    QTest::newRow("no PP 2") << "noPP.2.cpp" << "noPP.2.cpp" << "";
    QTest::newRow("identifier-expansion 1")
        << "identifier-expansion.1.cpp" << "identifier-expansion.1.out.cpp" << "";
    QTest::newRow("identifier-expansion 2")
        << "identifier-expansion.2.cpp" << "identifier-expansion.2.out.cpp" << "";
    QTest::newRow("identifier-expansion 3")
        << "identifier-expansion.3.cpp" << "identifier-expansion.3.out.cpp" << "";
    QTest::newRow("identifier-expansion 4")
        << "identifier-expansion.4.cpp" << "identifier-expansion.4.out.cpp" << "";
    QTest::newRow("identifier-expansion 5")
        << "identifier-expansion.5.cpp" << "identifier-expansion.5.out.cpp" << "";
    QTest::newRow("reserved 1")
        << "reserved.1.cpp" << "reserved.1.out.cpp" << "";
    QTest::newRow("recursive 1")
        << "recursive.1.cpp" << "recursive.1.out.cpp" << "";
    QTest::newRow("recursive 2")
        << "recursive.2.cpp" << "recursive.2.out.cpp" << "";
    QTest::newRow("macro_pounder_fn")
        << "macro_pounder_fn.c" << "" << "";
    QTest::newRow("macro_expand")
        << "macro_expand.c" << "macro_expand.out.c" << "";
    QTest::newRow("macro_expand_1")
        << "macro_expand_1.cpp" << "macro_expand_1.out.cpp" << "";
    QTest::newRow("macro-test")
        << "macro-test.cpp" << "macro-test.out.cpp" << "";
    QTest::newRow("empty-macro")
        << "empty-macro.cpp" << "empty-macro.out.cpp" << "";
    QTest::newRow("empty-macro 2")
        << "empty-macro.2.cpp" << "empty-macro.2.out.cpp" << "";
    QTest::newRow("poundpound 1")
        << "poundpound.1.cpp" << "poundpound.1.out.cpp" << "";
}

void tst_Preprocessor::comparisons()
{
    QFETCH(QString, infile);
    QFETCH(QString, outfile);
    QFETCH(QString, errorfile);

    QByteArray errors;
    QByteArray preprocessed = preprocess(infile, &errors, infile == outfile);


    // DUMP_OUTPUT(preprocessed);

    if (!outfile.isEmpty()) {
        // These weird underscores are here to make the name as long as
        // "preprocessed", so the QCOMPARE error messages are nicely aligned.
        QByteArray output____ = loadSource("data/" + outfile);
        QVERIFY(compare(preprocessed, output____));
    }

    if (!errorfile.isEmpty()) {
        QByteArray errorFileContents = loadSource("data/" + errorfile);
        QVERIFY(compare(errors, errorFileContents));
    }
}

void tst_Preprocessor::blockSkipping()
{
    QByteArray output;
    Environment env;
    MockClient client(&env, &output);
    Preprocessor pp(&client, &env);
    /*QByteArray preprocessed =*/ pp.run(
                QLatin1String("<stdin>"),
                QByteArray("#if 0\n"
                           "\n"
                           "int yes;\n"
                           "\n"
                           "#elif 0\n"
                           "\n"
                           "int no;\n"
                           "\n"
                           "#else // foobar\n"
                           "\n"
                           "void also_not;\n"
                           "\n"
                           "#endif\n"
                           ));

    QList<MockClient::Block> blocks = client.skippedBlocks();
    QCOMPARE(blocks.size(), 1);
    MockClient::Block b = blocks.at(0);
    QCOMPARE(b.start, 6U);
    QCOMPARE(b.end, 34U);
}

void tst_Preprocessor::includes_1()
{
    QByteArray output;
    Environment env;
    MockClient client(&env, &output);
    Preprocessor pp(&client, &env);
    /*QByteArray preprocessed =*/ pp.run(
                QLatin1String("<stdin>"),
                QByteArray("#define FOO <foo.h>\n"
                           "#define BAR \"bar.h\"\n"
                           "\n"
                           "#include FOO\n"
                           "#include BAR\n"
                           "\n"
                           "#include <zoo.h>\n"
                           "#include \"mooze.h\"\n"
                           ));

    QList<Include> incs = client.recordedIncludes();
//    qDebug()<<incs;
    QCOMPARE(incs.size(), 4);
    QCOMPARE(incs.at(0).fileName, QLatin1String("foo.h"));
    QCOMPARE(incs.at(0).type, Client::IncludeGlobal);
    QCOMPARE(incs.at(0).line, 4U);
    QCOMPARE(incs.at(1).fileName, QLatin1String("bar.h"));
    QCOMPARE(incs.at(1).type, Client::IncludeLocal);
    QCOMPARE(incs.at(1).line, 5U);
    QCOMPARE(incs.at(2).fileName, QLatin1String("zoo.h"));
    QCOMPARE(incs.at(2).type, Client::IncludeGlobal);
    QCOMPARE(incs.at(2).line, 7U);
    QCOMPARE(incs.at(3).fileName, QLatin1String("mooze.h"));
    QCOMPARE(incs.at(3).type, Client::IncludeLocal);
    QCOMPARE(incs.at(3).line, 8U);
}

void tst_Preprocessor::defined()
{
    QFETCH(bool, xdefined);
    QFETCH(bool, ydefined);
    QFETCH(QString, input);
    QByteArray output;
    Environment env;
    MockClient client(&env, &output);
    Preprocessor pp(&client, &env);
    pp.run(QLatin1String("<stdin>"), input.toLatin1(), false, true);
    QList<QByteArray> expected;
    if (xdefined)
        expected.append("X");
    if (ydefined)
        expected.append("Y");
    if (client.definedMacros() != expected)
        qWarning() << "\nSource: " << input.replace('\n', "    ");
    QCOMPARE(client.definedMacros(), expected);
}

void tst_Preprocessor::defined_data()
{
    QTest::addColumn<bool>("xdefined");
    QTest::addColumn<bool>("ydefined");
    QTest::addColumn<QString>("input");

    QTest::newRow("1a") << true << true <<
        "#define X\n#if defined(X)\n#define Y\n#endif";
    QTest::newRow("1b") << true << true <<
        "#define X\n#if defined X \n#define Y\n#endif";
    QTest::newRow("1c") << true << true <<
        "#define X\n#ifdef X \n#define Y\n#endif";

    QTest::newRow("2a") << false << false <<
        "#if defined(X)\n#define Y\n#endif";
    QTest::newRow("2b") << false << false <<
        "#if defined X \n#define Y\n#endif";
    QTest::newRow("2c") << false << false <<
        "#ifdef X \n#define Y\n#endif";

    QTest::newRow("3a") << true << false <<
        "#define X\n#if !defined(X)\n#define Y\n#endif";
    QTest::newRow("3b") << true << false <<
        "#define X\n#if !defined X \n#define Y\n#endif";
    QTest::newRow("3c") << true << false <<
        "#define X\n#ifndef X \n#define Y\n#endif";

    QTest::newRow("4a") << false << true <<
        "#if !defined(X)\n#define Y\n#endif";
    QTest::newRow("4b") << false << true <<
        "#if !defined X \n#define Y\n#endif";
    QTest::newRow("4c") << false << true <<
        "#ifndef X \n#define Y\n#endif";

    QTest::newRow("5a") << false << false <<
        "#if !defined(X) && (defined(Y))\n"
        "#define X\n"
        "#endif\n";
    QTest::newRow("5b") << false << false <<
        "#if !defined(X) && defined(Y)\n"
        "#define X\n"
        "#endif\n";
    QTest::newRow("5c") << false << false <<
        "#if !defined(X) && 0"
        "#define X\n"
        "#endif\n";
    QTest::newRow("5d") << false << false <<
        "#if (!defined(X)) && defined(Y)\n"
        "#define X\n"
        "#endif\n";
    QTest::newRow("5d") << false << false <<
        "#if (define(Y))\n"
        "#define X\n"
        "#endif\n";

    QTest::newRow("6a") << true << true <<
        "#define X 0x040500\n"
        "#if X > 0x040000\n"
        "#define Y 1\n"
        "#endif\n";
    QTest::newRow("6b") << true << true <<
        "#define X 0x040500\n"
        "#if X >= 0x040000\n"
        "#define Y 1\n"
        "#endif\n";
    QTest::newRow("6c") << true << false <<
        "#define X 0x040500\n"
        "#if X == 0x040000\n"
        "#define Y 1\n"
        "#endif\n";
    QTest::newRow("6d") << true << true <<
        "#define X 0x040500\n"
        "#if X == 0x040500\n"
        "#define Y 1\n"
        "#endif\n";
    QTest::newRow("6e") << true << false <<
        "#define X 0x040500\n"
        "#if X < 0x040000\n"
        "#define Y 1\n"
        "#endif\n";
    QTest::newRow("6f") << true << false <<
        "#define X 0x040500\n"
        "#if X <= 0x040000\n"
        "#define Y 1\n"
        "#endif\n";

    QTest::newRow("incomplete defined 1") << true << true <<
        "#define X 0x040500\n"
        "#if defined(X\n"
        "#define Y 1\n"
        "#endif\n";
    QTest::newRow("incomplete defined 2") << false << false <<
        "#if defined(X\n"
        "#define Y 1\n"
        "#endif\n";
    QTest::newRow("complete defined 1") << true << true <<
        "#define X 0x040500\n"
        "#if defined(X )\n"
        "#define Y 1\n"
        "#endif\n";
    QTest::newRow("complete defined 2") << true << true <<
        "#define X 0x040500\n"
        "#if defined(X/*xxx*/)\n"
        "#define Y 1\n"
        "#endif\n";
}

void tst_Preprocessor::defined_usage()
{
    QByteArray output;
    Environment env;
    MockClient client(&env, &output);
    Preprocessor pp(&client, &env);
    QByteArray source =
            "#define X\n"
            "#define Y\n"
            "#ifdef X\n"
            "#endif\n"
            "#ifdef Y\n"
            "#endif\n"
            "#ifndef X\n"
            "#endif\n"
            "#ifndef Y\n"
            "#endif\n"
            "#ifdef ABSENT\n"
            "#endif\n"
            "#ifndef ABSENT2\n"
            "#endif\n"
            "#if defined(ABSENT3)\n"
            "#endif\n"
            "#if defined(X)\n"
            "#endif\n"
            "#if defined(X) || defined(Y)\n"
            "#endif\n"
            ;
    pp.run(QLatin1String("<stdin>"), source);
    QHash<QByteArray, QList<unsigned> > definitionsResolvedFromLines =
            client.definitionsResolvedFromLines();
    QCOMPARE(definitionsResolvedFromLines["X"], QList<unsigned>() << 3 << 7 << 17 << 19);
    QCOMPARE(definitionsResolvedFromLines["Y"], QList<unsigned>() << 5 << 9 << 19);
    QCOMPARE(client.unresolvedDefines(), QSet<QByteArray>() << "ABSENT" << "ABSENT2" << "ABSENT3");
}

void tst_Preprocessor::dont_eagerly_expand_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("output");

    // Expansion must be processed upon invocation of the macro. Therefore a particular
    // identifier within a define must not be expanded (in the case it matches an
    // already known macro) during the processor directive handling, but only when
    // it's actually "used". Naturally, if it's still not replaced after an invocation
    // it should then be expanded. This is consistent with clang and gcc for example.

    QTest::newRow("case 1") << _(
            "#define T int\n"
            "#define FOO(T) T\n"
            "FOO(double)\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "# expansion begin 31,3 3:4\n"
            "double\n"
            "# expansion end\n"
            "# 4 \"<stdin>\"\n"
    );

    QTest::newRow("case 2") << _(
            "#define T int\n"
            "#define FOO(X) T\n"
            "FOO(double)\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "# expansion begin 31,3 ~1\n"
            "int\n"
            "# expansion end\n"
            "# 4 \"<stdin>\"\n"
    );
}

void tst_Preprocessor::dont_eagerly_expand()
{
    compare_input_output();
}

void tst_Preprocessor::comments_within()
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, without_comments);
    QFETCH(QByteArray, with_comments);

    Environment env;
    Preprocessor preprocess(0, &env);
    preprocess.setKeepComments(false);
    QByteArray prep = preprocess.run(QLatin1String("<stdin>"), input);
    QVERIFY(compare(prep, without_comments));
    preprocess.setKeepComments(true);
    prep = preprocess.run(QLatin1String("<stdin>"), input);
    QVERIFY(compare(prep, with_comments));
}

void tst_Preprocessor::comments_within_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("without_comments");
    QTest::addColumn<QByteArray>("with_comments");

    QTest::newRow("case 1") << _(
            "#define FOO int x;\n"
            "\n"
            "   // comment\n"
            "   // comment\n"
            "   // comment\n"
            "   // comment\n"
            "FOO\n"
            "x = 10\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "# expansion begin 76,3 ~3\n"
            "int x;\n"
            "# expansion end\n"
            "# 8 \"<stdin>\"\n"
            "x = 10\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "   // comment\n"
            "   // comment\n"
            "   // comment\n"
            "   // comment\n"
            "# expansion begin 76,3 ~3\n"
            "int x;\n"
            "# expansion end\n"
            "# 8 \"<stdin>\"\n"
            "x = 10\n"
    );

    QTest::newRow("case 2") << _(
            "#define FOO int x;\n"
            "\n"
            "   /* comment\n"
            "      comment\n"
            "      comment\n"
            "      comment */\n"
            "FOO\n"
            "x = 10\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "# expansion begin 79,3 ~3\n"
            "int x;\n"
            "# expansion end\n"
            "# 8 \"<stdin>\"\n"
            "x = 10\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "   /* comment\n"
            "      comment\n"
            "      comment\n"
            "      comment */\n"
            "# expansion begin 79,3 ~3\n"
            "int x;\n"
            "# expansion end\n"
            "# 8 \"<stdin>\"\n"
            "x = 10\n"
    );

    QTest::newRow("case 3") << _(
            "#define FOO int x;\n"
            "\n"
            "   // comment\n"
            "   // comment\n"
            "   // comment\n"
            "   // comment\n"
            "FOO\n"
            "// test\n"
            "// test again\n"
            "x = 10\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "# expansion begin 76,3 ~3\n"
            "int x;\n"
            "# expansion end\n"
            "# 10 \"<stdin>\"\n"
            "x = 10\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "   // comment\n"
            "   // comment\n"
            "   // comment\n"
            "   // comment\n"
            "# expansion begin 76,3 ~3\n"
            "int x;\n"
            "# expansion end\n"
            "# 8 \"<stdin>\"\n"
            "// test\n"
            "// test again\n"
            "x = 10\n"
    );

    QTest::newRow("case 4") << _(
            "#define FOO int x;\n"
            "\n"
            "   /* comment\n"
            "      comment\n"
            "      comment\n"
            "      comment */\n"
            "FOO\n"
            "/*  \n"
            "*/\n"
            "x = 10\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "# expansion begin 79,3 ~3\n"
            "int x;\n"
            "# expansion end\n"
            "# 10 \"<stdin>\"\n"
            "x = 10\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "   /* comment\n"
            "      comment\n"
            "      comment\n"
            "      comment */\n"
            "# expansion begin 79,3 ~3\n"
            "int x;\n"
            "# expansion end\n"
            "# 8 \"<stdin>\"\n"
            "/*  \n"
            "*/\n"
            "x = 10\n"
    );

    QTest::newRow("case 5") << _(
            "#define FOO(x, y) { (void)x; (void)y; }\n"
            "\n"
            "void foo() {\n"
            "   FOO(10,\n"
            "       //comment\n"
            "       12\n"
            "}\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "void foo() {\n"
            "# expansion begin 57,3 ~4 4:7 ~4 6:7 7:0 ~2\n"
            "{ (void)10; (void)12}; }\n"
            "# expansion end\n"
            "# 8 \"<stdin>\"\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "void foo() {\n"
            "# expansion begin 57,3 ~4 4:7 ~5 6:7 7:0 ~2\n"
            "{ (void)10; (void)/*comment*/ 12}; }\n"
            "# expansion end\n"
            "# 8 \"<stdin>\"\n"
    );

    QTest::newRow("case 6") << _(
            "#define FOO(x, y) { (void)x; (void)y; }\n"
            "\n"
            "void foo() {\n"
            "   FOO(10,\n"
            "       //tricky*/comment\n"
            "       12\n"
            "}\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "void foo() {\n"
            "# expansion begin 57,3 ~4 4:7 ~4 6:7 7:0 ~2\n"
            "{ (void)10; (void)12}; }\n"
            "# expansion end\n"
            "# 8 \"<stdin>\"\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "void foo() {\n"
            "# expansion begin 57,3 ~4 4:7 ~5 6:7 7:0 ~2\n"
            "{ (void)10; (void)/*tricky*|comment*/ 12}; }\n"
            "# expansion end\n"
            "# 8 \"<stdin>\"\n"
    );

    QTest::newRow("case 7") << _(
            "#define FOO 0 //comment\n"
            "#define BAR (1 == FOO)\n"
            "void foo() {\n"
            "    if (BAR) {}\n"
            "}\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "void foo() {\n"
            "    if (\n"
            "# expansion begin 68,3 ~5\n"
            "(1 == 0)\n"
            "# expansion end\n"
            "# 4 \"<stdin>\"\n"
            "           ) {}\n"
            "}\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "              //comment\n"
            "\n"
            "void foo() {\n"
            "    if (\n"
            "# expansion begin 68,3 ~5\n"
            "(1 == 0)\n"
            "# expansion end\n"
            "# 4 \"<stdin>\"\n"
            "           ) {}\n"
            "}\n"
    );
    QTest::newRow("case 8") << _(
            "#define FOO /* comment */ 0\n"
            "FOO\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "# expansion begin 28,3 ~1\n"
            "0\n"
            "# expansion end\n"
            "# 3 \"<stdin>\"\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "            /* comment */\n"
            "# expansion begin 28,3 ~1\n"
            "0\n"
            "# expansion end\n"
            "# 3 \"<stdin>\"\n"
    );

    QTest::newRow("case 9") << _(
            "#define FOO /* comment1 */ /* comment2 */ 0\n"
            "FOO\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "# expansion begin 44,3 ~1\n"
            "0\n"
            "# expansion end\n"
            "# 3 \"<stdin>\"\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "            /* comment1 */ /* comment2 */\n"
            "# expansion begin 44,3 ~1\n"
            "0\n"
            "# expansion end\n"
            "# 3 \"<stdin>\"\n"
    );

    QTest::newRow("case 10") << _(
            "#define FOO /* comment1 */   /* comment2 */ 0 /* comment3\n"
            "comment4 */\n"
            "FOO\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "# expansion begin 70,3 ~1\n"
            "0\n"
            "# expansion end\n"
            "# 4 \"<stdin>\"\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "            /* comment1 */ /* comment2 */ /* comment3\n"
            "comment4 */\n"
            "# expansion begin 70,3 ~1\n"
            "0\n"
            "# expansion end\n"
            "# 4 \"<stdin>\"\n"
    );

    QTest::newRow("case 11") << _(
            "#include <foo.h> // comment\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "                 // comment\n"
    );

    QTest::newRow("joined") << _(
            "// comment \\\n"
            "\n"
            "int foo = 4;"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "int foo = 4;"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "// comment \\\n"
            "\n"
            "int foo = 4;"
    );

    QTest::newRow("joined_unterminated") << _(
            "// comment \\\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "int foo = 4;"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "# 12 \"<stdin>\"\n"
            "int foo = 4;"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "// comment \\\n"
            "\n"
            "# 12 \"<stdin>\"\n"
            "int foo = 4;"
    );

    QTest::newRow("inside_function_like_macro") << _(
            "#define /* comment */ ASSIGN1(VAR, VALUE) VAR = VALUE\n"
            "#define ASSIGN2(/* comment */ VAR, VALUE) VAR = VALUE\n"
            "#define ASSIGN3(VAR /* comment */, VALUE) VAR = VALUE\n"
            "#define ASSIGN4(VAR, /* comment */ VALUE) VAR = VALUE\n"
            "#define ASSIGN5(VAR, VALUE /* comment */) VAR = VALUE\n"
            "#define ASSIGN6(VAR, VALUE) /* comment */ VAR = VALUE\n"
            "#define ASSIGN7(VAR, ... /* comment */) VAR\n"
            "void func()\n"
            "{\n"
            "    int i;\n"
            "    ASSIGN1(i, 3);\n"
            "    ASSIGN2(i, 3);\n"
            "    ASSIGN3(i, 3);\n"
            "    ASSIGN4(i, 3);\n"
            "    ASSIGN5(i, 3);\n"
            "    ASSIGN6(i, 3);\n"
            "    ASSIGN7(i, 3);\n"
            "}\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "void func()\n"
            "{\n"
            "    int i;\n"
            "# expansion begin 397,7 11:12 ~1 11:15\n"
            "i = 3\n"
            "# expansion end\n"
            "# 11 \"<stdin>\"\n"
            "                 ;\n"
            "# expansion begin 416,7 12:12 ~1 12:15\n"
            "i = 3\n"
            "# expansion end\n"
            "# 12 \"<stdin>\"\n"
            "                 ;\n"
            "# expansion begin 435,7 13:12 ~1 13:15\n"
            "i = 3\n"
            "# expansion end\n"
            "# 13 \"<stdin>\"\n"
            "                 ;\n"
            "# expansion begin 454,7 14:12 ~1 14:15\n"
            "i = 3\n"
            "# expansion end\n"
            "# 14 \"<stdin>\"\n"
            "                 ;\n"
            "# expansion begin 473,7 15:12 ~1 15:15\n"
            "i = 3\n"
            "# expansion end\n"
            "# 15 \"<stdin>\"\n"
            "                 ;\n"
            "# expansion begin 492,7 16:12 ~1 16:15\n"
            "i = 3\n"
            "# expansion end\n"
            "# 16 \"<stdin>\"\n"
            "                 ;\n"
            "# expansion begin 511,7 17:12\n"
            "i\n"
            "# expansion end\n"
            "# 17 \"<stdin>\"\n"
            "                 ;\n"
            "}\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "        /* comment */\n"
            "                /* comment */\n"
            "                    /* comment */\n"
            "                     /* comment */\n"
            "                           /* comment */\n"
            "                            /* comment */\n"
            "                         /* comment */\n"
            "void func()\n"
            "{\n"
            "    int i;\n"
            "# expansion begin 397,7 11:12 ~1 11:15\n"
            "i = 3\n"
            "# expansion end\n"
            "# 11 \"<stdin>\"\n"
            "                 ;\n"
            "# expansion begin 416,7 12:12 ~1 12:15\n"
            "i = 3\n"
            "# expansion end\n"
            "# 12 \"<stdin>\"\n"
            "                 ;\n"
            "# expansion begin 435,7 13:12 ~1 13:15\n"
            "i = 3\n"
            "# expansion end\n"
            "# 13 \"<stdin>\"\n"
            "                 ;\n"
            "# expansion begin 454,7 14:12 ~1 14:15\n"
            "i = 3\n"
            "# expansion end\n"
            "# 14 \"<stdin>\"\n"
            "                 ;\n"
            "# expansion begin 473,7 15:12 ~1 15:15\n"
            "i = 3\n"
            "# expansion end\n"
            "# 15 \"<stdin>\"\n"
            "                 ;\n"
            "# expansion begin 492,7 16:12 ~1 16:15\n"
            "i = 3\n"
            "# expansion end\n"
            "# 16 \"<stdin>\"\n"
            "                 ;\n"
            "# expansion begin 511,7 17:12\n"
            "i\n"
            "# expansion end\n"
            "# 17 \"<stdin>\"\n"
            "                 ;\n"
            "}\n"
    );
}

void tst_Preprocessor::comments_before_args()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    preprocess.setKeepComments(true);
    QByteArray preprocessed = preprocess.run(QLatin1String("<stdin>"),
                                                "#define foo(a,b) int a = b;\n"
                                                "foo/*C comment*/(a,1)\n"
                                                "foo/**Doxygen comment*/(b,2)\n"
                                                "foo//C++ comment\n"
                                                "(c,3)\n"
                                                "foo///Doxygen C++ comment\n"
                                                "(d,4)\n"
                                                "foo/*multiple*///comments\n"
                                                "/**as well*/(e,5)\n",
                                             true, false);

//    DUMP_OUTPUT(preprocessed);
    QByteArray expected =
            "\n"
            "   /*C comment*/int a = 1;\n"
            "   /**Doxygen comment*/int b = 2;\n"
            "   //C++ comment\n"
            "int\n"
            "c = 3;\n"
            "   ///Doxygen C++ comment\n"
            "int\n"
            "d = 4;\n"
            "   /*multiple*/               //comments\n"
            "/**as well*/ int\n"
            "e = 5;\n";
    QVERIFY(compare(preprocessed, expected));
}

void tst_Preprocessor::multiline_strings()
{
    compare_input_output();
}

void tst_Preprocessor::multiline_strings_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("output");

    QTest::newRow("case 1") << _(
            "const char *s = \"abc\\\n"
            "xyz\";\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "const char *s = \"abc\\\n"
            "xyz\";\n"
    );
}

void tst_Preprocessor::skip_unknown_directives()
{
    compare_input_output();
}

void tst_Preprocessor::skip_unknown_directives_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("output");

    // We should skip "weird" things when preprocessing. Particularly useful when we preprocess
    // a particular expression from a document which has already been processed.

    QTest::newRow("case 1") << _(
            "# foo\n"
            "# 10 \"file.cpp\"\n"
            "# ()\n"
            "#\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
            "\n"
            "\n"
    );
}

void tst_Preprocessor::include_guard()
{
    QFETCH(QString, includeGuard);
    QFETCH(QString, input);

    QByteArray output;
    Environment env;
    MockClient client(&env, &output);
    Preprocessor preprocess(&client, &env);
    preprocess.setKeepComments(true);
    /*QByteArray prep =*/ preprocess.run(QLatin1String("<test-case>"), input);
    QCOMPARE(QString::fromUtf8(client.includeGuard()), includeGuard);
}

void tst_Preprocessor::include_guard_data()
{
    QTest::addColumn<QString>("includeGuard");
    QTest::addColumn<QString>("input");

    QTest::newRow("basic-test") << "BASIC_TEST"
                                << "#ifndef BASIC_TEST\n"
                                   "#define BASIC_TEST\n"
                                   "\n"
                                   "#endif // BASIC_TEST\n";
    QTest::newRow("comments-1") << "GUARD"
                                << "/* some\n"
                                   " * copyright\n"
                                   " * header.\n"
                                   " */\n"
                                   "#ifndef GUARD\n"
                                   "#define GUARD\n"
                                   "\n"
                                   "#endif // GUARD\n";
    QTest::newRow("comments-2") << "GUARD"
                                << "#ifndef GUARD\n"
                                   "#define GUARD\n"
                                   "\n"
                                   "#endif // GUARD\n"
                                   "/* some\n"
                                   " * trailing\n"
                                   " * comments.\n"
                                   " */\n"
                                   ;
    QTest::newRow("nested-ifdef") << "GUARD"
                                  << "#ifndef GUARD\n"
                                     "#define GUARD\n"
                                     "#ifndef NOT_GUARD\n"
                                     "#define NOT_GUARD\n"
                                     "#endif // NOT_GUARD\n"
                                     "\n"
                                     "#endif // GUARD\n"
                                     ;
    QTest::newRow("leading-tokens") << ""
                                    << "int i;\n"
                                       "#ifndef GUARD\n"
                                       "#define GUARD\n"
                                       "\n"
                                       "#endif // GUARD\n"
                                       ;
    QTest::newRow("trailing-tokens") << ""
                                     << "#ifndef GUARD\n"
                                        "#define GUARD\n"
                                        "\n"
                                        "#endif // GUARD\n"
                                        "int i;\n"
                                        ;
    QTest::newRow("surprising-but-correct") << "GUARD"
                                            << "#ifndef GUARD\n"
                                               "int i;\n"
                                               "\n"
                                               "#define GUARD\n"
                                               "#endif // GUARD\n"
                                               ;
    QTest::newRow("incomplete-1") << ""
                                  << "#ifndef GUARD\n"
                                     ;
    QTest::newRow("incomplete-2") << "GUARD"
                                  << "#ifndef GUARD\n"
                                     "#define GUARD\n"
                                     ;
}

void tst_Preprocessor::empty_trailing_lines()
{
    compare_input_output();
}

void tst_Preprocessor::empty_trailing_lines_data()
{
    // Test if the number of lines at the end of a file is correct. This is important to make the
    // EOF token for the end up at the correct line.

    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("output");

    const QByteArray original =
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n";

    QTest::newRow("9 empty lines")
            << original
            << _("# 1 \"<stdin>\"\n") + original;

    QTest::newRow("11 empty lines") << _(
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "# 11 \"<stdin>\"\n"
    );

    QTest::newRow("1 include") << _(
            "#include <something>\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
    );

    QTest::newRow("1 empty line with 1 include") << _(
            "#include <something>\n"
            "\n"
        ) << _(
            "# 1 \"<stdin>\"\n"
            "\n"
            "\n"
    );
}

void tst_Preprocessor::undef()
{
    Environment env;
    QByteArray output;
    MockClient client(&env, &output);
    Preprocessor preprocess(&client, &env);
    QByteArray input =
            "#define FOO\n"
            "#define FOO2\n"
            "#undef FOO\n"
            "#undef BAR\n";
    preprocess.run(QLatin1String("<stdin>"), input);
    QCOMPARE(env.macroCount(), 4U);
    Macro *macro = env.macroAt(0);
    QCOMPARE(macro->name(), QByteArray("FOO"));
    QCOMPARE(macro->bytesOffset(), 8U);
    QCOMPARE(macro->line(), 1U);
    QVERIFY(!macro->isHidden());
    macro = env.macroAt(1);
    QCOMPARE(macro->name(), QByteArray("FOO2"));
    QCOMPARE(macro->bytesOffset(), 20U);
    QCOMPARE(macro->line(), 2U);
    QVERIFY(!macro->isHidden());
    macro = env.macroAt(2);
    QCOMPARE(macro->name(), QByteArray("FOO"));
    QCOMPARE(macro->bytesOffset(), 32U);
    QCOMPARE(macro->line(), 3U);
    QVERIFY(macro->isHidden());
    macro = env.macroAt(3);
    QCOMPARE(macro->name(), QByteArray("BAR"));
    QCOMPARE(macro->bytesOffset(), 43U);
    QCOMPARE(macro->line(), 4U);
    QVERIFY(macro->isHidden());
    QList<QByteArray> macros = client.definedMacros();
    QVERIFY(macros.contains("FOO"));
    QVERIFY(macros.contains("FOO2"));
    QCOMPARE(client.macroUsesLine()["FOO"], (QList<unsigned>() << 3U));
    QVERIFY(client.macroUsesLine()["BAR"].isEmpty());
}

void tst_Preprocessor::concat()
{
    Environment env;
    Preprocessor preprocess(0, &env);
    QByteArray input =
            "#define concat(x,y) x ## y\n"
            "#define xconcat(x, y) concat(x, y)\n"
            "#define FOO 42\n"
            "int var1 = concat(0x, FOO);\n"
            "int var2 = xconcat(0x, FOO);\n";
    QByteArray prep = preprocess.run(QLatin1String("<stdin>"), input);
    const QByteArray output = _(
        "# 1 \"<stdin>\"\n"
        "\n"
        "\n"
        "\n"
        "int var1 =\n"
        "# expansion begin 88,6 ~1\n"
        "0xFOO\n"
        "# expansion end\n"
        "# 4 \"<stdin>\"\n"
        "                          ;\n"
        "int var2 =\n"
        "# expansion begin 116,7 ~1\n"
        "0x42\n"
        "# expansion end\n"
        "# 5 \"<stdin>\"\n"
        "                           ;\n"
    );
    QVERIFY(compare(prep, output));
}

void tst_Preprocessor::excessive_nesting()
{
    Environment env;
    Preprocessor preprocess(0, &env);
    QByteArray input;
    const QByteArray output =
        "# 1 \"<stdin>\"\n"
        "# 2001 \"<stdin>\"\n";
    for (int i = 0; i < 1000; ++i)
        input += "#if FOO\n";
    for (int i = 0; i < 1000; ++i)
        input += "#endif\n";
    QByteArray prep = preprocess.run(QLatin1String("<stdin>"), input);
    // Output cannot be precisely determined, but it shouldn't crash.
    QCOMPARE(prep, output);
}

void tst_Preprocessor::multi_byte_code_point_in_expansion()
{
    Environment env;
    Preprocessor preprocess(0, &env);
    const QByteArray input =
        "#define FOO(x) x\n"
        "FOO(arg" UC_U00FC "\n)\n";

    const QByteArray actual = preprocess.run(QLatin1String("<stdin>"), input);

    const QByteArray expected =
        "# 1 \"<stdin>\"\n"
        "\n"
        "# expansion begin 17,3 2:4\n"
        "arg" UC_U00FC "\n"
        "# expansion end\n"
        "# 4 \"<stdin>\"\n";
    QCOMPARE(actual, expected);
}

void tst_Preprocessor::compare_input_output(bool keepComments)
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, output);

    Environment env;
    Preprocessor preprocess(0, &env);
    preprocess.setKeepComments(keepComments);
    QByteArray prep = preprocess.run(QLatin1String("<stdin>"), input);
    QVERIFY(compare(prep, output));
}

QTEST_APPLESS_MAIN(tst_Preprocessor)

#include "tst_preprocessor.moc"
