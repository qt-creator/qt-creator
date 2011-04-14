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


#include "metainfo.h"
#include <QScopedPointer>
#include <QLatin1String>
#include <QGraphicsObject>
#include <QApplication>
#include <QSettings>
#include <QFileInfo>

#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljslookupcontext.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljstools/qmljsrefactoringchanges.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <QtTest>

using namespace QmlJS;
using namespace QmlJSTools;

class tst_Basic : public QObject
{
    Q_OBJECT
public:
    tst_Basic();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void basicObjectTests();
    void testMetaInfo();
};


#ifdef Q_OS_MAC
#  define SHARE_PATH "/Resources"
#else
#  define SHARE_PATH "/share/qtcreator"
#endif


QString resourcePath()
{
    return QDir::cleanPath(QTCREATORDIR + QLatin1String(SHARE_PATH));
}

class TestModelManager : public Internal::ModelManager
{
public:
    TestModelManager() : Internal::ModelManager()
    {
        loadQmlTypeDescriptions(resourcePath());
    }
    void loadFile(QString fileName)
    {
        refreshSourceFiles(QStringList() << fileName, false).waitForFinished();
    }
};

tst_Basic::tst_Basic()
{
}

void tst_Basic::initTestCase()
{
}

void tst_Basic::cleanupTestCase()
{
}

void tst_Basic::basicObjectTests()
{
    TestModelManager modelManager;

    modelManager.loadFile(QString(QTCREATORDIR) + "/tests/auto/qml/qmldesigner/data/fx/usingmybutton.qml");

    Snapshot snapshot = modelManager.snapshot();
    Document::Ptr doc = snapshot.document(QString(QTCREATORDIR) + "/tests/auto/qml/qmldesigner/data/fx/usingmybutton.qml");
    QVERIFY(doc && doc->isQmlDocument());

    LookupContext::Ptr lookupContext = LookupContext::create(doc, snapshot, QList<AST::Node*>());
    QVERIFY(lookupContext);

    QVERIFY(lookupContext->engine()->cppQmlTypes().types().value("QtQuick.Rectangle 1.0"));
    QVERIFY(!lookupContext->engine()->cppQmlTypes().types().value("QtQuick.Rectangle 1.0")->isWritable("border"));
    QVERIFY(lookupContext->engine()->cppQmlTypes().types().value("QtQuick.Rectangle 1.0")->hasProperty("border"));
    QVERIFY(lookupContext->engine()->cppQmlTypes().types().value("QtQuick.Rectangle 1.0")->isPointer("border"));
    QVERIFY(lookupContext->engine()->cppQmlTypes().types().value("QtQuick.Rectangle 1.0")->isWritable("color"));
    QVERIFY(!lookupContext->engine()->cppQmlTypes().types().value("QtQuick.Rectangle 1.0")->isPointer("color"));

    const Interpreter::ObjectValue *ovItem = lookupContext->context()->lookupType(doc.data(), QStringList() << "Item");
    QCOMPARE(ovItem->className(), QString("Item"));
    QCOMPARE(lookupContext->context()->scopeChain().qmlTypes->importInfo("Item", lookupContext->context()).name(), QString("Qt"));
    const Interpreter::ObjectValue *ovButton = lookupContext->context()->lookupType(doc.data(), QStringList() << "MyButton");
    QCOMPARE(ovButton->className(), QString("MyButton"));
    QCOMPARE(ovButton->prototype(lookupContext->context())->className(), QString("Rectangle"));

    const Interpreter::ObjectValue *ovProperty = lookupContext->context()->lookupType(doc.data(), QStringList() << "Item" << "states");
    QVERIFY(ovProperty);
    QCOMPARE(ovProperty->className(), QString("State"));

    const Interpreter::QmlObjectValue * qmlItemValue = dynamic_cast<const Interpreter::QmlObjectValue *>(ovItem);
    QVERIFY(qmlItemValue);
    QCOMPARE(qmlItemValue->defaultPropertyName(), QString("data"));
    QCOMPARE(qmlItemValue->propertyType("state"), QString("string"));

    const Interpreter::ObjectValue *ovState = lookupContext->context()->lookupType(doc.data(), QStringList() << "State");
    const Interpreter::QmlObjectValue * qmlState2Value = dynamic_cast<const Interpreter::QmlObjectValue *>(ovState);
    QCOMPARE(qmlState2Value->className(), QString("State"));

    const Interpreter::ObjectValue *ovImage = lookupContext->context()->lookupType(doc.data(), QStringList() << "Image");
    const Interpreter::QmlObjectValue * qmlImageValue = dynamic_cast<const Interpreter::QmlObjectValue *>(ovImage);
    QCOMPARE(qmlImageValue->className(), QString("Image"));
    QCOMPARE(qmlImageValue->propertyType("source"), QString("QUrl"));
}

void tst_Basic::testMetaInfo()
{
    TestModelManager modelManager;

    modelManager.loadFile(QString(QTCREATORDIR) + "/tests/auto/qml/qmldesigner/data/fx/usingmybutton.qml");

    Snapshot snapshot = modelManager.snapshot();
    Document::Ptr doc = snapshot.document(QString(QTCREATORDIR) + "/tests/auto/qml/qmldesigner/data/fx/usingmybutton.qml");
    QVERIFY(doc && doc->isQmlDocument());

    LookupContext::Ptr lookupContext = LookupContext::create(doc, snapshot, QList<AST::Node*>());

    MetaInfo::setDocument(doc);
    MetaInfo::setLookupContext(lookupContext);

    MetaInfo item("Qt/Item", 4, 7);
    MetaInfo myButton("MyButton");
    MetaInfo textEdit("TextEdit");
    MetaInfo super("Qt/Super", 4, 7);
    MetaInfo maniac("Maniac");

    QVERIFY(item.isValid());
    QVERIFY(myButton.isValid());
    QVERIFY(textEdit.isValid());

    QVERIFY(!super.isValid());
    QVERIFY(!maniac.isValid());

    QVERIFY(textEdit.localProperties().contains("color"));
    QVERIFY(textEdit.isPropertyWritable("color"));
    MetaInfo text("Qt/Text", 4, 7);

    QVERIFY(item.isValid());
    QVERIFY(myButton.isValid());

    QVERIFY(!item.isComponent());
    QVERIFY(myButton.isComponent());

    QVERIFY(myButton.properties().contains("text"));
    QVERIFY(myButton.properties().contains("myNumber"));
    QVERIFY(myButton.properties().contains("gradient"));
    QVERIFY(myButton.properties().contains("border.width"));
    QVERIFY(myButton.properties().contains("anchors.bottomMargin"));
    QVERIFY(myButton.localProperties().contains("text"));
    QVERIFY(myButton.localProperties().contains("myNumber"));
    QVERIFY(!myButton.localProperties().contains("border.width"));

    QVERIFY(!item.properties().contains("text"));
    QVERIFY(!item.properties().contains("myNumber"));
    QVERIFY(item.properties().contains("anchors.bottomMargin"));
    QVERIFY(item.properties().contains("x"));
    QVERIFY(!item.localProperties().contains("x"));
    QVERIFY(!item.localProperties().contains("enabled"));
    QVERIFY(item.localProperties().contains("anchors.bottomMargin"));
    QVERIFY(item.localProperties().contains("states"));
    QVERIFY(item.localProperties().contains("parent"));

    QCOMPARE(myButton.propertyTypeName("text"), QString("string"));
    QCOMPARE(myButton.propertyTypeName("myNumber"), QString("real"));
    QCOMPARE(myButton.propertyTypeName("x"), QString("qreal"));
    QCOMPARE(myButton.propertyTypeName("border.width"), QString("int"));
    QCOMPARE(myButton.propertyTypeName("gradient"), QString("Qt/Gradient"));

    QCOMPARE(myButton.defaultPropertyName(), QString("content"));
    QCOMPARE(item.defaultPropertyName(), QString("data"));

    QVERIFY(item.isPropertyList("children"));
    QVERIFY(myButton.isPropertyList("children"));
    QVERIFY(item.isPropertyList("data"));
    QVERIFY(myButton.isPropertyList("data"));
    QVERIFY(item.isPropertyList("states"));
    QVERIFY(myButton.isPropertyList("states"));

    QVERIFY(!item.isPropertyList("x"));
    QVERIFY(!myButton.isPropertyList("x"));
    QVERIFY(!item.isPropertyList("state"));
    QVERIFY(!myButton.isPropertyList("state"));

    QVERIFY(myButton.isPropertyPointer("parent"));
    QVERIFY(text.localProperties().contains("font"));
    QVERIFY(!text.isPropertyPointer("font"));
    QVERIFY(myButton.isPropertyWritable("state"));
    QVERIFY(!myButton.isPropertyWritable("border"));

    QVERIFY(text.localProperties().contains("horizontalAlignment"));
    QVERIFY(text.isPropertyEnum("horizontalAlignment"));

    foreach (Interpreter::ImportInfo import, doc->bind()->imports()) {
        QCOMPARE(import.name(), QString("Qt"));
    }
}

QTEST_MAIN(tst_Basic);

#include "tst_basic.moc"
