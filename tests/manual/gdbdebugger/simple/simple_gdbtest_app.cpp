/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QSettings>
#include <QtCore/QStack>
#include <QtCore/QThread>
#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtCore/QUrl>
#if QT_VERSION >= 0x040500
#include <QtCore/QSharedPointer>
#endif

#include <QtGui/QApplication>
#include <QtGui/QAction>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QLabel>
//#include <QtGui/private/qfixed_p.h>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QRegion>
#include <QtGui/QStandardItemModel>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>

#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

#include <QtNetwork/QHostAddress>

#include <deque>
#include <iostream>
#include <map>
#include <list>
#include <set>
#include <stack>
#include <string>
#include <vector>

#define USE_PRIVATE 1
//#define USE_BOOST 1

#if USE_BOOST
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#endif

#if USE_PRIVATE
#include <QtCore/private/qobject_p.h>
#endif

#if defined(__GNUC__) && !defined(__llvm__) && !defined(Q_OS_MAC)
#    define USE_GCC_EXT 1
#    undef __DEPRECATED
#    include <ext/hash_set>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#undef min
#undef max
#endif

#ifdef __SSE__
#include <xmmintrin.h>
#include <stddef.h>
#endif
Q_DECLARE_METATYPE(QHostAddress)
Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QStringList)

typedef QMap<uint, QStringList> MyType;
#define COMMA ,
Q_DECLARE_METATYPE(QMap<uint COMMA QStringList>)

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

#if USE_PRIVATE

class DerivedObjectPrivate : public QObjectPrivate
{
public:
    DerivedObjectPrivate()
    {
        m_extraX = 43;
        m_extraY.append("xxx");
        m_extraZ = 1;
    }

    int m_extraX;
    QStringList m_extraY;
    uint m_extraZ : 1;
    bool m_extraA : 1;
    bool m_extraB;
};

class DerivedObject : public QObject
{
    Q_OBJECT

public:
    DerivedObject()
       : QObject(*new DerivedObjectPrivate, 0)
    {}

    Q_PROPERTY(int x READ x WRITE setX)
    Q_PROPERTY(QStringList y READ y WRITE setY)
    Q_PROPERTY(uint z READ z WRITE setZ)

    int x() const;
    void setX(int x);
    QStringList y() const;
    void setY(QStringList y);
    uint z() const;
    void setZ(uint z);

private:
    Q_DECLARE_PRIVATE(DerivedObject)
};

int DerivedObject::x() const
{
    Q_D(const DerivedObject);
    return d->m_extraX;
}

void DerivedObject::setX(int x)
{
    Q_D(DerivedObject);
    d->m_extraX = x;
    d->m_extraA = !d->m_extraA;
    d->m_extraB = !d->m_extraB;
}

QStringList DerivedObject::y() const
{
    Q_D(const DerivedObject);
    return d->m_extraY;
}

void DerivedObject::setY(QStringList y)
{
    Q_D(DerivedObject);
    d->m_extraY = y;
}

uint DerivedObject::z() const
{
    Q_D(const DerivedObject);
    return d->m_extraZ;
}

void DerivedObject::setZ(uint z)
{
    Q_D(DerivedObject);
    d->m_extraZ = z;
}

#endif

struct S
{
    uint x : 1;
    uint y : 1;
    bool c : 1;
    bool b;
    float f;
    double d;
    qreal q;
    int i;
};

void testPrivate()
{
    S s;
    s.x = 1;
#if USE_PRIVATE
    DerivedObject ob;
    ob.setX(23);
    ob.setX(25);
    ob.setX(26);
    ob.setX(63);
    ob.setX(32);
#endif
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

    virtual ~Foo()
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

class X : public Foo
{
public:
    X() {
    }
};

class XX : virtual public Foo
{
public:
    XX() {
    }
};

class Y : virtual public Foo
{
public:
    Y() {
    }
};

class D : public X, public Y
{
    int diamond;
};

void testArray()
{
#if 1
    X x;
    XX xx;
    D diamond;
    Q_UNUSED(diamond);
    Foo *f = &xx;
    Q_UNUSED(f);
    Foo ff;
    Q_UNUSED(ff);
    double d[3][3];
    for (int i = 0; i != 3; ++i)
        for (int j = 0; j != 3; ++j)
            d[i][j] = i + j;
    Q_UNUSED(d);
#endif

#if 1
    char c[20];
    c[0] = 'a';
    c[1] = 'b';
    c[2] = 'c';
    c[3] = 'd';
    Q_UNUSED(c);
#endif

#if 1
    QString s[20];
    s[0] = "a";
    s[1] = "b";
    s[2] = "c";
    s[3] = "d";
    Q_UNUSED(s);
#endif

#if 1
    QByteArray b[20];
    b[0] = "a";
    b[1] = "b";
    b[2] = "c";
    b[3] = "d";
    Q_UNUSED(b);
#endif

#if 1
    Foo foo[10];
    //for (int i = 0; i != sizeof(foo)/sizeof(foo[0]); ++i) {
    for (int i = 0; i < 5; ++i) {
        foo[i].a = i;
        foo[i].doit();
    }
#endif
}

#ifndef Q_CC_RVCT
struct TestAnonymous
{
    union {
        struct { int i; int b; };
        struct { float f; };
        double d;
    };
};
#endif

void testPeekAndPoke3()
{
    // Anonymous structs
    {
#ifndef Q_CC_RVCT
        union {
            struct { int i; int b; };
            struct { float f; };
            double d;
        } a = { { 42, 43 } };
        a.i = 1; // Break here. Expand a. Step.
        a.i = 2; // Change a.i in Locals view to 0. This changes f, d but expectedly not b. Step.
        a.i = 3; // Continue.
        Q_UNUSED(a);
#endif
    }

    // Complex watchers
    {
        struct S { int a; double b; } s[10];
        for (int i = 0; i != 10; ++i) {
            s[i].a = i;  // Break here. Expand s and s[0]. Step.
            // Watcher Context: "Add New Watcher".
            // Type    ['s[%d].a' % i for i in range(5)]
            // Expand it, continue stepping. This should result in a list
            // of five items containing the .a fields of s[0]..s[4].
        }
        Q_UNUSED(s);
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


#ifndef Q_CC_RVCT
namespace { // anon

struct Something
{
    Something() { a = b = 1; }

    void foo()
    {
        a = 42;
        b = 43;
    }

    int a, b;
};

} // anon
#endif


void testAnonymous()
{
#ifndef Q_CC_RVCT
    TestAnonymous a;
    a.i = 1;
    a.i = 2;
    a.i = 3;
    Q_UNUSED(a);

    Something s;
    s.foo();
    Q_UNUSED(s);
#endif
}

typedef void (*func_t)();
func_t testFunctionPointer()
{
    func_t f1 = testAnonymous;
    func_t f2 = testPeekAndPoke3;
    func_t f3 = testPeekAndPoke3;
    Q_UNUSED(f1);
    Q_UNUSED(f2);
    Q_UNUSED(f3);
    return f1;
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

int testQByteArray2()
{
    QByteArray ba;
    for (int i = 256; --i >= 0; )
        ba.append(char(i));
    return ba.size();
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
    // Set a breakpoint on "throw" in the BreakWindow context menu
    // before stepping further.
    int gotit = 0;
    try {
        throwit();
    } catch (int what) {
        gotit = what;
    }
    return gotit;
}

void testQDate()
{
    QDate date;
    date = QDate::currentDate();
    date = date.addDays(5);
    date = date.addDays(5);
    date = date.addDays(5);
    date = date.addDays(5);
}

void testQTime()
{
    QTime time;
    time = QTime::currentTime();
    time = time.addSecs(5);
    time = time.addSecs(5);
    time = time.addSecs(5);
    time = time.addSecs(5);
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

QFileInfo testQFileInfo()
{
    QFileInfo fi("/tmp/t");
    QString s = fi.absoluteFilePath();
    s = fi.bundleName();
    s = fi.bundleName();
    s = fi.bundleName();

    QFileInfo result("/tmp/t");
    return result;
}

void testQFixed()
{
/*
    QFixed f = QFixed::fromReal(4.2);
    f += 1;
    f += 1;
    f *= -1;
    f += 1;
    f += 1;
*/
}

QHash<int, float> testQHash()
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
    QHash<int, float> result;
    return result;
}

void testQImage()
{
    // only works with Python dumper
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

void testOutput()
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

void testInput()
{
#if 0
    // This works only when "Run in terminal" is selected
    // in the Run Configuration.
    int i;
    std::cin >> i;
    int j;
    std::cin >> j;
#endif
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

void testQLocale()
{
    QLocale loc = QLocale::system();
    //QString s = loc.name();
    //QVariant v = loc;
    QLocale::MeasurementSystem m = loc.measurementSystem();
    Q_UNUSED(m);
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

namespace A {
namespace B {

struct SomeType
{
    SomeType(int a) : a(a) {}
    int a;
};

} // namespace B
} // namespace A

void testQMap()
{
#if 0
    QMap<uint, QStringList> ggl;
    ggl[11] = QStringList() << "11";
    ggl[22] = QStringList() << "22";

    typedef QMap<uint, QStringList> T;
    T ggt;
    ggt[11] = QStringList() << "11";
    ggt[22] = QStringList() << "22";
#endif

#if 0
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

#if 1
    QList<A::B::SomeType *> x;
    x.append(new A::B::SomeType(1));
    x.append(new A::B::SomeType(2));
    x.append(new A::B::SomeType(3));
    QMap<QString, QList<A::B::SomeType *> > mlt;
    mlt["foo"] = x;
    mlt["bar"] = x;
    mlt["1"] = x;
    mlt["2"] = x;
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
        m_ui->w = new QWidget;
        Q_UNUSED(parent);
    }

    Q_PROPERTY(QString myProp1 READ myProp1 WRITE setMyProp1)
    QString myProp1() const { return m_myProp1; }
    Q_SLOT void setMyProp1(const QString &mt) { m_myProp1 = mt; }

    Q_PROPERTY(QString myProp2 READ myProp2 WRITE setMyProp2)
    QString myProp2() const { return m_myProp2; }
    Q_SLOT void setMyProp2(const QString &mt) { m_myProp2 = mt; }

public:
    Ui *m_ui;
    QString m_myProp1;
    QString m_myProp2;
};

} // namespace Bar
} // namespace Names

void testQObject(int &argc, char *argv[])
{
    QApplication app(argc, argv);
    //QString longString = QString(10000, QLatin1Char('A'));
#if 1
    Names::Bar::TestObject test;
    test.setMyProp1("HELLO");
    test.setMyProp2("WORLD");
    QString s = test.myProp1();
    s += test.myProp2();
    Q_UNUSED(s);
#endif

#if 0
    QAction act("xxx", &app);
    QString t = act.text();
    t += "y";
    t += "y";
    t += "y";
    t += "y";
    t += "y";
#endif

#if 1
    QWidget ob;
    ob.setObjectName("An Object");
    ob.setProperty("USER DEFINED 1", 44);
    ob.setProperty("USER DEFINED 2", QStringList() << "FOO" << "BAR");
    QObject ob1;
    ob1.setObjectName("Another Object");

    QObject::connect(&ob, SIGNAL(destroyed()), &ob1, SLOT(deleteLater()));
    QObject::connect(&ob, SIGNAL(destroyed()), &ob1, SLOT(deleteLater()));
    //QObject::connect(&app, SIGNAL(lastWindowClosed()), &ob, SLOT(deleteLater()));
#endif

#if 0
    QList<QObject *> obs;
    obs.append(&ob);
    obs.append(&ob1);
    obs.append(0);
    obs.append(&app);
    ob1.setObjectName("A Subobject");
#endif

#if 1
    QString str = QString::fromUtf8("XXXXXXXXXXXXXXyyXXX ö");
    QLabel l(str);
    l.setObjectName("Some Label");
    l.show();
    app.exec();
#endif
}

class Sender : public QObject
{
    Q_OBJECT
public:
    Sender() { setObjectName("Sender"); }
    void doEmit() { emit aSignal(); }
signals:
    void aSignal();
};

class Receiver : public QObject
{
    Q_OBJECT
public:
    Receiver() { setObjectName("Receiver"); }
public slots:
    void aSlot() {
        QObject *s = sender();
        if (s) {
            qDebug() << "SENDER: " << s;
        } else {
            qDebug() << "NO SENDER";
        }
    }
};

void testSignalSlot(int &argc, char *argv[])
{
    QApplication app(argc, argv);
    Sender sender;
    Receiver receiver;
    QObject::connect(&sender, SIGNAL(aSignal()), &receiver, SLOT(aSlot()));
    sender.doEmit();
};


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

void testQRegion()
{
    // only works with Python dumper
    QRegion region;
    region += QRect(100, 100, 200, 200);
    region += QRect(300, 300, 400, 500);
    region += QRect(500, 500, 600, 600);
    region += QRect(500, 500, 600, 600);
    region += QRect(500, 500, 600, 600);
    region += QRect(500, 500, 600, 600);
}


void testPlugin()
{
    QString dir = QDir::currentPath();
#ifdef Q_OS_LINUX
    QLibrary lib(dir + "/libsimple_gdbtest_plugin.so");
#endif
#ifdef Q_OS_MAC
    dir = QFileInfo(dir + "/../..").canonicalPath();
    QLibrary lib(dir + "/libsimple_gdbtest_plugin.dylib");
#endif
#ifdef Q_OS_WIN
    QLibrary lib(dir + "/debug/simple_gdbtest_plugin.dll");
#endif
#ifdef Q_OS_SYMBIAN
    QLibrary lib(dir + "/libsimple_gdbtest_plugin.dll");
#endif
    int (*foo)() = (int(*)()) lib.resolve("pluginTest");
    qDebug() << "library resolve: " << foo << lib.fileName();
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
    Q_UNUSED(ptr);
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
#if 0
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
#endif

    QSharedPointer<Foo> fptr(new Foo(1));
    QWeakPointer<Foo> wfptr(fptr);
    QWeakPointer<Foo> wfptr2 = wfptr;
    QWeakPointer<Foo> wfptr3 = wfptr;
}
#endif

void stringRefTest(const QString &refstring)
{
    Q_UNUSED(refstring);
}

void testStdDeque()
{
    // This is not supposed to work with the compiled dumpers.
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

void testStdHashSet()
{
    // This is not supposed to work with the compiled dumpers.
#if USE_GCC_EXT
    using namespace __gnu_cxx;
    hash_set<int> h;
    h.insert(1);
    h.insert(194);
    h.insert(2);
    h.insert(3);
    h.insert(4);
    h.insert(5);
#endif
}

std::list<int> testStdList()
{
    // This is not supposed to work with the compiled dumpers.
    std::list<int> big;
    for (int i = 0; i < 10000; ++i)
        big.push_back(i);

    std::list<Foo> flist;
    for (int i = 0; i < 100; ++i)
        flist.push_back(i + 15);

#if 1
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
    std::list<int> result;
    return result;
}

void testStdMap()
{
    // This is not supposed to work with the compiled dumpers.
#if 0
    std::map<QString, Foo> gg3;
    gg3["22.0"] = Foo(22);
    gg3["33.0"] = Foo(33);
    gg3["44.0"] = Foo(44);

    std::map<const char *, Foo> m1;
    m1["22.0"] = Foo(22);
    m1["33.0"] = Foo(33);
    m1["44.0"] = Foo(44);

    std::map<uint, uint> gg;
    gg[11] = 1;
    gg[22] = 2;
    gg[33] = 3;
    gg[44] = 4;
    gg[55] = 5;

    std::pair<uint, QStringList> p = std::make_pair(3, QStringList() << "11");
    std::vector< std::pair<uint, QStringList> > v;
    v.push_back(p);
    v.push_back(p);

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
#endif

#if 1
    QObject ob;
    std::map<QString, QPointer<QObject> > map;
    map["Hallo"] = QPointer<QObject>(&ob);
    map["Welt"] = QPointer<QObject>(&ob);
    map["."] = QPointer<QObject>(&ob);
#endif
}

std::set<int> testStdSet()
{
    // This is not supposed to work with the compiled dumpers.
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
    std::set<int> result;
    return result;
}

std::stack<int> testStdStack()
{
    // This is not supposed to work with the compiled dumpers.
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

    std::stack<int> result;
    return result;
}

std::string testStdString()
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

    std::string result = "hi";
    return result;
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
    vec.push_back(false);
    vec.push_back(true);
    vec.push_back(false);
}

void testQStandardItemModel()
{
    //char buf[100];
    //QString *s = static_cast<QString *>(static_cast<void *>(&(v.data_ptr().data.c)));
    //QString *t = (QString *)&(v.data_ptr());

    QStandardItemModel m;
    QStandardItem *i1, *i2, *i11;
    m.appendRow(QList<QStandardItem *>()
         << (i1 = new QStandardItem("1")) << (new QStandardItem("a")) << (new QStandardItem("a2")));
    QModelIndex mi = i1->index();
    m.appendRow(QList<QStandardItem *>()
         << (i2 = new QStandardItem("2")) << (new QStandardItem("b")));
    i1->appendRow(QList<QStandardItem *>()
         << (i11 = new QStandardItem("11")) << (new QStandardItem("aa")));
    int i = 1;
    ++i;
    ++i;
}

QStack<int> testQStack()
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
    QStack<int> result;
    return result;
}


void testQUrl()
{
    QUrl url(QString("http://www.nokia.com"));
    (void) url;
}


#ifdef FOP

int xxxx()
{
}

#else

void xxxx()
{
}


#endif


void testQString()
{
    QImage im;

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

QStringList testQStringList()
{
    QStringList l;
    l << "Hello ";
    l << " big, ";
    l << " fat ";
    l.takeFirst();
    l << " World ";
    QStringList result;
    return result;
}

Foo testStruct()
{
    Foo f(2);
    f.doit();
    f.doit();
    f.doit();
    Foo f1 = f;
    f1.doit();
    return f1;
}

void testTypeFormats()
{
    // These tests should result in properly displayed umlauts in the
    // Locals&Watchers view. It is only support on gdb with Python.

    // Select UTF-8 in "Change Format for Type" in L&W context menu.
    const char *s = "aöa";

    // Windows: Select UTF-16 in "Change Format for Type" in L&W context menu.
    // Other: Select UCS-6 in "Change Format for Type" in L&W context menu.
    const wchar_t *w = L"aöa";
    QString u;
    if (sizeof(wchar_t) == 4)
        u = QString::fromUcs4((uint *)w);
    else
        u = QString::fromUtf16((ushort *)w);

    // Make sure to undo "Change Format".
    int dummy = 1;
    Q_UNUSED(s);
    Q_UNUSED(w);
    Q_UNUSED(dummy);
}


class Thread : public QThread
{
public:
    Thread(int id) : m_id(id) {}

    void run()
    {
        int j = 2;
        ++j;
        for (int i = 0; i != 100000; ++i) {
            //sleep(1);
            std::cerr << m_id;
        }
        if (m_id == 2) {
            ++j;
        }
        std::cerr << j;
    }

private:
    int m_id;
};

void testQTextCursor()
{
    //int argc = 0;
    //char *argv[] = { "xxx", 0 };
    //QApplication app(argc, argv);
    QTextDocument doc;
    doc.setPlainText("Hallo\nWorld");
    QTextCursor tc;
    tc = doc.find("all");
    int pos = tc.position();
    int anc = tc.anchor();
    Q_UNUSED(pos);
    Q_UNUSED(anc);
}

void testQThread()
{
    Thread thread1(1);
    Thread thread2(2);
    thread1.setObjectName("This is the first thread");
    thread2.setObjectName("This is another thread");
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
    v = QRect(100, 200, 300, 400);
    v = QRectF(100, 200, 300, 400);
    v = 1;
    //return v;
}

QVariant testQVariant2()
{
    QVariant value;
    QVariant::Type t = QVariant::String;
    value = QVariant(t, (void*)0);
    *(QString*)value.data() = QString("XXX");

    int i = 1;
    ++i;
    ++i;
    ++i;
#if 1
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
#if 1
    QVariant var3;
    QHostAddress ha("127.0.0.1");
    var.setValue(ha);
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
    // FIXME: Known to break
    QString type = var.typeName();
    var.setValue(my);
    var.setValue(my);
    var.setValue(my);
    var.setValue(my);
#endif
    QVariant result("sss");
    return result;
}

QVariant testQVariant3()
{
    QVariantList vl;
    vl.append(QVariant(1));
    vl.append(QVariant(2));
    vl.append(QVariant("Some String"));
    vl.append(QVariant(21));
    vl.append(QVariant(22));
    vl.append(QVariant("2Some String"));

    QList<int> list;
    list << 1 << 2 << 3;
    QVariant variant = qVariantFromValue(list);
    list.clear();
    list = qVariantValue<QList<int> >(variant);

    QVariant result("xxx");
    return result;
}

void testQVector()
{
    QVector<int> big(10000);
    big[1] = 1;
    big[2] = 2;
    big[3] = 3;
    big[4] = 4;
    big[5] = 5;
    big[9] = 9;
    big.append(1);
    big.append(1);
    big.append(1);
    big.append(1);

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
    //return v;
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
    // This is not supposed to work with the compiled dumpers.
    GooList list;
    list.append(Goo("Hello", 1));
    list.append(Goo("World", 2));

    QList<Goo> list2;
    list2.append(Goo("Hello", 1));
    list2.append(Goo("World", 2));

    ++i;
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
    n = 4;
    n = 4;
    n = 5;
    n = 6;
    n = 7;
    n = 8;
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
    Q_UNUSED(f);
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

void testMemoryView()
{
    int a[20];
    for (int i = 0; i != 20; ++i)
        a[i] = i;
}

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

int testEndlessLoop()
{
    qlonglong a = 1;
    // gdb:
    // Breakpoint at "while" will stop only once
    // Hitting "Pause" button might show backtrace of different thread
    while (a > 0)
        ++a;
    return a;
}

QString fooxx()
{
    return "bababa";
}

int testReference()
{
    QString a = "hello";
    const QString &b = fooxx();
    const QString c = "world";
    return a.size() + b.size() + c.size();
}

struct Color
{
    int r,g,b,a;
    Color() { r = 1, g = 2, b = 3, a = 4; }
};

void testColor()
{
    Color c;
    c.r = 5;
}

int fooii()
{
    return 3;
}

typedef QVector<Foo> FooVector;

FooVector fooVector()
{
    FooVector f;
    f.append(Foo(2));
    fprintf(stderr, "xxx\n");
    f.append(Foo(3));
    f.append(Foo(4));
    for (int i = 0; i < 1000; ++i)
        f.append(Foo(i));
    return f;
}

namespace ns {
    typedef unsigned long long vl;
    typedef vl verylong;
}

void testTypedef()
{
    typedef quint32 myType1;
    typedef unsigned int myType2;
    myType1 t1 = 0;
    myType2 t2 = 0;
    ns::vl j = 1000;
    ns::verylong k = 1000;

    ++j;
    ++k;
    ++t1;
    ++t2;
}

void testConditional(const QString &str)
{
    //
    QString res = str;
    res += "x";
    res += "x";
    res += "x";
}

void testChar()
{
    char s[5];
    s[0] = 0;
    strcat(s,"\""); // add a quote
    strcat(s,"\""); // add a quote
    strcat(s,"\""); // add a quote
    strcat(s,"\""); // add a quote
}

struct Tx
{
    Tx() { data = new char[20](); data[0] = '1'; }

    char *GetStringPtr() const
    {
        return data;
    }

    char *data;
};

struct Ty
{
    void doit()
    {
        int i = 1;
        i = 2;
        i = 2;
        i = 2;
        i = 2;
    }

    Tx m_buffer;
};

void testStuff()
{
    using namespace std;
    typedef map<string, list<string> > map_t;
    map_t m;
    m["one"].push_back("a");
    m["one"].push_back("b");
    m["one"].push_back("c");
    m["two"].push_back("1");
    m["two"].push_back("2");
    m["two"].push_back("3");
    map_t::const_iterator i = m.begin();

    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    pair<vector<int>, vector<int> > a(pair<vector<int>,vector<int> >(vec, vec));

    i++;
}

void testStuff4()
{
    Ty x;
    x.doit();
    char *s = x.m_buffer.GetStringPtr();
    Q_UNUSED(s);
}

void testStuff3()
{
    typedef unsigned char byte;
    byte f = '2';
    Q_UNUSED(f);
    testConditional("foo");
    testConditional(fooxx());
    testConditional("bar");
    testConditional("zzz");
    Foo *f1 = new Foo(1);
    X *x = new X();
    Foo *f2 = x;
    XX *xx = new XX();
    Foo *f3 = xx;
    Y *y = new Y();
    Foo *f4 = y;
    //Foo *f5 = new D();
    qDebug() << f1 << f2 << f3 << f4;
}

void testStuff2()
{
    QList<QList<int> > list1;
    QList<QList<int> > list2;
    list1.append(QList<int>() << 1);
    list2.append(QList<int>() << 2);
    for (int i = 0; i < list1.size(); ++i) {
        for (int j = 0; j < list1.at(i).size(); ++j) {
            for (int k = i; k < list1.size(); ++k) {
                for (int l = j; l < list1.at(k).size(); ++l) {
                    qDebug() << list1.at(i).at(j)+list2.at(k).at(l);
                }
            }
        }
    }

    // special char* support only with Python.
    typedef unsigned short wchar;
    wchar *str = new wchar[10];
    str[2] = 0;
    str[1] = 'B';
    str[0] = 'A';
    str[0] = 'A';
    str[0] = 'A';
    str[0] = 'A';
    QString foo = "foo";
    wchar *f = (wchar*)foo.utf16();
    Q_UNUSED(f);
    str[0] = 'A';
    str[0] = 'A';
    str[0] = 'A';
}

void testPassByReferenceHelper(Foo &f)
{
    ++f.a;
}

void testPassByReference()
{
    Foo f(12);
    testPassByReferenceHelper(f);
}

void testWCout()
{
    using namespace std;
    wstring x = L"xxxxx";
    wstring::iterator i = x.begin();
    while (i != x.end()) {
        wcout << *i;
        i++;
    }
    wcout.flush();
    string y = "yyyyy";
    string::iterator j = y.begin();
    while (j != y.end()) {
        cout << *j;
        j++;
    }
    cout.flush();
};

void testWCout0()
{
    // Mixing cout and wcout does not work with gcc.
    // See http://gcc.gnu.org/ml/gcc-bugs/2006-05/msg01193.html
    // which also says "you can obtain something close to your
    // expectations by calling std::ios::sync_with_stdio(false);
    // at the beginning of your program."

    using namespace std;
    //std::ios::sync_with_stdio(false);
    wcout << L"WWWWWW" << endl;
    wcerr << L"YYYYYY" << endl;
    cout << "CCCCCC" << endl;
    cerr << "EEEEEE" << endl;
    wcout << L"WWWWWW" << endl;
    wcerr << L"YYYYYY" << endl;
    cout << "CCCCCC" << endl;
    cerr << "EEEEEE" << endl;
    wcout << L"WWWWWW" << endl;
    wcerr << L"YYYYYY" << endl;
    cout << "CCCCCC" << endl;
    cerr << "EEEEEE" << endl;
}

void testSSE()
{
#ifdef __SSE__
    float a[4];
    float b[4];
    int i;
    for (i = 0; i < 4; i++) {
        a[i] = 2 * i;
        b[i] = 2 * i;
    }
    __m128 sseA, sseB;
    sseA = _mm_loadu_ps(a);
    sseB = _mm_loadu_ps(b);
    ++i;
#endif
}

void testQSettings()
{
    // Note: Construct a QCoreApplication first.
    QSettings settings("/tmp/test.ini", QSettings::IniFormat);
    QVariant value = settings.value("item1","").toString();
    int x = 1;
    Q_UNUSED(x);
}

void testQScriptValue(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QScriptEngine engine;
    QDateTime date = QDateTime::currentDateTime();
    QScriptValue s;
    s = QScriptValue(33);
    int x = s.toInt32();

    s = QScriptValue(QString("34"));
    QString x1 = s.toString();

    s = engine.newVariant(QVariant(43));
    QVariant v = s.toVariant();

    s = engine.newVariant(QVariant(43.0));
    s = engine.newVariant(QVariant(QString("sss")));
    s = engine.newDate(date);
    x = s.toInt32();
    bool xx = s.isDate();
    Q_UNUSED(xx);
    date = s.toDateTime();
    s.setProperty("a", QScriptValue());
    QScriptValue d = s.data();
}

void testBoostOptional()
{
#if USE_BOOST
    boost::optional<int> i;
    i = 1;
    boost::optional<QStringList> sl;
    sl = (QStringList() << "xxx" << "yyy");
    sl.get().append("zzz");
    i = 3;
    i = 4;
    i = 5;
#endif
}

void testBoostSharedPtr()
{
#if USE_BOOST
    QSharedPointer<int> qs;
    QSharedPointer<int> qi(new int(43));
    QSharedPointer<int> qj = qi;

    boost::shared_ptr<int> s;
    boost::shared_ptr<int> i(new int(43));
    boost::shared_ptr<int> j = i;
    boost::shared_ptr<QStringList> sl(new QStringList);
    int k = 2;
    ++k;
#endif
}

void testFork()
{
    QProcess proc;
    proc.start("/bin/ls");
    proc.waitForFinished();
    QByteArray ba = proc.readAllStandardError();
    ba.append('x');
    ba.append('x');
}

int main(int argc, char *argv[])
{
    testPrivate();
    testMemoryView();
    //testQSettings();
    //testWCout0();
    //testWCout();
    testSSE();
    testQLocale();
    testColor();
    testQRegion();
    testTypedef();
    testChar();
    testStuff();
    testPeekAndPoke3();
    testFunctionPointer();
    testAnonymous();
    testReference();
    //testEndlessLoop();
    //testEndlessRecursion();
    testQStack();
    testUninitialized();
    testPointer();
    testQDate();
    testQDateTime();
    testQTime();
    testQFileInfo();
    testQFixed();
    testObject1();
    testVector1();
    testQHash1();
    testSignalSlot(argc, argv);

    QString hallo = "hallo\nwelt";
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
    testQTextCursor();
    testInput();
    testOutput();
    testHidden();
    testArray();
    testCatchThrow();
    testQByteArray();
    testQByteArray2();

    testStdDeque();
    testStdList();
    testStdHashSet();
    testStdMap();
    testStdSet();
    testStdStack();
    testStdString();
    testStdVector();
    testTypeFormats();

    testPassByReference();
    testPlugin();
    testQList();
    testQLinkedList();
    testNamespace();
    //return 0;
    testQHash();
    testQImage();
    testQMap();
    testQMultiMap();
    testQString();
    testQUrl();
    testQSet();
#    if QT_VERSION >= 0x040500
    testQSharedPointer();
#    endif
    testQStringList();
    testQScriptValue(argc, argv);
    testStruct();
    //testQThread();
    testQVariant1();
    testQVariant2();
    testQVariant3();
    testQVector();
    testQVectorOfQList();

    testBoostOptional();
    testBoostSharedPtr();

    testFork();

    testQObject(argc, argv);

    //QColor color(255,128,10);
    //QFont font;
    return 0;
}

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

#include "simple_gdbtest_app.moc"
