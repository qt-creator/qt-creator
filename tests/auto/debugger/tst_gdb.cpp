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

// Note: Keep the strange formating of closing braces in the  void dump*()
// functions as this reduces the risk of gdb reporting 'wrong' line numbers
// when stopping and therefore the risk of the autotest to fail.

//bool checkUninitialized = true;
bool checkUninitialized = false;
#define DO_DEBUG 1
//TESTED_COMPONENT=src/plugins/debugger/gdb

#include "debuggerprotocol.h"

#ifdef QT_GUI_LIB
#include <QBitmap>
#include <QBrush>
#include <QColor>
#include <QCursor>
#include <QFont>
#include <QIcon>
#include <QKeySequence>
#include <QMatrix4x4>
#include <QPen>
#include <QQuaternion>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QTextFormat>
#include <QTextLength>
#include <QVector2D>
#include <QWidget>
#endif

#include <QtTest>

#include <deque>
#include <map>
#include <set>
#include <vector>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#undef NS
#ifdef QT_NAMESPACE
#   define STRINGIFY0(s) #s
#   define STRINGIFY1(s) STRINGIFY0(s)
#   define NS STRINGIFY1(QT_NAMESPACE) "::"
#   define NSX "'" STRINGIFY1(QT_NAMESPACE) "::"
#   define NSY "'"
#else
#   define NS ""
#   define NSX ""
#   define NSY ""
#endif

#undef DEBUG
#if DO_DEBUG
#   define DEBUG(s) qDebug() << s
#else
#   define DEBUG(s)
#endif
#define DEBUGX(s) qDebug() << s

#define gettid()  QString("0x%1").arg((qulonglong)(void *)currentGdbWrapper(), 0, 16)

#ifdef Q_OS_WIN
QString gdbBinary = "c:\\MinGw\\bin\\gdb.exe";
#else
QString gdbBinary = "gdb";
#endif

#if  QT_VERSION >= 0x050000
#define MSKIP_SINGLE(x) QSKIP(x)
#define MSKIP_ALL(x) QSKIP(x);
#else
#define MSKIP_SINGLE(x) QSKIP(x, SkipSingle)
#define MSKIP_ALL(x) QSKIP(x, SkipAll)
#endif

void nothing() {}

template <typename T> QByteArray N(T v) { return QByteArray::number(v); }

class Foo
{
public:
    Foo(int i = 0)
        : a(i), b(2)
    {}

    ~Foo()
    {
    }

    void doit()
    {
        static QObject ob;
        m["1"] = "2";
        h[&ob] = m.begin();

        a += 1;
        --b;
        //s += 'x';
    }


    struct Bar {
        Bar() : ob(0) {}
        QObject *ob;
    };

public:
    int a, b;
    char x[6];

private:
    //QString s;
    typedef QMap<QString, QString> Map;
    Map m;
    QHash<QObject *, Map::iterator> h;
};


/////////////////////////////////////////////////////////////////////////
//
// Helper stuff
//
/////////////////////////////////////////////////////////////////////////

typedef QList<QByteArray> QByteArrayList;

struct Int3
{
    Int3(int base = 0) { i1 = 42 + base; i2 = 43 + base; i3 = 44 + base; }
    int i1, i2, i3;
};

uint qHash(const Int3 &i)
{
    return (i.i1 ^ i.i2) ^ i.i3;
}

bool operator==(const Int3 &a, const Int3 &b)
{
    return a.i1 == b.i1 && a.i2 == b.i2 && a.i3 == b.i3;
}

bool operator<(const Int3 &a, const Int3 &b)
{
    return a.i1 < b.i1;
}

struct QString3
{
    QString3() { s1 = "a"; s2 = "b"; s3 = "c"; }
    QString s1, s2, s3;
};

class tst_Gdb;

class GdbWrapper : public QObject
{
    Q_OBJECT

public:
    GdbWrapper(tst_Gdb *test);
    ~GdbWrapper();

    void startup(QProcess *proc);
    void run();

    QString errorString() const { return  m_errorString; }

public slots:
    void readStandardOutput();
    bool parseOutput(const QByteArray &ba);
    void readStandardError();
    void handleGdbStarted();
    void handleGdbError(QProcess::ProcessError);
    void handleGdbFinished(int, QProcess::ExitStatus);

    QByteArray writeToGdbRequested(const QByteArray &ba)
    {
        DEBUG("GDB IN: " << ba);
        m_proc.write(ba);
        m_proc.write("\n");
        while (true) {
            m_proc.waitForReadyRead();
            QByteArray output = m_proc.readAllStandardOutput();
            if (parseOutput(output))
                break;
        }
        return m_buffer;
    }


public:
    QByteArray m_lastStopped; // last seen "*stopped" message
    int m_line; // line extracted from last "*stopped" message
    QProcess m_proc;
    tst_Gdb *m_test; // not owned
    QString m_errorString;
    QByteArray m_buffer;
};

class tst_Gdb : public QObject
{
    Q_OBJECT

public:
    tst_Gdb();

    void prepare(const QByteArray &function);
    void check(const QByteArray &label, const QByteArray &expected,
        const QByteArray &expanded = QByteArray(), bool fancy = true);
    void next(int n = 1);

    QByteArray writeToGdb(const QByteArray &ba)
    {
        return m_gdb->writeToGdbRequested(ba);
    }

private slots:
    void initTestCase();
    void cleanupTestCase();

    void dump_array();
    void dump_misc();
    void dump_typedef();
    void dump_std_deque();
    void dump_std_list();
    void dump_std_map_int_int();
    void dump_std_map_string_string();
    void dump_std_set_Int3();
    void dump_std_set_int();
    void dump_std_string();
    void dump_std_vector();
    void dump_std_wstring();
    void dump_Foo();
    //void dump_QImageData();
    void dump_QAbstractItemAndModelIndex();
    void dump_QAbstractItemModel();
    void dump_QByteArray();
    void dump_QChar();
    void dump_QDateTime();
    void dump_QDir();
    void dump_QFile();
    void dump_QFileInfo();
    void dump_QHash_QString_QString();
    void dump_QHash_int_int();
    void dump_QImage();
    void dump_QLinkedList_int();
    void dump_QList_Int3();
    void dump_QList_QString();
    void dump_QList_QString3();
    void dump_QList_char();
    void dump_QList_char_star();
    void dump_QList_int();
    void dump_QList_int_star();
    void dump_QLocale();
    void dump_QMap_QString_QString();
    void dump_QMap_int_int();
    void dump_QObject();
    void dump_QPixmap();
    void dump_QPoint();
    void dump_QRect();
    void dump_QSet_Int3();
    void dump_QSet_int();
    void dump_QSharedPointer();
    void dump_QSize();
    void dump_QStack();
    void dump_QString();
    void dump_QStringList();
    void dump_QTextCodec();
    void dump_QVariant();
    void dump_QVector();
    void dump_QWeakPointer_11();
    void dump_QWeakPointer_12();
    void dump_QWeakPointer_13();
    void dump_QWeakPointer_2();
    void dump_QWidget();

public slots:
    void dumperCompatibility();

private:
#if 0
    void dump_QAbstractItemModelHelper(QAbstractItemModel &m);
    void dump_QObjectChildListHelper(QObject &o);
    void dump_QObjectSignalHelper(QObject &o, int sigNum);
#endif

    QHash<QByteArray, int> m_lineForLabel;
    QByteArray m_function;
    GdbWrapper *m_gdb;
};


//
// Dumpers
//

QByteArray str(const void *p)
{
    char buf[100];
    sprintf(buf, "%p", p);
    return buf;
}

void tst_Gdb::dumperCompatibility()
{
#ifdef Q_OS_MACX
    // Ensure that no arbitrary padding is introduced by QVectorTypedData.
    const size_t qVectorDataSize = 16;
    QCOMPARE(sizeof(QVectorData), qVectorDataSize);
    QVectorTypedData<int> *v = 0;
    QCOMPARE(size_t(&v->array), qVectorDataSize);
#endif
}

static const QByteArray utfToHex(const QString &string)
{
    return QByteArray(reinterpret_cast<const char *>(string.utf16()),
        2 * string.size()).toHex();
}

static const QByteArray specQString(const QString &str)
{
    return "valueencoded='7',value='" + utfToHex(str) + "'";
}

static const QByteArray specQChar(QChar ch)
{
    return QByteArray("value=''") + char(ch.unicode()) + "' ("
        + QByteArray::number(ch.unicode()) + ")'";
    //return "valueencoded='7',value='" +
    //    utfToHex(QString(QLatin1String("'%1' (%2)")).
    //              arg(ch).arg(ch.unicode())) + "'";
}



/////////////////////////////////////////////////////////////////////////
//
// GdbWrapper
//
/////////////////////////////////////////////////////////////////////////


GdbWrapper::GdbWrapper(tst_Gdb *test) : m_test(test)
{
    qWarning() << "SETUP START\n\n";
#ifndef Q_CC_GNU
    MSKIP_ALL("gdb test not applicable for compiler");
#endif
    //qDebug() << "\nRUN" << getpid() << gettid();
    QStringList args;
    args << QLatin1String("-i")
            << QLatin1String("mi") << QLatin1String("--args")
            << qApp->applicationFilePath();
    qWarning() << "Starting" << gdbBinary << args;
    m_proc.start(gdbBinary, args);
    if (!m_proc.waitForStarted()) {
        const QString msg = QString::fromLatin1("Unable to run %1: %2")
            .arg(gdbBinary, m_proc.errorString());
        MSKIP_ALL(msg.toLatin1().constData());
    }

    connect(&m_proc, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(handleGdbError(QProcess::ProcessError)));
    connect(&m_proc, SIGNAL(finished(int,QProcess::ExitStatus)),
        this, SLOT(handleGdbFinished(int,QProcess::ExitStatus)));
    connect(&m_proc, SIGNAL(started()),
        this, SLOT(handleGdbStarted()));
    connect(&m_proc, SIGNAL(readyReadStandardOutput()),
        this, SLOT(readStandardOutput()));
    connect(&m_proc, SIGNAL(readyReadStandardError()),
        this, SLOT(readStandardError()));

    m_proc.write("python execfile('../../../share/qtcreator/dumper/bridge.py')\n");
    m_proc.write("python execfile('../../../share/qtcreator/dumper/dumper.py')\n");
    m_proc.write("python execfile('../../../share/qtcreator/dumper/qttypes.py')\n");
    m_proc.write("bbsetup\n");
    m_proc.write("break breaker\n");
    m_proc.write("handle SIGSTOP stop pass\n");
    m_proc.write("run\n");
}

GdbWrapper::~GdbWrapper()
{
}

void GdbWrapper::handleGdbError(QProcess::ProcessError error)
{
    qDebug() << "GDB ERROR: " << error;
}

void GdbWrapper::handleGdbFinished(int code, QProcess::ExitStatus st)
{
    qDebug() << "GDB FINISHED: " << code << st;
}

void GdbWrapper::readStandardOutput()
{
    //parseOutput(m_proc.readAllStandardOutput());
}

bool GdbWrapper::parseOutput(const QByteArray &ba0)
{
    if (ba0.isEmpty())
        return false;
    QByteArray ba = ba0;
    DEBUG("GDB OUT: " << ba);
    // =library-loaded...
    if (ba.startsWith("=")) {
        DEBUG("LIBRARY LOADED");
        return false;
    }
    if (ba.startsWith("*stopped")) {
        m_lastStopped = ba;
        DEBUG("GDB OUT 2: " << ba);
        if (!ba.contains("func=\"breaker\"")) {
            int pos1 = ba.indexOf(",line=\"") + 7;
            int pos2 = ba.indexOf("\"", pos1);
            m_line = ba.mid(pos1, pos2 - pos1).toInt();
            DEBUG(" LINE 1: " << m_line);
        }
    }

    // The "call" is always aborted with a message like:
    //  "~"2321\t    /* A */ QString s;\n" "
    //  "&"The program being debugged stopped while in a function called ..."
    //  "^error,msg="The program being debugged stopped ..."
    // Extract the "2321" from this
    static QByteArray lastText;
    if (ba.startsWith("~")) {
        lastText = ba;
        if (ba.size() > 8
                && (ba.at(2) < 'a' || ba.at(2) > 'z')
                && (ba.at(2) < '0' || ba.at(2) > '9')
                && !ba.startsWith("~\"Breakpoint ")
                && !ba.startsWith("~\"    at ")
                && !ba.startsWith("~\"    locals=")
                && !ba.startsWith("~\"XXX:")) {
            QByteArray ba1 = ba.mid(2, ba.size() - 6);
            if (ba1.startsWith("  File "))
                ba1 = ba1.replace(2, ba1.indexOf(','), "");
            //qWarning() << "OUT: " << ba1;
        }
    }
    if (ba.startsWith("&\"The program being debugged")) {
        int pos1 = 2;
        int pos2 = lastText.indexOf("\\", pos1);
        m_line = lastText.mid(pos1, pos2 - pos1).toInt();
        DEBUG(" LINE 2: " << m_line);
    }
    if (ba.startsWith("^error,msg=")) {
        if (!ba.startsWith("^error,msg=\"The program being debugged stopped"))
            qWarning() << "ERROR: " << ba.mid(1, ba.size() - 3);
    }

    if (ba.startsWith("~\"XXX: ")) {
        QByteArray ba1 = ba.mid(7, ba.size() - 11);
        qWarning() << "MESSAGE: " << ba.mid(7, ba.size() - 12);
    }

    ba = ba.replace("\\\"", "\"");

    // No interesting output before 'locals=...'
    int pos = ba.indexOf("locals={iname=");
    if (pos == -1 && ba.isEmpty())
        return true;

    QByteArray output;
    if (output.isEmpty())
        output = ba.mid(pos);
    else
        output += ba;
    // Up to ^done\n(gdb)
    pos = output.indexOf("(gdb)");
    if (pos == -1)
        return true;
    output = output.left(pos);
    pos = output.indexOf("^done");
    if (pos >= 4)
        output = output.left(pos - 4);

    if (output.isEmpty())
        return true;

    m_buffer += output;
    return true;
}

void GdbWrapper::readStandardError()
{
    QByteArray ba = m_proc.readAllStandardError();
    qDebug() << "GDB ERR: " << ba;
}

void GdbWrapper::handleGdbStarted()
{
    //qDebug() << "\n\nGDB STARTED" << getpid() << gettid() << "\n\n";
}


/////////////////////////////////////////////////////////////////////////
//
// Test Class Framework Implementation
//
/////////////////////////////////////////////////////////////////////////

tst_Gdb::tst_Gdb()
    : m_gdb(0)
{
}

void tst_Gdb::initTestCase()
{
    const QString fileName = "tst_gdb.cpp";
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        const QString msg = QString::fromLatin1("Unable to open %1: %2")
            .arg(fileName, file.errorString());
        MSKIP_ALL(msg.toLatin1().constData());
    }

    QByteArray funcName;
    const QByteArrayList bal = file.readAll().split('\n');
    Q_ASSERT(bal.size() > 100);
    for (int i = 0; i != bal.size(); ++i) {
        const QByteArray &ba = bal.at(i);
        if (ba.startsWith("void dump")) {
            int pos = ba.indexOf('(');
            funcName = ba.mid(5, pos - 5) + '@';
        } else if (ba.startsWith("    /*")) {
            int pos = ba.indexOf('*', 7);
            m_lineForLabel[QByteArray(funcName + ba.mid(7, pos - 8)).trimmed()] = i + 1;
        }
    }

    Q_ASSERT(!m_gdb);
    m_gdb = new GdbWrapper(this);
}

void tst_Gdb::prepare(const QByteArray &function)
{
    Q_ASSERT(m_gdb);
    m_function = function;
    writeToGdb("b " + function);
    writeToGdb("call " + function + "()");
}

static bool isJoker(const QByteArray &ba)
{
    return ba.endsWith("'-'") || ba.contains("'-'}");
}

void tst_Gdb::check(const QByteArray &label, const QByteArray &expected0,
    const QByteArray &expanded, bool fancy)
{
    //qDebug() << "\nABOUT TO RUN TEST: " << expanded;
    qWarning() << label << "...";
    QByteArray options = "pe";
    if (fancy)
        options += ",fancy";
    options += ",autoderef";
    QByteArray ba = writeToGdb("bb options:" + options
        + " vars: expanded:" + expanded
        + " typeformats: formats: watchers:\n");

    //bb options:fancy,autoderef vars: expanded: typeformats:63686172202a=1
    //formats: watchers:

    //locals.fromString("{" + ba + "}");
    QByteArray received = ba.replace("\"", "'");
    QByteArray actual = received.trimmed();
    int pos = actual.indexOf("^done");
    if (pos != -1)
        actual = actual.left(pos);
    if (actual.endsWith("\n"))
        actual.chop(1);
    if (actual.endsWith("\\n"))
        actual.chop(2);
    QByteArray expected = "locals={iname='local',name='Locals',value=' ',type=' ',"
        "children=[" + expected0 + "]}";
    int line = m_gdb->m_line;

    QByteArrayList l1_0 = actual.split(',');
    QByteArrayList l1;
    for (int i = 0; i != l1_0.size(); ++i) {
        const QByteArray &ba = l1_0.at(i);
        if (ba.startsWith("watchers={iname"))
            break;
        if (!ba.startsWith("addr"))
            l1.append(ba);
    }

    QByteArrayList l2 = expected.split(',');

    bool ok = l1.size() == l2.size();
    if (ok) {
        for (int i = 0 ; i < l1.size(); ++i) {
            // Use "-" as joker.
            if (l1.at(i) != l2.at(i) && !isJoker(l2.at(i)))
                ok = false;
        }
    } else {
        qWarning() << "!= size: " << l1.size() << l2.size();
    }

    if (!ok) {
        int i = 0;
        for ( ; i < l1.size() && i < l2.size(); ++i) {
            if (l1.at(i) == l2.at(i) || isJoker(l2.at(i))) {
                qWarning() << "== " << l1.at(i);
            } else {
                //qWarning() << "!= " << l1.at(i).right(30) << l2.at(i).right(30);
                qWarning() << "!= " << l1.at(i) << l2.at(i);
                ok = false;
            }
        }
        for ( ; i < l2.size(); ++i)
            qWarning() << "!= " << "-----" << l2.at(i);
        for ( ; i < l1.size(); ++i)
            qWarning() << "!= " << l1.at(i) << "-----";
        if (l1.size() != l2.size()) {
            ok = false;
            qWarning() << "!= size: " << l1.size() << l2.size();
        }
        qWarning() << "RECEIVED: " << received;
        qWarning() << "ACTUAL  : " << actual;
    }
    QCOMPARE(ok, true);
    //qWarning() << "LINE: " << line << "ACT/EXP" << m_function + '@' + label;
    //QCOMPARE(actual, expected);


    int expline = m_lineForLabel.value(m_function + '@' + label);
    int actline = line;
    if (actline != expline) {
        qWarning() << "LAST STOPPED: " << m_gdb->m_lastStopped;
    }
    QCOMPARE(actline, expline);
}

void tst_Gdb::next(int n)
{
    for (int i = 0; i != n; ++i)
        writeToGdb("next");
}

void tst_Gdb::cleanupTestCase()
{
    Q_ASSERT(m_gdb);
    writeToGdb("kill");
    writeToGdb("quit");
    //m_gdb.m_proc.waitForFinished();
    //m_gdb.wait();
    delete m_gdb;
    m_gdb = 0;
}


/////////////////////////////////////////////////////////////////////////
//
// Dumper Tests
//
/////////////////////////////////////////////////////////////////////////


///////////////////////////// Foo structure /////////////////////////////////

/*
    Foo:
        int a, b;
        char x[6];
        typedef QMap<QString, QString> Map;
        Map m;
        QHash<QObject *, Map::iterator> h;
*/

void dump_Foo()
{
    /* A */ Foo f;
    /* B */ f.doit();
    /* D */ (void) 0;
}

void tst_Gdb::dump_Foo()
{
    prepare("dump_Foo");
    next();
    check("B","{iname='local.f',name='f',type='Foo',"
            "value='-',numchild='5'}", "", 0);
    check("B","{iname='local.f',name='f',type='Foo',"
            "value='-',numchild='5',children=["
            "{name='a',type='int',value='0',numchild='0'},"
            "{name='b',type='int',value='2',numchild='0'},"
            "{name='x',type='char [6]',"
                "value='{...}',numchild='1'},"
            "{name='m',type='" NS "QMap<" NS "QString, " NS "QString>',"
                "value='{...}',numchild='1'},"
            "{name='h',type='" NS "QHash<" NS "QObject*, "
                "" NS "QMap<" NS "QString, " NS "QString>::iterator>',"
                "value='{...}',numchild='1'}]}",
            "local.f", 0);
}


///////////////////////////// Array ///////////////////////////////////////

void dump_array_char()
{
    /* A */ const char s[] = "XYZ";
    /* B */ (void) &s; }

void dump_array_int()
{
    /* A */ int s[] = {1, 2, 3};
    /* B */ (void) s; }

void tst_Gdb::dump_array()
{
    prepare("dump_array_char");
    next();
    // FIXME: numchild should be '4', not '1'
    check("B","{iname='local.s',name='s',type='char [4]',"
            "value='-',numchild='1'}", "");
    check("B","{iname='local.s',name='s',type='char [4]',"
            "value='-',numchild='1',childtype='char',childnumchild='0',"
            "children=[{value='88 'X''},{value='89 'Y''},{value='90 'Z''},"
            "{value='0 '\\\\000''}]}",
            "local.s");

    prepare("dump_array_int");
    next();
    // FIXME: numchild should be '3', not '1'
    check("B","{iname='local.s',name='s',type='int [3]',"
            "value='-',numchild='1'}", "");
    check("B","{iname='local.s',name='s',type='int [3]',"
            "value='-',numchild='1',childtype='int',childnumchild='0',"
            "children=[{value='1'},{value='2'},{value='3'}]}",
            "local.s");
}


///////////////////////////// Misc stuff /////////////////////////////////

void dump_misc()
{
#if 1
    /* A */ int *s = new int(1);
    /* B */ *s += 1;
    /* D */ (void) 0;
#else
    QVariant v1(QLatin1String("hallo"));
    QVariant v2(QStringList(QLatin1String("hallo")));
    QVector<QString> vec;
    vec.push_back("Hallo");
    vec.push_back("Hallo2");
    std::set<std::string> stdSet;
    stdSet.insert("s1");
#    ifdef QT_GUI_LIB
    QWidget *ww = 0; //this;
    QWidget &wwr = *ww;
    Q_UNUSED(wwr);
#    endif

    QSharedPointer<QString> sps(new QString("hallo"));
    QList<QSharedPointer<QString> > spsl;
    spsl.push_back(sps);
    QMap<QString,QString> stringmap;
    QMap<int,int> intmap;
    std::map<std::string, std::string> stdstringmap;
    stdstringmap[std::string("A")]  = std::string("B");
    int xxx = 45;

    if (1 == 1) {
        int xxx = 7;
        qDebug() << xxx;
    }

    QLinkedList<QString> lls;
    lls << "link1" << "link2";
#    ifdef QT_GUI_LIB
    QStandardItemModel *model = new QStandardItemModel;
    model->appendRow(new QStandardItem("i1"));
#    endif

    QList <QList<int> > nestedIntList;
    nestedIntList << QList<int>();
    nestedIntList.front() << 1 << 2;

    QVariantList vList;
    vList.push_back(QVariant(42));
    vList.push_back(QVariant("HALLO"));


    stringmap.insert("A", "B");
    intmap.insert(3,4);
    QSet<QString> stringSet;
    stringSet.insert("S1");
    stringSet.insert("S2");
    qDebug() << *(spsl.front()) << xxx;
#endif
}


void tst_Gdb::dump_misc()
{
    prepare("dump_misc");
    next();
    check("B","{iname='local.s',name='s',type='int *',"
            "value='-',numchild='1'}", "", 0);
    //check("B","{iname='local.s',name='s',type='int *',"
    //        "value='1',numchild='0'}", "local.s,local.model", 0);
    check("B","{iname='local.s',name='s',type='int *',"
            "value='-',numchild='1',children=["
            "{name='*',type='int',value='1',numchild='0'}]}",
            "local.s,local.model", 0);
}


///////////////////////////// typedef  ////////////////////////////////////

void dump_typedef()
{
    /* A */ typedef QMap<uint, double> T;
    /* B */ T t;
    /* C */ t[11] = 13.0;
    /* D */ (void) 0;
}

void tst_Gdb::dump_typedef()
{
    prepare("dump_typedef");
    next(2);
    check("D","{iname='local.t',name='t',type='T',"
            //"basetype='" NS "QMap<unsigned int, double>',"
            "value='-',numchild='1',"
            "childtype='double',childnumchild='0',"
            "children=[{name='11',value='13'}]}", "local.t");
}

#if 0
void tst_Gdb::dump_QAbstractItemHelper(QModelIndex &index)
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
        append("',numchild='1',children=[");
    int rowCount = model->rowCount(index);
    int columnCount = model->columnCount(index);
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < columnCount; ++col) {
            const QModelIndex &childIndex = model->index(row, col, index);
            expected.append("{name='[").append(valToString(row)).append(",").
                append(N(col)).append("]',numchild='1',addr='$").
                append(N(childIndex.row())).append(",").
                append(N(childIndex.column())).append(",").
                append(ptrToBa(childIndex.internalPointer())).append(",").
                append(modelPtrStr).append("',type='" NS "QAbstractItem',value='").
                append(valToString(model->data(childIndex).toString())).append("'}");
            if (col < columnCount - 1 || row < rowCount - 1)
                expected.append(",");
        }
    }
    expected.append("]");
    testDumper(expected, &index, NS"QAbstractItem", true, indexSpecValue);
}
#endif

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



///////////////////////////// QAbstractItemAndModelIndex //////////////////////////

//    /* A */ QStringListModel m(QStringList() << "item1" << "item2" << "item3");
//    /* B */ index = m.index(2, 0);
void dump_QAbstractItemAndModelIndex()
{
    /* A */ PseudoTreeItemModel m2; QModelIndex index;
    /* C */ index = m2.index(0, 0);
    /* D */ index = m2.index(1, 0);
    /* E */ index = m2.index(0, 0, index);
    /* F */ (void) index.row();
}

void tst_Gdb::dump_QAbstractItemAndModelIndex()
{
    prepare("dump_QAbstractItemAndModelIndex");
    if (checkUninitialized)
        check("A", "");
    next();
    check("C", "{iname='local.m2',name='m2',type='PseudoTreeItemModel',"
            "value='{...}',numchild='6'},"
        "{iname='local.index',name='index',type='" NS "QModelIndex',"
            "value='(invalid)',numchild='0'}",
        "local.index");
    next();
    check("D", "{iname='local.m2',name='m2',type='PseudoTreeItemModel',"
           "value='{...}',numchild='6'},"
        "{iname='local.index',name='index',type='" NS "QModelIndex',"
          "value='(0, 0)',numchild='5',children=["
            "{name='row',value='0',type='int',numchild='0'},"
            "{name='column',value='0',type='int',numchild='0'},"
            "{name='parent',type='" NS "QModelIndex',value='(invalid)',numchild='0'},"
            "{name='model',value='-',type='" NS "QAbstractItemModel*',numchild='1'}]}",
        "local.index");
    next();
    check("E", "{iname='local.m2',name='m2',type='PseudoTreeItemModel',"
           "value='{...}',numchild='6'},"
        "{iname='local.index',name='index',type='" NS "QModelIndex',"
          "value='(1, 0)',numchild='5',children=["
            "{name='row',value='1',type='int',numchild='0'},"
            "{name='column',value='0',type='int',numchild='0'},"
            "{name='parent',type='" NS "QModelIndex',value='(invalid)',numchild='0'},"
            "{name='model',value='-',type='" NS "QAbstractItemModel*',numchild='1'}]}",
        "local.index");
    next();
    check("F", "{iname='local.m2',name='m2',type='PseudoTreeItemModel',"
           "value='{...}',numchild='6'},"
        "{iname='local.index',name='index',type='" NS "QModelIndex',"
          "value='(0, 0)',numchild='5',children=["
            "{name='row',value='0',type='int',numchild='0'},"
            "{name='column',value='0',type='int',numchild='0'},"
            "{name='parent',type='" NS "QModelIndex',value='(1, 0)',numchild='5'},"
            "{name='model',value='-',type='" NS "QAbstractItemModel*',numchild='1'}]}",
        "local.index");
}

/*
QByteArray dump_QAbstractItemModelHelper(QAbstractItemModel &m)
{
    QByteArray address = ptrToBa(&m);
    QByteArray expected = QByteArray("tiname='iname',"
        "type='" NS "QAbstractItemModel',value='(%,%)',numchild='1',children=["
        "{numchild='1',name='" NS "QObject',value='%',"
        "valueencoded='2',type='" NS "QObject',displayedtype='%'}")
            << address
            << N(m.rowCount())
            << N(m.columnCount())
            << address
            << utfToHex(m.objectName())
            << m.metaObject()->className();

    for (int row = 0; row < m.rowCount(); ++row) {
        for (int column = 0; column < m.columnCount(); ++column) {
            QModelIndex mi = m.index(row, column);
            expected.append(QByteArray(",{name='[%,%]',value='%',"
                "valueencoded='2',numchild='1',%,%,%',"
                "type='" NS "QAbstractItem'}")
                << N(row)
                << N(column)
                << utfToHex(m.data(mi).toString())
                << N(mi.row())
                << N(mi.column())
                << ptrToBa(mi.internalPointer())
                << ptrToBa(mi.model()));
        }
    }
    expected.append("]");
    return expected;
}
*/

void dump_QAbstractItemModel()
{
#    ifdef QT_GUI_LIB
    /* A */ QStringList strList;
            strList << "String 1";
            QStringListModel model1(strList);
            QStandardItemModel model2(0, 2);
    /* B */ model1.setStringList(strList);
    /* C */ strList << "String 2";
    /* D */ model1.setStringList(strList);
    /* E */ model2.appendRow(QList<QStandardItem *>() << (new QStandardItem("Item (0,0)")) << (new QStandardItem("Item (0,1)")));
    /* F */ model2.appendRow(QList<QStandardItem *>() << (new QStandardItem("Item (1,0)")) << (new QStandardItem("Item (1,1)")));
    /* G */ (void) (model1.rowCount() + model2.rowCount() + strList.size());
#    endif
}

void tst_Gdb::dump_QAbstractItemModel()
{
#    ifdef QT_GUI_LIB
    /* A */ QStringList strList;
    QString template_ =
        "{iname='local.strList',name='strList',type='" NS "QStringList',"
            "value='<%1 items>',numchild='%1'},"
        "{iname='local.model1',name='model1',type='" NS "QStringListModel',"
            "value='{...}',numchild='3'},"
        "{iname='local.model2',name='model2',type='" NS "QStandardItemModel',"
            "value='{...}',numchild='2'}";

    prepare("dump_QAbstractItemModel");
    if (checkUninitialized)
        check("A", template_.arg("1").toAscii());
    next(4);
    check("B", template_.arg("1").toAscii());
#    endif
}


///////////////////////////// QByteArray /////////////////////////////////

void dump_QByteArray()
{
    /* A */ QByteArray ba;                       // Empty object.
    /* B */ ba.append('a');                      // One element.
    /* C */ ba.append('b');                      // Two elements.
    /* D */ ba = QByteArray(101, 'a');           // > 100 elements.
    /* E */ ba = QByteArray("abc\a\n\r\033\'\"?"); // Mixed.
    /* F */ (void) 0;
}

void tst_Gdb::dump_QByteArray()
{
    prepare("dump_QByteArray");
    if (checkUninitialized)
        check("A","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "valueencoded='6',value='',numchild='0'}");
    next();
    check("C","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "valueencoded='6',value='61',numchild='1'}");
    next();
    check("D","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "valueencoded='6',value='6162',numchild='2'}");
    next();
    check("E","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "valueencoded='6',value='616161616161616161616161616161616161"
            "616161616161616161616161616161616161616161616161616161616161"
            "616161616161616161616161616161616161616161616161616161616161"
            "6161616161616161616161616161616161616161616161',numchild='101'}");
    next();
    check("F","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "valueencoded='6',value='616263070a0d1b27223f',numchild='10'}");
    check("F","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "valueencoded='6',value='616263070a0d1b27223f',numchild='10',"
            "childtype='char',childnumchild='0',"
            "children=[{value='97 'a''},{value='98 'b''},"
            "{value='99 'c''},{value='7 '\\\\a''},"
            "{value='10 '\\\\n''},{value='13 '\\\\r''},"
            "{value='27 '\\\\033''},{value='39 '\\\\'''},"
            "{value='34 ''''},{value='63 '?''}]}",
            "local.ba");
}


///////////////////////////// QChar /////////////////////////////////

void dump_QChar()
{
    /* A */ QChar c('X');               // Printable ASCII character.
    /* B */ c = QChar(0x600);           // Printable non-ASCII character.
    /* C */ c = QChar::fromAscii('\a'); // Non-printable ASCII character.
    /* D */ c = QChar(0x9f);            // Non-printable non-ASCII character.
    /* E */ c = QChar::fromAscii('?');  // The replacement character.
    /* F */ (void) 0; }

void tst_Gdb::dump_QChar()
{
    prepare("dump_QChar");
    next();

    // Case 1: Printable ASCII character.
    check("B","{iname='local.c',name='c',type='" NS "QChar',"
        "value=''X' (88)',numchild='0'}");
    next();

    // Case 2: Printable non-ASCII character.
    check("C","{iname='local.c',name='c',type='" NS "QChar',"
        "value=''?' (1536)',numchild='0'}");
    next();

    // Case 3: Non-printable ASCII character.
    check("D","{iname='local.c',name='c',type='" NS "QChar',"
        "value=''?' (7)',numchild='0'}");
    next();

    // Case 4: Non-printable non-ASCII character.
    check("E","{iname='local.c',name='c',type='" NS "QChar',"
        "value=''?' (159)',numchild='0'}");
    next();

    // Case 5: Printable ASCII Character that looks like the replacement character.
    check("F","{iname='local.c',name='c',type='" NS "QChar',"
        "value=''?' (63)',numchild='0'}");
}


///////////////////////////// QDateTime /////////////////////////////////

void dump_QDateTime()
{
#    ifndef QT_NO_DATESTRING
    /* A */ QDateTime d;
    /* B */ d = QDateTime::fromString("M5d21y7110:31:02", "'M'M'd'd'y'yyhh:mm:ss");
    /* C */ (void) d.isNull();
#    endif
}

void tst_Gdb::dump_QDateTime()
{
#    ifndef QT_NO_DATESTRING
    prepare("dump_QDateTime");
    if (checkUninitialized)
        check("A","{iname='local.d',name='d',"
            "type='" NS "QDateTime',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B", "{iname='local.d',name='d',type='" NS "QDateTime',"
          "valueencoded='7',value='-',numchild='3',children=["
            "{name='isNull',type='bool',value='true',numchild='0'},"
            "{name='toTime_t',type='unsigned int',value='4294967295',numchild='0'},"
            "{name='toString',type='myns::QString',valueencoded='7',"
                "value='',numchild='0'},"
            "{name='(ISO)',type='myns::QString',valueencoded='7',"
                "value='',numchild='0'},"
            "{name='(SystemLocale)',type='myns::QString',valueencoded='7',"
                "value='',numchild='0'},"
            "{name='(Locale)',type='myns::QString',valueencoded='7',"
                "value='',numchild='0'},"
            "{name='toUTC',type='myns::QDateTime',valueencoded='7',"
                "value='',numchild='3'},"
            "{name='toLocalTime',type='myns::QDateTime',valueencoded='7',"
                "value='',numchild='3'}"
            "]}",
          "local.d");
    next();
    check("C", "{iname='local.d',name='d',type='" NS "QDateTime',"
          "valueencoded='7',value='-',"
                "numchild='3',children=["
            "{name='isNull',type='bool',value='false',numchild='0'},"
            "{name='toTime_t',type='unsigned int',value='43666262',numchild='0'},"
            "{name='toString',type='myns::QString',valueencoded='7',"
                "value='-',numchild='0'},"
            "{name='(ISO)',type='myns::QString',valueencoded='7',"
                "value='-',numchild='0'},"
            "{name='(SystemLocale)',type='myns::QString',valueencoded='7',"
                "value='-',numchild='0'},"
            "{name='(Locale)',type='myns::QString',valueencoded='7',"
                "value='-',numchild='0'},"
            "{name='toUTC',type='myns::QDateTime',valueencoded='7',"
                "value='-',numchild='3'},"
            "{name='toLocalTime',type='myns::QDateTime',valueencoded='7',"
                "value='-',numchild='3'}"
            "]}",
          "local.d");
#    endif
}


///////////////////////////// QDir /////////////////////////////////

void dump_QDir()
{
    /* A */ QDir dir = QDir::current(); // Case 1: Current working directory.
    /* B */ dir = QDir::root();         // Case 2: Root directory.
    /* C */ (void) dir.absolutePath(); }

void tst_Gdb::dump_QDir()
{
    prepare("dump_QDir");
    if (checkUninitialized)
        check("A","{iname='local.dir',name='dir',"
            "type='" NS "QDir',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B", "{iname='local.dir',name='dir',type='" NS "QDir',"
               "valueencoded='7',value='-',numchild='2',children=["
                  "{name='absolutePath',type='" NS "QString',"
                    "valueencoded='7',value='-',numchild='0'},"
                  "{name='canonicalPath',type='" NS "QString',"
                    "valueencoded='7',value='-',numchild='0'}]}",
                "local.dir");
    next();
    check("C", "{iname='local.dir',name='dir',type='" NS "QDir',"
               "valueencoded='7',value='-',numchild='2',children=["
                  "{name='absolutePath',type='" NS "QString',"
                    "valueencoded='7',value='-',numchild='0'},"
                  "{name='canonicalPath',type='" NS "QString',"
                    "valueencoded='7',value='-',numchild='0'}]}",
                "local.dir");
}


///////////////////////////// QFile /////////////////////////////////

void dump_QFile()
{
    /* A */ QFile file1(""); // Case 1: Empty file name => Does not exist.
    /* B */ QTemporaryFile file2; // Case 2: File that is known to exist.
            file2.open();
    /* C */ QFile file3("jfjfdskjdflsdfjfdls");
    /* D */ (void) (file1.fileName() + file2.fileName() + file3.fileName()); }

void tst_Gdb::dump_QFile()
{
    prepare("dump_QFile");
    next(4);
    check("D", "{iname='local.file1',name='file1',type='" NS "QFile',"
            "valueencoded='7',value='',numchild='2',children=["
                "{name='fileName',type='" NS "QString',"
                    "valueencoded='7',value='',numchild='0'},"
                "{name='exists',type='bool',value='false',numchild='0'}"
            "]},"
        "{iname='local.file2',name='file2',type='" NS "QTemporaryFile',"
            "valueencoded='7',value='-',numchild='2',children=["
                "{name='fileName',type='" NS "QString',"
                    "valueencoded='7',value='-',numchild='0'},"
                "{name='exists',type='bool',value='true',numchild='0'}"
            "]},"
        "{iname='local.file3',name='file3',type='" NS "QFile',"
            "valueencoded='7',value='-',numchild='2',children=["
                "{name='fileName',type='" NS "QString',"
                    "valueencoded='7',value='-',numchild='0'},"
                "{name='exists',type='bool',value='false',numchild='0'}"
            "]}",
        "local.file1,local.file2,local.file3");
}


///////////////////////////// QFileInfo /////////////////////////////////

void dump_QFileInfo()
{
    /* A */ QFileInfo fi(".");
    /* B */ (void) fi.baseName().size();
}

void tst_Gdb::dump_QFileInfo()
{
    QFileInfo fi(".");
    prepare("dump_QFileInfo");
    next();
    check("B", "{iname='local.fi',name='fi',type='" NS "QFileInfo',"
        "valueencoded='7',value='" + utfToHex(fi.filePath()) + "',numchild='3',"
        "childtype='" NS "QString',childnumchild='0',children=["
        "{name='absolutePath'," + specQString(fi.absolutePath()) + "},"
        "{name='absoluteFilePath'," + specQString(fi.absoluteFilePath()) + "},"
        "{name='canonicalPath'," + specQString(fi.canonicalPath()) + "},"
        "{name='canonicalFilePath'," + specQString(fi.canonicalFilePath()) + "},"
        "{name='completeBaseName'," + specQString(fi.completeBaseName()) + "},"
        "{name='completeSuffix'," + specQString(fi.completeSuffix()) + "},"
        "{name='baseName'," + specQString(fi.baseName()) + "},"
#if 0
        "{name='isBundle'," + specBool(fi.isBundle()) + "},"
        "{name='bundleName'," + specQString(fi.bundleName()) + "},"
#endif
        "{name='fileName'," + specQString(fi.fileName()) + "},"
        "{name='filePath'," + specQString(fi.filePath()) + "},"
        //"{name='group'," + specQString(fi.group()) + "},"
        //"{name='owner'," + specQString(fi.owner()) + "},"
        "{name='path'," + specQString(fi.path()) + "},"
        "{name='groupid',type='unsigned int',value='" + N(fi.groupId()) + "'},"
        "{name='ownerid',type='unsigned int',value='" + N(fi.ownerId()) + "'},"
        "{name='permissions',value=' ',type='" NS "QFile::Permissions',numchild='10'},"
        "{name='caching',type='bool',value='true'},"
        "{name='exists',type='bool',value='true'},"
        "{name='isAbsolute',type='bool',value='false'},"
        "{name='isDir',type='bool',value='true'},"
        "{name='isExecutable',type='bool',value='true'},"
        "{name='isFile',type='bool',value='false'},"
        "{name='isHidden',type='bool',value='false'},"
        "{name='isReadable',type='bool',value='true'},"
        "{name='isRelative',type='bool',value='true'},"
        "{name='isRoot',type='bool',value='false'},"
        "{name='isSymLink',type='bool',value='false'},"
        "{name='isWritable',type='bool',value='true'},"
        "{name='created',type='" NS "QDateTime',"
            + specQString(fi.created().toString()) + ",numchild='3'},"
        "{name='lastModified',type='" NS "QDateTime',"
            + specQString(fi.lastModified().toString()) + ",numchild='3'},"
        "{name='lastRead',type='" NS "QDateTime',"
            + specQString(fi.lastRead().toString()) + ",numchild='3'}]}",
        "local.fi");
}


#if 0
void tst_Gdb::dump_QImageDataHelper(QImage &img)
{
    const QByteArray ba(QByteArray::fromRawData((const char*) img.bits(), img.numBytes()));
    QByteArray expected = QByteArray("tiname='$I',addr='$A',type='" NS "QImageData',").
        append("numchild='0',value='<hover here>',valuetooltipencoded='1',").
        append("valuetooltipsize='").append(N(ba.size())).append("',").
        append("valuetooltip='").append(ba.toBase64()).append("'");
    testDumper(expected, &img, NS"QImageData", false);
}

#endif // 0


///////////////////////////// QImage /////////////////////////////////

void dump_QImage()
{
#    ifdef QT_GUI_LIB
    /* A */ QImage image; // Null image.
    /* B */ image = QImage(30, 700, QImage::Format_RGB555); // Normal image.
    /* C */ image = QImage(100, 0, QImage::Format_Invalid); // Invalid image.
    /* D */ (void) image.size();
    /* E */ (void) image.size();
#    endif
}

void tst_Gdb::dump_QImage()
{
#    ifdef QT_GUI_LIB
    prepare("dump_QImage");
    next();
    check("B", "{iname='local.image',name='image',type='" NS "QImage',"
        "value='(null)',numchild='0'}");
    next();
    check("C", "{iname='local.image',name='image',type='" NS "QImage',"
        "value='(30x700)',numchild='0'}");
    next(2);
    // FIXME:
    //check("E", "{iname='local.image',name='image',type='" NS "QImage',"
    //    "value='(100x0)',numchild='0'}");
#    endif
}

#if 0
void tst_Gdb::dump_QImageData()
{
    // Case 1: Null image.
    QImage img;
    dump_QImageDataHelper(img);

    // Case 2: Normal image.
    img = QImage(3, 700, QImage::Format_RGB555);
    dump_QImageDataHelper(img);

    // Case 3: Invalid image.
    img = QImage(100, 0, QImage::Format_Invalid);
    dump_QImageDataHelper(img);
}
#endif

void dump_QLocale()
{
    /* A */ QLocale german(QLocale::German);
            QLocale chinese(QLocale::Chinese);
            QLocale swahili(QLocale::Swahili);
    /* C */ (void) (german.name() + chinese.name() + swahili.name());
}

QByteArray dump_QLocaleHelper(const QLocale &loc)
{
    return "type='" NS "QLocale',valueencoded='7',value='" + utfToHex(loc.name()) +
            "',numchild='8',childtype='" NS "QChar',childnumchild='0',children=["
         "{name='country',type='" NS "QLocale::Country',value='-'},"
         "{name='language',type='" NS "QLocale::Language',value='-'},"
         "{name='measurementSystem',type='" NS "QLocale::MeasurementSystem',"
            "value='-'},"
         "{name='numberOptions',type='" NS "QFlags<myns::QLocale::NumberOption>',"
            "value='-'},"
         "{name='timeFormat_(short)',type='" NS "QString',"
            + specQString(loc.timeFormat(QLocale::ShortFormat)) + "},"
         "{name='timeFormat_(long)',type='" NS "QString',"
            + specQString(loc.timeFormat()) + "},"
         "{name='decimalPoint'," + specQChar(loc.decimalPoint()) + "},"
         "{name='exponential'," + specQChar(loc.exponential()) + "},"
         "{name='percent'," + specQChar(loc.percent()) + "},"
         "{name='zeroDigit'," + specQChar(loc.zeroDigit()) + "},"
         "{name='groupSeparator'," + specQChar(loc.groupSeparator()) + "},"
         "{name='negativeSign'," + specQChar(loc.negativeSign()) + "}]";
}

void tst_Gdb::dump_QLocale()
{
    QLocale german(QLocale::German);
    QLocale chinese(QLocale::Chinese);
    QLocale swahili(QLocale::Swahili);
    prepare("dump_QLocale");
    if (checkUninitialized)
        check("A","{iname='local.english',name='english',"
            "type='" NS "QLocale',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("C", "{iname='local.german',name='german',"
            + dump_QLocaleHelper(german) + "},"
        "{iname='local.chinese',name='chinese',"
            + dump_QLocaleHelper(chinese) + "},"
        "{iname='local.swahili',name='swahili',"
            + dump_QLocaleHelper(swahili) + "}",
        "local.german,local.chinese,local.swahili");
}

///////////////////////////// QHash<int, int> //////////////////////////////

void dump_QHash_int_int()
{
    /* A */ QHash<int, int> h;
    /* B */ h[43] = 44;
    /* C */ h[45] = 46;
    /* D */ (void) 0;
}

void tst_Gdb::dump_QHash_int_int()
{
    // Need to check the following combinations:
    // int-key optimization, small value
    //struct NodeOS { void *next; uint k; uint  v; } nodeOS
    // int-key optimiatzion, large value
    //struct NodeOL { void *next; uint k; void *v; } nodeOL
    // no optimization, small value
    //struct NodeNS +  { void *next; uint h; uint  k; uint  v; } nodeNS
    // no optimization, large value
    //struct NodeNL { void *next; uint h; uint  k; void *v; } nodeNL
    // complex key
    //struct NodeL  { void *next; uint h; void *k; void *v; } nodeL

    prepare("dump_QHash_int_int");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" NS "QHash<int, int>',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("D","{iname='local.h',name='h',"
            "type='" NS "QHash<int, int>',value='<2 items>',numchild='2',"
            "childtype='int',childnumchild='0',children=["
            "{name='43',value='44'},"
            "{name='45',value='46'}]}",
            "local.h");
}

///////////////////////////// QHash<QString, QString> //////////////////////////////

void dump_QHash_QString_QString()
{
    /* A */ QHash<QString, QString> h;
    /* B */ h["hello"] = "world";
    /* C */ h["foo"] = "bar";
    /* D */ (void) 0;
}

void tst_Gdb::dump_QHash_QString_QString()
{
    prepare("dump_QHash_QString_QString");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" NS "QHash<" NS "QString, " NS "QString>',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B","{iname='local.h',name='h',"
            "type='" NS "QHash<" NS "QString, " NS "QString>',value='<0 items>',"
            "numchild='0'}");
    next();
    next();
    check("D","{iname='local.h',name='h',"
            "type='" NS "QHash<" NS "QString, " NS "QString>',value='<2 items>',"
            "numchild='2'}");
    check("D","{iname='local.h',name='h',"
            "type='" NS "QHash<" NS "QString, " NS "QString>',value='<2 items>',"
            "numchild='2',childtype='" NS "QHashNode<" NS "QString, " NS "QString>',"
            "children=["
              "{value=' ',numchild='2',children=[{name='key',type='" NS "QString',"
                    "valueencoded='7',value='66006f006f00',numchild='0'},"
                 "{name='value',type='" NS "QString',"
                    "valueencoded='7',value='620061007200',numchild='0'}]},"
              "{value=' ',numchild='2',children=[{name='key',type='" NS "QString',"
                    "valueencoded='7',value='680065006c006c006f00',numchild='0'},"
                 "{name='value',type='" NS "QString',valueencoded='7',"
                     "value='77006f0072006c006400',numchild='0'}]}"
            "]}",
            "local.h,local.h.0,local.h.1");
}


///////////////////////////// QLinkedList<int> ///////////////////////////////////

void dump_QLinkedList_int()
{
    /* A */ QLinkedList<int> h;
    /* B */ h.append(42);
    /* C */ h.append(44);
    /* D */ (void) 0; }

void tst_Gdb::dump_QLinkedList_int()
{
    prepare("dump_QLinkedList_int");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" NS "QLinkedList<int>',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("D","{iname='local.h',name='h',"
            "type='" NS "QLinkedList<int>',value='<2 items>',numchild='2',"
            "childtype='int',childnumchild='0',children=["
            "{value='42'},{value='44'}]}", "local.h");
}


///////////////////////////// QList<int> /////////////////////////////////

void dump_QList_int()
{
    /* A */ QList<int> list;
    /* B */ list.append(1);
    /* C */ list.append(2);
    /* D */ (void) 0;
}

void tst_Gdb::dump_QList_int()
{
    prepare("dump_QList_int");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
                "type='" NS "QList<int>',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.list',name='list',"
            "type='" NS "QList<int>',value='<0 items>',numchild='0'}");
    next();
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<int>',value='<1 items>',numchild='1'}");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<int>',value='<1 items>',numchild='1',"
            "childtype='int',childnumchild='0',children=["
            "{value='1'}]}", "local.list");
    next();
    check("D","{iname='local.list',name='list',"
            "type='" NS "QList<int>',value='<2 items>',numchild='2'}");
    check("D","{iname='local.list',name='list',"
            "type='" NS "QList<int>',value='<2 items>',numchild='2',"
            "childtype='int',childnumchild='0',children=["
            "{value='1'},{value='2'}]}", "local.list");
}


///////////////////////////// QList<int *> /////////////////////////////////

void dump_QList_int_star()
{
    /* A */ QList<int *> list;
    /* B */ list.append(new int(1));
    /* C */ list.append(0);
    /* D */ list.append(new int(2));
    /* E */ (void) 0;
}

void tst_Gdb::dump_QList_int_star()
{
    prepare("dump_QList_int_star");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
                "type='" NS "QList<int*>',value='<invalid>',numchild='0'}");
    next();
    next();
    next();
    next();
    check("E","{iname='local.list',name='list',"
            "type='" NS "QList<int*>',value='<3 items>',numchild='3',"
            "childtype='int',childnumchild='0',children=["
            "{value='1'},{value='0x0',type='int *'},{value='2'}]}", "local.list");
}


///////////////////////////// QList<char> /////////////////////////////////

void dump_QList_char()
{
    /* A */ QList<char> list;
    /* B */ list.append('a');
    /* C */ (void) 0;
}

void tst_Gdb::dump_QList_char()
{
    prepare("dump_QList_char");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
            "type='" NS "QList<char>',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.list',name='list',"
            "type='" NS "QList<char>',value='<0 items>',numchild='0'}");
    next();
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<char>',value='<1 items>',numchild='1'}");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<char>',value='<1 items>',numchild='1',"
            "childtype='char',childnumchild='0',children=["
            "{value='97 'a''}]}", "local.list");
}


///////////////////////////// QList<const char *> /////////////////////////////////

void dump_QList_char_star()
{
    /* A */ QList<const char *> list;
    /* B */ list.append("a");
    /* C */ list.append(0);
    /* D */ list.append("bc");
    /* E */ (void) 0;
}

void tst_Gdb::dump_QList_char_star()
{
    prepare("dump_QList_char_star");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
            "type='" NS "QList<char const*>',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.list',name='list',"
            "type='" NS "QList<char const*>',value='<0 items>',numchild='0'}");
    next();
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<char const*>',value='<1 items>',numchild='1'}");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<char const*>',value='<1 items>',numchild='1',"
            "childtype='const char *',childnumchild='1',children=["
            "{valueencoded='6',value='61',numchild='0'}]}", "local.list");
    next();
    next();
    check("E","{iname='local.list',name='list',"
            "type='" NS "QList<char const*>',value='<3 items>',numchild='3',"
            "childtype='const char *',childnumchild='1',children=["
            "{valueencoded='6',value='61',numchild='0'},"
            "{value='0x0',numchild='0'},"
            "{valueencoded='6',value='6263',numchild='0'}]}", "local.list");
}


///////////////////////////// QList<QString> /////////////////////////////////////

void dump_QList_QString()
{
    /* A */ QList<QString> list;
    /* B */ list.append("Hallo");
    /* C */ (void) 0;
}

void tst_Gdb::dump_QList_QString()
{
    prepare("dump_QList_QString");
    if (0 && checkUninitialized)
        check("A","{iname='local.list',name='list',"
            "type='" NS "QList<" NS "QString>',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.list',name='list',"
            "type='" NS "QList<" NS "QString>',value='<0 items>',numchild='0'}");
    next();
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<" NS "QString>',value='<1 items>',numchild='1'}");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<" NS "QString>',value='<1 items>',numchild='1',"
            "childtype='" NS "QString',childnumchild='0',children=["
            "{valueencoded='7',value='480061006c006c006f00'}]}", "local.list");
}


///////////////////////////// QList<QString3> ///////////////////////////////////

void dump_QList_QString3()
{
    /* A */ QList<QString3> list;
    /* B */ list.append(QString3());
    /* C */ (void) 0;
}

void tst_Gdb::dump_QList_QString3()
{
    prepare("dump_QList_QString3");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
            "type='" NS "QList<QString3>',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.list',name='list',"
            "type='" NS "QList<QString3>',value='<0 items>',numchild='0'}");
    next();
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<QString3>',value='<1 items>',numchild='1'}");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<QString3>',value='<1 items>',numchild='1',"
            "childtype='QString3',children=["
            "{value='{...}',numchild='3'}]}", "local.list");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<QString3>',value='<1 items>',numchild='1',"
            "childtype='QString3',children=[{value='{...}',numchild='3',children=["
                 "{name='s1',type='" NS "QString',"
                    "valueencoded='7',value='6100',numchild='0'},"
                 "{name='s2',type='" NS "QString',"
                    "valueencoded='7',value='6200',numchild='0'},"
                 "{name='s3',type='" NS "QString',"
                    "valueencoded='7',value='6300',numchild='0'}]}]}",
            "local.list,local.list.0");
}


///////////////////////////// QList<Int3> /////////////////////////////////////

void dump_QList_Int3()
{
    /* A */ QList<Int3> list;
    /* B */ list.append(Int3());
    /* C */ (void) 0;
}

void tst_Gdb::dump_QList_Int3()
{
    prepare("dump_QList_Int3");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
                "type='" NS "QList<Int3>',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.list',name='list',"
            "type='" NS "QList<Int3>',value='<0 items>',numchild='0'}");
    next();
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<Int3>',value='<1 items>',numchild='1'}");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<Int3>',value='<1 items>',numchild='1',"
            "childtype='Int3',children=[{value='{...}',numchild='3'}]}",
            "local.list");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<Int3>',value='<1 items>',numchild='1',"
            "childtype='Int3',children=[{value='{...}',numchild='3',children=["
             "{name='i1',type='int',value='42',numchild='0'},"
             "{name='i2',type='int',value='43',numchild='0'},"
             "{name='i3',type='int',value='44',numchild='0'}]}]}",
            "local.list,local.list.0");
}


///////////////////////////// QMap<int, int> //////////////////////////////

void dump_QMap_int_int()
{
    /* A */ QMap<int, int> h;
    /* B */ h[12] = 34;
    /* C */ h[14] = 54;
    /* D */ (void) 0;
}

void tst_Gdb::dump_QMap_int_int()
{
    prepare("dump_QMap_int_int");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" NS "QMap<int, int>',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B","{iname='local.h',name='h',"
            "type='" NS "QMap<int, int>',value='<0 items>',"
            "numchild='0'}");
    next();
    next();
    check("D","{iname='local.h',name='h',"
            "type='" NS "QMap<int, int>',value='<2 items>',"
            "numchild='2'}");
    check("D","{iname='local.h',name='h',"
            "type='" NS "QMap<int, int>',value='<2 items>',"
            "numchild='2',childtype='int',childnumchild='0',"
            "children=[{name='12',value='34'},{name='14',value='54'}]}",
            "local.h,local.h.0,local.h.1");
}


///////////////////////////// QMap<QString, QString> //////////////////////////////

void dump_QMap_QString_QString()
{
    /* A */ QMap<QString, QString> m;
    /* B */ m["hello"] = "world";
    /* C */ m["foo"] = "bar";
    /* D */ (void) 0;
}

void tst_Gdb::dump_QMap_QString_QString()
{
    prepare("dump_QMap_QString_QString");
    if (checkUninitialized)
        check("A","{iname='local.m',name='m',"
            "type='" NS "QMap<" NS "QString, " NS "QString>',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B","{iname='local.m',name='m',"
            "type='" NS "QMap<" NS "QString, " NS "QString>',value='<0 items>',"
            "numchild='0'}");
    next();
    next();
    check("D","{iname='local.m',name='m',"
            "type='" NS "QMap<" NS "QString, " NS "QString>',value='<2 items>',"
            "numchild='2'}");
    check("D","{iname='local.m',name='m',"
            "type='" NS "QMap<" NS "QString, " NS "QString>',value='<2 items>',"
            "numchild='2',childtype='" NS "QMapNode<" NS "QString, " NS "QString>',"
            "children=["
              "{value=' ',numchild='2',children=[{name='key',type='" NS "QString',"
                    "valueencoded='7',value='66006f006f00',numchild='0'},"
                 "{name='value',type='" NS "QString',"
                    "valueencoded='7',value='620061007200',numchild='0'}]},"
              "{value=' ',numchild='2',children=[{name='key',type='" NS "QString',"
                    "valueencoded='7',value='680065006c006c006f00',numchild='0'},"
                 "{name='value',type='" NS "QString',valueencoded='7',"
                     "value='77006f0072006c006400',numchild='0'}]}"
            "]}",
            "local.m,local.m.0,local.m.1");
}


///////////////////////////// QObject /////////////////////////////////

void dump_QObject()
{
    /* B */ QObject ob;
    /* D */ ob.setObjectName("An Object");
    /* E */ QObject::connect(&ob, SIGNAL(destroyed()), qApp, SLOT(quit()));
    /* F */ QObject::disconnect(&ob, SIGNAL(destroyed()), qApp, SLOT(quit()));
    /* G */ ob.setObjectName("A renamed Object");
    /* H */ (void) 0; }

void dump_QObject1()
{
    /* A */ QObject parent;
    /* B */ QObject child(&parent);
    /* C */ parent.setObjectName("A Parent");
    /* D */ child.setObjectName("A Child");
    /* H */ (void) 0; }

void tst_Gdb::dump_QObject()
{
    prepare("dump_QObject");
    if (checkUninitialized)
        check("A","{iname='local.ob',name='ob',"
            "type='" NS "QObject',value='<invalid>',"
            "numchild='0'}");
    next(4);

    check("G","{iname='local.ob',name='ob',type='" NS "QObject',valueencoded='7',"
      "value='41006e0020004f0062006a00650063007400',numchild='4',children=["
        "{name='parent',value='0x0',type='" NS "QObject *',"
            "numchild='0'},"
        "{name='children',type='-'," // NS"QObject{Data,}::QObjectList',"
            "value='<0 items>',numchild='0',children=[]},"
        "{name='properties',value='<1 items>',type=' ',numchild='1',children=["
            "{name='objectName',type='" NS "QString',valueencoded='7',"
                "value='41006e0020004f0062006a00650063007400',numchild='0'}]},"
        "{name='connections',value='<0 items>',type=' ',numchild='0',"
            "children=[]},"
        "{name='signals',value='<2 items>',type=' ',numchild='2',"
            "childnumchild='0',children=["
            "{name='signal 0',type=' ',value='destroyed(QObject*)'},"
            "{name='signal 1',type=' ',value='destroyed()'}]},"
        "{name='slots',value='<2 items>',type=' ',numchild='2',"
            "childnumchild='0',children=["
            "{name='slot 0',type=' ',value='deleteLater()'},"
            "{name='slot 1',type=' ',value='_q_reregisterTimers(void*)'}]}]}",
        "local.ob"
        ",local.ob.children"
        ",local.ob.properties"
        ",local.ob.connections"
        ",local.ob.signals"
        ",local.ob.slots"
    );
#if 0
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
#endif
}

#if 0
void tst_Gdb::dump_QObjectChildListHelper(QObject &o)
{
    const QObjectList children = o.children();
    const int size = children.size();
    const QString sizeStr = N(size);
    QByteArray expected = QByteArray("numchild='").append(sizeStr).append("',value='<").
        append(sizeStr).append(" items>',type='" NS "QObjectChildList',children=[");
    for (int i = 0; i < size; ++i) {
        const QObject *child = children.at(i);
        expected.append("{addr='").append(ptrToBa(child)).append("',value='").
            append(utfToHex(child->objectName())).
            append("',valueencoded='2',type='" NS "QObject',displayedtype='").
            append(child->metaObject()->className()).append("',numchild='1'}");
        if (i < size - 1)
            expected.append(",");
    }
    expected.append("]");
    testDumper(expected, &o, NS"QObjectChildList", true);
}

void tst_Gdb::dump_QObjectChildList()
{
    // Case 1: Object with no children.
    QObject o;
    dump_QObjectChildListHelper(o);

    // Case 2: Object with one child.
    QObject o2(&o);
    dump_QObjectChildListHelper(o);

    // Case 3: Object with two children.
    QObject o3(&o);
    dump_QObjectChildListHelper(o);
}

void tst_Gdb::dump_QObjectMethodList()
{
    QStringListModel m;
    testDumper("addr='<synthetic>',type='$T',numchild='20',"
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
        "{name='18 18 submit()',value='<Slot> (2)'},"
        "{name='19 19 revert()',value='<Slot> (2)'}]",
    &m, NS"QObjectMethodList", true);
}

void tst_Gdb::dump_QObjectPropertyList()
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

void tst_Gdb::dump_QObjectSignalHelper(QObject &o, int sigNum)
{
    //qDebug() << o.objectName() << sigNum;
    QByteArray expected("addr='<synthetic>',numchild='1',type='" NS "QObjectSignal'");
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
                append(utfToHex(conn->receiver->objectName())).append("',valueencoded='2',").
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

void tst_Gdb::dump_QObjectSignal()
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
        dump_QObjectSignalHelper(m, signalIndex);

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
        dump_QObjectSignalHelper(m, signalIndex);
}

void tst_Gdb::dump_QObjectSignalList()
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
    QByteArray expected = "type='" NS "QObjectSignalList',value='<16 items>',"
        "addr='$A',numchild='16',children=["
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

void tst_Gdb::dump_QObjectSlot()
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

void tst_Gdb::dump_QObjectSlotList()
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
        "{name='18',value='submit()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='19',value='revert()',numchild='0',"
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
        "{name='18',value='submit()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='19',value='revert()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'}]",
        &m, NS"QObjectSlotList", true);
}


#endif // #if 0

///////////////////////////// QPixmap /////////////////////////////////

const char * const pixmap[] = {
    "2 24 3 1", "       c None", ".      c #DBD3CB", "+      c #FCFBFA",
    "  ", "  ", "  ", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+",
    ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", "  ", "  ", "  "
};

void dump_QPixmap()
{
#    ifdef QT_GUI_LIB
    /* A */ QPixmap p; // Case 1: Null Pixmap.
    /* B */ p = QPixmap(20, 100); // Case 2: Uninitialized non-null pixmap.
    /* C */ p = QPixmap(pixmap); // Case 3: Initialized non-null pixmap.
    /* D */ (void) p.size();
#    endif
}

void tst_Gdb::dump_QPixmap()
{
#    ifdef QT_GUI_LIB
    prepare("dump_QPixmap");
    next();
    check("B", "{iname='local.p',name='p',type='" NS "QPixmap',"
        "value='(null)',numchild='0'}");
    next();
    check("C", "{iname='local.p',name='p',type='" NS "QPixmap',"
        "value='(20x100)',numchild='0'}");
    next();
    check("D", "{iname='local.p',name='p',type='" NS "QPixmap',"
        "value='(2x24)',numchild='0'}");
#    endif
}


///////////////////////////// QPoint /////////////////////////////////

void dump_QPoint()
{
    /* A */ QPoint p(43, 44);
    /* B */ QPointF f(45, 46);
    /* C */ (void) (p.x() + f.x()); }

void tst_Gdb::dump_QPoint()
{
    prepare("dump_QPoint");
    next();
    next();
    check("C","{iname='local.p',name='p',type='" NS "QPoint',"
        "value='(43, 44)',numchild='2',childtype='int',childnumchild='0',"
            "children=[{name='x',value='43'},{name='y',value='44'}]},"
        "{iname='local.f',name='f',type='" NS "QPointF',"
        "value='(45, 46)',numchild='2',childtype='double',childnumchild='0',"
            "children=[{name='x',value='45'},{name='y',value='46'}]}",
        "local.p,local.f");
}


///////////////////////////// QRect /////////////////////////////////

void dump_QRect()
{
    /* A */ QRect p(43, 44, 100, 200);
    /* B */ QRectF f(45, 46, 100, 200);
    /* C */ (void) (p.x() + f.x()); }

void tst_Gdb::dump_QRect()
{
    prepare("dump_QRect");
    next();
    next();

    check("C","{iname='local.p',name='p',type='" NS "QRect',"
        "value='100x200+43+44',numchild='4',childtype='int',childnumchild='0',"
            "children=[{name='x1',value='43'},{name='y1',value='44'},"
            "{name='x2',value='142'},{name='y2',value='243'}]},"
        "{iname='local.f',name='f',type='" NS "QRectF',"
        "value='100x200+45+46',numchild='4',childtype='double',childnumchild='0',"
            "children=[{name='x',value='45'},{name='y',value='46'},"
            "{name='w',value='100'},{name='h',value='200'}]}",
        "local.p,local.f");
}


///////////////////////////// QSet<int> ///////////////////////////////////

void dump_QSet_int()
{
    /* A */ QSet<int> h;
    /* B */ h.insert(42);
    /* C */ h.insert(44);
    /* D */ (void) 0;
}

void tst_Gdb::dump_QSet_int()
{
    prepare("dump_QSet_int");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" NS "QSet<int>',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("D","{iname='local.h',name='h',"
            "type='" NS "QSet<int>',value='<2 items>',numchild='2',"
            "childtype='int',childnumchild='0',children=["
            "{value='42'},{value='44'}]}", "local.h");
}


///////////////////////////// QSet<Int3> ///////////////////////////////////

void dump_QSet_Int3()
{
    /* A */ QSet<Int3> h;
    /* B */ h.insert(Int3(42));
    /* C */ h.insert(Int3(44));
    /* D */ (void) 0;
}

void tst_Gdb::dump_QSet_Int3()
{
    prepare("dump_QSet_Int3");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" NS "QSet<Int3>',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("D","{iname='local.h',name='h',"
            "type='" NS "QSet<Int3>',value='<2 items>',numchild='2',"
            "childtype='Int3',children=["
            "{value='{...}',numchild='3'},{value='{...}',numchild='3'}]}", "local.h");
}


///////////////////////////// QSize /////////////////////////////////

#if QT_VERSION >= 0x040500
void dump_QSharedPointer()
{
    /* A */ // Case 1: Simple type.
            // Case 1.1: Null pointer.
            QSharedPointer<int> simplePtr;
            // Case 1.2: Non-null pointer,
            QSharedPointer<int> simplePtr2(new int(99));
            // Case 1.3: Shared pointer.
            QSharedPointer<int> simplePtr3 = simplePtr2;
            // Case 1.4: Weak pointer.
            QWeakPointer<int> simplePtr4(simplePtr2);

            // Case 2: Composite type.
            // Case 2.1: Null pointer.
            QSharedPointer<QString> compositePtr;
            // Case 2.2: Non-null pointer,
            QSharedPointer<QString> compositePtr2(new QString("Test"));
            // Case 2.3: Shared pointer.
            QSharedPointer<QString> compositePtr3 = compositePtr2;
            // Case 2.4: Weak pointer.
            QWeakPointer<QString> compositePtr4(compositePtr2);
    /* C */ (void) simplePtr.data();
}

#endif

void tst_Gdb::dump_QSharedPointer()
{
#if QT_VERSION >= 0x040500
    prepare("dump_QSharedPointer");
    if (checkUninitialized)
        check("A","{iname='local.simplePtr',name='simplePtr',"
            "type='" NS "QSharedPointer<int>',value='<invalid>',numchild='0'},"
        "{iname='local.simplePtr2',name='simplePtr2',"
            "type='" NS "QSharedPointer<int>',value='<invalid>',numchild='0'},"
        "{iname='local.simplePtr3',name='simplePtr3',"
            "type='" NS "QSharedPointer<int>',value='<invalid>',numchild='0'},"
        "{iname='local.simplePtr4',name='simplePtr4',"
            "type='" NS "QWeakPointer<int>',value='<invalid>',numchild='0'},"
        "{iname='local.compositePtr',name='compositePtr',"
            "type='" NS "QSharedPointer<" NS "QString>',value='<invalid>',numchild='0'},"
        "{iname='local.compositePtr2',name='compositePtr2',"
            "type='" NS "QSharedPointer<" NS "QString>',value='<invalid>',numchild='0'},"
        "{iname='local.compositePtr3',name='compositePtr3',"
            "type='" NS "QSharedPointer<" NS "QString>',value='<invalid>',numchild='0'},"
        "{iname='local.compositePtr4',name='compositePtr4',"
            "type='" NS "QWeakPointer<" NS "QString>',value='<invalid>',numchild='0'}");

    next(8);
    check("C","{iname='local.simplePtr',name='simplePtr',"
            "type='" NS "QSharedPointer<int>',value='(null)',numchild='0'},"
        "{iname='local.simplePtr2',name='simplePtr2',"
            "type='" NS "QSharedPointer<int>',value='',numchild='3'},"
        "{iname='local.simplePtr3',name='simplePtr3',"
            "type='" NS "QSharedPointer<int>',value='',numchild='3'},"
        "{iname='local.simplePtr4',name='simplePtr4',"
            "type='" NS "QWeakPointer<int>',value='',numchild='3'},"
        "{iname='local.compositePtr',name='compositePtr',"
            "type='" NS "QSharedPointer<" NS "QString>',value='(null)',numchild='0'},"
        "{iname='local.compositePtr2',name='compositePtr2',"
            "type='" NS "QSharedPointer<" NS "QString>',value='',numchild='3'},"
        "{iname='local.compositePtr3',name='compositePtr3',"
            "type='" NS "QSharedPointer<" NS "QString>',value='',numchild='3'},"
        "{iname='local.compositePtr4',name='compositePtr4',"
            "type='" NS "QWeakPointer<" NS "QString>',value='',numchild='3'}");

    check("C","{iname='local.simplePtr',name='simplePtr',"
        "type='" NS "QSharedPointer<int>',value='(null)',numchild='0'},"
            "{iname='local.simplePtr2',name='simplePtr2',"
        "type='" NS "QSharedPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='3',type='int',numchild='0'},"
                "{name='strongref',value='2',type='int',numchild='0'}]},"
        "{iname='local.simplePtr3',name='simplePtr3',"
            "type='" NS "QSharedPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='3',type='int',numchild='0'},"
                "{name='strongref',value='2',type='int',numchild='0'}]},"
        "{iname='local.simplePtr4',name='simplePtr4',"
            "type='" NS "QWeakPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='3',type='int',numchild='0'},"
                "{name='strongref',value='2',type='int',numchild='0'}]},"
        "{iname='local.compositePtr',name='compositePtr',"
            "type='" NS "QSharedPointer<" NS "QString>',value='(null)',numchild='0'},"
        "{iname='local.compositePtr2',name='compositePtr2',"
            "type='" NS "QSharedPointer<" NS "QString>',value='',numchild='3'},"
        "{iname='local.compositePtr3',name='compositePtr3',"
            "type='" NS "QSharedPointer<" NS "QString>',value='',numchild='3'},"
        "{iname='local.compositePtr4',name='compositePtr4',"
            "type='" NS "QWeakPointer<" NS "QString>',value='',numchild='3'}",
        "local.simplePtr,local.simplePtr2,local.simplePtr3,local.simplePtr4,"
        "local.compositePtr,local.compositePtr,local.compositePtr,"
        "local.compositePtr");

#endif
}

///////////////////////////// QSize /////////////////////////////////

void dump_QSize()
{
    /* A */ QSize p(43, 44);
//    /* B */ QSizeF f(45, 46);
    /* C */ (void) 0; }

void tst_Gdb::dump_QSize()
{
    prepare("dump_QSize");
    next(1);
// FIXME: Enable child type as soon as 'double' is not reported
// as 'myns::QVariant::Private::Data::qreal' anymore
    check("C","{iname='local.p',name='p',type='" NS "QSize',"
            "value='(43, 44)',numchild='2',childtype='int',childnumchild='0',"
                "children=[{name='w',value='43'},{name='h',value='44'}]}"
//            ",{iname='local.f',name='f',type='" NS "QSizeF',"
//            "value='(45, 46)',numchild='2',childtype='double',childnumchild='0',"
//                "children=[{name='w',value='45'},{name='h',value='46'}]}"
                "",
            "local.p,local.f");
}


///////////////////////////// QStack /////////////////////////////////

void dump_QStack()
{
    /* A */ QStack<int> v;
    /* B */ v.append(3);
    /* C */ v.append(2);
    /* D */ (void) 0;
}

void tst_Gdb::dump_QStack()
{
    prepare("dump_QStack");
    if (checkUninitialized)
        check("A","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<0 items>',numchild='0'}");
    check("B","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<0 items>',numchild='0',children=[]}", "local.v");
    next();
    check("C","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<1 items>',numchild='1'}");
    check("C","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<1 items>',numchild='1',childtype='int',"
            "childnumchild='0',children=[{value='3'}]}",  // rounding...
            "local.v");
    next();
    check("D","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<2 items>',numchild='2'}");
    check("D","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<2 items>',numchild='2',childtype='int',"
            "childnumchild='0',children=[{value='3'},{value='2'}]}",
            "local.v");
}


///////////////////////////// QString /////////////////////////////////////

void dump_QString()
{
    /* A */ QString s;
    /* B */ s = "hallo";
    /* C */ s += "x";
    /* D */ (void) 0; }

void tst_Gdb::dump_QString()
{
    prepare("dump_QString");
    if (checkUninitialized)
        check("A","{iname='local.s',name='s',type='" NS "QString',"
                "value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.s',name='s',type='" NS "QString',"
            "valueencoded='7',value='',numchild='0'}", "local.s");
    // Plain C style dumping:
    check("B","{iname='local.s',name='s',type='" NS "QString',"
            "value='{...}',numchild='5'}", "", 0);
    check("B","{iname='local.s',name='s',type='" NS "QString',"
            "value='{...}',numchild='5',children=["
            "{name='d',type='" NS "QString::Data *',"
            "value='-',numchild='1'}]}", "local.s", 0);
/*
    // FIXME: changed after auto-deref commit
    check("B","{iname='local.s',name='s',type='" NS "QString',"
            "value='{...}',numchild='5',"
            "children=[{name='d',"
              "type='" NS "QString::Data *',value='-',numchild='1',"
                "children=[{iname='local.s.d.*',name='*d',"
                "type='" NS "QString::Data',value='{...}',numchild='11'}]}]}",
            "local.s,local.s.d", 0);
*/
    next();
    check("C","{iname='local.s',name='s',type='" NS "QString',"
            "valueencoded='7',value='680061006c006c006f00',numchild='0'}");
    next();
    check("D","{iname='local.s',name='s',type='" NS "QString',"
            "valueencoded='7',value='680061006c006c006f007800',numchild='0'}");
}


///////////////////////////// QStringList /////////////////////////////////

void dump_QStringList()
{
    /* A */ QStringList s;
    /* B */ s.append("hello");
    /* C */ s.append("world");
    /* D */ (void) 0;
}

void tst_Gdb::dump_QStringList()
{
    prepare("dump_QStringList");
    if (checkUninitialized)
        check("A","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<0 items>',numchild='0'}");
    check("B","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<0 items>',numchild='0',children=[]}", "local.s");
    next();
    check("C","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<1 items>',numchild='1'}");
    check("C","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<1 items>',numchild='1',childtype='" NS "QString',"
            "childnumchild='0',children=[{valueencoded='7',"
            "value='680065006c006c006f00'}]}",
            "local.s");
    next();
    check("D","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<2 items>',numchild='2'}");
    check("D","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<2 items>',numchild='2',childtype='" NS "QString',"
            "childnumchild='0',children=["
            "{valueencoded='7',value='680065006c006c006f00'},"
            "{valueencoded='7',value='77006f0072006c006400'}]}",
            "local.s");
}


///////////////////////////// QTextCodec /////////////////////////////////

void dump_QTextCodec()
{
    /* A */ QTextCodec *codec = QTextCodec::codecForName("UTF-16");
    /* D */ (void) codec; }

void tst_Gdb::dump_QTextCodec()
{
    // FIXME
    prepare("dump_QTextCodec");
    next();
    check("D", "{iname='local.codec',name='codec',type='" NS "QTextCodec',"
           "valueencoded='6',value='5554462d3136',numchild='2',children=["
            "{name='name',type='" NS "QByteArray',valueencoded='6',"
             "value='5554462d3136',numchild='6'},"
            "{name='mibEnum',type='int',value='1015',numchild='0'}]}",
        "local.codec,local.codec.*");
}


///////////////////////////// QVector /////////////////////////////////

void dump_QVector()
{
    /* A */ QVector<double> v;
    /* B */ v.append(3.14);
    /* C */ v.append(2.81);
    /* D */ (void) 0;
}

void tst_Gdb::dump_QVector()
{
    prepare("dump_QVector");
    if (checkUninitialized)
        check("A","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<0 items>',numchild='0'}");
    check("B","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<0 items>',numchild='0',children=[]}", "local.v");
    next();
    check("C","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<1 items>',numchild='1'}");
    check("C","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<1 items>',numchild='1',childtype='double',"
            "childnumchild='0',children=[{value='-'}]}",  // rounding...
            "local.v");
    next();
    check("D","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<2 items>',numchild='2'}");
    check("D","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<2 items>',numchild='2',childtype='double',"
            "childnumchild='0',children=[{value='-'},{value='-'}]}",
            "local.v");
}


///////////////////////////// QVariant /////////////////////////////////

void dump_QVariant1()
{
    QVariant v(QLatin1String("hallo"));
    (void) v.toInt();
}

#ifdef QT_GUI_LIB
#define GUI(s) s
#else
#define GUI(s) 0
#endif

void dump_QVariant()
{
    /*<invalid>*/ QVariant v;
    /* <invalid>    */ v = QBitArray();
    /* QBitArray    */ v = GUI(QBitmap());
    /* QBitMap      */ v = bool(true);
    /* bool         */ v = GUI(QBrush());
    /* QBrush       */ v = QByteArray("abc");
    /* QByteArray   */ v = QChar(QLatin1Char('x'));
    /* QChar        */ v = GUI(QColor());
    /* QColor       */ v = GUI(QCursor());
    /* QCursor      */ v = QDate();
    /* QDate        */ v = QDateTime();
    /* QDateTime    */ v = double(46);
    /* double       */ v = GUI(QFont());
    /* QFont        */ v = QVariantHash();
    /* QVariantHash */ v = GUI(QIcon());
    /* QIcon        */ v = GUI(QImage(10, 10, QImage::Format_RGB32));
    /* QImage       */ v = int(42);
    /* int          */ v = GUI(QKeySequence());
    /* QKeySequence */ v = QLine();
    /* QLine        */ v = QLineF();
    /* QLineF       */ v = QVariantList();
    /* QVariantList */ v = QLocale();
    /* QLocale      */ v = qlonglong(44);
    /* qlonglong    */ v = QVariantMap();
    /* QVariantMap  */ v = GUI(QTransform());
    /* QTransform   */ v = GUI(QMatrix4x4());
    /* QMatrix4x4   */ v = GUI(QPalette());
    /* QPalette     */ v = GUI(QPen());
    /* QPen         */ v = GUI(QPixmap());
    /* QPixmap      */ v = QPoint(45, 46);
    /* QPoint       */ v = QPointF(41, 42);
    /* QPointF      */ v = GUI(QPolygon());
    /* QPolygon     */ v = GUI(QQuaternion());
    /* QQuaternion  */ v = QRect(1, 2, 3, 4);
    /* QRect        */ v = QRectF(1, 2, 3, 4);
    /* QRectF       */ v = QRegExp("abc");
    /* QRegExp      */ v = GUI(QRegion());
    /* QRegion      */ v = QSize(0, 0);
    /* QSize        */ v = QSizeF(0, 0);
    /* QSizeF       */ v = GUI(QSizePolicy());
    /* QSizePolicy  */ v = QString("abc");
    /* QString      */ v = QStringList() << "abc";
    /* QStringList  */ v = GUI(QTextFormat());
    /* QTextFormat  */ v = GUI(QTextLength());
    /* QTextLength  */ v = QTime();
    /* QTime        */ v = uint(43);
    /* uint         */ v = qulonglong(45);
    /* qulonglong   */ v = QUrl("http://foo");
    /* QUrl         */ v = GUI(QVector2D());
    /* QVector2D    */ v = GUI(QVector3D());
    /* QVector3D    */ v = GUI(QVector4D());
    /* QVector4D    */ (void) 0;
}

void tst_Gdb::dump_QVariant()
{
#    define PRE "iname='local.v',name='v',type='" NS "QVariant',"
    prepare("dump_QVariant");
    if (checkUninitialized) /*<invalid>*/
        check("A","{" PRE "'value=<invalid>',numchild='0'}");
    next();
    check("<invalid>", "{" PRE "value='(invalid)',numchild='0'}");
    next();
    check("QBitArray", "{" PRE "value='(" NS "QBitArray)',numchild='1',children=["
        "{name='data',type='" NS "QBitArray',value='{...}',numchild='1'}]}",
        "local.v");
    next();
    (void)GUI(check("QBitMap", "{" PRE "value='(" NS "QBitmap)',numchild='1',children=["
        "{name='data',type='" NS "QBitmap',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
    check("bool", "{" PRE "value='true',numchild='0'}", "local.v");
    next();
    (void)GUI(check("QBrush", "{" PRE "value='(" NS "QBrush)',numchild='1',children=["
        "{name='data',type='" NS "QBrush',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
    check("QByteArray", "{" PRE "value='(" NS "QByteArray)',numchild='1',"
        "children=[{name='data',type='" NS "QByteArray',valueencoded='6',"
            "value='616263',numchild='3'}]}", "local.v");
    next();
    check("QChar", "{" PRE "value='(" NS "QChar)',numchild='1',"
        "children=[{name='data',type='" NS "QChar',value=''x' (120)',numchild='0'}]}", "local.v");
    next();
    (void)GUI(check("QColor", "{" PRE "value='(" NS "QColor)',numchild='1',children=["
        "{name='data',type='" NS "QColor',value='{...}',numchild='2'}]}",
        "local.v"));
    next();
    (void)GUI(check("QCursor", "{" PRE "value='(" NS "QCursor)',numchild='1',children=["
        "{name='data',type='" NS "QCursor',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
    check("QDate", "{" PRE "value='(" NS "QDate)',numchild='1',children=["
        "{name='data',type='" NS "QDate',value='{...}',numchild='1'}]}", "local.v");
    next();
    check("QDateTime", "{" PRE "value='(" NS "QDateTime)',numchild='1',children=["
        "{name='data',type='" NS "QDateTime',valueencoded='7',value='',numchild='3'}]}", "local.v");
    next();
    check("double", "{" PRE "value='46',numchild='0'}", "local.v");
    next();
    (void)GUI(check("QFont", "{" PRE "value='(" NS "QFont)',numchild='1',children=["
        "{name='data',type='" NS "QFont',value='{...}',numchild='3'}]}",
        "local.v"));
    next();
    check("QVariantHash", "{" PRE "value='(" NS "QVariantHash)',numchild='1',children=["
        "{name='data',type='" NS "QHash<" NS "QString, " NS "QVariant>',"
            "value='<0 items>',numchild='0'}]}", "local.v");
    next();
    (void)GUI(check("QIcon", "{" PRE "value='(" NS "QIcon)',numchild='1',children=["
        "{name='data',type='" NS "QIcon',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
// FIXME:
//    (void)GUI(check("QImage", "{" PRE "value='(" NS "QImage)',numchild='1',children=["
//        "{name='data',type='" NS "QImage',value='{...}',numchild='1'}]}",
//        "local.v"));
    next();
    check("int", "{" PRE "value='42',numchild='0'}", "local.v");
    next();
    (void)GUI(check("QKeySequence","{" PRE "value='(" NS "QKeySequence)',numchild='1',children=["
        "{name='data',type='" NS "QKeySequence',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
    check("QLine", "{" PRE "value='(" NS "QLine)',numchild='1',children=["
        "{name='data',type='" NS "QLine',value='{...}',numchild='2'}]}", "local.v");
    next();
    check("QLineF", "{" PRE "value='(" NS "QLineF)',numchild='1',children=["
        "{name='data',type='" NS "QLineF',value='{...}',numchild='2'}]}", "local.v");
    next();
    check("QVariantList", "{" PRE "value='(" NS "QVariantList)',numchild='1',children=["
        "{name='data',type='" NS "QList<" NS "QVariant>',"
            "value='<0 items>',numchild='0'}]}", "local.v");
    next();
    check("QLocale", "{" PRE "value='(" NS "QLocale)',numchild='1',children=["
        "{name='data',type='" NS "QLocale',valueencoded='7',value='-',numchild='8'}]}", "local.v");
    next();
    check("qlonglong", "{" PRE "value='44',numchild='0'}", "local.v");
    next();
    check("QVariantMap", "{" PRE "value='(" NS "QVariantMap)',numchild='1',children=["
        "{name='data',type='" NS "QMap<" NS "QString, " NS "QVariant>',"
            "value='<0 items>',numchild='0'}]}", "local.v");
    next();
    (void)GUI(check("QTransform", "{" PRE "value='(" NS "QTransform)',numchild='1',children=["
        "{name='data',type='" NS "QTransform',value='{...}',numchild='7'}]}",
        "local.v"));
    next();
    (void)GUI(check("QMatrix4x4", "{" PRE "value='(" NS "QMatrix4x4)',numchild='1',children=["
        "{name='data',type='" NS "QMatrix4x4',value='{...}',numchild='2'}]}",
        "local.v"));
    next();
    (void)GUI(check("QPalette", "{" PRE "value='(" NS "QPalette)',numchild='1',children=["
        "{name='data',type='" NS "QPalette',value='{...}',numchild='4'}]}",
        "local.v"));
    next();
    (void)GUI(check("QPen", "{" PRE "value='(" NS "QPen)',numchild='1',children=["
        "{name='data',type='" NS "QPen',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
// FIXME:
//    (void)GUI(check("QPixmap", "{" PRE "value='(" NS "QPixmap)',numchild='1',children=["
//        "{name='data',type='" NS "QPixmap',value='{...}',numchild='1'}]}",
//        "local.v"));
    next();
    check("QPoint", "{" PRE "value='(" NS "QPoint)',numchild='1',children=["
        "{name='data',type='" NS "QPoint',value='(45, 46)',numchild='2'}]}",
            "local.v");
    next();
// FIXME
//    check("QPointF", "{" PRE "value='(" NS "QPointF)',numchild='1',children=["
//        "{name='data',type='" NS "QBrush',value='{...}',numchild='1'}]}",
//        "local.v"));
    next();
    (void)GUI(check("QPolygon", "{" PRE "value='(" NS "QPolygon)',numchild='1',children=["
        "{name='data',type='" NS "QPolygon',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
// FIXME:
//    (void)GUI(check("QQuaternion", "{" PRE "value='(" NS "QQuaternion)',numchild='1',children=["
//        "{name='data',type='" NS "QQuadernion',value='{...}',numchild='1'}]}",
//        "local.v"));
    next();
// FIXME: Fix value
    check("QRect", "{" PRE "value='(" NS "QRect)',numchild='1',children=["
        "{name='data',type='" NS "QRect',value='-',numchild='4'}]}", "local.v");
    next();
// FIXME: Fix value
    check("QRectF", "{" PRE "value='(" NS "QRectF)',numchild='1',children=["
        "{name='data',type='" NS "QRectF',value='-',numchild='4'}]}", "local.v");
    next();
    check("QRegExp", "{" PRE "value='(" NS "QRegExp)',numchild='1',children=["
        "{name='data',type='" NS "QRegExp',value='{...}',numchild='1'}]}", "local.v");
    next();
    (void)GUI(check("QRegion", "{" PRE "value='(" NS "QRegion)',numchild='1',children=["
        "{name='data',type='" NS "QRegion',value='{...}',numchild='2'}]}",
        "local.v"));
    next();
    check("QSize", "{" PRE "value='(" NS "QSize)',numchild='1',children=["
        "{name='data',type='" NS "QSize',value='(0, 0)',numchild='2'}]}", "local.v");
    next();

// FIXME:
//  check("QSizeF", "{" PRE "value='(" NS "QSizeF)',numchild='1',children=["
//        "{name='data',type='" NS "QBrush',value='{...}',numchild='1'}]}",
//        "local.v");
    next();
    (void)GUI(check("QSizePolicy", "{" PRE "value='(" NS "QSizePolicy)',numchild='1',children=["
        "{name='data',type='" NS "QSizePolicy',value='{...}',numchild='2'}]}",
        "local.v"));
    next();
    check("QString", "{" PRE "value='(" NS "QString)',numchild='1',children=["
        "{name='data',type='" NS "QString',valueencoded='7',value='610062006300',numchild='0'}]}",
        "local.v");
    next();
    check("QStringList", "{" PRE "value='(" NS "QStringList)',numchild='1',children=["
        "{name='data',type='" NS "QStringList',value='<1 items>',numchild='1'}]}", "local.v");
    next();
    (void)GUI(check("QTextFormat", "{" PRE "value='(" NS "QTextFormat)',numchild='1',children=["
        "{name='data',type='" NS "QTextFormat',value='{...}',numchild='3'}]}",
        "local.v"));
    next();
    (void)GUI(check("QTextLength", "{" PRE "value='(" NS "QTextLength)',numchild='1',children=["
        "{name='data',type='" NS "QTextLength',value='{...}',numchild='2'}]}",
        "local.v"));
    next();
    check("QTime", "{" PRE "value='(" NS "QTime)',numchild='1',children=["
        "{name='data',type='" NS "QTime',value='{...}',numchild='1'}]}", "local.v");
    next();
    check("uint", "{" PRE "value='43',numchild='0'}", "local.v");
    next();
    check("qulonglong", "{" PRE "value='45',numchild='0'}", "local.v");
    next();
    check("QUrl", "{" PRE "value='(" NS "QUrl)',numchild='1',children=["
        "{name='data',type='" NS "QUrl',value='{...}',numchild='1'}]}", "local.v");
    next();
    (void)GUI(check("QVector2D", "{" PRE "value='(" NS "QVector2D)',numchild='1',children=["
        "{name='data',type='" NS "QVector2D',value='{...}',numchild='2'}]}",
        "local.v"));
    next();
    (void)GUI(check("QVector3D", "{" PRE "value='(" NS "QVector3D)',numchild='1',children=["
        "{name='data',type='" NS "QVector3D',value='{...}',numchild='3'}]}",
        "local.v"));
    next();
    (void)GUI(check("QVector4D", "{" PRE "value='(" NS "QVector4D)',numchild='1',children=["
        "{name='data',type='" NS "QVector4D',value='{...}',numchild='4'}]}",
        "local.v"));
}


///////////////////////////// QWeakPointer /////////////////////////////////

#if QT_VERSION >= 0x040500

void dump_QWeakPointer_11()
{
    /* A */ QSharedPointer<int> sp;
    /*   */ QWeakPointer<int> wp = sp.toWeakRef();
    /* B */ (void) 0; }

void tst_Gdb::dump_QWeakPointer_11()
{
    // Case 1.1: Null pointer.
    prepare("dump_QWeakPointer_11");
    if (checkUninitialized)
        check("A","{iname='local.sp',name='sp',"
               "type='" NS "QSharedPointer<int>',value='<invalid>',numchild='0'},"
            "{iname='local.wp',name='wp',"
               "type='" NS "QWeakPointer<int>',value='<invalid>',numchild='0'}");
    next(2);
    check("B","{iname='local.sp',name='sp',"
            "type='" NS "QSharedPointer<int>',value='(null)',numchild='0'},"
            "{iname='local.wp',name='wp',"
            "type='" NS "QWeakPointer<int>',value='(null)',numchild='0'}");
}


void dump_QWeakPointer_12()
{
    /* A */ QSharedPointer<int> sp(new int(99));
    /*   */ QWeakPointer<int> wp = sp.toWeakRef();
    /* B */ (void) 0; }

void tst_Gdb::dump_QWeakPointer_12()
{
return;
    // Case 1.2: Weak pointer is unique.
    prepare("dump_QWeakPointer_12");
    if (checkUninitialized)
        check("A","{iname='local.sp',name='sp',"
               "type='" NS "QSharedPointer<int>',value='<invalid>',numchild='0'},"
            "{iname='local.wp',name='wp',"
                "type='" NS "QWeakPointer<int>',value='<invalid>',numchild='0'}");
    next();
    next();
    check("B","{iname='local.sp',name='sp',"
                "type='" NS "QSharedPointer<int>',value='',numchild='3'},"
            "{iname='local.wp',name='wp',"
                "type='" NS "QWeakPointer<int>',value='',numchild='3'}");
    check("B","{iname='local.sp',name='sp',"
            "type='" NS "QSharedPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='2',type='int',numchild='0'},"
                "{name='strongref',value='1',type='int',numchild='0'}]},"
            "{iname='local.wp',name='wp',"
            "type='" NS "QWeakPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='2',type='int',numchild='0'},"
                "{name='strongref',value='1',type='int',numchild='0'}]}",
            "local.sp,local.wp");
}


void dump_QWeakPointer_13()
{
    /* A */ QSharedPointer<int> sp(new int(99));
    /*   */ QWeakPointer<int> wp = sp.toWeakRef();
    /*   */ QWeakPointer<int> wp2 = sp.toWeakRef();
    /* B */ (void) 0; }

void tst_Gdb::dump_QWeakPointer_13()
{
    // Case 1.3: There are other weak pointers.
    prepare("dump_QWeakPointer_13");
    if (checkUninitialized)
       check("A","{iname='local.sp',name='sp',"
              "type='" NS "QSharedPointer<int>',value='<invalid>',numchild='0'},"
            "{iname='local.wp',name='wp',"
              "type='" NS "QWeakPointer<int>',value='<invalid>',numchild='0'},"
            "{iname='local.wp2',name='wp2',"
              "type='" NS "QWeakPointer<int>',value='<invalid>',numchild='0'}");
    next(3);
    check("B","{iname='local.sp',name='sp',"
              "type='" NS "QSharedPointer<int>',value='',numchild='3'},"
            "{iname='local.wp',name='wp',"
              "type='" NS "QWeakPointer<int>',value='',numchild='3'},"
            "{iname='local.wp2',name='wp2',"
              "type='" NS "QWeakPointer<int>',value='',numchild='3'}");
    check("B","{iname='local.sp',name='sp',"
              "type='" NS "QSharedPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='3',type='int',numchild='0'},"
                "{name='strongref',value='1',type='int',numchild='0'}]},"
            "{iname='local.wp',name='wp',"
              "type='" NS "QWeakPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='3',type='int',numchild='0'},"
                "{name='strongref',value='1',type='int',numchild='0'}]},"
            "{iname='local.wp2',name='wp2',"
              "type='" NS "QWeakPointer<int>',value='',numchild='3'}",
            "local.sp,local.wp");
}


void dump_QWeakPointer_2()
{
    /* A */ QSharedPointer<QString> sp(new QString("Test"));
    /*   */ QWeakPointer<QString> wp = sp.toWeakRef();
    /* B */ (void *) wp.data(); }

void tst_Gdb::dump_QWeakPointer_2()
{
    // Case 2: Composite type.
    prepare("dump_QWeakPointer_2");
    if (checkUninitialized)
        check("A","{iname='local.sp',name='sp',"
                    "type='" NS "QSharedPointer<" NS "QString>',"
                    "value='<invalid>',numchild='0'},"
                "{iname='local.wp',name='wp',"
                    "type='" NS "QWeakPointer<" NS "QString>',"
                    "value='<invalid>',numchild='0'}");
    next(2);
    check("B","{iname='local.sp',name='sp',"
          "type='" NS "QSharedPointer<" NS "QString>',value='',numchild='3',children=["
            "{name='data',type='" NS "QString',"
                "valueencoded='7',value='5400650073007400',numchild='0'},"
            "{name='weakref',value='2',type='int',numchild='0'},"
            "{name='strongref',value='1',type='int',numchild='0'}]},"
        "{iname='local.wp',name='wp',"
          "type='" NS "QWeakPointer<" NS "QString>',value='',numchild='3',children=["
            "{name='data',type='" NS "QString',"
                "valueencoded='7',value='5400650073007400',numchild='0'},"
            "{name='weakref',value='2',type='int',numchild='0'},"
            "{name='strongref',value='1',type='int',numchild='0'}]}",
        "local.sp,local.wp");
}

#else // before Qt 4.5

void tst_Gdb::dump_QWeakPointer_11() {}
void tst_Gdb::dump_QWeakPointer_12() {}
void tst_Gdb::dump_QWeakPointer_13() {}
void tst_Gdb::dump_QWeakPointer_2() {}

#endif


///////////////////////////// QWidget //////////////////////////////

void dump_QWidget()
{
#    ifdef QT_GUI_LIB
    /* A */ QWidget w;
    /* B */ (void) w.size();
#    endif
}

void tst_Gdb::dump_QWidget()
{
#    ifdef QT_GUI_LIB
    prepare("dump_QWidget");
    if (checkUninitialized)
        check("A","{iname='local.w',name='w',"
                "type='" NS "QWidget',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.w',name='w',type='" NS "QWidget',"
        "value='{...}',numchild='4',children=["
      "{name='" NS "QObject',type='" NS "QObject',"
        "valueencoded='7',value='',numchild='4',children=["
          "{name='parent',type='" NS "QObject *',value='0x0',numchild='0'},"
          "{name='children',type='" NS "QObject::QObjectList',"
            "value='<0 items>',numchild='0'},"
          "{name='properties',value='<1 items>',type='',numchild='1'},"
          "{name='connections',value='<0 items>',type='',numchild='0'},"
          "{name='signals',value='<2 items>',type='',"
            "numchild='2',childnumchild='0'},"
          "{name='slots',value='<2 items>',type='',"
            "numchild='2',childnumchild='0'}"
        "]},"
      "{name='" NS "QPaintDevice',type='" NS "QPaintDevice',"
        "value='{...}',numchild='2'},"
      "{name='data',type='" NS "QWidgetData *',"
        "value='-',numchild='1'}]}",
      "local.w,local.w." NS "QObject");
#    endif
}


///////////////////////////// std::deque<int> //////////////////////////////

void dump_std_deque()
{
    /* A */ std::deque<int> deque;
    /* B */ deque.push_back(45);
    /* C */ deque.push_back(46);
    /* D */ deque.push_back(47);
    /* E */ (void) 0;
}

void tst_Gdb::dump_std_deque()
{
    prepare("dump_std_deque");
    if (checkUninitialized)
        check("A","{iname='local.deque',name='deque',"
            "type='std::deque<int, std::allocator<int> >',"
            "value='<invalid>',numchild='0'}");
    next();
    check("B", "{iname='local.deque',name='deque',"
            "type='std::deque<int, std::allocator<int> >',"
            "value='<0 items>',numchild='0',children=[]}",
            "local.deque");
    next(3);
    check("E", "{iname='local.deque',name='deque',"
            "type='std::deque<int, std::allocator<int> >',"
            "value='<3 items>',numchild='3',"
            "childtype='int',childnumchild='0',children=["
                "{value='45'},{value='46'},{value='47'}]}",
            "local.deque");
    // FIXME: Try large container
}


///////////////////////////// std::list<int> //////////////////////////////

void dump_std_list()
{
    /* A */ std::list<int> list;
    /* B */ list.push_back(45);
    /* C */ list.push_back(46);
    /* D */ list.push_back(47);
    /* E */ (void) 0;
}

void tst_Gdb::dump_std_list()
{
    prepare("dump_std_list");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
            "numchild='0'}");
    next();
    check("B", "{iname='local.list',name='list',"
            "type='std::list<int, std::allocator<int> >',"
            "value='<0 items>',numchild='0',children=[]}",
            "local.list");
    next();
    check("C", "{iname='local.list',name='list',"
            "type='std::list<int, std::allocator<int> >',"
            "value='<1 items>',numchild='1',"
            "childtype='int',childnumchild='0',children=[{value='45'}]}",
            "local.list");
    next();
    check("D", "{iname='local.list',name='list',"
            "type='std::list<int, std::allocator<int> >',"
            "value='<2 items>',numchild='2',"
            "childtype='int',childnumchild='0',children=["
                "{value='45'},{value='46'}]}",
            "local.list");
    next();
    check("E", "{iname='local.list',name='list',"
            "type='std::list<int, std::allocator<int> >',"
            "value='<3 items>',numchild='3',"
            "childtype='int',childnumchild='0',children=["
                "{value='45'},{value='46'},{value='47'}]}",
            "local.list");
}


///////////////////////////// std::map<int, int> //////////////////////////////

void dump_std_map_int_int()
{
    /* A */ std::map<int, int> h;
    /* B */ h[12] = 34;
    /* C */ h[14] = 54;
    /* D */ (void) 0;
}

void tst_Gdb::dump_std_map_int_int()
{
    QByteArray type = "std::map<int, int, std::less<int>, "
        "std::allocator<std::pair<int const, int> > >";

    prepare("dump_std_map_int_int");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" + type + "',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B","{iname='local.h',name='h',"
            "type='" + type + "',value='<0 items>',"
            "numchild='0'}");
    next(2);
    check("D","{iname='local.h',name='h',"
            "type='" + type + "',value='<2 items>',"
            "numchild='2'}");
    check("D","{iname='local.h',name='h',"
            "type='" + type + "',value='<2 items>',"
            "numchild='2',childtype='int',childnumchild='0',"
            "children=[{name='12',value='34'},{name='14',value='54'}]}",
            "local.h,local.h.0,local.h.1");
}


//////////////////////// std::map<std::string, std::string> ////////////////////////

void dump_std_map_string_string()
{
    /* A */ std::map<std::string, std::string> m;
    /* B */ m["hello"] = "world";
    /* C */ m["foo"] = "bar";
    /* D */ (void) 0; }

void tst_Gdb::dump_std_map_string_string()
{
    QByteArray strType =
        "std::basic_string<char, std::char_traits<char>, std::allocator<char> >";
    QByteArray pairType =
        + "std::pair<" + strType + " const, " + strType + " >";
    QByteArray type = "std::map<" + strType + ", " + strType + ", "
        + "std::less<" + strType + " >, "
        + "std::allocator<" + pairType + " > >";

    prepare("dump_std_map_string_string");
    if (checkUninitialized)
        check("A","{iname='local.m',name='m',"
            "type='" + type + "',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B","{iname='local.m',name='m',"
            "type='" + type + "',value='<0 items>',"
            "numchild='0'}");
    next();
    next();
    check("D","{iname='local.m',name='m',"
            "type='" + type + "',value='<2 items>',"
            "numchild='2'}");
    check("D","{iname='local.m',name='m',type='" + type + "',"
            "value='<2 items>',numchild='2',childtype='" + pairType + "',"
            "childnumchild='2',children=["
              "{value=' ',children=["
                 "{name='first',type='const " + strType + "',"
                    "type='std::string',"
                    "valueencoded='6',value='666f6f',numchild='0'},"
                 "{name='second',type='" + strType + "',"
                    "type='std::string',"
                    "valueencoded='6',value='626172',numchild='0'}]},"
              "{value=' ',children=["
                 "{name='first',type='const " + strType + "',"
                    "type='std::string',"
                    "valueencoded='6',value='68656c6c6f',numchild='0'},"
                 "{name='second',type='" + strType + "',"
                    "type='std::string',"
                    "valueencoded='6',value='776f726c64',numchild='0'}]}"
            "]}",
            "local.m,local.m.0,local.m.1");
}


///////////////////////////// std::set<int> ///////////////////////////////////

void dump_std_set_int()
{
    /* A */ std::set<int> h;
    /* B */ h.insert(42);
    /* C */ h.insert(44);
    /* D */ (void) 0;
}

void tst_Gdb::dump_std_set_int()
{
    QByteArray setType = "std::set<int, std::less<int>, std::allocator<int> >";
    prepare("dump_std_set_int");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" + setType + "',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("D","{iname='local.h',name='h',"
            "type='" + setType + "',value='<2 items>',numchild='2',"
            "childtype='int',childnumchild='0',children=["
            "{value='42'},{value='44'}]}", "local.h");
}


///////////////////////////// QSet<Int3> ///////////////////////////////////

void dump_std_set_Int3()
{
    /* A */ std::set<Int3> h;
    /* B */ h.insert(Int3(42));
    /* C */ h.insert(Int3(44));
    /* D */ (void) 0; }

void tst_Gdb::dump_std_set_Int3()
{
    QByteArray setType = "std::set<Int3, std::less<Int3>, std::allocator<Int3> >";
    prepare("dump_std_set_Int3");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" + setType + "',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("D","{iname='local.h',name='h',"
            "type='" + setType + "',value='<2 items>',numchild='2',"
            "childtype='Int3',children=["
            "{value='{...}',numchild='3'},{value='{...}',numchild='3'}]}", "local.h");
}



///////////////////////////// std::string //////////////////////////////////

void dump_std_string()
{
    /* A */ std::string str;
    /* B */ str = "Hallo";
    /* C */ (void) 0; }

void tst_Gdb::dump_std_string()
{
    prepare("dump_std_string");
    if (checkUninitialized)
        check("A","{iname='local.str',name='str',type='-',"
            "value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.str',name='str',type='std::string',"
            "valueencoded='6',value='',numchild='0'}");
    next();
    check("C","{iname='local.str',name='str',type='std::string',"
            "valueencoded='6',value='48616c6c6f',numchild='0'}");
}


///////////////////////////// std::wstring //////////////////////////////////

void dump_std_wstring()
{
    /* A */ std::wstring str;
    /* B */ str = L"Hallo";
    /* C */ (void) 0;
}

void tst_Gdb::dump_std_wstring()
{
    prepare("dump_std_wstring");
    if (checkUninitialized)
        check("A","{iname='local.str',name='str',"
            "numchild='0'}");
    next();
    check("B","{iname='local.str',name='str',type='std::string',valueencoded='6',"
            "value='',numchild='0'}");
    next();
    if (sizeof(wchar_t) == 2)
        check("C","{iname='local.str',name='str',type='std::string',valueencoded='6',"
            "value='00480061006c006c006f',numchild='0'}");
    else
        check("C","{iname='local.str',name='str',type='std::string',valueencoded='6',"
            "value='00000048000000610000006c0000006c0000006f',numchild='0'}");
}


///////////////////////////// std::vector<int> //////////////////////////////

void dump_std_vector()
{
    /* A */ std::vector<std::list<int> *> vector;
            std::list<int> list;
    /* B */ list.push_back(45);
    /* C */ vector.push_back(new std::list<int>(list));
    /* D */ vector.push_back(0);
    /* E */ (void) list.size();
    /* F */ (void) list.size(); }

void tst_Gdb::dump_std_vector()
{
    QByteArray listType = "std::list<int, std::allocator<int> >";
    QByteArray vectorType = "std::vector<" + listType + "*, "
        "std::allocator<" + listType + "*> >";

    prepare("dump_std_vector");
    if (checkUninitialized)
        check("A","{iname='local.vector',name='vector',type='" + vectorType + "',"
            "value='<invalid>',numchild='0'},"
        "{iname='local.list',name='list',type='" + listType + "',"
            "value='<invalid>',numchild='0'}");
    next(2);
    check("B","{iname='local.vector',name='vector',type='" + vectorType + "',"
            "value='<0 items>',numchild='0'},"
        "{iname='local.list',name='list',type='" + listType + "',"
            "value='<0 items>',numchild='0'}");
    next(3);
    check("E","{iname='local.vector',name='vector',type='" + vectorType + "',"
            "value='<2 items>',numchild='2',childtype='" + listType + " *',"
            "childnumchild='1',children=["
                "{type='" + listType + "',value='<1 items>',"
                    "childtype='int',"
                    "childnumchild='0',children=[{value='45'}]},"
                "{value='0x0',numchild='0'}]},"
            "{iname='local.list',name='list',type='" + listType + "',"
                "value='<1 items>',numchild='1'}",
        "local.vector,local.vector.0");
}


/////////////////////////////////////////////////////////////////////////
//
// Main
//
/////////////////////////////////////////////////////////////////////////

void breaker() {}

int main(int argc, char *argv[])
{
#    ifdef QT_GUI_LIB
    QApplication app(argc, argv);
#    else
    QCoreApplication app(argc, argv);
#    endif
    breaker();

    QStringList args = app.arguments();
    if (args.size() == 2 && args.at(1) == "run") {
        // We are the debugged process, recursively called and steered
        // by our spawning alter ego.
        return 0;
    }

    if (args.size() == 2 && args.at(1) == "debug") {
        dump_array_char();
        dump_array_int();
        dump_std_deque();
        dump_std_list();
        dump_std_map_int_int();
        dump_std_map_string_string();
        dump_std_set_Int3();
        dump_std_set_int();
        dump_std_vector();
        dump_std_string();
        dump_std_wstring();
        dump_Foo();
        dump_misc();
        dump_typedef();
        dump_QAbstractItemModel();
        dump_QAbstractItemAndModelIndex();
        dump_QByteArray();
        dump_QChar();
        dump_QDir();
        dump_QFile();
        dump_QFileInfo();
        dump_QHash_int_int();
        dump_QHash_QString_QString();
        dump_QImage();
        dump_QLinkedList_int();
        dump_QList_char();
        dump_QList_char_star();
        dump_QList_int();
        dump_QList_int_star();
        dump_QList_Int3();
        dump_QList_QString();
        dump_QList_QString3();
        dump_QLocale();
        dump_QMap_int_int();
        dump_QMap_QString_QString();
        dump_QPixmap();
        dump_QObject();
        dump_QPoint();
        dump_QRect();
        dump_QSet_int();
        dump_QSet_Int3();
        dump_QSharedPointer();
        dump_QSize();
        dump_QStack();
        dump_QString();
        dump_QStringList();
        dump_QTextCodec();
        dump_QVariant();
        dump_QVector();
        dump_QWeakPointer_11();
        dump_QWeakPointer_12();
        dump_QWeakPointer_13();
        dump_QWeakPointer_2();
        dump_QWidget();
    }

    try {
        // Plain call. Start the testing.
        tst_Gdb *test = new tst_Gdb;
        return QTest::qExec(test, argc, argv);
    } catch (...) {
        qDebug() << "TEST ABORTED ";
    }
}

#include "tst_gdb.moc"

