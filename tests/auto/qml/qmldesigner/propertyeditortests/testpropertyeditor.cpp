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

#include "testpropertyeditor.h"
#include "../testview.h"

#include <memory>
#include <cstdio>

#include <metainfo.h>
#include <model.h>
#include <modelnode.h>
#include <variantproperty.h>
#include <bytearraymodifier.h>
#include <invalididexception.h>
#include <invalidmodelnodeexception.h>
#include <nodeproperty.h>
#include <propertyeditor.h>
#include <QDebug>
#include <QDir>
#include <QSpinBox>
#include <QLineEdit>
#include <QStackedWidget>
#include <QDeclarativeView>
#include <QVariant>

using namespace QmlDesigner;

#include <cstdio>
#include "../common/statichelpers.cpp"


static void inspectPropertyEditor(ModelNode node, QWidget* propWidget)
{
    if (!propWidget)
        return;

    QStackedWidget * stackedWidget = qobject_cast<QStackedWidget *> (propWidget);
    QVERIFY(stackedWidget);
    QDeclarativeView *view = qobject_cast<QDeclarativeView*>(stackedWidget->currentWidget());
    QVERIFY(view);

    QLineEdit * idLineEdit = view->findChild<QLineEdit* >("IdLineEdit");

    if (!idLineEdit)
        return ;

    QCOMPARE(idLineEdit->text(), node.id());

    if (node.hasProperty("width")) {
         QSpinBox * widthSpinBox = view->findChild<QSpinBox* >("WidthSpinBox");
         QVERIFY(widthSpinBox);
         QCOMPARE(widthSpinBox->value(), node.variantProperty("width").value().toInt());
    }

    if (node.hasProperty("height")) {
        QSpinBox * heightSpinBox = view->findChild<QSpinBox* >("HeightSpinBox");
        QVERIFY(heightSpinBox);
        QCOMPARE(heightSpinBox->value(), node.variantProperty("height").value().toInt());// this can be dangerous
    }

    if (node.hasProperty("x")) {
        QSpinBox * xSpinBox = view->findChild<QSpinBox* >("XSpinBox");
        QVERIFY(xSpinBox);
        QCOMPARE(xSpinBox->value(), node.variantProperty("x").value().toInt()); // this can be dangerous
    }

    if (node.hasProperty("y")) {
        QSpinBox * ySpinBox = view->findChild<QSpinBox* >("YSpinBox");
        QVERIFY(ySpinBox);
        QCOMPARE(ySpinBox->value(), node.variantProperty("y").value().toInt());
    }
}
static void selectThrough(ModelNode node, QWidget* propWidget = 0)
{
    QVERIFY(node.isValid());
    int numberOfProperties = node.propertyNames().count();
    QList<AbstractProperty> properties = node.properties();
    node.view()->clearSelectedModelNodes();
    node.view()->selectModelNode(node);
    QString name = node.id();
    qApp->processEvents();
    QTest::qSleep(100);
    qApp->processEvents();
    QTest::qSleep(100);
    qApp->processEvents();
    inspectPropertyEditor(node, propWidget);
    //selecting should not effect any properties at all!
    QCOMPARE(node.propertyNames().count(), numberOfProperties);
    foreach (const AbstractProperty &property, properties)
        if (property.isVariantProperty()) {
            QCOMPARE(property.toVariantProperty().value(), node.variantProperty(property.name()).value());
        }
    QList<ModelNode> childNodes = node.allDirectSubModelNodes();
    foreach (const ModelNode &childNode, childNodes)
        selectThrough(childNode, propWidget);
}

static QWidget * setupPropertyEditor(QWidget *widget, Model *model)
{
    PropertyEditor *properties = new PropertyEditor(widget);

    QString qmlDir = QDir::cleanPath(qApp->applicationDirPath() + QString("/../shared/propertyeditor/"));
    qDebug() << qmlDir;
    properties->setQmlDir(qmlDir);
    model->attachView(properties);
    QWidget *pane = properties->createPropertiesPage();
    pane->setParent(widget);
    widget->show();
    widget->resize(300, 800);
    qApp->processEvents();
    pane->resize(300, 800);
    pane->move(0,0);
    qApp->processEvents();
    QTest::qSleep(100);

    return pane;
}

static void loadFileAndTest(const QString &fileName)
{
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));

    QList<QDeclarativeError> errors;
    //std::auto_ptr<ByteArrayModifier> modifier1(ByteArrayModifier::create(QString(file.readAll())));
    //std::auto_ptr<Model> model1(Model::create(modifier1.get(), QUrl::fromLocalFile(file.fileName()), &errors));

    QScopedPointer<Model> model1(Model::create("Item"));
    QVERIFY(model1.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model1->attachView(view.data());


    QVERIFY(model1.data());

    std::auto_ptr<QWidget> widget(new QWidget());
    QWidget *propWidget = setupPropertyEditor(widget.get(), model1.data());

    selectThrough(view->rootModelNode(), propWidget);
}

TestPropertyEditor::TestPropertyEditor()
    : QObject()
{
}

void TestPropertyEditor::initTestCase()
{
#ifndef QDEBUG_IN_TESTS
    qInstallMsgHandler(testMessageOutput);
#endif
    Exception::setShouldAssert(false);
}


void TestPropertyEditor::createCoreModel()
{
    try {
        std::auto_ptr<QWidget> widget(new QWidget());

        QScopedPointer<Model> model(Model::create("import Qt 4.6\n Item {}"));
        QVERIFY(model.data());

        QScopedPointer<TestView> view(new TestView);
        QVERIFY(view.data());
        model->attachView(view.data());

        QVERIFY(model.data());
        setupPropertyEditor(widget.get(), model.data());

        QVERIFY(view->rootModelNode().isValid());
        int numberOfProperties = view->rootModelNode().propertyNames().count();
        selectThrough(view->rootModelNode());
        QCOMPARE(view->rootModelNode().propertyNames().count(), numberOfProperties);
    } catch (Exception &) {
        QFAIL("Exception thrown");
    }
}

void TestPropertyEditor::loadEmptyCoreModel()
{
   /* QList<QDeclarativeError> errors;
    QFile file(":/fx/empty.qml");
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));

    std::auto_ptr<QWidget> widget(new QWidget());
    std::auto_ptr<ByteArrayModifier> modifier1(ByteArrayModifier::create(QString(file.readAll())));
    std::auto_ptr<Model> model1(Model::create(modifier1.get(), QUrl(), &errors));
    foreach (const QDeclarativeError &error, errors)
        QFAIL(error.toString().toLatin1());
    QVERIFY(model1.get());
    setupPropertyEditor(widget.get(), model1.get());

    selectThrough(model1->rootNode());

    std::auto_ptr<ByteArrayModifier> modifier2(ByteArrayModifier::create("import Qt 4.6\n Item{}"));
    std::auto_ptr<Model> model2(Model::create(modifier2.get(), QUrl(), &errors));
    foreach (const QDeclarativeError &error, errors)
        QFAIL(error.toString().toLatin1());
    QVERIFY(model2.get());

    QVERIFY(compareTree(model1->rootNode(), model2->rootNode()));*/
}

void TestPropertyEditor::createSubNode()
{
    std::auto_ptr<QWidget> widget(new QWidget());

    QScopedPointer<Model> model(Model::create("import Qt 4.6\n Item {}"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    setupPropertyEditor(widget.get(), model.data());
    QVERIFY(view->rootModelNode().isValid());
    selectThrough(view->rootModelNode());

    ModelNode childNode = view->createModelNode("Qt/Rectangle", 4, 6);
    view->rootModelNode().nodeListProperty("data").reparentHere(childNode);

    QVERIFY(childNode.isValid());
    QVERIFY(view->rootModelNode().allDirectSubModelNodes().contains(childNode));
    QVERIFY(childNode.parentProperty().parentModelNode() == view->rootModelNode());
    QCOMPARE(childNode.type(), QString("Qt/Rectangle"));

    selectThrough(childNode);

    QVERIFY(childNode.id().isEmpty());

    childNode.setId("Blah");
    QCOMPARE(childNode.id(), QString("Blah"));


    QCOMPARE(childNode.id(), QString("Blah"));
}

void TestPropertyEditor::createRect()
{
    try {

        std::auto_ptr<QWidget> widget(new QWidget());

        QScopedPointer<Model> model(Model::create("import Qt 4.6\n Item {}"));
        QVERIFY(model.data());

        QScopedPointer<TestView> view(new TestView);
        QVERIFY(view.data());
        model->attachView(view.data());

        setupPropertyEditor(widget.get(), model.data());

        QVERIFY(view->rootModelNode().isValid());

        //selectThrough(view->rootModelNode());

        ModelNode childNode = view->createModelNode("Qt/Rectangle", 4, 6);
        view->rootModelNode().nodeListProperty("data").reparentHere(childNode);

        QVERIFY(childNode.isValid());
        QVERIFY(view->rootModelNode().allDirectSubModelNodes().contains(childNode));
        QVERIFY(childNode.parentProperty().parentModelNode() == view->rootModelNode());
        QCOMPARE(childNode.type(), QString("Qt/Rectangle"));

        QVERIFY(childNode.id().isEmpty());

        childNode.setId("Rect01");
        QCOMPARE(childNode.id(), QString("Rect01"));

        childNode.variantProperty("x") = 100;
        QCOMPARE(QmlObjectNode(childNode).instanceValue("x").toInt(), 100);
        childNode.variantProperty("y") = 100;
        QCOMPARE(QmlObjectNode(childNode).instanceValue("y").toInt(), 100);
        childNode.variantProperty("width") = 100;
        QCOMPARE(QmlObjectNode(childNode).instanceValue("width").toInt(), 100);
        childNode.variantProperty("height") = 100;
        QCOMPARE(QmlObjectNode(childNode).instanceValue("height").toInt(), 100);

        selectThrough(childNode);

        QCOMPARE(childNode.propertyNames().count(), 4);
        QCOMPARE(childNode.variantProperty("scale").value(), QVariant());

    } catch (Exception &) {
        QFAIL("Exception thrown");
    }
}

void TestPropertyEditor::removeNode()
{
    std::auto_ptr<QWidget> widget(new QWidget());

    QScopedPointer<Model> model(Model::create("import Qt 4.6\n Item {}"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    setupPropertyEditor(widget.get(), model.data());

    QCOMPARE(view->rootModelNode().allDirectSubModelNodes().count(), 0);

    selectThrough(view->rootModelNode());

    ModelNode childNode = view->createModelNode("Qt/Rectangle", 4, 6);
    view->rootModelNode().nodeListProperty("data").reparentHere(childNode);
    QVERIFY(childNode.isValid());
    QCOMPARE(view->rootModelNode().allDirectSubModelNodes().count(), 1);
    QVERIFY(view->rootModelNode().allDirectSubModelNodes().contains(childNode));
    QVERIFY(childNode.parentProperty().parentModelNode() == view->rootModelNode());

    selectThrough(childNode);

    ModelNode subChildNode = view->createModelNode("Qt/Rectangle", 4, 6);
    childNode.nodeListProperty("data").reparentHere(subChildNode);
    QVERIFY(subChildNode.isValid());
    QCOMPARE(childNode.allDirectSubModelNodes().count(), 1);
    QVERIFY(childNode.allDirectSubModelNodes().contains(subChildNode));
    QVERIFY(subChildNode.parentProperty().parentModelNode() == childNode);

    selectThrough(subChildNode);

    childNode.destroy();

    QCOMPARE(view->rootModelNode().allDirectSubModelNodes().count(), 0);
    QVERIFY(!view->rootModelNode().allDirectSubModelNodes().contains(childNode));
    QVERIFY(!childNode.isValid());
    QVERIFY(!subChildNode.isValid());
}

void TestPropertyEditor::loadWelcomeScreen()
{
    loadFileAndTest(QCoreApplication::applicationDirPath() + "/../shared/welcomescreen.qml");
}

void TestPropertyEditor::loadHelloWorld()
{
    loadFileAndTest(":/fx/helloworld.qml");
}



void TestPropertyEditor::cleanupTestCase()
{
    MetaInfo::clearGlobal();
}


QTEST_MAIN(TestPropertyEditor);
