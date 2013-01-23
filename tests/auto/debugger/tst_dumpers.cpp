/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerprotocol.h"

#include <QtTest>
#include <QTemporaryFile>
#include <QTemporaryDir>

using namespace Debugger;
using namespace Internal;

class Data
{
public:
    Data() {}

    Data(const QByteArray &includes, const QByteArray &code,
        const QByteArray &iname, const QByteArray &name,
        const QByteArray &value, const QByteArray &type)
        : includes(includes), code(code), iname(iname),
          expectedName(name), expectedValue(value), expectedType(type)
    {}

public:
    QByteArray includes;
    QByteArray code;
    QByteArray iname;

    QByteArray expectedName;
    QByteArray expectedValue;
    QByteArray expectedType;
};

Q_DECLARE_METATYPE(Data)

class tst_Dumpers : public QObject
{
    Q_OBJECT

public:
    tst_Dumpers() {}

private slots:
    void dumper();
    void dumper_data();
};

void tst_Dumpers::dumper()
{
    QFETCH(Data, data);

    bool ok;
    QString cmd;
    QByteArray output;
    QByteArray error;

    QTemporaryDir buildDir(QLatin1String("qt_tst_dumpers_"));
    const bool keepTemp = qgetenv("QTC_KEEP_TEMP").toInt();
    buildDir.setAutoRemove(!keepTemp);
    //qDebug() << "Temp dir: " << buildDir.path();
    QVERIFY(!buildDir.path().isEmpty());

    QFile proFile(buildDir.path() + QLatin1String("/doit.pro"));
    ok = proFile.open(QIODevice::ReadWrite);
    QVERIFY(ok);
    proFile.write("SOURCES = main.cpp\nTARGET = doit\n");
    proFile.close();

    QFile source(buildDir.path() + QLatin1String("/main.cpp"));
    ok = source.open(QIODevice::ReadWrite);
    QVERIFY(ok);
    source.write(data.includes +
            "\n\nvoid stopper() {}"
            "\n\nint main()"
            "\n{"
            "\n    " + data.code +
            "\n    stopper();"
            "\n}\n");
    source.close();

    QProcess qmake;
    qmake.setWorkingDirectory(buildDir.path());
    cmd = QString::fromLatin1("qmake");
    //qDebug() << "Starting qmake: " << cmd;
    qmake.start(cmd);
    ok = qmake.waitForFinished();
    QVERIFY(ok);
    output = qmake.readAllStandardOutput();
    error = qmake.readAllStandardError();
    //qDebug() << "stdout: " << output;
    if (!error.isEmpty()) { qDebug() << error; QVERIFY(false); }

    QProcess make;
    make.setWorkingDirectory(buildDir.path());
    cmd = QString::fromLatin1("make");
    //qDebug() << "Starting make: " << cmd;
    make.start(cmd);
    ok = make.waitForFinished();
    QVERIFY(ok);
    output = make.readAllStandardOutput();
    error = make.readAllStandardError();
    //qDebug() << "stdout: " << output;
    if (!error.isEmpty()) { qDebug() << error; QVERIFY(false); }

    QByteArray dumperDir = DUMPERDIR;

    QByteArray cmds =
        "set confirm off\n"
        "file doit\n"
        "break stopper\n"
        "set auto-load python-scripts off\n"
        "python execfile('" + dumperDir + "/bridge.py')\n"
        "python execfile('" + dumperDir + "/dumper.py')\n"
        "python execfile('" + dumperDir + "/qttypes.py')\n"
        "bbsetup\n"
        "run\n"
        "up\n"
        "python print('@%sS@%s@' % ('N', qtNamespace()))\n"
        "bb options:fancy,autoderef,dyntype vars: expanded:local typeformats:\n"
        "quit\n";

    if (keepTemp) {
        QFile logger(buildDir.path() + QLatin1String("/input.txt"));
        ok = logger.open(QIODevice::ReadWrite);
        logger.write(cmds);
    }

    QProcess debugger;
    debugger.setWorkingDirectory(buildDir.path());
    //qDebug() << "Starting debugger ";
    debugger.start(QLatin1String("gdb -i mi -quiet -nx"));
    ok = debugger.waitForStarted();
    debugger.write(cmds);
    ok = debugger.waitForFinished();
    QVERIFY(ok);
    output = debugger.readAllStandardOutput();
    //qDebug() << "stdout: " << output;
    error = debugger.readAllStandardError();
    if (!error.isEmpty()) { qDebug() << error; }

    if (keepTemp) {
        QFile logger(buildDir.path() + QLatin1String("/output.txt"));
        ok = logger.open(QIODevice::ReadWrite);
        logger.write("=== STDOUT ===\n");
        logger.write(output);
        logger.write("\n=== STDERR ===\n");
        logger.write(error);
    }

    int pos1 = output.indexOf("data=");
    QVERIFY(pos1 != -1);
    int pos2 = output.indexOf(",typeinfo", pos1);
    QVERIFY(pos2 != -1);
    QByteArray contents = output.mid(pos1, pos2 - pos1);
    contents.replace("\\\"", "\"");

    int pos3 = output.indexOf("@NS@");
    QVERIFY(pos3 != -1);
    pos3 += sizeof("@NS@") - 1;
    int pos4 = output.indexOf("@", pos3);
    QVERIFY(pos4 != -1);
    QByteArray nameSpace = output.mid(pos3, pos4 - pos3);
    //qDebug() << "FOUND NS: " << nameSpace;
    if (nameSpace == "::")
        nameSpace.clear();
    data.expectedType.replace("@NS@", nameSpace);

    GdbMi actual;
    actual.fromString(contents);
    ok = false;
    QList<QByteArray> inames;
    foreach (const GdbMi &child, actual.children()) {
        QByteArray iname = child.findChild("iname").data();
        inames.append(child.name());
        if (iname == data.iname) {
            QByteArray name = child.findChild("name").data();
            QByteArray encvalue = child.findChild("value").data();
            int encoding = child.findChild("valueencoded").data().toInt();
            QByteArray value = decodeData(encvalue, encoding).toLatin1();
            QByteArray type = child.findChild("type").data();
            if (name != data.expectedName) {
                qDebug() << "NAME ACTUAL  : " << name;
                qDebug() << "NAME EXPECTED: " << data.expectedName;
                QVERIFY(false);
            }
            if (value != data.expectedValue) {
                qDebug() << "VALUE ACTUAL  : " << value;
                qDebug() << "VALUE EXPECTED: " << data.expectedValue;
                QVERIFY(false);
            }
            if (type != data.expectedType) {
                qDebug() << "TYPE ACTUAL  : " << type;
                qDebug() << "TYPE EXPECTED: " << data.expectedType;
                QVERIFY(false);
            }
            ok = true;
            break;
        }
    }
    if (!ok) { qDebug() << "NAMES: " << inames; QVERIFY(false); }
}

void tst_Dumpers::dumper_data()
{
    QTest::addColumn<Data>("data");

    QTest::newRow("QStringRef1") << Data(
        "#include <QString>",
        "QString str = \"Hello\";\nQStringRef ref(&str, 1, 2);",
        "local.ref", "ref", "\"el\"", "@NS@QStringRef");

}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    tst_Dumpers test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_dumpers.moc"

