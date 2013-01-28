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
#include "watchdata.h"
#include "watchutils.h"

#include <QtTest>
#include <QTemporaryFile>
#include <QTemporaryDir>

using namespace Debugger;
using namespace Internal;

static QByteArray noValue = "\001";

static QByteArray nameFromIName(const QByteArray &iname)
{
    int pos = iname.lastIndexOf('.');
    return pos == -1 ? iname : iname.mid(pos + 1);
}

static QByteArray parentIName(const QByteArray &iname)
{
    int pos = iname.lastIndexOf('.');
    return pos == -1 ? QByteArray() : iname.left(pos);
}

struct Check
{
    Check() {}

    Check(const QByteArray &iname, const QByteArray &value,
          const QByteArray &type)
        : iname(iname), expectedName(nameFromIName(iname)),
          expectedValue(value), expectedType(type)
    {}

    Check(const QByteArray &iname, const QByteArray &name,
         const QByteArray &value, const QByteArray &type)
        : iname(iname), expectedName(name),
          expectedValue(value), expectedType(type)
    {}

    QByteArray iname;
    QByteArray expectedName;
    QByteArray expectedValue;
    QByteArray expectedType;
};

struct CheckType : public Check
{
    CheckType(const QByteArray &iname, const QByteArray &name,
         const QByteArray &type)
        : Check(iname, name, noValue, type)
    {}

    CheckType(const QByteArray &iname, const QByteArray &type)
        : Check(iname, noValue, type)
    {}
};

struct Profile
{
    Profile(const QByteArray &contents) : contents(contents) {}

    QByteArray contents;
};

struct GuiProfile : public Profile
{
    GuiProfile() : Profile("QT+=gui\ngreaterThan(QT_MAJOR_VERSION, 4):QT *= widgets\n") {}
};

class Data
{
public:
    Data() {}
    Data(const QByteArray &code) : code(code) {}

    Data(const QByteArray &includes, const QByteArray &code)
        : includes(includes), code(code)
    {}

    const Data &operator%(const Check &check) const
    {
        checks.insert("local." + check.iname, check);
        return *this;
    }

    const Data &operator%(const Profile &profile) const
    {
        profileExtra += profile.contents;
        return *this;
    }

public:
    mutable QByteArray profileExtra;
    QByteArray includes;
    QByteArray code;
    mutable QMap<QByteArray, Check> checks;
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
    proFile.write("QT -= widgets gui\n");
    proFile.write(data.profileExtra);
    proFile.close();

    QFile source(buildDir.path() + QLatin1String("/main.cpp"));
    ok = source.open(QIODevice::ReadWrite);
    QVERIFY(ok);
    source.write(
            "\n\nvoid dummyStatement(...) {}"
            "\n\n" + data.includes +
            "\n\nvoid stopper() {}"
            "\n\nint main(int argc, char *argv[])"
            "\n{"
            "\n    dummyStatement(&argc, &argv);\n"
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

    QSet<QByteArray> expandedINames;
    expandedINames.insert("local");
    foreach (const Check &check, data.checks) {
        QByteArray parent = check.iname;
        while (true) {
            parent = parentIName(parent);
            if (parent.isEmpty())
                break;
            expandedINames.insert("local." + parent);
        }
    }

    QByteArray expanded;
    foreach (const QByteArray &iname, expandedINames) {
        if (!expanded.isEmpty())
            expanded.append(',');
        expanded += iname;
    }

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
        "bb options:fancy,autoderef,dyntype vars: expanded:" + expanded + " typeformats:\n"
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

    GdbMi actual;
    actual.fromString(contents);
    ok = false;
    WatchData local;
    local.iname = "local";

    QList<WatchData> list;
    foreach (const GdbMi &child, actual.children()) {
        WatchData dummy;
        dummy.iname = child.findChild("iname").data();
        dummy.name = QLatin1String(child.findChild("name").data());
        parseWatchData(expandedINames, dummy, child, &list);
    }

    foreach (const WatchData &item, list) {
        if (data.checks.contains(item.iname)) {
            Check check = data.checks.take(item.iname);
            check.expectedValue.replace(' ', "");
            check.expectedType.replace('@', nameSpace);
            check.expectedType.replace(' ', "");
            QByteArray actualValue = item.value.toLatin1();
            actualValue.replace(' ', "");
            QByteArray actualType = item.type;
            actualType.replace(' ', "");
            if (item.name.toLatin1() != check.expectedName) {
                qDebug() << "INAME        : " << item.iname;
                qDebug() << "NAME ACTUAL  : " << item.name;
                qDebug() << "NAME EXPECTED: " << check.expectedName;
                qDebug() << "CONTENTS     : " << contents;
                QVERIFY(false);
            }
            if (check.expectedValue != noValue && actualValue != check.expectedValue) {
                qDebug() << "INAME         : " << item.iname;
                qDebug() << "VALUE ACTUAL  : " << item.value;
                qDebug() << "VALUE EXPECTED: " << check.expectedValue;
                qDebug() << "CONTENTS      : " << contents;
                QVERIFY(false);
            }
            if (check.expectedType != noValue && actualType != check.expectedType) {
                qDebug() << "INAME        : " << item.iname;
                qDebug() << "TYPE ACTUAL  : " << item.type;
                qDebug() << "TYPE EXPECTED: " << check.expectedType;
                qDebug() << "CONTENTS     : " << contents;
                QVERIFY(false);
            }
        }
    }

    if (!data.checks.isEmpty()) {
        qDebug() << "SOME TESTS NOT EXECUTED: ";
        foreach (const Check &check, data.checks)
            qDebug() << "  TEST NOT FOUND FOR INAME: " << check.iname;
        qDebug() << "CONTENTS     : " << contents;
        qDebug() << "EXPANDED     : " << expanded;
        QVERIFY(false);
    }
}

void tst_Dumpers::dumper_data()
{
    QTest::addColumn<Data>("data");

    QByteArray fooData =
            "#include <QHash>\n"
            "#include <QMap>\n"
            "#include <QObject>\n"
            "#include <QString>\n"
            "class Foo\n"
            "{\n"
            "public:\n"
            "    Foo(int i = 0)\n"
            "        : a(i), b(2)\n"
            "    {}\n"
            "    virtual ~Foo()\n"
            "    {\n"
            "        a = 5;\n"
            "    }\n"
            "    void doit()\n"
            "    {\n"
            "        static QObject ob;\n"
            "        m[\"1\"] = \"2\";\n"
            "        h[&ob] = m.begin();\n"
            "        --b;\n"
            "    }\n"
            "public:\n"
            "    int a, b;\n"
            "    char x[6];\n"
            "private:\n"
            "    typedef QMap<QString, QString> Map;\n"
            "    Map m;\n"
            "    QHash<QObject *, Map::iterator> h;\n"
            "};";

    QTest::newRow("AnonymousStruct")
            << Data("union {\n"
                    "     struct { int i; int b; };\n"
                    "     struct { float f; };\n"
                    "     double d;\n"
                    " } a = { { 42, 43 } };\n (void)a;")
               % CheckType("a", "a", "union {...}")
               % Check("a.b", "43", "int")
               % Check("a.d", "9.1245819032257467e-313", "double")
               % Check("a.f", "5.88545355e-44", "float")
               % Check("a.i", "42", "int");

    QTest::newRow("QByteArray0")
            << Data("#include <QByteArray>\n",
                    "QByteArray ba;")
               % Check("ba", "ba", "\"\"", "@QByteArray");

    QTest::newRow("QByteArray1")
            << Data("#include <QByteArray>\n",
                    "QByteArray ba = \"Hello\\\"World\";\n"
                    "ba += char(0);\n"
                    "ba += 1;\n"
                    "ba += 2;\n")
               % Check("ba", "\"Hello\"World\"", "@QByteArray")
               % Check("ba.0", "[0]", "72", "char")
               % Check("ba.11", "[11]", "0", "char")
               % Check("ba.12", "[12]", "1", "char")
               % Check("ba.13", "[13]", "2", "char");

    QTest::newRow("QByteArray2")
            << Data("#include <QByteArray>\n",
                    "QByteArray ba;\n"
                    "for (int i = 256; --i >= 0; )\n"
                    "    ba.append(char(i));\n"
                    "QString s(10000, 'x');\n"
                    "std::string ss(10000, 'c');")
               % CheckType("ba", "@QByteArray")
               % Check("s", '"' + QByteArray(10000, 'x') + '"', "@QString")
               % Check("ss", '"' + QByteArray(10000, 'c') + '"', "std::string");

    QTest::newRow("QByteArray3")
            << Data("#include <QByteArray>\n",
                    "const char *str1 = \"\356\";\n"
                    "const char *str2 = \"\xee\";\n"
                    "const char *str3 = \"\\ee\";\n"
                    "QByteArray buf1(str1);\n"
                    "QByteArray buf2(str2);\n"
                    "QByteArray buf3(str3);\n")
               % Check("buf1", "\"î\"", "@QByteArray")
               % Check("buf2", "\"î\"", "@QByteArray")
               % Check("buf3", "\"\ee\"", "@ByteArray")
               % CheckType("str1", "char *");

    QTest::newRow("QByteArray4")
            << Data("#include <QByteArray>\n",
                    "char data[] = { 'H', 'e', 'l', 'l', 'o' };\n"
                    "QByteArray ba1 = QByteArray::fromRawData(data, 4);\n"
                    "QByteArray ba2 = QByteArray::fromRawData(data + 1, 4);\n")
               % Check("ba1", "\"Hell\"", "@QByteArray")
               % Check("ba2", "\"ello\"", "@QByteArray");

    QTest::newRow("QDate0")
            << Data("#include <QDate>\n",
                    "QDate date;\n")
               % CheckType("date", "@QDate")
               % Check("date.(ISO)", "", "@QString")
               % Check("date.(Locale)", "", "@QString")
               % Check("date.(SystemLocale)", "", "@QString")
               % Check("date.toString", "", "@QString");

    QTest::newRow("QDate1")
            << Data("#include <QDate>\n",
                    "QDate date;\n"
                    "date.setDate(1980, 1, 1);\n")
               % CheckType("date", "@QDate")
               % Check("date.(ISO)", "\"1980-01-01\"", "@QString")
               % CheckType("date.(Locale)", "@QString")
               % CheckType("date.(SystemLocale)", "@QString")
               % Check("date.toString", "\"Tue Jan 1 1980\"", "@QString");

    QTest::newRow("QTime0")
            << Data("#include <QTime>\n",
                    "QTime time;\n")
               % CheckType("time", "@QTime")
               % Check("time.(ISO)", "", "@QString")
               % CheckType("time.(Locale)", "@QString")
               % CheckType("time.(SystemLocale)", "@QString")
               % Check("time.toString", "", "@QString");

    QTest::newRow("QTime1")
            << Data("#include <QTime>\n",
                    "QTime time(13, 15, 32);")
               % Check("time", "13:15:32", "@QTime")
               % Check("time.(ISO)", "\"13:15:32\"", "@QString")
               % CheckType("time.(Locale)", "@QString")
               % CheckType("time.(SystemLocale)", "@QString")
               % Check("time.toString", "\"13:15:32\"", "@QString");

    QTest::newRow("QDateTime")
            << Data("#include <QDateTime>\n",
                    "QDateTime date;\n")
               % CheckType("date", "@QDateTime")
               % Check("date.(ISO)", "", "@QString")
               % Check("date.(Locale)", "", "@QString")
               % Check("date.(SystemLocale)", "", "@QString")
               % Check("date.toString", "\"\"", "@QString")
               % Check("date.toUTC", "", "@QDateTime");

    QTest::newRow("QDir")
#ifdef Q_OS_WIN
            << Data("#include <QDir>\n",
                    "QDir dir(\"C:\\\\Program Files\");")
               % Check("dir", "C:/Program Files", "@QDir")
               % Check("dir.absolutePath", "C:/Program Files", "@QString")
               % Check("dir.canonicalPath", "C:/Program Files", "@QString");
#else
            << Data("#include <QDir>\n",
                    "QDir dir(\"/tmp\"); QString s = dir.absolutePath();")
               % Check("dir", "/tmp", "@QDir")
               % Check("dir.absolutePath", "\"/tmp\"", "@QString")
               % Check("dir.canonicalPath", "\"/tmp\"", "@QString");
#endif

    QTest::newRow("QFileInfo")
#ifdef Q_OS_WIN
            << Data("#include <QFile>\n"
                    "#include <QFileInfo>\n",
                    "QFile file(\"C:\\\\Program Files\\t\");\n"
                    "file.setObjectName(\"A QFile instance\");\n"
                    "QFileInfo fi(\"C:\\Program Files\\tt\");\n"
                    "QString s = fi.absoluteFilePath();\n")
               % Check("fi", "\"C:/Program Files/tt\"", "QFileInfo")
               % Check("file", "\"C:\Program Files\t\"", "QFile")
               % Check("s", "\"C:/Program Files/tt\"", "QString");
#else
            << Data("#include <QFile>\n"
                    "#include <QFileInfo>\n",
                    "QFile file(\"/tmp/t\");\n"
                    "file.setObjectName(\"A QFile instance\");\n"
                    "QFileInfo fi(\"/tmp/tt\");\n"
                    "QString s = fi.absoluteFilePath();\n")
               % Check("fi", "\"/tmp/tt\"", "@QFileInfo")
               % Check("file", "\"/tmp/t\"", "@QFile")
               % Check("s", "\"/tmp/tt\"", "@QString");
#endif

    QTest::newRow("QHash1")
            << Data("#include <QHash>\n",
                    "QHash<QString, QList<int> > hash;\n"
                    "hash.insert(\"Hallo\", QList<int>());\n"
                    "hash.insert(\"Welt\", QList<int>() << 1);\n"
                    "hash.insert(\"!\", QList<int>() << 1 << 2);\n")
               % Check("hash", "<3 items>", "@QHash<@QString, @QList<int> >")
               % Check("hash.0", "[0]", "", "@QHashNode<@QString, @QList<int>>")
               % Check("hash.0.key", "\"Hallo\"", "@QString")
               % Check("hash.0.value", "<0 items>", "@QList<int>")
               % Check("hash.1", "[1]", "", "@QHashNode<@QString, @QList<int>>")
               % Check("hash.1.key", "key", "\"Welt\"", "@QString")
               % Check("hash.1.value", "value", "<1 items>", "@QList<int>")
               % Check("hash.1.value.0", "[0]", "1", "int")
               % Check("hash.2", "[2]", "", "@QHashNode<@QString, @QList<int>>")
               % Check("hash.2.key", "key", "\"!\"", "@QString")
               % Check("hash.2.value", "value", "<2 items>", "@QList<int>")
               % Check("hash.2.value.0", "[0]", "1", "int")
               % Check("hash.2.value.1", "[1]", "2", "int");

    QTest::newRow("QHash2")
            << Data("#include <QHash>\n",
                    "QHash<int, float> hash;\n"
                    "hash[11] = 11.0;\n"
                    "hash[22] = 22.0;\n")
               % Check("hash", "hash", "<2 items>", "@QHash<int, float>")
               % Check("hash.22", "[22]", "22", "float")
               % Check("hash.11", "[11]", "11", "float");

    QTest::newRow("QHash3")
            << Data("#include <QHash>\n",
                    "QHash<QString, int> hash;\n"
                    "hash[\"22.0\"] = 22.0;\n"
                    "hash[\"123.0\"] = 22.0;\n"
                    "hash[\"111111ss111128.0\"] = 28.0;\n"
                    "hash[\"11124.0\"] = 22.0;\n"
                    "hash[\"1111125.0\"] = 22.0;\n"
                    "hash[\"11111126.0\"] = 22.0;\n"
                    "hash[\"111111127.0\"] = 27.0;\n"
                    "hash[\"111111111128.0\"] = 28.0;\n"
                    "hash[\"111111111111111111129.0\"] = 29.0;\n")
               % Check("hash", "hash", "<9 items>", "@QHash<@QString, int>")
               % Check("hash.0", "[0]", "", "@QHashNode<@QString, int>")
               % Check("hash.0.key", "key", "\"123.0\"", "@QString")
               % Check("hash.0.value", "22", "int")
               % Check("hash.8", "[8]", "", "@QHashNode<@QString, int>")
               % Check("hash.8.key", "key", "\"11124.0\"", "@QString")
               % Check("hash.8.value", "value", "22", "int");

    QTest::newRow("QHash4")
            << Data("#include <QHash>\n",
                    "QHash<QByteArray, float> hash;\n"
                    "hash[\"22.0\"] = 22.0;\n"
                    "hash[\"123.0\"] = 22.0;\n"
                    "hash[\"111111ss111128.0\"] = 28.0;\n"
                    "hash[\"11124.0\"] = 22.0;\n"
                    "hash[\"1111125.0\"] = 22.0;\n"
                    "hash[\"11111126.0\"] = 22.0;\n"
                    "hash[\"111111127.0\"] = 27.0;\n"
                    "hash[\"111111111128.0\"] = 28.0;\n"
                    "hash[\"111111111111111111129.0\"] = 29.0;\n")
               % Check("hash", "<9 items>", "@QHash<@QByteArray, float>")
               % Check("hash.0", "[0]", "@QHashNode<@QByteArray, float>")
               % Check("hash.0.key", "\"123.0\"", "@QByteArray")
               % Check("hash.0.value", "22", "float")
               % Check("hash.8", "[8]", "@QHashNode<@QByteArray, float>")
               % Check("hash.8.key", "\"11124.0\"", "@QByteArray")
               % Check("hash.8.value", "22", "float");

    QTest::newRow("QHash5")
            << Data("#include <QHash>\n",
                    "QHash<int, QString> hash;\n"
                    "hash[22] = \"22.0\";\n")
               % Check("hash", "<1 items>", "@QHash<int, @QString>")
               % Check("hash.0", "[0]", "", "@QHashNode<int, @QString>")
               % Check("hash.0.key", "22", "int")
               % Check("hash.0.value", "\"22.0\"", "@QString");

    QTest::newRow("QHash6")
            << Data("#include <QHash>\n" + fooData,
                    "QHash<QString, Foo> hash;\n"
                    "hash[\"22.0\"] = Foo(22);\n"
                    "hash[\"33.0\"] = Foo(33);\n")
               % Check("hash", "<2 items>", "@QHash<@QString, Foo>")
               % Check("hash.0", "[0]", "", "@QHashNode<@QString, Foo>")
               % Check("hash.0.key", "\"22.0\"", "@QString")
               % CheckType("hash.0.value", "Foo")
               % Check("hash.0.value.a", "22", "int")
               % Check("hash.1", "[1]", "", "@QHashNode<@QString, Foo>")
               % Check("hash.1.key", "\"33.0\"", "@QString")
               % CheckType("hash.1.value", "Foo");

    QTest::newRow("QHash7")
            << Data("#include <QHash>\n"
                    "#include <QPointer>\n",
                    "QObject ob;\n"
                    "QHash<QString, QPointer<QObject> > hash;\n"
                    "hash.insert(\"Hallo\", QPointer<QObject>(&ob));\n"
                    "hash.insert(\"Welt\", QPointer<QObject>(&ob));\n"
                    "hash.insert(\".\", QPointer<QObject>(&ob));\n")
               % Check("hash", "<3 items>", "@QHash<@QString, @QPointer<@QObject>>")
               % Check("hash.0", "[0]", "", "@QHashNode<@QString, @QPointer<@QObject>>")
               % Check("hash.0.key", "\"Hallo\"", "@QString")
               % CheckType("hash.0.value", "@QPointer<@QObject>")
               % CheckType("hash.0.value.o", "@QObject")
               % Check("hash.2", "[2]", "", "@QHashNode<@QString, @QPointer<@QObject>>")
               % Check("hash.2.key", "\".\"", "@QString")
               % CheckType("hash.2.value", "@QPointer<@QObject>");

    QTest::newRow("QHashIntFloatIterator")
            << Data("#include <QHash>\n",
                    "typedef QHash<int, float> Hash;\n"
                    "Hash hash;\n"
                    "hash[11] = 11.0;\n"
                    "hash[22] = 22.0;\n"
                    "hash[33] = 33.0;\n"
                    "hash[44] = 44.0;\n"
                    "hash[55] = 55.0;\n"
                    "hash[66] = 66.0;\n"
                    "Hash::iterator it1 = hash.begin();\n"
                    "Hash::iterator it2 = it1; ++it2;\n"
                    "Hash::iterator it3 = it2; ++it3;\n"
                    "Hash::iterator it4 = it3; ++it4;\n"
                    "Hash::iterator it5 = it4; ++it5;\n"
                    "Hash::iterator it6 = it5; ++it6;\n")
               % Check("hash", "<6 items>", "Hash")
               % Check("hash.11", "[11]", "11", "float")
               % Check("it1.key", "55", "int")
               % Check("it1.value", "55", "float")
               % Check("it6.key", "33", "int")
               % Check("it6.value", "33", "float");

    QTest::newRow("QHostAddress")
            << Data("#include <QHostAddress>\n",
                    "QHostAddress ha1(129u * 256u * 256u * 256u + 130u);\n"
                    "QHostAddress ha2(\"127.0.0.1\");\n")
               % Profile("QT += network\n")
               % Check("ha1", "129.0.0.130", "@QHostAddress")
               % Check("ha2", "\"127.0.0.1\"", "@QHostAddress");

    QTest::newRow("QImage")
            << Data("#include <QImage>\n"
                    "#include <QPainter>\n",
                    "QImage im(QSize(200, 200), QImage::Format_RGB32);\n"
                    "im.fill(QColor(200, 100, 130).rgba());\n"
                    "QPainter pain;\n"
                    "pain.begin(&im);\n")
               % GuiProfile()
               % Check("im", "(200x200)", "@QImage")
               % CheckType("pain", "@QPainter");

    QTest::newRow("QPixmap")
            << Data("#include <QImage>\n"
                    "#include <QPainter>\n"
                    "#include <QApplication>\n",
                    "QApplication app(argc, argv);\n"
                    "QImage im(QSize(200, 200), QImage::Format_RGB32);\n"
                    "im.fill(QColor(200, 100, 130).rgba());\n"
                    "QPainter pain;\n"
                    "pain.begin(&im);\n"
                    "pain.drawLine(2, 2, 130, 130);\n"
                    "pain.end();\n"
                    "QPixmap pm = QPixmap::fromImage(im);\n"
                    "dummyStatement(&pm);\n")
               % GuiProfile()
               % Check("im", "(200x200)", "@QImage")
               % CheckType("pain", "@QPainter")
               % Check("pm", "(200x200)", "@QPixmap");

    QTest::newRow("QLinkedListInt")
            << Data("#include <QLinkedList>\n",
                    "QLinkedList<int> list;\n"
                    "list.append(101);\n"
                    "list.append(102);\n")
               % Check("list", "<2 items>", "@QLinkedList<int>")
               % Check("list.0", "[0]", "101", "int")
               % Check("list.1", "[1]", "102", "int");

    QTest::newRow("QLinkedListUInt")
            << Data("#include <QLinkedList>\n",
                    "QLinkedList<uint> list;\n"
                    "list.append(103);\n"
                    "list.append(104);\n")
               % Check("list", "<2 items>", "@QLinkedList<unsigned int>")
               % Check("list.0", "[0]", "103", "unsigned int")
               % Check("list.1", "[1]", "104", "unsigned int");

    QTest::newRow("QLinkedListFooStar")
            << Data("#include <QLinkedList>\n" + fooData,
                    "QLinkedList<Foo *> list;\n"
                    "list.append(new Foo(1));\n"
                    "list.append(0);\n"
                    "list.append(new Foo(3));\n")
               % Check("list", "<3 items>", "@QLinkedList<Foo*>")
               % CheckType("list.0", "[0]", "Foo")
               % Check("list.0.a", "1", "int")
               % Check("list.1", "[1]", "0x0", "Foo *")
               % CheckType("list.2", "[2]", "Foo")
               % Check("list.2.a", "3", "int");

    QTest::newRow("QLinkedListULongLong")
            << Data("#include <QLinkedList>\n",
                    "QLinkedList<qulonglong> list;\n"
                    "list.append(42);\n"
                    "list.append(43);\n")
               % Check("list", "<2 items>", "@QLinkedList<unsigned long long>")
               % Check("list.0", "[0]", "42", "unsigned long long")
               % Check("list.1", "[1]", "43", "unsigned long long");

    QTest::newRow("QLinkedListFoo")
            << Data("#include <QLinkedList>\n" + fooData,
                    "QLinkedList<Foo> list;\n"
                    "list.append(Foo(1));\n"
                    "list.append(Foo(2));\n")
               % Check("list", "<2 items>", "@QLinkedList<Foo>")
               % CheckType("list.0", "[0]", "Foo")
               % Check("list.0.a", "1", "int")
               % CheckType("list.1", "[1]", "Foo")
               % Check("list.1.a", "2", "int");

    QTest::newRow("QLinkedListStdString")
            << Data("#include <QLinkedList>\n"
                    "#include <string>\n",
                    "QLinkedList<std::string> list;\n"
                    "list.push_back(\"aa\");\n"
                    "list.push_back(\"bb\");\n")
               % Check("list", "<2 items>", "@QLinkedList<std::string>")
               % Check("list.0", "[0]", "\"aa\"", "std::string")
               % Check("list.1", "[1]", "\"bb\"", "std::string");

    QTest::newRow("QListInt")
            << Data("#include <QList>\n",
                    "QList<int> big;\n"
                    "for (int i = 0; i < 10000; ++i)\n"
                    "    big.push_back(i);\n")
               % Check("big", "<10000 items>", "@QList<int>")
               % Check("big.0", "[0]", "0", "int")
               % Check("big.1999", "[1999]", "1999", "int");

    QTest::newRow("QListIntTakeFirst")
            << Data("#include <QList>\n",
                    "QList<int> l;\n"
                    "l.append(0);\n"
                    "l.append(1);\n"
                    "l.append(2);\n"
                    "l.takeFirst();\n")
               % Check("l", "<2 items>", "@QList<int>")
               % Check("l.0", "[0]", "1", "int");

    QTest::newRow("QListStringTakeFirst")
            << Data("#include <QList>\n"
                    "#include <QString>\n",
                    "QList<QString> l;\n"
                    "l.append(\"0\");\n"
                    "l.append(\"1\");\n"
                    "l.append(\"2\");\n"
                    "l.takeFirst();\n")
               % Check("l", "<2 items>", "@QList<@QString>")
               % Check("l.0", "[0]", "\"1\"", "@QString");

    QTest::newRow("QStringListTakeFirst")
            << Data("#include <QStringList>\n",
                    "QStringList l;\n"
                    "l.append(\"0\");\n"
                    "l.append(\"1\");\n"
                    "l.append(\"2\");\n"
                    "l.takeFirst();\n")
               % Check("l", "<2 items>", "@QStringList")
               % Check("l.0", "[0]", "\"1\"", "@QString");

    QTest::newRow("QListIntStar")
            << Data("#include <QList>\n",
                    "QList<int *> l0, l;\n"
                    "l.append(new int(1));\n"
                    "l.append(new int(2));\n"
                    "l.append(new int(3));\n")
               % Check("l0", "<0 items>", "@QList<int*>")
               % Check("l", "<3 items>", "@QList<int*>")
               % CheckType("l.0", "[0]", "int")
               % CheckType("l.2", "[2]", "int");

    QTest::newRow("QListUInt")
            << Data("#include <QList>\n",
                    "QList<uint> l0,l;\n"
                    "l.append(101);\n"
                    "l.append(102);\n"
                    "l.append(102);\n")
               % Check("l0", "<0 items>", "@QList<unsigned int>")
               % Check("l", "<3 items>", "@QList<unsigned int>")
               % Check("l.0", "[0]", "101", "unsigned int")
               % Check("l.2", "[2]", "102", "unsigned int");

    QTest::newRow("QListUShort")
            << Data("#include <QList>\n",
                    "QList<ushort> l0,l;\n"
                    "l.append(101);\n"
                    "l.append(102);\n"
                    "l.append(102);\n")
               % Check("l0", "<0 items>", "@QList<unsigned short>")
               % Check("l", "<3 items>", "@QList<unsigned short>")
               % Check("l.0", "[0]", "101", "unsigned short")
               % Check("l.2", "[2]", "102", "unsigned short");

    QTest::newRow("QListQChar")
            << Data("#include <QList>\n"
                    "#include <QChar>\n",
                    "QList<QChar> l0, l;\n"
                    "l.append(QChar('a'));\n"
                    "l.append(QChar('b'));\n"
                    "l.append(QChar('c'));\n")
               % Check("l0", "<0 items>", "@QList<@QChar>")
               % Check("l", "<3 items>", "@QList<@QChar>")
               % Check("l.0", "[0]", "'a' (97)", "@QChar")
               % Check("l.2", "[2]", "'c' (99)", "@QChar");

    QTest::newRow("QListQULongLong")
            << Data("#include <QList>\n",
                    "QList<qulonglong> l0, l;\n"
                    "l.append(101);\n"
                    "l.append(102);\n"
                    "l.append(102);\n")
               % Check("l0", "<0 items>", "@QList<unsigned long long>")
               % Check("l", "<3 items>", "@QList<unsigned long long>")
               % Check("l.0", "[0]", "101", "unsigned long long")
               % Check("l.2", "[2]", "102", "unsigned long long");

    QTest::newRow("QListStdString")
            << Data("#include <QList>\n"
                    "#include <string>\n",
                    "QList<std::string> l0, l;\n"
                    "l.push_back(\"aa\");\n"
                    "l.push_back(\"bb\");\n"
                    "l.push_back(\"cc\");\n"
                    "l.push_back(\"dd\");")
               % Check("l0", "<0 items>", "@QList<std::string>")
               % Check("l", "<4 items>", "@QList<std::string>")
               % CheckType("l.0", "[0]", "std::string")
               % Check("l.3", "[3]" ,"\"dd\"", "std::string");

   QTest::newRow("QListFoo")
            << Data("#include <QList>\n" + fooData,
                    "QList<Foo> l0, l;\n"
                    "for (int i = 0; i < 100; ++i)\n"
                    "    l.push_back(i + 15);\n")
               % Check("l0", "<0 items>", "@QList<Foo>")
               % Check("l", "<100 items>", "@QList<Foo>")
               % Check("l.0", "[0]", "", "Foo")
               % Check("l.99", "[99]", "", "Foo");

   QTest::newRow("QListReverse")
           << Data("#include <QList>\n",
                   "QList<int> l = QList<int>() << 1 << 2 << 3;\n"
                   "typedef std::reverse_iterator<QList<int>::iterator> Reverse;\n"
                   "Reverse rit(l.end());\n"
                   "Reverse rend(l.begin());\n"
                   "QList<int> r;\n"
                   "while (rit != rend)\n"
                   "    r.append(*rit++);\n")
              % Check("l", "<3 items>", "@QList<int>")
              % Check("l.0", "[0]", "1", "int")
              % Check("l.1", "[1]", "2", "int")
              % Check("l.2", "[2]", "3", "int")
              % Check("r", "<3 items>", "@QList<int>")
              % Check("r.0", "[0]", "3", "int")
              % Check("r.1", "[1]", "2", "int")
              % Check("r.2", "[2]", "1", "int")
              % Check("rend", "", "Reverse")
              % Check("rit", "", "Reverse");

   QTest::newRow("QLocale")
           << Data("#include <QLocale>\n",
                   "QLocale loc = QLocale::system();\n"
                   "QLocale::MeasurementSystem m = loc.measurementSystem();\n"
                   "dummyStatement(&m);\n")
              % Check("loc", "", "@QLocale")
              % Check("m", "", "@QLocale::MeasurementSystem");


   QTest::newRow("QMapUIntStringList")
           << Data("#include <QMap>\n"
                   "#include <QStringList>\n",
                   "QMap<uint, QStringList> map;\n"
                   "map[11] = QStringList() << \"11\";\n"
                   "map[22] = QStringList() << \"22\";\n")
              % Check("map", "<2 items>", "@QMap<unsigned int, @QStringList>")
              % Check("map.0", "[0]", "", "@QMapNode<unsigned int, @QStringList>")
              % Check("map.0.key", "11", "unsigned int")
              % Check("map.0.value", "<1 items>", "@QStringList")
              % Check("map.0.value.0", "[0]", "\"11\"", "@QString")
              % Check("map.1", "[1]", "", "@QMapNode<unsigned int, @QStringList>")
              % Check("map.1.key", "22", "unsigned int")
              % Check("map.1.value", "<1 items>", "@QStringList")
              % Check("map.1.value.0", "[0]", "\"22\"", "@QString");

   QTest::newRow("QMapUIntStringListTypedef")
           << Data("#include <QMap>\n"
                   "#include <QStringList>\n",
                   "typedef QMap<uint, QStringList> T;\n"
                   "T map;\n"
                   "map[11] = QStringList() << \"11\";\n"
                   "map[22] = QStringList() << \"22\";\n")
              % Check("map", "<2 items>", "T")
              % Check("map.0", "[0]", "", "@QMapNode<unsigned int, @QStringList>");

   QTest::newRow("QMapUIntFloat")
           << Data("#include <QMap>\n",
                   "QMap<uint, float> map;\n"
                   "map[11] = 11.0;\n"
                   "map[22] = 22.0;\n")
              % Check("map", "<2 items>", "@QMap<unsigned int, float>")
              % Check("map.11", "[11]", "11", "float")
              % Check("map.22", "[22]", "22", "float");

   QTest::newRow("QMapStringFloat")
           << Data("#include <QMap>\n",
                   "QMap<QString, float> map;\n"
                   "map[\"22.0\"] = 22.0;\n")
              % Check("map", "<1 items>", "@QMap<@QString, float>")
              % Check("map.0", "", "@QMapNode<@QString, float>")
              % Check("map.0.key", "\"22.0\"", "@QString")
              % Check("map.0.value", "22", "float");

   QTest::newRow("QMapIntString")
           << Data("#include <QMap>\n",
                   "QMap<int, QString> map;\n"
                   "map[22] = \"22.0\";\n")
              % Check("map", "<1 items>", "@QMap<int, @QString>")
              % Check("map.0", "", "@QMapNode<int, @QString>")
              % Check("map.0.key", "22", "int")
              % Check("map.0.value", "\"22.0\"", "@QString");

   QTest::newRow("QMapStringFoo")
           << Data("#include <QMap>\n" + fooData,
                   "QMap<QString, Foo> map;\n"
                   "map[\"22.0\"] = Foo(22);\n"
                   "map[\"33.0\"] = Foo(33);\n")
              % Check("map", "<2 items>", "@QMap<@QString, Foo>")
              % Check("map.0", "", "@QMapNode<@QString, Foo>")
              % Check("map.0.key", "\"22.0\"", "@QString")
              % Check("map.0.value", "", "Foo")
              % Check("map.0.value.a", "22", "int")
              % Check("map.1", "", "@QMapNode<@QString, Foo>")
              % Check("map.1.key", "\"33.0\"", "@QString")
              % Check("map.1.value", "", "Foo")
              % Check("map.1.value.a", "33", "int");

   QTest::newRow("QMapStringPointer")
           << Data("#include <QMap>\n",
                   "QObject ob;\n"
                   "QMap<QString, QPointer<QObject> > map;\n"
                   "map.insert(\"Hallo\", QPointer<QObject>(&ob));\n"
                   "map.insert(\"Welt\", QPointer<QObject>(&ob));\n"
                   "map.insert(\".\", QPointer<QObject>(&ob));\n")
              % Check("map", "<3 items>", "@QMap<@QString, @QPointer<@QObject>>")
              % Check("map.0", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.0.key", ".", "@QString")
              % Check("map.0.value", "", "@QPointer<@QObject>")
              % Check("map.0.value.o", "", "@QObject")
              % Check("map.1", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.1.key", "\"Hallo\"", "@QString")
              % Check("map.2", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.2.key", "\"Welt\"", "@QString");

   QTest::newRow("QMapStringList")
           << Data("#include <QMap>\n",
                   "QList<nsA::nsB::SomeType *> x;\n"
                   "x.append(new nsA::nsB::SomeType(1));\n"
                   "x.append(new nsA::nsB::SomeType(2));\n"
                   "x.append(new nsA::nsB::SomeType(3));\n"
                   "QMap<QString, QList<nsA::nsB::SomeType *> > map;\n"
                   "map[\"foo\"] = x;\n"
                   "map[\"bar\"] = x;\n"
                   "map[\"1\"] = x;\n"
                   "map[\"2\"] = x;\n")
              % Check("map", "<4 items>", "@QMap<@QString, @QList<nsA::nsB::SomeType*>>")
              % Check("map.0", "", "@QMapNode<@QString, @QList<nsA::nsB::SomeType*>>")
              % Check("map.0.key", "\"1\"", "@QString")
              % Check("map.0.value", "<3 items>", "@QList<nsA::nsB::SomeType*>")
              % Check("map.0.value.0", "", "nsA::nsB::SomeType")
              % Check("map.0.value.0.a", "1", "int")
              % Check("map.0.value.1", "", "nsA::nsB::SomeType")
              % Check("map.0.value.1.a", "2", "int")
              % Check("map.0.value.2", "", "nsA::nsB::SomeType")
              % Check("map.0.value.2.a", "3", "int")
              % Check("map.3", "", "@QMapNode<@QString, QList<nsA::nsB::SomeType*>>")
              % Check("map.3.key", "\"foo\"", "@QString")
              % Check("map.3.value", "<3 items>", "@QList<nsA::nsB::SomeType*>")
              % Check("map.3.value.2", "", "nsA::nsB::SomeType")
              % Check("map.3.value.2.a", "3", "int")
              % Check("x", "<3 items>", "@QList<nsA::nsB::SomeType*>");

   QTest::newRow("QMultiMapUintFloat")
           << Data("#include <QMap>\n",
                   "QMultiMap<uint, float> map;\n"
                   "map.insert(11, 11.0);\n"
                   "map.insert(22, 22.0);\n"
                   "map.insert(22, 33.0);\n"
                   "map.insert(22, 34.0);\n"
                   "map.insert(22, 35.0);\n"
                   "map.insert(22, 36.0);\n")
              % Check("map", "<6 items>", "@QMultiMap<unsigned int, float>")
              % Check("map.0", "11", "float")
              % Check("map.5", "22", "float");

   QTest::newRow("QMultiMapStringFloat")
           << Data("#include <QMap>\n",
                   "QMultiMap<QString, float> map;\n"
                   "map.insert(\"22.0\", 22.0);\n")
              % Check("map", "<1 items>", "@QMultiMap<@QString, float>")
              % Check("map.0", "", "@QMapNode<@QString, float>")
              % Check("map.0.key", "\"22.0\"", "@QString")
              % Check("map.0.value", "22", "float");

   QTest::newRow("QMultiMapIntString")
           << Data("#include <QMap>\n",
                   "QMultiMap<int, QString> map;\n"
                   "map.insert(22, \"22.0\");\n")
              % Check("map", "<1 items>", "@QMultiMap<int, @QString>")
              % Check("map.0", "", "@QMapNode<int, @QString>")
              % Check("map.0.key", "22", "int")
              % Check("map.0.value", "\"22.0\"", "@QString");

   QTest::newRow("QMultiMapStringFoo")
           << Data("#include <QMultiMap>\n" + fooData,
                   "QMultiMap<QString, Foo> map;\n"
                   "map.insert(\"22.0\", Foo(22));\n"
                   "map.insert(\"33.0\", Foo(33));\n"
                   "map.insert(\"22.0\", Foo(22));\n")
              % Check("map", "<3 items>", "@QMultiMap<@QString, Foo>")
              % Check("map.0", "[0]", "", "@QMapNode<@QString, Foo>")
              % Check("map.0.key", "\"22.0\"", "@QString")
              % Check("map.0.value", "", "Foo")
              % Check("map.0.value.a", "22", "int")
              % Check("map.2", "[2]", "", "@QMapNode<@QString, Foo>");

   QTest::newRow("QMultiMapStringPointer")
           << Data("#include <QMap>\n"
                   "#include <QObject>\n"
                   "#include <QPointer>\n"
                   "#include <QString>\n",
                   "QObject ob;\n"
                   "QMultiMap<QString, QPointer<QObject> > map;\n"
                   "map.insert(\"Hallo\", QPointer<QObject>(&ob));\n"
                   "map.insert(\"Welt\", QPointer<QObject>(&ob));\n"
                   "map.insert(\".\", QPointer<QObject>(&ob));\n"
                   "map.insert(\".\", QPointer<QObject>(&ob));\n")
              % Check("map", "<4 items>", "@QMultiMap<@QString, @QPointer<@QObject>>")
              % Check("map.0", "[0]", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.0.key", "\".\"", "@QString")
              % Check("map.0.value", "", "@QPointer<@QObject>")
              % Check("map.1", "[1]", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.1.key", "\".\"", "@QString")
              % Check("map.2", "[2]", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.2.key", "\"Hallo\"", "@QString")
              % Check("map.3", "[3]", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.3.key", "\"Welt\"", "@QString");


   QTest::newRow("QObject1")
           << Data("#include <QObject>\n",
                   "QObject parent;\n"
                   "parent.setObjectName(\"A Parent\");\n"
                   "QObject child(&parent);\n"
                   "child.setObjectName(\"A Child\");\n"
                   "QObject::connect(&child, SIGNAL(destroyed()), &parent, SLOT(deleteLater()));\n"
                   "QObject::disconnect(&child, SIGNAL(destroyed()), &parent, SLOT(deleteLater()));\n"
                   "child.setObjectName(\"A renamed Child\");\n")
              % Check("child", "\"A renamed Child\"", "@QObject")
              % Check("parent", "\"A Parent\"", "@QObject");

    QByteArray objectData =
"    namespace Names {\n"
"        namespace Bar {\n"
"\n"
"        struct Ui { Ui() { w = 0; } QWidget *w; };\n"
"\n"
"        class TestObject : public QObject\n"
"        {\n"
"            Q_OBJECT\n"
"        public:\n"
"            TestObject(QObject *parent = 0)\n"
"                : QObject(parent)\n"
"            {\n"
"                m_ui = new Ui;\n"
"                #if USE_GUILIB\n"
"                m_ui->w = new QWidget;\n"
"                #else\n"
"                m_ui->w = 0;\n"
"                #endif\n"
"            }\n"
"\n"
"            Q_PROPERTY(QString myProp1 READ myProp1 WRITE setMyProp1)\n"
"            QString myProp1() const { return m_myProp1; }\n"
"            Q_SLOT void setMyProp1(const QString&mt) { m_myProp1 = mt; }\n"
"\n"
"            Q_PROPERTY(QString myProp2 READ myProp2 WRITE setMyProp2)\n"
"            QString myProp2() const { return m_myProp2; }\n"
"            Q_SLOT void setMyProp2(const QString&mt) { m_myProp2 = mt; }\n"
"\n"
"        public:\n"
"            Ui *m_ui;\n"
"            QString m_myProp1;\n"
"            QString m_myProp2;\n"
"        };\n"
"\n"
"        } // namespace Bar\n"
"    } // namespace Names\n"
"\n";

    QTest::newRow("QObject2")
            << Data(objectData,
                    "Names::Bar::TestObject test;\n"
                    "test.setMyProp1(\"HELLO\");\n"
                    "test.setMyProp2(\"WORLD\");\n"
                    "QString s = test.myProp1();\n"
                    "s += test.myProp2();\n")
               % Check("s", "\"HELLOWORLD\"", "@QString")
               % Check("test", "", "Names::Bar::TestObject");

//"    #if 1\n"
//"        #if USE_GUILIB\n"
//"        QWidget ob;\n"
//"        ob.setObjectName("An Object");\n"
//"        ob.setProperty("USER DEFINED 1", 44);\n"
//"        ob.setProperty("USER DEFINED 2", QStringList() << "FOO" << "BAR");\n"
//"        QObject ob1;\n"
//"        ob1.setObjectName("Another Object");\n"
//"\n"
//"        QObject::connect(&ob, SIGNAL(destroyed()), &ob1, SLOT(deleteLater()));\n"
//"        QObject::connect(&ob, SIGNAL(destroyed()), &ob1, SLOT(deleteLater()));\n"
//"        //QObject::connect(&app, SIGNAL(lastWindowClosed()), &ob, SLOT(deleteLater()));\n"
//"        #endif\n"
//"    #endif\n"
//"\n"
//"    #if 0\n"
//"        QList<QObject *> obs;\n"
//"        obs.append(&ob);\n"
//"        obs.append(&ob1);\n"
//"        obs.append(0);\n"
//"        obs.append(&app);\n"
//"        ob1.setObjectName("A Subobject");\n"
//"    #endif\n"
//"    }\n"
//"\n"

    QByteArray senderData =
            "    class Sender : public QObject\n"
            "    {\n"
            "        Q_OBJECT\n"
            "    public:\n"
            "        Sender() { setObjectName(\"Sender\"); }\n"
            "        void doEmit() { emit aSignal(); }\n"
            "    signals:\n"
            "        void aSignal();\n"
            "    };\n"
            "\n"
            "    class Receiver : public QObject\n"
            "    {\n"
            "        Q_OBJECT\n"
            "    public:\n"
            "        Receiver() { setObjectName(\"Receiver\"); }\n"
            "    public slots:\n"
            "        void aSlot() {\n"
            "            QObject *s = sender();\n"
            "            if (s) {\n"
            "                qDebug() << \"SENDER: \" << s;\n"
            "            } else {\n"
            "                qDebug() << \"NO SENDER\";\n"
            "            }\n"
            "        }\n"
            "    };\n";

    QByteArray derivedData =
            "    class DerivedObjectPrivate : public QObjectPrivate\n"
            "    {\n"
            "    public:\n"
            "        DerivedObjectPrivate() {\n"
            "            m_extraX = 43;\n"
            "            m_extraY.append(\"xxx\");\n"
            "            m_extraZ = 1;\n"
            "        }\n"
            "        int m_extraX;\n"
            "        QStringList m_extraY;\n"
            "        uint m_extraZ : 1;\n"
            "        bool m_extraA : 1;\n"
            "        bool m_extraB;\n"
            "    };\n"
            "\n"
            "    class DerivedObject : public QObject\n"
            "    {\n"
            "        Q_OBJECT\n"
            "\n"
            "    public:\n"
            "        DerivedObject() : QObject(*new DerivedObjectPrivate, 0) {}\n"
            "\n"
            "        Q_PROPERTY(int x READ x WRITE setX)\n"
            "        Q_PROPERTY(QStringList y READ y WRITE setY)\n"
            "        Q_PROPERTY(uint z READ z WRITE setZ)\n"
            "\n"
            "        int x() const;\n"
            "        void setX(int x);\n"
            "        QStringList y() const;\n"
            "        void setY(QStringList y);\n"
            "        uint z() const;\n"
            "        void setZ(uint z);\n"
            "\n"
            "    private:\n"
            "        Q_DECLARE_PRIVATE(DerivedObject)\n"
            "    };\n"
            "\n"
            "    int DerivedObject::x() const\n"
            "    {\n"
            "        Q_D(const DerivedObject);\n"
            "        return d->m_extraX;\n"
            "    }\n"
            "\n"
            "    void DerivedObject::setX(int x)\n"
            "    {\n"
            "        Q_D(DerivedObject);\n"
            "        d->m_extraX = x;\n"
            "        d->m_extraA = !d->m_extraA;\n"
            "        d->m_extraB = !d->m_extraB;\n"
            "    }\n"
            "\n"
            "    QStringList DerivedObject::y() const\n"
            "    {\n"
            "        Q_D(const DerivedObject);\n"
            "        return d->m_extraY;\n"
            "    }\n"
            "\n"
            "    void DerivedObject::setY(QStringList y)\n"
            "    {\n"
            "        Q_D(DerivedObject);\n"
            "        d->m_extraY = y;\n"
            "    }\n"
            "\n"
            "    uint DerivedObject::z() const\n"
            "    {\n"
            "        Q_D(const DerivedObject);\n"
            "        return d->m_extraZ;\n"
            "    }\n"
            "\n"
            "    void DerivedObject::setZ(uint z)\n"
            "    {\n"
            "        Q_D(DerivedObject);\n"
            "        d->m_extraZ = z;\n"
            "    }\n"
            "\n"
            "    #endif\n"
            "\n";

    QTest::newRow("QObjectData")
            << Data(derivedData,
                    "DerivedObject ob;\n"
                    "ob.setX(26);\n")
               % Check("ob.properties.x", "26", "@QVariant (int)");


    QTest::newRow("QRegExp")
            << Data("QRegExp re(QString(\"a(.*)b(.*)c\"));\n"
                    "QString str1 = \"a1121b344c\";\n"
                    "QString str2 = \"Xa1121b344c\";\n"
                    "int pos2 = re.indexIn(str2);\n"
                    "int pos1 = re.indexIn(str1);\n")
               % Check("re", "\"a(.*)b(.*)c\"", "@QRegExp")
               % Check("str1", "\"a1121b344c\"", "@QString")
               % Check("str2", "\"Xa1121b344c\"", "@QString")
               % Check("pos1", "0", "int")
               % Check("pos2", "1", "int");

    QTest::newRow("QPoint")
            << Data("QPoint s0, s;\n"
                    "s = QPoint(100, 200);\n")
               % Check("s0", "(0, 0)", "@QPoint")
               % Check("s", "(100, 200)", "@QPoint");

    QTest::newRow("QPointF")
            << Data("QPointF s0, s;\n"
                    "s = QPointF(100, 200);\n")
               % Check("s0", "(0, 0)", "@QPointF")
               % Check("s", "(100, 200)", "@QPointF");

    QTest::newRow("QRect")
            << Data("QRect rect0, rect;\n"
                    "rect = QRect(100, 100, 200, 200);\n")
               % Check("rect", "0x0+0+0", "QRect")
               % Check("rect", "200x200+100+100", "QRect");

    QTest::newRow("QRectF")
            << Data("QRectF rect0, rect;\n"
                    "rect = QRectF(100, 100, 200, 200);\n")
               % Check("rect", "0x0+0+0", "QRectF")
               % Check("rect", "200x200+100+100", "QRectF");

    QTest::newRow("QSize")
            << Data("QSize s0, s;\n"
                    "s = QSize(100, 200);\n")
               % Check("s0", "(-1, -1)", "@QSize")
               % Check("s", "(100, 200)", "@QSize");

    QTest::newRow("QSizeF")
            << Data("QSizeF s0, s;\n"
                    "s = QSizeF(100, 200);\n")
               % Check("s0", "(-1, -1)", "@QSizeF")
               % Check("s", "(100, 200)", "@QSizeF");

    QTest::newRow("QRegion")
            << Data("QRegion region, region0, region1, region2, region4;\n"
                    "region += QRect(100, 100, 200, 200);\n"
                    "region0 = region;\n"
                    "region += QRect(300, 300, 400, 500);\n"
                    "region1 = region;\n"
                    "region += QRect(500, 500, 600, 600);\n"
                    "region2 = region;\n")
               % Check("region", "<empty>", "@QRegion")
               % Check("region", "<1 items>", "@QRegion")
               % Check("region.extents", "", "@QRect")
               % Check("region.innerArea", "40000", "int")
               % Check("region.innerRect", "", "@QRect")
               % Check("region.numRects", "1", "int")
               % Check("region.rects", "<0 items>", "@QVector<@QRect>")
               % Check("region", "<2 items>", "@QRegion")
               % Check("region.extents", "", "@QRect")
               % Check("region.innerArea", "200000", "int")
               % Check("region.innerRect", "", "@QRect")
               % Check("region.numRects", "2", "int")
               % Check("region.rects", "<2 items>", "@QVector<@QRect>")
               % Check("region", "<4 items>", "@QRegion")
               % Check("region.extents", "", "@QRect")
               % Check("region.innerArea", "360000", "int")
               % Check("region.innerRect", "", "@QRect")
               % Check("region.numRects", "4", "int")
               % Check("region.rects", "<8 items>", "@QVector<@QRect>")
               % Check("region", "<4 items>", "@QRegion");

    QTest::newRow("QSettings")
            << Data("QCoreApplication app(argc, argv);\n"
                    "QSettings settings(\"/tmp/test.ini\", QSettings::IniFormat);\n"
                    "QVariant value = settings.value(\"item1\", \"\").toString();\n")
               % Check("settings", "", "@QSettings")
               % Check("settings.@1", "", "@QObject")
               % Check("value", "", "@QVariant (QString)");

    QTest::newRow("QSet1")
            << Data("QSet<int> s;\n"
                    "s.insert(11);\n"
                    "s.insert(22);\n")
               % Check("s", "<2 items>", "@QSet<int>")
               % Check("s.22", "22", "int")
               % Check("s.11", "11", "int");

    QTest::newRow("QSet2")
            << Data("QSet<QString> s;\n"
                    "s.insert(\"11.0\");\n"
                    "s.insert(\"22.0\");\n")
               % Check("s", "<2 items>", "@QSet<@QString>")
               % Check("s.0", "\"11.0\"", "@QString")
               % Check("s.1", "\"22.0\"", "@QString");

    QTest::newRow("QSet3")
            << Data("QObject ob;\n"
                    "QSet<QPointer<QObject> > s;\n"
                    "QPointer<QObject> ptr(&ob);\n"
                    "s.insert(ptr);\n"
                    "s.insert(ptr);\n"
                    "s.insert(ptr);\n")
               % Check("s", "<1 items>", "@QSet<@QPointer<@QObject>>")
               % Check("s.0", "", "@QPointer<@QObject>");


    QByteArray sharedData =
            "    class EmployeeData : public QSharedData\n"
            "    {\n"
            "    public:\n"
            "        EmployeeData() : id(-1) { name.clear(); }\n"
            "        EmployeeData(const EmployeeData &other)\n"
            "            : QSharedData(other), id(other.id), name(other.name) { }\n"
            "        ~EmployeeData() { }\n"
            "\n"
            "        int id;\n"
            "        QString name;\n"
            "    };\n"
            "\n"
            "    class Employee\n"
            "    {\n"
            "    public:\n"
            "        Employee() { d = new EmployeeData; }\n"
            "        Employee(int id, QString name) {\n"
            "            d = new EmployeeData;\n"
            "            setId(id);\n"
            "            setName(name);\n"
            "        }\n"
            "        Employee(const Employee &other)\n"
            "              : d (other.d)\n"
            "        {\n"
            "        }\n"
            "        void setId(int id) { d->id = id; }\n"
            "        void setName(QString name) { d->name = name; }\n"
            "\n"
            "        int id() const { return d->id; }\n"
            "        QString name() const { return d->name; }\n"
            "\n"
            "       private:\n"
            "         QSharedDataPointer<EmployeeData> d;\n"
            "    };\n";


    QTest::newRow("QSharedPointer1")
            << Data("QSharedPointer<int> ptr2 = ptr;\n"
                    "QSharedPointer<int> ptr3 = ptr;\n");

    QTest::newRow("QSharedPointer2")
            << Data("QSharedPointer<QString> ptr(new QString(\"hallo\"));\n"
                    "QSharedPointer<QString> ptr2 = ptr;\n"
                    "QSharedPointer<QString> ptr3 = ptr;\n");

    QTest::newRow("QSharedPointer3")
            << Data("QSharedPointer<int> iptr(new int(43));\n"
                    "QWeakPointer<int> ptr(iptr);\n"
                    "QWeakPointer<int> ptr2 = ptr;\n"
                    "QWeakPointer<int> ptr3 = ptr;\n");

    QTest::newRow("QSharedPointer4")
            << Data("QSharedPointer<QString> sptr(new QString(\"hallo\"));\n"
                    "QWeakPointer<QString> ptr(sptr);\n"
                    "QWeakPointer<QString> ptr2 = ptr;\n"
                    "QWeakPointer<QString> ptr3 = ptr;\n");

    QTest::newRow("QSharedPointer5")
            << Data("#include <QSharedPointer>\n" + fooData,
                    "QSharedPointer<Foo> fptr(new Foo(1));\n"
                    "QWeakPointer<Foo> ptr(fptr);\n"
                    "QWeakPointer<Foo> ptr2 = ptr;\n"
                    "QWeakPointer<Foo> ptr3 = ptr;\n");

    QTest::newRow("QXmlAttributes")
            << Data("QXmlAttributes atts;\n"
                    "atts.append(\"name1\", \"uri1\", \"localPart1\", \"value1\");\n"
                    "atts.append(\"name2\", \"uri2\", \"localPart2\", \"value2\");\n"
                    "atts.append(\"name3\", \"uri3\", \"localPart3\", \"value3\");\n")
               % Check("atts", "", "QXmlAttributes")
               % CheckType("atts.[vptr]", "")
               % Check("atts.attList", "<3 items>", "@QXmlAttributes::AttributeList")
               % Check("atts.attList.0", "", "QXmlAttributes::Attribute")
               % Check("atts.attList.0.localname", "\"localPart1\"", "@QString")
               % Check("atts.attList.0.qname", "\"name1\"", "@QString")
               % Check("atts.attList.0.uri", "\"uri1\"", "@QString")
               % Check("atts.attList.0.value", "\"value1\"", "@QString")
               % Check("atts.attList.1", "", "@QXmlAttributes::Attribute")
               % Check("atts.attList.1.localname", "\"localPart2\"", "@QString")
               % Check("atts.attList.1.qname", "\"name2\"", "@QString")
               % Check("atts.attList.1.uri", "\"uri2\"", "@QString")
               % Check("atts.attList.1.value", "\"value2\"", "@QString")
               % Check("atts.attList.2", "", "@QXmlAttributes::Attribute")
               % Check("atts.attList.2.localname", "\"localPart3\"", "@QString")
               % Check("atts.attList.2.qname", "\"name3\"", "@QString")
               % Check("atts.attList.2.uri", "\"uri3\"", "@QString")
               % Check("atts.attList.2.value", "\"value3\"", "@QString")
               % Check("atts.d", "", "@QXmlAttributesPrivate");

    QTest::newRow("StdArray")
            << Data("std::array<int, 4> a = { { 1, 2, 3, 4} };\n"
                    "std::array<QString, 4> b = { { \"1\", \"2\", \"3\", \"4\"} };\n")
               % Check("a", "<4 items>", "std::array<int, 4u>")
               % Check("a", "<4 items>", "std::array<QString, 4u>");

    QTest::newRow("StdComplex")
            << Data("std::complex<double> c(1, 2);\n")
               % Check("c", "(1.000000, 2.000000)", "std::complex<double>");

    QTest::newRow("StdDequeInt")
            << Data("std::deque<int> deque;\n"
                    "deque.push_back(1);\n"
                    "deque.push_back(2);\n")
               % Check("deque", "<2 items>", "std::deque<int>")
               % Check("deque.0", "1", "int")
               % Check("deque.1", "2", "int");

    QTest::newRow("StdDequeIntStar")
            << Data("std::deque<int *> deque;\n"
                    "deque.push_back(new int(1));\n"
                    "deque.push_back(0);\n"
                    "deque.push_back(new int(2));\n"
                    "deque.pop_back();\n"
                    "deque.pop_front();\n")
               % Check("deque", "<3 items>", "std::deque<int*>")
               % Check("deque.0", "", "int")
               % Check("deque.1", "0x0", "int *");

    QTest::newRow("StdDequeFoo")
            << Data("#include <deque>\n" + fooData,
                    "std::deque<Foo> deque;\n"
                    "deque.push_back(1);\n"
                    "deque.push_front(2);\n")
               % Check("deque", "<2 items>", "std::deque<Foo>")
               % Check("deque.0", "", "Foo")
               % Check("deque.0.a", "2", "int")
               % Check("deque.1", "", "Foo")
               % Check("deque.1.a", "1", "int");

    QTest::newRow("StdDequeFooStar")
            << Data("#include <deque>\n" + fooData,
                    "std::deque<Foo *> deque;\n"
                    "deque.push_back(new Foo(1));\n"
                    "deque.push_back(new Foo(2));\n")
               % Check("deque", "<2 items>", "std::deque<Foo*>")
               % Check("deque.0", "", "Foo")
               % Check("deque.0.a", "1", "int")
               % Check("deque.1", "", "Foo")
               % Check("deque.1.a", "2", "int");

    QTest::newRow("StdHashSet")
            << Data("#include <hash_set>\n"
                    "using namespace __gnu_cxx;\n"
                    "hash_set<int> h;\n"
                    "h.insert(1);\n"
                    "h.insert(194);\n"
                    "h.insert(2);\n"
                    "h.insert(3);\n")
               % Check("h", "<4 items>", "__gnu__cxx::hash_set<int>")
               % Check("h.0", "194", "int")
               % Check("h.1", "1", "int")
               % Check("h.2", "2", "int")
               % Check("h.3", "3", "int");

    QTest::newRow("StdListInt")
            << Data("#include <list>\n"
                    "std::list<int> list;\n"
                    "list.push_back(1);\n"
                    "list.push_back(2);\n")
               % Check("list", "<2 items>", "std::list<int>")
               % Check("list.0", "1", "int")
               % Check("list.1", "2", "int");

    QTest::newRow("StdListIntStar")
            << Data("#include <list>\n"
                    "std::list<int *> list;\n"
                    "list.push_back(new int(1));\n"
                    "list.push_back(0);\n"
                    "list.push_back(new int(2));\n")
               % Check("list", "<3 items>", "std::list<int*>")
               % Check("list.0", "", "int")
               % Check("list.1", "0x0", "int *")
               % Check("list.2", "", "int");

    QTest::newRow("StdListIntBig")
            << Data("#include <list>\n"
                    "std::list<int> list;\n"
                    "for (int i = 0; i < 10000; ++i)\n"
                    "    list.push_back(i);\n")
               % Check("list", "<more than 1000 items>", "std::list<int>")
               % Check("list.0", "0", "int")
               % Check("list.999", "999", "int");

    QTest::newRow("StdListFoo")
            << Data("#include <list>\n"
                    "std::list<Foo> list;\n"
                    "list.push_back(15);\n"
                    "list.push_back(16);\n")
               % Check("list", "<2 items>", "std::list<Foo>")
               % Check("list.0", "", "Foo")
               % Check("list.0.a", "15", "int")
               % Check("list.1", "", "Foo")
               % Check("list.1.a", "16", "int");

    QTest::newRow("StdListFooStar")
            << Data("#include <list>\n"
                    "std::list<Foo *> list;\n"
                    "list.push_back(new Foo(1));\n"
                    "list.push_back(0);\n"
                    "list.push_back(new Foo(2));\n")
               % Check("list", "<3 items>", "std::list<Foo*>")
               % Check("list.0", "", "Foo")
               % Check("list.0.a", "1", "int")
               % Check("list.1", "0x0", "Foo *")
               % Check("list.2", "", "Foo")
               % Check("list.2.a", "2", "int");

    QTest::newRow("StdListBool")
            << Data("#include <list>\n"
                    "std::list<bool> list;\n"
                    "list.push_back(true);\n"
                    "list.push_back(false);\n")
               % Check("list", "<2 items>", "std::list<bool>")
               % Check("list.0", "true", "bool")
               % Check("list.1", "false", "bool");

    QTest::newRow("StdMapStringFoo")
            << Data("#include <map>\n",
                    "std::map<QString, Foo> map;\n"
                    "map[\"22.0\"] = Foo(22);\n"
                    "map[\"33.0\"] = Foo(33);\n"
                    "map[\"44.0\"] = Foo(44);\n")
               % Check("map", "<3 items>", "std::map<@QString, Foo>")
               % Check("map.0", "", "std::pair<@QString const, Foo>")
               % Check("map.0.first", "\"22.0\"", "@QString")
               % Check("map.0.second", "", "Foo")
               % Check("map.0.second.a", "22", "int")
               % Check("map.1", "", "std::pair<@QString const, Foo>")
               % Check("map.2.first", "\"44.0\"", "@QString")
               % Check("map.2.second", "", "Foo")
               % Check("map.2.second.a", "44", "int");

    QTest::newRow("StdMapCharStarFoo")
            << Data("#include <map>\n",
                    "std::map<const char *, Foo> map;\n"
                    "map[\"22.0\"] = Foo(22);\n"
                    "map[\"33.0\"] = Foo(33);\n")
               % Check("map", "<2 items>", "std::map<char const*, Foo>")
               % Check("map.0", "", "std::pair<char const* const, Foo>")
               % CheckType("map.0.first", "char *")
               % Check("map.0.first.*first", "50 '2'", "char")
               % Check("map.0.second", "", "Foo")
               % Check("map.0.second.a", "22", "int")
               % Check("map.1", "", "std::pair<char const* const, Foo>")
               % CheckType("map.1.first", "char *")
               % Check("map.1.first.*first", "51 '3'", "char")
               % Check("map.1.second", "", "Foo")
               % Check("map.1.second.a", "33", "int");

    QTest::newRow("StdMapUIntUInt")
            << Data("#include <map>\n",
                    "std::map<uint, uint> map;\n"
                    "map[11] = 1;\n"
                    "map[22] = 2;\n")
               % Check("map", "<2 items>", "std::map<unsigned int, unsigned int>")
               % Check("map.11", "1", "unsigned int")
               % Check("map.22", "2", "unsigned int");

    QTest::newRow("StdMapUIntStringList")
            << Data("#include <map>\n",
                    "std::map<uint, QStringList> map;\n"
                    "map[11] = QStringList() << \"11\";\n"
                    "map[22] = QStringList() << \"22\";\n")
               % Check("map", "<2 items>", "std::map<unsigned int, @QStringList>")
               % Check("map.0", "", "std::pair<unsigned int const, @QStringList>")
               % Check("map.0.first", "11", "unsigned int")
               % Check("map.0.second", "<1 items>", "@QStringList")
               % Check("map.0.second.0", "\"11\"", "@QString")
               % Check("map.1", "", "std::pair<unsigned int const, @QStringList>")
               % Check("map.1.first", "22", "unsigned int")
               % Check("map.1.second", "<1 items>", "@QStringList")
               % Check("map.1.second.0", "\"22\"", "@QString");

    QTest::newRow("StdMapUIntStringListTypedef")
            << Data("#include <map>\n",
                    "typedef std::map<uint, QStringList> T;\n"
                    "T map;\n"
                    "map[11] = QStringList() << \"11\";\n"
                    "map[22] = QStringList() << \"22\";\n");

    QTest::newRow("StdMapUIntFloat")
            << Data("#include <map>\n",
                    "std::map<uint, float> map;\n"
                    "map[11] = 11.0;\n"
                    "map[22] = 22.0;\n")
               % Check("map", "<2 items>", "std::map<unsigned int, float>")
               % Check("map.11", "[11]", "11", "float")
               % Check("map.22", "[22]", "22", "float");

    QTest::newRow("StdMapUIntFloatIterator")
            << Data("#include <map>\n",
                    "typedef std::map<int, float> Map;\n"
                    "Map map;\n"
                    "map[11] = 11.0;\n"
                    "map[22] = 22.0;\n"
                    "map[33] = 33.0;\n"
                    "map[44] = 44.0;\n"
                    "map[55] = 55.0;\n"
                    "map[66] = 66.0;\n"
                    "Map::iterator it1 = map.begin();\n"
                    "Map::iterator it2 = it1; ++it2;\n"
                    "Map::iterator it3 = it2; ++it3;\n"
                    "Map::iterator it4 = it3; ++it4;\n"
                    "Map::iterator it5 = it4; ++it5;\n"
                    "Map::iterator it6 = it5; ++it6;\n")
               % Check("map", "<6 items>", "Map")
               % Check("map.11", "[11]", "11", "float")
               % Check("it1.first", "11", "int")
               % Check("it1.second", "11", "float")
               % Check("it6.first", "66", "int")
               % Check("it6.second", "66", "float");

    QTest::newRow("StdMapStringFloat")
            << Data("#include <map>\n",
                    "std::map<QString, float> map;\n"
                    "map[\"11.0\"] = 11.0;\n"
                    "map[\"22.0\"] = 22.0;\n")
               % Check("map", "<2 items>", "std::map<@QString, float>")
               % Check("map.0", "[0]", "", "std::pair<@QString const, float>")
               % Check("map.0.first", "\"11.0\"", "@QString")
               % Check("map.0.second", "11", "float")
               % Check("map.1", "[1]", "", "std::pair<@QString const, float>")
               % Check("map.1.first", "\"22.0\"", "@QString")
               % Check("map.1.second", "22", "float");

    QTest::newRow("StdMapIntString")
            << Data("#include <map>\n",
                    "std::map<int, QString> map;\n"
                    "map[11] = \"11.0\";\n"
                    "map[22] = \"22.0\";\n")
               % Check("map", "<2 items>", "std::map<int, @QString>")
               % Check("map.0", "[0]", "", "std::pair<int const, @QString>")
               % Check("map.0.first", "11", "int")
               % Check("map.0.second", "\"11.0\"", "@QString")
               % Check("map.1", "[1]", "", "std::pair<int const, @QString>")
               % Check("map.1.first", "22", "int")
               % Check("map.1.second", "\"22.0\"", "@QString");

    QTest::newRow("StdMapStringPointer")
            << Data("#include <map>\n",
                    "QObject ob;\n"
                    "std::map<QString, QPointer<QObject> > map;\n"
                    "map[\"Hallo\"] = QPointer<QObject>(&ob);\n"
                    "map[\"Welt\"] = QPointer<QObject>(&ob);\n"
                    "map[\".\"] = QPointer<QObject>(&ob);\n")
               % Check("map", "<3 items>", "std::map<@QString, @QPointer<@QObject>>")
               % Check("map.0", "[0]", "", "std::pair<@QString const, @QPointer<@QObject>>")
               % Check("map.0.first", "", "@QString")
               % Check("map.0.second", "", "@QPointer<@QObject>")
               % Check("map.2", "[2]", "", "std::pair<@QString const, @QPointer<@QObject>>")
               % Check("map.2.first", "\"Welt\"", "@QString");

    QTest::newRow("StdUniquePtr")
            << Data("std::unique_ptr<int> pi(new int(32));\n"
                    "std::unique_ptr<Foo> pf(new Foo);\n")
               % Check("pi", "32", "std::unique_ptr<int, std::default_delete<int> >")
               % Check("pf", "32", "std::unique_ptr<Foo, std::default_delete<Foo> >");

    QTest::newRow("StdSharedPtr")
            << Data("std::shared_ptr<int> pi(new int(32));\n"
                    "std::shared_ptr<Foo> pf(new Foo);\n")
                    % Check("pi", "32", "std::shared_ptr<int, std::default_delete<int> >")
                    % Check("pf", "32", "std::shared_ptr<Foo, std::default_delete<int> >");

    QTest::newRow("StdSetInt")
            << Data("#include <set>\n",
                    "std::set<int> set;\n"
                    "set.insert(11);\n"
                    "set.insert(22);\n"
                    "set.insert(33);\n")
               % Check("set", "<3 items>", "std::set<int>");

    QTest::newRow("StdSetIntIterator")
            << Data("#include <set>\n",
                    "typedef std::set<int> Set;\n"
                    "Set set;\n"
                    "set.insert(11.0);\n"
                    "set.insert(22.0);\n"
                    "set.insert(33.0);\n"
                    "set.insert(44.0);\n"
                    "set.insert(55.0);\n"
                    "set.insert(66.0);\n"
                    "Set::iterator it1 = set.begin();\n"
                    "Set::iterator it2 = it1; ++it2;\n"
                    "Set::iterator it3 = it2; ++it3;\n"
                    "Set::iterator it4 = it3; ++it4;\n"
                    "Set::iterator it5 = it4; ++it5;\n"
                    "Set::iterator it6 = it5; ++it6;\n")
               % Check("set", "<6 items>", "Set")
               % Check("it1.value", "11", "int")
               % Check("it6.value", "66", "int");

    QTest::newRow("StdSetString")
            << Data("#include <set>\n",
                    "std::set<QString> set;\n"
                    "set.insert(\"22.0\");\n")
               % Check("set", "<1 items>", "std::set<Q@String>")
               % Check("set.0", "[0]", "\"22.0\"", "@QString");

    QTest::newRow("StdSetPointer")
            << Data("#include <set>\n",
                    "QObject ob;\n"
                    "std::set<QPointer<QObject> > hash;\n"
                    "QPointer<QObject> ptr(&ob);\n")
               % Check("hash", "<0 items>", "std::set<@QPointer<@QObject>, std::less<@QPointer<@QObject>>, std::allocator<@QPointer<@QObject>>>")
               % Check("ob", "\"\"", "@QObject")
               % Check("ptr", "", "@QPointer<@QObject>");

    QTest::newRow("StdStack1")
            << Data("#include <stack>\n",
                    "std::stack<int *> s0, s;\n"
                    "s.push(new int(1));\n"
                    "s.push(0);\n"
                    "s.push(new int(2));\n")
               % Check("s0", "<0 items>", "std::stack<int*>")
               % Check("s", "<3 items>", "std::stack<int*>")
               % Check("s.0", "[0]", "", "int")
               % Check("s.1", "[1]", "0x0", "int *")
               % Check("s.2", "[2]", "", "int");

    QTest::newRow("StdStack2")
            << Data("#include <stack>\n",
                    "std::stack<int> s0, s;\n"
                    "s.push(1);\n"
                    "s.push(2);\n")
               % Check("s0", "<0 items>", "std::stack<int>")
               % Check("s", "<2 items>", "std::stack<int>")
               % Check("s.0", "1", "int")
               % Check("s.1", "2", "int");

    QTest::newRow("StdStack3")
            << Data("#include <stack>\n",
                    "std::stack<Foo *> s, s0;\n"
                    "s.push(new Foo(1));\n"
                    "s.push(new Foo(2));\n")
               % Check("s", "<0 items>", "std::stack<Foo*>")
               % Check("s", "<1 items>", "std::stack<Foo*>")
               % Check("s", "<2 items>", "std::stack<Foo*>")
               % Check("s.0", "", "Foo")
               % Check("s.0.a", "1", "int")
               % Check("s.1", "", "Foo")
               % Check("s.1.a", "2", "int");

    QTest::newRow("StdStack4")
            << Data("#include <stack>\n",
                    "std::stack<Foo> s0, s;\n"
                    "s.push(1);\n"
                    "s.push(2);\n")
               % Check("s0", "<0 items>", "std::stack<Foo>")
               % Check("s", "<2 items>", "std::stack<Foo>")
               % Check("s.0", "[0]", "", "Foo")
               % Check("s.0.a", "1", "int")
               % Check("s.1", "[1]", "", "Foo")
               % Check("s.1.a", "2", "int");

    QTest::newRow("StdString1")
            << Data("#include <string>\n",
                    "std::string str0, str;\n"
                    "std::wstring wstr0, wstr;\n"
                    "str += \"b\";\n"
                    "wstr += wchar_t('e');\n"
                    "str += \"d\";\n"
                    "wstr += wchar_t('e');\n"
                    "str += \"e\";\n"
                    "str += \"b\";\n"
                    "str += \"d\";\n"
                    "str += \"e\";\n"
                    "wstr += wchar_t('e');\n"
                    "wstr += wchar_t('e');\n"
                    "str += \"e\";\n")
               % Check("str0", "\"\"", "std::string")
               % Check("wstr0", "\"\"", "std::wstring")
               % Check("str", "\"bdebdee\"", "std::string")
               % Check("wstr", "\"eeee\"", "std::wstring");

    QTest::newRow("StdString2")
            << Data("#include <string>\n",
                    "std::string str = \"foo\";\n"
                    "std::vector<std::string> v;\n"
                    "QList<std::string> l0, l;\n"
                    "v.push_back(str);\n"
                    "v.push_back(str);\n"
                    "l.push_back(str);\n"
                    "l.push_back(str);\n")
               % Check("l0", "<0 items>", "@QList<std::string>")
               % Check("l", "<2 items>", "@QList<std::string>")
               % Check("str", "\"foo\"", "std::string")
               % Check("v", "<2 items>", "std::vector<std::string>")
               % Check("v.0", "[0]", "\"foo\"", "std::string");

    QTest::newRow("StdVector1")
            << Data("#include <vector>\n",
                    "std::vector<int *> v0, v;\n"
                    "v.push_back(new int(1));\n"
                    "v.push_back(0);\n"
                    "v.push_back(new int(2));\n")
               % Check("v0", "<0 items>", "std::vector<int*>")
               % Check("v", "<3 items>", "std::vector<int*>")
               % Check("v.0", "[1]", "1", "int")
               % Check("v.1", "0x0", "int *")
               % Check("v.2", "[2]", "2", "int");

    QTest::newRow("StdVector2")
            << Data("#include <vector>\n",
                    "std::vector<int> v;\n"
                    "v.push_back(1);\n"
                    "v.push_back(2);\n"
                    "v.push_back(3);\n"
                    "v.push_back(4);\n")
               % Check("v", "<4 items>", "std::vector<int>")
               % Check("v.0", "[0]", "1", "int")
               % Check("v.3", "[3]", "4", "int");

    QTest::newRow("StdVector3")
            << Data("#include <vector>\n",
                    "std::vector<Foo *> v;\n"
                    "v.push_back(new Foo(1));\n"
                    "v.push_back(0);\n"
                    "v.push_back(new Foo(2));\n")
               % Check("v", "<3 items>", "std::vector<Foo*>")
               % Check("v.0", "[0]", "", "Foo")
               % Check("v.0.a", "1", "int")
               % Check("v.1", "[1]", "0x0", "Foo *")
               % Check("v.2", "[2]", "", "Foo")
               % Check("v.2.a", "2", "int");

    QTest::newRow("StdVector4")
            << Data("#include <vector>\n",
                    "std::vector<Foo> v;\n"
                    "v.push_back(1);\n"
                    "v.push_back(2);\n"
                    "v.push_back(3);\n"
                    "v.push_back(4);\n")
               % Check("v", "<4 items>", "std::vector<Foo>")
               % Check("v.0", "[0]", "", "Foo")
               % Check("v.1.a", "2", "int")
               % Check("v.3", "[3]", "", "Foo");

    QTest::newRow("StdVectorBool1")
            << Data("#include <vector>\n",
                    "std::vector<bool> v;\n"
                    "v.push_back(true);\n"
                    "v.push_back(false);\n"
                    "v.push_back(false);\n"
                    "v.push_back(true);\n"
                    "v.push_back(false);\n")
               % Check("v", "<5 items>", "std::vector<bool>")
               % Check("v.0", "[0]", "true", "bool")
               % Check("v.1", "[1]", "false", "bool")
               % Check("v.2", "[2]", "false", "bool")
               % Check("v.3", "[3]", "true", "bool")
               % Check("v.4", "[4]", "false", "bool");

    QTest::newRow("StdVectorBool2")
            << Data("#include <vector>\n",
                    "std::vector<bool> v1(65, true);\n"
                    "std::vector<bool> v2(65);\n")
               % Check("v1", "<65 items>", "std::vector<bool>")
               % Check("v1.0", "[0]", "true", "bool")
               % Check("v1.64", "[64]", "true", "bool")
               % Check("v2", "<65 items>", "std::vector<bool>")
               % Check("v2.0", "[0]", "false", "bool")
               % Check("v2.64", "[64]", "false", "bool");

    QTest::newRow("StdVector6")
            << Data("#include <vector>\n",
                    "std::vector<std::list<int> *> vector;\n"
                    "std::list<int> list;\n"
                    "vector.push_back(new std::list<int>(list));\n"
                    "vector.push_back(0);\n"
                    "list.push_back(45);\n"
                    "vector.push_back(new std::list<int>(list));\n"
                    "vector.push_back(0);\n")
               % Check("list", "<1 items>", "std::list<int>")
               % Check("list.0", "[0]", "45", "int")
               % Check("vector", "<4 items>", "std::vector<std::list<int>*>")
               % Check("vector.0", "[0]", "<0 items>", "std::list<int>")
               % Check("vector.2", "[2]", "<1 items>", "std::list<int>")
               % Check("vector.2.0", "45", "int")
               % Check("vector.3", "[3]", "0x0", "std::list<int> *");

    QTest::newRow("StdStream")
            << Data("#include <vector>\n",
                    "using namespace std;\n"
                    "ifstream is0, is;\n"
                    "#ifdef Q_OS_WIN\n"
                    "        is.open(\"C:\\\\Program Files\\\\Windows NT\\\\Accessories\\\\wordpad.exe\");\n"
                    "#else\n"
                    "        is.open(\"/etc/passwd\");\n"
                    "#endif\n"
                    "        bool ok = is.good();\n")
               % Check("is", "", "std::ifstream")
               % Check("ok", "true", "bool");

    QTest::newRow("ItemModel")
            << Data("#include <QStandardItemModel>\n",
                    "QStandardItemModel m;\n"
                    "QStandardItem *i1, *i2, *i11;\n"
                    "m.appendRow(QList<QStandardItem *>()\n"
                    "     << (i1 = new QStandardItem(\"1\")) << (new QStandardItem(\"a\")) << (new QStandardItem(\"a2\")));\n"
                    "QModelIndex mi = i1->index();\n"
                    "m.appendRow(QList<QStandardItem *>()\n"
                    "     << (i2 = new QStandardItem(\"2\")) << (new QStandardItem(\"b\")));\n"
                    "i1->appendRow(QList<QStandardItem *>()\n"
                    "     << (i11 = new QStandardItem(\"11\")) << (new QStandardItem(\"aa\")));\n")
               % Check("i1", "", "@QStandardItem")
               % Check("i11", "", "@QStandardItem")
               % Check("i2", "", "@QStandardItem")
               % Check("m", "", "@QStandardItemModel")
               % Check("mi", "\"1\"", "@QModelIndex");

    QTest::newRow("QStackInt")
            << Data("#include <QStack>",
                    "QStack<int> s;\n"
                    "s.append(1);\n"
                    "s.append(2);\n")
               % Check("s", "<2 items>", "@QStack<int>")
               % Check("s.0", "[0]", "1", "int")
               % Check("s.1", "[1]", "2", "int");

    QTest::newRow("QStackBig")
            << Data("#include <QStack>",
                    "QStack<int> s;\n"
                    "for (int i = 0; i != 10000; ++i)\n"
                    "    s.append(i);\n")
               % Check("s", "<10000 items>", "@QStack<int>")
               % Check("s.0", "[0]", "0", "int")
               % Check("s.1999", "[1999]", "1999", "int");

    QTest::newRow("QStackFooPointer")
            << Data("#include <QStack>",
                    "QStack<Foo *> s;\n"
                    "s.append(new Foo(1));\n"
                    "s.append(0);\n"
                    "s.append(new Foo(2));\n")
               % Check("s", "<3 items>", "@QStack<Foo*>")
               % Check("s.0", "[0]", "", "Foo")
               % Check("s.0.a", "1", "int")
               % Check("s.1", "[1]", "0x0", "Foo *")
               % Check("s.2", "[2]", "", "Foo")
               % Check("s.2.a", "2", "int");

    QTest::newRow("QStackFoo")
            << Data("#include <QStack>" + fooData,
                    "QStack<Foo> s;\n"
                    "s.append(1);\n"
                    "s.append(2);\n"
                    "s.append(3);\n"
                    "s.append(4);\n")
               % Check("s", "<4 items>", "@QStack<Foo>")
               % Check("s.0", "[0]", "", "Foo")
               % Check("s.0.a", "1", "int")
               % Check("s.3", "[3]", "", "Foo")
               % Check("s.3.a", "4", "int");

    QTest::newRow("QStackBool")
            << Data("#include <QStack>",
                    "QStack<bool> s;\n"
                    "s.append(true);\n"
                    "s.append(false);\n")
               % Check("s", "<2 items>", "@QStack<bool>")
               % Check("s.0", "true", "bool")
               % Check("s.1", "false", "bool");

    QTest::newRow("QUrl")
            << Data("#include <QUrl>",
                    "QUrl url(QString(\"http://qt-project.org\"));\n")
               % Check("url", "\"http://qt-project.org\"", "@QUrl");

    QTest::newRow("QStringQuotes")
            << Data("QString str1(\"Hello Qt\");\n"
                    // --> Value: \"Hello Qt\"
                    "QString str2(\"Hello\\nQt\");\n"
                    // --> Value: \\"\"Hello\nQt"" (double quote not expected)
                    "QString str3(\"Hello\\rQt\");\n"
                    // --> Value: ""HelloQt"" (double quote and missing \r not expected)
                    "QString str4(\"Hello\\tQt\");\n")
               // --> Value: "Hello\9Qt" (expected \t instead of \9)
               % Check("str1", "\"Hello Qt\"", "@QString")
               % Check("str2", "\"Hello\nQt\"", "@QString")
               % Check("str3", "\"Hello\rQt\"", "@QString")
               % Check("str4", "\"Hello\tQt\"", "@QString");

    QByteArray stringData = "QString str = \"Hello \";\n"
                    "str += '\\t';\n"
                    "str += '\\r';\n"
                    "str += '\\n';\n"
                    "str += QLatin1Char(0);\n"
                    "str += QLatin1Char(1);\n"
                    "str += \" World\";\n"
                    "str.prepend(\"Prefix: \");\n";

    QTest::newRow("QString1")
            << Data(stringData)
               % Check("str", "\"Prefix: Hello  big, \\t\\r\\n\\000\\001 World\"", "@QString");

    QTest::newRow("QString2")
            << Data("QChar data[] = { 'H', 'e', 'l', 'l', 'o' };\n"
                    "QString str1 = QString::fromRawData(data, 4);\n"
                    "QString str2 = QString::fromRawData(data + 1, 4);\n")
               % Check("str1", "\"Hell\"", "@QString")
               % Check("str2", "\"ello\"", "@QString");

    QTest::newRow("QString3")
                  << Data(
        "QString str = \"Hello \";\n"
        "str += \" big, \";\n"
        "str += \" World \";\n"
        "QString string(\"String Test\");\n"
        "QString *pstring = new QString(\"Pointer String Test\");\n"
        "stringRefTest(QString(\"Ref String Test\"));\n"
        "string = \"Hi\";\n"
        "string += \"Du\";\n"
//void stringRefTest(const QString &refstring)\n"
//        dummyStatement(&refstring);\n"
             )
         % Check("pstring", "", "@QString")
         % Check("str", "\"Hello World\"", "@QString")
         % Check("string", "\"HiDu\"", "@QString");

    QTest::newRow("QStringRef1")
            << Data("QString str = \"Hello\";\n"
                    "QStringRef ref(&str, 1, 2);")
               % Check("ref", "\"el\"", "@QStringRef");

    QTest::newRow("QStringList")
            << Data("#include <QStringList>\n",
                    "QStringList l;\n"
                    "l << \"Hello \";\n"
                    "l << \" big, \";\n"
                    "l.takeFirst();\n"
                    "l << \" World \";\n")
               % Check("l", "<2 items>", "@QStringList")
               % Check("l.0", "\" big, \"", "@QString")
               % Check("l.1", "\" World \"", "@QString");

    QTest::newRow("String")
            << Data("#include <QString>",
                    "const wchar_t *w = L\"aöa\";\n"
                    "QString u;\n"
                    "if (sizeof(wchar_t) == 4)\n"
                    "    u = QString::fromUcs4((uint *)w);\n"
                    "else\n"
                    "    u = QString::fromUtf16((ushort *)w);\n")
               % Check("u", "\"aöa\"", "@QString")
               % CheckType("w", "", "wchar_t *");

        // All: Select UTF-8 in "Change Format for Type" in L&W context menu");
        // Windows: Select UTF-16 in "Change Format for Type" in L&W context menu");
        // Other: Select UCS-6 in "Change Format for Type" in L&W context menu");


    // These tests should result in properly displayed umlauts in the
    // Locals&Watchers view. It is only support on gdb with Python");
    QTest::newRow("CharPointers")
            << Data("const char *s = \"aöa\";\n"
                    "const char *t = \"a\\xc3\\xb6\";\n"
                    "const unsigned char uu[] = { 'a', 153 /* ö Latin1 */, 'a' };\n"
                    "const unsigned char *u = uu;\n"
                    "const wchar_t *w = L\"aöa\";\n")
               % CheckType("u", "unsigned char *")
               % CheckType("uu", "unsigned char [3]")
               % CheckType("s", "char *")
               % CheckType("t", "char *")
               % CheckType("w", "wchar_t *");

        // All: Select UTF-8 in "Change Format for Type" in L&W context menu");
        // Windows: Select UTF-16 in "Change Format for Type" in L&W context menu");
        // Other: Select UCS-6 in "Change Format for Type" in L&W context menu");


    QTest::newRow("CharArrays")
            << Data("const char s[] = \"aöa\";\n"
                    "const char t[] = \"aöax\";\n"
                    "const wchar_t w[] = L\"aöa\";\n")
               % CheckType("s", "char [5]")
               % CheckType("t", "char [6]")
               % CheckType("w", "wchar_t [4]");


    QTest::newRow("Text")
            << Data("#include <QApplication>\n"
                    "#include <QTextCursor>\n"
                    "#include <QTextDocument>\n",
                    "QApplication app(arrc, argv)\n"
                    "QTextDocument doc;\n"
                    "doc.setPlainText(\"Hallo\\nWorld\");\n"
                    "QTextCursor tc;\n"
                    "tc = doc.find(\"all\");\n"
                    "int pos = tc.position();\n"
                    "int anc = tc.anchor();\n")
               % CheckType("doc", "QTextDocument")
               % Check("tc", "4", "QTextCursor")
               % Check("pos", "4", "int")
               % Check("anc", "1", "int");

    QTest::newRow("QThread")
            << Data("#include <QThread>\n",
                    "const int N = 14;\n"
                    "Thread thread[N];\n"
                    "for (int i = 0; i != N; ++i) {\n"
                    "    thread[i].setId(i);\n"
                    "    thread[i].setObjectName(\"This is thread #\" + QString::number(i));\n"
                    "    thread[i].start();\n"
                    "}\n")
               % CheckType("thread", "Thread [14]")
               % Check("thread.0", "", "Thread")
               % Check("thread.0.@1.@1", "\"This is thread #0\"", "Thread")
               % Check("thread.13", "", "Thread")
               % Check("thread.13.@1.@1", "\"This is thread #13\"", "Thread");

    QByteArray variantData =
            "Q_DECLARE_METATYPE(QHostAddress)\n"
            "Q_DECLARE_METATYPE(QList<int>)\n"
            "Q_DECLARE_METATYPE(QStringList)\n"
            "#define COMMA ,\n"
            "Q_DECLARE_METATYPE(QMap<uint COMMA QStringList>)\n";

    QTest::newRow("QVariant1")
            << Data("#include <QVariant>\n",
                    "QVariant value;\n"
                    "QVariant::Type t = QVariant::String;\n"
                    "value = QVariant(t, (void*)0);\n"
                    "*(QString*)value.data() = QString(\"Some string\");\n"
                    "int i = 1;\n")
               % Check("t", "QVariant::String (10)", "@QVariant::Type")
               % Check("value", "\"Some string\"", "@QVariant (QString)");

    QTest::newRow("QVariant2")
            << Data("#include <QVariant>\n" + variantData,
                    "QVariant var;                               // Type 0, invalid\n"
                    "QVariant var1(true);                        // 1, bool\n"
                    "QVariant var2(2);                           // 2, int\n"
                    "QVariant var3(3u);                          // 3, uint\n"
                    "QVariant var4(qlonglong(4));                // 4, qlonglong\n"
                    "QVariant var5(qulonglong(5));               // 5, qulonglong\n"
                    "QVariant var6(double(6));                   // 6, double\n"
                    "QVariant var7(QChar(7));                    // 7, QChar\n"
                    //None,          # 8, QVariantMap
                    // None,          # 9, QVariantList
                    "QVariant var10(QString(\"Hello 10\"));      // 10, QString\n"
                    "QVariant var19(QRect(100, 200, 300, 400));  // 19 QRect\n"
                    "QVariant var20(QRectF(100, 200, 300, 400)); // 20 QRectF\n"
                    )
               % Check("var", "(invalid)", "@QVariant (invalid)")
               % Check("var1", "true", "@QVariant (bool)")
               % Check("var2", "2", "@QVariant (int)")
               % Check("var3", "3", "@QVariant (uint)")
               % Check("var4", "4", "@QVariant (qlonglong)")
               % Check("var5", "5", "@QVariant (qulonglong)")
               % Check("var6", "6", "@QVariant (double)")
               % Check("var7", "'?' (7)", "@QVariant (QChar)")
               % Check("var10", "\"Hello 10\"", "@QVariant (QString)")
               % Check("var19", "300x400+100+200", "@QVariant (QRect)")
               % Check("var20", "300x400+100+200", "@QVariant (QRectF)");

        /*
         "QStringList", # 11
         "QByteArray",  # 12
         "QBitArray",   # 13
         "QDate",       # 14
         "QTime",       # 15
         "QDateTime",   # 16
         "QUrl",        # 17
         "QLocale",     # 18
         "QRect",       # 19
         "QRectF",      # 20
         "QSize",       # 21
         "QSizeF",      # 22
         "QLine",       # 23
         "QLineF",      # 24
         "QPoint",      # 25
         "QPointF",     # 26
         "QRegExp",     # 27
         */

//    QTest::newRow("QVariant3")
//                  << Data(
//        "QVariant var;\n"
//         % Check("var (invalid) QVariant (invalid)");
//         % Check("the list is updated properly");
//        "var.setValue(QStringList() << "World");\n"
//         % Check("var <1 items> QVariant (QStringList)");
//         % Check("var.0 "World" QString");
//        "var.setValue(QStringList() << "World" << "Hello");\n"
//         % Check("var <2 items> QVariant (QStringList)");
//         % Check("var.1 "Hello" QString");
//        "var.setValue(QStringList() << "Hello" << "Hello");\n"
//         % Check("var <2 items> QVariant (QStringList)");
//         % Check("var.0 "Hello" QString");
//         % Check("var.1 "Hello" QString");
//        "var.setValue(QStringList() << "World" << "Hello" << "Hello");\n"
//         % Check("var <3 items> QVariant (QStringList)");
//         % Check("var.0 "World" QString");

    QTest::newRow("QVariant4")
            << Data("#include <QHostAddress>\n"
                    "#include <QVariant>\n",
                    "QVariant var;\n"
                    "QHostAddress ha(\"127.0.0.1\");\n"
                    "var.setValue(ha);\n"
                    "QHostAddress ha1 = var.value<QHostAddress>();\n")
               % Check("ha", "\"127.0.0.1\"", "@QHostAddress")
               % Check("ha.a", "0", "quint32")
               % Check("ha.a6", "", "Q_IPV6ADDR")
               % Check("ha.ipString", "\"127.0.0.1\"", "@QString")
               % Check("ha.isParsed", "false", "bool")
               % Check("ha.protocol", "QAbstractSocket::UnknownNetworkLayerProtocol (-1)",
                       "@QAbstractSocket::NetworkLayerProtocol")
               % Check("ha.scopeId", "", "@QString")
               % Check("ha1", "\"127.0.0.1\"", "@QHostAddress")
               % Check("ha1.a", "0", "quint32")
               % Check("ha1.a6", "", "Q_IPV6ADDR")
               % Check("ha1.ipString", "\"127.0.0.1\"", "@QString")
               % Check("ha1.isParsed", "false", "bool")
               % Check("ha1.protocol", "QAbstractSocket::UnknownNetworkLayerProtocol (-1)",
                       "@QAbstractSocket::NetworkLayerProtocol")
               % Check("ha1.scopeId", "", "@QString")
               % CheckType("var", "", "@QVariant (QHostAddress)")
               % Check("var.data", "\"127.0.0.1\"", "@QHostAddress");

//    QTest::newRow("QVariant5")
//                          << Data(
//        // This checks user defined types in QVariants");
//        "typedef QMap<uint, QStringList>\n" MyType;
//        "MyType my;\n"
//        "my[1] = (QStringList() << "Hello");\n"
//        "my[3] = (QStringList() << "World");\n"
//        "QVariant var;\n"
//        "var.setValue(my);\n"
//        // FIXME: Known to break\n"
//        //QString type = var.typeName();\n"
//        "var.setValue(my);\n"
//         % Check("my <2 items> qvariant::MyType");
//         % Check("my.0   QMapNode<unsigned int, QStringList>");
//         % Check("my.0.key 1 unsigned int");
//         % Check("my.0.value <1 items> QStringList");
//         % Check("my.0.value.0 "Hello" QString");
//         % Check("my.1   QMapNode<unsigned int, QStringList>");
//         % Check("my.1.key 3 unsigned int");
//         % Check("my.1.value <1 items> QStringList");
//         % Check("my.1.value.0 "World" QString");
//         % CheckType("var QVariant (QMap<unsigned int , QStringList>)");
//         % Check("var.data <2 items> QMap<unsigned int, QStringList>");
//         % Check("var.data.0   QMapNode<unsigned int, QStringList>");
//         % Check("var.data.0.key 1 unsigned int");
//         % Check("var.data.0.value <1 items> QStringList");
//         % Check("var.0.value.0 "Hello" QString");
//         % Check("var.data.1   QMapNode<unsigned int, QStringList>");
//         % Check("var.data.1.key 3 unsigned int");
//         % Check("var.data.1.value <1 items> QStringList");
//         % Check("var.data.1.value.0 "World" QString");
//        // Continue");
//        var.setValue(my);

//    QTest::newRow("QVariant6")
//                              << Data(
//        "QList<int> list;\n"
//        "list << 1 << 2 << 3;\n"
//        "QVariant variant = qVariantFromValue(list);\n"
//         % Check("list <3 items> QList<int>");
//         % Check("list.0", "1", "int");
//         % Check("list.1", "2", "int");
//         % Check("list.2", "3", "int");
//         % CheckType("variant QVariant (QList<int>)");
//         % Check("variant.data <3 items> QList<int>");
//         % Check("variant.data.0", "1", "int");
//         % Check("variant.data.1", "2", "int");
//         % Check("variant.data.2", "3", "int");
//        // Continue");
//        list.clear();\n"
//        list = variant.value<QList<int> >();\n"
//         % Check("list <3 items> QList<int>");\n"
//         % Check("list.0", "1", "int");
//         % Check("list.1", "2", "int");
//         % Check("list.2", "3", "int");

//    QTest::newRow("QVariantList")
//                                  << Data(
//        "QVariantList vl;\n"
//        " % Check("vl <0 items> QVariantList");\n"
//        "// Continue");\n"
//        "vl.append(QVariant(1));\n"
//        "vl.append(QVariant(2));\n"
//        "vl.append(QVariant("Some String"));\n"
//        "vl.append(QVariant(21));\n"
//        "vl.append(QVariant(22));\n"
//        "vl.append(QVariant("2Some String"));\n"
//         % Check("vl <6 items> QVariantList");
//         % CheckType("vl.0 QVariant (int)");
//         % CheckType("vl.2 QVariant (QString)");

//    QTest::newRow("QVariantMap")
//                                      << Data(
//        "QVariantMap vm;\n"
//         % Check("vm <0 items> QVariantMap");
//        "// Continue");\n"
//        "vm["a"] = QVariant(1);\n"
//        "vm["b"] = QVariant(2);\n"
//        "vm["c"] = QVariant("Some String");\n"
//        "vm["d"] = QVariant(21);\n"
//        "vm["e"] = QVariant(22);\n"
//        "vm["f"] = QVariant("2Some String");\n"
//         % Check("vm <6 items> QVariantMap");
//         % Check("vm.0   QMapNode<QString, QVariant>");
//         % Check("vm.0.key "a" QString");
//         % Check("vm.0.value 1 QVariant (int)");
//         % Check("vm.5   QMapNode<QString, QVariant>");
//         % Check("vm.5.key "f" QString");
//         % Check("vm.5.value "2Some String" QVariant (QString)");

//    QTest::newRow("QVectorIntBig")
//          << Data(
//        // This tests the display of a big vector");
//        QVector<int> vec(10000);
//        for (int i = 0; i != vec.size(); ++i)
//            vec[i] = i * i;
//         % Check("vec <10000 items> QVector<int>");
//         % Check("vec.0", "0", "int");
//         % Check("vec.1999", "3996001", "int");
//        // Continue");

//        // step over
//        // check that the display updates in reasonable time
//        vec[1] = 1;
//        vec[2] = 2;
//        vec.append(1);
//        vec.append(1);
//         % Check("vec <10002 items> QVector<int>");
//         % Check("vec.1", "1", "int");
//         % Check("vec.2", "2", "int");

//    QTest::newRow("QVectorFoo")
//                 << Data(
//        // This tests the display of a vector of pointers to custom structs");\n"
//        "QVector<Foo> vec;\n"
//         % Check("vec <0 items> QVector<Foo>");
//        // Continue");\n"
//        // step over, check display");\n"
//        "vec.append(1);\n"
//        "vec.append(2);\n"
//         % Check("vec <2 items> QVector<Foo>");
//         % Check("vec.0", "Foo");
//         % Check("vec.0.a", "1", "int");
//         % Check("vec.1", "Foo");
//         % Check("vec.1.a", "2", "int");

//    typedef QVector<Foo> FooVector;

//    QTest::newRow("QVectorFooTypedef")
//                 << Data(
//    {
//        "FooVector vec;\n"
//        "vec.append(Foo(2));\n"
//        "vec.append(Foo(3));\n"
//        dummyStatement(&vec);
//    }

//    QTest::newRow("QVectorFooStar")
//                     << Data(
//    {
//        // This tests the display of a vector of pointers to custom structs");
//        "QVector<Foo *> vec;\n"
//         % Check("vec <0 items> QVector<Foo*>");
//        // Continue");\n"
//        // step over\n"
//        // check that the display is ok");\n"
//        "vec.append(new Foo(1));\n"
//        "vec.append(0);\n"
//        "vec.append(new Foo(2));\n"
//        "vec.append(new Fooooo(3));\n"
//        // switch "Auto derefencing pointers" in Locals context menu\n"
//        // off and on again, and check result looks sane");\n"
//         % Check("vec <4 items> QVector<Foo*>");
//         % Check("vec.0", "Foo");
//         % Check("vec.0.a", "1", "int");
//         % Check("vec.1 0x0 Foo *");
//         % Check("vec.2", "Foo");
//         % Check("vec.2.a", "2", "int");
//         % Check("vec.3", "Fooooo");
//         % Check("vec.3.a", "5", "int");

//    QTest::newRow("QVectorBool")
//                         << Data(
//        QVector<bool> vec;
//         % Check("vec <0 items> QVector<bool>");
//        // Continue");
//        // step over
//        // check that the display is ok");
//        vec.append(true);
//        vec.append(false);
//         % Check("vec <2 items> QVector<bool>");
//         % Check("vec.0", "true", "bool");
//         % Check("vec.1", "false", "bool");

//    QTest::newRow("QVectorListInt")
//                             << Data(
//        "QVector<QList<int> > vec;
//        "QVector<QList<int> > *pv = &vec;
//         % Check("pv", "QVector<QList<int>>");
//         % Check("vec <0 items> QVector<QList<int>>");
//        // Continue");
//        "vec.append(QList<int>() << 1);
//        "vec.append(QList<int>() << 2 << 3);
//         % Check("pv", "QVector<QList<int>>");
//         % Check("pv.0 <1 items> QList<int>");
//         % Check("pv.0.0", "1", "int");
//         % Check("pv.1 <2 items> QList<int>");
//         % Check("pv.1.0", "2", "int");
//         % Check("pv.1.1", "3", "int");
//         % Check("vec <2 items> QVector<QList<int>>");
//         % Check("vec.0 <1 items> QList<int>");
//         % Check("vec.0.0", "1", "int");
//         % Check("vec.1 <2 items> QList<int>");
//         % Check("vec.1.0", "2", "int");
//         % Check("vec.1.1", "3", "int");

//namespace noargs {

//    class Goo
//    {
//    public:
//       Goo(const QString &str, const int n) : str_(str), n_(n) {}
//    private:
//       QString str_;
//       int n_;
//    };

//    typedef QList<Goo> GooList;

//    QTest::newRow("NoArgumentName(int i, int, int k)
//    {
//        // This is not supposed to work with the compiled dumpers");
//        "GooList list;
//        "list.append(Goo("Hello", 1));
//        "list.append(Goo("World", 2));

//        "QList<Goo> list2;
//        "list2.append(Goo("Hello", 1));
//        "list2.append(Goo("World", 2));

//         % Check("i", "1", "int");
//         % Check("k", "3", "int");
//         % Check("list <2 items> noargs::GooList");
//         % Check("list.0", "noargs::Goo");
//         % Check("list.0.n_", "1", "int");
//         % Check("list.0.str_ "Hello" QString");
//         % Check("list.1", "noargs::Goo");
//         % Check("list.1.n_", "2", "int");
//         % Check("list.1.str_ "World" QString");
//         % Check("list2 <2 items> QList<noargs::Goo>");
//         % Check("list2.0", "noargs::Goo");
//         % Check("list2.0.n_", "1", "int");
//         % Check("list2.0.str_ "Hello" QString");
//         % Check("list2.1", "noargs::Goo");
//         % Check("list2.1.n_", "2", "int");
//         % Check("list2.1.str_ "World" QString");


//"void foo() {}\n"
//"void foo(int) {}\n"
//"void foo(QList<int>) {}\n"
//"void foo(QList<QVector<int> >) {}\n"
//"void foo(QList<QVector<int> *>) {}\n"
//"void foo(QList<QVector<int *> *>) {}\n"
//"\n"
//"template <class T>\n"
//"void foo(QList<QVector<T> *>) {}\n"
//"\n"
//"\n"
//"namespace namespc {\n"
//"\n"
//"    class MyBase : public QObject\n"
//"    {\n"
//"    public:\n"
//"        MyBase() {}\n"
//"        virtual void doit(int i)\n"
//"        {\n"
//"           n = i;\n"
//"        }\n"
//"    protected:\n"
//"        int n;\n"
//"    };\n"
//"\n"
//"    namespace nested {\n"
//"\n"
//"        class MyFoo : public MyBase\n"
//"        {\n"
//"        public:\n"
//"            MyFoo() {}\n"
//"            virtual void doit(int i)\n"
//"            {\n"
//"                // Note there's a local 'n' and one in the base class");\n"
//"                n = i;\n"
//"            }\n"
//"        protected:\n"
//"            int n;\n"
//"        };\n"
//"\n"
//"        class MyBar : public MyFoo\n"
//"        {\n"
//"        public:\n"
//"            virtual void doit(int i)\n"
//"            {\n"
//"               n = i + 1;\n"
//"            }\n"
//"        };\n"
//"\n"
//"        namespace {\n"
//"\n"
//"            class MyAnon : public MyBar\n"
//"            {\n"
//"            public:\n"
//"                virtual void doit(int i)\n"
//"                {\n"
//"                   n = i + 3;\n"
//"                }\n"
//"            };\n"
//"\n"
//"            namespace baz {\n"
//"\n"
//"                class MyBaz : public MyAnon\n"
//"                {\n"
//"                public:\n"
//"                    virtual void doit(int i)\n"
//"                    {\n"
//"                       n = i + 5;\n"
//"                    }\n"
//"                };\n"
//"\n"
//"            } // namespace baz\n"
//"\n"
//"        } // namespace anon\n"
//"\n"
//"    } // namespace nested\n"
//"\n"
//    QTest::newRow("Namespace()
//    {
//        // This checks whether classes with "special" names are
//        // properly displayed");
//        "using namespace nested;\n"
//        "MyFoo foo;\n"
//        "MyBar bar;\n"
//        "MyAnon anon;\n"
//        "baz::MyBaz baz;\n"
//         % CheckType("anon namespc::nested::(anonymous namespace)::MyAnon");
//         % Check("bar", "namespc::nested::MyBar");
//         % CheckType("baz namespc::nested::(anonymous namespace)::baz::MyBaz");
//         % Check("foo", "namespc::nested::MyFoo");
//        // Continue");
//        // step into the doit() functions

//        baz.doit(1);\n"
//        anon.doit(1);\n"
//        foo.doit(1);\n"
//        bar.doit(1);\n"
//        dummyStatement();\n"
//    }


//    QTest::newRow("GccExtensions")
//        "#ifdef __GNUC__\n"
//        "char v[8] = { 1, 2 };\n"
//        "char w __attribute__ ((vector_size (8))) = { 1, 2 };\n"
//        "int y[2] = { 1, 2 };\n"
//        "int z __attribute__ ((vector_size (8))) = { 1, 2 };\n"
//        "#endif\n"
//         % Check("v.0", "1", "char");
//         % Check("v.1", "2", "char");
//         % Check("w.0", "1", "char");
//         % Check("w.1", "2", "char");
//         % Check("y.0", "1", "int");
//         % Check("y.1", "2", "int");
//         % Check("z.0", "1", "int");
//         % Check("z.1", "2", "int");


//class Z : public QObject
//{
//public:
//    Z() {
//        f = new Foo();
//        i = 0;
//        i = 1;
//        i = 2;
//        i = 3;
//    }
//    int i;
//    Foo *f;
//};

//QString fooxx()
//{
//    return "bababa";
//}


//    QTest::newRow("Int()
//        "quint64 u64 = ULLONG_MAX;\n"
//        "qint64 s64 = LLONG_MAX;\n"
//        "quint32 u32 = ULONG_MAX;\n"
//        "qint32 s32 = LONG_MAX;\n"
//        "quint64 u64s = 0;\n"
//        "qint64 s64s = LLONG_MIN;\n"
//        "quint32 u32s = 0;\n"
//        "qint32 s32s = LONG_MIN;\n"

//         % Check("u64", "18446744073709551615", "quint64");
//         % Check("s64", "9223372036854775807", "qint64");
//         % Check("u32", "4294967295", "quint32");
//         % Check("s32", "2147483647", "qint32");
//         % Check("u64s", "0", "quint64");
//         % Check("s64s", "-9223372036854775808", "qint64");
//         % Check("u32s", "0", "quint32");
//         % Check("s32s", "-2147483648", "qint32");


//    QTest::newRow("Array1")
//                  << Data(
//        "double d[3][3];\n"
//        "for (int i = 0; i != 3; ++i)\n"
//        "    for (int j = 0; j != 3; ++j)\n"
//        "        d[i][j] = i + j;\n"
//         % CheckType("d double [3][3]");
//         % CheckType("d.0 double [3]");
//         % Check("d.0.0", "0", "double");
//         % Check("d.0.2", "2", "double");
//         % CheckType("d.2 double [3]");

//    QTest::newRow("Array2")
//                  << Data(
//        "char c[20];\n"
//        "c[0] = 'a';\n"
//        "c[1] = 'b';\n"
//        "c[2] = 'c';\n"
//        "c[3] = 'd';\n"
//         % CheckType("c char [20]");
//         % Check("c.0 97 'a' char");
//         % Check("c.3 100 'd' char");

//    QTest::newRow("Array3")
//                  << Data(
//        "QString s[20];
//        "s[0] = "a";\n"
//        "s[1] = "b";\n"
//        "s[2] = "c";\n"
//        "s[3] = "d";\n"
//         % CheckType("s QString [20]");
//         % Check("s.0 "a" QString");
//         % Check("s.3 "d" QString");
//         % Check("s.4 "" QString");
//         % Check("s.19 "" QString");

//    QTest::newRow("Array4")
//                  << Data(
//        "QByteArray b[20];\n"
//        "b[0] = "a";\n"
//        "b[1] = "b";\n"
//        "b[2] = "c";\n"
//        "b[3] = "d";\n"
//         % CheckType("b QByteArray [20]");
//         % Check("b.0 "a" QByteArray");
//         % Check("b.3 "d" QByteArray");
//         % Check("b.4 "" QByteArray");
//         % Check("b.19 "" QByteArray");

//    QTest::newRow("Array5")
//                  << Data(
//        "Foo foo[10];\n"
//        "//for (int i = 0; i != sizeof(foo)/sizeof(foo[0]); ++i) {\n"
//        "for (int i = 0; i < 5; ++i) {\n"
//        "    foo[i].a = i;\n"
//        "    foo[i].doit")\n"
//        "                  << Data(\n"
//        "}\n"
//         % CheckType("foo Foo [10]");
//         % Check("foo.0", "Foo");
//         % Check("foo.9", "Foo");


    QTest::newRow("Bitfields")
            << Data("struct S\n"
                    "{\n"
                    "    uint x : 1;\n"
                    "    uint y : 1;\n"
                    "    bool c : 1;\n"
                    "    bool b;\n"
                    "    float f;\n"
                    "    double d;\n"
                    "    qreal q;\n"
                    "    int i;\n"
                    "};\n"
                    "\n"
                    "S s;\n")
               % Check("s", "", "S")
               % Check("s.b", "", "bool")
               % Check("s.c", "", "bool")
               % Check("s.d", "", "double")
               % Check("s.f", "", "float")
               % Check("s.i", "", "int")
               % Check("s.q", "", "qreal")
               % Check("s.x", "", "uint")
               % Check("s.y", "", "uint");


    QTest::newRow("Function")
                  << Data("#include <QByteArray>\n",
    "struct Function\n"
    "{\n"
    "    Function(QByteArray var, QByteArray f, double min, double max)\n"
    "      : var(var), f(f), min(min), max(max) {}\n"
    "    QByteArray var;\n"
    "    QByteArray f;\n"
    "    double min;\n"
    "    double max;\n"
    "};\n"
        "// In order to use this, switch on the 'qDump__Function' in dumper.py\n"
        "Function func0, func(\"x\", \"sin(x)\", 0, 1);\n"
        "func.max = 10;\n"
        "func.f = \"cos(x)\";\n"
        "func.max = 4;\n"
        "func.max = 5;\n"
        "func.max = 6;\n"
        "func.max = 7;\n")
         % Check("func", "", "basic::Function")
         % Check("func.f", "sin(x)", "@QByteArray")
         % Check("func.max", "1", "double")
         % Check("func.min", "0", "double")
         % Check("func.var", "\"x\"", "@QByteArray")
         % Check("func", "", "basic::Function")
         % Check("func.f", "\"cos(x)\"", "@QByteArray")
         % Check("func.max", "7", "double");

//    QTest::newRow("AlphabeticSorting")
//                  << Data(
//        "// This checks whether alphabetic sorting of structure\n"
//        "// members work");\n"
//        "struct Color\n"
//        "{\n"
//        "    int r,g,b,a;\n"
//        "    Color() { r = 1, g = 2, b = 3, a = 4; }\n"
//        "};\n"
//        "    Color c;\n"
//         % Check("c", "basic::Color");
//         % Check("c.a", "4", "int");
//         % Check("c.b", "3", "int");
//         % Check("c.g", "2", "int");
//         % Check("c.r", "1", "int");

//        // Manual: Toogle "Sort Member Alphabetically" in context menu
//        // Manual: of "Locals and Expressions" view");
//        // Manual: Check that order of displayed members changes");

//    QTest::newRow("Typedef")
//                  << Data(
//    "namespace ns {\n"
//    "    typedef unsigned long long vl;\n"
//    "    typedef vl verylong;\n"
//    "}\n"

//    "typedef quint32 myType1;\n"
//    "typedef unsigned int myType2;\n"
//    "myType1 t1 = 0;\n"
//    "myType2 t2 = 0;\n"
//    "ns::vl j = 1000;\n"
//    "ns::verylong k = 1000;\n"
//         % Check("j", "1000", "basic::ns::vl");
//         % Check("k", "1000", "basic::ns::verylong");
//         % Check("t1", "0", "basic::myType1");
//         % Check("t2", "0", "basic::myType2");

//    QTest::newRow("Struct")
//                  << Data(
//    "Foo f(2);\n"
//    "f.doit();\n"
//    "f.doit();\n"
//    "f.doit();\n"
//         % Check("f", "Foo");
//         % Check("f.a", "5", "int");
//         % Check("f.b", "2", "int");

//    QTest::newRow("Union")
//                  << Data(
//    "union U\n"
//    "{\n"
//    "  int a;\n"
//    "  int b;\n"
//    "} u;\n"
//         % Check("u", "basic::U");
//         % Check("u.a", "int");
//         % Check("u.b", "int");

//    QTest::newRow("TypeFormats")
//                  << Data(
//    "// These tests should result in properly displayed umlauts in the\n"
//    "// Locals and Expressions view. It is only support on gdb with Python");\n"
//    "const char *s = "aöa";\n"
//    "const wchar_t *w = L"aöa";\n"
//    "QString u;\n"
//    "// All: Select UTF-8 in "Change Format for Type" in L&W context menu");\n"
//    "// Windows: Select UTF-16 in "Change Format for Type" in L&W context menu");\n"
//    "// Other: Select UCS-6 in "Change Format for Type" in L&W context menu");\n"
//    "if (sizeof(wchar_t) == 4)\n"
//    "    u = QString::fromUcs4((uint *)w);\n"
//    "else\n"
//    "    u = QString::fromUtf16((ushort *)w);\n"
//         % CheckType("s char *");
//         % Check("u "" QString");
//         % CheckType("w wchar_t *");



//    typedef void *VoidPtr;
//    typedef const void *CVoidPtr;

//    struct A
//    {
//        A() : test(7) {}
//        int test;
//        void doSomething(CVoidPtr cp) const;
//    };

//    void A::doSomething(CVoidPtr cp) const
//         % Check("cp", "basic::CVoidPtr");

//    QTest::newRow("Pointer")
//                  << Data(
//        "Foo *f = new Foo();\n"
//         % Check("f", "Foo");
//         % Check("f.a", "0", "int");
//         % Check("f.b", "5", "int");

//    QTest::newRow("PointerTypedef")
//                  << Data(
//        "A a;\n"
//        "VoidPtr p = &a;\n"
//        "CVoidPtr cp = &a;\n"
//         % Check("a", "basic::A");
//         % Check("cp", "basic::CVoidPtr");
//         % Check("p", "basic::VoidPtr");

//    QTest::newRow("StringWithNewline")
//                  << Data(
//        "QString hallo = "hallo\nwelt";\n"
//         % Check("hallo "hallo\nwelt" QString");
//         % Check("that string is properly displayed");

//    QTest::newRow("Reference1")
//                  << Data(
//        "int a = 43;\n"
//        "const int &b = a;\n"
//        "typedef int &Ref;\n"
//        "const int c = 44;\n"
//        "const Ref d = a;\n"
//         % Check("a", "43", "int");
//         % Check("b 43 int &");
//         % Check("c", "44", "int");
//         % Check("d", "43", "basic::Ref");

//    QTest::newRow("Reference2")
//                  << Data(
//        "QString a = "hello";\n"
//        "const QString &b = fooxx();\n"
//        "typedef QString &Ref;\n"
//        "const QString c = "world";\n"
//        "const Ref d = a;\n"
//         % Check("a "hello" QString");
//         % Check("b "bababa" QString &");
//         % Check("c "world" QString");
//         % Check("d "hello" basic::Ref");

    QTest::newRow("Reference3")
            << Data("#include <QString>\n",
                    "const QString &b = a;\n"
                    "typedef QString &Ref;\n"
                    "const Ref d = const_cast<Ref>(a);\n")
               % Check("a", "\"hello\"", "@QString &")
               % Check("b", "\"hello\"", "@QString &")
               % Check("d", "\"hello\"", "Ref");

    QTest::newRow("DynamicReference")
            << Data("DerivedClass d;\n"
                    "BaseClass *b1 = &d;\n"
                    "BaseClass &b2 = d;\n")
               % CheckType("b1", "DerivedClass *")
               % CheckType("b2", "DerivedClass &");

//    QTest::newRow("LongEvaluation1")
//                  << Data(
//        "QDateTime time = QDateTime::currentDateTime();\n"
//        "const int N = 10000;\n"
//        "QDateTime bigv[N];\n"
//        "for (int i = 0; i < 10000; ++i) {\n"
//        "    bigv[i] = time;\n"
//        "    time = time.addDays(1);\n"
//        "}\n"
//         % Check("N", "10000", "int");
//         % CheckType("bigv QDateTime [10000]");
//         % Check("bigv.0", "QDateTime");
//         % Check("bigv.9999", "QDateTime");
//         % Check("time", "QDateTime");

//    QTest::newRow("LongEvaluation2")
//                  << Data(
//        "const int N = 10000;\n"
//        "int bigv[N];\n"
//        "for (int i = 0; i < 10000; ++i)\n"
//        "    bigv[i] = i;\n"
//         % Check("N", "10000", "int");
//         % CheckType("bigv int [10000]");
//         % Check("bigv.0", "0", "int");
//         % Check("bigv.9999", "9999", "int");

//    QTest::newRow("Fork")
//                  << Data(
//        "QProcess proc;\n"
//        "proc.start("/bin/ls");\n"
//        "proc.waitForFinished();\n"
//        "QByteArray ba = proc.readAllStandardError();\n"
//        "ba.append('x');\n"
//         % Check("ba "x" QByteArray");
//         % Check("proc  QProcess");


//    QTest::newRow("FunctionPointer")
//                  << Data(
//        "int testFunctionPointerHelper(int x) { return x; }\n"
//        "typedef int (*func_t)(int);\n"
//        "func_t f = testFunctionPointerHelper;\n"
//        "int a = f(43);\n"
//         % Check("f", "basic::func_t");

//    QByteArray dataClass = "struct Class
//    "{\n"
//    "    Class() : a(34) {}\n"
//    "    int testFunctionPointerHelper(int x) { return x; }\n"
//    "    int a;\n"
//    "};\n"

//    QTest::newRow("MemberFunctionPointer")
//                  << Data(

//        "Class x;\n"
//        "typedef int (Class::*func_t)(int);\n"
//        "func_t f = &Class::testFunctionPointerHelper;\n"
//        "int a = (x.*f)(43);\n"
//         % Check("f", "basic::func_t");

//    QTest::newRow("MemberPointer")
//                  << Data(
//        Class x;\n"
//        typedef int (Class::*member_t);\n"
//        member_t m = &Class::a;\n"
//        int a = x.*m;\n"
//         % Check("m", "basic::member_t");\n"
//        // Continue");\n"

//         % Check("there's a valid display for m");

//    QTest::newRow("PassByReferenceHelper(Foo &f)
//         % CheckType("f Foo &");\n"
//         % Check("f.a", "12", "int");\n"
//        // Continue");\n"
//        ++f.a;\n"
//         % CheckType("f Foo &");\n"
//         % Check("f.a", "13", "int");\n"

//    QTest::newRow("PassByReference")
//                  << Data(
//        "Foo f(12);\n"
//        "testPassByReferenceHelper(f);\n"

//    QTest::newRow("BigInt")
//                  << Data(
//        "qint64 a = Q_INT64_C(0xF020304050607080);\n"
//        "quint64 b = Q_UINT64_C(0xF020304050607080);\n"
//        "quint64 c = std::numeric_limits<quint64>::max() - quint64(1);\n"
//         % Check("a", "-1143861252567568256", "qint64");
//         % Check("b", "-1143861252567568256", "quint64");
//         % Check("c", "-2", "quint64");

//    QTest::newRow("Hidden")
//                  << Data(
//    {
//        int  n = 1;\n"
//        n = 2;\n"
//        BREAK_HERE;\n"
//         % Check("n", "2", "int");\n"
//        // Continue");\n"
//        n = 3;\n"
//        BREAK_HERE;\n"
//         % Check("n", "3", "int");\n"
//        // Continue");\n"
//        {\n"
//            QString n = "2";\n"
//            BREAK_HERE;\n"
//             % Check("n "2" QString");\n"
//             % Check("n@1", "3", "int");\n"
//            // Continue");\n"
//            n = "3";\n"
//            BREAK_HERE;\n"
//             % Check("n "3" QString");\n"
//             % Check("n@1", "3", "int");\n"
//            // Continue");\n"
//            {\n"
//                double n = 3.5;\n"
//                BREAK_HERE;\n"
//                 % Check("n", "3.5", "double");\n"
//                 % Check("n@1 "3" QString");\n"
//                 % Check("n@2", "3", "int");\n"
//                // Continue");\n"
//                ++n;\n"
//                BREAK_HERE;\n"
//                 % Check("n", "4.5", "double");\n"
//                 % Check("n@1 "3" QString");\n"
//                 % Check("n@2", "3", "int");\n"
//                // Continue");\n"
//                dummyStatement(&n);\n"
//            }\n"
//            BREAK_HERE;\n"
//             % Check("n "3" QString");\n"
//             % Check("n@1", "3", "int");\n"
//            // Continue");\n"
//            n = "3";\n"
//            n = "4";
//            BREAK_HERE;\n"
//             % Check("n "4" QString");\n"
//             % Check("n@1", "3", "int");\n"
//            // Continue");\n"
//            dummyStatement(&n);\n"
//        }\n"
//        ++n;\n"
//        dummyStatement(&n);\n"
//    }\n"
//\n"
//    int testReturnInt")
//                  << Data(
//    {
//        return 1;
//    }

//    bool testReturnBool")
//                  << Data(
//    {
//        return true;
//    }

//    QString testReturnQString")
//                  << Data(
//    {
//        return "string";
//    }

//    QTest::newRow("Return")
//                  << Data(
//    {
//        bool b = testReturnBool();
//        BREAK_HERE;
//         % Check("b", "true", "bool");
//        // Continue");
//        int i = testReturnInt();
//        BREAK_HERE;
//         % Check("i", "1", "int");
//        // Continue");
//        QString s = testReturnQString();
//        BREAK_HERE;
//         % Check("s "string" QString");
//        // Continue");
//        dummyStatement(&i, &b, &s);
//    }

//    #ifdef Q_COMPILER_RVALUE_REFS\n"
//    struct X { X() : a(2), b(3) {} int a, b; };\n"
//\n"
//    X testRValueReferenceHelper1()\n"
//    {\n"
//        return X();\n"
//    }\n"
//\n"
//    X testRValueReferenceHelper2(X &&x)\n"
//    {\n"
//        return x;\n"
//    }\n"
//\n"
//    QTest::newRow("RValueReference()\n"
//    {\n"
//        X &&x1 = testRValueReferenceHelper1();\n"
//        X &&x2 = testRValueReferenceHelper2(std::move(x1));\n"
//        X &&x3 = testRValueReferenceHelper2(testRValueReferenceHelper1());\n"
//\n"
//        X y1 = testRValueReferenceHelper1();\n"
//        X y2 = testRValueReferenceHelper2(std::move(y1));\n"
//        X y3 = testRValueReferenceHelper2(testRValueReferenceHelper1());\n"
//\n"
//        BREAK_HERE;\n"
//        // Continue");\n"
//        dummyStatement(&x1, &x2, &x3, &y1, &y2, &y3);\n"
//    }\n"

//    QTest::newRow("SSE")
//                  << Data(
//    {
//    #ifdef __SSE__\n"
//        float a[4];\n"
//        float b[4];\n"
//        int i;\n"
//        for (i = 0; i < 4; i++) {\n"
//            a[i] = 2 * i;\n"
//            b[i] = 2 * i;\n"
//        }\n"
//        __m128 sseA, sseB;\n"
//        sseA = _mm_loadu_ps(a);\n"
//        sseB = _mm_loadu_ps(b);\n"
//         % Check("sseA", "__m128");
//         % Check("sseB", "__m128");
//        // Continue");
//        dummyStatement(&i, &sseA, &sseB);
//    #endif
//    }

//} // namespace sse


//namespace qscript {

//    QTest::newRow("QScript")
//                  << Data(
//"        QScriptEngine engine;n"
//"        QDateTime date = QDateTime::currentDateTime();n"
//"        QScriptValue s;n"
//"n"
//"         % Check("engine  QScriptEngine");n"
//"         % Check("s", "(invalid)", "QScriptValue");n"
//"         % Check("x1 <not accessible> QString");n"
//"        // Continue");n"
//"        s = QScriptValue(33);n"
//"        int x = s.toInt32();n"
//"n"
//"        s = QScriptValue(QString("34"));n"
//"        QString x1 = s.toString();n"
//"n"
//"        s = engine.newVariant(QVariant(43));n"
//"        QVariant v = s.toVariant();n"
//"n"
//"        s = engine.newVariant(QVariant(43.0));n"
//"        s = engine.newVariant(QVariant(QString("sss")));n"
//"        s = engine.newDate(date);n"
//"        date = s.toDateTime();n"
//"        s.setProperty("a", QScriptValue());n"
//"        QScriptValue d = s.data();n"
//         % Check("d", "(invalid)", "QScriptValue");
//         % Check("v 43 QVariant (int)");
//         % Check("x", "33", "int");
//         % Check("x1 "34" QString");


//    QTest::newRow("WTFString")
//                  << Data(
//        "QWebPage p;
//         % Check("p", "QWebPage");

//    QTest::newRow("BoostOptional1")
//                  << Data(
//        "boost::optional<int> i0, i;
//        "i = 1;
//         % Check("i0", "<uninitialized>", "boost::optional<int>");
//         % Check("i", "1", "boost::optional<int>");

//    QTest::newRow("BoostOptional2")
//                  << Data(
//        "boost::optional<QStringList> sl0, sl;\n"
//        "sl = (QStringList() << "xxx" << "yyy");\n"
//        "sl.get().append("zzz");\n"
//         % Check("sl", "<uninitialized>", "boost::optional<QStringList>");
//         % Check("sl <3 items> boost::optional<QStringList>");

//    QTest::newRow("BoostSharedPtr")
//                  << Data(
//        "boost::shared_ptr<int> s;\n"
//        "boost::shared_ptr<int> i(new int(43));\n"
//        "boost::shared_ptr<int> j = i;\n"
//        "boost::shared_ptr<QStringList> sl(new QStringList(QStringList() << "HUH!"));
//         % Check("s", "(null)", "boost::shared_ptr<int>");
//         % Check("i", "43", "boost::shared_ptr<int>");
//         % Check("j", "43", "boost::shared_ptr<int>");
//         % Check("sl  boost::shared_ptr<QStringList>");
//         % Check("sl.data <1 items> QStringList

//    QTest::newRow("BoostGregorianDate")
//                  << Data(
//        "using namespace boost;\n"
//        "using namespace gregorian;\n"
//        "date d(2005, Nov, 29);\n"
//        "d += months(1);\n"
//        "d += months(1);\n"
//        "// snap-to-end-of-month behavior kicks in:\n"
//        "d += months(1);\n"
//        "// Also end of the month (expected in boost)\n"
//        "d += months(1);\n"
//        "// Not where we started (expected in boost)\n"
//        "d -= months(4);\n"
//         % Check("d Tue Nov 29 2005 boost::gregorian::date");
//         % Check("d Thu Dec 29 2005 boost::gregorian::date");
//         % Check("d Sun Jan 29 2006 boost::gregorian::date");
//         % Check("d Tue Feb 28 2006 boost::gregorian::date");
//         % Check("d Fri Mar 31 2006 boost::gregorian::date");
//         % Check("d Wed Nov 30 2005 boost::gregorian::date");

//    QTest::newRow("BoostPosixTimeTimeDuration")
//                  << Data(
//        "using namespace boost;\n"
//        "using namespace posix_time;\n"
//        "time_duration d1(1, 0, 0);\n"
//        "time_duration d2(0, 1, 0);\n"
//        "time_duration d3(0, 0, 1);\n"
//         % Check("d1", "01:00:00", "boost::posix_time::time_duration");
//         % Check("d2", "00:01:00", "boost::posix_time::time_duration");
//         % Check("d3", "00:00:01", "boost::posix_time::time_duration");

//    QTest::newRow("BoostBimap")
//                  << Data(
//        "typedef boost::bimap<int, int> B;\n"
//        "B b;\n"
//        "b.left.insert(B::left_value_type(1, 2));\n"
//        "B::left_const_iterator it = b.left.begin();\n"
//        "int l = it->first;\n"
//        "int r = it->second;\n"
//         % Check("b <0 items> boost::B");
//         % Check("b <1 items> boost::B");

//    QTest::newRow("BoostPosixTimePtime")
//                  << Data(
//        "using namespace boost;\n"
//        "using namespace gregorian;\n"
//        "using namespace posix_time;\n"
//        "ptime p1(date(2002, 1, 10), time_duration(1, 0, 0));\n"
//        "ptime p2(date(2002, 1, 10), time_duration(0, 0, 0));\n"
//        "ptime p3(date(1970, 1, 1), time_duration(0, 0, 0));\n"
//         % Check("p1 Thu Jan 10 01:00:00 2002 boost::posix_time::ptime");
//         % Check("p2 Thu Jan 10 00:00:00 2002 boost::posix_time::ptime");
//         % Check("p3 Thu Jan 1 00:00:00 1970 boost::posix_time::ptime");



//    // This tests qdump__KRBase in dumpers/qttypes.py which uses
//    // a static typeflag to dispatch to subclasses");

//    QTest::newRow("KR")
//          << Data(
//            "namespace kr {\n"
//\n"
//            "   // FIXME: put in namespace kr, adjust qdump__KRBase in dumpers/qttypes.py\n"
//            "   struct KRBase\n"
//            "   {\n"
//            "       enum Type { TYPE_A, TYPE_B } type;\n"
//            "       KRBase(Type _type) : type(_type) {}\n"
//            "   };\n"
//\n"
//            "   struct KRA : KRBase { int x; int y; KRA():KRBase(TYPE_A), x(1), y(32) {} };\n"
//            "   struct KRB : KRBase { KRB():KRBase(TYPE_B) {} };\n"

//            "/} // namespace kr\n"

//        "KRBase *ptr1 = new KRA;\n"
//        "KRBase *ptr2 = new KRB;\n"
//        "ptr2 = new KRB;\n"
//         % Check("ptr1", "KRBase");
//         % Check("ptr1.type KRBase::TYPE_A (0) KRBase::Type");
//         % Check("ptr2", "KRBase");
//         % Check("ptr2.type KRBase::TYPE_B (1) KRBase::Type");



//    QTest::newRow("Eigen")
//             << Data(
//        "using namespace Eigen;\n"

//        "Vector3d test = Vector3d::Zero();\n"

//        "Matrix3d myMatrix = Matrix3d::Constant(5);\n"
//        "MatrixXd myDynamicMatrix(30, 10);\n"

//        "myDynamicMatrix(0, 0) = 0;\n"
//        "myDynamicMatrix(1, 0) = 1;\n"
//        "myDynamicMatrix(2, 0) = 2;\n"

//        "Matrix<double, 12, 15, ColMajor> colMajorMatrix;\n"
//        "Matrix<double, 12, 15, RowMajor> rowMajorMatrix;\n"

//        "int k = 0;\n"
//        "for (int i = 0; i != 12; ++i) {\n"
//        "    for (int j = 0; j != 15; ++j) {\n"
//        "        colMajorMatrix(i, j) = k;\n"
//        "        rowMajorMatrix(i, j) = k;\n"
//        "        ++k;\n"
//        "    }\n"
//        "}\n"



//    QTest::newRow("https://bugreports.qt-project.org/browse/QTCREATORBUG-3611")
//        "typedef unsigned char byte;\n"
//        "byte f = '2';\n"
//        "int *x = (int*)&f;\n"
//         % Check("f 50 '2' bug3611::byte");



//    QTest::newRow("https://bugreports.qt-project.org/browse/QTCREATORBUG-4904")
//        << Data(

//    "struct CustomStruct {\n"
//    "    int id;\n"
//    "    double dvalue;\n"
//    "};",
//        "QMap<int, CustomStruct> map;\n"
//        "CustomStruct cs1;\n"
//        "cs1.id = 1;\n"
//        "cs1.dvalue = 3.14;\n"
//        "CustomStruct cs2 = cs1;\n"
//        "cs2.id = -1;\n"
//        "map.insert(cs1.id, cs1);\n"
//        "map.insert(cs2.id, cs2);\n"
//        "QMap<int, CustomStruct>::iterator it = map.begin();\n"
//         % Check("map <2 items> QMap<int, bug4904::CustomStruct>");
//         % Check("map.0   QMapNode<int, bug4904::CustomStruct>");
//         % Check("map.0.key", "-1", "int");
//         % Check("map.0.value", "bug4904::CustomStruct");
//         % Check("map.0.value.dvalue", "3.1400000000000001", "double");
//         % Check("map.0.value.id", "-1", "int");


//    QTest::newRow("// https://bugreports.qt-project.org/browse/QTCREATORBUG-5106")
//        << Data(
//    "class A5106\n"
//    "{\n"
//    "public:\n"
//    "        A5106(int a, int b) : m_a(a), m_b(b) {}\n"
//    "        virtual int test() { return 5; }\n"
//    "private:\n"
//    "        int m_a, m_b;\n"
//    "};\n"

//    "class B5106 : public A5106\n"
//    "{\n"
//    "public:\n"
//    "        B5106(int c, int a, int b) : A5106(a, b), m_c(c) {}\n"
//    "        virtual int test() { return 4; BREAK_HERE; }\n"
//    "private:\n"
//    "        int m_c;\n"
//    "};\n"
//    ,
//    "    B5106 b(1,2,3);\n"
//    "    b.test();\n"
//    "    b.A5106::test();\n"
//    }


//    // https://bugreports.qt-project.org/browse/QTCREATORBUG-5184

//    // Note: The report there shows type field "QUrl &" instead of QUrl");
//    // It's unclear how this can happen. It should never have been like
//    // that with a stock 7.2 and any version of Creator");

//    void helper(const QUrl &url)\n"
//    {\n"
//        QNetworkRequest request(url);\n"
//        QList<QByteArray> raw = request.rawHeaderList();\n"
//         % Check("raw <0 items> QList<QByteArray>");
//         % Check("request", "QNetworkRequest");
//         % Check("url "http://127.0.0.1/" QUrl &");
//    }

//    QTest::newRow("5184()
//    {
//        QUrl url(QString("http://127.0.0.1/"));\n"
//        helper(url);\n"
//    }


//namespace qc42170 {

//    // http://www.qtcentre.org/threads/42170-How-to-watch-data-of-actual-type-in-debugger

//    struct Object
//    {
//        Object(int id_) : id(id_) {}
//        virtual ~Object() {}
//        int id;
//    };

//    struct Point : Object
//    {
//        Point(double x_, double y_) : Object(1), x(x_), y(y_) {}
//        double x, y;
//    };

//    struct Circle : Point
//    {
//        Circle(double x_, double y_, double r_) : Point(x_, y_), r(r_) { id = 2; }
//        double r;
//    };

//    void helper(Object *obj)
//    {
//         % Check("obj", "qc42170::Circle");
//        // Continue");

//         % Check("that obj is shown as a 'Circle' object");
//        dummyStatement(obj);
//    }

//    QTest::newRow("42170")
//                      << Data(
//    {
//        Circle *circle = new Circle(1.5, -2.5, 3.0);
//        Object *obj = circle;
//        helper(circle);
//        helper(obj);
//    }

//} // namespace qc42170


//namespace bug5799 {

//    // https://bugreports.qt-project.org/browse/QTCREATORBUG-5799

//    QTest::newRow("5799()
//    "typedef struct { int m1; int m2; } S1;\n"
//    "struct S2 : S1 { };\n"
//    "typedef struct S3 { int m1; int m2; } S3;\n"
//    "struct S4 : S3 { };\n"

//        "S2 s2;\n"
//        "s2.m1 = 5;\n"
//        "S4 s4;\n"
//        "s4.m1 = 5;\n"
//        "S1 a1[10];\n"
//        "typedef S1 Array[10];\n"
//        "Array a2;\n"
//         % CheckType("a1 bug5799::S1 [10]");
//         % Check("a2", "bug5799::Array");
//         % Check("s2", "bug5799::S2");
//         % Check("s2.@1", "bug5799::S1");
//         % Check("s2.@1.m1", "5", "int");
//         % Check("s2.@1.m2", "int");
//         % Check("s4", "bug5799::S4");
//         % Check("s4.@1", "bug5799::S3");
//         % Check("s4.@1.m1", "5", "int");
//         % Check("s4.@1.m2", "int");


//    // http://www.qtcentre.org/threads/41700-How-to-watch-STL-containers-iterators-during-debugging

//    QTest::newRow("41700")
//                            << Data(
//    {
//        "using namespace std;\n"
//        "typedef map<string, list<string> > map_t;\n"
//        "map_t m;\n"
//        "m["one"].push_back("a");\n"
//        "m["one"].push_back("b");\n"
//        "m["one"].push_back("c");\n"
//        "m["two"].push_back("1");\n"
//        "m["two"].push_back("2");\n"
//        "m["two"].push_back("3");\n"
//        "map_t::const_iterator it = m.begin();
//         % Check("m <2 items> qc41700::map_t");
//         % Check("m.0   std::pair<std::string const, std::list<std::string>>");
//         % Check("m.0.first "one" std::string");
//         % Check("m.0.second <3 items> std::list<std::string>");
//         % Check("m.0.second.0 "a" std::string");
//         % Check("m.0.second.1 "b" std::string");
//         % Check("m.0.second.2 "c" std::string");
//         % Check("m.1   std::pair<std::string const, std::list<std::string>>");
//         % Check("m.1.first "two" std::string");
//         % Check("m.1.second <3 items> std::list<std::string>");
//         % Check("m.1.second.0 "1" std::string");
//         % Check("m.1.second.1 "2" std::string");
//         % Check("m.1.second.2 "3" std::string");


//    // http://codepaster.europe.nokia.com/?id=42895

//    QTest::newRow("42895()
//    "void g(int c, int d)\n"
//    "{\n"
//        "qDebug() << c << d;\n"
//        "stopper()"\n"
//\n"
//    "void f(int a, int b)\n"
//    "{\n"
//    "    g(a, b);\n"
//    "}\n"

//    "    f(3, 4);\n"

//         % Check("c", "3", "int");
//         % Check("d", "4", "int");
//         % Check("there are frames for g and f in the stack view");




//    // https://bugreports.qt-project.org/browse/QTCREATORBUG-6465

//    QTest::newRow("6465()\n"
//    {\n"
//        typedef char Foo[20];\n"
//        Foo foo = "foo";\n"
//        char bar[20] = "baz";

//namespace bug6857 {

//    class MyFile : public QFile
//    {
//    public:
//        MyFile(const QString &fileName)
//            : QFile(fileName) {}
//    };

//    QTest::newRow("6857")
//                               << Data(
//        MyFile file("/tmp/tt");
//        file.setObjectName("A file");
//         % Check("file  bug6857::MyFile");
//         % Check("file.@1 "/tmp/tt" QFile");
//         % Check("file.@1.@1.@1 "A file" QObject");


//namespace bug6858 {

//    class MyFile : public QFile\n"
//    {\n"
//    public:\n"
//        MyFile(const QString &fileName)\n"
//            : QFile(fileName) {}\n"
//    };\n"

//    QTest::newRow("6858")
//                                  << Data(
//        MyFile file("/tmp/tt");\n"
//        file.setObjectName("Another file");\n"
//        QFile *pfile = &file;\n"
//         % Check("pfile  bug6858::MyFile");
//         % Check("pfile.@1 "/tmp/tt" QFile");
//         % Check("pfile.@1.@1.@1 "Another file" QObject");

//namespace bug6863 {

//    class MyObject : public QObject\n"
//    {\n"
//        Q_OBJECT\n"
//    public:\n"
//        MyObject() {}\n"
//    };\n"
//\n"
//    void setProp(QObject *obj)\n"
//    {\n"
//        obj->setProperty("foo", "bar");
//         % Check("obj.[QObject].properties", "<2", "items>");
//        // Continue");
//        dummyStatement(&obj);
//    }

//    QTest::newRow("6863")
//                                     << Data(
//    {
//        QFile file("/tmp/tt");\n"
//        setProp(&file);\n"
//        MyObject obj;\n"
//        setProp(&obj);\n"
//    }\n"

//}



//    QTest::newRow("6933")
//                                        << Data(
//    "class Base\n"
//    "{\n"
//    "public:\n"
//    "    Base() : a(21) {}\n"
//    "    virtual ~Base() {}\n"
//    "    int a;\n"
//    "};\n"

//    "class Derived : public Base\n"
//    "{\n"
//    "public:\n"
//    "    Derived() : b(42) {}\n"
//    "    int b;\n"
//    "};\n"

//        "Derived d;\n"
//        "Base *b = &d;\n"
//         % Check("b.[bug6933::Base].[vptr]
//         % Check("b.b", "42", "int");


    QTest::newRow("valist")
            << Data("#include <stdarg.h>\n"
                    "void test(const char *format, ...)\n"
                    "{\n"
                    "    va_list arg;\n"
                    "    va_start(arg, format);\n"
                    "    int i = va_arg(arg, int);\n"
                    "    double f = va_arg(arg, double);\n"
                    "    dummyStatement(&i, &f);\n"
                    "    va_end(arg);\n"
                    "}\n",
                    "test(\"abc\", 1, 2.0);\n")
               % Check("i", "1", "int")
               % Check("f", "2", "double");


    QTest::newRow("gdb13393")
            << Data(
                   "struct Base {\n"
                   "    Base() : a(1) {}\n"
                   "    virtual ~Base() {}  // Enforce type to have RTTI\n"
                   "    int a;\n"
                   "};\n"
                   "struct Derived : public Base {\n"
                   "    Derived() : b(2) {}\n"
                   "    int b;\n"
                   "};\n"
                   "struct S\n"
                   "{\n"
                   "    Base *ptr;\n"
                   "    const Base *ptrConst;\n"
                   "    Base &ref;\n"
                   "    const Base &refConst;\n"
                   "        S(Derived &d)\n"
                   "            : ptr(&d), ptrConst(&d), ref(d), refConst(d)\n"
                   "        {}\n"
                   "    };\n"
                   ,
                   "Derived d;\n"
                   "S s(d);\n"
                   "Base *ptr = &d;\n"
                   "const Base *ptrConst = &d;\n"
                   "Base &ref = d;\n"
                   "const Base &refConst = d;\n"
                   "Base **ptrToPtr = &ptr;\n"
                   "#if USE_BOOST\n"
                   "boost::shared_ptr<Base> sharedPtr(new Derived());\n"
                   "#else\n"
                   "int sharedPtr = 1;\n"
                   "#endif\n"
                   "dummyStatement(&ptrConst, &ref, &refConst, &ptrToPtr, &sharedPtr);\n")
               % Check("d", "", "Derived")
               % Check("d.@1", "[Base]", "", "Base")
               % Check("d.b", "2", "int")
               % Check("ptr", "", "Derived")
               % Check("ptr.@1", "[Base]", "", "Base")
               % Check("ptr.@1.a", "1", "int")
               % Check("ptrConst", "", "Derived")
               % Check("ptrConst.@1", "[Base]", "", "Base")
               % Check("ptrConst.b", "2", "int")
               % Check("ptrToPtr", "", "Derived")
               //% CheckType("ptrToPtr.[vptr]", " ")
               % Check("ptrToPtr.@1.a", "1", "int")
               % Check("ref", "", "Derived &")
               //% CheckType("ref.[vptr]", "")
               % Check("ref.@1.a", "1", "int")
               % Check("refConst", "", "Derived &")
               //% CheckType("refConst.[vptr]", "")
               % Check("refConst.@1.a", "1", "int")
               % Check("s", "", "S")
               % Check("s.ptr", "", "Derived")
               % Check("s.ptrConst", "", "Derived")
               % Check("s.ref", "", "Derived &")
               % Check("s.refConst", "", "Derived &")
               % Check("sharedPtr", "1", "int");


    // http://sourceware.org/bugzilla/show_bug.cgi?id=10586. fsf/MI errors out
    // on -var-list-children on an anonymous union. mac/MI was fixed in 2006");
    // The proposed fix has been reported to crash gdb steered from eclipse");
    // http://sourceware.org/ml/gdb-patches/2011-12/msg00420.html
    QTest::newRow("gdb10586mi")
            << Data("struct Test {\n"
                    "    struct { int a; float b; };\n"
                    "    struct { int c; float d; };\n"
                    "} v = {{1, 2}, {3, 4}};\n"
                    "dummyStatement(&v);\n")
               % Check("v", "", "Test")
               % Check("v.a", "1", "int");

    QTest::newRow("gdb10586eclipse")
            << Data("struct { int x; struct { int a; }; struct { int b; }; } v = {1, {2}, {3}};\n"
                    "struct S { int x, y; } n = {10, 20};\n"
                    "dummyStatement(&v, &n);\n")
               % Check("v", "", "{...}")
               % Check("n", "", "S")
               % Check("v.a", "2", "int")
               % Check("v.b", "3", "int")
               % Check("v.x", "1", "int")
               % Check("n.x", "10", "int")
               % Check("n.y", "20", "int");
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    tst_Dumpers test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_dumpers.moc"
