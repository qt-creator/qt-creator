/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include <QtTest>
#include <pp.h>

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

#define DUMP_OUTPUT(x)     {QByteArray b(x);qDebug("output: [[%s]]", b.replace("\n", "<<\n").constData());}


QByteArray loadSource(const QString &fileName)
{
    QFile inf(fileName);
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
    QFile inf(fileName);
    if (!inf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug("Cannot open \"%s\"", fileName.toUtf8().constData());
        return;
    }

    inf.write(data);
    inf.close();
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
QDebug &operator<<(QDebug& d, const Include &i) {
    d << '[' << i.fileName
      << ',' << (i.type == Client::IncludeGlobal ? "Global" : (i.type == Client::IncludeLocal ? "Local" : "Unknown"))
      << ',' << i.line
      << ']';
    return d;
}

class MockClient: public Client
{
public:
    struct Block {
        Block(): start(0), end(0) {}
        Block(unsigned start): start(start), end(0) {}

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

    virtual void macroAdded(const Macro &/*macro*/) {}

    virtual void passedMacroDefinitionCheck(unsigned /*offset*/, const Macro &/*macro*/) {}
    virtual void failedMacroDefinitionCheck(unsigned /*offset*/, const QByteArray &/*name*/) {}

    virtual void startExpandingMacro(unsigned /*offset*/,
                                     const Macro &/*macro*/,
                                     const QByteArray &/*originalText*/,
                                     const QVector<MacroArgumentReference> &/*actuals*/
                                              = QVector<MacroArgumentReference>()) {}

    virtual void stopExpandingMacro(unsigned /*offset*/,
                                    const Macro &/*macro*/) {}

    virtual void startSkippingBlocks(unsigned offset)
    { m_skippedBlocks.append(Block(offset)); }

    virtual void stopSkippingBlocks(unsigned offset)
    { m_skippedBlocks.last().end = offset; }

    virtual void sourceNeeded(QString &includedFileName, IncludeType mode,
                              unsigned line)
    {
#if 1
        m_recordedIncludes.append(Include(includedFileName, mode, line));
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

        m_pp.preprocess(fileName, src, m_output, nolines, true, false);
    }

    QList<Block> skippedBlocks() const
    { return m_skippedBlocks; }

    QList<Include> recordedIncludes() const
    { return m_recordedIncludes; }

private:
    Environment *m_env;
    QByteArray *m_output;
    Preprocessor m_pp;
    QList<QDir> m_includePaths;
    unsigned m_includeDepth;
    QList<Block> m_skippedBlocks;
    QList<Include> m_recordedIncludes;
};

QDebug &operator<<(QDebug& d, const MockClient::Block &b) { d << '[' << b.start << ',' << b.end << ']'; return d; }

class tst_Preprocessor: public QObject
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

private /* not corrected yet */:
    void macro_definition_lineno();

private slots:
    void va_args();
    void named_va_args();
    void first_empty_macro_arg();
    void invalid_param_count();
    void unfinished_function_like_macro_call();
    void nasty_macro_expansion();
    void tstst();
    void test_file_builtin();

    void blockSkipping();
    void includes_1();

    void comparisons_data();
    void comparisons();
};

void tst_Preprocessor::va_args()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess(QLatin1String("<stdin>"),
                                         QByteArray("#define foo(...) int f(__VA_ARGS__);\n"
                                                    "\nfoo(  )\n"
                                                    "\nfoo(int a)\n"
                                                    "\nfoo(int a,int b)\n"),
                                         true,
                                         false);

    preprocessed = preprocessed.simplified();
//    DUMP_OUTPUT(preprocessed);
    QVERIFY(preprocessed.contains("int f();"));
    QVERIFY(preprocessed.contains("int f( int a );"));
    QVERIFY(preprocessed.contains("int f( int a, int b );"));
}

void tst_Preprocessor::named_va_args()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess(QLatin1String("<stdin>"),
                                         QByteArray("\n#define foo(ARGS...) int f(ARGS);"
                                                    "\nfoo(  )\n"
                                                    "\nfoo(int a)\n"
                                                    "\nfoo(int a,int b)\n"),
                                         true, false);

    preprocessed = preprocessed.simplified();
    QVERIFY(preprocessed.contains("int f();"));
    QVERIFY(preprocessed.contains("int f( int a );"));
    QVERIFY(preprocessed.contains("int f( int a, int b );"));
}

void tst_Preprocessor::first_empty_macro_arg()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess(QLatin1String("<stdin>"),
                                         QByteArray("\n#define foo(a,b) a int b;"
                                                    "\nfoo(const,cVal)\n"
                                                    "\nfoo(,Val)\n"
                                                    "\nfoo( ,Val2)\n"),
                                         true, false);

    preprocessed = preprocessed.simplified();
//    DUMP_OUTPUT(preprocessed);
    QVERIFY(preprocessed.contains("const int cVal ;"));
    QVERIFY(preprocessed.contains("int Val ;"));
    QVERIFY(preprocessed.contains("int Val2 ;"));
}

void tst_Preprocessor::invalid_param_count()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    // The following is illegal, but shouldn't crash the preprocessor.
    // GCC says: 3:14: error: macro "foo" requires 2 arguments, but only 1 given
    QByteArray preprocessed = preprocess(QLatin1String("<stdin>"),
                                         QByteArray("\n#define foo(a,b) int f(a,b);"
                                                    "\n#define ARGS(t)  t a,t b"
                                                    "\nfoo(ARGS(int))"),
                                         true, false);
    // do not verify the output: it's illegal, so anything might be outputted.
}

void tst_Preprocessor::macro_definition_lineno()
{
    Client *client = 0; // no client.
    Environment env;
    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess(QLatin1String("<stdin>"),
                                         QByteArray("#define foo(ARGS) int f(ARGS)\n"
                                                    "foo(int a);\n"));
    QVERIFY(preprocessed.contains("#gen true\n# 2 "));

    preprocessed = preprocess(QLatin1String("<stdin>"),
                              QByteArray("#define foo(ARGS) int f(ARGS)\n"
                                         "foo(int a)\n"
                                         ";\n"));
    QVERIFY(preprocessed.contains("#gen true\n# 2 "));

    preprocessed = preprocess(QLatin1String("<stdin>"),
                              QByteArray("#define foo(ARGS) int f(ARGS)\n"
                                         "foo(int  \n"
                                         "    a);\n"));
    QVERIFY(preprocessed.contains("#gen true\n# 2 "));

    preprocessed = preprocess(QLatin1String("<stdin>"),
                              QByteArray("#define foo int f\n"
                                         "foo;\n"));
    QVERIFY(preprocessed.contains("#gen true\n# 2 "));

    preprocessed = preprocess(QLatin1String("<stdin>"),
                              QByteArray("#define foo int f\n"
                                         "foo\n"
                                         ";\n"));
    QVERIFY(preprocessed.contains("#gen true\n# 2 "));
}

void tst_Preprocessor::unfinished_function_like_macro_call()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess(QLatin1String("<stdin>"),
                                         QByteArray("\n#define foo(a,b) a + b"
                                         "\nfoo(1, 2\n"));
    QByteArray expected__("# 1 \"<stdin>\"\n\n\n    1\n#gen true\n# 2 \"<stdin>\"\n+\n#gen false\n# 3 \"<stdin>\"\n       2\n");
//    DUMP_OUTPUT(preprocessed);
    QCOMPARE(preprocessed, expected__);
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
    QByteArray preprocessed = preprocess(QLatin1String("<stdin>"), input);

    QVERIFY(!preprocessed.contains("FIELD32"));
}

void tst_Preprocessor::tstst()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess(
                QLatin1String("<stdin>"),
                QByteArray("\n"
                           "# define _GLIBCXX_VISIBILITY(V) __attribute__ ((__visibility__ (#V)))\n"
                           "namespace std _GLIBCXX_VISIBILITY(default) {\n"
                           "}\n"
                           ));
    const QByteArray result____ ="# 1 \"<stdin>\"\n\n\n"
            "namespace std\n"
            "#gen true\n"
            "# 2 \"<stdin>\"\n"
            "__attribute__ ((__visibility__ (\n"
            "\"default\"\n"
            "# 2 \"<stdin>\"\n"
            ")))\n"
            "#gen false\n"
            "# 3 \"<stdin>\"\n"
            "                                           {\n"
            "}\n";

//    DUMP_OUTPUT(preprocessed);
    QCOMPARE(preprocessed, result____);
}

void tst_Preprocessor::test_file_builtin()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess(
                QLatin1String("some-file.c"),
                QByteArray("const char *f = __FILE__\n"
                           ));
    const QByteArray result____ =
            "# 1 \"some-file.c\"\n"
            "const char *f =\n"
            "#gen true\n"
            "# 1 \"some-file.c\"\n"
            "\"some-file.c\"\n"
            "#gen false\n"
            "# 2 \"some-file.c\"\n"
            ;
    QCOMPARE(preprocessed, result____);
}

void tst_Preprocessor::comparisons_data()
{
    QTest::addColumn<QString>("infile");
    QTest::addColumn<QString>("outfile");
    QTest::addColumn<QString>("errorfile");

    QTest::newRow("do nothing") << "noPP.1.cpp" << "noPP.1.cpp" << "";
    QTest::newRow("identifier-expansion 1") << "identifier-expansion.1.cpp" << "identifier-expansion.1.out.cpp" << "";
    QTest::newRow("identifier-expansion 2") << "identifier-expansion.2.cpp" << "identifier-expansion.2.out.cpp" << "";
    QTest::newRow("identifier-expansion 3") << "identifier-expansion.3.cpp" << "identifier-expansion.3.out.cpp" << "";
    QTest::newRow("identifier-expansion 4") << "identifier-expansion.4.cpp" << "identifier-expansion.4.out.cpp" << "";
    QTest::newRow("identifier-expansion 5") << "identifier-expansion.5.cpp" << "identifier-expansion.5.out.cpp" << "";
    QTest::newRow("reserved 1") << "reserved.1.cpp" << "reserved.1.out.cpp" << "";
    QTest::newRow("recursive 1") << "recursive.1.cpp" << "recursive.1.out.cpp" << "";
    QTest::newRow("macro_pounder_fn") << "macro_pounder_fn.c" << "" << "";
    QTest::newRow("macro_expand") << "macro_expand.c" << "macro_expand.out.c" << "";
    QTest::newRow("macro-test") << "macro-test.cpp" << "macro-test.out.cpp" << "";
    QTest::newRow("empty-macro") << "empty-macro.cpp" << "empty-macro.out.cpp" << "";
    QTest::newRow("empty-macro 2") << "empty-macro.2.cpp" << "empty-macro.2.out.cpp" << "";
}

void tst_Preprocessor::comparisons()
{
    QFETCH(QString, infile);
    QFETCH(QString, outfile);
    QFETCH(QString, errorfile);

    QByteArray errors;
    QByteArray preprocessed = preprocess(infile, &errors, infile == outfile);

//    DUMP_OUTPUT(preprocessed);

    if (!outfile.isEmpty()) {
        QByteArray output____ = loadSource("data/"+outfile); // these weird underscores are here to make the name as long as "preprocessed", so the QCOMPARE error messages are nicely aligned.
        //    QCOMPARE(preprocessed, output____);
        QCOMPARE(QString::fromUtf8(preprocessed.constData()), QString::fromUtf8(output____.constData()));
    }

    if (!errorfile.isEmpty()) {
        QByteArray errorFileContents = loadSource("data/"+errorfile);
        QCOMPARE(QString::fromUtf8(errors.constData()), QString::fromUtf8(errorFileContents.constData()));
    }
}

void tst_Preprocessor::blockSkipping()
{
    QByteArray output;
    Environment env;
    MockClient client(&env, &output);
    Preprocessor pp(&client, &env);
    /*QByteArray preprocessed =*/ pp(
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
    /*QByteArray preprocessed =*/ pp(
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

QTEST_APPLESS_MAIN(tst_Preprocessor)
#include "tst_preprocessor.moc"
