#include <ctype.h>

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>
#include <QtCore/QMetaMethod>
#include <QtCore/QModelIndex>
#if QT_VERSION >= 0x040500
#include <QtCore/QSharedPointer>
#endif
#include <QtCore/QStringList>
#include <QtCore/QTextCodec>
#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QStandardItemModel>
#include <QtGui/QStringListModel>
#include <QtTest/QtTest>

//#include <QtTest/qtest_gui.h>

#include <QtCore/private/qobject_p.h>

#include "gdb/gdbmi.h"
#include "tcf/json.h"
#include "gdbmacros.h"
#include "gdbmacros_p.h"

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
    void dumpQAbstractItemAndModelIndex();
    void dumpQAbstractItemModel();
    void dumpQByteArray();
    void dumpQChar();
    void dumpQDateTime();
    void dumpQDir();
    void dumpQFile();
    void dumpQFileInfo();
    void dumpQHash();
    void dumpQHashNode();
    void dumpQImage();
    void dumpQImageData();
    void dumpQLinkedList();
    void dumpQList_int();
    void dumpQList_char();
    void dumpQList_QString();
    void dumpQList_QString3();
    void dumpQList_Int3();
    void dumpQLocale();
    void dumpQMap();
    void dumpQMapNode();
    void dumpQObject();
    void dumpQObjectChildList();
    void dumpQObjectMethodList();
    void dumpQObjectPropertyList();
    void dumpQObjectSignal();
    void dumpQObjectSignalList();
    void dumpQObjectSlot();
    void dumpQObjectSlotList();
    void dumpQPixmap();
    void dumpQSharedPointer();
    void dumpQString();
    void dumpQTextCodec();
    void dumpQVariant_invalid();
    void dumpQVariant_QString();
    void dumpQVariant_QStringList();
    void dumpStdVector();
    void dumpQWeakPointer();
    void initTestCase();

public slots:
    void runQtc();

public slots:
    void readStandardOutput();
    void readStandardError();

private:
    QProcess m_proc; // the Qt Creator process

    void dumpQAbstractItemHelper(QModelIndex &index);
    void dumpQAbstractItemModelHelper(QAbstractItemModel &m);
    void dumpQByteArrayHelper(QByteArray &ba);
    void dumpQCharHelper(QChar &c);
    void dumpQDateTimeHelper(QDateTime &d);
    void dumpQDirHelper(QDir &d);
    void dumpQFileHelper(const QString &name, bool exists);
    template <typename K, typename V> void dumpQHashNodeHelper(QHash<K, V> &hash);
    void dumpQImageHelper(QImage &img);
    void dumpQImageDataHelper(QImage &img);
    template <typename T> void dumpQLinkedListHelper(QLinkedList<T> &l);
    void dumpQLocaleHelper(QLocale &loc);
    template <typename K, typename V> void dumpQMapHelper(QMap<K, V> &m);
    template <typename K, typename V> void dumpQMapNodeHelper(QMap<K, V> &m);
    void dumpQModelIndexHelper(QModelIndex &index);
    void dumpQObjectChildListHelper(QObject &o);
    void dumpQObjectMethodListHelper(QObject &obj);
    void dumpQObjectPropertyListHelper(QObject &obj);
    void dumpQObjectSignalHelper(QObject &o, int sigNum);
    void dumpQObjectSignalListHelper(QObject &o);
    void dumpQObjectSlotHelper(QObject &o, int slot);
    void dumpQObjectSlotListHelper(QObject &o);
    void dumpQPixmapHelper(QPixmap &p);
#if QT_VERSION >= 0x040500
    template <typename T>
    void dumpQSharedPointerHelper(QSharedPointer<T> &ptr);
    template <typename T>
    void dumpQWeakPointerHelper(QWeakPointer<T> &ptr);
#endif
    void dumpQTextCodecHelper(QTextCodec *codec);
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
    // Ensure that no arbitrary padding is introduced by QVectorTypedData.
    const size_t qVectorDataSize = 16;
    QCOMPARE(sizeof(QVectorData), qVectorDataSize);
    QVectorTypedData<int> *v = 0;
    QCOMPARE(size_t(&v->array), qVectorDataSize);
}

static const QByteArray utfToBase64(const QString &string)
{
    return QByteArray(reinterpret_cast<const char *>(string.utf16()), 2 * string.size()).toBase64();
}

static const char *boolToVal(bool b)
{
    return b ? "'true'" : "'false'";
}

static const QByteArray ptrToBa(const void *p, bool symbolicNull = true)
{
    return QByteArray().append(p == 0 && symbolicNull ?
        "<null>" :
        QString("0x") + QString::number((quintptr) p, 16));
}

static const QByteArray generateQStringSpec(const QString& str)
{
    return QByteArray("value='").append(utfToBase64(str)).
        append("',type='"NS"QString',numchild='0',valueencoded='2'");
}

static const QByteArray generateQCharSpec(const QChar& ch)
{
    return QByteArray("value='").append(utfToBase64(QString(QLatin1String("'%1' (%2, 0x%3)")).
                                        arg(ch).arg(ch.unicode()).arg(ch.unicode(), 0, 16))).
            append("',valueencoded='2',type='"NS"QChar',numchild='0'");
}

static const QByteArray generateBoolSpec(bool b)
{
    return QByteArray("value=").append(boolToVal(b)).append(",type='bool',numchild='0'");
}

template <typename T>
static const QByteArray generateIntegerSpec(T n, QString type)
{
    return QByteArray("value='").append(QString::number(n)).append("',type='").
        append(type).append("',numchild='0'");
}

static const QByteArray generateLongSpec(long n)
{
    return generateIntegerSpec(n, "long");
}

static const QByteArray generateIntSpec(int n)
{
    return generateIntegerSpec(n, "int");
}

const QString createExp(const void *ptr, const QString &type, const QString &method)
{
    return QString("exp='((").append(NSX).append(type).append(NSY).append("*)").
        append(ptrToBa(ptr)).append(")->").append(method).append("()'");
}

// Helper functions.

#ifdef Q_CC_MSVC
#  define MAP_NODE_TYPE_END ">"
#else
#  define MAP_NODE_TYPE_END " >"
#endif

template <typename T> static const char *typeToString()
{
    return "<unknown type>";
}
template <typename T> const QByteArray valToString(const T &)
{
    return "<unknown value>";
}
template <> const QByteArray valToString(const int &n)
{
    return QByteArray().append(QString::number(n));
}
template <> const QByteArray valToString(const QString &s)
{
    return QByteArray(utfToBase64(s)).append("',valueencoded='2");
}
template <> const QByteArray valToString(int * const &p)
{
    return ptrToBa(p);
}
template <typename T> const QByteArray derefValToString(const T &v)
{
    return valToString(v);
}
template <> const QByteArray derefValToString(int * const &ptr)
{
    return valToString(*ptr);
}
const QString stripPtrType(const QString &ptrTypeStr)
{
    return ptrTypeStr.mid(0, ptrTypeStr.size() - 2);
}
template <> const char *typeToString<int>()
{
    return "int";
}
template <> const char *typeToString<QString>()
{
    return NS"QString";
}
template <> const char *typeToString<int *>()
{
    return "int *";
}
template <typename T> bool isSimpleType()
{
    return false;
}
template <> bool isSimpleType<int>()
{
    return true;
}
template <typename T> bool isPointer()
{
    return false;
}
template <> bool isPointer<int *>()
{
    return true;
}

template <typename T> static const char *typeToNumchild()
{
    return "1";
}
template <> const char *typeToNumchild<int>()
{
    return "0";
}
template <> const char *typeToNumchild<QString>()
{
    return "0";
}
template <typename K, typename V>
QByteArray getMapType()
{
    return QByteArray(typeToString<K>()) + "@" + QByteArray(typeToString<V>());
}
template <typename K, typename V>
void getMapNodeParams(size_t &nodeSize, size_t &valOffset)
{
#if QT_VERSION >= 0x040500
    typedef QMapNode<K, V> NodeType;
    NodeType *node = 0;
    nodeSize = sizeof(NodeType);
    valOffset = size_t(&node->value);
#else
    nodeSize = sizeof(K) + sizeof(V) + 2*sizeof(void *);
    valOffset = sizeof(K);
#endif
}


static int qobj_priv_conn_list_size(const QObjectPrivate::ConnectionList &l)
{
#if QT_VERSION >= 0x040600
    int count = 0;
    for (QObjectPrivate::Connection *c = l.first; c != 0; c = c->nextConnectionList)
        ++count;
    return count;
#else
    return l.size();
#endif
}

static QObjectPrivate::Connection &qobj_priv_conn_list_at(
        const QObjectPrivate::ConnectionList &l, int pos)
{
#if QT_VERSION >= 0x040600
    QObjectPrivate::Connection *c = l.first;
    for (int i = 0; i < pos; ++i)
        c = c->nextConnectionList;
    return *c;
#else
    return l.at(pos);
#endif
}


void tst_Debugger::dumpQAbstractItemHelper(QModelIndex &index)
{
    const QAbstractItemModel *model = index.model();
    const QString &rowStr = QString::number(index.row());
    const QString &colStr = QString::number(index.column());
    const QByteArray &internalPtrStrSymbolic = ptrToBa(index.internalPointer());
    const QByteArray &internalPtrStrValue = ptrToBa(index.internalPointer(), false);
    const QByteArray &modelPtrStr = ptrToBa(model);
    QByteArray indexSpecSymbolic = QByteArray().append(rowStr + "," + colStr + ",").
        append(internalPtrStrSymbolic + "," + modelPtrStr);
    QByteArray indexSpecValue = QByteArray().append(rowStr + "," + colStr + ",").
        append(internalPtrStrValue + "," + modelPtrStr);
    QByteArray expected = QByteArray("tiname='iname',addr='").append(ptrToBa(&index)).
        append("',type='"NS"QAbstractItem',addr='$").append(indexSpecSymbolic).
        append("',value='").append(valToString(model->data(index).toString())).
        append("',numchild='1',children=[");
    int rowCount = model->rowCount(index);
    int columnCount = model->columnCount(index);
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < columnCount; ++col) {
            const QModelIndex &childIndex = model->index(row, col, index);
            expected.append("{name='[").append(valToString(row)).append(",").
                append(QString::number(col)).append("]',numchild='1',addr='$").
                append(QString::number(childIndex.row())).append(",").
                append(QString::number(childIndex.column())).append(",").
                append(ptrToBa(childIndex.internalPointer())).append(",").
                append(modelPtrStr).append("',type='"NS"QAbstractItem',value='").
                append(valToString(model->data(childIndex).toString())).append("'}");
            if (col < columnCount - 1 || row < rowCount - 1)
                expected.append(",");
        }
    }
    expected.append("]");
    testDumper(expected, &index, NS"QAbstractItem", true, indexSpecValue);
}

void tst_Debugger::dumpQModelIndexHelper(QModelIndex &index)
{
    QByteArray expected = QByteArray("tiname='iname',addr='").
        append(ptrToBa(&index)).append("',type='"NS"QModelIndex',value='");
    if (index.isValid()) {
        const int row = index.row();
        const int col = index.column();
        const QString &rowStr = QString::number(row);
        const QString &colStr = QString::number(col);
        const QModelIndex &parent = index.parent();
        expected.append("(").append(rowStr).append(", ").append(colStr).
            append(")',numchild='5',children=[").append("{name='row',").
            append(generateIntSpec(row)).append("},{name='column',").
            append(generateIntSpec(col)).append("},{name='parent',value='");
        if (parent.isValid()) {
            expected.append("(").append(QString::number(parent.row())).
                append(", ").append(QString::number(parent.column())).append(")");
        } else {
            expected.append("<invalid>");
        }
        expected.append("',").append(createExp(&index, "QModelIndex", "parent")).
            append(",type='"NS"QModelIndex',numchild='1'},").
            append("{name='internalId',").
            append(generateQStringSpec(QString::number(index.internalId()))).
            append("},{name='model',value='").append(ptrToBa(index.model())).
            append("',type='"NS"QAbstractItemModel*',numchild='1'}]");
    } else {
        expected.append("<invalid>',numchild='0'");
    }
    testDumper(expected, &index, NS"QModelIndex", true);
}

void tst_Debugger::dumpQAbstractItemAndModelIndex()
{
    // Case 1: ModelIndex with no children.
    QStringListModel m(QStringList() << "item1" << "item2" << "item3");
    QModelIndex index = m.index(2, 0);
    dumpQAbstractItemHelper(index);
    dumpQModelIndexHelper(index);

    class PseudoTreeItemModel : public QAbstractItemModel
    {
    public:
        PseudoTreeItemModel() : QAbstractItemModel(), parent1(0),
            parent1Child(1), parent2(10), parent2Child1(11), parent2Child2(12)
        {}

        int columnCount(const QModelIndex &parent = QModelIndex()) const
        {
            Q_UNUSED(parent);
            return 1;
        }

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
        {
            return !index.isValid() || role != Qt::DisplayRole ?
                    QVariant() : *static_cast<int *>(index.internalPointer());
        }

        QModelIndex index(int row, int column,
                          const QModelIndex & parent = QModelIndex()) const
        {
            QModelIndex index;
            if (column == 0) {
                if (!parent.isValid()) {
                    if (row == 0)
                        index = createIndex(row, column, &parent1);
                    else if (row == 1)
                        index = createIndex(row, column, &parent2);
                } else if (parent.internalPointer() == &parent1 && row == 0) {
                    index = createIndex(row, column, &parent1Child);
                } else if (parent.internalPointer() == &parent2) {
                    index = createIndex(row, column,
                                row == 0 ? &parent2Child1 : &parent2Child2);
                }
            }
            return index;
        }

        QModelIndex parent(const QModelIndex & index) const
        {
            QModelIndex parent;
            if (index.isValid()) {
                if (index.internalPointer() == &parent1Child)
                    parent = createIndex(0, 0, &parent1);
                else if (index.internalPointer() == &parent2Child1 ||
                         index.internalPointer() == &parent2Child2)
                    parent = createIndex(1, 0, &parent2);
            }
            return parent;
        }

        int rowCount(const QModelIndex &parent = QModelIndex()) const
        {
            int rowCount;
            if (!parent.isValid() || parent.internalPointer() == &parent2)
                rowCount = 2;
            else if (parent.internalPointer() == &parent1)
                rowCount = 1;
            else
                rowCount = 0;
            return rowCount;
        }

    private:
        mutable int parent1;
        mutable int parent1Child;
        mutable int parent2;
        mutable int parent2Child1;
        mutable int parent2Child2;
    };

    PseudoTreeItemModel m2;

    // Case 2: ModelIndex with one child.
    QModelIndex index2 = m2.index(0, 0);
    dumpQAbstractItemHelper(index2);
    dumpQModelIndexHelper(index2);

    // Case 3: ModelIndex with two children.
    QModelIndex index3 = m2.index(1, 0);
    dumpQAbstractItemHelper(index3);
    dumpQModelIndexHelper(index3);

    // Case 4: ModelIndex with a parent.
    index = m2.index(0, 0, index3);
    dumpQModelIndexHelper(index);

    // Case 5: Empty ModelIndex
    QModelIndex index4;
    dumpQModelIndexHelper(index4);
}

void tst_Debugger::dumpQAbstractItemModelHelper(QAbstractItemModel &m)
{
    QByteArray expected("tiname='iname',addr='");
    QString address = ptrToBa(&m);
    expected.append(address).append("',type='"NS"QAbstractItemModel',value='(").
        append(QString::number(m.rowCount())).append(",").
        append(QString::number(m.columnCount())).append(")',numchild='1',").
        append("children=[{numchild='1',name='"NS"QObject',addr='").append(address).
        append("',value='").append(utfToBase64(m.objectName())).
        append("',valueencoded='2',type='"NS"QObject',displayedtype='").
        append(m.metaObject()->className()).append("'}");
    for (int row = 0; row < m.rowCount(); ++row) {
        for (int column = 0; column < m.columnCount(); ++column) {
            QModelIndex mi = m.index(row, column);
            expected.append(",{name='[").append(QString::number(row)).append(",").
                append(QString::number(column)).append("]',value='").
                append(utfToBase64(m.data(mi).toString())).
                append("',valueencoded='2',numchild='1',addr='$").
                append(QString::number(mi.row())).append(",").append(QString::number(mi.column())).
                append(",").append(ptrToBa(mi.internalPointer())).append(",").
                append(ptrToBa(mi.model())).append("',type='"NS"QAbstractItem'}");
        }
    }
    expected.append("]");
    testDumper(expected, &m, NS"QAbstractItemModel", true);
}

void tst_Debugger::dumpQAbstractItemModel()
{
    // Case 1: No rows, one column.
    QStringList strList;
    QStringListModel model(strList);
    dumpQAbstractItemModelHelper(model);

    // Case 2: One row, one column.
    strList << "String 1";
    model.setStringList(strList);
    dumpQAbstractItemModelHelper(model);

    // Case 3: Two rows, one column.
    strList << "String 2";
    model.setStringList(strList);
    dumpQAbstractItemModelHelper(model);

    // Case 4: No rows, two columns.
    QStandardItemModel model2(0, 2);
    dumpQAbstractItemModelHelper(model2);

    // Case 5: One row, two columns.
    QStandardItem item1("Item (0,0)");
    QStandardItem item2("(Item (0,1)");
    model2.appendRow(QList<QStandardItem *>() << &item1 << &item2);
    dumpQAbstractItemModelHelper(model2);

    // Case 6: Two rows, two columns
    QStandardItem item3("Item (1,0");
    QStandardItem item4("Item (1,1)");
    model2.appendRow(QList<QStandardItem *>() << &item3 << &item4);
    dumpQAbstractItemModelHelper(model);
}

void tst_Debugger::dumpQByteArrayHelper(QByteArray &ba)
{
    QString size = QString::number(ba.size());
    QByteArray value;
    if (ba.size() <= 100)
        value = ba;
    else
        value.append(ba.left(100).toBase64()).append(" <size: ").append(size).
            append(", cut...>");
    QByteArray expected("value='");
    expected.append(value.toBase64()).append("',valueencoded='1',type='"NS"QByteArray',numchild='").
        append(size).append("',childtype='char',childnumchild='0',children=[");
    for (int i = 0; i < ba.size(); ++i) {
        char c = ba.at(i);
        char printedVal = isprint(c) && c != '\'' && c != '"' ? c : '?';
        QString hexNumber = QString::number(c, 16);
        if (hexNumber.size() < 2)
            hexNumber.prepend("0");
        expected.append("{value='").append(hexNumber).append("  (").
            append(QString::number(c)).append(" '").append(printedVal).append("')'}");
        if (i != ba.size() - 1)
            expected.append(",");
    }
    expected.append("]");
    testDumper(expected, &ba, NS"QByteArray", true);
}

void tst_Debugger::dumpQByteArray()
{
    // Case 1: Empty object.
    QByteArray ba;
    dumpQByteArrayHelper(ba);

    // Case 2: One element.
    ba.append('a');
    dumpQByteArrayHelper(ba);

    // Case 3: Two elements.
    ba.append('b');
    dumpQByteArrayHelper(ba);

    // Case 4: > 100 elements.
    ba = QByteArray(101, 'a');

    // Case 5: Regular and special characters and the replacement character.
    ba = QByteArray("abc\a\n\r\e\'\"?");
    dumpQByteArrayHelper(ba);
}

void tst_Debugger::dumpQCharHelper(QChar &c)
{
    char ch = c.isPrint() && c.unicode() < 127 ? c.toAscii() : '?';
    testDumper(QByteArray("value=''") + QByteArray(1, ch) + QByteArray("', ucs=")
        + QByteArray(QString::number(c.unicode()).toAscii())
        + QByteArray("',numchild='0'"), &c, NS"QChar", false);
}

void tst_Debugger::dumpQChar()
{
    // Case 1: Printable ASCII character.
    QChar c('X');
    dumpQCharHelper(c);

    // Case 2: Printable non-ASCII character.
    c = QChar(0x600);
    dumpQCharHelper(c);

    // Case 3: Non-printable ASCII character.
    c = QChar::fromAscii('\a');
    dumpQCharHelper(c);

    // Case 4: Non-printable non-ASCII character.
    c = QChar(0x9f);
    dumpQCharHelper(c);

    // Case 5: Printable ASCII Character that looks like the replacement character.
    c = QChar::fromAscii('?');
    dumpQCharHelper(c);
}

void tst_Debugger::dumpQDateTimeHelper(QDateTime &d)
{
    QByteArray expected("value='");
    if (d.isNull())
        expected.append("(null)',");
    else
      expected.append(utfToBase64(d.toString())).append("',valueencoded='2',");
    expected.append("type='$T',numchild='3',children=[");
    expected.append("{name='isNull',").append(generateBoolSpec(d.isNull())).append("},");
    expected.append("{name='toTime_t',").append(generateLongSpec((d.toTime_t()))).append("},");
    expected.append("{name='toString',").append(generateQStringSpec(d.toString())).append("},");
    expected.append("{name='toString_(ISO)',").append(generateQStringSpec(d.toString(Qt::ISODate))).append("},");
    expected.append("{name='toString_(SystemLocale)',").
            append(generateQStringSpec(d.toString(Qt::SystemLocaleDate))).append("},");
    expected.append("{name='toString_(Locale)',").
            append(generateQStringSpec(d.toString(Qt::LocaleDate))).append("}]");
    testDumper(expected, &d, NS"QDateTime", true);

}

void tst_Debugger::dumpQDateTime()
{
    // Case 1: Null object.
    QDateTime d;
    dumpQDateTimeHelper(d);

    // Case 2: Non-null object.
    d = QDateTime::currentDateTime();
    dumpQDateTimeHelper(d);
}

void tst_Debugger::dumpQDirHelper(QDir &d)
{
    testDumper(QByteArray("value='") + utfToBase64(d.absolutePath())
               + QByteArray("',valueencoded='2',type='"NS"QDir',numchild='3',children=[{name='absolutePath',")
               + generateQStringSpec(d.absolutePath()) + QByteArray("},{name='canonicalPath',")
               + generateQStringSpec(d.canonicalPath()) + QByteArray("}]"), &d, NS"QDir", true);
}

void tst_Debugger::dumpQDir()
{
    // Case 1: Current working directory.
    QDir dir = QDir::current();
    dumpQDirHelper(dir);

    // Case 2: Root directory.
    dir = QDir::root();
    dumpQDirHelper(dir);
}

void tst_Debugger::dumpQFileHelper(const QString &name, bool exists)
{
    QFile file(name);
    QByteArray filenameAsBase64 = utfToBase64(name);
    testDumper(
            QByteArray("value='") + filenameAsBase64 +
            QByteArray("',valueencoded='2',type='$T',numchild='2',children=[{name='fileName',value='") +
            filenameAsBase64 +
            QByteArray("',type='"NS"QString',numchild='0',valueencoded='2'},{name='exists',value=") +
            QByteArray(boolToVal(exists)) + QByteArray(",type='bool',numchild='0'}]"),
            &file, NS"QFile", true);
}

void tst_Debugger::dumpQFile()
{
    // Case 1: Empty file name => Does not exist.
    dumpQFileHelper("", false);

    // Case 2: File that is known to exist.
    QTemporaryFile file;
    file.open();
    dumpQFileHelper(file.fileName(), true);

    // Case 3: File with a name that most likely does not exist.
    dumpQFileHelper("jfjfdskjdflsdfjfdls", false);
}

void tst_Debugger::dumpQFileInfo()
{
    QFileInfo fi(".");
    QByteArray expected("value='");
    expected.append(utfToBase64(fi.filePath())).
        append("',valueencoded='2',type='"NS"QFileInfo',numchild='3',").
        append("children=[").
        append("{name='absolutePath',").append(generateQStringSpec(fi.absolutePath())).append("},").
        append("{name='absoluteFilePath',").append(generateQStringSpec(fi.absoluteFilePath())).append("},").
        append("{name='canonicalPath',").append(generateQStringSpec(fi.canonicalPath())).append("},").
        append("{name='canonicalFilePath',").append(generateQStringSpec(fi.canonicalFilePath())).append("},").
        append("{name='completeBaseName',").append(generateQStringSpec(fi.completeBaseName())).append("},").
        append("{name='completeSuffix',").append(generateQStringSpec(fi.completeSuffix())).append("},").
        append("{name='baseName',").append(generateQStringSpec(fi.baseName())).append("},").
#ifdef Q_OS_MACX
        append("{name='isBundle',").append(generateBoolSpec(fi.isBundle()).append("},").
        append("{name='bundleName',").append(generateQStringSpec(fi.bundleName())).append("'},").
#endif
        append("{name='fileName',").append(generateQStringSpec(fi.fileName())).append("},").
        append("{name='filePath',").append(generateQStringSpec(fi.filePath())).append("},").
        append("{name='group',").append(generateQStringSpec(fi.group())).append("},").
        append("{name='owner',").append(generateQStringSpec(fi.owner())).append("},").
        append("{name='path',").append(generateQStringSpec(fi.path())).append("},").
        append("{name='groupid',").append(generateLongSpec(fi.groupId())).append("},").
        append("{name='ownerid',").append(generateLongSpec(fi.ownerId())).append("},").
        append("{name='permissions',").append(generateLongSpec(fi.permissions())).append("},").
        append("{name='caching',").append(generateBoolSpec(fi.caching())).append("},").
        append("{name='exists',").append(generateBoolSpec(fi.exists())).append("},").
        append("{name='isAbsolute',").append(generateBoolSpec(fi.isAbsolute())).append("},").
        append("{name='isDir',").append(generateBoolSpec(fi.isDir())).append("},").
        append("{name='isExecutable',").append(generateBoolSpec(fi.isExecutable())).append("},").
        append("{name='isFile',").append(generateBoolSpec(fi.isFile())).append("},").
        append("{name='isHidden',").append(generateBoolSpec(fi.isHidden())).append("},").
        append("{name='isReadable',").append(generateBoolSpec(fi.isReadable())).append("},").
        append("{name='isRelative',").append(generateBoolSpec(fi.isRelative())).append("},").
        append("{name='isRoot',").append(generateBoolSpec(fi.isRoot())).append("},").
        append("{name='isSymLink',").append(generateBoolSpec(fi.isSymLink())).append("},").
        append("{name='isWritable',").append(generateBoolSpec(fi.isWritable())).append("},").
        append("{name='created',value='").append(utfToBase64(fi.created().toString())).
        append("',valueencoded='2',").append(createExp(&fi, "QFileInfo", "created")).
        append(",type='"NS"QDateTime',numchild='1'},").append("{name='lastModified',value='").
        append(utfToBase64(fi.lastModified().toString())).append("',valueencoded='2',").
        append(createExp(&fi, "QFileInfo", "lastModified")).
        append(",type='"NS"QDateTime',numchild='1'},").
        append("{name='lastRead',value='").append(utfToBase64(fi.lastRead().toString())).
        append("',valueencoded='2',").append(createExp(&fi, "QFileInfo", "lastRead")).
        append(",type='"NS"QDateTime',numchild='1'}]");
    testDumper(expected, &fi, NS"QFileInfo", true);
}

void tst_Debugger::dumpQHash()
{
    QHash<QString, QList<int> > hash;
    hash.insert("Hallo", QList<int>());
    hash.insert("Welt", QList<int>() << 1);
    hash.insert("!", QList<int>() << 1 << 2);
    hash.insert("!", QList<int>() << 1 << 2);
}

template <typename K, typename V>
void tst_Debugger::dumpQHashNodeHelper(QHash<K, V> &hash)
{
    typename QHash<K, V>::iterator it = hash.begin();
    typedef QHashNode<K, V> HashNode;
    HashNode *dummy = 0;
    HashNode *node =
        reinterpret_cast<HashNode *>(reinterpret_cast<char *>(const_cast<K *>(&it.key())) -
            size_t(&dummy->key));
    const K &key = it.key();
    const V &val = it.value();
    QByteArray expected("value='");
    if (isSimpleType<V>())
        expected.append(valToString(val));
    expected.append("',numchild='2',children=[{name='key',type='").
        append(typeToString<K>()).append("',addr='").append(ptrToBa(&key)).
        append("'},{name='value',type='").append(typeToString<V>()).
        append("',addr='").append(ptrToBa(&val)).append("'}]");
    testDumper(expected, node, NS"QHashNode", true,
        getMapType<K, V>(), "", sizeof(it.key()), sizeof(it.value()));
}

void tst_Debugger::dumpQHashNode()
{
    // Case 1: simple type -> simple type.
    QHash<int, int> hash1;
    hash1[2] = 3;
    dumpQHashNodeHelper(hash1);

    // Case 2: simple type -> composite type.
    QHash<int, QString> hash2;
    hash2[5] = "String 7";
    dumpQHashNodeHelper(hash2);

    // Case 3: composite type -> simple type
    QHash<QString, int> hash3;
    hash3["String 11"] = 13;
    dumpQHashNodeHelper(hash3);

    // Case 4: composite type -> composite type
    QHash<QString, QString> hash4;
    hash4["String 17"] = "String 19";
    dumpQHashNodeHelper(hash4);
}

void tst_Debugger::dumpQImageHelper(QImage &img)
{
    QByteArray expected = QByteArray("value='(").
        append(QString::number(img.width())).append("x").
        append(QString::number(img.height())).append(")',type='"NS"QImage',").
        append("numchild='1',children=[{name='data',type='"NS"QImageData',").
        append("addr='").append(ptrToBa(&img)).append("'}]");
    testDumper(expected, &img, NS"QImage", true);
}

void tst_Debugger::dumpQImage()
{
    // Case 1: Null image.
    QImage img;
    dumpQImageHelper(img);

    // Case 2: Normal image.
    img = QImage(3, 700, QImage::Format_RGB555);
    dumpQImageHelper(img);

    // Case 3: Invalid image.
    img = QImage(100, 0, QImage::Format_Invalid);
    dumpQImageHelper(img);
}

void tst_Debugger::dumpQImageDataHelper(QImage &img)
{
    const QByteArray ba(QByteArray::fromRawData((const char*) img.bits(), img.numBytes()));
    QByteArray expected = QByteArray("tiname='$I',addr='$A',type='"NS"QImageData',").
        append("numchild='0',value='<hover here>',valuetooltipencoded='1',").
        append("valuetooltipsize='").append(QString::number(ba.size())).append("',").
        append("valuetooltip='").append(ba.toBase64()).append("'");
    testDumper(expected, &img, NS"QImageData", false);
}

void tst_Debugger::dumpQImageData()
{
    // Case 1: Null image.
    QImage img;
    dumpQImageDataHelper(img);

    // Case 2: Normal image.
    img = QImage(3, 700, QImage::Format_RGB555);
    dumpQImageDataHelper(img);

    // Case 3: Invalid image.
    img = QImage(100, 0, QImage::Format_Invalid);
    dumpQImageDataHelper(img);
}

template <typename T>
        void tst_Debugger::dumpQLinkedListHelper(QLinkedList<T> &l)
{
    const int size = qMin(l.size(), 1000);
    const QString &sizeStr = QString::number(size);
    const QByteArray elemTypeStr = typeToString<T>();
    QByteArray expected = QByteArray("value='<").append(sizeStr).
        append(" items>',valuedisabled='true',numchild='").append(sizeStr).
        append("',childtype='").append(elemTypeStr).append("',childnumchild='").
        append(typeToNumchild<T>()).append("',children=[");
    typename QLinkedList<T>::const_iterator iter = l.constBegin();
    for (int i = 0; i < size; ++i, ++iter) {
        expected.append("{");
        const T &curElem = *iter;
        if (isPointer<T>()) {
            const QString typeStr = stripPtrType(typeToString<T>());
            const QByteArray addrStr = valToString(curElem);
            if (curElem != 0) {
                expected.append("addr='").append(addrStr).append("',saddr='").
                        append(addrStr).append("',type='").append(typeStr).
                        append("',value='").
                        append(derefValToString(curElem)).append("'");
            } else {
                expected.append("addr='").append(ptrToBa(&curElem)).append("',type='").
                    append(typeStr).append("',value='<null>',numchild='0'");
            }
        } else {
            expected.append("addr='").append(ptrToBa(&curElem)).
                append("',value='").append(valToString(curElem)).append("'");
        }
        expected.append("}");
        if (i < size - 1)
            expected.append(",");
    }
    if (size < l.size())
        expected.append(",...");
    expected.append("]");
    testDumper(expected, &l, NS"QLinkedList", true, elemTypeStr);
}

void tst_Debugger::dumpQLinkedList()
{
    // Case 1: Simple element type.
    QLinkedList<int> l;

    // Case 1.1: Empty list.
    dumpQLinkedListHelper(l);

    // Case 1.2: One element.
    l.append(2);
    dumpQLinkedListHelper(l);

    // Case 1.3: Two elements
    l.append(3);
    dumpQLinkedListHelper(l);

    // Case 2: Composite element type.
    QLinkedList<QString> l2;

    // Case 2.1: Empty list.
    dumpQLinkedListHelper(l2);

    // Case 2.2: One element.
    l2.append("Teststring 1");
    dumpQLinkedListHelper(l2);

    // Case 2.3: Two elements.
    l2.append("Teststring 2");
    dumpQLinkedListHelper(l2);

    // Case 2.4: > 1000 elements.
    for (int i = 3; i <= 1002; ++i)
        l2.append("Test " + QString::number(i));

    // Case 3: Pointer type.
    QLinkedList<int *> l3;
    l3.append(new int(5));
    l3.append(new int(7));
    l3.append(0);
    dumpQLinkedListHelper(l3);
}

void tst_Debugger::dumpQList_int()
{
    QList<int> ilist;
    testDumper("value='<0 items>',valuedisabled='true',numchild='0',"
        "internal='1',children=[]",
        &ilist, NS"QList", true, "int");
    ilist.append(1);
    ilist.append(2);
    testDumper("value='<2 items>',valuedisabled='true',numchild='2',"
        "internal='1',childtype='int',childnumchild='0',children=["
        "{addr='" + str(&ilist.at(0)) + "',value='1'},"
        "{addr='" + str(&ilist.at(1)) + "',value='2'}]",
        &ilist, NS"QList", true, "int");
}

void tst_Debugger::dumpQList_char()
{
    QList<char> clist;
    testDumper("value='<0 items>',valuedisabled='true',numchild='0',"
        "internal='1',children=[]",
        &clist, NS"QList", true, "char");
    clist.append('a');
    clist.append('b');
    testDumper("value='<2 items>',valuedisabled='true',numchild='2',"
        "internal='1',childtype='char',childnumchild='0',children=["
        "{addr='" + str(&clist.at(0)) + "',value=''a', ascii=97'},"
        "{addr='" + str(&clist.at(1)) + "',value=''b', ascii=98'}]",
        &clist, NS"QList", true, "char");
}

void tst_Debugger::dumpQList_QString()
{
    QList<QString> slist;
    testDumper("value='<0 items>',valuedisabled='true',numchild='0',"
        "internal='1',children=[]",
        &slist, NS"QList", true, NS"QString");
    slist.append("a");
    slist.append("b");
    testDumper("value='<2 items>',valuedisabled='true',numchild='2',"
        "internal='1',childtype='"NS"QString',childnumchild='0',children=["
        "{addr='" + str(&slist.at(0)) + "',value='YQA=',valueencoded='2'},"
        "{addr='" + str(&slist.at(1)) + "',value='YgA=',valueencoded='2'}]",
        &slist, NS"QList", true, NS"QString");
}

void tst_Debugger::dumpQList_Int3()
{
    QList<Int3> i3list;
    testDumper("value='<0 items>',valuedisabled='true',numchild='0',"
        "internal='0',children=[]",
        &i3list, NS"QList", true, "Int3");
    i3list.append(Int3());
    i3list.append(Int3());
    testDumper("value='<2 items>',valuedisabled='true',numchild='2',"
        "internal='0',childtype='Int3',children=["
        "{addr='" + str(&i3list.at(0)) + "'},"
        "{addr='" + str(&i3list.at(1)) + "'}]",
        &i3list, NS"QList", true, "Int3");
}

void tst_Debugger::dumpQList_QString3()
{
    QList<QString3> s3list;
    testDumper("value='<0 items>',valuedisabled='true',numchild='0',"
        "internal='0',children=[]",
        &s3list, NS"QList", true, "QString3");
    s3list.append(QString3());
    s3list.append(QString3());
    testDumper("value='<2 items>',valuedisabled='true',numchild='2',"
        "internal='0',childtype='QString3',children=["
        "{addr='" + str(&s3list.at(0)) + "'},"
        "{addr='" + str(&s3list.at(1)) + "'}]",
        &s3list, NS"QList", true, "QString3");
}

void tst_Debugger::dumpQLocaleHelper(QLocale &loc)
{
    QByteArray expected = QByteArray("value='").append(valToString(loc.name())).
        append("',type='"NS"QLocale',numchild='8',children=[{name='country',").
        append(createExp(&loc, "QLocale", "country")).append("},{name='language',").
        append(createExp(&loc, "QLocale", "language")).append("},{name='measurementSystem',").
        append(createExp(&loc, "QLocale", "measurementSystem")).
        append("},{name='numberOptions',").append(createExp(&loc, "QLocale", "numberOptions")).
        append("},{name='timeFormat_(short)',").
        append(generateQStringSpec(loc.timeFormat(QLocale::ShortFormat))).
        append("},{name='timeFormat_(long)',").append(generateQStringSpec(loc.timeFormat())).
        append("},{name='decimalPoint',").append(generateQCharSpec(loc.decimalPoint())).
        append("},{name='exponential',").append(generateQCharSpec(loc.exponential())).
        append("},{name='percent',").append(generateQCharSpec(loc.percent())).
        append("},{name='zeroDigit',").append(generateQCharSpec(loc.zeroDigit())).
        append("},{name='groupSeparator',").append(generateQCharSpec(loc.groupSeparator())).
        append("},{name='negativeSign',").append(generateQCharSpec(loc.negativeSign())).
        append("}]");
    testDumper(expected, &loc, NS"QLocale", true);
}

void tst_Debugger::dumpQLocale()
{
    QLocale english(QLocale::English);
    dumpQLocaleHelper(english);

    QLocale german(QLocale::German);
    dumpQLocaleHelper(german);

    QLocale chinese(QLocale::Chinese);
    dumpQLocaleHelper(chinese);

    QLocale swahili(QLocale::Swahili);
    dumpQLocaleHelper(swahili);
}

template <typename K, typename V>
        void tst_Debugger::dumpQMapHelper(QMap<K, V> &map)
{
    QByteArray sizeStr(valToString(map.size()));
    size_t nodeSize;
    size_t valOff;
    getMapNodeParams<K, V>(nodeSize, valOff);
    int transKeyOffset = static_cast<int>(2*sizeof(void *)) - static_cast<int>(nodeSize);
    int transValOffset = transKeyOffset + valOff;
    bool simpleKey = isSimpleType<K>();
    bool simpleVal = isSimpleType<V>();
    QByteArray expected = QByteArray("value='<").append(sizeStr).append(" items>',numchild='").
        append(sizeStr).append("',extra='simplekey: ").append(QString::number(simpleKey)).
        append(" isSimpleValue: ").append(QString::number(simpleVal)).
        append(" keyOffset: ").append(QString::number(transKeyOffset)).append(" valueOffset: ").
        append(QString::number(transValOffset)).append(" mapnodesize: ").
        append(QString::number(nodeSize)).append("',children=[");
    typedef typename QMap<K, V>::iterator mapIter;
    for (mapIter it = map.begin(); it != map.end(); ++it) {
        if (it != map.begin())
            expected.append(",");
        const QByteArray keyString =
            QByteArray(valToString(it.key())).replace("valueencoded","keyencoded");
        expected.append("{key='").append(keyString).append("',value='").
            append(valToString(it.value())).append("',");
        if (simpleKey && simpleVal) {
            expected.append("type='").append(typeToString<V>()).
                append("',addr='").append(ptrToBa(&it.value())).append("'");
        } else {
            QString keyTypeStr = typeToString<K>();
            QString valTypeStr = typeToString<V>();
#if QT_VERSION >= 0x040500
            expected.append("addr='").
                append(ptrToBa(reinterpret_cast<char *>(&(*it)) + sizeof(V))).
                append("',type='"NS"QMapNode<").append(keyTypeStr).append(",").
                append(valTypeStr).append(MAP_NODE_TYPE_END).append("'");
#else
            expected.append("type='"NS"QMapData::Node<").append(keyTypeStr).
                append(",").append(valTypeStr).append(MAP_NODE_TYPE_END).
                append("',exp='*('"NS"QMapData::Node<").append(keyTypeStr).
                append(",").append(valTypeStr).append(MAP_NODE_TYPE_END).
                append(" >'*)").append(ptrToBa(&(*it))).append("'");
#endif
        }
        expected.append("}");
    }
    expected.append("]");
    mapIter it = map.begin();
    testDumper(expected, *reinterpret_cast<QMapData **>(&it), NS"QMap",
               true, getMapType<K,V>(), "", 0, 0, nodeSize, valOff);
}

void tst_Debugger::dumpQMap()
{
    // Case 1: Simple type -> simple type.
    QMap<int, int> map1;

    // Case 1.1: Empty map.
    dumpQMapHelper(map1);

    // Case 1.2: One element.
    map1[2] = 3;
    dumpQMapHelper(map1);

    // Case 1.3: Two elements.
    map1[3] = 5;
    dumpQMapHelper(map1);

    // Case 2: Simple type -> composite type.
    QMap<int, QString> map2;

    // Case 2.1: Empty Map.
    dumpQMapHelper(map2);

    // Case 2.2: One element.
    map2[5] = "String 7";
    dumpQMapHelper(map2);

    // Case 2.3: Two elements.
    map2[7] = "String 11";
    dumpQMapHelper(map2);

    // Case 3: Composite type -> simple type.
    QMap<QString, int> map3;

    // Case 3.1: Empty map.
    dumpQMapHelper(map3);

    // Case 3.2: One element.
    map3["String 13"] = 11;
    dumpQMapHelper(map3);

    // Case 3.3: Two elements.
    map3["String 17"] = 13;
    dumpQMapHelper(map3);

    // Case 4: Composite type -> composite type.
    QMap<QString, QString> map4;

    // Case 4.1: Empty map.
    dumpQMapHelper(map4);

    // Case 4.2: One element.
    map4["String 19"] = "String 23";
    dumpQMapHelper(map4);

    // Case 4.3: Two elements.
    map4["String 29"] = "String 31";
    dumpQMapHelper(map4);

    // Case 4.4: Different value, same key (multimap functionality).
    map4["String 29"] = "String 37";
    dumpQMapHelper(map4);
}

template <typename K, typename V>
        void tst_Debugger::dumpQMapNodeHelper(QMap<K, V> &m)
{
    typename QMap<K, V>::iterator it = m.begin();
    const K &key = it.key();
    const V &val = it.value();
    //const char * const keyType = typeToString(key);
    QByteArray expected = QByteArray("value='',numchild='2',"
        "children=[{name='key',addr='").append(ptrToBa(&key)).
        append("',type='").append(typeToString<K>()).append("',value='").
        append(valToString(key)).append("'},{name='value',addr='").
        append(ptrToBa(&val)).append("',type='").append(typeToString<V>()).
        append("',value='").append(valToString(val)).
        append("'}]");
    size_t nodeSize;
    size_t valOffset;
    getMapNodeParams<K, V>(nodeSize, valOffset);
    testDumper(expected, *reinterpret_cast<QMapData **>(&it), NS"QMapNode",
               true, getMapType<K,V>(), "", 0, 0, nodeSize, valOffset);
}

void tst_Debugger::dumpQMapNode()
{
    // Case 1: simple type -> simple type.
    QMap<int, int> map;
    map[2] = 3;
    dumpQMapNodeHelper(map);

    // Case 2: simple type -> composite type.
    QMap<int, QString> map2;
    map2[3] = "String 5";
    dumpQMapNodeHelper(map2);

    // Case 3: composite type -> simple type.
    QMap<QString, int> map3;
    map3["String 7"] = 11;
    dumpQMapNodeHelper(map3);

    // Case 4: composite type -> composite type.
    QMap<QString, QString> map4;
    map4["String 13"] = "String 17";
    dumpQMapNodeHelper(map4);
}

void tst_Debugger::dumpQObject()
{
    QObject parent;
    testDumper("value='',valueencoded='2',type='$T',displayedtype='QObject',"
        "numchild='4'",
        &parent, NS"QObject", false);
    testDumper("value='',valueencoded='2',type='$T',displayedtype='QObject',"
        "numchild='4',children=["
        "{name='properties',addr='$A',type='$TPropertyList',"
            "value='<1 items>',numchild='1'},"
        "{name='signals',addr='$A',type='$TSignalList',"
            "value='<2 items>',numchild='2'},"
        "{name='slots',addr='$A',type='$TSlotList',"
            "value='<2 items>',numchild='2'},"
        "{name='parent',value='0x0',type='$T *',numchild='0'},"
        "{name='className',value='QObject',type='',numchild='0'}]",
        &parent, NS"QObject", true);

#if 0
    testDumper("numchild='2',value='<2 items>',type='QObjectSlotList',"
            "children=[{name='2',value='deleteLater()',"
            "numchild='0',addr='$A',type='QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',"
            "numchild='0',addr='$A',type='QObjectSlot'}]",
        &parent, NS"QObjectSlotList", true);
#endif

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
        "{name='properties',addr='$A',type='$TPropertyList',"
            "value='<1 items>',numchild='1'},"
        "{name='signals',addr='$A',type='$TSignalList',"
            "value='<2 items>',numchild='2'},"
        "{name='slots',addr='$A',type='$TSlotList',"
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

void tst_Debugger::dumpQObjectChildListHelper(QObject &o)
{
    const QObjectList children = o.children();
    const int size = children.size();
    const QString sizeStr = QString::number(size);
    QByteArray expected = QByteArray("numchild='").append(sizeStr).append("',value='<").
        append(sizeStr).append(" items>',type='"NS"QObjectChildList',children=[");
    for (int i = 0; i < size; ++i) {
        const QObject *child = children.at(i);
        expected.append("{addr='").append(ptrToBa(child)).append("',value='").
            append(utfToBase64(child->objectName())).
            append("',valueencoded='2',type='"NS"QObject',displayedtype='").
            append(child->metaObject()->className()).append("',numchild='1'}");
        if (i < size - 1)
            expected.append(",");
    }
    expected.append("]");
    testDumper(expected, &o, NS"QObjectChildList", true);
}

void tst_Debugger::dumpQObjectChildList()
{
    // Case 1: Object with no children.
    QObject o;
    dumpQObjectChildListHelper(o);

    // Case 2: Object with one child.
    QObject o2(&o);
    dumpQObjectChildListHelper(o);

    // Case 3: Object with two children.
    QObject o3(&o);
    dumpQObjectChildListHelper(o);
}

void tst_Debugger::dumpQObjectMethodListHelper(QObject &obj)
{
    const QMetaObject *mo = obj.metaObject();
    const int methodCount = mo->methodCount();
    QByteArray expected = QByteArray("addr='<synthetic>',type='"NS"QObjectMethodList',").
        append("numchild='").append(QString::number(methodCount)).
        append("',childtype='QMetaMethod::Method',childnumchild='0',children=[");
    for (int i = 0; i != methodCount; ++i) {
        const QMetaMethod & method = mo->method(i);
        int mt = method.methodType();
        expected.append("{name='").append(QString::number(i)).append(" ").
            append(QString::number(mo->indexOfMethod(method.signature()))).
            append(" ").append(method.signature()).append("',value='").
            append(mt == QMetaMethod::Signal ? "<Signal>" : "<Slot>").
            append(" (").append(QString::number(mt)).append(")'}");
        if (i != methodCount - 1)
            expected.append(",");
    }
    expected.append("]");
    testDumper(expected, &obj, NS"QObjectMethodList", true);
}

void tst_Debugger::dumpQObjectMethodList()
{
    QStringListModel m;
    dumpQObjectMethodListHelper(m);
}

void tst_Debugger::dumpQObjectPropertyListHelper(QObject &obj)
{
    const QMetaObject *mo = obj.metaObject();
    const QString propertyCount = QString::number(mo->propertyCount());
    QByteArray expected = QByteArray("addr='<synthetic>',type='"NS"QObjectPropertyList',").
        append("numchild='").append(propertyCount).append("',value='<").
        append(propertyCount).append(" items>',children=[");
    for (int i = mo->propertyCount() - 1; i >= 0; --i) {
        const QMetaProperty & prop = mo->property(i);
        expected.append("{name='").append(prop.name()).append("',");
        switch (prop.type()) {
        case QVariant::String:
            expected.append("type='").append(prop.typeName()).append("',value='").
                append(utfToBase64(prop.read(&obj).toString())).
                append("',valueencoded='2',numchild='0'");
            break;
        case QVariant::Bool:
            expected.append("type='").append(prop.typeName()).append("',value=").
                append(boolToVal(prop.read(&obj).toBool())).append("numchild='0'");
            break;
        case QVariant::Int: {
            const int value = prop.read(&obj).toInt();
            const QString valueString = QString::number(value);
            if (prop.isEnumType() || prop.isFlagType()) {
                const QMetaEnum &me = prop.enumerator();
                QByteArray type = me.scope();
                if (!type.isEmpty())
                    type += "::";
                type += me.name();
                expected.append("type='").append(type.constData()).append("',value='");
                if (prop.isEnumType()) {
                    if (const char *enumValue = me.valueToKey(value))
                        expected.append(enumValue);
                    else
                        expected.append(valueString);
                } else {
                    const QByteArray &flagsValue = me.valueToKeys(value);
                    expected.append(flagsValue.isEmpty() ? valueString : flagsValue.constData());
                }
            } else {
                expected.append("value='").append(valueString);
            }
            expected.append("',numchild='0'");
            break;
        }
        default:
            expected.append("addr='").append(ptrToBa(&obj)).
                append("',type'"NS"QObjectPropertyList',numchild='1'");
            break;
        }
        expected.append("}");
        if (i > 0)
            expected.append(",");
    }
    expected.append("]");
    testDumper(expected, &obj, NS"QObjectPropertyList", true);
}

void tst_Debugger::dumpQObjectPropertyList()
{
    // Case 1: Model without a parent.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    dumpQObjectPropertyListHelper(m);

    // Case 2: Model with a parent.
    QStringListModel m2(&m);
    dumpQObjectPropertyListHelper(m2);
}

static const char *connectionType(uint type)
{
    Qt::ConnectionType connType = static_cast<Qt::ConnectionType>(type);
    const char *output;
    switch (connType) {
        case Qt::AutoConnection: output = "auto"; break;
        case Qt::DirectConnection: output = "direct"; break;
        case Qt::QueuedConnection: output = "queued"; break;
        case Qt::BlockingQueuedConnection: output = "blockingqueued"; break;
        case 3: output = "autocompat"; break;
#if QT_VERSION >= 0x040600
        case Qt::UniqueConnection: break; // Can't happen.
#endif
        };
    return output;
};

class Cheater : public QObject
{
public:
    static const QObjectPrivate *getPrivate(const QObject &o)
    {
        return dynamic_cast<const QObjectPrivate *>(static_cast<const Cheater&>(o).d_ptr);
    }
};

typedef QVector<QObjectPrivate::ConnectionList> ConnLists;

void tst_Debugger::dumpQObjectSignalHelper(QObject &o, int sigNum)
{
    QByteArray expected("addr='<synthetic>',numchild='1',type='"NS"QObjectSignal'");
#if QT_VERSION >= 0x040400
    expected.append(",children=[");
    const QObjectPrivate *p = Cheater::getPrivate(o);
    Q_ASSERT(p != 0);
    const ConnLists *connLists = reinterpret_cast<const ConnLists *>(p->connectionLists);
    QObjectPrivate::ConnectionList connList =
        connLists != 0 && connLists->size() > sigNum ?
        connLists->at(sigNum) : QObjectPrivate::ConnectionList();
    int i = 0;
    for (QObjectPrivate::Connection *conn = connList.first; conn != 0;
         ++i, conn = conn->nextConnectionList) {
        const QString iStr = QString::number(i);
        expected.append("{name='").append(iStr).append(" receiver',");
        if (conn->receiver == &o)
            expected.append("value='").append("<this>").
                append("',valueencoded='2',type='").append(o.metaObject()->className()).
                append("',numchild='0',addr='").append(ptrToBa(&o)).append("'");
        else if (conn->receiver == 0)
            expected.append("value='0x0',type='"NS"QObject *',numchild='0'");
        else
            expected.append("addr='").append(ptrToBa(conn->receiver)).append("',value='").
                append(utfToBase64(conn->receiver->objectName())).append("',valueencoded='2',").
                append("type='"NS"QObject',displayedtype='").
                append(conn->receiver->metaObject()->className()).append("',numchild='1'");
        expected.append("},{name='").append(iStr).append(" slot',type='',value='");
        if (conn->receiver != 0)
            expected.append(conn->receiver->metaObject()->method(conn->method).signature());
        else
            expected.append("<invalid receiver>");
        expected.append("',numchild='0'},{name='").append(iStr).append(" type',type='',value='<").
            append(connectionType(conn->connectionType)).append(" connection>',").
            append("numchild='0'}");
        if (conn != connList.last)
            expected.append(",");
    }
    expected.append("],numchild='").append(QString::number(i)).append("'");
#endif
    testDumper(expected, &o, NS"QObjectSignal", true, "", "", sigNum);
}

void tst_Debugger::dumpQObjectSignal()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");
    dumpQObjectSignalHelper(o, 0);

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    const QMetaObject *mo = m.metaObject();
    QList<int> signalIndices;
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QMetaMethod &mm = mo->method(i);
        if (mm.methodType() == QMetaMethod::Signal) {
            int signalIndex = mo->indexOfSignal(mm.signature());
            Q_ASSERT(signalIndex != -1);
            signalIndices.append(signalIndex);
        }
    }
    foreach(const int signalIndex, signalIndices)
        dumpQObjectSignalHelper(m, signalIndex);

    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(const QModelIndex &, int, int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(const QModelIndex &, int, int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
#if QT_VERSION >= 0x040600
    connect(&m, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            &m, SLOT(revert()), Qt::UniqueConnection);
#endif
    foreach(const int signalIndex, signalIndices)
        dumpQObjectSignalHelper(m, signalIndex);
}

void tst_Debugger::dumpQObjectSignalListHelper(QObject &o)
{
    const QMetaObject *mo = o.metaObject();
    QList<QMetaMethod> methods;
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QMetaMethod &method = mo->method(i);
        if (method.methodType() == QMetaMethod::Signal)
            methods.append(method);
        }
    QString sizeStr = QString::number(methods.size());
    QByteArray addrString = ptrToBa(&o);
    QByteArray expected = QByteArray("tiname='$I',addr='$A',type='QObjectSignalList',value='<").
        append(sizeStr).append(" items>',addr='").append(addrString).append("',numchild='").
        append(sizeStr).append("'");
#if QT_VERSION >= 0x040400
    expected.append(",children=[");
    const QObjectPrivate *p = Cheater::getPrivate(o);
    Q_ASSERT(p != 0);
    const ConnLists *connLists = reinterpret_cast<const ConnLists *>(p->connectionLists);
    for (int i = 0; i < methods.size(); ++i) {
        const char * const signature = methods.at(i).signature();
        int sigNum = mo->indexOfSignal(signature);
        QObjectPrivate::ConnectionList connList =
                connLists != 0 && connLists->size() > sigNum ?
                connLists->at(sigNum) : QObjectPrivate::ConnectionList();
        expected.append("{name='").append(QString::number(sigNum)).append("',value='").
            append(signature).append("',numchild='").
            append(QString::number(qobj_priv_conn_list_size(connList))).
            append("',addr='").append(addrString).append("',type='"NS"QObjectSignal'}");
        if (i < methods.size() - 1)
            expected.append(",");
    }
    expected.append("]");
#endif
    testDumper(expected, &o, NS"QObjectSignalList", true);
}

void tst_Debugger::dumpQObjectSignalList()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");
    dumpQObjectSignalListHelper(o);

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    dumpQObjectSignalListHelper(m);

    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(const QModelIndex &, int, int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(const QModelIndex &, int, int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
    dumpQObjectSignalListHelper(m);
}

void tst_Debugger::dumpQObjectSlotHelper(QObject &o, int slot)
{
    // TODO: This test passes, but it's because both the dumper and
    //       the test are broken (no slots are ever listed).
    QByteArray expected = QByteArray("addr='").append(ptrToBa(&o)).
        append("',numchild='1',type='"NS"QObjectSlot'");
#if QT_VERSION >= 0x040400
   expected.append(",children=[");
    const QObjectPrivate *p = Cheater::getPrivate(o);
    int numChild = 0;
#if QT_VERSION >= 0x040600
    int senderNum = 0;
    for (QObjectPrivate::Connection *senderConn = p->senders;
         senderConn != 0; senderConn = senderConn->next, ++senderNum) {
#else
    for (senderNum = 0; senderNum < p->senders.size(); ++senderNum) {
        QObjectPrivate::Connection *senderConn = &p->senders.at(senderNum);
#endif
        QString senderNumStr = QString::number(senderNum);
        QObject *sender = senderConn->sender;
        int signal = senderConn->method;
        const ConnLists *connLists =
            reinterpret_cast<const ConnLists *>(p->connectionLists);
        const QObjectPrivate::ConnectionList &connList =
                connLists != 0 && connLists->size() > signal ?
                connLists->at(signal) : QObjectPrivate::ConnectionList();
        const int listSize = qobj_priv_conn_list_size(connList);
        for (int i = 0; i < listSize; ++i) {
            const QObjectPrivate::Connection *conn =
                    &qobj_priv_conn_list_at(connList, i);
            if (conn->receiver == &o && conn->method == slot) {
                ++numChild;
                const QMetaMethod &method = sender->metaObject()->method(signal);
                expected.append("{name='").append(senderNumStr).append(" sender',");
                if (sender == &o) {
                    expected.append("value='").append("<this>").
                        append("',type='").append(o.metaObject()->className()).
                        append("',numchild='0',addr='").append(ptrToBa(&o)).append("'");
                } else if (sender != 0) {
                       expected.append("addr='").append(ptrToBa(sender)).
                           append(",value='").append(utfToBase64(sender->objectName())).
                           append("',valueencoded='2',type='"NS"QObject',displayedtype='").
                           append(sender->metaObject()->className()).
                           append("',numchild='1'");
                } else {
                    expected.append("value='0x0',type='"NS"QObject *',numchild='0'");
                }
                expected.append("},{name='").append(senderNumStr).
                    append(" signal',type='',value='").append(method.signature()).
                    append("',numchild='0'},{name='").append(senderNumStr).
                    append(" type',type='',value='<'").append(connectionType(conn->method)).
                    append(" connection>',numchild='0'}");
            }
        }
    }
    expected.append("],numchild='0'");
#endif
    testDumper(expected, &o, NS"QObjectSlot", true, "", "", slot);
}

void tst_Debugger::dumpQObjectSlot()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");
    dumpQObjectSlotHelper(o, o.metaObject()->indexOfSlot("deleteLater()"));

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    const QMetaObject *mo = m.metaObject();
    QList<int> slotIndices;
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QMetaMethod &mm = mo->method(i);
        if (mm.methodType() == QMetaMethod::Slot) {
            int slotIndex = mo->indexOfSlot(mm.signature());
            Q_ASSERT(slotIndex != -1);
            slotIndices.append(slotIndex);
        }
    }
    foreach(const int slotIndex, slotIndices)
        dumpQObjectSlotHelper(m, slotIndex);

    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(const QModelIndex &, int, int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&o, SIGNAL(destroyed(QObject *)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(const QModelIndex &, int, int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
#if QT_VERSION >= 0x040600
    connect(&m, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            &m, SLOT(revert()), Qt::UniqueConnection);
#endif
    foreach(const int slotIndex, slotIndices)
        dumpQObjectSlotHelper(m, slotIndex);
}

void tst_Debugger::dumpQObjectSlotListHelper(QObject &o)
{
    const QMetaObject *mo = o.metaObject();
    QList<QMetaMethod> slotList;;
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QMetaMethod &method = mo->method(i);
        if (method.methodType() == QMetaMethod::Slot)
            slotList.append(method);
    }
    const QString numSlotsStr = QString::number(slotList.size());
    QByteArray expected = QByteArray("numchild='").append(numSlotsStr).
        append("',value='<").append(numSlotsStr).
        append(" items>',type='"NS"QObjectSlotList',children=[");
#if QT_VERSION >= 0x040400
    const QObjectPrivate *p = Cheater::getPrivate(o);
    for (int i = 0; i < slotList.size(); ++i) {
        const QMetaMethod &method = slotList.at(i);
        int k = mo->indexOfSlot(method.signature());
        Q_ASSERT(k != -1);
        expected.append("{name='").append(QString::number(k)).
            append("',value='").append(method.signature());
        int numChild = 0;
#if QT_VERSION >= 0x040600
        int s = 0;
        for (QObjectPrivate::Connection *senderList = p->senders; senderList != 0;
             senderList = senderList->next, ++s) {
#else
            for (int s = 0; s != p->senders.size(); ++s) {
                const Connection *senderList = &p->senders.at(s);
#endif // QT_VERSION >= 0x040600
                const QObject *sender = senderList->sender;
                int signal = senderList->method;
                const ConnLists *connLists =
                    reinterpret_cast<const ConnLists *>(Cheater::getPrivate(*sender)->connectionLists);
                const QObjectPrivate::ConnectionList &connList =
                        connLists != 0 && connLists->size() > signal ?
                        connLists->at(signal) : QObjectPrivate::ConnectionList();
                const int listSize = qobj_priv_conn_list_size(connList);
                for (int c = 0; c != listSize; ++c) {
                    const QObjectPrivate::Connection *conn =
                            &qobj_priv_conn_list_at(connList, c);
                    if (conn->receiver == &o && conn->method == k)
                        ++numChild;
                }
            }
        expected.append("',numchild='").append(QString::number(numChild)).
            append("',addr='").append(ptrToBa(&o)).append("',type='"NS"QObjectSlot'}");
        if (i < slotList.size() - 1)
            expected.append(",");
    }
#endif // QT_VERSION >= 0x040400
    expected.append("]");
    testDumper(expected, &o, NS"QObjectSlotList", true);
}

void tst_Debugger::dumpQObjectSlotList()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");
    dumpQObjectSlotListHelper(o);

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    dumpQObjectSlotListHelper(m);

    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(const QModelIndex &, int, int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(const QModelIndex &, int, int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
    connect(&o, SIGNAL(destroyed(QObject *)), &m, SLOT(submit()));
    dumpQObjectSlotListHelper(m);
}

void tst_Debugger::dumpQPixmapHelper(QPixmap &p)
{
    QByteArray expected = QByteArray("value='(").append(QString::number(p.width())).
        append("x").append(QString::number(p.height())).
        append(")',type='"NS"QPixmap',numchild='0'");
    testDumper(expected, &p, NS"QPixmap", true);
}

void tst_Debugger::dumpQPixmap()
{
    // Case 1: Null Pixmap.
    QPixmap p;
    dumpQPixmapHelper(p);

#if 0 // Crashes.
    // Case 2: Uninitialized non-null pixmap.
    p = QPixmap(20, 100);
    dumpQPixmapHelper(p);

    // Case 3: Initialized non-null pixmap.
    const char * const pixmap[] = {
        "2 24 3 1", "       c None", ".      c #DBD3CB", "+      c #FCFBFA",
        "  ", "  ", "  ", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+",
        ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", "  ", "  ", "  "
    };
    p = QPixmap(pixmap);
    dumpQPixmapHelper(p);
#endif
}

#if QT_VERSION >= 0x040500
template<typename T>
void tst_Debugger::dumpQSharedPointerHelper(QSharedPointer<T> &ptr)
{
    struct Cheater : public QSharedPointer<T>
    {
        static const typename QSharedPointer<T>::Data *getData(const QSharedPointer<T> &p)
        {
            return static_cast<const Cheater &>(p).d;
        }
    };

    QByteArray expected("value='");
    QString val1 = ptr.isNull() ? "<null>" : valToString(*ptr.data());
    QString val2 = isSimpleType<T>() ? val1 : "";
    const int *weakAddr;
    const int *strongAddr;
    int weakValue;
    int strongValue;
    if (!ptr.isNull()) {
        weakAddr = reinterpret_cast<const int *>(&Cheater::getData(ptr)->weakref);
        strongAddr = reinterpret_cast<const int *>(&Cheater::getData(ptr)->strongref);
        weakValue = *weakAddr;
        strongValue = *strongAddr;
    } else {
        weakAddr = strongAddr = 0;
        weakValue = strongValue = 0;
    }
    expected.append(val2).append("',valuedisabled='true',numchild='1',children=[").
        append("{name='data',addr='").append(ptrToBa(ptr.data())).
        append("',type='").append(typeToString<T>()).append("',value='").append(val1).
        append("'},{name='weakref',value='").append(QString::number(weakValue)).
        append("',type='int',addr='").append(ptrToBa(weakAddr)).append("',numchild='0'},").
        append("{name='strongref',value='").append(QString::number(strongValue)).
        append("',type='int',addr='").append(ptrToBa(strongAddr)).append("',numchild='0'}]");
    testDumper(expected, &ptr, NS"QSharedPointer", true, typeToString<T>());
}
#endif

void tst_Debugger::dumpQSharedPointer()
{
#if QT_VERSION >= 0x040500
    // Case 1: Simple type.
    // Case 1.1: Null pointer.
    QSharedPointer<int> simplePtr;
    dumpQSharedPointerHelper(simplePtr);

    // Case 1.2: Non-null pointer,
    QSharedPointer<int> simplePtr2(new int(99));
    dumpQSharedPointerHelper(simplePtr2);

    // Case 1.3: Shared pointer.
    QSharedPointer<int> simplePtr3 = simplePtr2;
    dumpQSharedPointerHelper(simplePtr2);

    // Case 1.4: Weak pointer.
    QWeakPointer<int> simplePtr4(simplePtr2);
    dumpQSharedPointerHelper(simplePtr2);

    // Case 2: Composite type.
    // Case 1.1: Null pointer.
    QSharedPointer<QString> compositePtr;
    // TODO: This case is not handled in gdbmacros.cpp (segfault!)
    //dumpQSharedPointerHelper(compoistePtr);

    // Case 1.2: Non-null pointer,
    QSharedPointer<QString> compositePtr2(new QString("Test"));
    dumpQSharedPointerHelper(compositePtr2);

    // Case 1.3: Shared pointer.
    QSharedPointer<QString> compositePtr3 = compositePtr2;
    dumpQSharedPointerHelper(compositePtr2);

    // Case 1.4: Weak pointer.
    QWeakPointer<QString> compositePtr4(compositePtr2);
    dumpQSharedPointerHelper(compositePtr2);
#endif
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

void tst_Debugger::dumpQVariant_invalid()
{
    QVariant v;
    testDumper("value='(invalid)',type='$T',numchild='0'",
        &v, NS"QVariant", false);
}

void tst_Debugger::dumpQVariant_QString()
{
    QVariant v = "abc";
    testDumper("value='KFFTdHJpbmcpICJhYmMi',valueencoded='5',type='$T',"
        "numchild='0'",
        &v, NS"QVariant", true);
/*
    FIXME: the QString version should have a child:
    testDumper("value='KFFTdHJpbmcpICJhYmMi',valueencoded='5',type='$T',"
        "numchild='1',children=[{name='value',value='IgBhAGIAYwAiAA==',"
        "valueencoded='4',type='QString',numchild='0'}]",
        &v, NS"QVariant", true);
*/
}

void tst_Debugger::dumpQVariant_QStringList()
{
   QVariant v = QStringList() << "Hi";
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
        "childtype='" + inner + "',childnumchild='1',"
        "children=[{addr='" + str(deref(&vector[0])) + "',"
            "saddr='" + str(deref(&vector[0])) + "',type='" + innerp + "'}]",
        &vector, "std::vector", true, inner, "", sizeof(std::list<int> *));
    vector.push_back(0);
    list.push_back(45);
    testDumper("value='<2 items>',valuedisabled='true',numchild='2',"
        "childtype='" + inner + "',childnumchild='1',"
        "children=[{addr='" + str(deref(&vector[0])) + "',"
            "saddr='" + str(deref(&vector[0])) + "',type='" + innerp + "'},"
          "{addr='" + str(&vector[1]) + "',"
            "type='" + innerp + "',value='<null>',numchild='0'}]",
        &vector, "std::vector", true, inner, "", sizeof(std::list<int> *));
    vector.push_back(new std::list<int>(list));
    vector.push_back(0);
}

void tst_Debugger::dumpQTextCodecHelper(QTextCodec *codec)
{
    const QByteArray &name = codec->name().toBase64();
    QByteArray expected = QByteArray("value='").append(name).
        append("',valueencoded='1',type='"NS"QTextCodec',numchild='2',children=[").
        append("{name='name',value='").append(name).append("',type='"NS"QByteArray'").
        append(",numchild='0',valueencoded='1'},").append("{name='mibEnum',").
        append(generateIntSpec(codec->mibEnum())).append("}");
    expected.append("]");
    testDumper(expected, codec, NS"QTextCodec", true);
}

void tst_Debugger::dumpQTextCodec()
{
    const QList<QByteArray> &codecNames = QTextCodec::availableCodecs();
    foreach (const QByteArray &codecName, codecNames)
        dumpQTextCodecHelper(QTextCodec::codecForName(codecName));
}

#if QT_VERSION >= 0x040500
template <typename T1, typename T2>
        size_t offsetOf(const T1 *klass, const T2 *member)
{
    return static_cast<size_t>(reinterpret_cast<const char *>(member) -
                               reinterpret_cast<const char *>(klass));
}

template <typename T>
        void tst_Debugger::dumpQWeakPointerHelper(QWeakPointer<T> &ptr)
{
    typedef QtSharedPointer::ExternalRefCountData Data;
    const size_t dataOffset = 0;
    const Data *d = *reinterpret_cast<const Data **>(
            reinterpret_cast<const char **>(&ptr) + dataOffset);
    const int *weakRefPtr = reinterpret_cast<const int *>(&d->weakref);
    const int *strongRefPtr = reinterpret_cast<const int *>(&d->strongref);
    T *data = ptr.toStrongRef().data();
    const QString dataStr = valToString(*data);
    QByteArray expected("value='");
    if (isSimpleType<T>())
        expected.append(dataStr);
    expected.append("',valuedisabled='true',numchild='1',children=[{name='data',addr='").
        append(ptrToBa(data)).append("',type='").append(typeToString<T>()).
        append("',value='").append(dataStr).append("'},{name='weakref',value='").
        append(valToString(*weakRefPtr)).append("',type='int',addr='").
        append(ptrToBa(weakRefPtr)).append("',numchild='0'},{name='strongref',value='").
        append(valToString(*strongRefPtr)).append("',type='int',addr='").
        append(ptrToBa(strongRefPtr)).append("',numchild='0'}]");
    testDumper(expected, &ptr, NS"QWeakPointer", true, typeToString<T>());
}
#endif

void tst_Debugger::dumpQWeakPointer()
{
#if QT_VERSION >= 0x040500
    // Case 1: Simple type.

    // Case 1.1: Null pointer.
    QSharedPointer<int> spNull;
    QWeakPointer<int> wp = spNull.toWeakRef();
    dumpQWeakPointerHelper(wp);

    // Case 1.2: Weak pointer is unique.
    QSharedPointer<int> sp(new int(99));
    wp = sp.toWeakRef();
    dumpQWeakPointerHelper(wp);

    // Case 1.3: There are other weak pointers.
    QWeakPointer<int> wp2 = sp.toWeakRef();
    dumpQWeakPointerHelper(wp);

    // Case 1.4: There are other strong shared pointers as well.
    QSharedPointer<int> sp2(sp);
    dumpQWeakPointerHelper(wp);

    // Case 2: Composite type.
    QSharedPointer<QString> spS(new QString("Test"));
    QWeakPointer<QString> wpS = spS.toWeakRef();
    dumpQWeakPointerHelper(wpS);
#endif
}

//
// Creator
//

#define VERIFY_OFFSETOF(member)                           \
do {                                                      \
    QObjectPrivate *qob = 0;                              \
    ObjectPrivate *ob = 0;                                \
    QVERIFY(size_t(&qob->member) == size_t(&ob->member)); \
} while (0)


void tst_Debugger::initTestCase()
{
    QVERIFY(sizeof(QWeakPointer<int>) == 2*sizeof(void *));
    QVERIFY(sizeof(QSharedPointer<int>) == 2*sizeof(void *));
    QtSharedPointer::ExternalRefCountData d;
    d.weakref = d.strongref = 0; // That's what the destructor expects.
    QVERIFY(sizeof(int) == sizeof(d.weakref));
    QVERIFY(sizeof(int) == sizeof(d.strongref));
    QVERIFY(sizeof(QObjectPrivate) == sizeof(ObjectPrivate));
    VERIFY_OFFSETOF(threadData);
    VERIFY_OFFSETOF(extraData);
    VERIFY_OFFSETOF(objectName);
    VERIFY_OFFSETOF(connectionLists);
    VERIFY_OFFSETOF(senders);
    VERIFY_OFFSETOF(currentSender);
    VERIFY_OFFSETOF(eventFilters);
    VERIFY_OFFSETOF(currentChildBeingDeleted);
    VERIFY_OFFSETOF(connectedSignals);
    VERIFY_OFFSETOF(deleteWatch);
#if QT_VERSION < 0x040600
    VERIFY_OFFSETOF(pendingChildInsertedEvents);
#else
    VERIFY_OFFSETOF(declarativeData);
    VERIFY_OFFSETOF(objectGuards);
    VERIFY_OFFSETOF(sharedRefcount);
#endif
}

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

