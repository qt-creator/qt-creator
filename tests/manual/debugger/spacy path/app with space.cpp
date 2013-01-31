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

#include <QDebug>
#include <QDir>
#include <QHash>
#include <QLibrary>
#include <QMap>
#include <QPointer>
#include <QString>
#include <QThread>
#include <QVariant>
#include <QVector>

#include <QApplication>
#include <QAction>
#include <QColor>
#include <QFont>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>

#include <QHostAddress>

#include <iostream>

#ifdef Q_OS_WIN
#include <windows.h>
#endif


uint qHash(const QMap<int, int> &)
{
    return 0;
}

uint qHash(const double & f)
{
    return int(f);
}

class  Foo
{
public:
    Foo(int i=0)
        : a(i), b(2)
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

void testArray()
{
    QString x[4];
    x[0] = "a";
    x[1] = "b";
    x[2] = "c";
    x[3] = "d";

    Foo foo[100];
    //for (int i = 0; i != sizeof(foo)/sizeof(foo[0]); ++i) {
    for (int i = 0; i < 5; ++i) {
        foo[i].a = i;
        foo[i].doit();
    }
}

void testByteArray()
{
    QByteArray ba = "Hello";
    ba += '"';
    ba += "World";
    ba += char(0);
    ba += 1;
    ba += 2;
}

void testHash()
{
    QHash<int, float> hgg0;
    hgg0[11] = 11.0;
    hgg0[22] = 22.0;


    QHash<QString, float> hgg1;
    hgg1["22.0"] = 22.0;

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
}

void testImage()
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

void testIO()
{
    qDebug() << "qDebug() 1";
    qDebug() << "qDebug() 2";
    qDebug() << "qDebug() 3";

    std::cout << "std::cout @@ 1\n";
    std::cout << "std::cout @@ 2\n";
    std::cout << "std::cout @@ 3\n";

    std::cerr << "std::cerr 1\n";
    std::cerr << "std::cerr 2\n";
    std::cerr << "std::cerr 3\n";
};


void testList()
{
    QList<int> foo;
    for (int i = 0; i != 100; ++i) {
        foo.append(i);
    }
    foo.append(101);
    foo.append(102);
 }

void testMap()
{
    QMap<int, float> gg0;
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
}

void testObject(int &argc, char *argv[])
{
    QApplication app(argc, argv);
    QAction act("xxx", &app);
    QString t = act.text();
    t += "y";

/*
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
*/
    QLabel l("XXXXXXXXXXXXXXXXX");
    l.show();
    app.exec();
}

void testPixmap()
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
    int (*foo)() = (int(*)()) lib.resolve("pluginTest");
    qDebug() << "library resolve: " << foo;
    if (foo) {
        int res = foo();
        res += 1;
    } else {
        qDebug() << lib.errorString();
    }
}

void stringRefTest(const QString &refstring)
{
    Q_UNUSED(refstring);
}

void testString()
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

void testStringList()
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
};

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

void testThreads()
{
    Thread thread1(1);
    Thread thread2(2);
    thread1.start();
    thread2.start();
    thread1.wait();
    thread2.wait();
}

void testVariant1()
{
    QVariant v;
    v = 1;
    v = 1.0;
    v = "string";
    v = 1;
}

void testVariant2()
{
    QVariant var;
    QVariant var3;
    QHostAddress ha("127.0.0.1");
    qVariantSetValue(var, ha);
    var3 = var;
    var3 = var;
    var3 = var;
    var3 = var;
    QHostAddress ha1 = var.value<QHostAddress>();
}

void testVariant3()
{
    QList<int> list;
    list << 1 << 2 << 3;
    QVariant variant = qVariantFromValue(list);
    list.clear();
    list = qVariantValue<QList<int> >(variant);
}

void testVector()
{
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

void testVectorOfList()
{
    QVector<QList<int> > v;
    QVector<QList<int> > *pv = &v;
    v.append(QList<int>() << 1);
    v.append(QList<int>() << 2 << 3);
    Q_UNUSED(pv);
}

int main(int argc, char *argv[])
{
    testArray();
    testPlugin();
    testList();
    return 0;
    testByteArray();
    testHash();
    testImage();
    testIO();
    testMap();
    testString();
    testStringList();
    testStruct();
    testThreads();
    testVariant1();
    testVariant2();
    testVariant3();
    testVector();
    testVectorOfList();

    testObject(argc, argv);

    QColor color(255,128,10);
    QFont font;

    while(true)
        ;

    return 0;
}

Q_DECLARE_METATYPE(QHostAddress)
Q_DECLARE_METATYPE(QList<int>)


