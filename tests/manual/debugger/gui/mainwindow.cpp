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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#define _USE_MATH_DEFINES
#include <math.h>
#include <QDebug>
#include <QRect>
#include <QRectF>
#include <QLine>
#include <QLineF>
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>

#include <QThread>
#include <string>
#include <set>
#include <QLibrary>
#include <QLibraryInfo>
#include <QFileInfo>
#include <QSharedPointer>
#include <QDir>
#include <QLinkedList>
#include <QStandardItemModel>

struct TestClass {
    TestClass();

    int m_i;
    float m_f;
};

TestClass::TestClass() :
        m_i(1),
        m_f(M_E)
{
}

namespace Foo {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindowClass),
    m_w(-42), m_wc("Hallo"), m_wqs("quallo"),
    m_thread1(0),m_thread2(0),m_fastThread(0)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
    terminateThread();
}

void MainWindow::simpleBP(int inc, const QString &inx)
{    
    int array[2] = {1,2};
    m_w++;
    QString x = QLatin1String("h\344all\366");
    QString *xp = &x;
    qDebug() << inc << inx << *xp;
    Q_UNUSED(array)
}

void MainWindow::on_actionDialog_triggered()
{
    complexBP(0, "ahh");
}

void MainWindow::complexBP(int *inc, QString inx)
{
    m_w++;
    const char *cc = "hallo";

    const char *np = 0;

    char c = 'c';
    unsigned char uc = 'u';
    short s = 5;
    unsigned short us =  56;

    int i = 5;
    int *ip = &i;
    const int * const ipc  = &i;
    unsigned int ui = 56;

    long l = 5;
    unsigned long ul = 55;

    qint64 i64 = 54354;
    quint64 iu64 = 54354;

    float r = M_PI;
    double d = M_PI;

    QString x = "Hallo ";
    x += QDateTime::currentDateTime().toString();

    const QString &xr = x;

    std::string stdString = "SHallo";
    std::list<std::string> stdList;
    stdList.push_back(stdString);

    std::list<int> intList;
    intList.push_back(1);
    intList.push_back(2);

    TestClass tc;
    ++i;
    qDebug() << i;
    ++i;
    qDebug() << i;

    QFileInfo dir(QDir::tempPath());

    QDebug nsp = qDebug().nospace();
    for (int j = 0; j < 80; j++) {
        nsp << 'x';
    }

    QStringList sl;
    sl << "one" << "two" << "three";

    QList<int> qintList;
    qintList << 4 << 6;

    qDebug() << inc << inx << dir.absoluteFilePath();
    //statusBar()->showMessage(x);
    Q_UNUSED(cc)
    Q_UNUSED(np)
    Q_UNUSED(c)
    Q_UNUSED(uc)
    Q_UNUSED(s)
    Q_UNUSED(us)
    Q_UNUSED(ip)
    Q_UNUSED(ipc)
    Q_UNUSED(ui)
    Q_UNUSED(l)
    Q_UNUSED(ul)
    Q_UNUSED(i64)
    Q_UNUSED(iu64)
    Q_UNUSED(r)
    Q_UNUSED(d)
    Q_UNUSED(xr)
}

void MainWindow::on_actionCrash_triggered()
{

    QString h = "Hallo";
    // fsdfd
    QString *s = 0;
    qDebug() << *s;
}

void MainWindow::on_actionSimpleBP_triggered()
{
    simpleBP(42, "Hallo");
}


void MainWindow::on_actionIncr_watch_triggered()
{
    m_w++;
}

class MyThread : public QThread {
public:
    MyThread(int base, QObject *parent) : QThread(parent), m_base(base) {}

    void run();
private:
    int m_base;
};

void MyThread::run()
{
    QString x;
    const int end = m_base + 20;
    for (int i = m_base; i < end; i++) {
        qDebug() << "L" << currentThreadId() << i << '/' << end;
        QThread::msleep(100);
        if (i < 10) {
            x += QString::number(i);
            qDebug() << "lt 10";
            x += ',';
        }
    }
    qDebug() << "Terminating" << currentThreadId();
}

class MyFastThread : public QThread {
public:
    MyFastThread(QObject *parent) : QThread(parent) {}
    void run() { qDebug() << "Done"  << currentThreadId(); }
};


void MainWindow::terminateThread()
{
    if (m_thread1 && m_thread1->isRunning())
        m_thread1->terminate();
    if (m_thread2 && m_thread2->isRunning())
        m_thread2->terminate();
}
void MainWindow::on_actionThread_triggered()
{
    if (!m_fastThread)
        m_fastThread = new MyFastThread(this);
    m_fastThread->start();
    if (!m_thread1)
        m_thread1 = new MyThread(0, this);
    if (!m_thread1->isRunning())
        m_thread1->start();
    if (!m_thread2)
        m_thread2 = new MyThread(20, this);
    if (!m_thread2->isRunning())
        m_thread2->start();
}

void MainWindow::on_actionException_triggered()
{
    try {
        throw (5) ;
    } catch(int e) {
        qDebug() << "caught  "<< e;
    }
}

void MainWindow::on_actionUncaughtException_triggered()
{
        throw QString("eeh!") ;
}

void MainWindow::on_actionDumperBP_triggered()
{
    m_w++;
    std::string stdS;
    std::list<int> sil;
    QList<int> il;
    std::list<std::string> stdStringList;
    std::list<std::wstring> stdWStringList;
    QStringList sl;
    QString s = "hallo";
    for (int c = 'a'; c < 'c'; c++) {
        s += c + 23;
        stdS += c;
        sl.push_back(s);
        stdStringList.push_back(std::string(1, c));
        stdWStringList.push_back(std::wstring(1, c));
        il.push_back(c);
        sil.push_back(c);
        qDebug() << s;
    }
}

void MainWindow::on_actionExtTypes_triggered()
{
    QVariant v1(QLatin1String("hallo"));
    QVariant v2(QStringList(QLatin1String("hallo")));
    QVector<QString> vec;
    vec.push_back("Hallo");
    vec.push_back("Hallo2");
    std::set<std::string> stdSet;
    stdSet.insert("s1");
    QWidget *ww = this;
    QWidget &wwr = *ww;    
    QSharedPointer<QString> sps(new QString("hallo"));
    QList<QSharedPointer<QString> > spsl;
    spsl.push_back(sps);
    QMap<QString,QString> stringmap;
    QMap<int,int> intmap;
    std::map<std::string, std::string> stdstringmap;
    stdstringmap[std::string("A")]  = std::string("B");
    int hidden = 45;

    if (1 == 1 ) {
        int hidden = 7;
        qDebug() << hidden;
    }

    QLinkedList<QString> lls;
    lls << "link1" << "link2";
    QStandardItemModel *model =new QStandardItemModel;
    model->appendRow(new QStandardItem("i1"));

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
    qDebug() << *(spsl.front()) << hidden;
    Q_UNUSED(wwr)
}

void MainWindow::on_actionForeach_triggered()
{
    QStringList sl;
    sl << "1" << "2" << "3";
    foreach(const QString &s, sl)
        qDebug() << s;
    sl.clear();
    qDebug() << sl;
}

void MainWindow::on_actionAssert_triggered()
{
    Q_ASSERT(0);
}
}


void Foo::MainWindow::on_actionScopes_triggered()
{
    int x = 0;
    if (x == 0) {
        int x = 1;
        Q_UNUSED(x)
    } else {
        int x = 2;
        Q_UNUSED(x)
    }
    qDebug() << x;    
}

void Foo::MainWindow::on_actionLongString_triggered()
{
    QString incr = QString::fromLatin1("0123456789").repeated(4);
    QString s = incr;
    for (int i = 0; i < 5; i++) {
        s += incr;
        qDebug() <<s;
    }

}

void Foo::MainWindow::on_actionStdTypes_triggered()
{
    std::string stdString = "s";
    std::wstring stdWString = L"ws";
    std::map<std::string, std::string> stdStringStringMap;
    stdStringStringMap.insert(std::map<std::string, std::string>::value_type(stdString, stdString));
    std::map<std::wstring, std::wstring> stdStringWStringMap;
    stdStringWStringMap.insert(std::map<std::wstring, std::wstring>::value_type(stdWString, stdWString));
    std::set<std::string> stringSet;
    std::list<std::string> stringList;
    std::vector<std::string> stringVector(1, "bla");
    std::vector<std::wstring> wStringVector(1, L"bla");
}

void Foo::MainWindow::on_actionVariousQtTypes_triggered()
{
    const QByteArray ba = "hallo\t";
    QSize size = QSize(42, 43);
    QSizeF sizeF(size);
    QPoint p1 = QPoint(42, 43);
    QPoint p2 = QPoint(100, 100);
    QLine line(p1, p2);
    QPointF p1f(p1);
    QPointF p2f(p2);
    QLineF linef(p1f, p2f);
    QRect rect(p1, p2);
    QRectF rectf(rect);
    qDebug() << sizeF << linef << rectf;

}
