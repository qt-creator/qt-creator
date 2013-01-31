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

#include <qmljs/qmljssimplereader.h>

#include <QtTest>
#include <algorithm>

using namespace QmlJS;

class tst_SimpleReader : public QObject
{
    Q_OBJECT
public:
    tst_SimpleReader();

private slots:
    void testWellFormed();
    void testIllFormed01();
    void testIllFormed02();
    void testArrays();
    void testBug01();
};

tst_SimpleReader::tst_SimpleReader()
{
}

void tst_SimpleReader::testWellFormed()
{
    char source[] = "RootNode {\n"
                    "   ChildNode {\n"
                    "       property01: 10\n"
                    "   }\n"
                    "   ChildNode {\n"
                    "       propertyString: \"str\"\n"
                    "       InnerChild {\n"
                    "           test: \"test\"\n"
                    "       }\n"
                    "   }\n"
                    "   propertyBlah: false\n"
                    "}\n";

    QmlJS::SimpleReaderNode::WeakPtr weak01;
    QmlJS::SimpleReaderNode::WeakPtr weak02;
    {
        QmlJS::SimpleReader reader;
        QmlJS::SimpleReaderNode::Ptr rootNode = reader.readFromSource(source);
        QVERIFY(reader.errors().isEmpty());
        QVERIFY(rootNode);
        QVERIFY(rootNode->isValid());
        QCOMPARE(rootNode->name(), QLatin1String("RootNode"));

        QCOMPARE(rootNode->children().count(), 2);
        QCOMPARE(rootNode->properties().count(), 1);

        QVERIFY(rootNode->properties().contains("propertyBlah"));
        QCOMPARE(rootNode->property("property01").toBool(), false);

        QVERIFY(rootNode->children().first()->isValid());
        QVERIFY(!rootNode->children().first()->isRoot());

        QVERIFY(rootNode->children().first()->properties().contains("property01"));
        QCOMPARE(rootNode->children().first()->property("property01").toInt(), 10);

        QmlJS::SimpleReaderNode::Ptr secondChild = rootNode->children().at(1);

        QVERIFY(secondChild);
        QVERIFY(secondChild->isValid());
        QVERIFY(!secondChild->isRoot());
        QCOMPARE(secondChild->name(), QLatin1String("ChildNode"));

        QVERIFY(secondChild->properties().contains("propertyString"));
        QCOMPARE(secondChild->property("propertyString").toString(), QLatin1String("str"));

        QCOMPARE(secondChild->children().count(), 1);

        QmlJS::SimpleReaderNode::Ptr innerChild = secondChild->children().first();

        QVERIFY(innerChild);
        QVERIFY(innerChild->isValid());
        QVERIFY(!innerChild->isRoot());
        QCOMPARE(innerChild->name(), QLatin1String("InnerChild"));

        QVERIFY(innerChild->properties().contains("test"));
        QCOMPARE(innerChild->property("test").toString(), QLatin1String("test"));

        weak01 = rootNode;
        weak02 = secondChild;
    }

    QVERIFY(!weak01);
    QVERIFY(!weak02);
}

void tst_SimpleReader::testIllFormed01()
{
    char source[] = "RootNode {\n"
                    "   ChildNode {\n"
                    "       property01: 10\n"
                    "   }\n"
                    "   ChildNode {\n"
                    "       propertyString: \"str\"\n"
                    "       InnerChild \n" //missing {
                    "           test: \"test\"\n"
                    "       }\n"
                    "   }\n"
                    "   propertyBlah: false\n"
                    "}\n";
    QmlJS::SimpleReader reader;
    QmlJS::SimpleReaderNode::Ptr rootNode = reader.readFromSource(source);

    QVERIFY(!rootNode);
    QVERIFY(!reader.errors().empty());
}

void tst_SimpleReader::testIllFormed02()
{
        char source[] = "RootNode {\n"
                    "   ChildNode {\n"
                    "       property01: 10\n"
                    "       property01: 20\n"
                    "   }\n"
                    "   ChildNode {\n"
                    "       propertyString: \"str\"\n"
                    "       InnerChild {\n"
                    "           test: \"test\"\n"
                    "           test: \"test2\"\n"
                    "       }\n"
                    "   }\n"
                    "}\n";

        QmlJS::SimpleReader reader;
        QmlJS::SimpleReaderNode::Ptr rootNode = reader.readFromSource(source);

        QVERIFY(rootNode);
        QVERIFY(rootNode->isValid());
        QVERIFY(rootNode->isRoot());

        QVERIFY(!reader.errors().empty());
        QCOMPARE(reader.errors().count(), 2);

        QmlJS::SimpleReaderNode::Ptr firstChild = rootNode->children().at(0);

        QVERIFY(firstChild);
        QVERIFY(firstChild->isValid());
        QVERIFY(!firstChild->isRoot());

        QCOMPARE(firstChild->properties().count(), 1);
        QVERIFY(firstChild->properties().contains("property01"));
        QCOMPARE(firstChild->property("property01").toString(), QLatin1String("20"));
}

void tst_SimpleReader::testArrays()
{
    char source[] = "RootNode {\n"
                    "   propertyArray: [\"string01\", \"string02\" ]\n"
                    "   ChildNode {\n"
                    "       propertyArray: [\"string01\", \"string02\" ]\n"
                    "       propertyArrayMixed: [\"string03\", [\"string01\", \"string02\"] ]\n"
                    "   }\n"
                    "}\n";

        QList<QVariant> variantList;
        variantList << QVariant(QLatin1String("string01")) << QVariant(QLatin1String("string02"));
        const QVariant variant = variantList;

        QmlJS::SimpleReader reader;
        QmlJS::SimpleReaderNode::Ptr rootNode = reader.readFromSource(source);

        QVERIFY(rootNode);
        QVERIFY(rootNode->isValid());
        QVERIFY(rootNode->isRoot());

        QCOMPARE(rootNode->property("propertyArray"), variant);


        QmlJS::SimpleReaderNode::Ptr firstChild = rootNode->children().at(0);

        QVERIFY(firstChild);
        QVERIFY(firstChild->isValid());
        QVERIFY(!firstChild->isRoot());
        QCOMPARE(firstChild->property("propertyArray"), variant);

        QList<QVariant> variantList2;
        variantList2 << QVariant(QLatin1String("string03")) << variant;
        const QVariant variant2 = variantList2;

        QCOMPARE(firstChild->property("propertyArrayMixed"), variant2);
}

void tst_SimpleReader::testBug01()
{
    char source[] = "\n"
        "AutoTypes {\n"
        "    imports: [ \"import HelperWidgets 1.0\", \"import QtQuick 1.0\", \"import Bauhaus 1.0\" ]\n"
        "    Type {\n"
        "        typeNames: [\"int\"]\n"
        "        sourceFile: \"IntEditorTemplate.qml\"\n"
        "    }\n"
        "    Type {\n"
        "        typeNames: [\"real\", \"double\", \"qreal\"]\n"
        "        sourceFile: \"RealEditorTemplate.qml\"\n"
        "    }\n"
        "    Type {\n"
        "        typeNames: [\"string\", \"QString\", \"QUrl\", \"url\"]\n"
        "        sourceFile: \"StringEditorTemplate.qml\"\n"
        "    }\n"
        "    Type {\n"
        "        typeNames: [\"bool\", \"boolean\"]\n"
        "        sourceFile: \"BooleanEditorTemplate.qml\"\n"
        "    }\n"
        "    Type {\n"
        "        typeNames: [\"color\", \"QColor\"]\n"
        "        sourceFile: \"ColorEditorTemplate.qml\"\n"
        "    }\n"
        "}\n";

    QmlJS::SimpleReader reader;
    QmlJS::SimpleReaderNode::Ptr rootNode = reader.readFromSource(source);

    QVERIFY(rootNode);
    QVERIFY(rootNode->isValid());
    QVERIFY(rootNode->isRoot());

    QCOMPARE(rootNode->propertyNames().count(), 1);
}

QTEST_MAIN(tst_SimpleReader);

#include "tst_qmljssimplereader.moc"
