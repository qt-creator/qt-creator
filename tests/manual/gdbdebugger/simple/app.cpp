/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

//#include <complex>


//template <typename T> class B;  B foo() {}

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QLibrary>
#include <QtCore/QLinkedList>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QStack>
#include <QtCore/QThread>
#include <QtCore/QVariant>
#include <QtCore/QVector>
#if QT_VERSION >= 0x040500
#include <QtCore/QSharedPointer>
#endif

#include <QtGui/QApplication>
#include <QtGui/QAction>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QLabel>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QStandardItemModel>

#include <QtNetwork/QHostAddress>

#include <deque>
#include <iostream>
#include <map>
#include <list>
#include <set>
#include <stack>
#include <string>
#include <vector>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

template <typename T> class Vector
{
public:
    explicit Vector(int size) : m_size(size), m_data(new T[size]) {}
    ~Vector() { delete [] m_data; }
    //...
private:
    int m_size;
    T *m_data;
};


namespace nsX {
namespace nsY {
int z;
}
}

namespace nsXY = nsX::nsY;

int qwert()
{
    return nsXY::z;
}

uint qHash(const QMap<int, int> &)
{
    return 0;
}

uint qHash(const double & f)
{
    return int(f);
}


class Foo
{
public:
    Foo(int i = 0)
        : a(i), b(2)
    {
        int s = 1;
        int t = 2;
        b = 2 + s + t;
        a += 1;
        a += 1;
        a += 1;
        a += 1;
        a += 1;
        a += 1;
        a += 1;
        a += 1;
        a += 1;
        a += 1;
        a += 1;
        a += 1;
        a += 1;
        a += 1;
        a += 1;
    }

    ~Foo()
    {
        a = 5;
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

class X : virtual public Foo
{
public:
    X() {
    }
};

class Y : virtual public Foo
{
public:
    Y() {
    }
};

void testArray()
{
    X xxx;
    (void *) 0;
    double d[3][3];
    for (int i = 0; i != 3; ++i)
        for (int j = 0; j != 3; ++j)
            d[i][j] = i + j;

    char c[20];
    c[0] = 'a';
    c[1] = 'b';
    c[2] = 'c';
    c[3] = 'd';

    QString x[20];
    x[0] = "a";
    x[1] = "b";
    x[2] = "c";
    x[3] = "d";

    Foo foo[10];
    //for (int i = 0; i != sizeof(foo)/sizeof(foo[0]); ++i) {
    for (int i = 0; i < 5; ++i) {
        foo[i].a = i;
        foo[i].doit();
    }
}

struct TestAnonymous
{
    union {
        struct { int i; int b; };
        struct { float f; };
        double d;
    };
};


void testPeekAndPoke3()
{
    // Anonymous structs
    {
        union {
            struct { int i; int b; };
            struct { float f; };
            double d;
        } a = { 42, 43 };
        a.i = 1; // Break here. Expand a. Step.
        a.i = 2; // Change a.i in Locals view to 0. This changes f, d but expectedly not b. Step.
        a.i = 3; // Continue.
    }

    // Complex watchers
    {
        struct S { int a; double b; } s[10];
        for (int i = 0; i != 10; ++i) {
            s[i].a = i;  // Break here. Expand s and s[0]. Step.
            // Watcher Context: "Add New Watcher".
            // Type    ['s[%d].a' % i for i in range(5)]
            // Expand it, continue stepping.
        }
    }

    // QImage display
    {
        QImage im(QSize(200, 200), QImage::Format_RGB32);
        im.fill(QColor(200, 10, 30).rgba());
        QPainter pain;
        pain.begin(&im);
        pain.setPen(QPen(Qt::black, 5.0, Qt::SolidLine, Qt::RoundCap));
        pain.drawEllipse(20, 20, 160, 160); // Break here. Step.
        // Toggle between "Normal" and "Displayed" in L&W Context Menu, entry "Display of Type QImage". Step.
        pain.drawArc(70, 115, 60, 30, 200 * 16, 140 * 16);
        pain.setBrush(Qt::black);
        pain.drawEllipse(65, 70, 15, 15); // Step.
        pain.drawEllipse(120, 70, 15, 15); // Step.
        pain.end();
    }

}

void testAnonymous()
{
    TestAnonymous a;
    a.i = 1;
    a.i = 2;
    a.i = 3;
}

void testFunctionPointer()
{
    typedef void (*func_t)();
    func_t f1 = testAnonymous;
    func_t f2 = testFunctionPointer;
    func_t f3 = testFunctionPointer;
}

void testQByteArray()
{
    QByteArray ba = "Hello";
    ba += '"';
    ba += "World";

    const char *str1 = "\356";
    const char *str2 = "\xee";
    const char *str3 = "\\ee";
    QByteArray buf1( str1 );
    QByteArray buf2( str2 );
    QByteArray buf3( str3 );

    ba += char(0);
    ba += 1;
    ba += 2;
}

static void throwit1()
{
    throw 14;
}

static void throwit()
{
    throwit1();
}

int testCatchThrow()
{
    int gotit = 0;
    try {
        throwit();
    } catch (int what) {
        gotit = what;
    }
    return gotit;
}

void testQDateTime()
{
    QDateTime date;
    date = QDateTime::currentDateTime();
    date = date.addSecs(5);
    date = date.addSecs(5);
    date = date.addSecs(5);
    date = date.addSecs(5);
}

void testQFileInfo()
{
    QFileInfo fi("/tmp/t");
    QString s = fi.absoluteFilePath();
    QString t = fi.bundleName();
}

void testQHash()
{
#if 1
    QHash<int, float> hgg0;
    hgg0[11] = 11.0;
    hgg0[22] = 22.0;
    hgg0[22] = 22.0;
    hgg0[22] = 22.0;
    hgg0[22] = 22.0;
    hgg0[22] = 22.0;
    hgg0[22] = 22.0;

#endif

#if 1

    QHash<QString, int> hgg1;
    hgg1["22.0"] = 22.0;
    hgg1["123.0"] = 22.0;
    hgg1["111111ss111128.0"] = 28.0;
    hgg1["11124.0"] = 22.0;
    hgg1["1111125.0"] = 22.0;
    hgg1["11111126.0"] = 22.0;
    hgg1["111111127.0"] = 27.0;
    hgg1["111111111128.0"] = 28.0;
    hgg1["111111111111111111129.0"] = 29.0;

    QHash<QByteArray, float> hgx1;
    hgx1["22.0"] = 22.0;
    hgx1["123.0"] = 22.0;
    hgx1["111111ss111128.0"] = 28.0;
    hgx1["11124.0"] = 22.0;
    hgx1["1111125.0"] = 22.0;
    hgx1["11111126.0"] = 22.0;
    hgx1["111111127.0"] = 27.0;
    hgx1["111111111128.0"] = 28.0;
    hgx1["111111111111111111129.0"] = 29.0;
#endif
#if 1
    QHash<int, QString> hgg2;
    hgg2[22] = "22.0";

    QHash<QString, Foo> hgg3;
    hgg3["22.0"] = Foo(22);
    hgg3["33.0"] = Foo(33);

    QObject ob;
    QHash<QString, QPointer<QObject> > hash;
    hash.insert("Hallo", QPointer<QObject>(&ob));
    hash.insert("Welt", QPointer<QObject>(&ob));
    hash.insert(".", QPointer<QObject>(&ob));
#endif
}

void testQImage()
{
    QImage im(QSize(200, 200), QImage::Format_RGB32);
    im.fill(QColor(200, 100, 130).rgba());
    QPainter pain;
    pain.begin(&im);
    pain.drawLine(2, 2, 130, 130);
    pain.drawLine(4, 2, 130, 140);
    pain.drawRect(30, 30, 80, 80);
    pain.end();
}

struct Function
{
    Function(QByteArray var, QByteArray f, double min, double max)
      : var(var), f(f), min(min), max(max) {}
    QByteArray var;
    QByteArray f;
    double min;
    double max;
};

void testFunction()
{
    // In order to use this, switch on the 'qDump__Function' in gdbmacros.py
    Function func("x", "sin(x)", 0, 1);
    func.max = 10;
    func.f = "cos(x)";
    func.max = 4;
    func.max = 5;
    func.max = 6;
    func.max = 7;
    func.max = 8;
}

void testIO()
{
    qDebug() << "qDebug() 1";
    qDebug() << "qDebug() 2";
    qDebug() << "qDebug() 3";
    qDebug() << "qDebug <foo & bar>";

    std::cout << "std::cout @@ 1" << std::endl;
    std::cout << "std::cout @@ 2\n";
    std::cout << "std::cout @@ 3" << std::endl;
    std::cout << "std::cout <foo & bar>\n";

    std::cerr << "std::cerr 1\n";
    std::cerr << "std::cerr 2\n";
    std::cerr << "std::cerr 3\n";
    std::cerr << "std::cerr <foo & bar>\n";
}

void testQLinkedList()
{
#if 1
    QLinkedList<int> li;
    QLinkedList<uint> lu;

    for (int i = 0; i != 3; ++i)
        li.append(i);
    li.append(102);


    lu.append(102);
    lu.append(102);
    lu.append(102);

    QLinkedList<Foo *> lpi;
    lpi.append(new Foo(1));
    lpi.append(0);
    lpi.append(new Foo(3));

    QLinkedList<qulonglong> l;
    l.append(42);
    l.append(43);
    l.append(44);
    l.append(45);

    QLinkedList<Foo> f;
    f.append(Foo(1));
    f.append(Foo(2));
#endif
    QLinkedList<std::string> v;
    v.push_back("aa");
    v.push_back("bb");
    v.push_back("cc");
    v.push_back("dd");
 }

void testQList()
{
    QList<int> big;
    for (int i = 0; i < 10000; ++i)
        big.push_back(i);

    QList<Foo> flist;
    for (int i = 0; i < 100; ++i)
        flist.push_back(i + 15);
    flist.push_back(1000);
    flist.push_back(1001);
    flist.push_back(1002);
#if 1
    QList<int> li;
    QList<uint> lu;

    for (int i = 0; i != 30; ++i) {
        li.append(i);
    }
    li.append(101);
    li.append(102);
    li.append(102);
    li.append(102);
    li.append(102);
    li.append(102);
    li.append(102);
    li.append(102);
    li.append(102);
    li.append(102);
    li.append(102);
    li.append(102);
    li.append(102);
    li.append(102);

    QList<int *> lpi;
    lpi.append(new int(1));
    lpi.append(new int(2));
    lpi.append(new int(3));


    for (int i = 0; i != 3; ++i) {
        lu.append(i);
    }
    lu.append(101);
    lu.append(102);
    lu.append(102);
    lu.append(102);

    QList<uint> i;
    i.append(42);
    i.append(43);
    i.append(44);
    i.append(45);

    QList<ushort> ls;
    ls.append(42);
    ls.append(43);
    ls.append(44);
    ls.append(45);

    QList<QChar> lc;
    lc.append(QChar('a'));
    lc.append(QChar('b'));
    lc.append(QChar('c'));
    lc.append(QChar('d'));

    QList<qulonglong> l;
    l.append(42);
    l.append(43);
    l.append(44);
    l.append(45);

    QList<Foo> f;
    f.append(Foo(1));
    f.append(Foo(2));

    QList<std::string> v;
    v.push_back("aa");
    v.push_back("bb");
    v.push_back("cc");
    v.push_back("dd");
#endif
 }

void testQMap()
{
    QMap<uint, QStringList> ggl;
    ggl[11] = QStringList() << "11";
    ggl[22] = QStringList() << "22";

    typedef QMap<uint, QStringList> T;
    T ggt;
    ggt[11] = QStringList() << "11";
    ggt[22] = QStringList() << "22";

#if 1
    QMap<uint, float> gg0;
    gg0[11] = 11.0;
    gg0[22] = 22.0;


    QMap<QString, float> gg1;
    gg1["22.0"] = 22.0;

    QMap<int, QString> gg2;
    gg2[22] = "22.0";

    QMap<QString, Foo> gg3;
    gg3["22.0"] = Foo(22);
    gg3["33.0"] = Foo(33);

    QObject ob;
    QMap<QString, QPointer<QObject> > map;
    map.insert("Hallo", QPointer<QObject>(&ob));
    map.insert("Welt", QPointer<QObject>(&ob));
    map.insert(".", QPointer<QObject>(&ob));
#endif
}

void testQMultiMap()
{
    QMultiMap<uint, float> gg0;
    gg0.insert(11, 11.0);
    gg0.insert(22, 22.0);
    gg0.insert(22, 33.0);
    gg0.insert(22, 34.0);
    gg0.insert(22, 35.0);
    gg0.insert(22, 36.0);
#if 1
    QMultiMap<QString, float> gg1;
    gg1.insert("22.0", 22.0);

    QMultiMap<int, QString> gg2;
    gg2.insert(22, "22.0");

    QMultiMap<QString, Foo> gg3;
    gg3.insert("22.0", Foo(22));
    gg3.insert("33.0", Foo(33));
    gg3.insert("22.0", Foo(22));

    QObject ob;
    QMultiMap<QString, QPointer<QObject> > map;
    map.insert("Hallo", QPointer<QObject>(&ob));
    map.insert("Welt", QPointer<QObject>(&ob));
    map.insert(".", QPointer<QObject>(&ob));
    map.insert(".", QPointer<QObject>(&ob));
#endif
}

namespace Names {
namespace Bar {

struct Ui {
    Ui() { w = 0; }
    QWidget *w;
};

class TestObject : public QObject
{
    Q_OBJECT

public:
    TestObject(QObject *parent = 0)
      : QObject(parent)
    {
        m_ui = new Ui;
        m_ui->w = 0;
    }

    Ui *m_ui;
};

} // namespace Bar
} // namespace Names

void testQObject(int &argc, char *argv[])
{
    QApplication app(argc, argv);
    Names::Bar::TestObject test;

    QAction act("xxx", &app);
    QString t = act.text();
    t += "y";
    t += "y";
    t += "y";
    t += "y";
    t += "y";

#if 1
    QObject ob(&app);
    ob.setObjectName("An Object");
    QObject ob1;
    ob1.setObjectName("Another Object");

    QObject::connect(&ob, SIGNAL(destroyed()), &ob1, SLOT(deleteLater()));
    QObject::connect(&app, SIGNAL(lastWindowClosed()), &ob, SLOT(deleteLater()));

    QList<QObject *> obs;
    obs.append(&ob);
    obs.append(&ob1);
    obs.append(0);
    obs.append(&app);
    ob1.setObjectName("A Subobject");
#endif
    QString str = QString::fromUtf8("XXXXXXXXXXXXXXyyXXX ö");
    QLabel l(str);
    l.setObjectName("Some Label");
    l.show();
    app.exec();
}

void testQPixmap()
{
    QImage im(QSize(200, 200), QImage::Format_RGB32);
    im.fill(QColor(200, 100, 130).rgba());
    QPainter pain;
    pain.begin(&im);
    pain.drawLine(2, 2, 130, 130);
    pain.end();
    QPixmap pm = QPixmap::fromImage(im);
    int i = 1;
    Q_UNUSED(i);
}

void testPlugin()
{
    QString dir = QDir::currentPath();
#ifdef Q_OS_LINUX
    QLibrary lib(dir + "/libplugin.so");
#endif
#ifdef Q_OS_MAC
    dir = QFileInfo(dir + "/../..").canonicalPath();
    QLibrary lib(dir + "/libplugin.dylib");
#endif
#ifdef Q_OS_WIN
    QLibrary lib(dir + "/plugin.dll");
#endif
#ifdef Q_OS_SYMBIAN
    QLibrary lib(dir + "/plugin.dll");
#endif
    int (*foo)() = (int(*)()) lib.resolve("pluginTest");
    qDebug() << "library resolve: " << foo;
    if (foo) {
        int res = foo();
        res += 1;
    } else {
        qDebug() << lib.errorString();
    }
}

void testQSet()
{
    QSet<int> hgg0;
    hgg0.insert(11);
    hgg0.insert(22);

    QSet<QString> hgg1;
    hgg1.insert("22.0");

    QObject ob;
    QSet<QPointer<QObject> > hash;
    QPointer<QObject> ptr(&ob);
    ptr;
    //hash.insert(ptr);
    //hash.insert(ptr);
    //hash.insert(ptr);
}

#if QT_VERSION >= 0x040500
class EmployeeData : public QSharedData
{
public:
    EmployeeData() : id(-1) { name.clear(); }
    EmployeeData(const EmployeeData &other)
        : QSharedData(other), id(other.id), name(other.name) { }
    ~EmployeeData() { }

    int id;
    QString name;
};

class Employee
{
public:
    Employee() { d = new EmployeeData; }
    Employee(int id, QString name) {
        d = new EmployeeData;
        setId(id);
        setName(name);
    }
    Employee(const Employee &other)
          : d (other.d)
    {
    }
    void setId(int id) { d->id = id; }
    void setName(QString name) { d->name = name; }

    int id() const { return d->id; }
    QString name() const { return d->name; }

   private:
     QSharedDataPointer<EmployeeData> d;
};


void testQSharedPointer()
{
    //Employee e1(1, "Herbert");
    //Employee e2 = e1;

    QSharedPointer<int> iptr(new int(43));
    QSharedPointer<int> iptr2 = iptr;
    QSharedPointer<int> iptr3 = iptr;

    QSharedPointer<QString> ptr(new QString("hallo"));
    QSharedPointer<QString> ptr2 = ptr;
    QSharedPointer<QString> ptr3 = ptr;

    QWeakPointer<int> wiptr(iptr);
    QWeakPointer<int> wiptr2 = wiptr;
    QWeakPointer<int> wiptr3 = wiptr;

    QWeakPointer<QString> wptr(ptr);
    QWeakPointer<QString> wptr2 = wptr;
    QWeakPointer<QString> wptr3 = wptr;
}
#endif

void stringRefTest(const QString &refstring)
{
    Q_UNUSED(refstring);
}

void testStdDeque()
{
    std::deque<int *> plist1;
    plist1.push_back(new int(1));
    plist1.push_back(0);
    plist1.push_back(new int(2));
    plist1.pop_back();
    plist1.pop_front();
    plist1.pop_front();

    std::deque<int> flist2;
    flist2.push_back(1);
    flist2.push_back(2);

    std::deque<Foo *> plist;
    plist.push_back(new Foo(1));
    plist.push_back(new Foo(2));

    std::deque<Foo> flist;
    flist.push_back(1);
    flist.push_front(2);
}

void testStdList()
{
    std::list<int> big;
    for (int i = 0; i < 10000; ++i)
        big.push_back(i);

    std::list<Foo> flist;
    for (int i = 0; i < 100; ++i)
        flist.push_back(i + 15);

#if 0
    std::list<int *> plist1;
    plist1.push_back(new int(1));
    plist1.push_back(0);
    plist1.push_back(new int(2));

    std::list<int> flist2;
    flist2.push_back(1);
    flist2.push_back(2);
    flist2.push_back(3);
    flist2.push_back(4);

    flist2.push_back(1);
    flist2.push_back(2);
    flist2.push_back(3);
    flist2.push_back(4);

    std::list<Foo *> plist;
    plist.push_back(new Foo(1));
    plist.push_back(0);
    plist.push_back(new Foo(2));


    foreach (Foo f, flist)
    {}

    std::list<bool> vec;
    vec.push_back(true);
    vec.push_back(false);
#endif
}

void testStdMap()
{
    std::map<QString, Foo> gg3;
    gg3["22.0"] = Foo(22);
    gg3["33.0"] = Foo(33);
    gg3["44.0"] = Foo(44);


    std::map<const char *, Foo> m1;
    m1["22.0"] = Foo(22);
    m1["33.0"] = Foo(33);
    m1["44.0"] = Foo(44);
#if 1
    std::map<uint, uint> gg;
    gg[11] = 1;
    gg[22] = 2;
    gg[33] = 3;
    gg[44] = 4;
    gg[55] = 5;

#endif

#if 0
    std::map<uint, QStringList> ggl;
    ggl[11] = QStringList() << "11";
    ggl[22] = QStringList() << "22";
    ggl[33] = QStringList() << "33";
    ggl[44] = QStringList() << "44";
    ggl[55] = QStringList() << "55";

    typedef std::map<uint, QStringList> T;
    T ggt;
    ggt[11] = QStringList() << "11";
    ggt[22] = QStringList() << "22";

    std::map<uint, float> gg0;
    gg0[11] = 11.0;
    gg0[22] = 22.0;


    std::map<QString, float> gg1;
    gg1["22.0"] = 22.0;

    std::map<int, QString> gg2;
    gg2[22] = "22.0";

    QObject ob;
    std::map<QString, QPointer<QObject> > map;
    map["Hallo"] = QPointer<QObject>(&ob);
    map["Welt"] = QPointer<QObject>(&ob);
    map["."] = QPointer<QObject>(&ob);
#endif
}

void testStdSet()
{
    std::set<int> hgg0;
    hgg0.insert(11);
    hgg0.insert(22);
    hgg0.insert(33);
#if 1
    std::set<QString> hgg1;
    hgg1.insert("22.0");

    QObject ob;
    std::set<QPointer<QObject> > hash;
    QPointer<QObject> ptr(&ob);
#endif
}

void testStdStack()
{
    std::stack<int *> plist1;
    plist1.push(new int(1));
    plist1.push(0);
    plist1.push(new int(2));
    plist1.pop();
    plist1.pop();
    plist1.pop();

    std::stack<int> flist2;
    flist2.push(1);
    flist2.push(2);

    std::stack<Foo *> plist;
    plist.push(new Foo(1));
    plist.push(new Foo(2));

    std::stack<Foo> flist;
    flist.push(1);
    flist.push(2);
}

void testStdString()
{
    QString foo;
    std::string str;
    std::wstring wstr;

    std::vector<std::string> v;

    foo += "a";
    str += "b";
    wstr += wchar_t('e');
    foo += "c";
    str += "d";
    foo += "e";
    wstr += wchar_t('e');
    str += "e";
    foo += "a";
    str += "b";
    foo += "c";
    str += "d";
    str += "e";
    wstr += wchar_t('e');
    wstr += wchar_t('e');
    str += "e";

    QList<std::string> l;

    l.push_back(str);
    l.push_back(str);
    l.push_back(str);
    l.push_back(str);

    v.push_back(str);
    v.push_back(str);
    v.push_back(str);
    v.push_back(str);
}

void testStdVector()
{
    std::vector<int *> plist1;
    plist1.push_back(new int(1));
    plist1.push_back(0);
    plist1.push_back(new int(2));

    std::vector<int> flist2;
    flist2.push_back(1);
    flist2.push_back(2);
    flist2.push_back(3);
    flist2.push_back(4);

    flist2.push_back(1);
    flist2.push_back(2);
    flist2.push_back(3);
    flist2.push_back(4);

    std::vector<Foo *> plist;
    plist.push_back(new Foo(1));
    plist.push_back(0);
    plist.push_back(new Foo(2));

    std::vector<Foo> flist;
    flist.push_back(1);
    flist.push_back(2);
    flist.push_back(3);
    flist.push_back(4);
    //flist.takeFirst();
    //flist.takeFirst();

    std::vector<bool> vec;
    vec.push_back(true);
    vec.push_back(false);
}

void testQStandardItemModel()
{
    QStandardItemModel m;
    QStandardItem *i1, *i2, *i11;
    m.appendRow(QList<QStandardItem *>()
         << (i1 = new QStandardItem("1")) << (new QStandardItem("a")));
    m.appendRow(QList<QStandardItem *>()
         << (i2 = new QStandardItem("2")) << (new QStandardItem("b")));
    i1->appendRow(QList<QStandardItem *>()
         << (i11 = new QStandardItem("11")) << (new QStandardItem("aa")));
    int i = 1;
    ++i;
    +i;
}

void testQStack()
{
    QVector<int> bigv;
    for (int i = 0; i < 10; ++i)
        bigv.append(i);
    QStack<int> big;
    for (int i = 0; i < 10; ++i)
        big.append(i);
    QStack<Foo *> plist;
    plist.append(new Foo(1));
    plist.append(0);
    plist.append(new Foo(2));
    QStack<Foo> flist;
    flist.append(1);
    flist.append(2);
    flist.append(3);
    flist.append(4);
    //flist.takeFirst();
    //flist.takeFirst();
    QStack<bool> vec;
    vec.append(true);
    vec.append(false);
}

void testQString()
{
    QString str = "Hello ";
    str += " big, ";
    str += " fat ";
    str += " World ";
    str += " World ";
    str += " World ";
    str += " World ";
    str += " World ";
}

void testQString3()
{
    QString str = "Hello ";
    str += " big, ";
    str += " fat ";
    str += " World ";

    QString string("String Test");
    QString *pstring = new QString("Pointer String Test");
    stringRefTest(QString("Ref String Test"));
    string = "Hi";
    string += "Du";
    qDebug() << string;
    delete pstring;
}

void testQStringList()
{
    QStringList l;
    l << "Hello ";
    l << " big, ";
    l << " fat ";
    l << " World ";
}

void testStruct()
{
    Foo f(2);
    f.doit();
    f.doit();
    f.doit();
}

class Thread : public QThread
{
public:
    Thread(int id) : m_id(id) {}

    void run()
    {
        for (int i = 0; i != 100000; ++i) {
            //sleep(1);
            std::cerr << m_id;
        }
    }

private:
    int m_id;
};

void testQThread()
{
    Thread thread1(1);
    Thread thread2(2);
    thread1.start();
    thread2.start();
    thread1.wait();
    thread2.wait();
}

void testQVariant1()
{
    QVariant v;
    v = 1;
    v = 1.0;
    v = "string";
    v = 1;
}

void testQVariant2()
{
    QVariant value;
    QVariant::Type t = QVariant::String;
    value = QVariant(t, (void*)0);
    *(QString*)value.data() = QString("XXX");

    int i = 1;
#if 0
    QVariant var;
    var.setValue(1);
    var.setValue(2);
    var.setValue(3);
    var.setValue(QString("Hello"));
    var.setValue(QString("World"));
    var.setValue(QString("Hello"));
    var.setValue(QStringList() << "World");
    var.setValue(QStringList() << "World" << "Hello");
    var.setValue(QStringList() << "Hello" << "Hello");
    var.setValue(QStringList() << "World" << "Hello" << "Hello");
#endif
#if 0
    QVariant var3;
    QHostAddress ha("127.0.0.1");
    qVariantSetValue(var, ha);
    var3 = var;
    var3 = var;
    var3 = var;
    var3 = var;
    QHostAddress ha1 = var.value<QHostAddress>();
    typedef QMap<uint, QStringList> MyType;
    MyType my;
    my[1] = (QStringList() << "Hello");
    my[3] = (QStringList() << "World");
    var.setValue(my);
    QString type = var.typeName();
    var.setValue(my);
    var.setValue(my);
    var.setValue(my);
    var.setValue(my);
#endif
}

void testQVariant3()
{
    QList<int> list;
    list << 1 << 2 << 3;
    QVariant variant = qVariantFromValue(list);
    list.clear();
    list = qVariantValue<QList<int> >(variant);
}

void testQVector()
{
    QVector<int> big(10000);

    QVector<Foo *> plist;
    plist.append(new Foo(1));
    plist.append(0);
    plist.append(new Foo(2));

    QVector<Foo> flist;
    flist.append(1);
    flist.append(2);
    flist.append(3);
    flist.append(4);
    //flist.takeFirst();
    //flist.takeFirst();

    QVector<bool> vec;
    vec.append(true);
    vec.append(false);
}

void testQVectorOfQList()
{
    QVector<QList<int> > v;
    QVector<QList<int> > *pv = &v;
    v.append(QList<int>() << 1);
    v.append(QList<int>() << 2 << 3);
    Q_UNUSED(pv);
}


class Goo
{
public:
   Goo(const QString& str, const int n) :
           str_(str), n_(n)
   {
   }
private:
   QString str_;
   int n_;
};

typedef QList<Goo> GooList;

void testNoArgumentName(int i, int, int k)
{
    GooList list;
    list.append(Goo("Hello", 1));
    list.append(Goo("World", 2));

    QList<Goo> list2;
    list2.append(Goo("Hello", 1));
    list2.append(Goo("World", 2));

    i = 1000;
    k = 2000;
    ++k;
    ++k;

}

void foo() {}
void foo(int) {}
void foo(QList<int>) {}
void foo(QList<QVector<int> >) {}
void foo(QList<QVector<int> *>) {}
void foo(QList<QVector<int *> *>) {}

template <class T>
void foo(QList<QVector<T> *>) {}

namespace {

namespace A { int barz() { return 42;} }
namespace B { int barz() { return 43;} }

}

namespace somespace {

class MyBase : public QObject
{
public:
    MyBase() {}
    virtual void doit(int i)
    {
       n = i;
    }
protected:
    int n;
};

namespace nested {

class MyFoo : public MyBase
{
public:
    MyFoo() {}
    virtual void doit(int i)
    {
       n = i;
    }
protected:
    int n;
};

class MyBar : public MyFoo
{
public:
    virtual void doit(int i)
    {
       n = i + 1;
    }
};

namespace {

class MyAnon : public MyBar
{
public:
    virtual void doit(int i)
    {
       n = i + 3;
    }
};

namespace baz {

class MyBaz : public MyAnon
{
public:
    virtual void doit(int i)
    {
       n = i + 5;
    }
};

} // namespace baz

} // namespace anon


} // namespace nested
} // namespace somespace

void testNamespace()
{
    using namespace somespace;
    using namespace nested;
    MyFoo foo;
    MyBar bar;
    MyAnon anon;
    baz::MyBaz baz;
    baz.doit(1);
    anon.doit(1);
    foo.doit(1);
    bar.doit(1);
    bar.doit(1);
    bar.doit(1);
    bar.doit(1);
    bar.doit(1);
    bar.doit(1);
    bar.doit(1);
}

void testHidden()
{
    int  n = 1;
    n = 2;
    n = 3;
    n = 3;
    n = 3;
    n = 3;
    n = 3;
    n = 3;
    n = 3;
    {
        QString n = "2";
        n = "3";
        n = "4";
        {
            double n = 3.5;
            ++n;
            ++n;
        }
        n = "3";
        n = "4";
    }
    ++n;
    ++n;
}

void testObject1()
{
    QObject parent;
    parent.setObjectName("A Parent");
    QObject child(&parent);
    child.setObjectName("A Child");
    QObject::connect(&child, SIGNAL(destroyed()), qApp, SLOT(quit()));
    QObject::disconnect(&child, SIGNAL(destroyed()), qApp, SLOT(quit()));
    child.setObjectName("A renamed Child");
}

void testVector1()
{
    std::vector<std::list<int> *> vector;
    std::list<int> list;
    vector.push_back(new std::list<int>(list));
    vector.push_back(0);
    list.push_back(45);
    vector.push_back(new std::list<int>(list));
    vector.push_back(0);
}

void testQHash1()
{
    QHash<QString, QList<int> > hash;
    hash.insert("Hallo", QList<int>());
    hash.insert("Welt", QList<int>() << 1);
    hash.insert("!", QList<int>() << 1 << 2);
    hash.insert("!", QList<int>() << 1 << 2);
}

void testPointer()
{
    Foo *f = new Foo();
    int i = 0;
    ++i;
    ++i;
    ++i;
}

class Z : public QObject
{
public:
    Z() {
        f = new Foo();
        i = 0;
        i = 1;
        i = 2;
        i = 3;
    }
    int i;
    Foo *f;
};

void testUninitialized()
{
    QString s;
    QStringList sl;
    QMap<int, int> mii;
    QMap<QString, QString> mss;
    QHash<int, int> hii;
    QHash<QString, QString> hss;
    QList<int> li;
    QVector<int> vi;
    QStack<int> si;

    std::string ss;
    std::map<int, int> smii;
    std::map<std::string, std::string> smss;
    std::list<int> sli;
    std::list<std::string> ssl;
    std::vector<int> svi;
    std::stack<int> ssi;
}

void testEndlessRecursion()
{
    testEndlessRecursion();
}

QString fooxx()
{
    return "bababa";
}

int testReference()
{
    QString a = "hello";
    const QString &b = fooxx();
    QString c = "world";
    return a.size() + b.size() + c.size();
}

int main(int argc, char *argv[])
{
    testPeekAndPoke3();
    testFunctionPointer();
    testAnonymous();
    testReference();
    //testEndlessRecursion();
    testQStack();
    testUninitialized();
    testPointer();
    testQDateTime();
    testQFileInfo();
    testObject1();
    testVector1();
    testQHash1();

    QString hallo = "hallo";
    QStringList list;
    list << "aaa" << "bbb" << "cc";

    QList<const char *> list2;
    list2 << "foo";
    list2 << "bar";
    list2 << 0;
    list2 << "baz";
    list2 << 0;

    testQStandardItemModel();
    testFunction();
    testQImage();
    testNoArgumentName(1, 2, 3);
    testIO();
    testHidden();
    testArray();
    testCatchThrow();
    testQByteArray();

    testStdDeque();
    testStdList();
    testStdMap();
    testStdSet();
    testStdStack();
    testStdString();
    testStdVector();

    testPlugin();
    testQList();
    testQLinkedList();
	char *s = "aöa";
	wchar_t *w = L"aöa";
    testNamespace();
    //return 0;
    testQHash();
    testQImage();
    testQMap();
    testQMultiMap();
    testQString();
    testQSet();
    #if QT_VERSION >= 0x040500
    testQSharedPointer();
    #endif
    testQStringList();
    testStruct();
    //testThreads();
    testQVariant1();
    testQVariant2();
    testQVariant3();
    testQVector();
    testQVectorOfQList();


    //*(int *)0 = 0;

    testQObject(argc, argv);


    //QColor color(255,128,10);
    //QFont font;

    while(true)
        ;

    return 0;
}

//Q_DECLARE_METATYPE(QHostAddress)
Q_DECLARE_METATYPE(QList<int>)

//#define COMMA ,
//Q_DECLARE_METATYPE(QMap<uint COMMA QStringList>)

QT_BEGIN_NAMESPACE

template <>
struct QMetaTypeId<QHostAddress>
{
    enum { Defined = 1 };
    static int qt_metatype_id()
    {
        static QBasicAtomicInt metatype_id = Q_BASIC_ATOMIC_INITIALIZER(0);
        if (!metatype_id)
             metatype_id = qRegisterMetaType<QHostAddress>
                ("myns::QHostAddress");
        return metatype_id;                                    \
    }                                                           \
};

template <>
struct QMetaTypeId< QMap<uint, QStringList> >
{
    enum { Defined = 1 };
    static int qt_metatype_id()
    {
        static QBasicAtomicInt metatype_id = Q_BASIC_ATOMIC_INITIALIZER(0);
        if (!metatype_id)
             metatype_id = qRegisterMetaType< QMap<uint, QStringList> >
                ("myns::QMap<uint, myns::QStringList>");
        return metatype_id;                                    \
    }                                                           \
};
QT_END_NAMESPACE

#include "app.moc"
