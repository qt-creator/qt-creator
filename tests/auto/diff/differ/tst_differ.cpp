/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <QtTest>
#include <QObject>
#include <QList>
#include <QTextDocument>
#include <QTextBlock>
#include <QMetaType>

#include <diffeditor/differ.h>

Q_DECLARE_METATYPE(DiffEditor::Diff)

using namespace DiffEditor;

QT_BEGIN_NAMESPACE
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
QT_END_NAMESPACE


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
    void cleanupSemantics_data();
    void cleanupSemantics();
    void cleanupSemanticsLossless_data();
    void cleanupSemanticsLossless();
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

void tst_Differ::cleanupSemantics_data()
{
    QTest::addColumn<QList<Diff> >("input");
    QTest::addColumn<QList<Diff> >("expected");

    QTest::newRow("Empty")
               << QList<Diff>()
               << QList<Diff>();
    QTest::newRow("Don't cleanup 1")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("AB"))
               << Diff(Diff::Insert, QString("CD"))
               << Diff(Diff::Equal, QString("EF"))
               << Diff(Diff::Delete, QString("G")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("AB"))
               << Diff(Diff::Insert, QString("CD"))
               << Diff(Diff::Equal, QString("EF"))
               << Diff(Diff::Delete, QString("G")));
    QTest::newRow("Don't cleanup 2")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Insert, QString("DEF"))
               << Diff(Diff::Equal, QString("GHIJ"))
               << Diff(Diff::Delete, QString("KLMN")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Insert, QString("DEF"))
               << Diff(Diff::Equal, QString("GHIJ"))
               << Diff(Diff::Delete, QString("KLMN")));
    QTest::newRow("Don't cleanup 3")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Insert, QString("DEF"))
               << Diff(Diff::Equal, QString("GHIJ"))
               << Diff(Diff::Delete, QString("KLMNO"))
               << Diff(Diff::Insert, QString("PQRST")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Insert, QString("DEF"))
               << Diff(Diff::Equal, QString("GHIJ"))
               << Diff(Diff::Delete, QString("KLMNO"))
               << Diff(Diff::Insert, QString("PQRST")));
    QTest::newRow("Don't cleanup 4")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Equal, QString("DE"))
               << Diff(Diff::Delete, QString("F")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Equal, QString("DE"))
               << Diff(Diff::Delete, QString("F")));
    QTest::newRow("Simple cleanup")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("A"))
               << Diff(Diff::Equal, QString("B"))
               << Diff(Diff::Delete, QString("C")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Insert, QString("B")));
    QTest::newRow("Backward cleanup")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("AB"))
               << Diff(Diff::Equal, QString("CD"))
               << Diff(Diff::Delete, QString("E"))
               << Diff(Diff::Equal, QString("F"))
               << Diff(Diff::Insert, QString("G")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABCDEF"))
               << Diff(Diff::Insert, QString("CDFG")));
    QTest::newRow("Multi cleanup")
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("A"))
               << Diff(Diff::Equal, QString("B"))
               << Diff(Diff::Delete, QString("C"))
               << Diff(Diff::Insert, QString("D"))
               << Diff(Diff::Equal, QString("E"))
               << Diff(Diff::Insert, QString("F"))
               << Diff(Diff::Equal, QString("G"))
               << Diff(Diff::Delete, QString("H"))
               << Diff(Diff::Insert, QString("I")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("BCEGH"))
               << Diff(Diff::Insert, QString("ABDEFGI")));
    QTest::newRow("Fraser's example")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("H"))
               << Diff(Diff::Insert, QString("My g"))
               << Diff(Diff::Equal, QString("over"))
               << Diff(Diff::Delete, QString("i"))
               << Diff(Diff::Equal, QString("n"))
               << Diff(Diff::Delete, QString("g"))
               << Diff(Diff::Insert, QString("ment")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("Hovering"))
               << Diff(Diff::Insert, QString("My government")));
    QTest::newRow("Overlap keep without equality")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABCXXX"))
               << Diff(Diff::Insert, QString("XXXDEF")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABCXXX"))
               << Diff(Diff::Insert, QString("XXXDEF")));
    QTest::newRow("Overlap remove equality")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Equal, QString("XXX"))
               << Diff(Diff::Insert, QString("DEF")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABCXXX"))
               << Diff(Diff::Insert, QString("XXXDEF")));
    QTest::newRow("Overlap add equality")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABCXXXX"))
               << Diff(Diff::Insert, QString("XXXXDEF")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Equal, QString("XXXX"))
               << Diff(Diff::Insert, QString("DEF")));
    QTest::newRow("Overlap keep equality")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Equal, QString("XXXX"))
               << Diff(Diff::Insert, QString("DEF")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Equal, QString("XXXX"))
               << Diff(Diff::Insert, QString("DEF")));
    QTest::newRow("Reverse overlap keep without equality")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("XXXABC"))
               << Diff(Diff::Insert, QString("DEFXXX")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("XXXABC"))
               << Diff(Diff::Insert, QString("DEFXXX")));
    QTest::newRow("Reverse overlap remove equality")
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("ABC"))
               << Diff(Diff::Equal, QString("XXX"))
               << Diff(Diff::Delete, QString("DEF")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("XXXDEF"))
               << Diff(Diff::Insert, QString("ABCXXX")));
    QTest::newRow("Reverse overlap add equality")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("XXXXABC"))
               << Diff(Diff::Insert, QString("DEFXXXX")))
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("DEF"))
               << Diff(Diff::Equal, QString("XXXX"))
               << Diff(Diff::Delete, QString("ABC")));
    QTest::newRow("Reverse overlap keep equality")
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("ABC"))
               << Diff(Diff::Equal, QString("XXXX"))
               << Diff(Diff::Delete, QString("DEF")))
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("ABC"))
               << Diff(Diff::Equal, QString("XXXX"))
               << Diff(Diff::Delete, QString("DEF")));
    QTest::newRow("Two overlaps")
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABCDEFG"))
               << Diff(Diff::Insert, QString("DEFGHIJKLM"))
               << Diff(Diff::Equal, QString("NOPQR"))
               << Diff(Diff::Delete, QString("STU"))
               << Diff(Diff::Insert, QString("TUVW")))
               << (QList<Diff>()
               << Diff(Diff::Delete, QString("ABC"))
               << Diff(Diff::Equal, QString("DEFG"))
               << Diff(Diff::Insert, QString("HIJKLM"))
               << Diff(Diff::Equal, QString("NOPQR"))
               << Diff(Diff::Delete, QString("S"))
               << Diff(Diff::Equal, QString("TU"))
               << Diff(Diff::Insert, QString("VW")));
    // All ambiguous tests below should return the same result, but they don't.
    QTest::newRow("Ambiguous 1")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("X"))
               << Diff(Diff::Insert, QString("W  "))
               << Diff(Diff::Equal, QString("  "))
               << Diff(Diff::Delete, QString("Y"))
               << Diff(Diff::Insert, QString("Z"))
               << Diff(Diff::Equal, QString("B")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("X"))
               << Diff(Diff::Insert, QString("W  "))
               << Diff(Diff::Equal, QString("  "))
               << Diff(Diff::Delete, QString("Y"))
               << Diff(Diff::Insert, QString("Z"))
               << Diff(Diff::Equal, QString("B")));
    QTest::newRow("Ambiguous 2")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("X"))
               << Diff(Diff::Insert, QString("W "))
               << Diff(Diff::Equal, QString("  "))
               << Diff(Diff::Delete, QString("Y"))
               << Diff(Diff::Insert, QString(" Z"))
               << Diff(Diff::Equal, QString("B")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("X  Y"))
               << Diff(Diff::Insert, QString("W    Z"))
               << Diff(Diff::Equal, QString("B")));
    QTest::newRow("Ambiguous 3")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("X"))
               << Diff(Diff::Insert, QString("W"))
               << Diff(Diff::Equal, QString("  "))
               << Diff(Diff::Delete, QString("Y"))
               << Diff(Diff::Insert, QString("  Z"))
               << Diff(Diff::Equal, QString("B")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("X"))
               << Diff(Diff::Insert, QString("W"))
               << Diff(Diff::Equal, QString("  "))
               << Diff(Diff::Delete, QString("Y"))
               << Diff(Diff::Insert, QString("  Z"))
               << Diff(Diff::Equal, QString("B")));
}

void tst_Differ::cleanupSemantics()
{
    QFETCH(QList<Diff>, input);
    QFETCH(QList<Diff>, expected);

    Differ differ;
    QList<Diff> result = differ.cleanupSemantics(input);
    QCOMPARE(result, expected);
}

void tst_Differ::cleanupSemanticsLossless_data()
{
    QTest::addColumn<QList<Diff> >("input");
    QTest::addColumn<QList<Diff> >("expected");

    QTest::newRow("Empty")
               << QList<Diff>()
               << QList<Diff>();
    QTest::newRow("Start edge")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Insert, QString("A"))
               << Diff(Diff::Equal, QString("AB")))
               << (QList<Diff>()
               << Diff(Diff::Insert, QString("A"))
               << Diff(Diff::Equal, QString("AAB")));
    QTest::newRow("End edge")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AB"))
               << Diff(Diff::Insert, QString("B"))
               << Diff(Diff::Equal, QString("B")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("ABB"))
               << Diff(Diff::Insert, QString("B")));
    QTest::newRow("Blank lines")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AAA\r\n\r\nBBB"))
               << Diff(Diff::Insert, QString("\r\nCCC\r\n\r\nBBB"))
               << Diff(Diff::Equal, QString("\r\nDDD")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AAA\r\n\r\n"))
               << Diff(Diff::Insert, QString("BBB\r\nCCC\r\n\r\n"))
               << Diff(Diff::Equal, QString("BBB\r\nDDD")));
    QTest::newRow("Line breaks")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AAA\r\nBBB"))
               << Diff(Diff::Insert, QString("CCC\r\nBBB"))
               << Diff(Diff::Equal, QString(" DDD")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AAA\r\n"))
               << Diff(Diff::Insert, QString("BBBCCC\r\n"))
               << Diff(Diff::Equal, QString("BBB DDD")));
    QTest::newRow("The end of sentence")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AAA BBB. AAA "))
               << Diff(Diff::Insert, QString("CCC. AAA "))
               << Diff(Diff::Equal, QString("DDD.")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AAA BBB. "))
               << Diff(Diff::Insert, QString("AAA CCC. "))
               << Diff(Diff::Equal, QString("AAA DDD.")));
    QTest::newRow("Whitespaces")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AAAX BBB"))
               << Diff(Diff::Insert, QString("CCCX BBB"))
               << Diff(Diff::Equal, QString("DDD")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AAAX "))
               << Diff(Diff::Insert, QString("BBBCCCX "))
               << Diff(Diff::Equal, QString("BBBDDD")));
    QTest::newRow("Non-alphanumeric")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AAAX,BBB"))
               << Diff(Diff::Insert, QString("CCCX,BBB"))
               << Diff(Diff::Equal, QString("DDD")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("AAAX,"))
               << Diff(Diff::Insert, QString("BBBCCCX,"))
               << Diff(Diff::Equal, QString("BBBDDD")));
    QTest::newRow("Fraser's example")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("Th"))
               << Diff(Diff::Insert, QString("at c"))
               << Diff(Diff::Equal, QString("at cartoon")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("That "))
               << Diff(Diff::Insert, QString("cat "))
               << Diff(Diff::Equal, QString("cartoon")));
    // All ambiguous tests below should return the same result, but they don't.
    QTest::newRow("Ambiguous 1")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("X"))
               << Diff(Diff::Insert, QString("W  "))
               << Diff(Diff::Equal, QString("  "))
               << Diff(Diff::Delete, QString("Y"))
               << Diff(Diff::Insert, QString("Z"))
               << Diff(Diff::Equal, QString("B")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("X"))
               << Diff(Diff::Insert, QString("W  "))
               << Diff(Diff::Equal, QString("  "))
               << Diff(Diff::Delete, QString("Y"))
               << Diff(Diff::Insert, QString("Z"))
               << Diff(Diff::Equal, QString("B")));
    QTest::newRow("Ambiguous 2")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("X"))
               << Diff(Diff::Insert, QString("W "))
               << Diff(Diff::Equal, QString("  "))
               << Diff(Diff::Delete, QString("Y"))
               << Diff(Diff::Insert, QString(" Z"))
               << Diff(Diff::Equal, QString("B")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("X"))
               << Diff(Diff::Insert, QString("W "))
               << Diff(Diff::Equal, QString("  "))
               << Diff(Diff::Delete, QString("Y"))
               << Diff(Diff::Insert, QString(" Z"))
               << Diff(Diff::Equal, QString("B")));
    QTest::newRow("Ambiguous 3")
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("X"))
               << Diff(Diff::Insert, QString("W"))
               << Diff(Diff::Equal, QString("  "))
               << Diff(Diff::Delete, QString("Y"))
               << Diff(Diff::Insert, QString("  Z"))
               << Diff(Diff::Equal, QString("B")))
               << (QList<Diff>()
               << Diff(Diff::Equal, QString("A"))
               << Diff(Diff::Delete, QString("X"))
               << Diff(Diff::Insert, QString("W"))
               << Diff(Diff::Equal, QString("  "))
               << Diff(Diff::Delete, QString("Y"))
               << Diff(Diff::Insert, QString("  Z"))
               << Diff(Diff::Equal, QString("B")));
}

void tst_Differ::cleanupSemanticsLossless()
{
    QFETCH(QList<Diff>, input);
    QFETCH(QList<Diff>, expected);

    Differ differ;
    QList<Diff> result = differ.cleanupSemanticsLossless(input);
    QCOMPARE(result, expected);
}


QTEST_MAIN(tst_Differ)

#include "tst_differ.moc"


