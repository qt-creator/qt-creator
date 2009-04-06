
#include "gdbmi.h"

#include <QtCore/QObject>
#include <QtTest/QtTest>

using namespace Debugger;
using namespace Debugger::Internal;

static const char test1[] =
    "1^done,stack=[frame={level=\"0\",addr=\"0x00000000004061ca\","
    "func=\"main\",file=\"test1.cpp\","
    "fullname=\"/home/apoenitz/work/test1/test1.cpp\",line=\"209\"}]";

static const char test2[] =
    "2^done,stack=[frame={level=\"0\",addr=\"0x00002ac058675840\","
    "func=\"QApplication\",file=\"/home/apoenitz/dev/qt/src/gui/kernel/qapplication.cpp\","
    "fullname=\"/home/apoenitz/dev/qt/src/gui/kernel/qapplication.cpp\",line=\"592\"},"
    "frame={level=\"1\",addr=\"0x00000000004061e0\",func=\"main\",file=\"test1.cpp\","
    "fullname=\"/home/apoenitz/work/test1/test1.cpp\",line=\"209\"}]\n";

static const char test3[] =
    "3^done,stack=[frame={level=\"0\",addr=\"0x00000000004061ca\","
    "func=\"main\",file=\"test1.cpp\","
    "fullname=\"/home/apoenitz/work/test1/test1.cpp\",line=\"209\"}]\n";

static const char test4[] =
    "&\"source /home/apoenitz/dev/ide/main/bin/gdb/qt4macros\\n\"\n"
    "4^done\n";

static const char test5[] =
    "1*stopped,reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"1\","
    "frame={addr=\"0x0000000000405738\",func=\"main\","
    "args=[{name=\"argc\",value=\"1\"},{name=\"argv\",value=\"0x7fff1ac78f28\"}],"
    "file=\"test1.cpp\",fullname=\"/home/apoenitz/work/test1/test1.cpp\","
    "line=\"209\"}\n";

static const char test6[] =
    "{u = {u = 2048, v = 16788279, w = -689265400}, a = 1, b = -689265424, c = 11063, s = {static null = {<No data fields>}, static shared_null = {ref = {value = 2}, alloc = 0, size = 0, data = 0x6098da, clean = 0, simpletext = 0, righttoleft = 0, asciiCache = 0, capacity = 0, reserved = 0, array = {0}}, static shared_empty = {ref = {value = 1}, alloc = 0, size = 0, data = 0x2b37d84f8fba, clean = 0, simpletext = 0, righttoleft = 0, asciiCache = 0, capacity = 0, reserved = 0, array = {0}}, d = 0x6098c0, static codecForCStrings = 0x0}}";

static const char test8[] =
    "8^done,data={locals={{name=\"a\"},{name=\"w\"}}}\n";

static const char test9[] =
    "9^done,data={locals=[name=\"baz\",name=\"urgs\",name=\"purgs\"]}\n";

static const char test10[] =
    "16^done,name=\"urgs\",numchild=\"1\",type=\"Urgs\"\n";

static const char test11[] =
    "[{name=\"size\",value=\"1\",type=\"size_t\",readonly=\"true\"},"
     "{name=\"0\",value=\"one\",type=\"QByteArray\"}]";

static const char test12[] =
    "{iname=\"local.hallo\",value=\"\\\"\\\"\",type=\"QByteArray\",numchild=\"0\"}";

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

    void parse(const QByteArray &str)
    {
        QByteArray result;
        result += "\n ";
        int indent = 0;
        int from = 0;
        int to = str.size();
        if (str.size() && str[0] == '{' /*'}'*/) {
            ++from;
            --to;
        }
        for (int i = from; i < to; ++i) {
            if (str[i] == '{')
                result += "{\n" + QByteArray(2*++indent + 1, ' ');
            else if (str[i] == '}') {
                if (!result.isEmpty() && result[result.size() - 1] != '\n')
                    result += "\n";
                result += QByteArray(2*--indent + 1, ' ') + "}\n";
            }
            else if (str[i] == ',') {
                if (!result.isEmpty() && result[result.size() - 1] != '\n')
                    result += "\n";
                result += QByteArray(2*indent, ' ');
            }
            else
                result += str[i];
        }
        qDebug() << "result:\n" << result;
    }



private slots:
    void mi1()  { testMi(test1);  }
    void mi2()  { testMi(test2);  }
    void mi3()  { testMi(test3);  }
    void mi4()  { testMi(test4);  }
    void mi5()  { testMi(test5);  }
    void mi6()  { testMi(test6);  }
    void mi8()  { testMi(test8);  }
    void mi9()  { testMi(test9);  }
    void mi10() { testMi(test10); }
    void mi11() { testMi(test11); }
    void mi12() { testMi(test12); }
};

QTEST_MAIN(tst_Debugger)

#include "main.moc"

