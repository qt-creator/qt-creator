/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <QtTest>
#include <QFile>
#include <QCoreApplication>

enum {
    Gcc11 = 1,
    Gcc98 = 2,
    Clang = 4,
    Extern = 8,

    First = Gcc11,
    Last = Extern
};

struct Data
{
    Data()
    {}

    Data(QByteArray c, uint f = 0)
        : code(c), expectedFailure(f)
    {}

    QByteArray code;
    uint expectedFailure;
};

Q_DECLARE_METATYPE(Data)

class tst_Features : public QObject
{
    Q_OBJECT

public:
    tst_Features() {}

private slots:
    void feature();
    void feature_data();
};

void tst_Features::feature()
{
    QFETCH(Data, data);

    QByteArray mainFile = "main.cpp";

    QFile source(mainFile);
    QVERIFY(source.open(QIODevice::ReadWrite | QIODevice::Truncate));
    QByteArray fullCode =
        "#include <vector>\n"
        "#include <string>\n\n"
        "using namespace std;\n\n"
        + data.code + "\n";
    source.write(fullCode);
    source.close();

    for (uint i = First; i <= Last; i *= 2) {
        QByteArray compiler;
        if (i & Gcc11)
            compiler = "g++ -Werror -std=c++0x -c";
        else if (i & Gcc98)
            compiler = "g++ -Werror -std=c++98 -c";
        else if (i & Clang)
            compiler = "clang++ -Werror -std=c++11 -c";
        else if (i & Extern)
            compiler = qgetenv("QTC_COMPILER_PATH");

        if (compiler.isEmpty())
            continue;

        QProcess proc;
        proc.start(compiler + " " + mainFile);
        proc.waitForFinished();
        QByteArray output = proc.readAllStandardOutput();
        QByteArray error = proc.readAllStandardError();
        bool compileFailure = proc.exitCode() != 0;
        bool ok = compileFailure == bool(data.expectedFailure & i);
        if (!ok) {
            qDebug() << "\n------------------ CODE --------------------";
            qDebug() << fullCode;
            //qDebug() << "\n------------------ CODE --------------------";
            //qDebug() << ".pro: " << qPrintable(proFile.fileName());
            qDebug() << "Compiler: " << compiler;
            qDebug() << "Error: " << error;
            qDebug() << "Output: " << output;
            qDebug() << "Code: " << fullCode;
            qDebug() << "\n------------------ CODE --------------------";
        }
        QVERIFY(ok);
    }
}

void tst_Features::feature_data()
{
    QTest::addColumn<Data>("data");

    // Self-test. "$" should be expected to fail in any case.
    QTest::newRow("checkfail")
            << Data("$", -1);

    QTest::newRow("auto-keyword")
            << Data("auto i = vector<int>::iterator();", Gcc98);

    QTest::newRow("ranged-for")
            << Data("int foo() { int s = 0; vector<int> v; "
                    "for (int i: v) { s += v[i]; } return s; }", Gcc98);

    QTest::newRow("in-class-member-initialization")
            << Data("struct S { int a = 1; }; int main() { S s; return s.a; }", Gcc98);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    tst_Features test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_features.moc"
