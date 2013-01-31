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

////////////////  Some global configuration below ////////////////


// The following defines can be used to steer the kind of tests that
// can be done.

// With USE_AUTORUN, creator will automatically "execute" the commands
// in a comment following a BREAK_HERE line.
// The following commands are supported:
//   // Check <name> <value> <type>
//         - Checks whether the local variable is displayed with value and type.
//   // CheckType <name> <type>
//         - Checks whether the local variable is displayed with type.
//           The value is untested, so it can be used with pointers values etc.
//           that would change between test runs
//   // Continue
//       - Continues execution
// On the TODO list:
//   // Expand <name1>[ <name2> ...].
//         - Expands local variables with given names.
//           There should be at most one "Expand" line per BREAK_HERE,
//           and this should placed on the line following the BREAK_HERE
//           FIXME: Not implemented yet.


// Value: 1
// If the line after a BREAK_HERE line does not contain one of the
// supported commands, the test stops.
// Value: 2
// Same as 1, except that the debugger will stop automatically when
// a test after a BREAK_HERE failed
// Default: 0
// Before using this, make sure that "Show a message box when receiving a signal"
// is disabled in "Tools" -> "Options..." -> "Debugger" -> "GDB".
#ifndef USE_AUTORUN
#define USE_AUTORUN 0
#endif

// With USE_AUTOBREAK, the debugger will stop automatically on all
// lines containing the BREAK_HERE macro. This should be enabled
// during manual testing.
// Default: 0
#ifndef USE_AUTOBREAK
#define USE_AUTOBREAK 0
#endif

// With USE_UNINITIALIZED_AUTOBREAK, the debugger will stop automatically
// on all lines containing the BREAK_UNINITIALIZED_HERE macro.
// This should be enabled during manual testing.
// Default: 0
#ifndef USE_UNINITIALIZED_AUTOBREAK
#define USE_UNINITIALIZED_AUTOBREAK 0
#endif


////////////// No further global configuration below ////////////////

// AUTORUN is only sensibly with AUTOBREAK and without UNINITIALIZED_AUTOBREAK
#if USE_AUTORUN
#if !(USE_AUTOBREAK)
#undef USE_AUTOBREAK
#define USE_AUTOBREAK 1
#pragma message ("Switching on USE_AUTOBREAK")
#endif // !USE_AUTOBREAK
#if USE_UNINITIALIZED_AUTOBREAK
#undef USE_UNINITIALIZED_AUTOBREAK
#define USE_UNINITIALIZED_AUTOBREAK 0
#pragma message ("Switching off USE_UNINITIALIZED_AUTOBREAK")
#endif // USE_UNINITIALIZED_AUTOBREAK
#endif

#ifdef QT_SCRIPT_LIB
#define USE_SCRIPTLIB 1
#else
#define USE_SCRIPTLIB 0
#endif

#ifdef QT_WEBKIT_LIB
#define USE_WEBKITLIB 1
#else
#define USE_WEBKITLIB 0
#endif

#if QT_VERSION >= 0x040500
#define USE_SHARED_POINTER 1
#else
#define USE_SHARED_POINTER 0
#endif

void dummyStatement(...) {}

#if USE_CXX11 && defined(__GNUC__) && defined(__STRICT_ANSI__)
#undef __STRICT_ANSI__ // working around compile error with MinGW
#endif

#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QLibrary>
#include <QLinkedList>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QProcess>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QStack>
#include <QThread>
#include <QVariant>
#include <QVector>
#include <QUrl>
#if USE_SHARED_POINTER
#include <QSharedPointer>
#endif

#if USE_GUILIB
#include <QAction>
#include <QApplication> // QWidgets: Separate module as of Qt 5
#include <QColor>
#include <QFont>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QRegion>
#include <QStandardItemModel>
#include <QTextCursor>
#include <QTextDocument>
#endif

#if USE_SCRIPTLIB
#include <QScriptEngine>
#include <QScriptValue>
#endif

#if USE_WEBKITLIB
#include <QWebPage>
#endif

#include <QXmlAttributes>

#include <QHostAddress>
#include <QNetworkRequest>

#if USE_CXX11
#include <array>
#endif
#include <complex>
#include <deque>
#include <iostream>
#include <iterator>
#include <fstream>
#include <map>
#include <memory>
#include <list>
#include <limits>
#include <set>
#include <stack>
#include <string>
#include <vector>

#include <stdarg.h>

#include "../simple/deep/deep/simple_test_app.h"


#if USE_BOOST
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bimap.hpp>
#endif

#if USE_EIGEN
#include <eigen2/Eigen/Core>
#endif

#if USE_PRIVATE
#include <private/qobject_p.h>
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

#if USE_AUTOBREAK
#   ifdef Q_CC_MSVC
#       include <crtdbg.h>
#       define BREAK_HERE _CrtDbgReport(_CRT_WARN, NULL, NULL, "simple_test_app", NULL)
#   else
#       define BREAK_HERE asm("int $3; mov %eax, %eax")
#   endif
#else
#   define BREAK_HERE dummyStatement()
#endif

#if USE_UNINITIALIZED_AUTOBREAK
#   ifdef Q_CC_MSVC
#       include <crtdbg.h>
#       define BREAK_UNINITIALIZED_HERE _CrtDbgReport(_CRT_WARN, NULL, NULL, "simple_test_app", NULL)
#   else
#       define BREAK_UNINITIALIZED_HERE asm("int $3; mov %eax, %eax")
#   endif
#else
#   define BREAK_UNINITIALIZED_HERE dummyStatement()
#endif


QT_BEGIN_NAMESPACE
uint qHash(const QMap<int, int> &) { return 0; }
uint qHash(const double & f) { return int(f); }
uint qHash(const QPointer<QObject> &p) { return (ulong)p.data(); }
QT_END_NAMESPACE


namespace nsA {
namespace nsB {

struct SomeType
{
    SomeType(int a) : a(a) {}
    int a;
};

} // namespace nsB
} // namespace nsA


struct BaseClass
{
    BaseClass() : a(1) {}
    virtual ~BaseClass() {}
    virtual int foo() { return a; }
    int a;
};

struct DerivedClass : BaseClass
{
    DerivedClass() : b(2) {}
    int foo() { return b; }
    int b;
};

namespace multibp {

    // This tests multiple breakpoints.
    template <typename T> class Vector
    {
    public:
        explicit Vector(int size)
            : m_size(size), m_data(new T[size])
        {
            BREAK_HERE;
            // Check size 10 int.
            // Continue.
            // Manual: Add a breakpoint in the constructor
            // Manual: Check there are multiple entries in the Breakpoint view.
            dummyStatement(this);
        }
        ~Vector() { delete [] m_data; }
        int size() const { return m_size; }
    private:
        int m_size;
        T *m_data;
    };

    void testMultiBp()
    {
        Vector<int> vi(10);
        Vector<float> vf(10);
        Vector<double> vd(10);
        Vector<char> vc(10);
        dummyStatement(&vi, &vf, &vd, &vc);
    }

} // namespace multibp




class Foo
{
public:
    Foo(int i = 0)
        : a(i), b(2)
    {
        int s = 1;
        int t = 2;
        b = 2 + s + t;
        dummyStatement(&s, &t);
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
        dummyStatement(&a, &b);
    }

public:
    int a, b;
    char x[6];

private:
    typedef QMap<QString, QString> Map;
    Map m;
    QHash<QObject *, Map::iterator> h;
};

class Fooooo : public Foo
{
public:
    Fooooo(int x) : Foo(x), a(x + 2) {}
    int a;
};

class X : virtual public Foo { public: X() { } };

class XX : virtual public Foo { public: XX() { } };

class Y : virtual public Foo { public: Y() { } };

class D : public X, public Y { int diamond; };


namespace peekandpoke {

    void testAnonymousStructs()
    {
        #ifndef Q_CC_RVCT
        union {
            struct { int i; int b; };
            struct { float f; };
            double d;
        } a = { { 42, 43 } };
        BREAK_HERE;
        // Expand a.
        // CheckType a union {...}.
        // Check a.b 43 int.
        // Check a.d 9.1245819032257467e-313 double.
        // Check a.f 5.88545355e-44 float.
        // Check a.i 42 int.
        // Continue.

        a.i = 1;
        BREAK_HERE;
        // Expand a.
        // CheckType a union {...}.
        // Check a.b 43 int.
        // Check a.d 9.1245819012000775e-313 double.
        // Check a.f 1.40129846e-45 float.
        // Check a.i 1 int.
        // Continue.

        a.i = 2;
        BREAK_HERE;
        // Expand a.
        // CheckType a union {...}.
        // Check a.b 43 int.
        // Check a.d 9.1245819012494841e-313 double.
        // Check a.f 2.80259693e-45 float.
        // Check a.i 2 int.
        // Continue.

        dummyStatement(&a);
        #endif
    }

    void testComplexWatchers()
    {
        struct S { int a; double b; } s[10];
        BREAK_HERE;
        // Expand s and s[0].
        // CheckType s peekandpoke::S [10].
        // Continue.

        // Manual: Watcher Context: "Add New Watcher".
        // Manual: Type    ['s[%d].a' % i for i in range(5)]
        // Manual: Expand it, continue stepping. This should result in a list
        // Manual: of five items containing the .a fields of s[0]..s[4].
        for (int i = 0; i != 10; ++i)
            s[i].a = i;

        dummyStatement(&s);
    }

    void testQImageDisplay()
    {
        #if USE_GUILIB
        QImage im(QSize(200, 200), QImage::Format_RGB32);
        im.fill(QColor(200, 10, 30).rgba());
        QPainter pain;
        pain.begin(&im);
        pain.setPen(QPen(Qt::black, 5.0, Qt::SolidLine, Qt::RoundCap));
        BREAK_HERE;
        // Check im (200x200) QImage.
        // CheckType pain QPainter.
        // Continue.

        pain.drawEllipse(20, 20, 160, 160);
        BREAK_HERE;
        // Continue.

        // Manual: Toggle between "Normal" and "Displayed" in L&W Context Menu,
        // Manual: entry "Display of Type QImage".
        pain.drawArc(70, 115, 60, 30, 200 * 16, 140 * 16);
        BREAK_HERE;
        // Continue.

        pain.setBrush(Qt::black);
        BREAK_HERE;
        // Continue.

        pain.drawEllipse(65, 70, 15, 15);
        BREAK_HERE;
        // Continue.

        // Manual: Toggle between "Normal" and "Displayed" in L&W Context Menu,
        // Manual: entry "Display of Type QImage".
        pain.drawEllipse(120, 70, 15, 15);
        BREAK_HERE;
        // Continue.

        pain.end();
        dummyStatement(&pain);
        #endif
    }

    void testPeekAndPoke3()
    {
        testAnonymousStructs();
        testComplexWatchers();
        testQImageDisplay();
    }

} // namespace peekandpoke


namespace anon {

    #ifndef Q_CC_RVCT
    struct TestAnonymous
    {
        union {
            struct { int i; int b; };
            struct { float f; };
            double d;
        };
    };

    namespace {

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

    }
    #endif

    void testAnonymous()
    {
    #ifndef Q_CC_RVCT
        TestAnonymous a;
        BREAK_HERE;
        // Expand a a.#1 a.#2.
        // CheckType a anon::TestAnonymous.
        // Check a.#1   {...}.
        // CheckType a.#1.b int.
        // CheckType a.#1.i int.
        // CheckType a.#2.f float.
        // CheckType a.d double.
        // Continue.
        a.i = 1;
        a.i = 2;
        a.i = 3;

        Something s;
        BREAK_HERE;
        // Expand s.
        // CheckType s anon::(anonymous namespace)::Something.
        // Check s.a 1 int.
        // Check s.b 1 int.
        // Continue.

        s.foo();
        BREAK_HERE;
        // Expand s.
        // Check s.a 42 int.
        // Check s.b 43 int.
        // Continue.

        std::map<int, Something> m;
        BREAK_HERE;
        // CheckType m std::map<int, anon::{anonymous}::Something>.
        // Continue.

        dummyStatement(&a, &s, &m);
    #endif
    }

} // namespace anon


namespace qbytearray {

    void testQByteArray1()
    {
        QByteArray ba;
        BREAK_HERE;
        // Check ba "" QByteArray.
        // Continue.
        ba += "Hello";
        ba += '"';
        ba += "World";
        ba += char(0);
        ba += 1;
        ba += 2;
        BREAK_HERE;
        // Expand ba.
        // Check ba "Hello"World" QByteArray.
        // Check ba.0 72 'H' char.
        // Check ba.11 0 '\0' char.
        // Check ba.12 1 char.
        // Check ba.13 2 char.
        // Continue.
        dummyStatement(&ba);
    }

    void testQByteArray2()
    {
        QByteArray ba;
        for (int i = 256; --i >= 0; )
            ba.append(char(i));
        QString s(10000, 'x');
        std::string ss(10000, 'c');
        BREAK_HERE;
        // CheckType ba QByteArray.
        // Check s "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx..." QString.
        // Check ss "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc..." std::string.
        // Continue.
        dummyStatement(&ba, &ss, &s);
    }

    void testQByteArray3()
    {
        const char *str1 = "\356";
        const char *str2 = "\xee";
        const char *str3 = "\\ee";
        QByteArray buf1(str1);
        QByteArray buf2(str2);
        QByteArray buf3(str3);
        BREAK_HERE;
        // Check buf1 "î" QByteArray.
        // Check buf2 "î" QByteArray.
        // Check buf3 "\ee" QByteArray.
        // CheckType str1 char *.
        // Continue.
        dummyStatement(&buf1, &buf2, &buf3);
    }

    void testQByteArray4()
    {
        char data[] = { 'H', 'e', 'l', 'l', 'o' };
        QByteArray ba1 = QByteArray::fromRawData(data, 4);
        QByteArray ba2 = QByteArray::fromRawData(data + 1, 4);
        BREAK_HERE;
        // Check ba1 "Hell" QByteArray.
        // Check ba2 "ello" QByteArray.
        // Continue.
        dummyStatement(&ba1, &ba2, &data);
    }

    void testQByteArray()
    {
        testQByteArray1();
        testQByteArray2();
        testQByteArray3();
        testQByteArray4();
    }

} // namespace qbytearray


namespace catchthrow {

    static void throwit1()
    {
        BREAK_HERE;
        // Continue.
        // Set a breakpoint on "throw" in the BreakWindow context menu
        // before stepping further.
        throw 14;
    }

    static void throwit()
    {
        throwit1();
    }

    void testCatchThrow()
    {
        int gotit = 0;
        try {
            throwit();
        } catch (int what) {
            gotit = what;
        }
        dummyStatement(&gotit);
    }

} // namespace catchthrow


namespace undefined {

    void testUndefined()
    {
        int *i = new int;
        delete i;
        BREAK_HERE;
        // Continue.
        // Manual: Uncomment the following line. Step.
        // On Linux, a SIGABRT should be received.
        //delete i;
        dummyStatement(&i);
    }

} // namespace undefined


namespace qdatetime {

    void testQDate()
    {
        QDate date;
        BREAK_HERE;
        // Expand date.
        // CheckType date QDate.
        // Check date.(ISO) "" QString.
        // Check date.(Locale) "" QString.
        // Check date.(SystemLocale) "" QString.
        // Check date.toString "" QString.
        // Continue.

        // Step, check display.
        date = QDate::currentDate();
        date = date.addDays(5);
        date = date.addDays(5);
        dummyStatement(&date);
    }

    void testQTime()
    {
        QTime time;
        BREAK_HERE;
        // Expand time.
        // CheckType time QTime.
        // Check time.(ISO) "" QString.
        // Check time.(Locale) "" QString.
        // Check time.(SystemLocale) "" QString.
        // Check time.toString "" QString.
        // Continue.

        // Step, check display.
        time = QTime::currentTime();
        time = time.addSecs(5);
        time = time.addSecs(5);
        dummyStatement(&time);
    }

    void testQDateTime()
    {
        QDateTime date;
        BREAK_HERE;
        // Expand date.
        // CheckType date QDateTime.
        // Check date.(ISO) "" QString.
        // Check date.(Locale) "" QString.
        // Check date.(SystemLocale) "" QString.
        // Check date.toString "" QString.
        // Check date.toUTC  QDateTime.
        // Continue.

        // Step, check display
        date = QDateTime::currentDateTime();
        date = date.addDays(5);
        date = date.addDays(5);
        dummyStatement(&date);
    }

    void testDateTime()
    {
        testQDate();
        testQDateTime();
        testQTime();
    }

} // namespace qdatetime


namespace qdir {

    void testQDir()
    {
#ifdef Q_OS_WIN
        QDir dir("C:\\Program Files");
        dir.absolutePath(); // Keep in to facilitate stepping
        BREAK_HERE;
        // Check dir "C:/Program Files" QDir.
        // Check dir.absolutePath "C:/Program Files" QString.
        // Check dir.canonicalPath "C:/Program Files" QString.
        // Continue.
#else
        QDir dir("/tmp");
        dir.absolutePath(); // Keep in to facilitate stepping
        BREAK_HERE;
        // Check dir "/tmp" QDir.
        // Check dir.absolutePath "/tmp" QString.
        // Check dir.canonicalPath "/tmp" QString.
        // Continue.
#endif
        dummyStatement(&dir);
    }

} // namespace qdir


namespace qfileinfo {

    void testQFileInfo()
    {
#ifdef Q_OS_WIN
        QFile file("C:\\Program Files\\t");
        file.setObjectName("A QFile instance");
        QFileInfo fi("C:\\Program Files\\tt");
        QString s = fi.absoluteFilePath();
        BREAK_HERE;
        // Check fi "C:/Program Files/tt" QFileInfo.
        // Check file "C:\Program Files\t" QFile.
        // Check s "C:/Program Files/tt" QString.
        // Continue.
        dummyStatement(&file, &s);
#else
        QFile file("/tmp/t");
        file.setObjectName("A QFile instance");
        QFileInfo fi("/tmp/tt");
        QString s = fi.absoluteFilePath();
        BREAK_HERE;
        // Check fi "/tmp/tt" QFileInfo.
        // Check file "/tmp/t" QFile.
        // Check s "/tmp/tt" QString.
        // Continue.
        dummyStatement(&file, &s);
#endif
    }

} // namespace qfileinfo


namespace qhash {

    void testQHash1()
    {
        QHash<QString, QList<int> > hash;
        hash.insert("Hallo", QList<int>());
        hash.insert("Welt", QList<int>() << 1);
        hash.insert("!", QList<int>() << 1 << 2);
        hash.insert("!", QList<int>() << 1 << 2);
        BREAK_HERE;
        // Expand hash hash.0 hash.1 hash.1.value hash.2 hash.2.value.
        // Check hash <3 items> QHash<QString, QList<int>>.
        // Check hash.0   QHashNode<QString, QList<int>>.
        // Check hash.0.key "Hallo" QString.
        // Check hash.0.value <0 items> QList<int>.
        // Check hash.1   QHashNode<QString, QList<int>>.
        // Check hash.1.key "Welt" QString.
        // Check hash.1.value <1 items> QList<int>.
        // Check hash.1.value.0 1 int.
        // Check hash.2   QHashNode<QString, QList<int>>.
        // Check hash.2.key "!" QString.
        // Check hash.2.value <2 items> QList<int>.
        // Check hash.2.value.0 1 int.
        // Check hash.2.value.1 2 int.
        // Continue.
        dummyStatement(&hash);
    }

    void testQHash2()
    {
        QHash<int, float> hash;
        hash[11] = 11.0;
        hash[22] = 22.0;
        BREAK_HERE;
        // Expand hash.
        // Check hash <2 items> QHash<int, float>.
        // Check hash.22 22 float.
        // Check hash.11 11 float.
        // Continue.
        dummyStatement(&hash);
    }

    void testQHash3()
    {
        QHash<QString, int> hash;
        hash["22.0"] = 22.0;
        hash["123.0"] = 22.0;
        hash["111111ss111128.0"] = 28.0;
        hash["11124.0"] = 22.0;
        hash["1111125.0"] = 22.0;
        hash["11111126.0"] = 22.0;
        hash["111111127.0"] = 27.0;
        hash["111111111128.0"] = 28.0;
        hash["111111111111111111129.0"] = 29.0;
        BREAK_HERE;
        // Expand hash hash.0 hash.8.
        // Check hash <9 items> QHash<QString, int>.
        // Check hash.0   QHashNode<QString, int>.
        // Check hash.0.key "123.0" QString.
        // Check hash.0.value 22 int.
        // Check hash.8   QHashNode<QString, int>.
        // Check hash.8.key "11124.0" QString.
        // Check hash.8.value 22 int.
        // Continue.
        dummyStatement(&hash);
    }

    void testQHash4()
    {
        QHash<QByteArray, float> hash;
        hash["22.0"] = 22.0;
        hash["123.0"] = 22.0;
        hash["111111ss111128.0"] = 28.0;
        hash["11124.0"] = 22.0;
        hash["1111125.0"] = 22.0;
        hash["11111126.0"] = 22.0;
        hash["111111127.0"] = 27.0;
        hash["111111111128.0"] = 28.0;
        hash["111111111111111111129.0"] = 29.0;
        BREAK_HERE;
        // Expand hash hash.0 hash.8/
        // Check hash <9 items> QHash<QByteArray, float>.
        // Check hash.0   QHashNode<QByteArray, float>.
        // Check hash.0.key "123.0" QByteArray.
        // Check hash.0.value 22 float.
        // Check hash.8   QHashNode<QByteArray, float>.
        // Check hash.8.key "11124.0" QByteArray.
        // Check hash.8.value 22 float.
        // Continue.
        dummyStatement(&hash);
    }

    void testQHash5()
    {
        QHash<int, QString> hash;
        hash[22] = "22.0";
        BREAK_HERE;
        // Expand hash hash.0.
        // Check hash <1 items> QHash<int, QString>.
        // Check hash.0   QHashNode<int, QString>.
        // Check hash.0.key 22 int.
        // Check hash.0.value "22.0" QString.
        // Continue.
        dummyStatement(&hash);
    }

    void testQHash6()
    {
        QHash<QString, Foo> hash;
        hash["22.0"] = Foo(22);
        hash["33.0"] = Foo(33);
        BREAK_HERE;
        // Expand hash hash.0 hash.0.value hash.1.
        // Check hash <2 items> QHash<QString, Foo>.
        // Check hash.0   QHashNode<QString, Foo>.
        // Check hash.0.key "22.0" QString.
        // CheckType hash.0.value Foo.
        // Check hash.0.value.a 22 int.
        // Check hash.1   QHashNode<QString, Foo>.
        // Check hash.1.key "33.0" QString.
        // CheckType hash.1.value Foo.
        // Continue.
        dummyStatement(&hash);
    }

    void testQHash7()
    {
        QObject ob;
        QHash<QString, QPointer<QObject> > hash;
        hash.insert("Hallo", QPointer<QObject>(&ob));
        hash.insert("Welt", QPointer<QObject>(&ob));
        hash.insert(".", QPointer<QObject>(&ob));
        BREAK_HERE;
        // Expand hash hash.0 hash.0.value hash.2.
        // Check hash <3 items> QHash<QString, QPointer<QObject>>.
        // Check hash.0   QHashNode<QString, QPointer<QObject>>.
        // Check hash.0.key "Hallo" QString.
        // CheckType hash.0.value QPointer<QObject>.
        // CheckType hash.0.value.o QObject.
        // Check hash.2   QHashNode<QString, QPointer<QObject>>.
        // Check hash.2.key "." QString.
        // CheckType hash.2.value QPointer<QObject>.
        // Continue.
        dummyStatement(&hash, &ob);
    }

    void testQHashIntFloatIterator()
    {
        typedef QHash<int, float> Hash;
        Hash hash;
        hash[11] = 11.0;
        hash[22] = 22.0;
        hash[33] = 33.0;
        hash[44] = 44.0;
        hash[55] = 55.0;
        hash[66] = 66.0;

        Hash::iterator it1 = hash.begin();
        Hash::iterator it2 = it1; ++it2;
        Hash::iterator it3 = it2; ++it3;
        Hash::iterator it4 = it3; ++it4;
        Hash::iterator it5 = it4; ++it5;
        Hash::iterator it6 = it5; ++it6;

        BREAK_HERE;
        // Expand hash.
        // Check hash <6 items> qhash::Hash.
        // Check hash.11 11 float.
        // Check it1.key 55 int.
        // Check it1.value 55 float.
        // Check it6.key 33 int.
        // Check it6.value 33 float.
        // Continue.
        dummyStatement(&hash, &it1, &it2, &it3, &it4, &it5, &it6);
    }

    void testQHash()
    {
        testQHash1();
        testQHash2();
        testQHash3();
        testQHash4();
        testQHash5();
        testQHash6();
        testQHash7();
        testQHashIntFloatIterator();
    }

} // namespace qhash


namespace qhostaddress {

    void testQHostAddress()
    {
        QHostAddress ha1(129u * 256u * 256u * 256u + 130u);
        QHostAddress ha2("127.0.0.1");
        BREAK_HERE;
        // Check ha1 129.0.0.130 QHostAddress.
        // Check ha2 "127.0.0.1" QHostAddress.
        // Continue.
        dummyStatement(&ha1, &ha2);
    }

} // namespace qhostaddress


namespace painting {

    void testQImage()
    {
        #if USE_GUILIB
        // only works with Python dumper
        QImage im(QSize(200, 200), QImage::Format_RGB32);
        im.fill(QColor(200, 100, 130).rgba());
        QPainter pain;
        pain.begin(&im);
        BREAK_HERE;
        // Check im (200x200) QImage.
        // CheckType pain QPainter.
        // Continue.
        // Step.
        pain.drawLine(2, 2, 130, 130);
        pain.drawLine(4, 2, 130, 140);
        pain.drawRect(30, 30, 80, 80);
        pain.end();
        dummyStatement(&pain, &im);
        #endif
    }

    void testQPixmap()
    {
        #if USE_GUILIB
        QImage im(QSize(200, 200), QImage::Format_RGB32);
        im.fill(QColor(200, 100, 130).rgba());
        QPainter pain;
        pain.begin(&im);
        pain.drawLine(2, 2, 130, 130);
        pain.end();
        QPixmap pm = QPixmap::fromImage(im);
        BREAK_HERE;
        // Check im (200x200) QImage.
        // CheckType pain QPainter.
        // Check pm (200x200) QPixmap.
        // Continue.
        dummyStatement(&im, &pm);
        #endif
    }

    void testPainting()
    {
        testQImage();
        testQPixmap();
    }

} // namespace painting


namespace qlinkedlist {

    void testQLinkedListInt()
    {
        QLinkedList<int> list;
        list.append(101);
        list.append(102);
        BREAK_HERE;
        // Expand list.
        // Check list <2 items> QLinkedList<int>.
        // Check list.0 101 int.
        // Check list.1 102 int.
        // Continue.
        dummyStatement(&list);
    }

    void testQLinkedListUInt()
    {
        QLinkedList<uint> list;
        list.append(103);
        list.append(104);
        BREAK_HERE;
        // Expand list.
        // Check list <2 items> QLinkedList<unsigned int>.
        // Check list.0 103 unsigned int.
        // Check list.1 104 unsigned int.
        // Continue.
        dummyStatement(&list);
    }

    void testQLinkedListFooStar()
    {
        QLinkedList<Foo *> list;
        list.append(new Foo(1));
        list.append(0);
        list.append(new Foo(3));
        BREAK_HERE;
        // Expand list list.0 list.2.
        // Check list <3 items> QLinkedList<Foo*>.
        // CheckType list.0 Foo.
        // Check list.0.a 1 int.
        // Check list.1 0x0 Foo *.
        // CheckType list.2 Foo.
        // Check list.2.a 3 int.
        // Continue.
        dummyStatement(&list);
    }

    void testQLinkedListULongLong()
    {
        QLinkedList<qulonglong> list;
        list.append(42);
        list.append(43);
        BREAK_HERE;
        // Expand list.
        // Check list <2 items> QLinkedList<unsigned long long>.
        // Check list.0 42 unsigned long long.
        // Check list.1 43 unsigned long long.
        // Continue.
        dummyStatement(&list);
    }

    void testQLinkedListFoo()
    {
        QLinkedList<Foo> list;
        list.append(Foo(1));
        list.append(Foo(2));
        BREAK_HERE;
        // Expand list list.0 list.1.
        // Check list <2 items> QLinkedList<Foo>.
        // CheckType list.0 Foo.
        // Check list.0.a 1 int.
        // CheckType list.1 Foo.
        // Check list.1.a 2 int.
        // Continue.
        dummyStatement(&list);
    }

    void testQLinkedListStdString()
    {
        QLinkedList<std::string> list;
        list.push_back("aa");
        list.push_back("bb");
        BREAK_HERE;
        // Expand list.
        // Check list <2 items> QLinkedList<std::string>.
        // Check list.0 "aa" std::string.
        // Check list.1 "bb" std::string.
        // Continue.
        dummyStatement(&list);
    }

    void testQLinkedList()
    {
        testQLinkedListInt();
        testQLinkedListUInt();
        testQLinkedListFooStar();
        testQLinkedListULongLong();
        testQLinkedListFoo();
        testQLinkedListStdString();
    }

} // namespace qlinkedlist


namespace qlist {

    void testQListInt()
    {
        QList<int> big;
        for (int i = 0; i < 10000; ++i)
            big.push_back(i);
        BREAK_HERE;
        // Expand big.
        // Check big <10000 items> QList<int>.
        // Check big.0 0 int.
        // Check big.1999 1999 int.
        // Continue.
        dummyStatement(&big);
    }

    void testQListIntTakeFirst()
    {
        QList<int> l;
        l.append(0);
        l.append(1);
        l.append(2);
        l.takeFirst();
        BREAK_HERE;
        // Expand l.
        // Check l <2 items> QList<int>.
        // Check l.0 1 int.
        // Continue.
        dummyStatement(&l);
    }

    void testQListStringTakeFirst()
    {
        QList<QString> l;
        l.append("0");
        l.append("1");
        l.append("2");
        l.takeFirst();
        BREAK_HERE;
        // Expand l.
        // Check l <2 items> QList<QString>.
        // Check l.0 "1" QString.
        // Continue.
        dummyStatement(&l);
    }

    void testQStringListTakeFirst()
    {
        QStringList l;
        l.append("0");
        l.append("1");
        l.append("2");
        l.takeFirst();
        BREAK_HERE;
        // Expand l.
        // Check l <2 items> QStringList.
        // Check l.0 "1" QString.
        // Continue.
        dummyStatement(&l);
    }

    void testQListIntStar()
    {
        QList<int *> l;
        BREAK_HERE;
        // Check l <0 items> QList<int*>.
        // Continue.
        l.append(new int(1));
        l.append(new int(2));
        l.append(new int(3));
        BREAK_HERE;
        // Expand l.
        // Check l <3 items> QList<int*>.
        // CheckType l.0 int.
        // CheckType l.2 int.
        // Continue.
        dummyStatement(&l);
    }

    void testQListUInt()
    {
        QList<uint> l;
        BREAK_HERE;
        // Check l <0 items> QList<unsigned int>.
        // Continue.
        l.append(101);
        l.append(102);
        l.append(102);
        BREAK_HERE;
        // Expand l.
        // Check l <3 items> QList<unsigned int>.
        // Check l.0 101 unsigned int.
        // Check l.2 102 unsigned int.
        // Continue.
        dummyStatement(&l);
    }

    void testQListUShort()
    {
        QList<ushort> l;
        BREAK_HERE;
        // Check l <0 items> QList<unsigned short>.
        // Continue.
        l.append(101);
        l.append(102);
        l.append(102);
        BREAK_HERE;
        // Expand l.
        // Check l <3 items> QList<unsigned short>.
        // Check l.0 101 unsigned short.
        // Check l.2 102 unsigned short.
        // Continue.
        dummyStatement(&l);
    }

    void testQListQChar()
    {
        QList<QChar> l;
        BREAK_HERE;
        // Check l <0 items> QList<QChar>.
        // Continue.
        l.append(QChar('a'));
        l.append(QChar('b'));
        l.append(QChar('c'));
        BREAK_HERE;
        // Expand l.
        // Check l <3 items> QList<QChar>.
        // Check l.0 'a' (97) QChar.
        // Check l.2 'c' (99) QChar.
        // Continue.
        dummyStatement(&l);
    }

    void testQListQULongLong()
    {
        QList<qulonglong> l;
        BREAK_HERE;
        // Check l <0 items> QList<unsigned long long>.
        // Continue.
        l.append(101);
        l.append(102);
        l.append(102);
        BREAK_HERE;
        // Expand l.
        // Check l <3 items> QList<unsigned long long>.
        // CheckType l.0 unsigned long long.
        // CheckType l.2 unsigned long long.
        // Continue.
        dummyStatement(&l);
    }

    void testQListStdString()
    {
        QList<std::string> l;
        BREAK_HERE;
        // Check l <0 items> QList<std::string>.
        // Continue.
        l.push_back("aa");
        l.push_back("bb");
        l.push_back("cc");
        l.push_back("dd");
        BREAK_HERE;
        // Expand l.
        // Check l <4 items> QList<std::string>.
        // CheckType l.0 std::string.
        // CheckType l.3 std::string.
        // Continue.
        dummyStatement(&l);
    }

    void testQListFoo()
    {
        QList<Foo> l;
        BREAK_HERE;
        // Check l <0 items> QList<Foo>.
        // Continue.
        for (int i = 0; i < 100; ++i)
            l.push_back(i + 15);
        BREAK_HERE;
        // Check l <100 items> QList<Foo>.
        // Expand l.
        // CheckType l.0 Foo.
        // CheckType l.99 Foo.
        // Continue.
        l.push_back(1000);
        l.push_back(1001);
        l.push_back(1002);
        BREAK_HERE;
        // Check l <103 items> QList<Foo>.
        // Continue.
        dummyStatement(&l);
    }

    void testQListReverse()
    {
        QList<int> l = QList<int>() << 1 << 2 << 3;
        typedef std::reverse_iterator<QList<int>::iterator> Reverse;
        Reverse rit(l.end());
        Reverse rend(l.begin());
        QList<int> r;
        while (rit != rend)
            r.append(*rit++);
        BREAK_HERE;
        // Expand l r.
        // Check l <3 items> QList<int>.
        // Check l.0 1 int.
        // Check l.1 2 int.
        // Check l.2 3 int.
        // Check r <3 items> QList<int>.
        // Check r.0 3 int.
        // Check r.1 2 int.
        // Check r.2 1 int.
        // CheckType rend qlist::Reverse.
        // CheckType rit qlist::Reverse.
        // Continue.
        dummyStatement();
    }

    void testQList()
    {
        testQListInt();
        testQListIntStar();
        testQListUInt();
        testQListUShort();
        testQListQChar();
        testQListQULongLong();
        testQListStdString();
        testQListFoo();
        testQListReverse();
        testQListIntTakeFirst();
        testQListStringTakeFirst();
        testQStringListTakeFirst();
    }

} // namespace qlist


namespace qlocale {

    void testQLocale()
    {
        QLocale loc = QLocale::system();
        //QString s = loc.name();
        //QVariant v = loc;
        QLocale::MeasurementSystem m = loc.measurementSystem();
        BREAK_HERE;
        // CheckType loc QLocale.
        // CheckType m QLocale::MeasurementSystem.
        // Continue.
        dummyStatement(&loc, &m);
    }

} // namespace qlocale


namespace qmap {

    void testQMapUIntStringList()
    {
        QMap<uint, QStringList> map;
        map[11] = QStringList() << "11";
        map[22] = QStringList() << "22";
        BREAK_HERE;
        // Expand map map.0 map.0.value map.1 map.1.value.
        // Check map <2 items> QMap<unsigned int, QStringList>.
        // Check map.0   QMapNode<unsigned int, QStringList>.
        // Check map.0.key 11 unsigned int.
        // Check map.0.value <1 items> QStringList.
        // Check map.0.value.0 "11" QString.
        // Check map.1   QMapNode<unsigned int, QStringList>.
        // Check map.1.key 22 unsigned int.
        // Check map.1.value <1 items> QStringList.
        // Check map.1.value.0 "22" QString.
        // Continue.
        dummyStatement(&map);
    }

    void testQMapUIntStringListTypedef()
    {
        // only works with Python dumper
        typedef QMap<uint, QStringList> T;
        T map;
        map[11] = QStringList() << "11";
        map[22] = QStringList() << "22";
        BREAK_HERE;
        // Check map <2 items> qmap::T.
        // Continue.
        dummyStatement(&map);
    }

    void testQMapUIntFloat()
    {
        QMap<uint, float> map;
        map[11] = 11.0;
        map[22] = 22.0;
        BREAK_HERE;
        // Expand map.
        // Check map <2 items> QMap<unsigned int, float>.
        // Check map.11 11 float.
        // Check map.22 22 float.
        // Continue.
        dummyStatement(&map);
    }

    void testQMapStringFloat()
    {
        QMap<QString, float> map;
        map["22.0"] = 22.0;
        BREAK_HERE;
        // Expand map map.0.
        // Check map <1 items> QMap<QString, float>.
        // Check map.0   QMapNode<QString, float>.
        // Check map.0.key "22.0" QString.
        // Check map.0.value 22 float.
        // Continue.
        dummyStatement(&map);
    }

    void testQMapIntString()
    {
        QMap<int, QString> map;
        map[22] = "22.0";
        BREAK_HERE;
        // Expand map map.0.
        // Check map <1 items> QMap<int, QString>.
        // Check map.0   QMapNode<int, QString>.
        // Check map.0.key 22 int.
        // Check map.0.value "22.0" QString.
        // Continue.
        dummyStatement(&map);
    }

    void testQMapStringFoo()
    {
        QMap<QString, Foo> map;
        map["22.0"] = Foo(22);
        map["33.0"] = Foo(33);
        BREAK_HERE;
        // Expand map map.0 map.0.key map.0.value map.1 map.1.value.
        // Check map <2 items> QMap<QString, Foo>.
        // Check map.0   QMapNode<QString, Foo>.
        // Check map.0.key "22.0" QString.
        // CheckType map.0.value Foo.
        // Check map.0.value.a 22 int.
        // Check map.1   QMapNode<QString, Foo>.
        // Check map.1.key "33.0" QString.
        // CheckType map.1.value Foo.
        // Check map.1.value.a 33 int.
        // Continue.
        dummyStatement(&map);
    }

    void testQMapStringPointer()
    {
        // only works with Python dumper
        QObject ob;
        QMap<QString, QPointer<QObject> > map;
        map.insert("Hallo", QPointer<QObject>(&ob));
        map.insert("Welt", QPointer<QObject>(&ob));
        map.insert(".", QPointer<QObject>(&ob));
        BREAK_HERE;
        // Expand map map.0 map.0.key map.0.value map.1 map.2.
        // Check map <3 items> QMap<QString, QPointer<QObject>>.
        // Check map.0   QMapNode<QString, QPointer<QObject>>.
        // Check map.0.key "." QString.
        // CheckType map.0.value QPointer<QObject>.
        // CheckType map.0.value.o QObject.
        // Check map.1   QMapNode<QString, QPointer<QObject>>.
        // Check map.1.key "Hallo" QString.
        // Check map.2   QMapNode<QString, QPointer<QObject>>.
        // Check map.2.key "Welt" QString.
        // Continue.
        dummyStatement(&map);
    }

    void testQMapStringList()
    {
        // only works with Python dumper
        QList<nsA::nsB::SomeType *> x;
        x.append(new nsA::nsB::SomeType(1));
        x.append(new nsA::nsB::SomeType(2));
        x.append(new nsA::nsB::SomeType(3));
        QMap<QString, QList<nsA::nsB::SomeType *> > map;
        map["foo"] = x;
        map["bar"] = x;
        map["1"] = x;
        map["2"] = x;
        BREAK_HERE;
        // Expand map map.0 map.0.key map.0.value map.1 map.1.value.1 map.1.value.2 map.3 map.3.value map.3.value.2.
        // Check map <4 items> QMap<QString, QList<nsA::nsB::SomeType*>>.
        // Check map.0   QMapNode<QString, QList<nsA::nsB::SomeType*>>.
        // Check map.0.key "1" QString.
        // Check map.0.value <3 items> QList<nsA::nsB::SomeType*>.
        // CheckType map.0.value.0 nsA::nsB::SomeType.
        // Check map.0.value.0.a 1 int.
        // CheckType map.0.value.1 nsA::nsB::SomeType.
        // Check map.0.value.1.a 2 int.
        // CheckType map.0.value.2 nsA::nsB::SomeType.
        // Check map.0.value.2.a 3 int.
        // Check map.3   QMapNode<QString, QList<nsA::nsB::SomeType*>>.
        // Check map.3.key "foo" QString.
        // Check map.3.value <3 items> QList<nsA::nsB::SomeType*>.
        // CheckType map.3.value.2 nsA::nsB::SomeType.
        // Check map.3.value.2.a 3 int.
        // Check x <3 items> QList<nsA::nsB::SomeType*>.
        // Continue.
        dummyStatement(&map);
    }

    void testQMultiMapUintFloat()
    {
        QMultiMap<uint, float> map;
        map.insert(11, 11.0);
        map.insert(22, 22.0);
        map.insert(22, 33.0);
        map.insert(22, 34.0);
        map.insert(22, 35.0);
        map.insert(22, 36.0);
        BREAK_HERE;
        // Expand map.
        // Check map <6 items> QMultiMap<unsigned int, float>.
        // Check map.0 11 float.
        // Check map.5 22 float.
        // Continue.
        dummyStatement(&map);
    }

    void testQMultiMapStringFloat()
    {
        QMultiMap<QString, float> map;
        map.insert("22.0", 22.0);
        BREAK_HERE;
        // Expand map map.0.
        // Check map <1 items> QMultiMap<QString, float>.
        // Check map.0   QMapNode<QString, float>.
        // Check map.0.key "22.0" QString.
        // Check map.0.value 22 float.
        // Continue.
        dummyStatement(&map);
    }

    void testQMultiMapIntString()
    {
        QMultiMap<int, QString> map;
        map.insert(22, "22.0");
        BREAK_HERE;
        // Expand map map.0.
        // Check map <1 items> QMultiMap<int, QString>.
        // Check map.0   QMapNode<int, QString>.
        // Check map.0.key 22 int.
        // Check map.0.value "22.0" QString.
        // Continue.
        dummyStatement(&map);
    }

    void testQMultiMapStringFoo()
    {
        QMultiMap<QString, Foo> map;
        map.insert("22.0", Foo(22));
        map.insert("33.0", Foo(33));
        map.insert("22.0", Foo(22));
        BREAK_HERE;
        // Expand map map.0 map.0.value.
        // Check map <3 items> QMultiMap<QString, Foo>.
        // Check map.0   QMapNode<QString, Foo>.
        // Check map.0.key "22.0" QString.
        // CheckType map.0.value Foo.
        // Check map.0.value.a 22 int.
        // Check map.2   QMapNode<QString, Foo>.
        // Continue.
        dummyStatement(&map);
    }

    void testQMultiMapStringPointer()
    {
        QObject ob;
        QMultiMap<QString, QPointer<QObject> > map;
        map.insert("Hallo", QPointer<QObject>(&ob));
        map.insert("Welt", QPointer<QObject>(&ob));
        map.insert(".", QPointer<QObject>(&ob));
        map.insert(".", QPointer<QObject>(&ob));
        BREAK_HERE;
        // Expand map map.0 map.1 map.2 map.3.
        // Check map <4 items> QMultiMap<QString, QPointer<QObject>>.
        // Check map.0   QMapNode<QString, QPointer<QObject>>.
        // Check map.0.key "." QString.
        // CheckType map.0.value QPointer<QObject>.
        // Check map.1   QMapNode<QString, QPointer<QObject>>.
        // Check map.1.key "." QString.
        // Check map.2   QMapNode<QString, QPointer<QObject>>.
        // Check map.2.key "Hallo" QString.
        // Check map.3   QMapNode<QString, QPointer<QObject>>.
        // Check map.3.key "Welt" QString.
        // Continue.
        dummyStatement(&map);
    }

    void testQMap()
    {
        testQMapUIntStringList();
        testQMapUIntStringListTypedef();
        testQMapUIntFloat();
        testQMapStringFloat();
        testQMapIntString();
        testQMapStringFoo();
        testQMapStringPointer();
        testQMapStringList();
        testQMultiMapUintFloat();
        testQMultiMapStringFloat();
        testQMultiMapIntString();
        testQMultiMapStringFoo();
        testQMapUIntStringList();
        testQMultiMapStringFoo();
        testQMultiMapStringPointer();
    }

} // namespace qmap


namespace qobject {

    void testQObject1()
    {
        // This checks whether signal-slot connections are displayed.
        QObject parent;
        parent.setObjectName("A Parent");
        QObject child(&parent);
        child.setObjectName("A Child");
        QObject::connect(&child, SIGNAL(destroyed()), &parent, SLOT(deleteLater()));
        QObject::disconnect(&child, SIGNAL(destroyed()), &parent, SLOT(deleteLater()));
        child.setObjectName("A renamed Child");
        BREAK_HERE;
        // Check child "A renamed Child" QObject.
        // Check parent "A Parent" QObject.
        // Continue.
        dummyStatement(&parent, &child);
    }

    namespace Names {
        namespace Bar {

        struct Ui { Ui() { w = 0; } QWidget *w; };

        class TestObject : public QObject
        {
            Q_OBJECT
        public:
            TestObject(QObject *parent = 0)
                : QObject(parent)
            {
                m_ui = new Ui;
                #if USE_GUILIB
                m_ui->w = new QWidget;
                #else
                m_ui->w = 0;
                #endif
            }

            Q_PROPERTY(QString myProp1 READ myProp1 WRITE setMyProp1)
            QString myProp1() const { return m_myProp1; }
            Q_SLOT void setMyProp1(const QString&mt) { m_myProp1 = mt; }

            Q_PROPERTY(QString myProp2 READ myProp2 WRITE setMyProp2)
            QString myProp2() const { return m_myProp2; }
            Q_SLOT void setMyProp2(const QString&mt) { m_myProp2 = mt; }

        public:
            Ui *m_ui;
            QString m_myProp1;
            QString m_myProp2;
        };

        } // namespace Bar
    } // namespace Names

    void testQObject2()
    {
        //QString longString = QString(10000, QLatin1Char('A'));
    #if 1
        Names::Bar::TestObject test;
        test.setMyProp1("HELLO");
        test.setMyProp2("WORLD");
        QString s = test.myProp1();
        s += test.myProp2();
        BREAK_HERE;
        // Check s "HELLOWORLD" QString.
        // Check test  qobject::Names::Bar::TestObject.
        // Continue.
        dummyStatement(&s);
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
        #if USE_GUILIB
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
    #endif

    #if 0
        QList<QObject *> obs;
        obs.append(&ob);
        obs.append(&ob1);
        obs.append(0);
        obs.append(&app);
        ob1.setObjectName("A Subobject");
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

    void testSignalSlot()
    {
        Sender sender;
        Receiver receiver;
        QObject::connect(&sender, SIGNAL(aSignal()), &receiver, SLOT(aSlot()));
        // Break here.
        // Single step through signal emission.
        sender.doEmit();
        dummyStatement(&sender, &receiver);
    }

    #if USE_PRIVATE

    class DerivedObjectPrivate : public QObjectPrivate
    {
    public:
        DerivedObjectPrivate() {
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
        DerivedObject() : QObject(*new DerivedObjectPrivate, 0) {}

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

    void testQObjectData()
    {
        // This checks whether QObject-derived classes with Q_PROPERTYs
        // are displayed properly.
    #if USE_PRIVATE
        DerivedObject ob;
        BREAK_HERE;
        // Expand ob ob.properties.
        // Check ob.properties.x 43 QVariant (int).
        // Continue.

        // expand ob and ob.properties
        // step, and check whether x gets updated.
        ob.setX(23);
        ob.setX(25);
        ob.setX(26);
        BREAK_HERE;
        // Expand ob ob.properties.
        // Check ob.properties.x 26 QVariant (int).
        // Continue.
    #endif
    }

    void testQObject()
    {
        testQObjectData();
        testQObject1();
        testQObject2();
        testSignalSlot();
    }

} // namespace qobject



namespace qregexp {

    void testQRegExp()
    {
        // Works with Python dumpers only.
        QRegExp re(QString("a(.*)b(.*)c"));
        BREAK_HERE;
        // Check re "a(.*)b(.*)c" QRegExp.
        // Continue.
        QString str1 = "a1121b344c";
        QString str2 = "Xa1121b344c";
        BREAK_HERE;
        // Check str1 "a1121b344c" QString.
        // Check str2 "Xa1121b344c" QString.
        // Continue.
        int pos2 = re.indexIn(str2);
        int pos1 = re.indexIn(str1);
        BREAK_HERE;
        // Check pos1 0 int.
        // Check pos2 1 int.
        // Continue.
        dummyStatement(&pos1, &pos2);
    }

} // namespace qregexp

namespace qrect {

    #if USE_GUILIB
    void testQPoint()
    {
        QPoint s;
        BREAK_HERE;
        // Check s (0, 0) QPoint.
        // Continue.
        // Step over, check display looks sane.
        s = QPoint(100, 200);
        BREAK_HERE;
        // Check s (100, 200) QPoint.
        // Continue.
        dummyStatement(&s);
    }

    void testQPointF()
    {
        QPointF s;
        BREAK_HERE;
        // Check s (0, 0) QPointF.
        // Continue.
        // Step over, check display looks sane.
        s = QPointF(100, 200);
        BREAK_HERE;
        // Check s (100, 200) QPointF.
        // Continue.
        dummyStatement(&s);
    }

    void testQRect()
    {
        QRect rect;
        BREAK_HERE;
        // Check rect 0x0+0+0 QRect.
        // Continue.
        // Step over, check display looks sane.
        rect = QRect(100, 100, 200, 200);
        BREAK_HERE;
        // Check rect 200x200+100+100 QRect.
        // Continue.
        dummyStatement(&rect);
    }

    void testQRectF()
    {
        QRectF rect;
        BREAK_HERE;
        // Check rect 0x0+0+0 QRectF.
        // Continue.
        // Step over, check display looks sane.
        rect = QRectF(100, 100, 200, 200);
        BREAK_HERE;
        // Check rect 200x200+100+100 QRectF.
        // Continue.
        dummyStatement(&rect);
    }

    void testQSize()
    {
        QSize s;
        BREAK_HERE;
        // Check s (-1, -1) QSize.
        // Continue.
        s = QSize(100, 200);
        BREAK_HERE;
        // Check s (100, 200) QSize.
        // Continue.
        dummyStatement(&s);
    }

    void testQSizeF()
    {
        QSizeF s;
        BREAK_HERE;
        // Check s (-1, -1) QSizeF.
        // Continue.
        s = QSizeF(100, 200);
        BREAK_HERE;
        // Check s (100, 200) QSizeF.
        // Continue.
        dummyStatement(&s);
    }
    #endif

    void testGeometry()
    {
        #if USE_GUILIB
        testQPoint();
        testQPointF();
        testQRect();
        testQRectF();
        testQSize();
        testQSizeF();
        #endif
    }

} // namespace qrect


namespace qregion {

    void testQRegion()
    {
        #if USE_GUILIB
        // Works with Python dumpers only.
        QRegion region;
        BREAK_HERE;
        // Check region <empty> QRegion.
        // Continue.
        // Step over until end, check display looks sane.
        region += QRect(100, 100, 200, 200);
        BREAK_HERE;
        // Expand region.
        // Check region <1 items> QRegion.
        // CheckType region.extents QRect.
        // Check region.innerArea 40000 int.
        // CheckType region.innerRect QRect.
        // Check region.numRects 1 int.
        // Check region.rects <0 items> QVector<QRect>.
        // Continue.
        region += QRect(300, 300, 400, 500);
        BREAK_HERE;
        // Expand region.
        // Check region <2 items> QRegion.
        // CheckType region.extents QRect.
        // Check region.innerArea 200000 int.
        // CheckType region.innerRect QRect.
        // Check region.numRects 2 int.
        // Check region.rects <2 items> QVector<QRect>.
        // Continue.
        region += QRect(500, 500, 600, 600);
        BREAK_HERE;
        // Expand region.
        // Check region <4 items> QRegion.
        // CheckType region.extents QRect.
        // Check region.innerArea 360000 int.
        // CheckType region.innerRect QRect.
        // Check region.numRects 4 int.
        // Check region.rects <8 items> QVector<QRect>.
        // Continue.
        region += QRect(500, 500, 600, 600);
        BREAK_HERE;
        // Check region <4 items> QRegion.
        // Continue.
        region += QRect(500, 500, 600, 600);
        BREAK_HERE;
        // Check region <4 items> QRegion.
        // Continue.
        region += QRect(500, 500, 600, 600);
        BREAK_HERE;
        // Check region <4 items> QRegion.
        // Continue.
        dummyStatement(&region);
        #endif
    }

} // namespace qregion


namespace plugin {

    void testPlugin()
    {
        QString dir = QDir::currentPath();
    #ifdef Q_OS_LINUX
        QLibrary lib(dir + "/libsimple_test_plugin.so");
    #endif
    #ifdef Q_OS_MAC
        dir = QFileInfo(dir + "/../..").canonicalPath();
        QLibrary lib(dir + "/libsimple_test_plugin.dylib");
    #endif
    #ifdef Q_OS_WIN
        QLibrary lib(dir + "/debug/simple_test_plugin.dll");
    #endif
        BREAK_HERE;
        // CheckType dir QString.
        // CheckType lib QLibrary.
        // CheckType name QString.
        // CheckType res int.
        // Continue.
        // Step
        int (*foo)() = (int(*)()) lib.resolve("pluginTest");
        QString name = lib.fileName();
        int res = 4;
        if (foo) {
            BREAK_HERE;
            // Check res 4 int.
            // Continue.
            // Step
            res = foo();
        } else {
            BREAK_HERE;
            // Step
            name = lib.errorString();
        }
        dummyStatement(&name, &res);
    }

} // namespace plugin


namespace final {

    void testQSettings()
    {
        // Note: Construct a QCoreApplication first.
        QSettings settings("/tmp/test.ini", QSettings::IniFormat);
        QVariant value = settings.value("item1","").toString();
        BREAK_HERE;
        // Expand settings.
        // Check settings  QSettings.
        // Check settings.@1 "" QObject.
        // Check value "" QVariant (QString).
        // Continue.
        dummyStatement(&settings, &value);
    }

    void testNullPointerDeref()
    {
        int a = 'a';
        int b = 'b';
        BREAK_HERE;
        // Continue.

        return; // Uncomment.
        *(int *)0 = a + b;
    }

    void testEndlessRecursion(int i = 0)
    {
        BREAK_HERE;
        // Continue.

        return; // Uncomment.
        testEndlessRecursion(i + 1);
    }

    void testEndlessLoop()
    {
        qlonglong a = 1;
        // gdb:
        // Breakpoint at "while" will stop only once
        // Hitting "Pause" button might show backtrace of different thread
        BREAK_HERE;
        // Continue.

        // Jump over next line.
        return;
        while (a > 0)
            ++a;
        dummyStatement(&a);
    }

    void testUncaughtException()
    {
        BREAK_HERE;
        // Continue.

        // Jump over next line.
        return;
        throw 42;
    }

    void testApplicationStart(QCoreApplication *app)
    {
        #if USE_GUILIB
        QString str = QString::fromUtf8("XXXXXXXXXXXXXXyyXXX ö");
        QLabel l(str);
        l.setObjectName("Some Label");
        l.show();
        #endif
        // Jump over next line.
        return;
        app->exec();
        dummyStatement(&app);
    }

    void testNullReferenceHelper(int &i, int &j)
    {
        i += 1;
        j += 1;
    }

    void testNullReference()
    {
        int i = 21;
        int *p = &i;
        int *q = 0;
        int &pp = *p;
        int &qq = *q;
        BREAK_HERE;
        // Check i 21 int.
        // CheckType p int.
        // Check p 21 int.
        // Check q 0x0 int *.
        // Check pp 21 int &.
        // Check qq <null reference> int &.
        // Continue.
        return; // Uncomment.
        testNullReferenceHelper(pp, qq);
        dummyStatement(p, q, &i);
    }

    void testFinal(QCoreApplication *app)
    {
        // This contains all "final" tests that do not allow proceeding
        // with further tests.
        BREAK_HERE;
        // Continue.
        testQSettings();
        testNullPointerDeref();
        testNullReference();
        testEndlessLoop();
        testEndlessRecursion();
        testUncaughtException();
        testApplicationStart(app);
    }

} // namespace final


namespace qset {

    void testQSet1()
    {
        QSet<int> s;
        s.insert(11);
        s.insert(22);
        BREAK_HERE;
        // Expand s.
        // Check s <2 items> QSet<int>.
        // Check s.22 22 int.
        // Check s.11 11 int.
        // Continue.
        dummyStatement(&s);
    }

    void testQSet2()
    {
        QSet<QString> s;
        s.insert("11.0");
        s.insert("22.0");
        BREAK_HERE;
        // Expand s.
        // Check s <2 items> QSet<QString>.
        // Check s.0 "11.0" QString.
        // Check s.1 "22.0" QString.
        // Continue.
        dummyStatement(&s);
    }

    void testQSet3()
    {
        QObject ob;
        QSet<QPointer<QObject> > s;
        QPointer<QObject> ptr(&ob);
        s.insert(ptr);
        s.insert(ptr);
        s.insert(ptr);
        BREAK_HERE;
        // Expand s.
        // Check s <1 items> QSet<QPointer<QObject>>.
        // CheckType s.0 QPointer<QObject>.
        // Continue.
        dummyStatement(&ptr, &s);
    }

    void testQSet()
    {
        testQSet1();
        testQSet2();
        testQSet3();
    }

} // namespace qset


namespace qsharedpointer {

    #if USE_SHARED_POINTER

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


    void testQSharedPointer1()
    {
        QSharedPointer<int> ptr(new int(43));
        QSharedPointer<int> ptr2 = ptr;
        QSharedPointer<int> ptr3 = ptr;
        BREAK_HERE;
        dummyStatement(&ptr, &ptr2, &ptr3);
    }

    void testQSharedPointer2()
    {
        QSharedPointer<QString> ptr(new QString("hallo"));
        QSharedPointer<QString> ptr2 = ptr;
        QSharedPointer<QString> ptr3 = ptr;
        BREAK_HERE;
        dummyStatement(&ptr, &ptr2, &ptr3);
    }

    void testQSharedPointer3()
    {
        QSharedPointer<int> iptr(new int(43));
        QWeakPointer<int> ptr(iptr);
        QWeakPointer<int> ptr2 = ptr;
        QWeakPointer<int> ptr3 = ptr;
        BREAK_HERE;
        dummyStatement(&ptr, &ptr2, &ptr3);
    }

    void testQSharedPointer4()
    {
        QSharedPointer<QString> sptr(new QString("hallo"));
        QWeakPointer<QString> ptr(sptr);
        QWeakPointer<QString> ptr2 = ptr;
        QWeakPointer<QString> ptr3 = ptr;
        BREAK_HERE;
        dummyStatement(&ptr, &ptr2, &ptr3);
    }

    void testQSharedPointer5()
    {
        QSharedPointer<Foo> fptr(new Foo(1));
        QWeakPointer<Foo> ptr(fptr);
        QWeakPointer<Foo> ptr2 = ptr;
        QWeakPointer<Foo> ptr3 = ptr;
        BREAK_HERE;
        dummyStatement(&ptr, &ptr2, &ptr3);
    }

    void testQSharedPointer()
    {
        testQSharedPointer1();
        testQSharedPointer2();
        testQSharedPointer3();
        testQSharedPointer4();
        testQSharedPointer5();
    }

    #else

    void testQSharedPointer() {}

    #endif

} // namespace qsharedpointer


namespace qxml {

    void testQXmlAttributes()
    {
        // only works with Python dumper
        QXmlAttributes atts;
        atts.append("name1", "uri1", "localPart1", "value1");
        atts.append("name2", "uri2", "localPart2", "value2");
        atts.append("name3", "uri3", "localPart3", "value3");
        BREAK_HERE;
        // Expand atts atts.attList atts.attList.1 atts.attList.2.
        // CheckType atts QXmlAttributes.
        // CheckType atts.[vptr] .
        // Check atts.attList <3 items> QXmlAttributes::AttributeList.
        // CheckType atts.attList.0 QXmlAttributes::Attribute.
        // Check atts.attList.0.localname "localPart1" QString.
        // Check atts.attList.0.qname "name1" QString.
        // Check atts.attList.0.uri "uri1" QString.
        // Check atts.attList.0.value "value1" QString.
        // CheckType atts.attList.1 QXmlAttributes::Attribute.
        // Check atts.attList.1.localname "localPart2" QString.
        // Check atts.attList.1.qname "name2" QString.
        // Check atts.attList.1.uri "uri2" QString.
        // Check atts.attList.1.value "value2" QString.
        // CheckType atts.attList.2 QXmlAttributes::Attribute.
        // Check atts.attList.2.localname "localPart3" QString.
        // Check atts.attList.2.qname "name3" QString.
        // Check atts.attList.2.uri "uri3" QString.
        // Check atts.attList.2.value "value3" QString.
        // CheckType atts.d QXmlAttributesPrivate.
        // Continue.
        dummyStatement();
    }

} // namespace qxml


namespace stdarray {

    void testStdArray()
    {
        #if USE_CXX11
        std::array<int, 4> a = { { 1, 2, 3, 4} };
        std::array<QString, 4> b = { { "1", "2", "3", "4"} };
        BREAK_HERE;
        // Expand a.
        // Check a <4 items> std::array<int, 4u>.
        // Check a <4 items> std::array<QString, 4u>.
        // Continue.
        dummyStatement(&a, &b);
        #endif
    }

} // namespace stdcomplex



namespace stdcomplex {

    void testStdComplex()
    {
        std::complex<double> c(1, 2);
        BREAK_HERE;
        // Expand c.
        // Check c (1.000000, 2.000000) std::complex<double>.
        // Continue.
        dummyStatement(&c);
    }

} // namespace stdcomplex


namespace stddeque {

    void testStdDequeInt()
    {
        std::deque<int> deque;
        deque.push_back(1);
        deque.push_back(2);
        BREAK_HERE;
        // Expand deque.
        // Check deque <2 items> std::deque<int>.
        // Check deque.0 1 int.
        // Check deque.1 2 int.
        // Continue.
        dummyStatement(&deque);
    }

    void testStdDequeIntStar()
    {
        // This is not supposed to work with the compiled dumpers.
        std::deque<int *> deque;
        deque.push_back(new int(1));
        deque.push_back(0);
        deque.push_back(new int(2));
        BREAK_HERE;
        // Expand deque.
        // Check deque <3 items> std::deque<int*>.
        // CheckType deque.0 int.
        // Check deque.1 0x0 int *.
        // Continue.
        deque.pop_back();
        deque.pop_front();
        deque.pop_front();
        dummyStatement(&deque);
    }

    void testStdDequeFoo()
    {
        std::deque<Foo> deque;
        deque.push_back(1);
        deque.push_front(2);
        BREAK_HERE;
        // Expand deque deque.0 deque.1.
        // Check deque <2 items> std::deque<Foo>.
        // CheckType deque.0 Foo.
        // Check deque.0.a 2 int.
        // CheckType deque.1 Foo.
        // Check deque.1.a 1 int.
        // Continue.
        dummyStatement(&deque);
    }

    void testStdDequeFooStar()
    {
        std::deque<Foo *> deque;
        deque.push_back(new Foo(1));
        deque.push_back(new Foo(2));
        BREAK_HERE;
        // Expand deque deque.0 deque.1.
        // Check deque <2 items> std::deque<Foo*>.
        // CheckType deque.0 Foo.
        // Check deque.0.a 1 int.
        // CheckType deque.1 Foo.
        // Check deque.1.a 2 int.
        // Continue.
        dummyStatement(&deque);
    }

    void testStdDeque()
    {
        testStdDequeInt();
        testStdDequeIntStar();
        testStdDequeFoo();
        testStdDequeFooStar();
    }

} // namespace stddeque


namespace stdhashset {

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
        BREAK_HERE;
        // Expand h.
        // Check h <4 items> __gnu__cxx::hash_set<int>.
        // Check h.0 194 int.
        // Check h.1 1 int.
        // Check h.2 2 int.
        // Check h.3 3 int.
        // Continue.
        dummyStatement(&h);
    #endif
    }

} // namespace stdhashset


namespace stdlist {

    void testStdListInt()
    {
        std::list<int> list;
        list.push_back(1);
        list.push_back(2);
        BREAK_HERE;
        // Expand list.
        // Check list <2 items> std::list<int>.
        // Check list.0 1 int.
        // Check list.1 2 int.
        // Continue.
        dummyStatement(&list);
    }

    void testStdListIntStar()
    {
        std::list<int *> list;
        list.push_back(new int(1));
        list.push_back(0);
        list.push_back(new int(2));
        BREAK_HERE;
        // Expand list.
        // Check list <3 items> std::list<int*>.
        // CheckType list.0 int.
        // Check list.1 0x0 int *.
        // CheckType list.2 int.
        // Continue.
        dummyStatement(&list);
    }

    void testStdListIntBig()
    {
        // This is not supposed to work with the compiled dumpers.
        std::list<int> list;
        for (int i = 0; i < 10000; ++i)
            list.push_back(i);
        BREAK_HERE;
        // Expand list.
        // Check list <more than 1000 items> std::list<int>.
        // Check list.0 0 int.
        // Check list.999 999 int.
        // Continue.
        dummyStatement(&list);
    }

    void testStdListFoo()
    {
        std::list<Foo> list;
        list.push_back(15);
        list.push_back(16);
        BREAK_HERE;
        // Expand list list.0 list.1.
        // Check list <2 items> std::list<Foo>.
        // CheckType list.0 Foo.
        // Check list.0.a 15 int.
        // CheckType list.1 Foo.
        // Check list.1.a 16 int.
        // Continue.
        dummyStatement(&list);
    }

    void testStdListFooStar()
    {
        std::list<Foo *> list;
        list.push_back(new Foo(1));
        list.push_back(0);
        list.push_back(new Foo(2));
        BREAK_HERE;
        // Expand list list.0 list.2.
        // Check list <3 items> std::list<Foo*>.
        // CheckType list.0 Foo.
        // Check list.0.a 1 int.
        // Check list.1 0x0 Foo *.
        // CheckType list.2 Foo.
        // Check list.2.a 2 int.
        // Continue.
        dummyStatement(&list);
    }

    void testStdListBool()
    {
        std::list<bool> list;
        list.push_back(true);
        list.push_back(false);
        BREAK_HERE;
        // Expand list.
        // Check list <2 items> std::list<bool>.
        // Check list.0 true bool.
        // Check list.1 false bool.
        // Continue.
        dummyStatement(&list);
    }

    void testStdList()
    {
        testStdListInt();
        testStdListIntStar();
        testStdListIntBig();
        testStdListFoo();
        testStdListFooStar();
        testStdListBool();
    }

} // namespace stdlist


namespace stdmap {

    void testStdMapStringFoo()
    {
        // This is not supposed to work with the compiled dumpers.
        std::map<QString, Foo> map;
        map["22.0"] = Foo(22);
        map["33.0"] = Foo(33);
        map["44.0"] = Foo(44);
        BREAK_HERE;
        // Expand map map.0 map.0.second map.2 map.2.second.
        // Check map <3 items> std::map<QString, Foo>.
        // Check map.0   std::pair<QString const, Foo>.
        // Check map.0.first "22.0" QString.
        // CheckType map.0.second Foo.
        // Check map.0.second.a 22 int.
        // Check map.1   std::pair<QString const, Foo>.
        // Check map.2.first "44.0" QString.
        // CheckType map.2.second Foo.
        // Check map.2.second.a 44 int.
        // Continue.
        dummyStatement(&map);
    }

    void testStdMapCharStarFoo()
    {
        std::map<const char *, Foo> map;
        map["22.0"] = Foo(22);
        map["33.0"] = Foo(33);
        BREAK_HERE;
        // Expand map map.0 map.0.first map.0.second map.1 map.1.second.
        // Check map <2 items> std::map<char const*, Foo>.
        // Check map.0   std::pair<char const* const, Foo>.
        // CheckType map.0.first char *.
        // Check map.0.first.*first 50 '2' char.
        // CheckType map.0.second Foo.
        // Check map.0.second.a 22 int.
        // Check map.1   std::pair<char const* const, Foo>.
        // CheckType map.1.first char *.
        // Check map.1.first.*first 51 '3' char.
        // CheckType map.1.second Foo.
        // Check map.1.second.a 33 int.
        // Continue.
        dummyStatement(&map);
    }

    void testStdMapUIntUInt()
    {
        std::map<uint, uint> map;
        map[11] = 1;
        map[22] = 2;
        BREAK_HERE;
        // Expand map.
        // Check map <2 items> std::map<unsigned int, unsigned int>.
        // Check map.11 1 unsigned int.
        // Check map.22 2 unsigned int.
        // Continue.
        dummyStatement(&map);
    }

    void testStdMapUIntStringList()
    {
        std::map<uint, QStringList> map;
        map[11] = QStringList() << "11";
        map[22] = QStringList() << "22";
        BREAK_HERE;
        // Expand map map.0 map.0.first map.0.second map.1 map.1.second.
        // Check map <2 items> std::map<unsigned int, QStringList>.
        // Check map.0   std::pair<unsigned int const, QStringList>.
        // Check map.0.first 11 unsigned int.
        // Check map.0.second <1 items> QStringList.
        // Check map.0.second.0 "11" QString.
        // Check map.1   std::pair<unsigned int const, QStringList>.
        // Check map.1.first 22 unsigned int.
        // Check map.1.second <1 items> QStringList.
        // Check map.1.second.0 "22" QString.
        // Continue.
        dummyStatement(&map);
    }

    void testStdMapUIntStringListTypedef()
    {
        typedef std::map<uint, QStringList> T;
        T map;
        map[11] = QStringList() << "11";
        map[22] = QStringList() << "22";
        BREAK_HERE;
        // Check map <2 items> stdmap::T.
        // Continue.
        dummyStatement(&map);
    }

    void testStdMapUIntFloat()
    {
        std::map<uint, float> map;
        map[11] = 11.0;
        map[22] = 22.0;
        BREAK_HERE;
        // Expand map.
        // Check map <2 items> std::map<unsigned int, float>.
        // Check map.11 11 float.
        // Check map.22 22 float.
        // Continue.
        dummyStatement(&map);
    }

    void testStdMapUIntFloatIterator()
    {
        typedef std::map<int, float> Map;
        Map map;
        map[11] = 11.0;
        map[22] = 22.0;
        map[33] = 33.0;
        map[44] = 44.0;
        map[55] = 55.0;
        map[66] = 66.0;

        Map::iterator it1 = map.begin();
        Map::iterator it2 = it1; ++it2;
        Map::iterator it3 = it2; ++it3;
        Map::iterator it4 = it3; ++it4;
        Map::iterator it5 = it4; ++it5;
        Map::iterator it6 = it5; ++it6;

        BREAK_HERE;
        // Expand map.
        // Check map <6 items> stdmap::Map.
        // Check map.11 11 float.
        // Check it1.first 11 int.
        // Check it1.second 11 float.
        // Check it6.first 66 int.
        // Check it6.second 66 float.
        // Continue.
        dummyStatement(&map, &it1, &it2, &it3, &it4, &it5, &it6);
    }

    void testStdMapStringFloat()
    {
        std::map<QString, float> map;
        map["11.0"] = 11.0;
        map["22.0"] = 22.0;
        BREAK_HERE;
        // Expand map map.0 map.1.
        // Check map <2 items> std::map<QString, float>.
        // Check map.0   std::pair<QString const, float>.
        // Check map.0.first "11.0" QString.
        // Check map.0.second 11 float.
        // Check map.1   std::pair<QString const, float>.
        // Check map.1.first "22.0" QString.
        // Check map.1.second 22 float.
        // Continue.
        dummyStatement(&map);
    }

    void testStdMapIntString()
    {
        std::map<int, QString> map;
        map[11] = "11.0";
        map[22] = "22.0";
        BREAK_HERE;
        // Expand map map.0 map.1.
        // Check map <2 items> std::map<int, QString>.
        // Check map.0   std::pair<int const, QString>.
        // Check map.0.first 11 int.
        // Check map.0.second "11.0" QString.
        // Check map.1   std::pair<int const, QString>.
        // Check map.1.first 22 int.
        // Check map.1.second "22.0" QString.
        // Continue.
        dummyStatement(&map);
    }

    void testStdMapStringPointer()
    {
        QObject ob;
        std::map<QString, QPointer<QObject> > map;
        map["Hallo"] = QPointer<QObject>(&ob);
        map["Welt"] = QPointer<QObject>(&ob);
        map["."] = QPointer<QObject>(&ob);
        BREAK_HERE;
        // Expand map map.0 map.2.
        // Check map <3 items> std::map<QString, QPointer<QObject>>.
        // Check map.0   std::pair<QString const, QPointer<QObject>>.
        // Check map.0.first "." QString.
        // CheckType map.0.second QPointer<QObject>.
        // Check map.2   std::pair<QString const, QPointer<QObject>>.
        // Check map.2.first "Welt" QString.
        // Continue.
        dummyStatement(&map);
    }

    void testStdMap()
    {
        testStdMapStringFoo();
        testStdMapCharStarFoo();
        testStdMapUIntUInt();
        testStdMapUIntStringList();
        testStdMapUIntStringListTypedef();
        testStdMapUIntFloat();
        testStdMapUIntFloatIterator();
        testStdMapStringFloat();
        testStdMapIntString();
        testStdMapStringPointer();
    }

} // namespace stdmap


namespace stdptr {

    void testStdUniquePtrInt()
    {
        #ifdef USE_CXX11
        std::unique_ptr<int> p(new int(32));
        BREAK_HERE;
        // Check p 32 std::unique_ptr<int, std::default_delete<int> >.
        // Continue.
        dummyStatement(&p);
        #endif
    }

    void testStdUniquePtrFoo()
    {
        #ifdef USE_CXX11
        std::unique_ptr<Foo> p(new Foo);
        BREAK_HERE;
        // Check p 32 std::unique_ptr<Foo, std::default_delete<Foo> >.
        // Continue.
        dummyStatement(&p);
        #endif
    }

    void testStdSharedPtrInt()
    {
        #ifdef USE_CXX11
        std::shared_ptr<int> p(new int(32));
        BREAK_HERE;
        // Check p 32 std::shared_ptr<int, std::default_delete<int> >.
        // Continue.
        dummyStatement(&p);
        #endif
    }

    void testStdSharedPtrFoo()
    {
        #ifdef USE_CXX11
        std::shared_ptr<Foo> p(new Foo);
        BREAK_HERE;
        // Check p 32 std::shared_ptr<Foo, std::default_delete<int> >.
        // Continue.
        dummyStatement(&p);
        #endif
    }

    void testStdPtr()
    {
        testStdUniquePtrInt();
        testStdUniquePtrFoo();
        testStdSharedPtrInt();
        testStdSharedPtrFoo();
    }

} // namespace stdptr


namespace stdset {

    void testStdSetInt()
    {
        // This is not supposed to work with the compiled dumpers.
        std::set<int> set;
        set.insert(11);
        set.insert(22);
        set.insert(33);
        BREAK_HERE;
        // Check set <3 items> std::set<int>
        // Continue.
        dummyStatement(&set);
    }

    void testStdSetIntIterator()
    {
        typedef std::set<int> Set;
        Set set;
        set.insert(11.0);
        set.insert(22.0);
        set.insert(33.0);
        set.insert(44.0);
        set.insert(55.0);
        set.insert(66.0);

        Set::iterator it1 = set.begin();
        Set::iterator it2 = it1; ++it2;
        Set::iterator it3 = it2; ++it3;
        Set::iterator it4 = it3; ++it4;
        Set::iterator it5 = it4; ++it5;
        Set::iterator it6 = it5; ++it6;

        BREAK_HERE;
        // Check set <6 items> stdset::Set.
        // Check it1.value 11 int.
        // Check it6.value 66 int.
        // Continue.
        dummyStatement(&set, &it1, &it2, &it3, &it4, &it5, &it6);
    }

    void testStdSetString()
    {
        std::set<QString> set;
        set.insert("22.0");
        BREAK_HERE;
        // Expand set.
        // Check set <1 items> std::set<QString>.
        // Check set.0 "22.0" QString.
        // Continue.
        dummyStatement(&set);
    }

    void testStdSetPointer()
    {
        QObject ob;
        std::set<QPointer<QObject> > hash;
        QPointer<QObject> ptr(&ob);
        BREAK_HERE;
        // Check hash <0 items> std::set<QPointer<QObject>, std::less<QPointer<QObject>>, std::allocator<QPointer<QObject>>>.
        // Check ob "" QObject.
        // CheckType ptr QPointer<QObject>.
        // Continue.
        dummyStatement(&ptr);
    }

    void testStdSet()
    {
        testStdSetInt();
        testStdSetIntIterator();
        testStdSetString();
        testStdSetPointer();
    }

} // namespace stdset


namespace stdstack {

    void testStdStack1()
    {
        // This does not work with the compiled dumpers.
        std::stack<int *> s;
        BREAK_HERE;
        // Check s <0 items> std::stack<int*>.
        // Continue.
        s.push(new int(1));
        BREAK_HERE;
        // Check s <1 items> std::stack<int*>.
        // Continue.
        s.push(0);
        BREAK_HERE;
        // Check s <2 items> std::stack<int*>.
        // Continue.
        s.push(new int(2));
        BREAK_HERE;
        // Expand s.
        // Check s <3 items> std::stack<int*>.
        // CheckType s.0 int.
        // Check s.1 0x0 int *.
        // CheckType s.2 int.
        // Continue.
        s.pop();
        BREAK_HERE;
        // Check s <2 items> std::stack<int*>.
        // Continue.
        s.pop();
        BREAK_HERE;
        // Check s <1 items> std::stack<int*>.
        // Continue.
        s.pop();
        BREAK_HERE;
        // Check s <0 items> std::stack<int*>.
        // Continue.
        dummyStatement(&s);
    }

    void testStdStack2()
    {
        std::stack<int> s;
        BREAK_HERE;
        // Check s <0 items> std::stack<int>.
        // Continue.
        s.push(1);
        BREAK_HERE;
        // Check s <1 items> std::stack<int>.
        // Continue.
        s.push(2);
        BREAK_HERE;
        // Expand s.
        // Check s <2 items> std::stack<int>.
        // Check s.0 1 int.
        // Check s.1 2 int.
        // Continue.
        dummyStatement(&s);
    }

    void testStdStack3()
    {
        std::stack<Foo *> s;
        BREAK_HERE;
        // Check s <0 items> std::stack<Foo*>.
        // Continue.
        s.push(new Foo(1));
        BREAK_HERE;
        // Check s <1 items> std::stack<Foo*>.
        // Continue.
        s.push(new Foo(2));
        BREAK_HERE;
        // Expand s s.0 s.1.
        // Check s <2 items> std::stack<Foo*>.
        // CheckType s.0 Foo.
        // Check s.0.a 1 int.
        // CheckType s.1 Foo.
        // Check s.1.a 2 int.
        // Continue.
        dummyStatement(&s);
    }

    void testStdStack4()
    {
        std::stack<Foo> s;
        BREAK_HERE;
        // Check s <0 items> std::stack<Foo>.
        // Continue.
        s.push(1);
        BREAK_HERE;
        // Check s <1 items> std::stack<Foo>.
        // Continue.
        s.push(2);
        BREAK_HERE;
        // Expand s s.0 s.1.
        // Check s <2 items> std::stack<Foo>.
        // CheckType s.0 Foo.
        // Check s.0.a 1 int.
        // CheckType s.1 Foo.
        // Check s.1.a 2 int.
        // Continue.
        dummyStatement(&s);
    }

    void testStdStack()
    {
        testStdStack1();
        testStdStack2();
        testStdStack3();
        testStdStack4();
    }

} // namespace stdstack


namespace stdstring {

    void testStdString1()
    {
        std::string str;
        std::wstring wstr;
        BREAK_HERE;
        // Check str "" std::string.
        // Check wstr "" std::wstring.
        // Continue.
        str += "b";
        wstr += wchar_t('e');
        str += "d";
        wstr += wchar_t('e');
        str += "e";
        str += "b";
        str += "d";
        str += "e";
        wstr += wchar_t('e');
        wstr += wchar_t('e');
        str += "e";
        BREAK_HERE;
        // Check str "bdebdee" std::string.
        // Check wstr "eeee" std::wstring.
        // Continue.
        dummyStatement(&str, &wstr);
    }

    void testStdString2()
    {
        std::string str = "foo";
        QList<std::string> l;
        BREAK_HERE;
        // Check l <0 items> QList<std::string>.
        // Check str "foo" std::string.
        // Continue.
        l.push_back(str);
        BREAK_HERE;
        // Check l <1 items> QList<std::string>.
        // Continue.
        l.push_back(str);
        BREAK_HERE;
        // Check l <2 items> QList<std::string>.
        // Continue.
        l.push_back(str);
        BREAK_HERE;
        // Check l <3 items> QList<std::string>.
        // Continue.
        l.push_back(str);
        BREAK_HERE;
        // Expand l.
        // Check l <4 items> QList<std::string>.
        // CheckType l.0 std::string.
        // CheckType l.3 std::string.
        // Check str "foo" std::string.
        // Continue.
        dummyStatement(&str, &l);
    }

    void testStdString3()
    {
        std::string str = "foo";
        std::vector<std::string> v;
        BREAK_HERE;
        // Check str "foo" std::string.
        // Check v <0 items> std::vector<std::string>.
        // Continue.
        v.push_back(str);
        BREAK_HERE;
        // Check v <1 items> std::vector<std::string>.
        // Continue.
        v.push_back(str);
        BREAK_HERE;
        // Check v <2 items> std::vector<std::string>.
        // Continue.
        v.push_back(str);
        BREAK_HERE;
        // Check v <3 items> std::vector<std::string>.
        // Continue.
        v.push_back(str);
        BREAK_HERE;
        // Expand v.
        // Check v <4 items> std::vector<std::string>.
        // Check v.0 "foo" std::string.
        // Check v.3 "foo" std::string.
        // Continue.
        dummyStatement(&str, &v);
    }

    void testStdString()
    {
        testStdString1();
        testStdString2();
        testStdString3();
    }

} // namespace stdstring


namespace stdvector {

    void testStdVector1()
    {
        std::vector<int *> v;
        BREAK_HERE;
        // Check v <0 items> std::vector<int*>.
        // Continue.
        v.push_back(new int(1));
        BREAK_HERE;
        // Check v <1 items> std::vector<int*>.
        // Continue.
        v.push_back(0);
        BREAK_HERE;
        // Check v <2 items> std::vector<int*>.
        // Continue.
        v.push_back(new int(2));
        BREAK_HERE;
        // Expand v.
        // Check v <3 items> std::vector<int*>.
        // Check v.0 1 int.
        // Check v.1 0x0 int *.
        // Check v.2 2 int.
        // Continue.
        dummyStatement(&v);
    }

    void testStdVector2()
    {
        std::vector<int> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        v.push_back(4);
        BREAK_HERE;
        // Expand v.
        // Check v <4 items> std::vector<int>.
        // Check v.0 1 int.
        // Check v.3 4 int.
        // Continue.
        dummyStatement(&v);
    }

    void testStdVector3()
    {
        std::vector<Foo *> v;
        v.push_back(new Foo(1));
        v.push_back(0);
        v.push_back(new Foo(2));
        BREAK_HERE;
        // Expand v v.0 v.0.x v.2.
        // Check v <3 items> std::vector<Foo*>.
        // CheckType v.0 Foo.
        // Check v.0.a 1 int.
        // Check v.1 0x0 Foo *.
        // CheckType v.2 Foo.
        // Check v.2.a 2 int.
        // Continue.
        dummyStatement(&v);
    }

    void testStdVector4()
    {
        std::vector<Foo> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        v.push_back(4);
        BREAK_HERE;
        // Expand v v.0 v.0.x.
        // Check v <4 items> std::vector<Foo>.
        // CheckType v.0 Foo.
        // Check v.1.a 2 int.
        // CheckType v.3 Foo.
        // Continue.
        dummyStatement(&v);
    }

    void testStdVectorBool1()
    {
        std::vector<bool> v;
        v.push_back(true);
        v.push_back(false);
        v.push_back(false);
        v.push_back(true);
        v.push_back(false);
        BREAK_HERE;
        // Expand v.
        // Check v <5 items> std::vector<bool>.
        // Check v.0 true bool.
        // Check v.1 false bool.
        // Check v.2 false bool.
        // Check v.3 true bool.
        // Check v.4 false bool.
        // Continue.
        dummyStatement(&v);
    }

    void testStdVectorBool2()
    {
        std::vector<bool> v1(65, true);
        std::vector<bool> v2(65);
        BREAK_HERE;
        // Expand v1.
        // Expand v2.
        // Check v1 <65 items> std::vector<bool>.
        // Check v1.0 true bool.
        // Check v1.64 true bool.
        // Check v2 <65 items> std::vector<bool>.
        // Check v2.0 false bool.
        // Check v2.64 false bool.
        // Continue.
        dummyStatement(&v1, &v2);
    }

    void testStdVector6()
    {
        std::vector<std::list<int> *> vector;
        std::list<int> list;
        vector.push_back(new std::list<int>(list));
        vector.push_back(0);
        list.push_back(45);
        vector.push_back(new std::list<int>(list));
        vector.push_back(0);
        BREAK_HERE;
        // Expand list vector vector.2.
        // Check list <1 items> std::list<int>.
        // Check list.0 45 int.
        // Check vector <4 items> std::vector<std::list<int>*>.
        // Check vector.0 <0 items> std::list<int>.
        // Check vector.2 <1 items> std::list<int>.
        // Check vector.2.0 45 int.
        // Check vector.3 0x0 std::list<int> *.
        // Continue.
        dummyStatement(&vector, &list);
    }

    void testStdVector()
    {
        testStdVector1();
        testStdVector2();
        testStdVector3();
        testStdVector4();
        testStdVectorBool1();
        testStdVectorBool2();
        testStdVector6();
    }

} // namespace stdvector


namespace stdstream {

    void testStdStream()
    {
        using namespace std;
        ifstream is;
        BREAK_HERE;
        // CheckType is std::ifstream.
        // Continue.
#ifdef Q_OS_WIN
        is.open("C:\\Program Files\\Windows NT\\Accessories\\wordpad.exe");
#else
        is.open("/etc/passwd");
#endif
        BREAK_HERE;
        // Continue.
        bool ok = is.good();
        BREAK_HERE;
        // Check ok true bool.
        // Continue.
        dummyStatement(&is, &ok);
    }

} // namespace stdstream


namespace itemmodel {

    void testItemModel()
    {
        #if USE_GUILIB
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
        BREAK_HERE;
        // CheckType i1 QStandardItem.
        // CheckType i11 QStandardItem.
        // CheckType i2 QStandardItem.
        // Check m  QStandardItemModel.
        // Check mi "1" QModelIndex.
        // Continue.
        dummyStatement(&i1, &mi, &m, &i2, &i11);
        #endif
    }

} // namespace itemmodel


namespace qstack {

    void testQStackInt()
    {
        QStack<int> s;
        s.append(1);
        s.append(2);
        BREAK_HERE;
        // Expand s.
        // Check s <2 items> QStack<int>.
        // Check s.0 1 int.
        // Check s.1 2 int.
        // Continue.
        dummyStatement(&s);
    }

    void testQStackBig()
    {
        QStack<int> s;
        for (int i = 0; i != 10000; ++i)
            s.append(i);
        BREAK_HERE;
        // Expand s.
        // Check s <10000 items> QStack<int>.
        // Check s.0 0 int.
        // Check s.1999 1999 int.
        // Continue.
        dummyStatement(&s);
    }

    void testQStackFooPointer()
    {
        QStack<Foo *> s;
        s.append(new Foo(1));
        s.append(0);
        s.append(new Foo(2));
        BREAK_HERE;
        // Expand s s.0 s.2.
        // Check s <3 items> QStack<Foo*>.
        // CheckType s.0 Foo.
        // Check s.0.a 1 int.
        // Check s.1 0x0 Foo *.
        // CheckType s.2 Foo.
        // Check s.2.a 2 int.
        // Continue.
        dummyStatement(&s);
    }

    void testQStackFoo()
    {
        QStack<Foo> s;
        s.append(1);
        s.append(2);
        s.append(3);
        s.append(4);
        BREAK_HERE;
        // Expand s s.0 s.3.
        // Check s <4 items> QStack<Foo>.
        // CheckType s.0 Foo.
        // Check s.0.a 1 int.
        // CheckType s.3 Foo.
        // Check s.3.a 4 int.
        // Continue.
        dummyStatement(&s);
    }

    void testQStackBool()
    {
        QStack<bool> s;
        s.append(true);
        s.append(false);
        BREAK_HERE;
        // Expand s.
        // Check s <2 items> QStack<bool>.
        // Check s.0 true bool.
        // Check s.1 false bool.
        // Continue.
        dummyStatement(&s);
    }

    void testQStack()
    {
        testQStackInt();
        testQStackBig();
        testQStackFoo();
        testQStackFooPointer();
        testQStackBool();
    }

} // namespace qstack


namespace qurl {

    void testQUrl()
    {
        QUrl url(QString("http://qt-project.org"));
        BREAK_HERE;
        // Check url "http://qt-project.org" QUrl.
        // Continue.
        dummyStatement(&url);
    }

} // namespace qurl


namespace qstring  {

    void testQStringQuotes()
    {
        QString str1("Hello Qt"); // --> Value: "Hello Qt"
        QString str2("Hello\nQt"); // --> Value: ""Hello\nQt"" (double quote not expected)
        QString str3("Hello\rQt"); // --> Value: ""HelloQt"" (double quote and missing \r not expected)
        QString str4("Hello\tQt"); // --> Value: "Hello\9Qt" (expected \t instead of \9)
        BREAK_HERE;
        // Check str1 "Hello Qt" QString.
        // Check str2 "Hello\nQt" QString.
        // Check str3 "Hello\rQt" QString.
        // Check str4 "Hello\tQt" QString.
        // Continue.
        dummyStatement(&str1, &str2, &str3, &str4);
    }

    void testQString1()
    {
        QString str = "Hello ";
        str += " big, ";
        str += "\t";
        str += "\r";
        str += "\n";
        str += QLatin1Char(0);
        str += QLatin1Char(1);
        str += " fat ";
        str += " World";
        str.prepend("Prefix: ");
        BREAK_HERE;
        // Check str "Prefix: Hello  big, \t\r\n\000\001 fat  World" QString.
        // Continue.
        dummyStatement(&str);
    }

    void testQString2()
    {
        QChar data[] = { 'H', 'e', 'l', 'l', 'o' };
        QString str1 = QString::fromRawData(data, 4);
        QString str2 = QString::fromRawData(data + 1, 4);
        BREAK_HERE;
        // Check str1 "Hell" QString.
        // Check str2 "ello" QString.
        // Continue.
        dummyStatement(&str1, &str2, &data);
    }

    void stringRefTest(const QString &refstring)
    {
        dummyStatement(&refstring);
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
        BREAK_HERE;
        // CheckType pstring QString.
        // Check str "Hello  big,  fat  World " QString.
        // Check string "HiDu" QString.
        // Continue.
        delete pstring;
        dummyStatement(&str, &string, pstring);
    }

    void testQStringRef()
    {
        QString str = "Hello";
        QStringRef ref(&str, 1, 2);
        BREAK_HERE;
        // Check ref "el" QStringRef.
        // Continue.
        dummyStatement(&str, &ref);
    }

    void testQString()
    {
        testQString1();
        testQString2();
        testQString3();
        testQStringRef();
        testQStringQuotes();
    }

} // namespace qstring


namespace qstringlist {

    void testQStringList()
    {
        QStringList l;
        l << "Hello ";
        l << " big, ";
        l << " fat ";
        l.takeFirst();
        l << " World ";
        BREAK_HERE;
        // Expand l.
        // Check l <3 items> QStringList.
        // Check l.0 " big, " QString.
        // Check l.1 " fat " QString.
        // Check l.2 " World " QString.
        // Continue.
        dummyStatement(&l);
    }

} // namespace qstringlist


namespace formats {

    void testString()
    {
        const wchar_t *w = L"aöa";
        QString u;
        if (sizeof(wchar_t) == 4)
            u = QString::fromUcs4((uint *)w);
        else
            u = QString::fromUtf16((ushort *)w);
        BREAK_HERE;
        // Check u "aöa" QString.
        // CheckType w wchar_t *.
        // Continue.

        // All: Select UTF-8 in "Change Format for Type" in L&W context menu.
        // Windows: Select UTF-16 in "Change Format for Type" in L&W context menu.
        // Other: Select UCS-6 in "Change Format for Type" in L&W context menu.
        dummyStatement(&u, &w);
    }

    void testCharPointers()
    {
        // These tests should result in properly displayed umlauts in the
        // Locals&Watchers view. It is only support on gdb with Python.

        const char *s = "aöa";
        const char *t = "a\xc3\xb6";
        const wchar_t *w = L"aöa";
        BREAK_HERE;
        // CheckType s char *.
        // CheckType t char *.
        // CheckType w wchar_t *.
        // Continue.

        // All: Select UTF-8 in "Change Format for Type" in L&W context menu.
        // Windows: Select UTF-16 in "Change Format for Type" in L&W context menu.
        // Other: Select UCS-6 in "Change Format for Type" in L&W context menu.

        const unsigned char uu[] = { 'a', 153 /* ö Latin1 */, 'a' };
        const unsigned char *u = uu;
        BREAK_HERE;
        // CheckType u unsigned char *.
        // CheckType uu unsigned char [3].
        // Continue.

        // Make sure to undo "Change Format".
        dummyStatement(&s, &w, &t, &u);
    }

    void testCharArrays()
    {
        // These tests should result in properly displayed umlauts in the
        // Locals&Watchers view. It is only support on gdb with Python.

        const char s[] = "aöa";
        const char t[] = "aöax";
        const wchar_t w[] = L"aöa";
        BREAK_HERE;
        // CheckType s char [5].
        // CheckType t char [6].
        // CheckType w wchar_t [4].
        // Continue.

        // All: Select UTF-8 in "Change Format for Type" in L&W context menu.
        // Windows: Select UTF-16 in "Change Format for Type" in L&W context menu.
        // Other: Select UCS-6 in "Change Format for Type" in L&W context menu.

        // Make sure to undo "Change Format".
        dummyStatement(&s, &w, &t);
    }

    void testFormats()
    {
        testCharPointers();
        testCharArrays();
        testString();
    }

} // namespace formats


namespace text {

    void testText()
    {
        #if USE_GUILIB
        //char *argv[] = { "xxx", 0 };
        QTextDocument doc;
        doc.setPlainText("Hallo\nWorld");
        QTextCursor tc;
        BREAK_HERE;
        // CheckType doc QTextDocument.
        // Continue.
        tc = doc.find("all");
        BREAK_HERE;
        // Check tc 4 QTextCursor.
        // Continue.
        int pos = tc.position();
        BREAK_HERE;
        // Check pos 4 int.
        // Continue.
        int anc = tc.anchor();
        BREAK_HERE;
        // Check anc 1 int.
        // Continue.
        dummyStatement(&pos, &anc);
        #endif
    }

} // namespace text


namespace qprocess {

    void testQProcess()
    {
        return;
        const int N = 14;
        QProcess proc[N];
        for (int i = 0; i != N; ++i) {
            proc[i].start("sleep 10");
            proc[i].waitForStarted();
        }
        BREAK_HERE;
        dummyStatement(&proc);
    }

} // namespace qprocess


namespace qthread {

    class Thread : public QThread
    {
    public:
        Thread() {}

        void setId(int id) { m_id = id; }

        void run()
        {
            int j = 2;
            ++j;
            for (int i = 0; i != 1000; ++i) {
                //sleep(1);
                std::cerr << m_id;
            }
            if (m_id == 2) {
                ++j;
            }
            if (m_id == 3) {
                BREAK_HERE;
                // Expand this.
                // Expand this.@1.
                // Check j 3 int.
                // CheckType this qthread::Thread.
                // Check this.@1  QThread.
                // Check this.@1.@1 "This is thread #3" QObject.
                // Continue.
                dummyStatement(this);
            }
            std::cerr << j;
        }

    private:
        int m_id;
    };

    void testQThread()
    {
        //return;
        const int N = 14;
        Thread thread[N];
        for (int i = 0; i != N; ++i) {
            thread[i].setId(i);
            thread[i].setObjectName("This is thread #" + QString::number(i));
            thread[i].start();
        }
        BREAK_HERE;
        // Expand thread.
        // Expand thread.0.
        // Expand thread.0.@1.
        // Expand thread.13.
        // Expand thread.13.@1.
        // CheckType thread qthread::Thread [14].
        // Check thread.0  qthread::Thread.
        // Check thread.0.@1.@1 "This is thread #0" qthread::Thread.
        // Check thread.13  qthread::Thread.
        // Check thread.13.@1.@1 "This is thread #13" qthread::Thread.
        // Continue.
        for (int i = 0; i != N; ++i) {
            thread[i].wait();
        }
        dummyStatement(&thread);
    }
}


Q_DECLARE_METATYPE(QHostAddress)
Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QStringList)
#define COMMA ,
Q_DECLARE_METATYPE(QMap<uint COMMA QStringList>)

namespace qvariant {

    void testQVariant1()
    {
        QVariant value;
        QVariant::Type t = QVariant::String;
        value = QVariant(t, (void*)0);
        *(QString*)value.data() = QString("Some string");
        int i = 1;
        BREAK_HERE;
        // Check t QVariant::String (10) QVariant::Type.
        // Check value "Some string" QVariant (QString).
        // Continue.

        // Check the variant contains a proper QString.
        dummyStatement(&i);
    }

    void testQVariant2()
    {
        // Check var contains objects of the types indicated.

        QVariant var;                        // Type 0, invalid
        BREAK_HERE;
        // Check var (invalid) QVariant (invalid).
        // Continue.
        var.setValue(true);                  // 1, bool
        BREAK_HERE;
        // Check var true QVariant (bool).
        // Continue.
        var.setValue(2);                     // 2, int
        BREAK_HERE;
        // Check var 2 QVariant (int).
        // Continue.
        var.setValue(3u);                    // 3, uint
        BREAK_HERE;
        // Check var 3 QVariant (uint).
        // Continue.
        var.setValue(qlonglong(4));          // 4, qlonglong
        BREAK_HERE;
        // Check var 4 QVariant (qlonglong).
        // Continue.
        var.setValue(qulonglong(5));         // 5, qulonglong
        BREAK_HERE;
        // Check var 5 QVariant (qulonglong).
        // Continue.
        var.setValue(double(6));             // 6, double
        BREAK_HERE;
        // Check var 6 QVariant (double).
        // Continue.
        var.setValue(QChar(7));              // 7, QChar
        BREAK_HERE;
        // Check var '?' (7) QVariant (QChar).
        // Continue.
        //None,          # 8, QVariantMap
        // None,          # 9, QVariantList
        var.setValue(QString("Hello 10"));   // 10, QString
        BREAK_HERE;
        // Check var "Hello 10" QVariant (QString).
        // Continue.

        #if USE_GUILIB
        var.setValue(QRect(100, 200, 300, 400)); // 19 QRect
        BREAK_HERE;
        // Check var 300x400+100+200 QVariant (QRect).
        // Continue.
        var.setValue(QRectF(100, 200, 300, 400)); // 19 QRectF
        BREAK_HERE;
        // Check var 300x400+100+200 QVariant (QRectF).
        // Continue.
        #endif

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
        dummyStatement(&var);
    }

    void testQVariant3()
    {
        QVariant var;
        BREAK_HERE;
        // Check var (invalid) QVariant (invalid).
        // Continue.

        // Check the list is updated properly.
        var.setValue(QStringList() << "World");
        BREAK_HERE;
        // Expand var.
        // Check var <1 items> QVariant (QStringList).
        // Check var.0 "World" QString.
        // Continue.
        var.setValue(QStringList() << "World" << "Hello");
        BREAK_HERE;
        // Expand var.
        // Check var <2 items> QVariant (QStringList).
        // Check var.1 "Hello" QString.
        // Continue.
        var.setValue(QStringList() << "Hello" << "Hello");
        BREAK_HERE;
        // Expand var.
        // Check var <2 items> QVariant (QStringList).
        // Check var.0 "Hello" QString.
        // Check var.1 "Hello" QString.
        // Continue.
        var.setValue(QStringList() << "World" << "Hello" << "Hello");
        BREAK_HERE;
        // Expand var.
        // Check var <3 items> QVariant (QStringList).
        // Check var.0 "World" QString.
        // Continue.
        dummyStatement(&var);
    }

    void testQVariant4()
    {
        QVariant var;
        QHostAddress ha("127.0.0.1");
        var.setValue(ha);
        QHostAddress ha1 = var.value<QHostAddress>();
        BREAK_HERE;
        // Expand ha ha1 var.
        // Check ha "127.0.0.1" QHostAddress.
        // Check ha.a 0 quint32.
        // CheckType ha.a6 Q_IPV6ADDR.
        // Check ha.ipString "127.0.0.1" QString.
        // Check ha.isParsed false bool.
        // Check ha.protocol QAbstractSocket::UnknownNetworkLayerProtocol (-1) QAbstractSocket::NetworkLayerProtocol.
        // Check ha.scopeId "" QString.
        // Check ha1 "127.0.0.1" QHostAddress.
        // Check ha1.a 0 quint32.
        // CheckType ha1.a6 Q_IPV6ADDR.
        // Check ha1.ipString "127.0.0.1" QString.
        // Check ha1.isParsed false bool.
        // Check ha1.protocol QAbstractSocket::UnknownNetworkLayerProtocol (-1) QAbstractSocket::NetworkLayerProtocol.
        // Check ha1.scopeId "" QString.
        // CheckType var QVariant (QHostAddress).
        // Check var.data "127.0.0.1" QHostAddress.
        // Continue.
        dummyStatement(&ha1);
    }

    void testQVariant5()
    {
        // This checks user defined types in QVariants.
        typedef QMap<uint, QStringList> MyType;
        MyType my;
        my[1] = (QStringList() << "Hello");
        my[3] = (QStringList() << "World");
        QVariant var;
        var.setValue(my);
        // FIXME: Known to break
        //QString type = var.typeName();
        var.setValue(my);
        BREAK_HERE;
        // Expand my my.0 my.0.value my.1 my.1.value var var.data var.data.0 var.data.0.value var.data.1 var.data.1.value.
        // Check my <2 items> qvariant::MyType.
        // Check my.0   QMapNode<unsigned int, QStringList>.
        // Check my.0.key 1 unsigned int.
        // Check my.0.value <1 items> QStringList.
        // Check my.0.value.0 "Hello" QString.
        // Check my.1   QMapNode<unsigned int, QStringList>.
        // Check my.1.key 3 unsigned int.
        // Check my.1.value <1 items> QStringList.
        // Check my.1.value.0 "World" QString.
        // CheckType var QVariant (QMap<unsigned int , QStringList>).
        // Check var.data <2 items> QMap<unsigned int, QStringList>.
        // Check var.data.0   QMapNode<unsigned int, QStringList>.
        // Check var.data.0.key 1 unsigned int.
        // Check var.data.0.value <1 items> QStringList.
        // Check var.0.value.0 "Hello" QString.
        // Check var.data.1   QMapNode<unsigned int, QStringList>.
        // Check var.data.1.key 3 unsigned int.
        // Check var.data.1.value <1 items> QStringList.
        // Check var.data.1.value.0 "World" QString.
        // Continue.
        var.setValue(my);
        var.setValue(my);
        var.setValue(my);
        dummyStatement(&var);
    }

    void testQVariant6()
    {
        QList<int> list;
        list << 1 << 2 << 3;
        QVariant variant = qVariantFromValue(list);
        BREAK_HERE;
        // Expand list variant variant.data.
        // Check list <3 items> QList<int>.
        // Check list.0 1 int.
        // Check list.1 2 int.
        // Check list.2 3 int.
        // CheckType variant QVariant (QList<int>).
        // Check variant.data <3 items> QList<int>.
        // Check variant.data.0 1 int.
        // Check variant.data.1 2 int.
        // Check variant.data.2 3 int.
        // Continue.
        list.clear();
        list = variant.value<QList<int> >();
        BREAK_HERE;
        // Expand list.
        // Check list <3 items> QList<int>.
        // Check list.0 1 int.
        // Check list.1 2 int.
        // Check list.2 3 int.
        // Continue.
        dummyStatement(&list);
    }

    void testQVariantList()
    {
        QVariantList vl;
        BREAK_HERE;
        // Check vl <0 items> QVariantList.
        // Continue.
        vl.append(QVariant(1));
        vl.append(QVariant(2));
        vl.append(QVariant("Some String"));
        vl.append(QVariant(21));
        vl.append(QVariant(22));
        vl.append(QVariant("2Some String"));
        BREAK_HERE;
        // Expand vl.
        // Check vl <6 items> QVariantList.
        // CheckType vl.0 QVariant (int).
        // CheckType vl.2 QVariant (QString).
        // Continue.
        dummyStatement(&vl);
    }

    void testQVariantMap()
    {
        QVariantMap vm;
        BREAK_HERE;
        // Check vm <0 items> QVariantMap.
        // Continue.
        vm["a"] = QVariant(1);
        vm["b"] = QVariant(2);
        vm["c"] = QVariant("Some String");
        vm["d"] = QVariant(21);
        vm["e"] = QVariant(22);
        vm["f"] = QVariant("2Some String");
        BREAK_HERE;
        // Expand vm vm.0 vm.5.
        // Check vm <6 items> QVariantMap.
        // Check vm.0   QMapNode<QString, QVariant>.
        // Check vm.0.key "a" QString.
        // Check vm.0.value 1 QVariant (int).
        // Check vm.5   QMapNode<QString, QVariant>.
        // Check vm.5.key "f" QString.
        // Check vm.5.value "2Some String" QVariant (QString).
        // Continue.
        dummyStatement(&vm);
    }

    void testQVariant()
    {
        testQVariant1();
        testQVariant2();
        testQVariant3();
        testQVariant4();
        testQVariant5();
        testQVariant6();
        testQVariantList();
        testQVariantMap();
    }

} // namespace qvariant


namespace qvector {

    void testQVectorIntBig()
    {
        // This tests the display of a big vector.
        QVector<int> vec(10000);
        for (int i = 0; i != vec.size(); ++i)
            vec[i] = i * i;
        BREAK_HERE;
        // Expand vec.
        // Check vec <10000 items> QVector<int>.
        // Check vec.0 0 int.
        // Check vec.1999 3996001 int.
        // Continue.

        // step over
        // check that the display updates in reasonable time
        vec[1] = 1;
        vec[2] = 2;
        vec.append(1);
        vec.append(1);
        BREAK_HERE;
        // Expand vec.
        // Check vec <10002 items> QVector<int>.
        // Check vec.1 1 int.
        // Check vec.2 2 int.
        // Continue.
        dummyStatement(&vec);
    }

    void testQVectorFoo()
    {
        // This tests the display of a vector of pointers to custom structs.
        QVector<Foo> vec;
        BREAK_HERE;
        // Check vec <0 items> QVector<Foo>.
        // Continue.
        // step over, check display.
        vec.append(1);
        vec.append(2);
        BREAK_HERE;
        // Expand vec vec.0 vec.1.
        // Check vec <2 items> QVector<Foo>.
        // CheckType vec.0 Foo.
        // Check vec.0.a 1 int.
        // CheckType vec.1 Foo.
        // Check vec.1.a 2 int.
        // Continue.
        dummyStatement(&vec);
    }

    typedef QVector<Foo> FooVector;

    void testQVectorFooTypedef()
    {
        FooVector vec;
        vec.append(Foo(2));
        vec.append(Foo(3));
        dummyStatement(&vec);
    }

    void testQVectorFooStar()
    {
        // This tests the display of a vector of pointers to custom structs.
        QVector<Foo *> vec;
        BREAK_HERE;
        // Check vec <0 items> QVector<Foo*>.
        // Continue.
        // step over
        // check that the display is ok.
        vec.append(new Foo(1));
        vec.append(0);
        vec.append(new Foo(2));
        vec.append(new Fooooo(3));
        // switch "Auto derefencing pointers" in Locals context menu
        // off and on again, and check result looks sane.
        BREAK_HERE;
        // Expand vec vec.0 vec.2.
        // Check vec <4 items> QVector<Foo*>.
        // CheckType vec.0 Foo.
        // Check vec.0.a 1 int.
        // Check vec.1 0x0 Foo *.
        // CheckType vec.2 Foo.
        // Check vec.2.a 2 int.
        // CheckType vec.3 Fooooo.
        // Check vec.3.a 5 int.
        // Continue.
        dummyStatement(&vec);
    }

    void testQVectorBool()
    {
        // This tests the display of a vector of custom structs.
        QVector<bool> vec;
        BREAK_HERE;
        // Check vec <0 items> QVector<bool>.
        // Continue.
        // step over
        // check that the display is ok.
        vec.append(true);
        vec.append(false);
        BREAK_HERE;
        // Expand vec.
        // Check vec <2 items> QVector<bool>.
        // Check vec.0 true bool.
        // Check vec.1 false bool.
        // Continue.
        dummyStatement(&vec);
    }

    void testQVectorListInt()
    {
        QVector<QList<int> > vec;
        QVector<QList<int> > *pv = &vec;
        BREAK_HERE;
        // CheckType pv QVector<QList<int>>.
        // Check vec <0 items> QVector<QList<int>>.
        // Continue.
        vec.append(QList<int>() << 1);
        vec.append(QList<int>() << 2 << 3);
        BREAK_HERE;
        // Expand pv pv.0 pv.1 vec vec.0 vec.1.
        // CheckType pv QVector<QList<int>>.
        // Check pv.0 <1 items> QList<int>.
        // Check pv.0.0 1 int.
        // Check pv.1 <2 items> QList<int>.
        // Check pv.1.0 2 int.
        // Check pv.1.1 3 int.
        // Check vec <2 items> QVector<QList<int>>.
        // Check vec.0 <1 items> QList<int>.
        // Check vec.0.0 1 int.
        // Check vec.1 <2 items> QList<int>.
        // Check vec.1.0 2 int.
        // Check vec.1.1 3 int.
        // Continue.
        dummyStatement(pv, &vec);
    }

    void testQVector()
    {
        testQVectorIntBig();
        testQVectorFoo();
        testQVectorFooTypedef();
        testQVectorFooStar();
        testQVectorBool();
        testQVectorListInt();
    }

} // namespace qvector


namespace noargs {

    class Goo
    {
    public:
       Goo(const QString &str, const int n) : str_(str), n_(n) {}
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

        BREAK_HERE;
        // Expand list list.0 list.1 list2 list2.1.
        // Check i 1 int.
        // Check k 3 int.
        // Check list <2 items> noargs::GooList.
        // CheckType list.0 noargs::Goo.
        // Check list.0.n_ 1 int.
        // Check list.0.str_ "Hello" QString.
        // CheckType list.1 noargs::Goo.
        // Check list.1.n_ 2 int.
        // Check list.1.str_ "World" QString.
        // Check list2 <2 items> QList<noargs::Goo>.
        // CheckType list2.0 noargs::Goo.
        // Check list2.0.n_ 1 int.
        // Check list2.0.str_ "Hello" QString.
        // CheckType list2.1 noargs::Goo.
        // Check list2.1.n_ 2 int.
        // Check list2.1.str_ "World" QString.
        // Continue.
        dummyStatement(&i, &k);
    }

} // namespace noargs


void foo() {}
void foo(int) {}
void foo(QList<int>) {}
void foo(QList<QVector<int> >) {}
void foo(QList<QVector<int> *>) {}
void foo(QList<QVector<int *> *>) {}

template <class T>
void foo(QList<QVector<T> *>) {}


namespace namespc {

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
                // Note there's a local 'n' and one in the base class.
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

    void testNamespace()
    {
        // This checks whether classes with "special" names are
        // properly displayed.
        using namespace nested;
        MyFoo foo;
        MyBar bar;
        MyAnon anon;
        baz::MyBaz baz;
        BREAK_HERE;
        // CheckType anon namespc::nested::(anonymous namespace)::MyAnon.
        // CheckType bar namespc::nested::MyBar.
        // CheckType baz namespc::nested::(anonymous namespace)::baz::MyBaz.
        // CheckType foo namespc::nested::MyFoo.
        // Continue.
        // step into the doit() functions
        baz.doit(1);
        anon.doit(1);
        foo.doit(1);
        bar.doit(1);
        dummyStatement();
    }

} // namespace namespc


namespace gccextensions {

    void testGccExtensions()
    {
#ifdef __GNUC__
        char v[8] = { 1, 2 };
        char w __attribute__ ((vector_size (8))) = { 1, 2 };
        int y[2] = { 1, 2 };
        int z __attribute__ ((vector_size (8))) = { 1, 2 };
        BREAK_HERE;
        // Expand v.
        // Check v.0 1 char.
        // Check v.1 2 char.
        // Check w.0 1 char.
        // Check w.1 2 char.
        // Check y.0 1 int.
        // Check y.1 2 int.
        // Check z.0 1 int.
        // Check z.1 2 int.
        // Continue.
        dummyStatement(&v, &w, &y, &z);
#endif
    }

} // namespace gccextension

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
    dummyStatement(&a);
}

QString fooxx()
{
    return "bababa";
}


namespace basic {

    // This tests display of basic types.

    void testInt()
    {
        quint64 u64 = ULLONG_MAX;
        qint64 s64 = LLONG_MAX;
        quint32 u32 = ULONG_MAX;
        qint32 s32 = LONG_MAX;
        quint64 u64s = 0;
        qint64 s64s = LLONG_MIN;
        quint32 u32s = 0;
        qint32 s32s = LONG_MIN;

        BREAK_HERE;
        // Check u64 18446744073709551615 quint64.
        // Check s64 9223372036854775807 qint64.
        // Check u32 4294967295 quint32.
        // Check s32 2147483647 qint32.
        // Check u64s 0 quint64.
        // Check s64s -9223372036854775808 qint64.
        // Check u32s 0 quint32.
        // Check s32s -2147483648 qint32.
        // Continue.

        dummyStatement(&u64, &s64, &u32, &s32, &u64s, &s64s, &u32s, &s32s);
    }


    void testArray1()
    {
        double d[3][3];
        for (int i = 0; i != 3; ++i)
            for (int j = 0; j != 3; ++j)
                d[i][j] = i + j;
        BREAK_HERE;
        // Expand d d.0.
        // CheckType d double [3][3].
        // CheckType d.0 double [3].
        // Check d.0.0 0 double.
        // Check d.0.2 2 double.
        // CheckType d.2 double [3].
        // Continue.
        dummyStatement(&d);
    }

    void testArray2()
    {
        char c[20];
        c[0] = 'a';
        c[1] = 'b';
        c[2] = 'c';
        c[3] = 'd';
        BREAK_HERE;
        // Expand c.
        // CheckType c char [20].
        // Check c.0 97 'a' char.
        // Check c.3 100 'd' char.
        // Continue.
        dummyStatement(&c);
    }

    void testArray3()
    {
        QString s[20];
        s[0] = "a";
        s[1] = "b";
        s[2] = "c";
        s[3] = "d";
        BREAK_HERE;
        // Expand s.
        // CheckType s QString [20].
        // Check s.0 "a" QString.
        // Check s.3 "d" QString.
        // Check s.4 "" QString.
        // Check s.19 "" QString.
        // Continue.
        dummyStatement(&s);
    }

    void testArray4()
    {
        QByteArray b[20];
        b[0] = "a";
        b[1] = "b";
        b[2] = "c";
        b[3] = "d";
        BREAK_HERE;
        // Expand b.
        // CheckType b QByteArray [20].
        // Check b.0 "a" QByteArray.
        // Check b.3 "d" QByteArray.
        // Check b.4 "" QByteArray.
        // Check b.19 "" QByteArray.
        // Continue.
        dummyStatement(&b);
    }

    void testArray5()
    {
        Foo foo[10];
        //for (int i = 0; i != sizeof(foo)/sizeof(foo[0]); ++i) {
        for (int i = 0; i < 5; ++i) {
            foo[i].a = i;
            foo[i].doit();
        }
        BREAK_HERE;
        // Expand foo.
        // CheckType foo Foo [10].
        // CheckType foo.0 Foo.
        // CheckType foo.9 Foo.
        // Continue.
        dummyStatement(&foo);
    }

    // https://bugreports.qt-project.org/browse/QTCREATORBUG-5326

    void testChar()
    {
        char s[6];
        s[0] = 0;
        BREAK_HERE;
        // Expand s.
        // CheckType s char [6].
        // Check s.0 0 '\0' char.
        // Continue.

        // Manual: Open pinnable tooltip.
        // Manual: Step over.
        // Manual: Check that display and tooltip look sane.
        strcat(s, "\"");
        strcat(s, "\"");
        strcat(s, "a");
        strcat(s, "b");
        strcat(s, "\"");
        // Manual: Close tooltip.
        dummyStatement(&s);
    }

    static char buf[20] = { 0 };

    void testCharStar()
    {
        char *s = buf;
        BREAK_HERE;
        // Expand s.
        // CheckType s char *.
        // Continue.

        // Manual: Open pinnable tooltip.
        // Manual: Step over.
        // Manual: Check that display and tooltip look sane.
        s = strcat(s,"\"");
        s = strcat(s,"\"");
        s = strcat(s,"a");
        s = strcat(s,"b");
        s = strcat(s,"\"");
        // Close tooltip.
        dummyStatement(&s);
    }

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

    void testBitfields()
    {
        // This checks whether bitfields are properly displayed
        S s;
        BREAK_HERE;
        // Expand s.
        // CheckType s basic::S.
        // CheckType s.b bool.
        // CheckType s.c bool.
        // CheckType s.d double.
        // CheckType s.f float.
        // CheckType s.i int.
        // CheckType s.q qreal.
        // CheckType s.x uint.
        // CheckType s.y uint.
        // Continue.
        s.i = 0;
        dummyStatement(&s);
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
        // In order to use this, switch on the 'qDump__Function' in dumper.py
        Function func("x", "sin(x)", 0, 1);
        BREAK_HERE;
        // Expand func.
        // CheckType func basic::Function.
        // Check func.f "sin(x)" QByteArray.
        // Check func.max 1 double.
        // Check func.min 0 double.
        // Check func.var "x" QByteArray.
        // Continue.
        func.max = 10;
        func.f = "cos(x)";
        func.max = 4;
        func.max = 5;
        func.max = 6;
        func.max = 7;
        BREAK_HERE;
        // Expand func.
        // CheckType func basic::Function.
        // Check func.f "cos(x)" QByteArray.
        // Check func.max 7 double.
        // Continue.
        dummyStatement(&func);
    }

    struct Color
    {
        int r,g,b,a;
        Color() { r = 1, g = 2, b = 3, a = 4; }
    };

    void testAlphabeticSorting()
    {
        // This checks whether alphabetic sorting of structure
        // members work.
        Color c;
        BREAK_HERE;
        // Expand c.
        // CheckType c basic::Color.
        // Check c.a 4 int.
        // Check c.b 3 int.
        // Check c.g 2 int.
        // Check c.r 1 int.
        // Continue.

        // Manual: Toogle "Sort Member Alphabetically" in context menu
        // Manual: of "Locals and Expressions" view.
        // Manual: Check that order of displayed members changes.
        dummyStatement(&c);
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
        BREAK_HERE;
        // Check j 1000 basic::ns::vl.
        // Check k 1000 basic::ns::verylong.
        // Check t1 0 basic::myType1.
        // Check t2 0 basic::myType2.
        // Continue.
        dummyStatement(&j, &k, &t1, &t2);
    }

    void testStruct()
    {
        Foo f(2);
        f.doit();
        f.doit();
        f.doit();
        BREAK_HERE;
        // Expand f.
        // CheckType f Foo.
        // Check f.a 5 int.
        // Check f.b 2 int.
        // Continue.
        dummyStatement(&f);
    }

    void testUnion()
    {
        union U
        {
          int a;
          int b;
        } u;
        BREAK_HERE;
        // Expand u.
        // CheckType u basic::U.
        // CheckType u.a int.
        // CheckType u.b int.
        // Continue.
        dummyStatement(&u);
    }

    void testUninitialized()
    {
        // This tests the display of uninitialized data.

        BREAK_UNINITIALIZED_HERE;
        // Check hii <not accessible> QHash<int, int>.
        // Check hss <not accessible> QHash<QString, QString>.
        // Check li <not accessible> QList<int>.
        // CheckType mii <not accessible> QMap<int, int>.
        // Check mss <not accessible> QMap<QString, QString>.
        // Check s <not accessible> QString.
        // Check si <not accessible> QStack<int>.
        // Check sl <not accessible> QStringList.
        // Check sli <not accessible> std::list<int>.
        // CheckType smii std::map<int, int>.
        // CheckType smss std::map<std::string, std::string>.
        // CheckType ss std::string.
        // Check ssi <not accessible> std::stack<int>.
        // Check ssl <not accessible> std::list<std::string>.
        // CheckType svi std::vector<int>.
        // Check vi <not accessible> QVector<int>.
        // Continue.

        // Manual: Note: All values should be <uninitialized> or random data.
        // Manual: Check that nothing bad happens if items with random data
        // Manual: are expanded.
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

        dummyStatement(&s, &sl, &mii, &mss, &hii, &hss, &si, &vi, &li,
                       &ss, &smii, &smss, &sli, &svi, &ssi, &ssl);
    }

    void testTypeFormats()
    {
        // These tests should result in properly displayed umlauts in the
        // Locals and Expressions view. It is only support on gdb with Python.

        const char *s = "aöa";
        const wchar_t *w = L"aöa";
        QString u;
        BREAK_HERE;
        // Expand s.
        // CheckType s char *.
        // Skip Check s.*s 97 'a' char.
        // Check u "" QString.
        // CheckType w wchar_t *.
        // Continue.

        // All: Select UTF-8 in "Change Format for Type" in L&W context menu.
        // Windows: Select UTF-16 in "Change Format for Type" in L&W context menu.
        // Other: Select UCS-6 in "Change Format for Type" in L&W context menu.

        if (sizeof(wchar_t) == 4)
            u = QString::fromUcs4((uint *)w);
        else
            u = QString::fromUtf16((ushort *)w);

        // Make sure to undo "Change Format".
        dummyStatement(s, w);
    }

    typedef void *VoidPtr;
    typedef const void *CVoidPtr;

    struct A
    {
        A() : test(7) {}
        int test;
        void doSomething(CVoidPtr cp) const;
    };

    void A::doSomething(CVoidPtr cp) const
    {
        BREAK_HERE;
        // CheckType cp basic::CVoidPtr.
        // Continue.
        dummyStatement(&cp);
    }

    void testPointer()
    {
        Foo *f = new Foo();
        BREAK_HERE;
        // Expand f.
        // CheckType f Foo.
        // Check f.a 0 int.
        // Check f.b 5 int.
        // Continue.
        dummyStatement(f);
    }

    void testPointerTypedef()
    {
        A a;
        VoidPtr p = &a;
        CVoidPtr cp = &a;
        BREAK_HERE;
        // CheckType a basic::A.
        // CheckType cp basic::CVoidPtr.
        // CheckType p basic::VoidPtr.
        // Continue.
        a.doSomething(cp);
        dummyStatement(&a, &p);
    }

    void testStringWithNewline()
    {
        QString hallo = "hallo\nwelt";
        BREAK_HERE;
        // Check hallo "hallo\nwelt" QString.
        // Continue.

        // Check that string is properly displayed.
        dummyStatement(&hallo);
    }

    void testMemoryView()
    {
        int a[20];
        BREAK_HERE;
        // CheckType a int [20].
        // Continue.

        // Select "Open Memory View" from Locals and Expressions
        //    context menu for item 'a'.
        // Step several times.
        // Check the contents of the memory view updates.
        for (int i = 0; i != 20; ++i)
            a[i] = i;
        dummyStatement(&a);
    }

    void testColoredMemoryView()
    {
        int i = 42;
        double d = 23;
        QString s = "Foo";
        BREAK_HERE;
        // Check d 23 double.
        // Check i 42 int.
        // Check s "Foo" QString.
        // Continue.

        // Select "Open Memory Editor->Open Memory Editor Showing Stack Layout"
        // from Locals and Expressions context menu.
        // Check that the opened memory view contains coloured items
        //    for 'i', 'd', and 's'.
        dummyStatement(&i, &d, &s);
    }

    void testReference1()
    {
        int a = 43;
        const int &b = a;
        typedef int &Ref;
        const int c = 44;
        const Ref d = a;
        BREAK_HERE;
        // Check a 43 int.
        // Check b 43 int &.
        // Check c 44 int.
        // Check d 43 basic::Ref.
        // Continue.
        dummyStatement(&a, &b, &c, &d);
    }

    void testReference2()
    {
        QString a = "hello";
        const QString &b = fooxx();
        typedef QString &Ref;
        const QString c = "world";
        const Ref d = a;
        BREAK_HERE;
        // Check a "hello" QString.
        // Check b "bababa" QString &.
        // Check c "world" QString.
        // Check d "hello" basic::Ref.
        // Continue.
        dummyStatement(&a, &b, &c, &d);
    }

    void testReference3(const QString &a)
    {
        const QString &b = a;
        typedef QString &Ref;
        const Ref d = const_cast<Ref>(a);
        BREAK_HERE;
        // Check a "hello" QString &.
        // Check b "hello" QString &.
        // Check d "hello" basic::Ref.
        // Continue.
        dummyStatement(&a, &b, &d);
    }

    void testDynamicReference()
    {
        DerivedClass d;
        BaseClass *b1 = &d;
        BaseClass &b2 = d;
        BREAK_HERE;
        // CheckType b1 DerivedClass *.
        // CheckType b2 DerivedClass &.
        // Continue.
        int x = b1->foo();
        int y = b2.foo();
        dummyStatement(&d, &b1, &b2, &x, &y);
    }

    void testLongEvaluation1()
    {
        QDateTime time = QDateTime::currentDateTime();
        const int N = 10000;
        QDateTime bigv[N];
        for (int i = 0; i < 10000; ++i) {
            bigv[i] = time;
            time = time.addDays(1);
        }
        BREAK_HERE;
        // Expand bigv.
        // Check N 10000 int.
        // CheckType bigv QDateTime [10000].
        // CheckType bigv.0 QDateTime.
        // CheckType bigv.9999 QDateTime.
        // CheckType time QDateTime.
        // Continue.
        // Note: This is expected to _not_ take up to a minute.
        dummyStatement(&bigv);
    }

    void testLongEvaluation2()
    {
        const int N = 10000;
        int bigv[N];
        for (int i = 0; i < 10000; ++i)
            bigv[i] = i;
        BREAK_HERE;
        // Expand bigv.
        // Check N 10000 int.
        // CheckType bigv int [10000].
        // Check bigv.0 0 int.
        // Check bigv.9999 9999 int.
        // Continue.
        // Note: This is expected to take up to a minute.
        dummyStatement(&bigv);
    }

    void testFork()
    {
        QProcess proc;
        proc.start("/bin/ls");
        proc.waitForFinished();
        QByteArray ba = proc.readAllStandardError();
        ba.append('x');
        BREAK_HERE;
        // Check ba "x" QByteArray.
        // Check proc  QProcess.
        // Continue.

        // Check there is some contents in ba. Error message is expected.
        dummyStatement(&ba);
    }

    int testFunctionPointerHelper(int x) { return x; }

    void testFunctionPointer()
    {
        typedef int (*func_t)(int);
        func_t f = testFunctionPointerHelper;
        int a = f(43);
        BREAK_HERE;
        // CheckType f basic::func_t.
        // Continue.

        // Check there's a valid display for f.
        dummyStatement(&f, &a);
    }

    struct Class
    {
        Class() : a(34) {}
        int testFunctionPointerHelper(int x) { return x; }
        int a;
    };

    void testMemberFunctionPointer()
    {
        Class x;
        typedef int (Class::*func_t)(int);
        func_t f = &Class::testFunctionPointerHelper;
        int a = (x.*f)(43);
        BREAK_HERE;
        // CheckType f basic::func_t.
        // Continue.

        // Check there's a valid display for f.
        dummyStatement(&f, &a);
    }

    void testMemberPointer()
    {
        Class x;
        typedef int (Class::*member_t);
        member_t m = &Class::a;
        int a = x.*m;
        BREAK_HERE;
        // CheckType m basic::member_t.
        // Continue.

        // Check there's a valid display for m.
        dummyStatement(&m, &a);
    }

    void testPassByReferenceHelper(Foo &f)
    {
        BREAK_HERE;
        // Expand f.
        // CheckType f Foo &.
        // Check f.a 12 int.
        // Continue.
        ++f.a;
        BREAK_HERE;
        // Expand f.
        // CheckType f Foo &.
        // Check f.a 13 int.
        // Continue.
    }

    void testPassByReference()
    {
        Foo f(12);
        testPassByReferenceHelper(f);
        dummyStatement(&f);
    }

    void testBigInt()
    {
        qint64 a = Q_INT64_C(0xF020304050607080);
        quint64 b = Q_UINT64_C(0xF020304050607080);
        quint64 c = std::numeric_limits<quint64>::max() - quint64(1);
        BREAK_HERE;
        // Check a -1143861252567568256 qint64.
        // Check b -1143861252567568256 quint64.
        // Check c -2 quint64.
        // Continue.
        dummyStatement(&a, &b, &c);
    }

    void testHidden()
    {
        int  n = 1;
        n = 2;
        BREAK_HERE;
        // Check n 2 int.
        // Continue.
        n = 3;
        BREAK_HERE;
        // Check n 3 int.
        // Continue.
        {
            QString n = "2";
            BREAK_HERE;
            // Check n "2" QString.
            // Check n@1 3 int.
            // Continue.
            n = "3";
            BREAK_HERE;
            // Check n "3" QString.
            // Check n@1 3 int.
            // Continue.
            {
                double n = 3.5;
                BREAK_HERE;
                // Check n 3.5 double.
                // Check n@1 "3" QString.
                // Check n@2 3 int.
                // Continue.
                ++n;
                BREAK_HERE;
                // Check n 4.5 double.
                // Check n@1 "3" QString.
                // Check n@2 3 int.
                // Continue.
                dummyStatement(&n);
            }
            BREAK_HERE;
            // Check n "3" QString.
            // Check n@1 3 int.
            // Continue.
            n = "3";
            n = "4";
            BREAK_HERE;
            // Check n "4" QString.
            // Check n@1 3 int.
            // Continue.
            dummyStatement(&n);
        }
        ++n;
        dummyStatement(&n);
    }

    int testReturnInt()
    {
        return 1;
    }

    bool testReturnBool()
    {
        return true;
    }

    QString testReturnQString()
    {
        return "string";
    }

    void testReturn()
    {
        bool b = testReturnBool();
        BREAK_HERE;
        // Check b true bool.
        // Continue.
        int i = testReturnInt();
        BREAK_HERE;
        // Check i 1 int.
        // Continue.
        QString s = testReturnQString();
        BREAK_HERE;
        // Check s "string" QString.
        // Continue.
        dummyStatement(&i, &b, &s);
    }

    #ifdef Q_COMPILER_RVALUE_REFS
    struct X { X() : a(2), b(3) {} int a, b; };

    X testRValueReferenceHelper1()
    {
        return X();
    }

    X testRValueReferenceHelper2(X &&x)
    {
        return x;
    }

    void testRValueReference()
    {
        X &&x1 = testRValueReferenceHelper1();
        X &&x2 = testRValueReferenceHelper2(std::move(x1));
        X &&x3 = testRValueReferenceHelper2(testRValueReferenceHelper1());

        X y1 = testRValueReferenceHelper1();
        X y2 = testRValueReferenceHelper2(std::move(y1));
        X y3 = testRValueReferenceHelper2(testRValueReferenceHelper1());

        BREAK_HERE;
        // Continue.
        dummyStatement(&x1, &x2, &x3, &y1, &y2, &y3);
    }

    #else

    void testRValueReference() {}

    #endif

    void testBasic()
    {
        testInt();
        testReference1();
        testReference2();
        testReference3("hello");
        testRValueReference();
        testDynamicReference();
        testReturn();
        testArray1();
        testArray2();
        testArray3();
        testArray4();
        testArray5();
        testChar();
        testCharStar();
        testBitfields();
        testFunction();
        testAlphabeticSorting();
        testTypedef();
        testPointer();
        testPointerTypedef();
        testStruct();
        testUnion();
        testUninitialized();
        testTypeFormats();
        testStringWithNewline();
        testMemoryView();
        testColoredMemoryView();
        testLongEvaluation1();
        testLongEvaluation2();
        testFork();
        testFunctionPointer();
        testMemberPointer();
        testMemberFunctionPointer();
        testPassByReference();
        testBigInt();
        testHidden();
    }

} // namespace basic


namespace io {

    void testWCout()
    {
        using namespace std;
        wstring x = L"xxxxx";
        wstring::iterator i = x.begin();
        // Break here.
        // Step.
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
    }

    void testWCout0()
    {
        // Mixing cout and wcout does not work with gcc.
        // See http://gcc.gnu.org/ml/gcc-bugs/2006-05/msg01193.html
        // which also says "you can obtain something close to your
        // expectations by calling std::ios::sync_with_stdio(false);
        // at the beginning of your program."

        using namespace std;
        //std::ios::sync_with_stdio(false);
        // Break here.
        // Step.
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
        // This works only when "Run in terminal" is selected
        // in the Run Configuration.
        int i;
        std::cin >> i;
        int j;
        std::cin >> j;
        std::cout << "Values are " << i << " and " << j << "." << std::endl;
    }

    void testIO()
    {
        testOutput();
        //testInput();
        //testWCout();
        //testWCout0();
    }

} // namespace io


namespace sse {

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
        BREAK_HERE;
        // Expand a b.
        // CheckType sseA __m128.
        // CheckType sseB __m128.
        // Continue.
        dummyStatement(&i, &sseA, &sseB);
    #endif
    }

} // namespace sse


namespace qscript {

    void testQScript()
    {
    #if USE_SCRIPTLIB
        BREAK_UNINITIALIZED_HERE;
        QScriptEngine engine;
        QDateTime date = QDateTime::currentDateTime();
        QScriptValue s;

        BREAK_HERE;
        // Check engine  QScriptEngine.
        // Check s (invalid) QScriptValue.
        // Check x1 <not accessible> QString.
        // Continue.
        s = QScriptValue(33);
        int x = s.toInt32();

        s = QScriptValue(QString("34"));
        QString x1 = s.toString();

        s = engine.newVariant(QVariant(43));
        QVariant v = s.toVariant();

        s = engine.newVariant(QVariant(43.0));
        s = engine.newVariant(QVariant(QString("sss")));
        s = engine.newDate(date);
        date = s.toDateTime();
        s.setProperty("a", QScriptValue());
        QScriptValue d = s.data();
        BREAK_HERE;
        // Check d (invalid) QScriptValue.
        // Check v 43 QVariant (int).
        // Check x 33 int.
        // Check x1 "34" QString.
        // Continue.
        dummyStatement(&x1, &v, &s, &d, &x);
    #else
        dummyStatement();
    #endif
    }

} // namespace script


namespace webkit {

    void testWTFString()
    {
    #if USE_WEBKITLIB
        BREAK_UNINITIALIZED_HERE;
        QWebPage p;
        BREAK_HERE;
        // CheckType p QWebPage.
        // Continue.
        dummyStatement(&p);
    #else
        dummyStatement();
    #endif
    }

    void testWebKit()
    {
        testWTFString();
    }

} // namespace webkit


namespace boost {

    #if USE_BOOST
    void testBoostOptional1()
    {
        boost::optional<int> i;
        BREAK_HERE;
        // Check i <uninitialized> boost::optional<int>.
        // Continue.
        i = 1;
        BREAK_HERE;
        // Check i 1 boost::optional<int>.
        // Continue.
        dummyStatement(&i);
    }

    void testBoostOptional2()
    {
        boost::optional<QStringList> sl;
        BREAK_HERE;
        // Check sl <uninitialized> boost::optional<QStringList>.
        // Continue.
        sl = (QStringList() << "xxx" << "yyy");
        sl.get().append("zzz");
        BREAK_HERE;
        // Check sl <3 items> boost::optional<QStringList>.
        // Continue.
        dummyStatement(&sl);
    }

    void testBoostSharedPtr()
    {
        boost::shared_ptr<int> s;
        boost::shared_ptr<int> i(new int(43));
        boost::shared_ptr<int> j = i;
        boost::shared_ptr<QStringList> sl(new QStringList(QStringList() << "HUH!"));
        BREAK_HERE;
        // Expand sl.
        // Check s (null) boost::shared_ptr<int>.
        // Check i 43 boost::shared_ptr<int>.
        // Check j 43 boost::shared_ptr<int>.
        // Check sl  boost::shared_ptr<QStringList>.
        // Check sl.data <1 items> QStringList
        // Continue.
        dummyStatement(&s, &j, &sl);
    }

    void testBoostGregorianDate()
    {
        using namespace boost;
        using namespace gregorian;
        date d(2005, Nov, 29);
        BREAK_HERE;
        // Check d Tue Nov 29 2005 boost::gregorian::date.
        // Continue

        d += months(1);
        BREAK_HERE;
        // Check d Thu Dec 29 2005 boost::gregorian::date.
        // Continue

        d += months(1);
        BREAK_HERE;
        // Check d Sun Jan 29 2006 boost::gregorian::date.
        // Continue

        // snap-to-end-of-month behavior kicks in:
        d += months(1);
        BREAK_HERE;
        // Check d Tue Feb 28 2006 boost::gregorian::date.
        // Continue.

        // Also end of the month (expected in boost)
        d += months(1);
        BREAK_HERE;
        // Check d Fri Mar 31 2006 boost::gregorian::date.
        // Continue.

        // Not where we started (expected in boost)
        d -= months(4);
        BREAK_HERE;
        // Check d Wed Nov 30 2005 boost::gregorian::date.
        // Continue.

        dummyStatement(&d);
    }

    void testBoostPosixTimeTimeDuration()
    {
        using namespace boost;
        using namespace posix_time;
        time_duration d1(1, 0, 0);
        BREAK_HERE;
        // Check d1 01:00:00 boost::posix_time::time_duration.
        // Continue.
        time_duration d2(0, 1, 0);
        BREAK_HERE;
        // Check d2 00:01:00 boost::posix_time::time_duration.
        // Continue.
        time_duration d3(0, 0, 1);
        BREAK_HERE;
        // Check d3 00:00:01 boost::posix_time::time_duration.
        // Continue.
        dummyStatement(&d1, &d2, &d3);
    }

    void testBoostBimap()
    {
        typedef boost::bimap<int, int> B;
        B b;
        BREAK_HERE;
        // Check b <0 items> boost::B.
        // Continue.
        b.left.insert(B::left_value_type(1, 2));
        BREAK_HERE;
        // Check b <1 items> boost::B.
        // Continue.
        B::left_const_iterator it = b.left.begin();
        int l = it->first;
        int r = it->second;
        // Continue.
        dummyStatement(&b, &l, &r);
    }

    void testBoostPosixTimePtime()
    {
        using namespace boost;
        using namespace gregorian;
        using namespace posix_time;
        ptime p1(date(2002, 1, 10), time_duration(1, 0, 0));
        BREAK_HERE;
        // Check p1 Thu Jan 10 01:00:00 2002 boost::posix_time::ptime.
        // Continue.
        ptime p2(date(2002, 1, 10), time_duration(0, 0, 0));
        BREAK_HERE;
        // Check p2 Thu Jan 10 00:00:00 2002 boost::posix_time::ptime.
        // Continue.
        ptime p3(date(1970, 1, 1), time_duration(0, 0, 0));
        BREAK_HERE;
        // Check p3 Thu Jan 1 00:00:00 1970 boost::posix_time::ptime.
        // Continue.
        dummyStatement(&p1, &p2, &p3);
    }

    void testBoost()
    {
        testBoostOptional1();
        testBoostOptional2();
        testBoostSharedPtr();
        testBoostPosixTimeTimeDuration();
        testBoostPosixTimePtime();
        testBoostGregorianDate();
        testBoostBimap();
    }

    #else

    void testBoost() {}

    #endif

} // namespace boost


namespace mpi {

    struct structdata
    {
        int ints[8];
        char chars[32];
        double doubles[5];
    };

    enum type_t { MPI_LB, MPI_INT, MPI_CHAR, MPI_DOUBLE, MPI_UB };

    struct tree_entry
    {
        tree_entry() {}
        tree_entry(int l, int o, type_t t)
            : blocklength(l), offset(o), type(t)
        {}

        int blocklength;
        int offset;
        type_t type;
    };

    struct tree
    {
        enum kind_t { STRUCT };

        void *base;
        kind_t kind;
        int count;
        tree_entry entries[20];
    };

    void testMPI()
    {
        structdata buffer = {
            //{MPI_LB},
            {0, 1024, 2048, 3072, 4096, 5120, 6144 },
            {"message to 1 of 2: hello"},
            {0, 3.14, 6.2831853071795862, 9.4247779607693793, 13},
            //{MPI_UB}
        };

        tree x;
        x.base = &buffer;
        x.kind = tree::STRUCT;
        x.count = 5;
        x.entries[0] = tree_entry(1, -4, MPI_LB);
        x.entries[1] = tree_entry(5,  0, MPI_INT);
        x.entries[2] = tree_entry(7, 47, MPI_CHAR);
        x.entries[3] = tree_entry(2, 76, MPI_DOUBLE);
        x.entries[4] = tree_entry(1, 100, MPI_UB);


        int i = x.count;
        i = buffer.ints[0];
        i = buffer.ints[1];
        i = buffer.ints[2];
        i = buffer.ints[3];
        /*
                gdb) print datatype
                > $3 = {
                >   kind = STRUCT,
                >   count = 5,
                >   entries = {{
                >       blocklength = 1,
                >       offset = -4,
                >       type = MPI_LB
                >     }, {
                >       blocklength = 5,
                >       offset = 0,
                >       type = MPI_INT
                >     }, {
                >       blocklength = 7,
                >       offset = 47,
                >       type = MPI_CHAR
                >     }, {
                >       blocklength = 2,
                >       offset = 76,
                >       type = MPI_DOUBLE
                >     }, {
                >       blocklength = 1,
                >       offset = 100,
                >       type = MPI_UB
                >     }}
                > }
        */
        dummyStatement(&i);
    }

} // namespace mpi


//namespace kr {

    // FIXME: put in namespace kr, adjust qdump__KRBase in dumpers/qttypes.py
    struct KRBase
    {
        enum Type { TYPE_A, TYPE_B } type;
        KRBase(Type _type) : type(_type) {}
    };

    struct KRA : KRBase { int x; int y; KRA():KRBase(TYPE_A), x(1), y(32) {} };
    struct KRB : KRBase { KRB():KRBase(TYPE_B) {} };

//} // namespace kr


namespace kr {

    // Only with python.
    // This tests qdump__KRBase in dumpers/qttypes.py which uses
    // a static typeflag to dispatch to subclasses.

    void testKR()
    {
        KRBase *ptr1 = new KRA;
        KRBase *ptr2 = new KRB;
        ptr2 = new KRB;
        BREAK_HERE;
        // Expand ptr1 ptr2.
        // CheckType ptr1 KRBase.
        // Check ptr1.type KRBase::TYPE_A (0) KRBase::Type.
        // CheckType ptr2 KRBase.
        // Check ptr2.type KRBase::TYPE_B (1) KRBase::Type.
        // Continue.
        dummyStatement(&ptr1, &ptr2);
    }

} // namspace kr


namespace eigen {

    void testEigen()
    {
    #if USE_EIGEN
        using namespace Eigen;

        Vector3d test = Vector3d::Zero();

        Matrix3d myMatrix = Matrix3d::Constant(5);
        MatrixXd myDynamicMatrix(30, 10);

        myDynamicMatrix(0, 0) = 0;
        myDynamicMatrix(1, 0) = 1;
        myDynamicMatrix(2, 0) = 2;

        Matrix<double, 12, 15, ColMajor> colMajorMatrix;
        Matrix<double, 12, 15, RowMajor> rowMajorMatrix;

        int k = 0;
        for (int i = 0; i != 12; ++i) {
            for (int j = 0; j != 15; ++j) {
                colMajorMatrix(i, j) = k;
                rowMajorMatrix(i, j) = k;
                ++k;
            }
        }

        BREAK_HERE;

        // Continue.
        dummyStatement(&colMajorMatrix, &rowMajorMatrix, &test,
                       &myMatrix, &myDynamicMatrix);
    #endif
    }
}


namespace bug842 {

    void test842()
    {
        // https://bugreports.qt-project.org/browse/QTCREATORBUG-842
        qWarning("Test");
        BREAK_HERE;
        // Continue.
        // Manual: Check that Application Output pane contains string "Test".
        dummyStatement();
    }

} // namespace bug842


namespace bug3611 {

    void test3611()
    {
        // https://bugreports.qt-project.org/browse/QTCREATORBUG-3611
        typedef unsigned char byte;
        byte f = '2';
        int *x = (int*)&f;
        BREAK_HERE;
        // Check f 50 '2' bug3611::byte.
        // Continue.
        // Step.
        f += 1;
        f += 1;
        f += 1;
        BREAK_HERE;
        // Check f 53 '5' bug3611::byte.
        // Continue.
        dummyStatement(&f, &x);
    }

} // namespace bug3611


namespace bug4019 {

    // https://bugreports.qt-project.org/browse/QTCREATORBUG-4019

    class A4019
    {
    public:
        A4019() : test(7) {}
        int test;
        void doSomething() const;
    };

    void A4019::doSomething() const
    {
        std::cout << test << std::endl;
    }

    void test4019()
    {
        A4019 a;
        a.doSomething();
    }

} // namespave bug4019


namespace bug4997 {

    // https://bugreports.qt-project.org/browse/QTCREATORBUG-4997

    void test4997()
    {
        using namespace std;
        // cin.get(); // if commented out, the debugger doesn't stop at the breakpoint
        // in the next line on Windows when "Run in Terminal" is used.^
        dummyStatement();
    }
}


namespace bug4904 {

    // https://bugreports.qt-project.org/browse/QTCREATORBUG-4904

    struct CustomStruct {
        int id;
        double dvalue;
    };

    void test4904()
    {
        QMap<int, CustomStruct> map;
        CustomStruct cs1;
        cs1.id = 1;
        cs1.dvalue = 3.14;
        CustomStruct cs2 = cs1;
        cs2.id = -1;
        map.insert(cs1.id, cs1);
        map.insert(cs2.id, cs2);
        QMap<int, CustomStruct>::iterator it = map.begin();
        BREAK_HERE;
        // Expand map map.0 map.0.value.
        // Check map <2 items> QMap<int, bug4904::CustomStruct>.
        // Check map.0   QMapNode<int, bug4904::CustomStruct>.
        // Check map.0.key -1 int.
        // CheckType map.0.value bug4904::CustomStruct.
        // Check map.0.value.dvalue 3.1400000000000001 double.
        // Check map.0.value.id -1 int.
        // Continue.
        dummyStatement(&it);
    }

} // namespace bug4904


namespace bug5046 {

    // https://bugreports.qt-project.org/browse/QTCREATORBUG-5046

    struct Foo { int a, b, c; };

    void test5046()
    {
        Foo f;
        f.a = 1;
        f.b = 2;
        f.c = 3;
        f.a = 4;
        BREAK_HERE;
        // Expand f.
        // CheckType f bug5046::Foo.
        // Check f.a 4 int.
        // Check f.b 2 int.
        // Check f.c 3 int.
        // Continue.

        // Manual: pop up main editor tooltip over 'f'
        // Manual: verify that the entry is expandable, and expansion works
        dummyStatement(&f);
    }

} // namespace bug5046


namespace bug5106 {

    // https://bugreports.qt-project.org/browse/QTCREATORBUG-5106

    class A5106
    {
    public:
            A5106(int a, int b) : m_a(a), m_b(b) {}
            virtual int test() { return 5; }
    private:
            int m_a, m_b;
    };

    class B5106 : public A5106
    {
    public:
            B5106(int c, int a, int b) : A5106(a, b), m_c(c) {}
            virtual int test() { return 4; BREAK_HERE; }
    private:
            int m_c;
    };

    void test5106()
    {
        B5106 b(1,2,3);
        b.test();
        b.A5106::test();
    }

} // namespace bug5106


namespace bug5184 {

    // https://bugreports.qt-project.org/browse/QTCREATORBUG-5184

    // Note: The report there shows type field "QUrl &" instead of QUrl.
    // It's unclear how this can happen. It should never have been like
    // that with a stock 7.2 and any version of Creator.

    void helper(const QUrl &url)
    {
        QNetworkRequest request(url);
        QList<QByteArray> raw = request.rawHeaderList();
        BREAK_HERE;
        // Check raw <0 items> QList<QByteArray>.
        // CheckType request QNetworkRequest.
        // Check url "http://127.0.0.1/" QUrl &.
        // Continue.
        dummyStatement(&request, &raw);
    }

    void test5184()
    {
        QUrl url(QString("http://127.0.0.1/"));
        helper(url);
    }

} // namespace bug5184


namespace qc42170 {

    // http://www.qtcentre.org/threads/42170-How-to-watch-data-of-actual-type-in-debugger

    struct Object
    {
        Object(int id_) : id(id_) {}
        virtual ~Object() {}
        int id;
    };

    struct Point : Object
    {
        Point(double x_, double y_) : Object(1), x(x_), y(y_) {}
        double x, y;
    };

    struct Circle : Point
    {
        Circle(double x_, double y_, double r_) : Point(x_, y_), r(r_) { id = 2; }
        double r;
    };

    void helper(Object *obj)
    {
        BREAK_HERE;
        // CheckType obj qc42170::Circle.
        // Continue.

        // Check that obj is shown as a 'Circle' object.
        dummyStatement(obj);
    }

    void test42170()
    {
        Circle *circle = new Circle(1.5, -2.5, 3.0);
        Object *obj = circle;
        helper(circle);
        helper(obj);
    }

} // namespace qc42170


namespace bug5799 {

    // https://bugreports.qt-project.org/browse/QTCREATORBUG-5799

    typedef struct { int m1; int m2; } S1;

    struct S2 : S1 { };

    typedef struct S3 { int m1; int m2; } S3;

    struct S4 : S3 { };

    void test5799()
    {
        S2 s2;
        s2.m1 = 5;
        S4 s4;
        s4.m1 = 5;
        S1 a1[10];
        typedef S1 Array[10];
        Array a2;
        BREAK_HERE;
        // Expand s2 s2.@1 s4 s4.@1
        // CheckType a1 bug5799::S1 [10].
        // CheckType a2 bug5799::Array.
        // CheckType s2 bug5799::S2.
        // CheckType s2.@1 bug5799::S1.
        // Check s2.@1.m1 5 int.
        // CheckType s2.@1.m2 int.
        // CheckType s4 bug5799::S4.
        // CheckType s4.@1 bug5799::S3.
        // Check s4.@1.m1 5 int.
        // CheckType s4.@1.m2 int.
        // Continue.
        dummyStatement(&s2, &s4, &a1, &a2);
    }

} // namespace bug5799


namespace bug6813 {

    // https://bugreports.qt-project.org/browse/QTCREATORBUG-6813
    void test6813()
    {
      int foo = 0;
      int *bar = &foo;
      //std::cout << "&foo: " << &foo << "; bar: " << bar << "; &bar: " << &bar;
      dummyStatement(&foo, &bar);
    }

} // namespace bug6813


namespace qc41700 {

    // http://www.qtcentre.org/threads/41700-How-to-watch-STL-containers-iterators-during-debugging

    void test41700()
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
        map_t::const_iterator it = m.begin();
        BREAK_HERE;
        // Expand m m.0 m.0.second m.1 m.1.second.
        // Check m <2 items> qc41700::map_t.
        // Check m.0   std::pair<std::string const, std::list<std::string>>.
        // Check m.0.first "one" std::string.
        // Check m.0.second <3 items> std::list<std::string>.
        // Check m.0.second.0 "a" std::string.
        // Check m.0.second.1 "b" std::string.
        // Check m.0.second.2 "c" std::string.
        // Check m.1   std::pair<std::string const, std::list<std::string>>.
        // Check m.1.first "two" std::string.
        // Check m.1.second <3 items> std::list<std::string>.
        // Check m.1.second.0 "1" std::string.
        // Check m.1.second.1 "2" std::string.
        // Check m.1.second.2 "3" std::string.
        // Continue.
        dummyStatement(&it);
    }

} // namespace qc41700


namespace cp42895 {

    // http://codepaster.europe.nokia.com/?id=42895

    void g(int c, int d)
    {
        qDebug() << c << d;
        BREAK_HERE;
        // Check c 3 int.
        // Check d 4 int.
        // Continue.
        // Check there are frames for g and f in the stack view.
        dummyStatement(&c, &d);
    }

    void f(int a, int b)
    {
        g(a, b);
    }

    void test42895()
    {
        f(3, 4);
    }

} // namespace cp


namespace bug6465 {

    // https://bugreports.qt-project.org/browse/QTCREATORBUG-6465

    void test6465()
    {
        typedef char Foo[20];
        Foo foo = "foo";
        char bar[20] = "baz";
        // BREAK HERE
        dummyStatement(&foo, &bar);
    }

} // namespace bug6465


namespace bug6857 {

    class MyFile : public QFile
    {
    public:
        MyFile(const QString &fileName)
            : QFile(fileName) {}
    };

    void test6857()
    {
        MyFile file("/tmp/tt");
        file.setObjectName("A file");
        BREAK_HERE;
        // Expand file.
        // Expand file.@1.
        // Expand file.@1.@1.
        // Check file  bug6857::MyFile.
        // Check file.@1 "/tmp/tt" QFile.
        // Check file.@1.@1.@1 "A file" QObject.
        // Continue.
        dummyStatement(&file);
    }
}


namespace bug6858 {

    class MyFile : public QFile
    {
    public:
        MyFile(const QString &fileName)
            : QFile(fileName) {}
    };

    void test6858()
    {
        MyFile file("/tmp/tt");
        file.setObjectName("Another file");
        QFile *pfile = &file;
        BREAK_HERE;
        // Expand pfile.
        // Expand pfile.@1.
        // Expand pfile.@1.@1.
        // Check pfile  bug6858::MyFile.
        // Check pfile.@1 "/tmp/tt" QFile.
        // Check pfile.@1.@1.@1 "Another file" QObject.
        // Continue.
        dummyStatement(&file, pfile);
    }
}


namespace bug6863 {

    class MyObject : public QObject
    {
        Q_OBJECT
    public:
        MyObject() {}
    };

    void setProp(QObject *obj)
    {
        obj->setProperty("foo", "bar");
        BREAK_HERE;
        // Expand obj.
        // Check obj.[QObject].properties <2 items>.
        // Continue.
        dummyStatement(&obj);
    }

    void test6863()
    {
        QFile file("/tmp/tt");
        setProp(&file);
        MyObject obj;
        setProp(&obj);
    }

}


namespace bug6933 {

    class Base
    {
    public:
        Base() : a(21) {}
        virtual ~Base() {}
        int a;
    };

    class Derived : public Base
    {
    public:
        Derived() : b(42) {}
        int b;
    };

    void test6933()
    {
        Derived d;
        Base *b = &d;
        BREAK_HERE;
        // Expand b b.bug6933::Base
        // Check b.[bug6933::Base].[vptr]
        // Check b.b 42 int.
        // Continue.
        dummyStatement(&d, b);
    }
}

namespace varargs {

    void test(const char *format, ...)
    {
        va_list arg;
        va_start(arg, format);
        int i = va_arg(arg, int);
        double f = va_arg(arg, double);
        va_end(arg);
        dummyStatement(&i, &f);
    }

    void testVaList()
    {
        test("abc", 1, 2.0);
    }

} // namespace varargs


namespace gdb13393 {

    struct Base {
        Base() : a(1) {}
        virtual ~Base() {}  // Enforce type to have RTTI
        int a;
    };


    struct Derived : public Base {
        Derived() : b(2) {}
        int b;
    };

    struct S
    {
        Base *ptr;
        const Base *ptrConst;
        Base &ref;
        const Base &refConst;

        S(Derived &d)
            : ptr(&d), ptrConst(&d), ref(d), refConst(d)
        {}
    };

    void test13393()
    {
        Derived d;
        S s(d);
        Base *ptr = &d;
        const Base *ptrConst = &d;
        Base &ref = d;
        const Base &refConst = d;
        Base **ptrToPtr = &ptr;
        #if USE_BOOST
        boost::shared_ptr<Base> sharedPtr(new Derived());
        #else
        int sharedPtr = 1;
        #endif
        BREAK_HERE;
        // Expand d ptr ptr.@1 ptrConst ptrToPtr ref refConst s.
        // CheckType d gdb13393::Derived.
        // CheckType d.@1 gdb13393::Base.
        // Check d.b 2 int.
        // CheckType ptr gdb13393::Derived.
        // CheckType ptr.@1 gdb13393::Base.
        // Check ptr.@1.a 1 int.
        // CheckType ptrConst gdb13393::Derived.
        // CheckType ptrConst.@1 gdb13393::Base.
        // Check ptrConst.b 2 int.
        // CheckType ptrToPtr gdb13393::Derived.
        // CheckType ptrToPtr.[vptr] .
        // Check ptrToPtr.@1.a 1 int.
        // CheckType ref gdb13393::Derived.
        // CheckType ref.[vptr] .
        // Check ref.@1.a 1 int.
        // CheckType refConst gdb13393::Derived.
        // CheckType refConst.[vptr] .
        // Check refConst.@1.a 1 int.
        // CheckType s gdb13393::S.
        // CheckType s.ptr gdb13393::Derived.
        // CheckType s.ptrConst gdb13393::Derived.
        // CheckType s.ref gdb13393::Derived.
        // CheckType s.refConst gdb13393::Derived.
        // Check sharedPtr 1 int.
        // Continue.
        dummyStatement(&d, &s, &ptrToPtr, &sharedPtr, &ptrConst, &refConst, &ref);
    }

} // namespace gdb13393


namespace gdb10586 {

    // http://sourceware.org/bugzilla/show_bug.cgi?id=10586. fsf/MI errors out
    // on -var-list-children on an anonymous union. mac/MI was fixed in 2006.
    // The proposed fix has been reported to crash gdb steered from eclipse.
    // http://sourceware.org/ml/gdb-patches/2011-12/msg00420.html
    // Check we are not harmed by either version.
    void testmi()
    {
        struct test {
            struct { int a; float b; };
            struct { int c; float d; };
        } v = {{1, 2}, {3, 4}};
        BREAK_HERE;
        // Expand v.
        // Check v  gdb10586::test.
        // Check v.a 1 int.
        // Continue.
        dummyStatement(&v);
    }

    void testeclipse()
    {
        struct { int x; struct { int a; }; struct { int b; }; } v = {1, {2}, {3}};
        struct s { int x, y; } n = {10, 20};
        BREAK_HERE;
        // Expand v.
        // Expand n.
        // CheckType v {...}.
        // CheckType n gdb10586::s.
        // Check v.a 2 int.
        // Check v.b 3 int.
        // Check v.x 1 int.
        // Check n.x 10 int.
        // Check n.y 20 int.
        // Continue.
        dummyStatement(&v, &n);
    }

    void test10586()
    {
        testmi();
        testeclipse();
    }

} // namespace gdb10586


namespace valgrind {

    void testLeak()
    {
        new int[100]; // Leaks intentionally.
    }

    void testValgrind()
    {
        testLeak();
    }

} // namespace valgrind


namespace tmplate {

    template<typename T>  struct Template
    {
        Template() : t() {}

        // This serves as a manual test that multiple breakpoints work.
        // Each of the two '// BREAK_HERE' below is in a function that
        // is instantiated three times, so both should be reported as
        // breakpoints with three subbreakpoints each.
        template <typename S> void fooS(S s)
        {
            t = s;
            // BREAK_HERE;
        }
        template <int N> void fooN()
        {
            t = N;
            // BREAK_HERE;
        }

        T t;
    };

    void testTemplate()
    {
        Template<double> t;
        t.fooS(1);
        t.fooS(1.);
        t.fooS('a');
        t.fooN<2>();
        t.fooN<3>();
        t.fooN<4>();
        dummyStatement(&t, &t.t);
    }

} // namespace tmplate


namespace sanity {

    // A very quick check.
    void testSanity()
    {
        std::string s;
        s = "hallo";
        s += "hallo";

        QVector<int> qv;
        qv.push_back(2);

        std::vector<int> v;
        v.push_back(2);

        QStringList list;
        list << "aaa" << "bbb" << "cc";

        QList<const char *> list2;
        list2 << "foo";
        list2 << "bar";
        list2 << 0;
        list2 << "baz";
        list2 << 0;

        QObject obj;
        obj.setObjectName("An Object");

        BREAK_HERE;
        // Check list <3 items> QStringList
        // Check list2 <5 items> QList<char const*>
        // Check obj "An Object" QObject
        // Check qv <1 items> QVector<int>
        // Check s "hallohallo" std::string
        // Check v <1 items> std::vector<int>
        // Continue

        dummyStatement(&s, &qv, &v, &list, &list2, &obj);
    }

} // namespace sanity


int main(int argc, char *argv[])
{
    #if USE_GUILIB
    QApplication app(argc, argv);
    #else
    QCoreApplication app(argc, argv);
    #endif

    QChar c(0x1E9E);
    bool b = c.isPrint();
    qDebug() << c << b;

    // Notify Creator about auto run intention.
    if (USE_AUTORUN)
        qWarning("Creator: Switch on magic autorun.");
    else
        qWarning("Creator: Switch off magic autorun.");

    // For a very quick check, step into this one.
    sanity::testSanity();

    // Check for normal dumpers.
    basic::testBasic();
    gccextensions::testGccExtensions();
    qhostaddress::testQHostAddress();
    varargs::testVaList();

    formats::testFormats();
    breakpoints::testBreakpoints();
    peekandpoke::testPeekAndPoke3();
    anon::testAnonymous();

    itemmodel::testItemModel();
    noargs::testNoArgumentName(1, 2, 3);
    text::testText();
    io::testIO();
    catchthrow::testCatchThrow();
    undefined::testUndefined();
    plugin::testPlugin();
    valgrind::testValgrind();
    namespc::testNamespace();
    tmplate::testTemplate();
    painting::testPainting();
    webkit::testWebKit();

    stdarray::testStdArray();
    stdcomplex::testStdComplex();
    stddeque::testStdDeque();
    stdlist::testStdList();
    stdhashset::testStdHashSet();
    stdmap::testStdMap();
    stdset::testStdSet();
    stdstack::testStdStack();
    stdstream::testStdStream();
    stdstring::testStdString();
    stdvector::testStdVector();
    stdptr::testStdPtr();

    qbytearray::testQByteArray();
    qdatetime::testDateTime();
    qdir::testQDir();
    qfileinfo::testQFileInfo();
    qhash::testQHash();
    qlinkedlist::testQLinkedList();
    qlist::testQList();
    qlocale::testQLocale();
    qmap::testQMap();
    qobject::testQObject();
    qrect::testGeometry();
    qregexp::testQRegExp();
    qregion::testQRegion();
    qscript::testQScript();
    qset::testQSet();
    qsharedpointer::testQSharedPointer();
    qstack::testQStack();
    qstringlist::testQStringList();
    qstring::testQString();
    qthread::testQThread();
    qprocess::testQProcess();
    qurl::testQUrl();
    qvariant::testQVariant();
    qvector::testQVector();
    qxml::testQXmlAttributes();

    // Third party data types.
    boost::testBoost();
    eigen::testEigen();
    kr::testKR();
    mpi::testMPI();
    sse::testSSE();

    // The following tests are specific to certain bugs.
    // They need not to be checked during a normal release check.
    cp42895::test42895();
    bug5046::test5046();
    bug4904::test4904();
    qc41700::test41700();
    qc42170::test42170();
    multibp::testMultiBp();
    bug842::test842();
    bug3611::test3611();
    bug4019::test4019();
    bug4997::test4997();
    bug5106::test5106();
    bug5184::test5184();
    bug5799::test5799();
    bug6813::test6813();
    bug6465::test6465();
    bug6857::test6857();
    bug6858::test6858();
    bug6863::test6863();
    bug6933::test6933();
    gdb13393::test13393();
    gdb10586::test10586();

    final::testFinal(&app);

    return 0;
}

#include "simple_test_app.moc"
