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

#include <QtTest>
#include <QObject>
#include <QList>
#include <QTextDocument>
#include <QTextBlock>
#include <QMetaType>

#include <diffeditor/differ.h>

Q_DECLARE_METATYPE(DiffEditor::Diff);
Q_DECLARE_METATYPE(QList<DiffEditor::Diff>);

using namespace DiffEditor;

namespace QTest {
    template<>
    char *toString(const Diff &diff)
    {
        QByteArray ba = diff.toString().toUtf8();
        return qstrdup(ba.data());
    }
}

namespace QTest {
    template<>
    char *toString(const QList<Diff> &diffList)
    {
        QByteArray ba = "QList(" + QByteArray::number(diffList.count()) + " diffs:";
        for (int i = 0; i < diffList.count(); i++) {
            if (i > 0)
                ba += ",";
            ba += " " + QByteArray::number(i + 1) + ". " + diffList.at(i).toString().toUtf8();
        }
        ba += ")";
        return qstrdup(ba.data());
    }
}


class tst_Differ: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preprocess_data();
    void preprocess();
    void myers_data();
    void myers();
    void merge_data();
    void merge();
};


void tst_Differ::preprocess_data()
{
    QTest::addColumn<QString>("text1");
    QTest::addColumn<QString>("text2");
    QTest::addColumn<QList<Diff> >("expected");

    QTest::newRow("Null texts") << QString() << QString() << QList<Diff>();
    QTest::newRow("Empty texts") << QString("") << QString("") << QList<Diff>();
    QTest::newRow("Null and empty text") << QString() << QString("") << QList<Diff>();
    QTest::newRow("Empty and null text") << QString("") << QString() << QList<Diff>();
    QTest::newRow("Simple delete") << QString("A") << QString() << (QList<Diff>() << Diff(Diff::Delete, QString("A")));
    QTest::newRow("Simple insert") << QString() << QString("A") << (QList<Diff>() << Diff(Diff::Insert, QString("A")));
    QTest::newRow("Simple equal") << QString("A") << QString("A") << (QList<Diff>()
               << Diff(Diff::Equal, QString("A")));
    QTest::newRow("Common prefix 1") << QString("ABCD") << QString("AB") << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Delete, QString("CD")));
    QTest::newRow("Common prefix 2") << QString("AB") << QString("ABCD") << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Insert, QString("CD")));
    QTest::newRow("Common suffix 1") << QString("ABCD") << QString("CD") << (QList<Diff>()
               << Diff(Diff::Delete, QString("AB"))
               << Diff(Diff::Equal, QString("CD")));
    QTest::newRow("Common suffix 2") << QString("CD") << QString("ABCD") << (QList<Diff>()
               << Diff(Diff::Insert, QString("AB"))
               << Diff(Diff::Equal, QString("CD")));
    QTest::newRow("Common prefix and suffix 1") << QString("ABCDEF") << QString("ABEF") << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Delete, QString("CD"))
               << Diff(Diff::Equal, QString("EF")));
    QTest::newRow("Common prefix and suffix 2") << QString("ABEF") << QString("ABCDEF") << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Insert, QString("CD"))
               << Diff(Diff::Equal, QString("EF")));
    QTest::newRow("Two edits 1") << QString("ABCDEF") << QString("ACDF") << (QList<Diff>()
               << Diff(Diff::Equal, QString("A")) // prefix
               << Diff(Diff::Delete, QString("B"))
               << Diff(Diff::Equal, QString("CD")) // CD inside BCDE
               << Diff(Diff::Delete, QString("E"))
               << Diff(Diff::Equal, QString("F"))); // suffix
    QTest::newRow("Two edits 2") << QString("ACDF") << QString("ABCDEF") << (QList<Diff>()
               << Diff(Diff::Equal, QString("A")) // prefix
               << Diff(Diff::Insert, QString("B"))
               << Diff(Diff::Equal, QString("CD")) // CD inside BCDE
               << Diff(Diff::Insert, QString("E"))
               << Diff(Diff::Equal, QString("F"))); // suffix
    QTest::newRow("Single char not in the other text 1") << QString("ABCDEF") << QString("AXF") << (QList<Diff>()
               << Diff(Diff::Equal, QString("A")) // prefix
               << Diff(Diff::Delete, QString("BCDE"))
               << Diff(Diff::Insert, QString("X")) // single char not in the other text
               << Diff(Diff::Equal, QString("F"))); // suffix
    QTest::newRow("Single char not in the other text 2") << QString("AXF") << QString("ABCDEF") << (QList<Diff>()
               << Diff(Diff::Equal, QString("A")) // prefix
               << Diff(Diff::Delete, QString("X")) // single char not in the other text
               << Diff(Diff::Insert, QString("BCDE"))
               << Diff(Diff::Equal, QString("F"))); // suffix
}

void tst_Differ::preprocess()
{
    QFETCH(QString, text1);
    QFETCH(QString, text2);
    QFETCH(QList<Diff>, expected);

    Differ differ;
    QList<Diff> result = differ.diff(text1, text2);
    QCOMPARE(result, expected);
}

void tst_Differ::myers_data()
{
    QTest::addColumn<QString>("text1");
    QTest::addColumn<QString>("text2");
    QTest::addColumn<QList<Diff> >("expected");

    QTest::newRow("Myers 1") << QString("XAXCXABC") << QString("ABCY") << (QList<Diff>()
               << Diff(Diff::Delete, QString("XAXCX"))
               << Diff(Diff::Equal, QString("ABC"))
               << Diff(Diff::Insert, QString("Y")));
}

void tst_Differ::myers()
{
    QFETCH(QString, text1);
    QFETCH(QString, text2);
    QFETCH(QList<Diff>, expected);

    Differ differ;
    QList<Diff> result = differ.diff(text1, text2);
    QCOMPARE(result, expected);
}

void tst_Differ::merge_data()
{
    QTest::addColumn<QList<Diff> >("input");
    QTest::addColumn<QList<Diff> >("expected");

    QTest::newRow("Remove null insert")
               << (QList<Diff>()
               << Diff(Diff::Insert, QString()))
               << QList<Diff>();
    QTest::newRow("Remove null delete")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString()))
               << QList<Diff>();
    QTest::newRow("Remove null equal")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString()))
               << QList<Diff>();
    QTest::newRow("Remove empty insert")
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("")))
               << QList<Diff>();
    QTest::newRow("Remove empty delete")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("")))
               << QList<Diff>();
    QTest::newRow("Remove empty equal")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("")))
               << QList<Diff>();
    QTest::newRow("Remove empty inserts")
               << (QList<Diff>()
               << Diff(Diff::Insert, QString(""))
               << Diff(Diff::Insert, QString()))
               << QList<Diff>();
    QTest::newRow("Remove empty deletes")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString(""))
               << Diff(Diff::Delete, QString()))
               << QList<Diff>();
    QTest::newRow("Remove empty equals")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString(""))
               << Diff(Diff::Equal, QString()))
               << QList<Diff>();
    QTest::newRow("Remove empty inserts / deletes / equals")
               << (QList<Diff>()
               << Diff(Diff::Insert, QString(""))
               << Diff(Diff::Delete, QString(""))
               << Diff(Diff::Equal, QString(""))
               << Diff(Diff::Insert, QString())
               << Diff(Diff::Delete, QString())
               << Diff(Diff::Equal, QString()))
               << QList<Diff>();
    QTest::newRow("Two equals")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Equal, QString("CD")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("ABCD")));
    QTest::newRow("Two deletes")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("AB"))
               << Diff(Diff::Delete, QString("CD")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABCD")));
    QTest::newRow("Two inserts")
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("AB"))
               << Diff(Diff::Insert, QString("CD")))
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("ABCD")));
    QTest::newRow("Change order of insert / delete")
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("AB"))
               << Diff(Diff::Delete, QString("CD")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("CD"))
               << Diff(Diff::Insert, QString("AB")));
    QTest::newRow("Squash into equal 1")
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("AB"))
               << Diff(Diff::Delete, QString("AB")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB")));
    QTest::newRow("Squash into equal 2")
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("A"))
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Insert, QString("B"))
               << Diff(Diff::Delete, QString("D"))
               << Diff(Diff::Insert, QString("CD")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("ABCD")));
    QTest::newRow("Prefix and suffix detection 1")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("A"))
               << Diff(Diff::Insert, QString("ABC"))
               << Diff(Diff::Delete, QString("DC")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("D"))
               << Diff(Diff::Insert, QString("B"))
               << Diff(Diff::Equal, QString("C")));
    QTest::newRow("Prefix and suffix detection 2")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("X"))
               << Diff(Diff::Delete, QString("A"))
               << Diff(Diff::Insert, QString("ABC"))
               << Diff(Diff::Delete, QString("DC"))
               << Diff(Diff::Equal, QString("Y")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("XA"))
               << Diff(Diff::Delete, QString("D"))
               << Diff(Diff::Insert, QString("B"))
               << Diff(Diff::Equal, QString("CY")));
    QTest::newRow("Merge inserts")
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("AB"))
               << Diff(Diff::Delete, QString("CD"))
               << Diff(Diff::Insert, QString("EF")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("CD"))
               << Diff(Diff::Insert, QString("ABEF")));
    QTest::newRow("Merge deletes")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("AB"))
               << Diff(Diff::Insert, QString("CD"))
               << Diff(Diff::Delete, QString("EF")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABEF"))
               << Diff(Diff::Insert, QString("CD")));
    QTest::newRow("Merge many")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Equal, QString("B"))
               << Diff(Diff::Insert, QString("CD"))
               << Diff(Diff::Delete, QString("EF"))
               << Diff(Diff::Insert, QString("GH"))
               << Diff(Diff::Delete, QString("IJ"))
               << Diff(Diff::Equal, QString("K"))
               << Diff(Diff::Equal, QString("L")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Delete, QString("EFIJ"))
               << Diff(Diff::Insert, QString("CDGH"))
               << Diff(Diff::Equal, QString("KL")));
    QTest::newRow("Don't merge")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("AB"))
               << Diff(Diff::Insert, QString("CD"))
               << Diff(Diff::Equal, QString("EF"))
               << Diff(Diff::Delete, QString("GH"))
               << Diff(Diff::Insert, QString("IJ")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("AB"))
               << Diff(Diff::Insert, QString("CD"))
               << Diff(Diff::Equal, QString("EF"))
               << Diff(Diff::Delete, QString("GH"))
               << Diff(Diff::Insert, QString("IJ")));
    QTest::newRow("Squash equalities surrounding insert sliding edit left")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Insert, QString("CDAB"))
               << Diff(Diff::Equal, QString("EF")))
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("ABCD"))
               << Diff(Diff::Equal, QString("ABEF")));
    QTest::newRow("Squash equalities surrounding insert sliding edit right")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Insert, QString("CDEF"))
               << Diff(Diff::Equal, QString("CD")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("ABCD"))
               << Diff(Diff::Insert, QString("EFCD")));
    QTest::newRow("Squash equalities surrounding delete sliding edit left")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Delete, QString("CDAB"))
               << Diff(Diff::Equal, QString("EF")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABCD"))
               << Diff(Diff::Equal, QString("ABEF")));
    QTest::newRow("Squash equalities surrounding delete sliding edit right")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Delete, QString("CDEF"))
               << Diff(Diff::Equal, QString("CD")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("ABCD"))
               << Diff(Diff::Delete, QString("EFCD")));
    QTest::newRow("Squash equalities complex 1")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Insert, QString("CDGH"))
               << Diff(Diff::Equal, QString("CD"))
               << Diff(Diff::Insert, QString("EFIJ"))
               << Diff(Diff::Equal, QString("EF")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("ABCD"))
               << Diff(Diff::Insert, QString("GHCDEFIJ"))
               << Diff(Diff::Equal, QString("EF")));
    QTest::newRow("Squash equalities complex 2")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Insert, QString("CDGH"))
               << Diff(Diff::Equal, QString("CD"))
               << Diff(Diff::Delete, QString("EFIJ"))
               << Diff(Diff::Equal, QString("EF")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("ABCD"))
               << Diff(Diff::Delete, QString("EFIJ"))
               << Diff(Diff::Insert, QString("GHCD"))
               << Diff(Diff::Equal, QString("EF")));
    QTest::newRow("Squash equalities complex 3")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Insert, QString("CDEF"))
               << Diff(Diff::Equal, QString("CD"))
               << Diff(Diff::Insert, QString("GH")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("ABCD"))
               << Diff(Diff::Insert, QString("EFCDGH")));
    QTest::newRow("Squash equalities complex 4")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Insert, QString("CDEF"))
               << Diff(Diff::Equal, QString("CD"))
               << Diff(Diff::Insert, QString("GH"))
               << Diff(Diff::Equal, QString("EF"))
               << Diff(Diff::Insert, QString("IJ"))
               << Diff(Diff::Equal, QString("CD")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("ABCDEFCD"))
               << Diff(Diff::Insert, QString("GHEFIJCD")));
}

void tst_Differ::merge()
{
    QFETCH(QList<Diff>, input);
    QFETCH(QList<Diff>, expected);

    Differ differ;
    QList<Diff> result = differ.merge(input);
    QCOMPARE(result, expected);
}



QTEST_MAIN(tst_Differ)

#include "tst_differ.moc"


