/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#define private public // Give us access to private 'backward' member of QMapNode.
#    include <QMap>
#undef private

#include "gdb/gdbmi.h"
#include "dumper.h"
#include "dumper_p.h"

#include "json.h"

#ifdef USE_PRIVATE
#include <private/qobject_p.h>
#else
#warning "No private headers for this Qt version available"
#endif

#include <QStandardItemModel>
#include <QStringListModel>

#include <QtTest>
//#include <QtTest/qtest_gui.h>

//TESTED_COMPONENT=src/plugins/debugger/gdb

static const char *pointerPrintFormat()
{
    return sizeof(quintptr) == sizeof(long) ? "0x%lx" : "0x%llx";
}

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

static QByteArray operator<<(QByteArray ba, const QByteArray &replacement)
{
    int pos = ba.indexOf('%');
    Q_ASSERT(pos != -1);
    return ba.replace(pos, 1, replacement);
}

static QByteArray &operator<<=(QByteArray &ba, const QByteArray &replacement)
{
    int pos = ba.indexOf('%');
    Q_ASSERT(pos != -1);
    return ba.replace(pos, 1, replacement);
}


template <typename T>
inline QByteArray N(T t) { return QByteArray::number(t); }

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

class tst_Dumpers : public QObject
{
    Q_OBJECT

public:
    tst_Dumpers() {}

    void testMi(const char* input)
    {
        GdbMi gdbmi;
        gdbmi.fromString(QByteArray(input));
        QCOMPARE('\n' + QString::fromLatin1(gdbmi.toString(false)),
            '\n' + QString(input));
    }

    void testJson(const char* input)
    {
        QCOMPARE('\n' + Utils::JsonStringValue(QLatin1String(input)).value(),
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
    void dumpQList_int_star();
    void dumpQList_char();
    void dumpQList_QString();
    void dumpQList_QString3();
    void dumpQList_Int3();
    void dumpQLocale();
    void dumpQMap();
    void dumpQMapNode();
#ifdef USE_PRIVATE
    void dumpQObject();
    void dumpQObjectChildList();
    void dumpQObjectMethodList();
    void dumpQObjectPropertyList();
    void dumpQObjectSignal();
    void dumpQObjectSignalList();
    void dumpQObjectSlot();
    void dumpQObjectSlotList();
#endif
    void dumpQPixmap();
    void dumpQSharedPointer();
    void dumpQString();
    void dumpQTextCodec();
    void dumpQVariant_invalid();
    void dumpQVariant_QString();
    void dumpQVariant_QStringList();
#ifndef Q_CC_MSVC
    void dumpStdVector();
#endif // !Q_CC_MSVC
    void dumpQWeakPointer();
    void initTestCase();

private:
    void dumpQAbstractItemHelper(QModelIndex &index);
    void dumpQAbstractItemModelHelper(QAbstractItemModel &m);
    template <typename K, typename V> void dumpQHashNodeHelper(QHash<K, V> &hash);
    void dumpQImageHelper(const QImage &img);
    void dumpQImageDataHelper(QImage &img);
    template <typename T> void dumpQLinkedListHelper(QLinkedList<T> &l);
    void dumpQLocaleHelper(QLocale &loc);
    template <typename K, typename V> void dumpQMapHelper(QMap<K, V> &m);
    template <typename K, typename V> void dumpQMapNodeHelper(QMap<K, V> &m);
    void dumpQObjectChildListHelper(QObject &o);
    void dumpQObjectSignalHelper(QObject &o, int sigNum);
#if QT_VERSION >= 0x040500
    template <typename T>
    void dumpQSharedPointerHelper(QSharedPointer<T> &ptr);
    template <typename T>
    void dumpQWeakPointerHelper(QWeakPointer<T> &ptr);
#endif
    void dumpQTextCodecHelper(QTextCodec *codec);
};

void tst_Dumpers::infoBreak()
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

void tst_Dumpers::niceType()
{
    // cf. watchutils.cpp
    QFETCH(QString, input);
    QFETCH(QString, simplified);
    QCOMPARE(::niceType(input), simplified);
}

void tst_Dumpers::niceType_data()
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

static void testDumper(QByteArray expected0, const void *data, QByteArray outertype,
    bool dumpChildren, QByteArray innertype = "", QByteArray exp = "",
    int extraInt0 = 0, int extraInt1 = 0, int extraInt2 = 0, int extraInt3 = 0)
{
    sprintf(xDumpInBuffer, "%s%c%s%c%s%c%s%c%s%c",
        outertype.data(), 0, "iname", 0, exp.data(), 0,
        innertype.data(), 0, "iname", 0);
    //qDebug() << "FIXME qDumpObjectData440 signature to use const void *";
    void *res = qDumpObjectData440(2, 42, data, dumpChildren,
        extraInt0, extraInt1, extraInt2, extraInt3);
    QString expected(expected0);
    char buf[100];
    sprintf(buf, pointerPrintFormat(), (quintptr)data);
    if ((!expected.startsWith('t') && !expected.startsWith('f'))
            || expected.startsWith("type"))
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
                qWarning() << "== " << l1.at(i);
            else
                //qWarning() << "!= " << l1.at(i).right(30) << l2.at(i).right(30);
                qWarning() << "!= " << l1.at(i) << l2.at(i);
        }
        if (l1.size() != l2.size())
            qWarning() << "!= size: " << l1.size() << l2.size();
    }
    QCOMPARE(actual____, expected);
}

QByteArray str(const void *p)
{
    char buf[100];
    sprintf(buf, pointerPrintFormat(), (quintptr)p);
    return buf;
}

static const void *deref(const void *p)
{
    return *reinterpret_cast<const char* const*>(p);
}

void tst_Dumpers::dumperCompatibility()
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
        QByteArray("0x") + QByteArray::number((quintptr) p, 16));
}

static const QByteArray generateQStringSpec(const QString &str, bool isNull = false)
{
    if (isNull)
        return QByteArray("value='IiIgKG51bGwp',valueencoded='5',"
            "type='" NS "QString',numchild='0'");
    return
        QByteArray("value='%',valueencoded='2',type='" NS "QString',numchild='0'")
                << utfToBase64(str);
}

static const QByteArray generateQCharSpec(const QChar& ch)
{
    return QByteArray("value='%',valueencoded='2',type='" NS "QChar',numchild='0'")
        << utfToBase64(QString(QLatin1String("'%1' (%2, 0x%3)")).
                   arg(ch).arg(ch.unicode()).arg(ch.unicode(), 0, 16));
}

static const QByteArray generateBoolSpec(bool b)
{
    return QByteArray("value=%,type='bool',numchild='0'")
        << boolToVal(b);
}

static const QByteArray generateLongSpec(long n)
{
    return QByteArray("value='%',type='long',numchild='0'")
        << N(qlonglong(n));
}

static const QByteArray generateIntSpec(int n)
{
    return QByteArray("value='%',type='int',numchild='0'")
        << N(n);
}

const QByteArray createExp(const void *ptr,
    const QByteArray &type, const QByteArray &method)
{
    return QByteArray("exp='((" NSX "%" NSY "*)%)->%'")
        << type << ptrToBa(ptr) << method;
}

// Helper functions.

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
    return QByteArray().append(N(n));
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

void tst_Dumpers::dumpQAbstractItemHelper(QModelIndex &index)
{
    const QAbstractItemModel *model = index.model();
    const QString &rowStr = N(index.row());
    const QString &colStr = N(index.column());
    const QByteArray &internalPtrStrSymbolic = ptrToBa(index.internalPointer());
    const QByteArray &internalPtrStrValue = ptrToBa(index.internalPointer(), false);
    const QByteArray &modelPtrStr = ptrToBa(model);
    QByteArray indexSpecSymbolic = QByteArray().append(rowStr + "," + colStr + ",").
        append(internalPtrStrSymbolic + "," + modelPtrStr);
    QByteArray indexSpecValue = QByteArray().append(rowStr + "," + colStr + ",").
        append(internalPtrStrValue + "," + modelPtrStr);
    QByteArray expected = QByteArray("tiname='iname',addr='").append(ptrToBa(&index)).
        append("',type='" NS "QAbstractItem',addr='$").append(indexSpecSymbolic).
        append("',value='").append(valToString(model->data(index).toString())).
        append("',numchild='%',children=[");
    int rowCount = model->rowCount(index);
    int columnCount = model->columnCount(index);
    expected <<= N(rowCount * columnCount);
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < columnCount; ++col) {
            const QModelIndex &childIndex = model->index(row, col, index);
            expected.append("{name='[").append(valToString(row)).append(",").
                append(N(col)).append("]',numchild='%',addr='$").
                append(N(childIndex.row())).append(",").
                append(N(childIndex.column())).append(",").
                append(ptrToBa(childIndex.internalPointer())).append(",").
                append(modelPtrStr).append("',type='" NS "QAbstractItem',value='").
                append(valToString(model->data(childIndex).toString())).append("'}");
            expected <<= N(model->rowCount(childIndex)
                           * model->columnCount(childIndex));
            if (col < columnCount - 1 || row < rowCount - 1)
                expected.append(",");
        }
    }
    expected.append("]");
    testDumper(expected, &index, NS"QAbstractItem", true, indexSpecValue);
}

void tst_Dumpers::dumpQAbstractItemAndModelIndex()
{
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

    // Case 1: ModelIndex with no children.
    QStringListModel m(QStringList() << "item1" << "item2" << "item3");
    QModelIndex index = m.index(2, 0);

    testDumper(QByteArray("type='$T',value='(2, 0)',numchild='5',children=["
        "{name='row',value='2',type='int',numchild='0'},"
        "{name='column',value='0',type='int',numchild='0'},"
        "{name='parent',value='<invalid>',exp='(('$T'*)$A)->parent()',"
            "type='$T',numchild='1'},"
        "{name='internalId',%},"
        "{name='model',value='%',type='" NS "QAbstractItemModel*',"
            "numchild='1'}]")
         << generateQStringSpec(N(index.internalId()))
         << ptrToBa(&m),
        &index, NS"QModelIndex", true);

    // Case 2: ModelIndex with one child.
    QModelIndex index2 = m2.index(0, 0);
    dumpQAbstractItemHelper(index2);

    qDebug() << "FIXME: invalid indices should not have children";
    testDumper(QByteArray("type='$T',value='(0, 0)',numchild='5',children=["
        "{name='row',value='0',type='int',numchild='0'},"
        "{name='column',value='0',type='int',numchild='0'},"
        "{name='parent',value='<invalid>',exp='(('$T'*)$A)->parent()',"
            "type='$T',numchild='1'},"
        "{name='internalId',%},"
        "{name='model',value='%',type='" NS "QAbstractItemModel*',"
            "numchild='1'}]")
         << generateQStringSpec(N(index2.internalId()))
         << ptrToBa(&m2),
        &index2, NS"QModelIndex", true);


    // Case 3: ModelIndex with two children.
    QModelIndex index3 = m2.index(1, 0);
    dumpQAbstractItemHelper(index3);

    testDumper(QByteArray("type='$T',value='(1, 0)',numchild='5',children=["
        "{name='row',value='1',type='int',numchild='0'},"
        "{name='column',value='0',type='int',numchild='0'},"
        "{name='parent',value='<invalid>',exp='(('$T'*)$A)->parent()',"
            "type='$T',numchild='1'},"
        "{name='internalId',%},"
        "{name='model',value='%',type='" NS "QAbstractItemModel*',"
            "numchild='1'}]")
         << generateQStringSpec(N(index3.internalId()))
         << ptrToBa(&m2),
        &index3, NS"QModelIndex", true);


    // Case 4: ModelIndex with a parent.
    index = m2.index(0, 0, index3);
    testDumper(QByteArray("type='$T',value='(0, 0)',numchild='5',children=["
        "{name='row',value='0',type='int',numchild='0'},"
        "{name='column',value='0',type='int',numchild='0'},"
        "{name='parent',value='(1, 0)',exp='(('$T'*)$A)->parent()',"
            "type='$T',numchild='1'},"
        "{name='internalId',%},"
        "{name='model',value='%',type='" NS "QAbstractItemModel*',"
            "numchild='1'}]")
         << generateQStringSpec(N(index.internalId()))
         << ptrToBa(&m2),
        &index, NS"QModelIndex", true);


    // Case 5: Empty ModelIndex
    QModelIndex index4;
    testDumper("type='$T',value='<invalid>',numchild='0'",
        &index4, NS"QModelIndex", true);
}

void tst_Dumpers::dumpQAbstractItemModelHelper(QAbstractItemModel &m)
{
    QByteArray address = ptrToBa(&m);
    QByteArray expected = QByteArray("tiname='iname',addr='%',"
        "type='" NS "QAbstractItemModel',value='(%,%)',numchild='1',children=["
        "{numchild='1',name='" NS "QObject',addr='%',value='%',"
        "valueencoded='2',type='" NS "QObject',displayedtype='%'}")
            << address
            << N(m.rowCount())
            << N(m.columnCount())
            << address
            << utfToBase64(m.objectName())
            << m.metaObject()->className();

    for (int row = 0; row < m.rowCount(); ++row) {
        for (int column = 0; column < m.columnCount(); ++column) {
            QModelIndex mi = m.index(row, column);
            expected.append(QByteArray(",{name='[%,%]',value='%',"
                "valueencoded='2',numchild='%',addr='$%,%,%,%',"
                "type='" NS "QAbstractItem'}")
                << N(row)
                << N(column)
                << utfToBase64(m.data(mi).toString())
                << N(row * column)
                << N(mi.row())
                << N(mi.column())
                << ptrToBa(mi.internalPointer())
                << ptrToBa(mi.model()));
        }
    }
    expected.append("]");
    testDumper(expected, &m, NS"QAbstractItemModel", true);
}

void tst_Dumpers::dumpQAbstractItemModel()
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

void tst_Dumpers::dumpQByteArray()
{
    // Case 1: Empty object.
    QByteArray ba;
    testDumper("value='',valueencoded='1',type='" NS "QByteArray',numchild='0',"
            "childtype='char',childnumchild='0',children=[]",
        &ba, NS"QByteArray", true);

    // Case 2: One element.
    ba.append('a');
    testDumper("value='YQ==',valueencoded='1',type='" NS "QByteArray',numchild='1',"
            "childtype='char',childnumchild='0',children=[{value='61  (97 'a')'}]",
        &ba, NS"QByteArray", true);

    // Case 3: Two elements.
    ba.append('b');
    testDumper("value='YWI=',valueencoded='1',type='" NS "QByteArray',numchild='2',"
            "childtype='char',childnumchild='0',children=["
            "{value='61  (97 'a')'},{value='62  (98 'b')'}]",
        &ba, NS"QByteArray", true);

    // Case 4: > 100 elements.
    ba = QByteArray(101, 'a');
    QByteArray children;
    for (int i = 0; i < 101; i++)
        children.append("{value='61  (97 'a')'},");
    children.chop(1);
    testDumper(QByteArray("value='YWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFh"
            "YWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFh"
            "YWFhYWFhYWFhYWFhYWFhYWFhYWFhYQ== <size: 101, cut...>',"
            "valueencoded='1',type='" NS "QByteArray',numchild='101',"
            "childtype='char',childnumchild='0',children=[%]") << children,
        &ba, NS"QByteArray", true);

    // Case 5: Regular and special characters and the replacement character.
    ba = QByteArray("abc\a\n\r\033\'\"?");
    testDumper("value='YWJjBwoNGyciPw==',valueencoded='1',type='" NS "QByteArray',"
            "numchild='10',childtype='char',childnumchild='0',children=["
            "{value='61  (97 'a')'},{value='62  (98 'b')'},"
            "{value='63  (99 'c')'},{value='07  (7 '?')'},"
            "{value='0a  (10 '?')'},{value='0d  (13 '?')'},"
            "{value='1b  (27 '?')'},{value='27  (39 '?')'},"
            "{value='22  (34 '?')'},{value='3f  (63 '?')'}]",
        &ba, NS"QByteArray", true);
}

void tst_Dumpers::dumpQChar()
{
    // Case 1: Printable ASCII character.
    QChar c('X');
    testDumper("value=''X', ucs=88',numchild='0'",
        &c, NS"QChar", false);

    // Case 2: Printable non-ASCII character.
    c = QChar(0x600);
    testDumper("value=''?', ucs=1536',numchild='0'",
        &c, NS"QChar", false);

    // Case 3: Non-printable ASCII character.
    c = QChar::fromAscii('\a');
    testDumper("value=''?', ucs=7',numchild='0'",
        &c, NS"QChar", false);

    // Case 4: Non-printable non-ASCII character.
    c = QChar(0x9f);
    testDumper("value=''?', ucs=159',numchild='0'",
        &c, NS"QChar", false);

    // Case 5: Printable ASCII Character that looks like the replacement character.
    c = QChar::fromAscii('?');
    testDumper("value=''?', ucs=63',numchild='0'",
        &c, NS"QChar", false);
}

void tst_Dumpers::dumpQDateTime()
{
    // Case 1: Null object.
    QDateTime d;
    testDumper("value='(null)',type='$T',numchild='0'",
        &d, NS"QDateTime", true);

    // Case 2: Non-null object.
    d = QDateTime::currentDateTime();
    testDumper(QByteArray("value='%',valueencoded='2',"
        "type='$T',numchild='1',children=["
        "{name='toTime_t',%},"
        "{name='toString',%},"
        "{name='toString_(ISO)',%},"
        "{name='toString_(SystemLocale)',%},"
        "{name='toString_(Locale)',%}]")
            << utfToBase64(d.toString())
            << generateLongSpec((d.toTime_t()))
            << generateQStringSpec(d.toString())
            << generateQStringSpec(d.toString(Qt::ISODate))
            << generateQStringSpec(d.toString(Qt::SystemLocaleDate))
            << generateQStringSpec(d.toString(Qt::LocaleDate)),
        &d, NS"QDateTime", true);

}

void tst_Dumpers::dumpQDir()
{
    // Case 1: Current working directory.
    QDir dir = QDir::current();
    testDumper(QByteArray("value='%',valueencoded='2',type='" NS "QDir',numchild='3',"
        "children=[{name='absolutePath',%},{name='canonicalPath',%}]")
            << utfToBase64(dir.absolutePath())
            << generateQStringSpec(dir.absolutePath())
            << generateQStringSpec(dir.canonicalPath()),
        &dir, NS"QDir", true);

    // Case 2: Root directory.
    dir = QDir::root();
    testDumper(QByteArray("value='%',valueencoded='2',type='" NS "QDir',numchild='3',"
        "children=[{name='absolutePath',%},{name='canonicalPath',%}]")
            << utfToBase64(dir.absolutePath())
            << generateQStringSpec(dir.absolutePath())
            << generateQStringSpec(dir.canonicalPath()),
        &dir, NS"QDir", true);
}


void tst_Dumpers::dumpQFile()
{
    // Case 1: Empty file name => Does not exist.
    QFile file1("");
    testDumper(QByteArray("value='',valueencoded='2',type='$T',numchild='2',"
        "children=[{name='fileName',value='',valueencoded='2',type='" NS "QString',"
        "numchild='0'},"
        "{name='exists',value='false',type='bool',numchild='0'}]"),
        &file1, NS"QFile", true);

    // Case 2: File that is known to exist.
    QTemporaryFile file2;
    file2.open();
    testDumper(QByteArray("value='%',valueencoded='2',type='$T',numchild='2',"
        "children=[{name='fileName',value='%',valueencoded='2',type='" NS "QString',"
        "numchild='0'},"
        "{name='exists',value='true',type='bool',numchild='0'}]")
            << utfToBase64(file2.fileName()) << utfToBase64(file2.fileName()),
        &file2, NS"QFile", true);

    // Case 3: File with a name that most likely does not exist.
    QFile file3("jfjfdskjdflsdfjfdls");
    testDumper(QByteArray("value='%',valueencoded='2',type='$T',numchild='2',"
        "children=[{name='fileName',value='%',valueencoded='2',type='" NS "QString',"
        "numchild='0'},"
        "{name='exists',value='false',type='bool',numchild='0'}]")
            << utfToBase64(file3.fileName()) << utfToBase64(file3.fileName()),
        &file3, NS"QFile", true);
}

void tst_Dumpers::dumpQFileInfo()
{
    QFileInfo fi(".");
    QByteArray expected("value='%',valueencoded='2',type='$T',numchild='3',"
        "children=["
        "{name='absolutePath',%},"
        "{name='absoluteFilePath',%},"
        "{name='canonicalPath',%},"
        "{name='canonicalFilePath',%},"
        "{name='completeBaseName',%},"
        "{name='completeSuffix',%},"
        "{name='baseName',%},"
#ifdef QX
        "{name='isBundle',%},"
        "{name='bundleName',%},"
#endif
        "{name='fileName',%},"
        "{name='filePath',%},"
        "{name='group',%},"
        "{name='owner',%},"
        "{name='path',%},"
        "{name='groupid',%},"
        "{name='ownerid',%},"
        "{name='permissions',value=' ',type='%',numchild='10',"
        "children=[{name='ReadOwner',%},{name='WriteOwner',%},"
        "{name='ExeOwner',%},{name='ReadUser',%},{name='WriteUser',%},"
        "{name='ExeUser',%},{name='ReadGroup',%},{name='WriteGroup',%},"
        "{name='ExeGroup',%},{name='ReadOther',%},{name='WriteOther',%},"
        "{name='ExeOther',%}]},"
        "{name='caching',%},"
        "{name='exists',%},"
        "{name='isAbsolute',%},"
        "{name='isDir',%},"
        "{name='isExecutable',%},"
        "{name='isFile',%},"
        "{name='isHidden',%},"
        "{name='isReadable',%},"
        "{name='isRelative',%},"
        "{name='isRoot',%},"
        "{name='isSymLink',%},"
        "{name='isWritable',%},"
        "{name='created',value='%',valueencoded='2',%,"
            "type='" NS "QDateTime',numchild='1'},"
        "{name='lastModified',value='%',valueencoded='2',%,"
            "type='" NS "QDateTime',numchild='1'},"
        "{name='lastRead',value='%',valueencoded='2',%,"
            "type='" NS "QDateTime',numchild='1'}]");
    expected <<= utfToBase64(fi.filePath());
    expected <<= generateQStringSpec(fi.absolutePath());
    expected <<= generateQStringSpec(fi.absoluteFilePath());
    expected <<= generateQStringSpec(fi.canonicalPath());
    expected <<= generateQStringSpec(fi.canonicalFilePath());
    expected <<= generateQStringSpec(fi.completeBaseName());
    expected <<= generateQStringSpec(fi.completeSuffix(), true);
    expected <<= generateQStringSpec(fi.baseName());
#ifdef Q_OS_MACX
    expected <<= generateBoolSpec(fi.isBundle());
    expected <<= generateQStringSpec(fi.bundleName());
#endif
    expected <<= generateQStringSpec(fi.fileName());
    expected <<= generateQStringSpec(fi.filePath());
    expected <<= generateQStringSpec(fi.group());
    expected <<= generateQStringSpec(fi.owner());
    expected <<= generateQStringSpec(fi.path());
    expected <<= generateLongSpec(fi.groupId());
    expected <<= generateLongSpec(fi.ownerId());
    QFile::Permissions perms = fi.permissions();
    expected <<= QByteArray(NS"QFile::Permissions");
    expected <<= generateBoolSpec((perms & QFile::ReadOwner) != 0);
    expected <<= generateBoolSpec((perms & QFile::WriteOwner) != 0);
    expected <<= generateBoolSpec((perms & QFile::ExeOwner) != 0);
    expected <<= generateBoolSpec((perms & QFile::ReadUser) != 0);
    expected <<= generateBoolSpec((perms & QFile::WriteUser) != 0);
    expected <<= generateBoolSpec((perms & QFile::ExeUser) != 0);
    expected <<= generateBoolSpec((perms & QFile::ReadGroup) != 0);
    expected <<= generateBoolSpec((perms & QFile::WriteGroup) != 0);
    expected <<= generateBoolSpec((perms & QFile::ExeGroup) != 0);
    expected <<= generateBoolSpec((perms & QFile::ReadOther) != 0);
    expected <<= generateBoolSpec((perms & QFile::WriteOther) != 0);
    expected <<= generateBoolSpec((perms & QFile::ExeOther) != 0);
    expected <<= generateBoolSpec(fi.caching());
    expected <<= generateBoolSpec(fi.exists());
    expected <<= generateBoolSpec(fi.isAbsolute());
    expected <<= generateBoolSpec(fi.isDir());
    expected <<= generateBoolSpec(fi.isExecutable());
    expected <<= generateBoolSpec(fi.isFile());
    expected <<= generateBoolSpec(fi.isHidden());
    expected <<= generateBoolSpec(fi.isReadable());
    expected <<= generateBoolSpec(fi.isRelative());
    expected <<= generateBoolSpec(fi.isRoot());
    expected <<= generateBoolSpec(fi.isSymLink());
    expected <<= generateBoolSpec(fi.isWritable());
    expected <<= utfToBase64(fi.created().toString());
    expected <<= createExp(&fi, "QFileInfo", "created()");
    expected <<= utfToBase64(fi.lastModified().toString());
    expected <<= createExp(&fi, "QFileInfo", "lastModified()");
    expected <<= utfToBase64(fi.lastRead().toString());
    expected <<= createExp(&fi, "QFileInfo", "lastRead()");

    testDumper(expected, &fi, NS"QFileInfo", true);
}

void tst_Dumpers::dumpQHash()
{
    QHash<QString, QList<int> > hash;
    hash.insert("Hallo", QList<int>());
    hash.insert("Welt", QList<int>() << 1);
    hash.insert("!", QList<int>() << 1 << 2);
    hash.insert("!", QList<int>() << 1 << 2);
}

template <typename K, typename V>
void tst_Dumpers::dumpQHashNodeHelper(QHash<K, V> &hash)
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

void tst_Dumpers::dumpQHashNode()
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

void tst_Dumpers::dumpQImageHelper(const QImage &img)
{
    QByteArray expected = "value='(%x%)',type='" NS "QImage',numchild='0'"
            << N(img.width())
            << N(img.height());
    testDumper(expected, &img, NS"QImage", true);
}

void tst_Dumpers::dumpQImage()
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

void tst_Dumpers::dumpQImageDataHelper(QImage &img)
{
#if QT_VERSION >= 0x050000
    QSKIP("QImage::numBytes deprecated");
#else
    const QByteArray ba(QByteArray::fromRawData((const char*) img.bits(), img.numBytes()));
    QByteArray expected = QByteArray("tiname='$I',addr='$A',type='" NS "QImageData',").
        append("numchild='0',value='<hover here>',valuetooltipencoded='1',").
        append("valuetooltipsize='").append(N(ba.size())).append("',").
        append("valuetooltip='").append(ba.toBase64()).append("'");
    testDumper(expected, &img, NS"QImageData", false);
#endif
}

void tst_Dumpers::dumpQImageData()
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
        void tst_Dumpers::dumpQLinkedListHelper(QLinkedList<T> &l)
{
    const int size = qMin(l.size(), 1000);
    const QString &sizeStr = N(size);
    const QByteArray elemTypeStr = typeToString<T>();
    QByteArray expected = QByteArray("value='<").append(sizeStr).
        append(" items>',valueeditable='false',numchild='").append(sizeStr).
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
                expected.append("addr='").append(addrStr).append("',type='").
                        append(typeStr).append("',value='").
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

void tst_Dumpers::dumpQLinkedList()
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
        l2.append("Test " + N(i));

    // Case 3: Pointer type.
    QLinkedList<int *> l3;
    l3.append(new int(5));
    l3.append(new int(7));
    l3.append(0);
    dumpQLinkedListHelper(l3);
}

#    if 0
    void tst_Debugger::dumpQLinkedList()
    {
        // Case 1: Simple element type.
        QLinkedList<int> l;

        // Case 1.1: Empty list.
        testDumper("value='<0 items>',valueeditable='false',numchild='0',"
            "childtype='int',childnumchild='0',children=[]",
            &l, NS"QLinkedList", true, "int");

        // Case 1.2: One element.
        l.append(2);
        testDumper("value='<1 items>',valueeditable='false',numchild='1',"
            "childtype='int',childnumchild='0',children=[{addr='%',value='2'}]"
                << ptrToBa(l.constBegin().operator->()),
            &l, NS"QLinkedList", true, "int");

        // Case 1.3: Two elements
        l.append(3);
        QByteArray it0 = ptrToBa(l.constBegin().operator->());
        QByteArray it1 = ptrToBa(l.constBegin().operator++().operator->());
        testDumper("value='<2 items>',valueeditable='false',numchild='2',"
            "childtype='int',childnumchild='0',children=[{addr='%',value='2'},"
            "{addr='%',value='3'}]" << it0 << it1,
            &l, NS"QLinkedList", true, "int");

        // Case 2: Composite element type.
        QLinkedList<QString> l2;
        QLinkedList<QString>::const_iterator iter;

        // Case 2.1: Empty list.
        testDumper("value='<0 items>',valueeditable='false',numchild='0',"
            "childtype='" NS "QString',childnumchild='0',children=[]",
            &l2, NS"QLinkedList", true, NS"QString");

        // Case 2.2: One element.
        l2.append("Teststring 1");
        iter = l2.constBegin();
        qDebug() << *iter;
        testDumper("value='<1 items>',valueeditable='false',numchild='1',"
            "childtype='" NS "QString',childnumchild='0',children=[{addr='%',value='%',}]"
                << ptrToBa(iter.operator->()) << utfToBase64(*iter),
            &l2, NS"QLinkedList", true, NS"QString");

        // Case 2.3: Two elements.
        QByteArray expected = "value='<2 items>',valueeditable='false',numchild='2',"
            "childtype='int',childnumchild='0',children=[";
        iter = l2.constBegin();
        expected.append("{addr='%',%},"
            << ptrToBa(iter.operator->()) << utfToBase64(*iter));
        ++iter;
        expected.append("{addr='%',%}]"
            << ptrToBa(iter.operator->()) << utfToBase64(*iter));
        testDumper(expected,
            &l, NS"QLinkedList", true, NS"QString");

        // Case 2.4: > 1000 elements.
        for (int i = 3; i <= 1002; ++i)
            l2.append("Test " + N(i));

        expected = "value='<1002 items>',valueeditable='false',"
            "numchild='1002',childtype='" NS "QString',childnumchild='0',children=['";
        iter = l2.constBegin();
        for (int i = 0; i < 1002; ++i, ++iter)
            expected.append("{addr='%',value='%'},"
                << ptrToBa(iter.operator->()) << utfToBase64(*iter));
        expected.append(",...]");
        testDumper(expected, &l, NS"QLinkedList", true, NS"QString");


        // Case 3: Pointer type.
        QLinkedList<int *> l3;
        l3.append(new int(5));
        l3.append(new int(7));
        l3.append(0);
        //dumpQLinkedListHelper(l3);
        testDumper("", &l, NS"QLinkedList", true, NS"QString");
    }
#    endif

void tst_Dumpers::dumpQList_int()
{
    QList<int> ilist;
    testDumper("value='<0 items>',valueeditable='false',numchild='0',"
        "internal='1',children=[]",
        &ilist, NS"QList", true, "int");
    ilist.append(1);
    ilist.append(2);
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "internal='1',childtype='int',childnumchild='0',children=["
        "{addr='" + str(&ilist.at(0)) + "',value='1'},"
        "{addr='" + str(&ilist.at(1)) + "',value='2'}]",
        &ilist, NS"QList", true, "int");
}

void tst_Dumpers::dumpQList_int_star()
{
    QList<int *> ilist;
    testDumper("value='<0 items>',valueeditable='false',numchild='0',"
        "internal='1',children=[]",
        &ilist, NS"QList", true, "int*");
    ilist.append(new int(1));
    ilist.append(0);
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "internal='1',childtype='int*',childnumchild='1',children=["
        "{addr='" + str(deref(&ilist.at(0))) +
            "',type='int',value='1'},"
        "{value='<null>',numchild='0'}]",
        &ilist, NS"QList", true, "int*");
}

void tst_Dumpers::dumpQList_char()
{
    QList<char> clist;
    testDumper("value='<0 items>',valueeditable='false',numchild='0',"
        "internal='1',children=[]",
        &clist, NS"QList", true, "char");
    clist.append('a');
    clist.append('b');
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "internal='1',childtype='char',childnumchild='0',children=["
        "{addr='" + str(&clist.at(0)) + "',value=''a', ascii=97'},"
        "{addr='" + str(&clist.at(1)) + "',value=''b', ascii=98'}]",
        &clist, NS"QList", true, "char");
}

void tst_Dumpers::dumpQList_QString()
{
    QList<QString> slist;
    testDumper("value='<0 items>',valueeditable='false',numchild='0',"
        "internal='1',children=[]",
        &slist, NS"QList", true, NS"QString");
    slist.append("a");
    slist.append("b");
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "internal='1',childtype='" NS "QString',childnumchild='0',children=["
        "{addr='" + str(&slist.at(0)) + "',value='YQA=',valueencoded='2'},"
        "{addr='" + str(&slist.at(1)) + "',value='YgA=',valueencoded='2'}]",
        &slist, NS"QList", true, NS"QString");
}

void tst_Dumpers::dumpQList_Int3()
{
    QList<Int3> i3list;
    testDumper("value='<0 items>',valueeditable='false',numchild='0',"
        "internal='0',children=[]",
        &i3list, NS"QList", true, "Int3");
    i3list.append(Int3());
    i3list.append(Int3());
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "internal='0',childtype='Int3',children=["
        "{addr='" + str(&i3list.at(0)) + "'},"
        "{addr='" + str(&i3list.at(1)) + "'}]",
        &i3list, NS"QList", true, "Int3");
}

void tst_Dumpers::dumpQList_QString3()
{
    QList<QString3> s3list;
    testDumper("value='<0 items>',valueeditable='false',numchild='0',"
        "internal='0',children=[]",
        &s3list, NS"QList", true, "QString3");
    s3list.append(QString3());
    s3list.append(QString3());
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "internal='0',childtype='QString3',children=["
        "{addr='" + str(&s3list.at(0)) + "'},"
        "{addr='" + str(&s3list.at(1)) + "'}]",
        &s3list, NS"QList", true, "QString3");
}

void tst_Dumpers::dumpQLocaleHelper(QLocale &loc)
{
    QByteArray expected = QByteArray("value='%',type='$T',numchild='8',"
            "children=[{name='country',%},"
            "{name='language',%},"
            "{name='measurementSystem',%},"
            "{name='numberOptions',%},"
            "{name='timeFormat_(short)',%},"
            "{name='timeFormat_(long)',%},"
            "{name='decimalPoint',%},"
            "{name='exponential',%},"
            "{name='percent',%},"
            "{name='zeroDigit',%},"
            "{name='groupSeparator',%},"
            "{name='negativeSign',%}]")
        << valToString(loc.name())
        << createExp(&loc, "QLocale", "country()")
        << createExp(&loc, "QLocale", "language()")
        << createExp(&loc, "QLocale", "measurementSystem()")
        << createExp(&loc, "QLocale", "numberOptions()")
        << generateQStringSpec(loc.timeFormat(QLocale::ShortFormat))
        << generateQStringSpec(loc.timeFormat())
        << generateQCharSpec(loc.decimalPoint())
        << generateQCharSpec(loc.exponential())
        << generateQCharSpec(loc.percent())
        << generateQCharSpec(loc.zeroDigit())
        << generateQCharSpec(loc.groupSeparator())
        << generateQCharSpec(loc.negativeSign());
    testDumper(expected, &loc, NS"QLocale", true);
}

void tst_Dumpers::dumpQLocale()
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
        void tst_Dumpers::dumpQMapHelper(QMap<K, V> &map)
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
        append(sizeStr).append("',extra='simplekey: ").append(N(simpleKey)).
        append(" isSimpleValue: ").append(N(simpleVal)).
        append(" keyOffset: ").append(N(transKeyOffset)).append(" valueOffset: ").
        append(N(transValOffset)).append(" mapnodesize: ").
        append(N(qulonglong(nodeSize))).append("',children=["); // 64bit Linux hack
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
            QByteArray keyTypeStr = typeToString<K>();
            QByteArray valTypeStr = typeToString<V>();
#if QT_VERSION >= 0x040500
            QMapNode<K, V> *node = 0;
            size_t backwardOffset = size_t(&node->backward) - valOff;
            char *addr = reinterpret_cast<char *>(&(*it)) + backwardOffset;
            expected.append("addr='").append(ptrToBa(addr)).
                append("',type='" NS "QMapNode<").append(keyTypeStr).append(",").
                    append(valTypeStr).append(" >").append("'");
#else
            expected.append("type='" NS "QMapData::Node<").append(keyTypeStr).
                append(",").append(valTypeStr).append(" >").
                append("',exp='*('" NS "QMapData::Node<").append(keyTypeStr).
                append(",").append(valTypeStr).append(" >").
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

void tst_Dumpers::dumpQMap()
{
    qDebug() << "QMap<int, int>";
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
    qDebug() << "QMap<int, QString>";
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
    qDebug() << "QMap<QString, int>";
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
        void tst_Dumpers::dumpQMapNodeHelper(QMap<K, V> &m)
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

void tst_Dumpers::dumpQMapNode()
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

#ifdef USE_PRIVATE
void tst_Dumpers::dumpQObject()
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
    connect(&child, SIGNAL(destroyed()), qApp, SLOT(quit()));
    testDumper(ba, &child, NS"QObject", true);
    disconnect(&child, SIGNAL(destroyed()), qApp, SLOT(quit()));
    testDumper(ba, &child, NS"QObject", true);
    child.setObjectName("A renamed Child");
    testDumper("value='QQAgAHIAZQBuAGEAbQBlAGQAIABDAGgAaQBsAGQA',valueencoded='2',"
        "type='$T',displayedtype='QObject',numchild='4'",
        &child, NS"QObject", false);
}

void tst_Dumpers::dumpQObjectChildListHelper(QObject &o)
{
    const QObjectList children = o.children();
    const int size = children.size();
    const QString sizeStr = N(size);
    QByteArray expected = QByteArray("numchild='").append(sizeStr).append("',value='<").
        append(sizeStr).append(" items>',type='" NS "QObjectChildList',children=[");
    for (int i = 0; i < size; ++i) {
        const QObject *child = children.at(i);
        expected.append("{addr='").append(ptrToBa(child)).append("',value='").
            append(utfToBase64(child->objectName())).
            append("',valueencoded='2',type='" NS "QObject',displayedtype='").
            append(child->metaObject()->className()).append("',numchild='1'}");
        if (i < size - 1)
            expected.append(",");
    }
    expected.append("]");
    testDumper(expected, &o, NS"QObjectChildList", true);
}

void tst_Dumpers::dumpQObjectChildList()
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

void tst_Dumpers::dumpQObjectMethodList()
{
    QStringListModel m;
    testDumper("addr='<synthetic>',type='$T',numchild='24',"
        "childtype='" NS "QMetaMethod::Method',childnumchild='0',children=["
        "{name='0 0 destroyed(QObject*)',value='<Signal> (1)'},"
        "{name='1 1 destroyed()',value='<Signal> (1)'},"
        "{name='2 2 deleteLater()',value='<Slot> (2)'},"
        "{name='3 3 _q_reregisterTimers(void*)',value='<Slot> (2)'},"
        "{name='4 4 dataChanged(QModelIndex,QModelIndex)',value='<Signal> (1)'},"
        "{name='5 5 headerDataChanged(Qt::Orientation,int,int)',value='<Signal> (1)'},"
        "{name='6 6 layoutChanged()',value='<Signal> (1)'},"
        "{name='7 7 layoutAboutToBeChanged()',value='<Signal> (1)'},"
        "{name='8 8 rowsAboutToBeInserted(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='9 9 rowsInserted(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='10 10 rowsAboutToBeRemoved(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='11 11 rowsRemoved(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='12 12 columnsAboutToBeInserted(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='13 13 columnsInserted(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='14 14 columnsAboutToBeRemoved(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='15 15 columnsRemoved(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='16 16 modelAboutToBeReset()',value='<Signal> (1)'},"
        "{name='17 17 modelReset()',value='<Signal> (1)'},"
        "{name='18 18 rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)',value='<Signal> (1)'},"
        "{name='19 19 rowsMoved(QModelIndex,int,int,QModelIndex,int)',value='<Signal> (1)'},"
        "{name='20 20 columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)',value='<Signal> (1)'},"
        "{name='21 21 columnsMoved(QModelIndex,int,int,QModelIndex,int)',value='<Signal> (1)'},"
        "{name='22 22 submit()',value='<Slot> (2)'},"
        "{name='23 23 revert()',value='<Slot> (2)'}]",
    &m, NS"QObjectMethodList", true);
}

void tst_Dumpers::dumpQObjectPropertyList()
{
    // Case 1: Model without a parent.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    testDumper("addr='<synthetic>',type='$T',numchild='1',value='<1 items>',"
        "children=[{name='objectName',type='QString',value='',"
            "valueencoded='2',numchild='0'}]",
         &m, NS"QObjectPropertyList", true);

    // Case 2: Model with a parent.
    QStringListModel m2(&m);
    testDumper("addr='<synthetic>',type='$T',numchild='1',value='<1 items>',"
        "children=[{name='objectName',type='QString',value='',"
            "valueencoded='2',numchild='0'}]",
         &m2, NS"QObjectPropertyList", true);
}

static const char *connectionType(uint type)
{
    Qt::ConnectionType connType = static_cast<Qt::ConnectionType>(type);
    const char *output = "unknown";
    switch (connType) {
        case Qt::AutoConnection: output = "auto"; break;
        case Qt::DirectConnection: output = "direct"; break;
        case Qt::QueuedConnection: output = "queued"; break;
        case Qt::BlockingQueuedConnection: output = "blockingqueued"; break;
        case 3: output = "autocompat"; break;
#if QT_VERSION >= 0x040600
        case Qt::UniqueConnection: break; // Can't happen.
#endif
        default:
            qWarning() << "Unknown connection type: " << type;
            break;
        };
    return output;
};

class Cheater : public QObject
{
public:
    static const QObjectPrivate *getPrivate(const QObject &o)
    {
        const Cheater &cheater = static_cast<const Cheater&>(o);
#if QT_VERSION >= 0x040600
        return dynamic_cast<const QObjectPrivate *>(cheater.d_ptr.data());
#else
        return dynamic_cast<const QObjectPrivate *>(cheater.d_ptr);
#endif
    }
};

typedef QVector<QObjectPrivate::ConnectionList> ConnLists;

void tst_Dumpers::dumpQObjectSignalHelper(QObject &o, int sigNum)
{
    //qDebug() << o.objectName() << sigNum;
    QByteArray expected("addr='<synthetic>',numchild='1',type='" NS "QObjectSignal'");
#if QT_VERSION >= 0x040400 && QT_VERSION <= 0x040700
    expected.append(",children=[");
    const QObjectPrivate *p = Cheater::getPrivate(o);
    Q_ASSERT(p != 0);
    const ConnLists *connLists = reinterpret_cast<const ConnLists *>(p->connectionLists);
    QObjectPrivate::ConnectionList connList =
        connLists != 0 && connLists->size() > sigNum ?
        connLists->at(sigNum) : QObjectPrivate::ConnectionList();
    int i = 0;
    // FIXME: 4.6 only
    for (QObjectPrivate::Connection *conn = connList.first; conn != 0;
         ++i, conn = conn->nextConnectionList) {
        const QString iStr = N(i);
        expected.append("{name='").append(iStr).append(" receiver',");
        if (conn->receiver == &o)
            expected.append("value='<this>',type='").
                append(o.metaObject()->className()).
                append("',numchild='0',addr='").append(ptrToBa(&o)).append("'");
        else if (conn->receiver == 0)
            expected.append("value='0x0',type='" NS "QObject *',numchild='0'");
        else
            expected.append("addr='").append(ptrToBa(conn->receiver)).append("',value='").
                append(utfToBase64(conn->receiver->objectName())).append("',valueencoded='2',").
                append("type='" NS "QObject',displayedtype='").
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
    expected.append("],numchild='").append(N(i)).append("'");
#endif
    testDumper(expected, &o, NS"QObjectSignal", true, "", "", sigNum);
}

void tst_Dumpers::dumpQObjectSignal()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");
    testDumper("addr='<synthetic>',numchild='1',type='" NS "QObjectSignal',"
            "children=[],numchild='0'",
        &o, NS"QObjectSignal", true, "", "", 0);

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    for (int signalIndex = 0; signalIndex < 17; ++signalIndex)
        dumpQObjectSignalHelper(m, signalIndex);

    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
#if QT_VERSION >= 0x040600
    connect(&m, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            &m, SLOT(revert()), Qt::UniqueConnection);
#endif
    for (int signalIndex = 0; signalIndex < 17; ++signalIndex)
        dumpQObjectSignalHelper(m, signalIndex);
}

void tst_Dumpers::dumpQObjectSignalList()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");

    testDumper("type='" NS "QObjectSignalList',value='<2 items>',"
                "addr='$A',numchild='2',children=["
            "{name='0',value='destroyed(QObject*)',numchild='0',"
                "addr='$A',type='" NS "QObjectSignal'},"
            "{name='1',value='destroyed()',numchild='0',"
                "addr='$A',type='" NS "QObjectSignal'}]",
        &o, NS"QObjectSignalList", true);

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    QByteArray expected = "type='" NS "QObjectSignalList',value='<20 items>',"
        "addr='$A',numchild='20',children=["
        "{name='0',value='destroyed(QObject*)',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='1',value='destroyed()',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='4',value='dataChanged(QModelIndex,QModelIndex)',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='5',value='headerDataChanged(Qt::Orientation,int,int)',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='6',value='layoutChanged()',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='7',value='layoutAboutToBeChanged()',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='8',value='rowsAboutToBeInserted(QModelIndex,int,int)',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='9',value='rowsInserted(QModelIndex,int,int)',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='10',value='rowsAboutToBeRemoved(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='11',value='rowsRemoved(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='12',value='columnsAboutToBeInserted(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='13',value='columnsInserted(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='14',value='columnsAboutToBeRemoved(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='15',value='columnsRemoved(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='16',value='modelAboutToBeReset()',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='17',value='modelReset()',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='18',value='rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)',"
             "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='19',value='rowsMoved(QModelIndex,int,int,QModelIndex,int)',"
             "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='20',value='columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)',"
             "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='21',value='columnsMoved(QModelIndex,int,int,QModelIndex,int)',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'}]";


    testDumper(expected << "0" << "0" << "0" << "0" << "0" << "0",
        &m, NS"QObjectSignalList", true);


    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);

    testDumper(expected << "1" << "1" << "2" << "1" << "0" << "0",
        &m, NS"QObjectSignalList", true);
}

QByteArray slotIndexList(const QObject *ob)
{
    QByteArray slotIndices;
    const QMetaObject *mo = ob->metaObject();
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QMetaMethod &mm = mo->method(i);
        if (mm.methodType() == QMetaMethod::Slot) {
            int slotIndex = mo->indexOfSlot(mm.signature());
            Q_ASSERT(slotIndex != -1);
            slotIndices.append(N(slotIndex));
            slotIndices.append(',');
        }
    }
    slotIndices.chop(1);
    return slotIndices;
}

void tst_Dumpers::dumpQObjectSlot()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");

    QByteArray slotIndices = slotIndexList(&o);
    QCOMPARE(slotIndices, QByteArray("2,3"));
    QCOMPARE(o.metaObject()->indexOfSlot("deleteLater()"), 2);

    QByteArray expected = QByteArray("addr='$A',numchild='1',type='$T',"
        //"children=[{name='1 sender'}],numchild='1'");
        "children=[],numchild='0'");
    qDebug() << "FIXME!";
    testDumper(expected, &o, NS"QObjectSlot", true, "", "", 2);


    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    slotIndices = slotIndexList(&o);

    QCOMPARE(slotIndices, QByteArray("2,3"));
    QCOMPARE(o.metaObject()->indexOfSlot("deleteLater()"), 2);

    expected = QByteArray("addr='$A',numchild='1',type='$T',"
        //"children=[{name='1 sender'}],numchild='1'");
        "children=[],numchild='0'");
    qDebug() << "FIXME!";
    testDumper(expected, &o, NS"QObjectSlot", true, "", "", 2);


    // Case 3: QAbstractItemModel with connections to itself and to another
    //         o, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&o, SIGNAL(destroyed(QObject*)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
#if QT_VERSION >= 0x040600
    connect(&m, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            &m, SLOT(revert()), Qt::UniqueConnection);
#endif
    expected = QByteArray("addr='$A',numchild='1',type='$T',"
        //"children=[{name='1 sender'}],numchild='1'");
        "children=[],numchild='0'");
    qDebug() << "FIXME!";
    testDumper(expected, &o, NS"QObjectSlot", true, "", "", 2);

}

void tst_Dumpers::dumpQObjectSlotList()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");
    testDumper("numchild='2',value='<2 items>',type='$T',"
        "children=[{name='2',value='deleteLater()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'}]",
        &o, NS"QObjectSlotList", true);

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    testDumper("numchild='4',value='<4 items>',type='$T',"
        "children=[{name='2',value='deleteLater()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='22',value='submit()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='23',value='revert()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'}]",
        &m, NS"QObjectSlotList", true);

    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
    connect(&o, SIGNAL(destroyed(QObject*)), &m, SLOT(submit()));
    testDumper("numchild='4',value='<4 items>',type='$T',"
        "children=[{name='2',value='deleteLater()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='22',value='submit()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='23',value='revert()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'}]",
        &m, NS"QObjectSlotList", true);
}
#endif

void tst_Dumpers::dumpQPixmap()
{
    // Case 1: Null Pixmap.
    QPixmap p;

    testDumper("value='(0x0)',type='$T',numchild='0'",
        &p, NS"QPixmap", true);


    // Case 2: Uninitialized non-null pixmap.
    p = QPixmap(20, 100);
    testDumper("value='(20x100)',type='$T',numchild='0'",
        &p, NS"QPixmap", true);


    // Case 3: Initialized non-null pixmap.
    const char * const pixmap[] = {
        "2 24 3 1", "       c None", ".      c #DBD3CB", "+      c #FCFBFA",
        "  ", "  ", "  ", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+",
        ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", "  ", "  ", "  "
    };
    p = QPixmap(pixmap);
    testDumper("value='(2x24)',type='$T',numchild='0'",
        &p, NS"QPixmap", true);
}

#if QT_VERSION >= 0x040500
template<typename T>
void tst_Dumpers::dumpQSharedPointerHelper(QSharedPointer<T> &ptr)
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
/*
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
    expected.append(val2).append("',valueeditable='false',numchild='1',children=[").
        append("{name='data',addr='").append(ptrToBa(ptr.data())).
        append("',type='").append(typeToString<T>()).append("',value='").append(val1).
        append("'},{name='weakref',value='").append(N(weakValue)).
        append("',type='int',addr='").append(ptrToBa(weakAddr)).append("',numchild='0'},").
        append("{name='strongref',value='").append(N(strongValue)).
        append("',type='int',addr='").append(ptrToBa(strongAddr)).append("',numchild='0'}]");
    testDumper(expected, &ptr, NS"QSharedPointer", true, typeToString<T>());
*/
}
#endif

void tst_Dumpers::dumpQSharedPointer()
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
    // TODO: This case is not handled in dumper.cpp (segfault!)
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

void tst_Dumpers::dumpQString()
{
    QString s;
    testDumper("value='IiIgKG51bGwp',valueencoded='5',type='$T',numchild='0'",
        &s, NS"QString", false);
    s = "abc";
    testDumper("value='YQBiAGMA',valueencoded='2',type='$T',numchild='0'",
        &s, NS"QString", false);
}

void tst_Dumpers::dumpQVariant_invalid()
{
    QVariant v;
    testDumper("value='(invalid)',type='$T',numchild='0'",
        &v, NS"QVariant", false);
}

void tst_Dumpers::dumpQVariant_QString()
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

void tst_Dumpers::dumpQVariant_QStringList()
{
    QVariant v = QStringList() << "Hi";
    testDumper("value='(QStringList) ',type='$T',numchild='1',"
        "children=[{name='value',exp='(*('" NS "QStringList'*)%)',"
        "type='QStringList',numchild='1'}]"
            << QByteArray::number(quintptr(&v)),
        &v, NS"QVariant", true);
}

#ifndef Q_CC_MSVC

void tst_Dumpers::dumpStdVector()
{
    std::vector<std::list<int> *> vector;
    QByteArray inner = "std::list<int> *";
    QByteArray innerp = "std::list<int>";
    testDumper("value='<0 items>',valueeditable='false',numchild='0'",
        &vector, "std::vector", false, inner, "", sizeof(std::list<int> *));
    std::list<int> list;
    vector.push_back(new std::list<int>(list));
    testDumper("value='<1 items>',valueeditable='false',numchild='1',"
        "childtype='" + inner + "',childnumchild='1',"
        "children=[{addr='" + str(deref(&vector[0])) + "',type='" + innerp + "'}]",
        &vector, "std::vector", true, inner, "", sizeof(std::list<int> *));
    vector.push_back(0);
    list.push_back(45);
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "childtype='" + inner + "',childnumchild='1',"
        "children=[{addr='" + str(deref(&vector[0])) + "',type='" + innerp + "'},"
          "{addr='" + str(&vector[1]) + "',"
            "type='" + innerp + "',value='<null>',numchild='0'}]",
        &vector, "std::vector", true, inner, "", sizeof(std::list<int> *));
    vector.push_back(new std::list<int>(list));
    vector.push_back(0);
}

#endif // !Q_CC_MSVC

void tst_Dumpers::dumpQTextCodecHelper(QTextCodec *codec)
{
    const QByteArray name = codec->name().toBase64();
    QByteArray expected = QByteArray("value='%',valueencoded='1',type='$T',"
        "numchild='2',children=[{name='name',value='%',type='" NS "QByteArray',"
        "numchild='0',valueencoded='1'},{name='mibEnum',%}]")
        << name << name << generateIntSpec(codec->mibEnum());
    testDumper(expected, codec, NS"QTextCodec", true);
}

void tst_Dumpers::dumpQTextCodec()
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
void tst_Dumpers::dumpQWeakPointerHelper(QWeakPointer<T> &ptr)
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
    expected.append("',valueeditable='false',numchild='1',children=[{name='data',addr='").
        append(ptrToBa(data)).append("',type='").append(typeToString<T>()).
        append("',value='").append(dataStr).append("'},{name='weakref',value='").
        append(valToString(*weakRefPtr)).append("',type='int',addr='").
        append(ptrToBa(weakRefPtr)).append("',numchild='0'},{name='strongref',value='").
        append(valToString(*strongRefPtr)).append("',type='int',addr='").
        append(ptrToBa(strongRefPtr)).append("',numchild='0'}]");
    testDumper(expected, &ptr, NS"QWeakPointer", true, typeToString<T>());
}
#endif

void tst_Dumpers::dumpQWeakPointer()
{
#if QT_VERSION >= 0x040500
    // Case 1: Simple type.

    // Case 1.1: Null pointer.
    QSharedPointer<int> spNull;
    QWeakPointer<int> wp = spNull.toWeakRef();
    testDumper("value='<null>',valueeditable='false',numchild='0'",
        &wp, NS"QWeakPointer", true, "int");

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

#define VERIFY_OFFSETOF(member)                           \
do {                                                      \
    QObjectPrivate *qob = 0;                              \
    ObjectPrivate *ob = 0;                                \
    QVERIFY(size_t(&qob->member) == size_t(&ob->member)); \
} while (0)


void tst_Dumpers::initTestCase()
{
    QVERIFY(sizeof(QWeakPointer<int>) == 2*sizeof(void *));
    QVERIFY(sizeof(QSharedPointer<int>) == 2*sizeof(void *));
#if QT_VERSION < 0x050000
    QtSharedPointer::ExternalRefCountData d;
    d.weakref = d.strongref = 0; // That's what the destructor expects.
    QVERIFY(sizeof(int) == sizeof(d.weakref));
    QVERIFY(sizeof(int) == sizeof(d.strongref));
#endif
#ifdef USE_PRIVATE
    const size_t qObjectPrivateSize = sizeof(QObjectPrivate);
    const size_t objectPrivateSize = sizeof(ObjectPrivate);
    QVERIFY2(qObjectPrivateSize == objectPrivateSize, QString::fromLatin1("QObjectPrivate=%1 ObjectPrivate=%2").arg(qObjectPrivateSize).arg(objectPrivateSize).toLatin1().constData());
    VERIFY_OFFSETOF(threadData);
    VERIFY_OFFSETOF(extraData);
    VERIFY_OFFSETOF(objectName);
    VERIFY_OFFSETOF(connectionLists);
    VERIFY_OFFSETOF(senders);
    VERIFY_OFFSETOF(currentSender);
    VERIFY_OFFSETOF(eventFilters);
    VERIFY_OFFSETOF(currentChildBeingDeleted);
    VERIFY_OFFSETOF(connectedSignals);
    //VERIFY_OFFSETOF(deleteWatch);
#ifdef QT3_SUPPORT
#if QT_VERSION < 0x040600
    VERIFY_OFFSETOF(pendingChildInsertedEvents);
#endif
#endif
#if QT_VERSION >= 0x040600
    VERIFY_OFFSETOF(sharedRefcount);
#endif
#endif
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    tst_Dumpers test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_dumpers.moc"

