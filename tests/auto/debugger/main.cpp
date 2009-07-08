
#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QFileInfo>
#include <QtTest/QtTest>

#include <QtCore/private/qobject_p.h>

//#include <QtTest/qtest_gui.h>

#include "gdb/gdbmi.h"
#include "tcf/json.h"
#include "gdbmacros.h"


#undef NS
#ifdef QT_NAMESPACE
#   define STRINGIFY0(s) #s
#   define STRINGIFY1(s) STRINGIFY0(s)
#   define NS STRINGIFY1(QT_NAMESPACE) "::"
#else
#   define NS ""
#endif

using namespace Debugger;
using namespace Debugger::Internal;

static const char gdbmi1[] =
    "[frame={level=\"0\",addr=\"0x00000000004061ca\","
    "func=\"main\",file=\"test1.cpp\","
    "fullname=\"/home/apoenitz/work/test1/test1.cpp\",line=\"209\"}]";

static const char gdbmi2[] =
    "[frame={level=\"0\",addr=\"0x00002ac058675840\","
    "func=\"QApplication\",file=\"/home/apoenitz/dev/qt/src/gui/kernel/qapplication.cpp\","
    "fullname=\"/home/apoenitz/dev/qt/src/gui/kernel/qapplication.cpp\",line=\"592\"},"
    "frame={level=\"1\",addr=\"0x00000000004061e0\",func=\"main\",file=\"test1.cpp\","
    "fullname=\"/home/apoenitz/work/test1/test1.cpp\",line=\"209\"}]";

static const char gdbmi3[] =
    "[stack={frame={level=\"0\",addr=\"0x00000000004061ca\","
    "func=\"main\",file=\"test1.cpp\","
    "fullname=\"/home/apoenitz/work/test1/test1.cpp\",line=\"209\"}}]";

static const char gdbmi4[] =
    "&\"source /home/apoenitz/dev/ide/main/bin/gdb/qt4macros\\n\""
    "4^done\n";

static const char gdbmi5[] =
    "[reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"1\","
    "frame={addr=\"0x0000000000405738\",func=\"main\","
    "args=[{name=\"argc\",value=\"1\"},{name=\"argv\",value=\"0x7fff1ac78f28\"}],"
    "file=\"test1.cpp\",fullname=\"/home/apoenitz/work/test1/test1.cpp\","
    "line=\"209\"}]";

static const char gdbmi8[] =
    "[data={locals={{name=\"a\"},{name=\"w\"}}}]";

static const char gdbmi9[] =
    "[data={locals=[name=\"baz\",name=\"urgs\",name=\"purgs\"]}]";

static const char gdbmi10[] =
    "[name=\"urgs\",numchild=\"1\",type=\"Urgs\"]";

static const char gdbmi11[] =
    "[{name=\"size\",value=\"1\",type=\"size_t\",readonly=\"true\"},"
     "{name=\"0\",value=\"one\",type=\"QByteArray\"}]";

static const char gdbmi12[] =
    "[{iname=\"local.hallo\",value=\"\\\"\\\\\\00382\\t\\377\",type=\"QByteArray\","
     "numchild=\"0\"}]";


static const char jsont1[] =
    "{\"Size\":100564,\"UID\":0,\"GID\":0,\"Permissions\":33261,"
     "\"ATime\":1242370878000,\"MTime\":1239154689000}";

struct Int3 {
    Int3() { i1 = 42; i2 = 43; i3 = 44; }
    int i1, i2, i3;
};

struct QString3 {
    QString3() { s1 = "a"; s2 = "b"; s3 = "c"; }
    QString s1, s2, s3;
};

class tst_Debugger : public QObject
{
    Q_OBJECT

public:
    tst_Debugger() {}

    void testMi(const char* input)
    {
        QCOMPARE('\n' + QString::fromLatin1(GdbMi(input).toString(false)),
            '\n' + QString(input));
    }

    void testJson(const char* input)
    {
        QCOMPARE('\n' + QString::fromLatin1(JsonValue(input).toString(false)),
            '\n' + QString(input));
    }

private slots:
    void mi1()  { testMi(gdbmi1);  }
    void mi2()  { testMi(gdbmi2);  }
    void mi3()  { testMi(gdbmi3);  }
    //void mi4()  { testMi(gdbmi4);  }
    void mi5()  { testMi(gdbmi5);  }
    void mi8()  { testMi(gdbmi8);  }
    void mi9()  { testMi(gdbmi9);  }
    void mi10() { testMi(gdbmi10); }
    void mi11() { testMi(gdbmi11); }
    //void mi12() { testMi(gdbmi12); }

    void json1() { testJson(jsont1); }

    void infoBreak();
    void niceType();
    void niceType_data();

    void dumperCompatibility();
    void dumpQHash();
    void dumpQList();
    void dumpQObject();
    void dumpQString();
    void dumpQVariant();
    void dumpStdVector();

public slots:
    void runQtc();

public slots:
    void readStandardOutput();
    void readStandardError();

private:
    QProcess m_proc; // the Qt Creator process
};

static QByteArray stripped(QByteArray ba)
{
    for (int i = ba.size(); --i >= 0; ) {
        if (ba.at(i) == '\n' || ba.at(i) == ' ')
            ba.chop(1);
        else
            break;
    }
    return ba;
}

void tst_Debugger::infoBreak()
{
    // This tests the regular expression used in GdbEngine::extractDataFromInfoBreak 
    // to discover breakpoints in constructors.

    // Copied from gdbengine.cpp:

    QRegExp re("MULTIPLE.*(0x[0-9a-f]+) in (.*)\\s+at (.*):([\\d]+)([^\\d]|$)");
    re.setMinimal(true);

    QCOMPARE(re.indexIn(
        "2       breakpoint     keep y   <MULTIPLE> 0x0040168e\n"
        "2.1                         y     0x0040168e "
            "in MainWindow::MainWindow(QWidget*) at mainwindow.cpp:7\n"
        "2.2                         y     0x00401792 "
            "in MainWindow::MainWindow(QWidget*) at mainwindow.cpp:7\n"), 33);
    QCOMPARE(re.cap(1), QString("0x0040168e"));
    QCOMPARE(re.cap(2).trimmed(), QString("MainWindow::MainWindow(QWidget*)"));
    QCOMPARE(re.cap(3), QString("mainwindow.cpp"));
    QCOMPARE(re.cap(4), QString("7"));


    QCOMPARE(re.indexIn(
        "Num     Type           Disp Enb Address            What"
        "4       breakpoint     keep y   <MULTIPLE>         0x00000000004066ad"
        "4.1                         y     0x00000000004066ad in CTorTester"
        " at /main/tests/manual/gdbdebugger/simple/app.cpp:124"), 88);

    QCOMPARE(re.cap(1), QString("0x00000000004066ad"));
    QCOMPARE(re.cap(2).trimmed(), QString("CTorTester"));
    QCOMPARE(re.cap(3), QString("/main/tests/manual/gdbdebugger/simple/app.cpp"));
    QCOMPARE(re.cap(4), QString("124"));
}

//
// type simplification
//

static QString chopConst(QString type)
{
   while (1) {
        if (type.startsWith("const"))
            type = type.mid(5);
        else if (type.startsWith(' '))
            type = type.mid(1);
        else if (type.endsWith("const"))
            type.chop(5);
        else if (type.endsWith(' '))
            type.chop(1);
        else
            break;
    }
    return type;
}

QString niceType(QString type)
{
    type.replace('*', '@');

    for (int i = 0; i < 10; ++i) {
        int start = type.indexOf("std::allocator<");
        if (start == -1)
            break; 
        // search for matching '>'
        int pos;
        int level = 0;
        for (pos = start + 12; pos < type.size(); ++pos) {
            int c = type.at(pos).unicode();
            if (c == '<') {
                ++level;
            } else if (c == '>') {
                --level;
                if (level == 0)
                    break;
            }
        }
        QString alloc = type.mid(start, pos + 1 - start).trimmed();
        QString inner = alloc.mid(15, alloc.size() - 16).trimmed();
        //qDebug() << "MATCH: " << pos << alloc << inner;

        if (inner == QLatin1String("char"))
            // std::string
            type.replace(QLatin1String("basic_string<char, std::char_traits<char>, "
                "std::allocator<char> >"), QLatin1String("string"));
        else if (inner == QLatin1String("wchar_t"))
            // std::wstring
            type.replace(QLatin1String("basic_string<wchar_t, std::char_traits<wchar_t>, "
                "std::allocator<wchar_t> >"), QLatin1String("wstring"));

        // std::vector, std::deque, std::list
        QRegExp re1(QString("(vector|list|deque)<%1, %2\\s*>").arg(inner, alloc));
        if (re1.indexIn(type) != -1)
            type.replace(re1.cap(0), QString("%1<%2>").arg(re1.cap(1), inner));


        // std::stack
        QRegExp re6(QString("stack<%1, std::deque<%2> >").arg(inner, inner));
        re6.setMinimal(true);
        if (re6.indexIn(type) != -1)
            type.replace(re6.cap(0), QString("stack<%1>").arg(inner));

        // std::set
        QRegExp re4(QString("set<%1, std::less<%2>, %3\\s*>").arg(inner, inner, alloc));
        re4.setMinimal(true);
        if (re4.indexIn(type) != -1)
            type.replace(re4.cap(0), QString("set<%1>").arg(inner));


        // std::map
        if (inner.startsWith("std::pair<")) {
            // search for outermost ','
            int pos;
            int level = 0;
            for (pos = 10; pos < inner.size(); ++pos) {
                int c = inner.at(pos).unicode();
                if (c == '<')
                    ++level;
                else if (c == '>')
                    --level;
                else if (c == ',' && level == 0)
                    break;
            }
            QString ckey = inner.mid(10, pos - 10);
            QString key = chopConst(ckey);
            QString value = inner.mid(pos + 2, inner.size() - 3 - pos);

            QRegExp re5(QString("map<%1, %2, std::less<%3>, %4\\s*>")
                .arg(key, value, key, alloc));
            re5.setMinimal(true);
            if (re5.indexIn(type) != -1)
                type.replace(re5.cap(0), QString("map<%1, %2>").arg(key, value));
            else {
                QRegExp re7(QString("map<const %1, %2, std::less<const %3>, %4\\s*>")
                    .arg(key, value, key, alloc));
                re7.setMinimal(true);
                if (re7.indexIn(type) != -1)
                    type.replace(re7.cap(0), QString("map<const %1, %2>").arg(key, value));
            }
        }
    }
    type.replace('@', '*');
    type.replace(QLatin1String(" >"), QString(QLatin1Char('>')));
    return type;
}

void tst_Debugger::niceType()
{
    // cf. watchutils.cpp
    QFETCH(QString, input);
    QFETCH(QString, simplified);
    QCOMPARE(::niceType(input), simplified);
}

void tst_Debugger::niceType_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("simplified");

    QTest::newRow("list")
        << "std::list<int, std::allocator<int> >"
        << "std::list<int>";

    QTest::newRow("combined")
        << "std::vector<std::list<int, std::allocator<int> >*, "
           "std::allocator<std::list<int, std::allocator<int> >*> >"
        << "std::vector<std::list<int>*>";

    QTest::newRow("stack")
        << "std::stack<int, std::deque<int, std::allocator<int> > >"
        << "std::stack<int>";
    
    QTest::newRow("map")
        << "std::map<myns::QString, Foo, std::less<myns::QString>, "
           "std::allocator<std::pair<const myns::QString, Foo> > >"
        << "std::map<myns::QString, Foo>";

    QTest::newRow("map2")
        << "std::map<const char*, Foo, std::less<const char*>, "
           "std::allocator<std::pair<const char* const, Foo> > >"
        << "std::map<const char*, Foo>";
}

//
// Dumpers
//

static void testDumper(QByteArray expected0, void *data, QByteArray outertype,
    bool dumpChildren, QByteArray innertype = "", QByteArray exp = "",
    int extraInt0 = 0, int extraInt1 = 0, int extraInt2 = 0, int extraInt3 = 0)
{ 
    sprintf(xDumpInBuffer, "%s%c%s%c%s%c%s%c%s%c", 
        outertype.data(), 0, "iname", 0, exp.data(), 0,
        innertype.data(), 0, "iname", 0);
    void *res = qDumpObjectData440(2, 42, data, dumpChildren,
        extraInt0, extraInt1, extraInt2, extraInt3);
    QString expected(expected0);
    char buf[100];
    sprintf(buf, "%p", data);
    if (!expected.startsWith('t') && !expected.startsWith('f'))
        expected = "tiname='$I',addr='$A'," + expected;
    expected.replace("$I", "iname");
    expected.replace("$T", QByteArray(outertype));
    expected.replace("$A", QByteArray(buf));
    expected.replace('\'', '"');
    QString actual____ = QString::fromLatin1(xDumpOutBuffer);
    actual____.replace('\'', '"');
    QCOMPARE(res, xDumpOutBuffer);
    if (actual____ != expected) {
        QStringList l1 = actual____.split(",");
        QStringList l2 = expected.split(",");
        for (int i = 0; i < l1.size() && i < l2.size(); ++i) {
            if (l1.at(i) == l2.at(i))
                qDebug() << "== " << l1.at(i);
            else
                qDebug() << "!= " << l1.at(i) << l2.at(i);
        }
        if (l1.size() != l2.size())
            qDebug() << "!= size: " << l1.size() << l2.size();
    }
    QCOMPARE(actual____, expected);
}

QByteArray str(const void *p)
{
    char buf[100];
    sprintf(buf, "%p", p);
    return buf;
}

static const void *deref(const void *p)
{
    return *reinterpret_cast<const char* const*>(p);
}

void tst_Debugger::dumperCompatibility()
{
}

void tst_Debugger::dumpQHash()
{
    QHash<QString, QList<int> > hash;
    hash.insert("Hallo", QList<int>());
    hash.insert("Welt", QList<int>() << 1);
    hash.insert("!", QList<int>() << 1 << 2);
    hash.insert("!", QList<int>() << 1 << 2);
}

void tst_Debugger::dumpQList()
{
    QList<int> ilist;
    testDumper("value='<0 items>',valuedisabled='true',numchild='0',"
        "internal='1',childtype='int',children=[]",
        &ilist, NS"QList", true, "int");
    ilist.append(1);
    ilist.append(2);
    testDumper("value='<2 items>',valuedisabled='true',numchild='2',"
        "internal='1',childtype='int',children=["
        "{name='0',addr='" + str(&ilist.at(0)) + "',value='1'},"
        "{name='1',addr='" + str(&ilist.at(1)) + "',value='2'}],"
        "childnumchild='0'",
        &ilist, NS"QList", true, "int");

    QList<char> clist;
    testDumper("value='<0 items>',valuedisabled='true',numchild='0',"
        "internal='1',childtype='char',children=[]",
        &clist, NS"QList", true, "char");
    clist.append('a');
    clist.append('b');
    testDumper("value='<2 items>',valuedisabled='true',numchild='2',"
        "internal='1',childtype='char',children=["
        "{name='0',addr='" + str(&clist.at(0)) + "',"
            "value=''a', ascii=97',numchild='0'},"
        "{name='1',addr='" + str(&clist.at(1)) + "',"
            "value=''b', ascii=98',numchild='0'}],"
        "childnumchild='0'",
        &clist, NS"QList", true, "char");

    QList<QString> slist;
    testDumper("value='<0 items>',valuedisabled='true',numchild='0',"
        "internal='1',childtype='QString',children=[]",
        &slist, NS"QList", true, "QString");
    slist.append("a");
    slist.append("b");
    testDumper("value='<2 items>',valuedisabled='true',numchild='2',"
        "internal='1',childtype='QString',children=["
        "{name='0',addr='" + str(&slist.at(0)) + "',"
            "value='YQA=',valueencoded='2'},"
        "{name='1',addr='" + str(&slist.at(1)) + "',"
            "value='YgA=',valueencoded='2'}],"
        "childnumchild='0'",
        &slist, NS"QList", true, "QString");

    QList<Int3> i3list;
    testDumper("value='<0 items>',valuedisabled='true',numchild='0',"
        "internal='0',childtype='Int3',children=[]",
        &i3list, NS"QList", true, "Int3");
    i3list.append(Int3());
    i3list.append(Int3());
    testDumper("value='<2 items>',valuedisabled='true',numchild='2',"
        "internal='0',childtype='Int3',children=["
        "{name='0',addr='" + str(&i3list.at(0)) + "'},"
        "{name='1',addr='" + str(&i3list.at(1)) + "'}]",
        &i3list, NS"QList", true, "Int3");

    QList<QString3> s3list;
    testDumper("value='<0 items>',valuedisabled='true',numchild='0',"
        "internal='0',childtype='QString3',children=[]",
        &s3list, NS"QList", true, "QString3");
    s3list.append(QString3());
    s3list.append(QString3());
    testDumper("value='<2 items>',valuedisabled='true',numchild='2',"
        "internal='0',childtype='QString3',children=["
        "{name='0',addr='" + str(&s3list.at(0)) + "'},"
        "{name='1',addr='" + str(&s3list.at(1)) + "'}]",
        &s3list, NS"QList", true, "QString3");
}

void tst_Debugger::dumpQObject()
{
    QObject parent;
    testDumper("value='',valueencoded='2',type='$T',displayedtype='QObject',"
        "numchild='4'",
        &parent, NS"QObject", false);
    testDumper("value='',valueencoded='2',type='$T',displayedtype='QObject',"
        "numchild='4',children=["
        "{name='properties',exp='*(class '$T'*)$A',type='$TPropertyList',"
            "value='<1 items>',numchild='1'},"
        "{name='signals',exp='*(class '$T'*)$A',type='$TSignalList',"
            "value='<2 items>',numchild='2'},"
        "{name='slots',exp='*(class '$T'*)$A',type='$TSlotList',"
            "value='<2 items>',numchild='2'},"
        "{name='parent',value='0x0',type='$T *',numchild='0'},"
        "{name='className',value='QObject',type='',numchild='0'}]",
        &parent, NS"QObject", true);
/*
    testDumper("numchild='2',children=[{name='2',value='deleteLater()',"
            "numchild='0',exp='*(class 'QObject'*)$A',type='QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',"
            "numchild='0',exp='*(class 'QObject'*)$A',type='QObjectSlot'}]",
        &parent, NS"QObjectSlotList", true);
*/

    parent.setObjectName("A Parent");
    testDumper("value='QQAgAFAAYQByAGUAbgB0AA==',valueencoded='2',type='$T',"
        "displayedtype='QObject',numchild='4'",
        &parent, NS"QObject", false);
    QObject child(&parent);
    testDumper("value='',valueencoded='2',type='$T',"
        "displayedtype='QObject',numchild='4'",
        &child, NS"QObject", false);
    child.setObjectName("A Child");
    QByteArray ba ="value='QQAgAEMAaABpAGwAZAA=',valueencoded='2',type='$T',"
        "displayedtype='QObject',numchild='4',children=["
        "{name='properties',exp='*(class '$T'*)$A',type='$TPropertyList',"
            "value='<1 items>',numchild='1'},"
        "{name='signals',exp='*(class '$T'*)$A',type='$TSignalList',"
            "value='<2 items>',numchild='2'},"
        "{name='slots',exp='*(class '$T'*)$A',type='$TSlotList',"
            "value='<2 items>',numchild='2'},"
        "{name='parent',addr='" + str(&parent) + "',"
            "value='QQAgAFAAYQByAGUAbgB0AA==',valueencoded='2',type='$T',"
            "displayedtype='QObject',numchild='1'},"
        "{name='className',value='QObject',type='',numchild='0'}]";
    testDumper(ba, &child, NS"QObject", true);
    QObject::connect(&child, SIGNAL(destroyed()), qApp, SLOT(quit()));
    testDumper(ba, &child, NS"QObject", true);
    QObject::disconnect(&child, SIGNAL(destroyed()), qApp, SLOT(quit()));
    testDumper(ba, &child, NS"QObject", true);
    child.setObjectName("A renamed Child");
    testDumper("value='QQAgAHIAZQBuAGEAbQBlAGQAIABDAGgAaQBsAGQA',valueencoded='2',"
        "type='$T',displayedtype='QObject',numchild='4'",
        &child, NS"QObject", false);
}

void tst_Debugger::dumpQString()
{ 
    QString s;
    testDumper("value='',valueencoded='2',type='$T',numchild='0'",
        &s, NS"QString", false);
    s = "abc";
    testDumper("value='YQBiAGMA',valueencoded='2',type='$T',numchild='0'",
        &s, NS"QString", false);
}

void tst_Debugger::dumpQVariant()
{ 
    QVariant v;
    testDumper("value='(invalid)',type='$T',numchild='0'",
        &v, NS"QVariant", false);
    v = "abc";
    testDumper("value='KFFTdHJpbmcpICJhYmMi',valueencoded='5',type='$T',"
        "numchild='1',children=[{name='value',value='IgBhAGIAYwAiAA==',"
        "valueencoded='4',type='QString',numchild='0'}]",
        &v, NS"QVariant", true);
    v = QStringList() << "Hi";
return; // FIXME
    testDumper("value='(QStringList) ',type='$T',"
        "numchild='1',children=[{name='value',"
        "exp='(*('myns::QStringList'*)3215364300)',"
        "type='QStringList',numchild='1'}]",
        &v, NS"QVariant", true);
}

void tst_Debugger::dumpStdVector()
{
    std::vector<std::list<int> *> vector;
    QByteArray inner = "std::list<int> *";
    QByteArray innerp = "std::list<int>";
    testDumper("value='<0 items>',valuedisabled='true',numchild='0'",
        &vector, "std::vector", false, inner, "", sizeof(std::list<int> *));
    std::list<int> list;
    vector.push_back(new std::list<int>(list));
    testDumper("value='<1 items>',valuedisabled='true',numchild='1',"
        "children=[{name='0',addr='" + str(deref(&vector[0])) + "',"
            "saddr='" + str(deref(&vector[0])) + "',type='" + innerp + "'}]",
        &vector, "std::vector", true, inner, "", sizeof(std::list<int> *));
    vector.push_back(0);
    list.push_back(45);
    testDumper("value='<2 items>',valuedisabled='true',numchild='2',"
        "children=[{name='0',addr='" + str(deref(&vector[0])) + "',"
            "saddr='" + str(deref(&vector[0])) + "',type='" + innerp + "'},"
          "{name='1',addr='" + str(&vector[1]) + "',"
            "type='" + innerp + "',value='<null>',numchild='0'}]",
        &vector, "std::vector", true, inner, "", sizeof(std::list<int> *));
    vector.push_back(new std::list<int>(list));
    vector.push_back(0);
}

//
// Creator
//

void tst_Debugger::readStandardOutput()
{
    qDebug() << "qtcreator-out: " << stripped(m_proc.readAllStandardOutput());
}

void tst_Debugger::readStandardError()
{
    qDebug() << "qtcreator-err: " << stripped(m_proc.readAllStandardError());
}

void tst_Debugger::runQtc()
{
    QString test = QFileInfo(qApp->arguments().at(0)).absoluteFilePath();
    QString qtc = QFileInfo(test).absolutePath() + "/../../../bin/qtcreator.bin";
    qtc = QFileInfo(qtc).absoluteFilePath();
    QStringList env = QProcess::systemEnvironment();
    env.append("QTC_DEBUGGER_TEST=" + test);
    m_proc.setEnvironment(env);
    connect(&m_proc, SIGNAL(readyReadStandardOutput()),
        this, SLOT(readStandardOutput()));
    connect(&m_proc, SIGNAL(readyReadStandardError()),
        this, SLOT(readStandardError()));
    m_proc.start(qtc);
    m_proc.waitForStarted();
    QCOMPARE(m_proc.state(), QProcess::Running);
    m_proc.waitForFinished();
    QCOMPARE(m_proc.state(), QProcess::NotRunning);
}

void runDebuggee()
{
    qDebug() << "RUNNING DEBUGGEE";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();

    if (args.size() == 2 && args.at(1) == "--run-debuggee") {
        runDebuggee();
        app.exec();
        return 0;
    }

    tst_Debugger test;
    return QTest::qExec(&test, argc, argv);
}

#include "main.moc"

