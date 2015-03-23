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

#include <QDir>
#include <QProcess>
#include <QString>
#include <QtTest>

#include <iostream>

using namespace std;

enum Flags
{
    NoFlags = 0,
    Optimize = 1 << 1,
    DebugInfo = 1 << 2
};

struct Case
{
    Case() {}
    Case(const QByteArray &f, const QByteArray &c)
        : file(f), code(c)
    {}

    QByteArray file;
    QByteArray code;
    QByteArray gist;
    QByteArray extraCxxFlags;
    bool useExceptions;
};

struct Suite
{
    Suite() : flags(0) {}

    void clear() { cases.clear(); }

    QByteArray title;
    int flags;
    QByteArray cmd;
    QVector<Case> cases;
};

Q_DECLARE_METATYPE(Case)
Q_DECLARE_METATYPE(Suite)

struct TempStuff
{
    TempStuff() : buildTemp(QLatin1String("qt_tst_codesize"))
    {
        buildPath = QDir::currentPath() + QLatin1Char('/')  + buildTemp.path();
        buildTemp.setAutoRemove(false);
        QVERIFY(!buildPath.isEmpty());
    }

    QByteArray input;
    QTemporaryDir buildTemp;
    QString buildPath;
};

class tst_CodeSize : public QObject
{
    Q_OBJECT

public:
    tst_CodeSize()
    {
        m_makeBinary = "make";
#ifdef Q_OS_WIN
#  ifdef Q_CC_MSVC
        m_makeBinary = "nmake";
#  else
        m_makeBinary = "mingw32-make";
#  endif
#endif
        m_keepTemp = false;
        m_forceKeepTemp = false;
    }

private slots:
    void initTestCase();

    void codesize();
    void codesize_data();
    void init();
    void cleanup();

private:
    void disarm() { t->buildTemp.setAutoRemove(!keepTemp()); }
    bool keepTemp() const { return m_keepTemp || m_forceKeepTemp; }
    TempStuff *t;
    QByteArray m_qmakeBinary;
    QProcessEnvironment m_env;
    QString m_makeBinary;
    bool m_keepTemp;
    bool m_forceKeepTemp;
};

void tst_CodeSize::initTestCase()
{
    m_qmakeBinary = qgetenv("QTC_QMAKE_PATH_FOR_TEST");
    if (m_qmakeBinary.isEmpty())
        m_qmakeBinary = "qmake";
    qDebug() << "QMake              : " << m_qmakeBinary.constData();

    m_forceKeepTemp = qgetenv("QTC_KEEP_TEMP_FOR_TEST").toInt();
    qDebug() << "Force keep temp    : " << m_forceKeepTemp;
}

void tst_CodeSize::init()
{
    t = new TempStuff();
}

void tst_CodeSize::cleanup()
{
    if (!t->buildTemp.autoRemove()) {
        QFile logger(t->buildPath + QLatin1String("/input.txt"));
        logger.open(QIODevice::ReadWrite);
        logger.write(t->input);
    }
    delete t;
}

void tst_CodeSize::codesize()
{
    QFETCH(Suite, suite);
    static int suiteCount = 0;
    ++suiteCount;

    QFile bigPro(t->buildPath + QLatin1String("/doit.pro"));
    QVERIFY(bigPro.open(QIODevice::ReadWrite));
    bigPro.write("\nTEMPLATE = subdirs\n");
    bigPro.write("\nCONFIG += ordered\n");

    QFile mainPro(t->buildPath + "/main.pro");
    QVERIFY(mainPro.open(QIODevice::ReadWrite));
    mainPro.write("\n\nSOURCES += main.cpp");
    mainPro.write("\nCONFIG += c++11\n");

    QFile mainCpp(t->buildPath + QLatin1String("/main.cpp"));
    QVERIFY(mainCpp.open(QIODevice::ReadWrite));
    mainCpp.write("\n"
               "int main()\n"
               "{\n"
               "}\n");
    mainCpp.close();

    foreach (const Case &c, suite.cases) {
        QByteArray caseProName = c.file + ".pro";
        bigPro.write("\nSUBDIRS += " + caseProName);
        mainPro.write("\nLIBS += -l" + c.file);

        QFile casePro(t->buildPath + '/' + caseProName);
        QVERIFY(casePro.open(QIODevice::ReadWrite));
        casePro.write("\nTEMPLATE = lib");
        casePro.write("\nTARGET = " + c.file + "\n");
        casePro.write("\nCONFIG += staticlib\n");
        casePro.write("\nCONFIG += c++11\n");
        casePro.write("\nCONFIG -= app_bundle\n");

        if (suite.flags & Optimize) {
            casePro.write("\nCONFIG -= debug");
            casePro.write("\nCONFIG += release");
        } else {
            casePro.write("\nCONFIG -= release");
            casePro.write("\nCONFIG += debug");
        }

        casePro.write("\nSOURCES += " + c.file + ".cpp");
        if (!c.extraCxxFlags.isEmpty())
            casePro.write("\nQMAKE_CXXFLAGS += " + c.extraCxxFlags);

        QFile caseCpp(t->buildPath + QLatin1Char('/') + QLatin1String(c.file + ".cpp"));
        QVERIFY(caseCpp.open(QIODevice::ReadWrite));
        QByteArray fullCode = c.code;
        caseCpp.write(fullCode);
        caseCpp.close();
    }

#ifdef Q_OS_WIN
    mainPro.write("\nLIBS += -L$$OUT_PWD\\release");
#else
    mainPro.write("\nLIBS += -L$$OUT_PWD");
#endif
    mainPro.close();

    bigPro.write("\nSUBDIRS += main.pro");
    bigPro.close();

    QProcess qmake;
    qmake.setWorkingDirectory(t->buildPath);
    QString cmd = QString::fromLatin1(m_qmakeBinary + " -r doit.pro");
    qDebug() << "Starting qmake: " << cmd;
    qmake.start(cmd);
//    QVERIFY(qmake.waitForFinished());
    qmake.waitForFinished();
    QByteArray output = qmake.readAllStandardOutput();
    QByteArray error = qmake.readAllStandardError();
    //qDebug() << "stdout: " << output;
    if (!error.isEmpty()) { qDebug() << error; QVERIFY(false); }

    QProcess make;
    make.setWorkingDirectory(t->buildPath);
    make.setProcessEnvironment(m_env);

    make.start(m_makeBinary, QStringList());
    QVERIFY(make.waitForFinished());
    output = make.readAllStandardOutput();
    error = make.readAllStandardError();
    //qDebug() << "stdout: " << output;
    if (make.exitCode()) {
        qDebug() << error;
//        qDebug() << "\n------------------ CODE --------------------";
//        qDebug() << fullCode;
//        qDebug() << "\n------------------ CODE --------------------";
        qDebug() << ".pro: " << qPrintable(bigPro.fileName());
    }

    cout << "\n################################################################\n\n"
         << suite.title.data();

    bool ok = true;
    int i = 0;
    foreach (const Case &c, suite.cases) {
        ++i;
        cout << "\n\n===================== VARIANT " << suiteCount << '.' << i << ' '
             << " ================================"
             << "\n\nCode:            " << c.gist.data()
             << "\nOptimized:       " << ((suite.flags & Optimize) ? "Yes" : "No")
             << "\nExtra CXX Flags: " << c.extraCxxFlags.data();
#ifdef Q_OS_WIN
#    ifdef Q_CC_MSVC
        QByteArray finalCommand = suite.cmd + ' ' + "release\\" + c.file + ".obj";
#    else
        QByteArray finalCommand = suite.cmd + ' ' + "release\\" + c.file + ".o";
#    endif
#else
        QByteArray finalCommand = suite.cmd + ' ' + c.file + ".o";
#endif
        QProcess final;
        final.setWorkingDirectory(t->buildPath);
        final.setProcessEnvironment(m_env);
        cout << "\nCommand:         " << finalCommand.data()
             << "\n\n--------------------- OUTPUT " << suiteCount << '.' << i << ' '
             << " ---------------------------------\n\n";
        final.start(finalCommand);
        QVERIFY(final.waitForFinished());
        QByteArray stdOut = final.readAllStandardOutput();
        QByteArray stdErr = final.readAllStandardError();

        cout << stdOut.data();
        if (!stdErr.isEmpty()) {
            ok = false;
            qDebug() << "ERR: " << stdErr;
            break;
        }
    }
    cout << "\n#################################################################\n";

    QVERIFY(ok);
    disarm();
}

void tst_CodeSize::codesize_data()
{
    QTest::addColumn<Suite>("suite");

    Case c;
    Suite s;
    s.flags = Optimize;

// FIXME: Cannot be hardcoded. Assume matching qmake for now.
#ifdef Q_CC_MSVC
    s.cmd = "dumpbin /DISASM /SECTION:.text";
#else
    s.cmd = "objdump -D -j.text";
#endif
    s.title = "This 'test' compares different approaches to return something \n"
              "like an immutable string from a function.";
    c.file = "latin1string";
    c.gist = "QString f1() { return QLatin1String(\"foo\"); }\n";
    c.code = "#include <QString>\n" + c.gist;
    s.cases.append(c);

    c.file = "qstringliteral";
    c.gist = "QString f2() { return QStringLiteral(\"foo\"); }\n";
    c.code = "#include <QString>\n" + c.gist;
    s.cases.append(c);

    c.file = "std_string";
    c.gist = "std::string f3() { return \"foo\"; }\n";
    c.code = "#include <string>\n" + c.gist;
    s.cases.append(c);

    c.file = "std_string_2";
    c.gist = "std::string f4() { return \"foo\"; }\n";
    c.code = "#include <string>\n" + c.gist;
    c.extraCxxFlags = "-fno-stack-protector";
    s.cases.append(c);

    c.file = "char_pointer";
    c.gist = "const char *f5() { return \"foo\"; }\n";
    c.code = c.gist;
    s.cases.append(c);

    QTest::newRow("return_string") << s;
    s.clear();


    c.file = "one";
    c.gist = "one";
    c.code = "#include <functional>\n"
             "int x(int a) { return a * a; }\n"
             "void bar(std::function<int()>);\n"
             "void foo() { bar([] { return x(1); }); }\n";
    s.cases.append(c);

    c.file = "two";
    c.gist = "two";
    c.code = "#include <functional>\n"
             "int x(int a) { return a * a; }\n"
             "void bar(std::function<int()>);\n"
             "void foo1() { int a = 1; bar([a] { return x(a); }); }\n"
             "void foo2() { int a = 2; bar([a] { return x(a); }); }\n";
    s.cases.append(c);

    QTest::newRow("lambdas") << s;
    s.clear();


    QByteArray std_tie_code =
        "struct QMakeStepConfig\n"
        "{\n"
        "    enum TargetArchConfig { NoArch, X86, X86_64, PPC, PPC64 };\n"
        "    enum OsType { NoOsType, IphoneSimulator, IphoneOS };\n"
        "\n"
        "    QMakeStepConfig()\n"
        "        : e1(NoArch),\n"
        "          e2(NoOsType),\n"
        "          b1(false),\n"
        "          b2(false),\n"
        "          b3(false),\n"
        "          b4(false)\n"
        "    {}\n"
        "\n"
        "    TargetArchConfig e1;\n"
        "    OsType e2;\n"
        "    bool b1;\n"
        "    bool b2;\n"
        "    bool b3;\n"
        "    bool b4;\n"
        "};\n";

    c.file = "std__tie";
    c.gist = "std::tie";
    c.code = "#include <tuple>\n" + std_tie_code +
        "bool operator ==(const QMakeStepConfig &a, const QMakeStepConfig &b) {\n"
        "    return std::tie(a.e1, a.e2, a.b1, a.b2, a.b3, a.b4)\n"
        "        == std::tie(b.e1, b.e2, b.b1, b.b2, b.b3, b.b4);\n"
        "}\n";
    s.cases.append(c);

    c.file = "conventional";
    c.gist = "conventional";
    c.code = "" + std_tie_code +
        "bool operator ==(const QMakeStepConfig &a, const QMakeStepConfig &b) {\n"
        "    return a.e1 == b.e1 && a.e2 == b.e2 && a.b1 == b.b1\n"
        "        && a.b2 == b.b2 && a.b3 == b.b3 && a.b4 == b.b4;\n"
        "}\n";
    s.cases.append(c);

    QTest::newRow("std_tie") << s;
    s.clear();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    tst_CodeSize test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_codesize.moc"
