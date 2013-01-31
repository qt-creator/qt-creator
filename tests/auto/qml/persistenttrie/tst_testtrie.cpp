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

#include "tst_testtrie.h"
#include <qmljs/persistenttrie.h>

#include <QCoreApplication>
#include <QDebug>
#include <QLatin1String>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QTextStream>

using namespace QmlJS::PersistentTrie;

void tst_TestTrie::initTestCase() {
}

const bool VERBOSE=false;

tst_TestTrie::tst_TestTrie() { QObject::QObject(); }

void tst_TestTrie::testListAll_data()
{
    QTest::addColumn<QStringList>("strs");

    QFile f(QString(TESTSRCDIR)+QString("/listAll.data"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream stream(&f);
    int iline = 0;
    while (true) {
        QStringList list;
        QString line;
        while (true) {
            line=stream.readLine();
            if (line.isEmpty())
                break;
            list.append(line);
        }
        QTest::newRow(QString::number(iline++).toLatin1()) << list;
        list.clear();
        if (stream.atEnd())
            break;
    }
}

void tst_TestTrie::testListAll()
{
    QFETCH(QStringList, strs);
    Trie trie;
    foreach (const QString &s, strs)
        trie.insert(s);
    QStringList ref=strs;
    ref.sort();
    ref.removeDuplicates();
    QStringList content=trie.stringList();
    content.sort();
    if (VERBOSE && ref != content) {
        QDebug dbg = qDebug();
        dbg << "ERROR inserting [";
        bool comma = false;
        foreach (const QString &s, strs) {
            if (comma)
                dbg << ",";
            else
                comma = true;
            dbg << s;
        }
        dbg << "] one gets " << trie;
    }
    QCOMPARE(ref, content);
    foreach (const QString &s,strs) {
        if (VERBOSE && ! trie.contains(s)) {
            qDebug() << "ERROR could not find " << s << "in" << trie;
        }
        QVERIFY(trie.contains(s));
    }
}

void tst_TestTrie::testMerge_data()
{
    QTest::addColumn<QStringList>("str1");
    QTest::addColumn<QStringList>("str2");

    QFile f(QString(TESTSRCDIR)+QString("/merge.data"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream stream(&f);
    int iline = 0;
    while (true) {
        QStringList list1;
        QString line;
        while (true) {
            line=stream.readLine();
            if (line.isEmpty())
                break;
            list1.append(line);
        }
        QStringList list2;
        while (true) {
            line=stream.readLine();
            if (line.isEmpty())
                break;
            list2.append(line);
        }
        QTest::newRow(QString::number(iline++).toLatin1()) << list1 << list2;
        list1.clear();
        list2.clear();
        if (stream.atEnd())
            break;
    }
}

void tst_TestTrie::testMerge()
{
    QFETCH(QStringList, str1);
    QFETCH(QStringList, str2);
    Trie trie1;
    foreach (const QString &s, str1)
        trie1.insert(s);
    Trie trie2;
    foreach (const QString &s, str2)
        trie2.insert(s);
    QStringList ref=str1;
    ref.append(str2);
    ref.sort();
    ref.removeDuplicates();
    Trie trie3 = trie1.mergeF(trie2);
    QStringList content=trie3.stringList();
    content.sort();
    if (VERBOSE && ref != content) {
        QDebug dbg=qDebug();
        dbg << "ERROR merging [";
        bool comma = false;
        foreach (const QString &s, str1) {
            if (comma)
                dbg << ",";
            else
                comma = true;
            dbg << s;
        }
        dbg << "] => " << trie1 << " and [";
        comma = false;
        foreach (const QString &s, str2) {
            if (comma)
                dbg << ",";
            else
                comma = true;
            dbg << s;
        }
        dbg << "] => " << trie2
            << " to "  << trie3;
    }
    QCOMPARE(ref, content);
}

void tst_TestTrie::testIntersect_data()
{
    QTest::addColumn<QStringList>("str1");
    QTest::addColumn<QStringList>("str2");

    QFile f(QString(TESTSRCDIR)+QString("/intersect.data"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream stream(&f);
    int iline = 0;
    while (true) {
        QStringList list1;
        QString line;
        while (true) {
            line=stream.readLine();
            if (line.isEmpty())
                break;
            list1.append(line);
        }
        QStringList list2;
        while (true) {
            line=stream.readLine();
            if (line.isEmpty())
                break;
            list2.append(line);
        }
        QTest::newRow(QString::number(iline++).toLatin1()) << list1 << list2;
        list1.clear();
        list2.clear();
        if (stream.atEnd())
            break;
    }
}

void tst_TestTrie::testIntersect()
{
    QFETCH(QStringList, str1);
    QFETCH(QStringList, str2);
    Trie trie1;
    foreach (const QString &s, str1)
        trie1.insert(s);
    Trie trie2;
    foreach (const QString &s, str2)
        trie2.insert(s);
    QSet<QString> ref1;
    foreach (const QString &s, str1)
        ref1.insert(s);
    QSet<QString> ref2;
    foreach (const QString &s, str2)
        ref2.insert(s);
    ref1.intersect(ref2);
    Trie trie3 = trie1.intersectF(trie2);
    ref2.clear();
    foreach (const QString &s, trie3.stringList())
        ref2.insert(s);
    if (VERBOSE && ref1 != ref2) {
        QDebug dbg=qDebug();
        dbg << "ERROR intersecting [";
        bool comma = false;
        foreach (const QString &s, str1) {
            if (comma)
                dbg << ",";
            else
                comma = true;
            dbg << s;
        }
        dbg << "] => " << trie1 << " and [";
        comma = false;
        foreach (const QString &s, str2) {
            if (comma)
                dbg << ",";
            else
                comma = true;
            dbg << s;
        }
        dbg << "] => " << trie2
            << " to "  << trie3;
    }
    QCOMPARE(ref1, ref2);
}

void tst_TestTrie::testCompletion_data()
{
    QTest::addColumn<QStringList>("trieContents");
    QTest::addColumn<QString>("str");
    QTest::addColumn<QStringList>("completions");
    QTest::addColumn<int>("flags");

    QFile f(QString(TESTSRCDIR)+QString("/completion.data"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream stream(&f);
    int iline = 0;
    while (true) {
        QStringList list1;
        QString line;
        while (true) {
            line = stream.readLine();
            if (line.isEmpty())
                break;
            list1.append(line);
        }
        QString str = stream.readLine();
        QStringList list2;
        while (true) {
            line = stream.readLine();
            if (line.isEmpty())
                break;
            list2.append(line);
        }
        int flags = stream.readLine().toInt();
        QTest::newRow(QString::number(iline++).toLatin1()) << list1 << str << list2 << flags;
        list1.clear();
        list2.clear();
        if (stream.atEnd())
            break;
    }
}

void tst_TestTrie::testCompletion() {
    QFETCH(QStringList, trieContents);
    QFETCH(QString, str);
    QFETCH(QStringList, completions);
    QFETCH(int, flags);

    Trie trie;
    foreach (const QString &s, trieContents)
        trie.insert(s);
    QStringList res = trie.complete(str, QString(), LookupFlags(flags));
    res = matchStrengthSort(str, res);
    if (VERBOSE && res != completions) {
        qDebug() << "unexpected completions for " << str
            << " in " << trie;
        qDebug() << "expected :[";
        foreach (const QString &s, completions) {
            qDebug() << s;
        }
        qDebug() << "] got [";
        foreach (const QString &s, res) {
            qDebug() << s;
        }
        qDebug() << "]";
    }
    QCOMPARE(res, completions);
}

void interactiveCompletionTester(){
    Trie trie;
    qDebug() << "interactive completion tester, insert the strings int the trie (empty line to stop)";
    QTextStream stream(stdin);
    QString line;
    while (true) {
        line=stream.readLine();
        if (line.isEmpty())
            break;
        trie.insert(line);
    }
    qDebug() << "testing Complete on " << trie;
    while (true) {
        qDebug() << "insert a string to complete (empty line to stop)";
        line=stream.readLine();
        if (line.isEmpty())
            break;
        QStringList res=trie.complete(line, QString(),
            LookupFlags(CaseInsensitive|SkipChars|SkipSpaces));
        res = matchStrengthSort(line,res);
        qDebug() << "possible completions:[";
        foreach (const QString &s, res) {
            qDebug() << s;
        }
        qDebug() << "]";
    }
}

#ifdef INTERACTIVE_COMPLETION_TEST

int main(int , const char *[])
{
    interactiveCompletionTester();

    return 0;
}

#else

QTEST_MAIN(tst_TestTrie);

//#include "moc_tst_testtrie.cpp"

#endif
