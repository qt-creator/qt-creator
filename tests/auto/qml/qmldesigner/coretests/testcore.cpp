/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "testcore.h"

#include <QScopedPointer>
#include <QLatin1String>

#include <metainfo.h>
#include <model.h>
#include <modelmerger.h>
#include <modelnode.h>
#include <qmlanchors.h>
#include <invalididexception.h>
#include <invalidmodelnodeexception.h>
#include <nodeinstanceview.h>
#include <nodeinstance.h>
#include <QDebug>

#include "../testview.h"
#include <variantproperty.h>
#include <abstractproperty.h>
#include <bindingproperty.h>
#include <nodeproperty.h>

#include <nodelistproperty.h>
#include <nodeabstractproperty.h>
#include <plaintexteditmodifier.h>
#include <componenttextmodifier.h>

#include <bytearraymodifier.h>
#include "testrewriterview.h"

#include <QPlainTextEdit>
#include <private/qmlstate_p.h>


using namespace QmlDesigner;
#include <cstdio>
#include "../common/statichelpers.cpp"

TestCore::TestCore()
    : QObject()
{
}

void TestCore::initTestCase()
{
#ifndef QDEBUG_IN_TESTS
    qInstallMsgHandler(testMessageOutput);
#endif
    MetaInfo::setPluginPaths(pluginPaths());
    Exception::setShouldAssert(false);
}

void TestCore::cleanupTestCase()
{
    MetaInfo::clearGlobal();
}

void TestCore::testModelCreateCoreModel()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> testView(new TestView);
    QVERIFY(testView.data());
    model->attachView(testView.data());

    QVERIFY(testView->rootModelNode().isValid());
    NodeInstanceView *nodeInstanceView = new NodeInstanceView(model.data());
    model->attachView(nodeInstanceView);
    model->detachView(nodeInstanceView);
}

// TODO: this need to e updated for states
void TestCore::loadEmptyCoreModel()
{
    QList<QmlError> errors;
    QFile file(":/fx/empty.qml");
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));

    QPlainTextEdit textEdit1;
    textEdit1.setPlainText(file.readAll());
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model1(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model1->attachView(testRewriterView1.data());

    QPlainTextEdit textEdit2;
    textEdit2.setPlainText("import Qt 4.6; Item{}");
    PlainTextEditModifier modifier2(&textEdit2);

    QScopedPointer<Model> model2(Model::create("Qt/item"));

    QScopedPointer<TestRewriterView> testRewriterView2(new TestRewriterView());
    testRewriterView2->setTextModifier(&modifier2);
    model2->attachView(testRewriterView2.data());

    QVERIFY(compareTree(testRewriterView1->rootModelNode(), testRewriterView2->rootModelNode()));
}

void TestCore::testRewriterView()
{
    try {
        QPlainTextEdit textEdit;
        textEdit.setPlainText("import Qt 4.6;\n\nItem {\n}\n");
        PlainTextEditModifier textModifier(&textEdit);

        QScopedPointer<Model> model(Model::create("Qt/Item"));
        QVERIFY(model.data());

        QScopedPointer<TestView> view(new TestView);
        QVERIFY(view.data());
        model->attachView(view.data());

        ModelNode rootModelNode(view->rootModelNode());
        QVERIFY(rootModelNode.isValid());
        QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
        testRewriterView->setTextModifier(&textModifier);
        model->attachView(testRewriterView.data());

        ModelNode childNode(rootModelNode.addChildNode("Qt/Item", 4, 6, "data"));
        QVERIFY(childNode.isValid());

        childNode.changeType("Qt/Rectangle", 4, 6);
        childNode.setId("childNode");

        ModelNode childNode2(childNode.addChildNode("Qt/Rectangle", 4, 6, "data"));
        childNode2.setId("childNode2");
        ModelNode childNode3(childNode2.addChildNode("Qt/Rectangle", 4, 6, "data"));
        childNode3.setId("childNode3");
        ModelNode childNode4(childNode3.addChildNode("Qt/Rectangle", 4, 6, "data"));
        childNode4.setId("childNode4");

        QVERIFY(childNode.isValid());
        QVERIFY(childNode2.isValid());
        QVERIFY(childNode3.isValid());
        QVERIFY(childNode4.isValid());

        testRewriterView->setModificationGroupActive(true);

        childNode.destroy();

        QVERIFY(!childNode.isValid());
        QVERIFY(!childNode2.isValid());
        QVERIFY(!childNode3.isValid());
        QVERIFY(!childNode4.isValid());

        QVERIFY(testRewriterView->modelToTextMerger()->isNodeScheduledForRemoval(childNode));
        QVERIFY(!testRewriterView->modelToTextMerger()->isNodeScheduledForRemoval(childNode2));
        QVERIFY(!testRewriterView->modelToTextMerger()->isNodeScheduledForRemoval(childNode3));
        QVERIFY(!testRewriterView->modelToTextMerger()->isNodeScheduledForRemoval(childNode4));

        QVERIFY(!rootModelNode.hasProperty(QLatin1String("data")));

        testRewriterView->modelToTextMerger()->applyChanges();

        childNode = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
        QVERIFY(testRewriterView->modelToTextMerger()->isNodeScheduledForAddition(childNode));

        testRewriterView->modelToTextMerger()->applyChanges();

        childNode.variantProperty("x") = 70;
        childNode.variantProperty("y") = 90;

        QCOMPARE(testRewriterView->modelToTextMerger()->findAddedVariantProperty(childNode.variantProperty("x")).value(), QVariant(70));
        QCOMPARE(testRewriterView->modelToTextMerger()->findAddedVariantProperty(childNode.variantProperty("y")).value(), QVariant(90));

        model->detachView(testRewriterView.data());
    } catch (Exception &e) {
        QFAIL(qPrintable(e.description()));
    }
}

void TestCore::testRewriterErrors()
{
    QPlainTextEdit textEdit;
    textEdit.setPlainText("import Qt 4.6;\n\nItem {\n}\n");
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    QVERIFY(testRewriterView->errors().isEmpty());
    textEdit.setPlainText("import Qt 4.6;\n\nError {\n}\n");
    QVERIFY(!testRewriterView->errors().isEmpty());

    textEdit.setPlainText("import Qt 4.6;\n\nItem {\n}\n");
    QVERIFY(testRewriterView->errors().isEmpty());
}

void TestCore::saveEmptyCoreModel()
{
    QList<QmlError> errors;
    QFile file(":/fx/empty.qml");
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));

    QPlainTextEdit textEdit1;
    textEdit1.setPlainText(file.readAll());
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model1(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model1->attachView(testRewriterView1.data());


    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite | QIODevice::Text);
    modifier1.save(&buffer);

    QPlainTextEdit textEdit2;
    textEdit2.setPlainText("import Qt 4.6; Item{}");
    PlainTextEditModifier modifier2(&textEdit2);

    QScopedPointer<Model> model2(Model::create("Qt/item"));

    QScopedPointer<TestRewriterView> testRewriterView2(new TestRewriterView());
    testRewriterView2->setTextModifier(&modifier2);
    model2->attachView(testRewriterView2.data());

    QVERIFY(compareTree(testRewriterView1->rootModelNode(), testRewriterView2->rootModelNode()));

}

void TestCore::loadAttributesInCoreModel()
{
    QList<QmlError> errors;
    QFile file(":/fx/attributes.qml");
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));

    QPlainTextEdit textEdit1;
    textEdit1.setPlainText(file.readAll());
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model1(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model1->attachView(testRewriterView1.data());

    QPlainTextEdit textEdit2;
    textEdit2.setPlainText("import Qt 4.6; Item{}");
    PlainTextEditModifier modifier2(&textEdit2);

    QScopedPointer<Model> model2(Model::create("Qt/item"));

    QScopedPointer<TestRewriterView> testRewriterView2(new TestRewriterView());
    testRewriterView2->setTextModifier(&modifier2);
    model2->attachView(testRewriterView2.data());

    ModelNode rootModelNode = testRewriterView2->rootModelNode();

    rootModelNode.setId("theItem");
    rootModelNode.variantProperty("x").setValue(QVariant(300));
    rootModelNode.variantProperty("visible").setValue(true);
    rootModelNode.variantProperty("scale").setValue(0.5);

    QVERIFY(compareTree(testRewriterView1->rootModelNode(), testRewriterView2->rootModelNode()));
}

void TestCore::saveAttributesInCoreModel()
{
    QFile file(":/fx/attributes.qml");
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));

    QPlainTextEdit textEdit1;
    textEdit1.setPlainText(file.readAll());
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model1(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model1->attachView(testRewriterView1.data());

    QVERIFY(testRewriterView1->errors().isEmpty());

    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite | QIODevice::Text);
    modifier1.save(&buffer);

    QPlainTextEdit textEdit2;
    textEdit2.setPlainText(buffer.data());
    PlainTextEditModifier modifier2(&textEdit2);

    QScopedPointer<Model> model2(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView2(new TestRewriterView());
    testRewriterView2->setTextModifier(&modifier2);
    model2->attachView(testRewriterView2.data());

    QVERIFY(testRewriterView2->errors().isEmpty());

    QVERIFY(compareTree(testRewriterView1->rootModelNode(), testRewriterView2->rootModelNode()));
}

void TestCore::testModelCreateRect()
{
     try {

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QVERIFY(view->rootModelNode().isValid());
    ModelNode childNode = view->rootModelNode().addChildNode("Qt/Rectangle", 4, 6, "data");
    QVERIFY(childNode.isValid());
    QVERIFY(view->rootModelNode().allDirectSubModelNodes().contains(childNode));
    QVERIFY(childNode.parentProperty().parentModelNode() == view->rootModelNode());
    QCOMPARE(childNode.simplifiedTypeName(), QString("Rectangle"));

    QVERIFY(childNode.id().isEmpty());

    childNode.setId("Rect01");
    QCOMPARE(childNode.id(), QString("Rect01"));

    childNode.variantProperty("x") = 100;
    childNode.variantProperty("y") = 100;
    childNode.variantProperty("width") = 100;
    childNode.variantProperty("height") = 100;
    
    QCOMPARE(childNode.propertyNames().count(), 4);
    QCOMPARE(childNode.variantProperty("scale").value(), QVariant());

     } catch (Exception &exception) {
        QFAIL("Exception thrown");
    }

}

void TestCore::loadComponentPropertiesInCoreModel()
{
    QFile file(":/fx/properties.qml");
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));

    QPlainTextEdit textEdit1;
    textEdit1.setPlainText(file.readAll());
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model1(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model1->attachView(testRewriterView1.data());

    QVERIFY(testRewriterView1->rootModelNode().variantProperty("pushed").isDynamic());
    QVERIFY(!testRewriterView1->rootModelNode().variantProperty("pushed").value().isValid());

    QPlainTextEdit textEdit2;
    textEdit2.setPlainText("import Qt 4.6; Item{}");
    PlainTextEditModifier modifier2(&textEdit2);

    QScopedPointer<Model> model2(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView2(new TestRewriterView());
    testRewriterView2->setTextModifier(&modifier2);
    model2->attachView(testRewriterView2.data());

    testRewriterView2->rootModelNode().variantProperty("pushed").setDynamicTypeNameAndValue("bool", QVariant());

    QVERIFY(compareTree(testRewriterView1->rootModelNode(), testRewriterView2->rootModelNode()));
}

void TestCore::loadSubItems()
{
    QFile file(QCoreApplication::applicationDirPath() + "/../tests/data/fx/topitem.qml");
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));

    QPlainTextEdit textEdit1;
    textEdit1.setPlainText(file.readAll());
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model1(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model1->attachView(testRewriterView1.data());
}


void TestCore::createInvalidCoreModel()
{
    QScopedPointer<Model> invalidModel(Model::create("ItemSUX"));
    //QVERIFY(!invalidModel.data()); //#no direct ype checking in model atm

    QScopedPointer<Model> invalidModel2(Model::create("InvalidNode"));
    //QVERIFY(!invalidModel2.data());
}

void TestCore::testModelCreateSubNode()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QList<TestView::MethodCall> expectedCalls;
    expectedCalls << TestView::MethodCall("modelAttached", QStringList() << QString::number(reinterpret_cast<long>(model.data())));
    QCOMPARE(view->methodCalls(), expectedCalls);

    QVERIFY(view->rootModelNode().isValid());
    ModelNode childNode = view->rootModelNode().addChildNode("Qt/Rectangle", 4, 6, "data");
    QVERIFY(childNode.isValid());
    QVERIFY(view->rootModelNode().allDirectSubModelNodes().contains(childNode));
    QVERIFY(childNode.parentProperty().parentModelNode() == view->rootModelNode());
    QCOMPARE(childNode.simplifiedTypeName(), QString("Rectangle"));

    expectedCalls << TestView::MethodCall("nodeCreated", QStringList() << "");
    expectedCalls << TestView::MethodCall("nodeReparented", QStringList() << "" << "data" << "" << "PropertiesAdded");
    QCOMPARE(view->methodCalls(), expectedCalls);

    QVERIFY(childNode.id().isEmpty());
    childNode.setId("Blah");
    QCOMPARE(childNode.id(), QString("Blah"));

    expectedCalls << TestView::MethodCall("nodeIdChanged", QStringList() << "Blah" << "Blah" << "");
    QCOMPARE(view->methodCalls(), expectedCalls);

    try {
        childNode.setId("invalid id");
        QFAIL("Setting an invalid id does not throw an excxeption");
    } catch (Exception &exception) {
        QCOMPARE(exception.type(), QString("InvalidIdException"));
    }

    QCOMPARE(childNode.id(), QString("Blah"));
    QCOMPARE(view->methodCalls(), expectedCalls);

    childNode.setId(QString());
    QVERIFY(childNode.id().isEmpty());
}


void TestCore::testTypicalRewriterOperations()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode = view->rootModelNode();
    QCOMPARE(rootModelNode.allDirectSubModelNodes().count(), 0);

    QVERIFY(rootModelNode.property("test").isValid());
    QVERIFY(!rootModelNode.property("test").isVariantProperty());
    QVERIFY(!rootModelNode.property("test").isBindingProperty());

    QVERIFY(rootModelNode.variantProperty("test").isValid());
    QVERIFY(!rootModelNode.hasProperty("test"));

    rootModelNode.variantProperty("test") = 70;

    QVERIFY(rootModelNode.hasProperty("test"));
    QVERIFY(rootModelNode.property("test").isVariantProperty());
    QCOMPARE(rootModelNode.variantProperty("test").value(), QVariant(70));

    rootModelNode.bindingProperty("test") = "parent.x";
    QVERIFY(!rootModelNode.property("test").isVariantProperty());
    QVERIFY(rootModelNode.property("test").isBindingProperty());

    QCOMPARE(rootModelNode.bindingProperty("test").expression(), QString("parent.x"));

    ModelNode childNode(rootModelNode.addChildNode("Qt/Rectangle", 4 ,6, "data"));
    rootModelNode.nodeListProperty("test").reparentHere(childNode);
    QCOMPARE(childNode.parentProperty(), rootModelNode.nodeAbstractProperty("test"));
    QVERIFY(rootModelNode.property("test").isNodeAbstractProperty());
    QVERIFY(rootModelNode.property("test").isNodeListProperty());
    QVERIFY(!rootModelNode.property("test").isBindingProperty());
    QVERIFY(childNode.parentProperty().isNodeListProperty());    
    QCOMPARE(childNode, childNode.parentProperty().toNodeListProperty().toModelNodeList().first());
    QCOMPARE(rootModelNode, childNode.parentProperty().parentModelNode());
    QCOMPARE(childNode.parentProperty().name(), QString("test"));

    QVERIFY(!rootModelNode.property("test").isVariantProperty());
    rootModelNode.variantProperty("test") = 90;
    QVERIFY(rootModelNode.property("test").isVariantProperty());
    QCOMPARE(rootModelNode.variantProperty("test").value(), QVariant(90));

}

void TestCore::testBasicStates()
{
    char qmlString[] = "import Qt 4.6\n"
                       "Rectangle {\n"
                       "id: root;\n"
                            "Rectangle {\n"
                                "id: rect1;\n"
                            "}\n"
                            "Rectangle {\n"
                                "id: rect2;\n"
                            "}\n"
                            "states: [\n"
                                "State {\n"
                                    "name: \"state1\"\n"
                                    "PropertyChanges {\n"
                                        "target: rect1\n"
                                    "}\n"
                                    "PropertyChanges {\n"
                                        "target: rect2\n"
                                    "}\n"
                                "}\n"
                                ","
                                "State {\n"
                                    "name: \"state2\"\n"
                                    "PropertyChanges {\n"
                                        "target: rect1\n"
                                    "}\n"
                                    "PropertyChanges {\n"
                                        "target: rect2;\n"
                                        "x: 10;\n"
                                    "}\n"
                                "}\n"
                            "]\n"
                       "}\n";

    Exception::setShouldAssert(true);

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.type(), QString("Qt/Item"));
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);

    model->attachView(testRewriterView.data());
    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.type(), QString("Qt/Rectangle"));

    QVERIFY(rootModelNode.hasProperty("data"));

    QVERIFY(rootModelNode.property("data").isNodeListProperty());

    QCOMPARE(rootModelNode.nodeListProperty("data").toModelNodeList().count(), 2);

    ModelNode rect1 = rootModelNode.nodeListProperty("data").toModelNodeList().first();
    ModelNode rect2 = rootModelNode.nodeListProperty("data").toModelNodeList().last();

    QVERIFY(QmlItemNode(rect1).isValid());
    QVERIFY(QmlItemNode(rect2).isValid());

    QVERIFY(QmlItemNode(rootModelNode).isValid());

    QCOMPARE(QmlItemNode(rootModelNode).states().allStates().count(), 2);
    QCOMPARE(QmlItemNode(rootModelNode).states().names().count(), 2);
    QCOMPARE(QmlItemNode(rootModelNode).states().names().first(), QString("state1"));
    QCOMPARE(QmlItemNode(rootModelNode).states().names().last(), QString("state2"));

    //
    // QmlModelState API tests
    //
    QmlModelState state1 = QmlItemNode(rootModelNode).states().state("state1");
    QmlModelState state2 = QmlItemNode(rootModelNode).states().state("state2");

    QVERIFY(state1.isValid());
    QVERIFY(state2.isValid());

    QCOMPARE(state1.propertyChanges().count(), 2);
    QCOMPARE(state2.propertyChanges().count(), 2);

    QVERIFY(!state1.hasPropertyChanges(rootModelNode));

    QVERIFY(state1.propertyChanges(rect1).isValid());
    QVERIFY(state1.propertyChanges(rect2).isValid());

    state1.propertyChanges(rect2).modelNode().hasProperty("x");

    QCOMPARE(QmlItemNode(rect1).allAffectingStates().count(), 2);
    QCOMPARE(QmlItemNode(rect2).allAffectingStates().count(), 2);
    QCOMPARE(QmlItemNode(rootModelNode).allAffectingStates().count(), 0);
    QCOMPARE(QmlItemNode(rect1).allAffectingStatesOperations().count(), 2);
    QCOMPARE(QmlItemNode(rect2).allAffectingStatesOperations().count(), 2);

    //
    // check real state2 object
    //

    NodeInstance state2Instance = view->instanceForModelNode(state2.modelNode());
    QVERIFY(state2Instance.isValid());
    QmlState *stateObject = qobject_cast<QmlState*>(const_cast<QObject*>(state2Instance.testHandle()));
    QCOMPARE(stateObject->changes()->count(), 2);
    QCOMPARE(stateObject->changes()->at(0)->actions().size(), 0);
    QCOMPARE(stateObject->changes()->at(1)->actions().size(), 1);


    //
    // actual state switching
    //

    // base state
    QCOMPARE(view->currentState(), view->baseState());
    NodeInstance rect2Instance = view->instanceForModelNode(rect2);
    QVERIFY(rect2Instance.isValid());
    QCOMPARE(rect2Instance.property("x").toInt(), 0);

    int expectedViewMethodCount = view->methodCalls().count();

    // base state-> state2
    view->setCurrentState(state2);
    QCOMPARE(view->currentState(), state2);
    QCOMPARE(view->methodCalls().size(), ++expectedViewMethodCount);
    QCOMPARE(view->methodCalls().last(), TestView::MethodCall("stateChanged", QStringList() << "state2" << QString()));
    QCOMPARE(rect2Instance.property("x").toInt(), 10);

    // state2 -> state1
    view->setCurrentState(state1);
    QCOMPARE(view->currentState(), state1);
    QCOMPARE(view->methodCalls().size(), ++expectedViewMethodCount);
    QCOMPARE(view->methodCalls().last(), TestView::MethodCall("stateChanged", QStringList() << "state1" << "state2"));
    QCOMPARE(rect2Instance.property("x").toInt(), 0);

    // state1 -> baseState
    view->setCurrentState(view->baseState());
    QCOMPARE(view->currentState(), view->baseState());
    QCOMPARE(view->methodCalls().size(), ++expectedViewMethodCount);
    QCOMPARE(view->methodCalls().last(), TestView::MethodCall("stateChanged", QStringList() << QString() << "state1"));
    QCOMPARE(rect2Instance.property("x").toInt(), 0);
}

void TestCore::testModelBasicOperations()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode = view->rootModelNode();
    QCOMPARE(rootModelNode.allDirectSubModelNodes().count(), 0);

    rootModelNode.variantProperty("width").setValue(10);
    rootModelNode.variantProperty("height").setValue(10);

    QCOMPARE(rootModelNode.variantProperty("width").value().toInt(), 10);
    QCOMPARE(rootModelNode.variantProperty("height").value().toInt(), 10);

    QVERIFY(rootModelNode.hasProperty("width"));
    rootModelNode.removeProperty("width");
    QVERIFY(!rootModelNode.hasProperty("width"));

    QVERIFY(!rootModelNode.hasProperty("children"));
    ModelNode childNode1(rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "children"));
    ModelNode childNode2(rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data"));

    QVERIFY(childNode1.isValid());
    QVERIFY(childNode2.isValid());

    QVERIFY(childNode1.parentProperty().parentModelNode().isValid());
    QVERIFY(childNode2.parentProperty().parentModelNode().isValid());

    QVERIFY(rootModelNode.hasProperty("children"));
    childNode2.setParentProperty(rootModelNode, "children");
    QVERIFY(!rootModelNode.hasProperty("data"));
    QCOMPARE(childNode2.parentProperty().parentModelNode(), rootModelNode);
    QVERIFY(rootModelNode.hasProperty("children"));
    childNode2.setParentProperty(rootModelNode, "data");
    childNode1.setParentProperty(rootModelNode, "data");
    QCOMPARE(childNode2.parentProperty().parentModelNode(), rootModelNode);
    QVERIFY(!rootModelNode.hasProperty("children"));
    QVERIFY(rootModelNode.hasProperty("data"));

    QVERIFY(childNode2.isValid());
    QVERIFY(childNode2.parentProperty().parentModelNode().isValid());
    QCOMPARE(childNode2.parentProperty().parentModelNode(), rootModelNode);

    childNode1.setParentProperty(rootModelNode, "children");
    rootModelNode.removeProperty("data");
    QVERIFY(!childNode2.isValid());
    QVERIFY(!rootModelNode.hasProperty("data"));

    QVERIFY(childNode1.isValid());
    rootModelNode.nodeProperty("front").setModelNode(childNode1);
    QVERIFY(rootModelNode.hasProperty("front"));
    QCOMPARE(childNode1.parentProperty().parentModelNode(), rootModelNode);
    rootModelNode.removeProperty("front");
    QVERIFY(!childNode1.isValid());
}

void TestCore::testModelResolveIds()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootNode = view->rootModelNode();
    rootNode.setId("rootNode");

    ModelNode childNode1(rootNode.addChildNode("Qt/Rectangle", 4, 6, "children"));

    ModelNode childNode2(childNode1.addChildNode("Qt/Rectangle", 4, 6, "children"));
    childNode2.setId("childNode2");
    childNode2.bindingProperty("test").setExpression("parent.parent");

    QCOMPARE(childNode2.bindingProperty("test").resolveToModelNode(), rootNode);
    childNode1.setId("childNode1");
    childNode2.bindingProperty("test").setExpression("childNode1.parent");
    QCOMPARE(childNode2.bindingProperty("test").resolveToModelNode(), rootNode);
    childNode2.bindingProperty("test").setExpression("rootNode");
    QCOMPARE(childNode2.bindingProperty("test").resolveToModelNode(), rootNode);

    ModelNode childNode3(childNode2.addChildNode("Qt/Rectangle", 4, 6, "children"));
    childNode3.setId("childNode3");
    childNode2.nodeProperty("front").setModelNode(childNode3);
    childNode2.bindingProperty("test").setExpression("childNode3.parent");
    QCOMPARE(childNode2.bindingProperty("test").resolveToModelNode(), childNode2);
    childNode2.bindingProperty("test").setExpression("childNode3.parent.parent");
    QCOMPARE(childNode2.bindingProperty("test").resolveToModelNode(), childNode1);
    childNode2.bindingProperty("test").setExpression("childNode3.parent.parent.parent");
    QCOMPARE(childNode2.bindingProperty("test").resolveToModelNode(), rootNode);
    childNode2.bindingProperty("test").setExpression("childNode3");
    QCOMPARE(childNode2.bindingProperty("test").resolveToModelNode(), childNode3);
    childNode2.bindingProperty("test").setExpression("front");
    QCOMPARE(childNode2.bindingProperty("test").resolveToModelNode(), childNode3);
    childNode2.bindingProperty("test").setExpression("back");
    QVERIFY(!childNode2.bindingProperty("test").resolveToModelNode().isValid());
    childNode2.bindingProperty("test").setExpression("childNode3.parent.front");
    QCOMPARE(childNode2.bindingProperty("test").resolveToModelNode(), childNode3);

    childNode2.variantProperty("x") = 10;
    QCOMPARE(childNode2.variantProperty("x").value().toInt(), 10);

    childNode2.bindingProperty("test").setExpression("childNode3.parent.x");
    QVERIFY(childNode2.bindingProperty("test").resolveToProperty().isVariantProperty());
    QCOMPARE(childNode2.bindingProperty("test").resolveToProperty().toVariantProperty().value().toInt(), 10);

    childNode2.bindingProperty("test").setExpression("childNode3.parent.test");
    QVERIFY(childNode2.bindingProperty("test").resolveToProperty().isBindingProperty());
    QCOMPARE(childNode2.bindingProperty("test").resolveToProperty().toBindingProperty().expression(), QString("childNode3.parent.test"));
}

void TestCore::testModelNodeListProperty()
{
    QSKIP("Skip this for the time being", SkipAll);

    //
    // Test NodeListProperty API
    //
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootNode = view->rootModelNode();

    //
    // Item {}
    //
    NodeListProperty rootChildren = rootNode.nodeListProperty("children");
    QVERIFY(!rootNode.hasProperty("children"));
    QVERIFY(rootChildren.isValid());
    QVERIFY(!rootChildren.isNodeListProperty());
    QVERIFY(rootChildren.isEmpty());

    ModelNode rectNode = view->createModelNode("Qt/Rectangle", 4, 6);
    rootChildren.reparentHere(rectNode);

    //
    // Item { children: [ Rectangle {} ] }
    //
    QVERIFY(rootNode.hasProperty("children"));
    QVERIFY(rootChildren.isValid());
    QVERIFY(rootChildren.isNodeListProperty());
    QVERIFY(!rootChildren.isEmpty());

    ModelNode mouseRegionNode = view->createModelNode("Qt/Item", 4, 6);
    NodeListProperty rectChildren = rectNode.nodeListProperty("children");
    rectChildren.reparentHere(mouseRegionNode);

    //
    // Item { children: [ Rectangle { children : [ MouseRegion {} ] } ] }
    //
    QVERIFY(rectNode.hasProperty("children"));
    QVERIFY(rectChildren.isValid());
    QVERIFY(rectChildren.isNodeListProperty());
    QVERIFY(!rectChildren.isEmpty());

    rectNode.destroy();

    //
    // Item { }
    //
    QVERIFY(!rectNode.isValid());
    QVERIFY(!mouseRegionNode.isValid());
    QVERIFY(!rootNode.hasProperty("children"));
    QVERIFY(rootChildren.isValid());
    QVERIFY(!rootChildren.isNodeListProperty());
    QVERIFY(rootChildren.isEmpty());
    QVERIFY(!rectChildren.isValid());
}

void TestCore::testBasicOperationsWithView()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    NodeInstanceView *nodeInstanceView = new NodeInstanceView(model.data());
    model->attachView(nodeInstanceView);

    ModelNode rootModelNode = view->rootModelNode();
    QCOMPARE(rootModelNode.allDirectSubModelNodes().count(), 0);
    NodeInstance rootInstance = nodeInstanceView->instanceForNode(rootModelNode);

    QCOMPARE(rootInstance.size().width(), 0.0);
    QCOMPARE(rootInstance.size().height(), 0.0);

    QVERIFY(rootInstance.isValid());
    QVERIFY(rootInstance.isQmlGraphicsItem());

    QVERIFY(rootModelNode.isValid());

    rootModelNode.variantProperty("width").setValue(10);
    rootModelNode.variantProperty("height").setValue(10);

    QCOMPARE(rootModelNode.variantProperty("width").value().toInt(), 10);
    QCOMPARE(rootModelNode.variantProperty("height").value().toInt(), 10);

    QCOMPARE(rootInstance.size().width(), 10.0);
    QCOMPARE(rootInstance.size().height(), 10.0);

    ModelNode childNode(rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data"));
    ModelNode childNode2(childNode.addChildNode("Qt/Rectangle", 4, 6, "data"));
    QVERIFY(childNode2.parentProperty().parentModelNode() == childNode);

    QVERIFY(childNode.isValid());

    {
        NodeInstance childInstance2 = nodeInstanceView->instanceForNode(childNode2);
        NodeInstance childInstance = nodeInstanceView->instanceForNode(childNode);

        QVERIFY(childInstance.isValid());
        QVERIFY(childInstance.isQmlGraphicsItem());
        QVERIFY(childInstance2.testHandle()->parent() == childInstance.testHandle());
        QVERIFY(childInstance.testHandle()->parent() == rootInstance.testHandle());
        QCOMPARE(childInstance.size().width(), 0.0);
        QCOMPARE(childInstance.size().height(), 0.0);


        childNode.variantProperty("width").setValue(100);
        childNode.variantProperty("height").setValue(100);

        QCOMPARE(childNode.variantProperty("width").value().toInt(), 100);
        QCOMPARE(childNode.variantProperty("height").value().toInt(), 100);

        QCOMPARE(childInstance.size().width(), 100.0);
        QCOMPARE(childInstance.size().height(), 100.0);

        childNode.destroy();
        QVERIFY(!childNode.isValid());
        QVERIFY(!childNode2.isValid());
        QVERIFY(childInstance.testHandle() == 0);
        QVERIFY(childInstance2.testHandle() == 0);
        QVERIFY(!childInstance.isValid());
        QVERIFY(!childInstance2.isValid());
    }

    childNode = rootModelNode.addChildNode("Qt/Image", 4, 6, "data");
    QVERIFY(childNode.isValid());
    QCOMPARE(childNode.type(), QString("Qt/Image"));
    childNode2 = childNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    QVERIFY(childNode2.isValid());
    childNode2.setParentProperty(rootModelNode, "data");
    QVERIFY(childNode2.isValid());

    {
        NodeInstance childInstance2 = nodeInstanceView->instanceForNode(childNode2);
        NodeInstance childInstance = nodeInstanceView->instanceForNode(childNode);

        QVERIFY(childInstance.isValid());
        QVERIFY(childInstance.isQmlGraphicsItem());
        QVERIFY(childInstance2.testHandle()->parent() == rootInstance.testHandle());
        QVERIFY(childInstance.testHandle()->parent() == rootInstance.testHandle());
        QCOMPARE(childInstance.size().width(), 0.0);
        QCOMPARE(childInstance.size().height(), 0.0);

        QCOMPARE(rootModelNode, childNode2.parentProperty().parentModelNode());

        childNode.variantProperty("width").setValue(20);
        QCOMPARE(childInstance.property("width").toInt(), 20);

        QCOMPARE(childNode.variantProperty("width").value().toInt(), 20);
        childNode.removeProperty("width");
        QVERIFY(!childNode.hasProperty("width"));

        rootModelNode.removeProperty("data");
        QVERIFY(!childNode.isValid());
        QVERIFY(!childNode2.isValid());
        QVERIFY(childInstance.testHandle() == 0);
        QVERIFY(childInstance2.testHandle() == 0);
        QVERIFY(!childInstance.isValid());
        QVERIFY(!childInstance2.isValid());
    }

    model->detachView(nodeInstanceView);
}

void TestCore::testQmlModelView()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QmlModelView *view = new TestView(model.data());
    QVERIFY(view);
    model->attachView(view);
    QVERIFY(view->model());
    QVERIFY(view->rootQmlObjectNode().isValid());
    QVERIFY(!view->rootQmlObjectNode().hasNodeParent());
    QVERIFY(!view->rootQmlObjectNode().hasInstanceParent());
    QVERIFY(view->rootQmlObjectNode().isRootModelNode());

    PropertyListType propertyList;
    propertyList.append(qMakePair(QString("x"), QVariant(20)));
    propertyList.append(qMakePair(QString("y"), QVariant(20)));
    propertyList.append(qMakePair(QString("width"), QVariant(20)));
    propertyList.append(qMakePair(QString("height"), QVariant(20)));

    QmlObjectNode node1 = view->createQmlObjectNode("Qt/Rectangle", 4, 6, propertyList);

    QVERIFY(node1.isValid());
    QVERIFY(!node1.hasNodeParent());
    QVERIFY(!node1.hasInstanceParent());

    QCOMPARE(node1.instanceValue("x").toInt(), 20);
    QCOMPARE(node1.instanceValue("y").toInt(), 20);

    node1.setParentProperty(view->rootQmlObjectNode().nodeAbstractProperty("children"));

    QVERIFY(node1.hasNodeParent());
    QVERIFY(node1.hasInstanceParent());
    QVERIFY(node1.instanceParent() == view->rootQmlObjectNode());


    QmlObjectNode node2 = view->createQmlObjectNode("Qt/Rectangle", 4, 6, propertyList);

    QVERIFY(node2.isValid());
    QVERIFY(!node2.hasNodeParent());
    QVERIFY(!node2.hasInstanceParent());

    node2.setParentProperty(view->rootQmlObjectNode().nodeAbstractProperty("children"));

    QVERIFY(node2.hasNodeParent());
    QVERIFY(node2.hasInstanceParent());
    QVERIFY(node2.instanceParent() == view->rootQmlObjectNode());


    node2.setParentProperty(node1.nodeAbstractProperty("children"));

    QVERIFY(node2.hasNodeParent());
    QVERIFY(node2.hasInstanceParent());
    QVERIFY(node2.instanceParent() == node1);

        node2.setParentProperty(view->rootQmlObjectNode().nodeAbstractProperty("children"));

    QCOMPARE(node1.instanceValue("x").toInt(), 20);

    node1.setVariantProperty("x", 2);

    QCOMPARE(node1.instanceValue("x").toInt(), 2);


    QmlObjectNode node3 = view->createQmlObjectNode("Qt/Rectangle", 4, 6, propertyList);
    QmlObjectNode node4 = view->createQmlObjectNode("Qt/Rectangle", 4, 6, propertyList);
    QmlObjectNode node5 = view->createQmlObjectNode("Qt/Rectangle", 4, 6, propertyList);
    QmlObjectNode node6 = view->createQmlObjectNode("Qt/Rectangle", 4, 6, propertyList);
    QmlObjectNode node7 = view->createQmlObjectNode("Qt/Rectangle", 4, 6, propertyList);
    QmlObjectNode node8 = view->createQmlObjectNode("Qt/Rectangle", 4, 6, propertyList);

    node3.setParentProperty(node2.nodeAbstractProperty("children"));
    node4.setParentProperty(node3.nodeAbstractProperty("children"));
    node5.setParentProperty(node4.nodeAbstractProperty("children"));
    node6.setParentProperty(node5.nodeAbstractProperty("children"));
    node7.setParentProperty(node6.nodeAbstractProperty("children"));
    node8.setParentProperty(node7.nodeAbstractProperty("children"));

    QCOMPARE(node2.nodeAbstractProperty("children").allSubNodes().count(), 6);
    QVERIFY(node8.isValid());
    node3.destroy();
    QVERIFY(!node8.isValid());
    QCOMPARE(node2.nodeAbstractProperty("children").allSubNodes().count(), 0);

    node1.setId("node1");

    node2.setVariantProperty("x", 20);
    node2.setBindingProperty("x", "node1.x");
    QCOMPARE(node2.instanceValue("x").toInt(), 2);
    node1.setVariantProperty("x", 10);
    QCOMPARE(node2.instanceValue("x").toInt(), 10);

    node1.destroy();
    QVERIFY(!node1.isValid());

    QCOMPARE(node2.instanceValue("x").toInt(), 10); // is this right? or should it be a invalid qvariant?

    node1 = view->createQmlObjectNode("Qt/Rectangle", 4, 6, propertyList);
    node1.setId("node1");

    QCOMPARE(node2.instanceValue("x").toInt(), 20);

    node3 = view->createQmlObjectNode("Qt/Rectangle", 4, 6, propertyList);
    node3.setParentProperty(node2.nodeAbstractProperty("children"));
    QCOMPARE(node3.instanceValue("width").toInt(), 20);
    node3.setVariantProperty("width", 0);
    QCOMPARE(node3.instanceValue("width").toInt(), 0);

    QCOMPARE(node3.instanceValue("x").toInt(), 20);
    QVERIFY(!QmlMetaType::toQObject(node3.instanceValue("anchors.fill")));
    node3.setBindingProperty("anchors.fill", "parent");
    QVERIFY(QmlMetaType::toQObject(node3.instanceValue("anchors.fill")));
    QCOMPARE(node3.instanceValue("x").toInt(), 0);
    QCOMPARE(node3.instanceValue("width").toInt(), 20);

    node3.setParentProperty(node1.nodeAbstractProperty("children"));
    node1.setVariantProperty("width", 50);
    node2.setVariantProperty("width", 100);
    QCOMPARE(node3.instanceValue("width").toInt(), 50);

}

void TestCore::testModelCreateInvalidSubNode()
{
    QSKIP("type checking not in the model", SkipAll);
    ModelNode node;
    QVERIFY(!node.isValid());
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    try {
        ModelNode invalidChildNode = view->rootModelNode().addChildNode("InvalidNode", 0, 0, "data");
        QFAIL("Adding an invalid typed node should result in an exception");
    } catch (Exception& exception) {
        QCOMPARE(exception.type(), QString("InvalidModelNodeException"));
    }
}

void TestCore::testModelRemoveNode()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    NodeInstanceView *nodeInstanceView = new NodeInstanceView(model.data());
    model->attachView(nodeInstanceView);

    QCOMPARE(view->rootModelNode().allDirectSubModelNodes().count(), 0);


    ModelNode childNode = view->rootModelNode().addChildNode("Qt/Rectangle", 4, 6, "data");
    QVERIFY(childNode.isValid());
    QCOMPARE(view->rootModelNode().allDirectSubModelNodes().count(), 1);
    QVERIFY(view->rootModelNode().allDirectSubModelNodes().contains(childNode));
    QVERIFY(childNode.parentProperty().parentModelNode() == view->rootModelNode());

    {
        NodeInstance childInstance = nodeInstanceView->instanceForNode(childNode);
        QVERIFY(childInstance.isValid());
        QVERIFY(childInstance.parent() == nodeInstanceView->instanceForNode(view->rootModelNode()));
    }

    ModelNode subChildNode = childNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    QVERIFY(subChildNode.isValid());
    QCOMPARE(childNode.allDirectSubModelNodes().count(), 1);
    QVERIFY(childNode.allDirectSubModelNodes().contains(subChildNode));
    QVERIFY(subChildNode.parentProperty().parentModelNode() == childNode);

    {
        NodeInstance subChildInstance = nodeInstanceView->instanceForNode(subChildNode);
        QVERIFY(subChildInstance.isValid());
        QVERIFY(subChildInstance.parent() == nodeInstanceView->instanceForNode(childNode));
    }

    childNode.destroy();

    QVERIFY(!nodeInstanceView->hasInstanceForNode(childNode));

    QCOMPARE(view->rootModelNode().allDirectSubModelNodes().count(), 0);
    QVERIFY(!view->rootModelNode().allDirectSubModelNodes().contains(childNode));
    QVERIFY(!childNode.isValid());
    QVERIFY(!subChildNode.isValid());

    try {
        view->rootModelNode().destroy();
        QFAIL("remove the rootModelNode should be throw a exception");
    } catch (Exception &exception) {
        QCOMPARE(exception.type(), QString("InvalidArgumentException"));
    }

    QVERIFY(view->rootModelNode().isValid());

    // delete node not in hierarchy
    childNode = view->createModelNode("Qt/Item", 4, 6);
    childNode.destroy();

    model->detachView(nodeInstanceView);
}

void TestCore::reparentingNode()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode = view->rootModelNode();
    QVERIFY(rootModelNode.isValid());
    rootModelNode.setId("rootModelNode");
    QCOMPARE(rootModelNode.id(), QString("rootModelNode"));


    NodeInstanceView *nodeInstanceView = new NodeInstanceView(model.data());
    model->attachView(nodeInstanceView);

    ModelNode childNode = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    QCOMPARE(childNode.parentProperty().parentModelNode(), rootModelNode);
    QVERIFY(rootModelNode.allDirectSubModelNodes().contains(childNode));

    {
        NodeInstance childInstance = nodeInstanceView->instanceForNode(childNode);
        QVERIFY(childInstance.isValid());
        QVERIFY(childInstance.parent() == nodeInstanceView->instanceForNode(view->rootModelNode()));
    }

    ModelNode childNode2 = rootModelNode.addChildNode("Qt/Item", 4, 6, "data");
    QCOMPARE(childNode2.parentProperty().parentModelNode(), rootModelNode);
    QVERIFY(rootModelNode.allDirectSubModelNodes().contains(childNode2));

    {
        NodeInstance childIstance2 = nodeInstanceView->instanceForNode(childNode2);
        QVERIFY(childIstance2.isValid());
        QVERIFY(childIstance2.parent() == nodeInstanceView->instanceForNode(view->rootModelNode()));
    }

    childNode.setParentProperty(childNode2, "data");

    QCOMPARE(childNode.parentProperty().parentModelNode(), childNode2);
    QVERIFY(childNode2.allDirectSubModelNodes().contains(childNode));
    QVERIFY(!rootModelNode.allDirectSubModelNodes().contains(childNode));
    QVERIFY(rootModelNode.allDirectSubModelNodes().contains(childNode2));

    {
        NodeInstance childIstance = nodeInstanceView->instanceForNode(childNode);
        QVERIFY(childIstance.isValid());
        QVERIFY(childIstance.parent() == nodeInstanceView->instanceForNode(childNode2));
    }

    childNode2.setParentProperty(rootModelNode, "data");
    QCOMPARE(childNode2.parentProperty().parentModelNode(), rootModelNode);
    QVERIFY(rootModelNode.allDirectSubModelNodes().contains(childNode2));

    {
        NodeInstance childIstance2 = nodeInstanceView->instanceForNode(childNode2);
        QVERIFY(childIstance2.isValid());
        QVERIFY(childIstance2.parent() == nodeInstanceView->instanceForNode(rootModelNode));
    }

    QCOMPARE(childNode.parentProperty().parentModelNode(), childNode2);

    model->detachView(nodeInstanceView);
}

void TestCore::reparentingNodeLikeDragAndDrop()
{
    QPlainTextEdit textEdit;
    textEdit.setPlainText("import Qt 4.6;\n\nItem {\n}\n");
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    NodeInstanceView *nodeInstanceView = new NodeInstanceView(model.data());
    model->attachView(nodeInstanceView);
    view->rootModelNode().setId("rootModelNode");
    QCOMPARE(view->rootModelNode().id(), QString("rootModelNode"));

    ModelNode rectNode = view->rootModelNode().addChildNode("Qt/Rectangle", 4, 6, "data");
    rectNode.setId("Rect_1");
    rectNode.variantProperty("x").setValue(20);
    rectNode.variantProperty("y").setValue(30);
    rectNode.variantProperty("width").setValue(40);
    rectNode.variantProperty("height").setValue(50);

    RewriterTransaction transaction(view->beginRewriterTransaction());

    ModelNode textNode = view->rootModelNode().addChildNode("Qt/Text", 4, 6, "data");
    QCOMPARE(textNode.parentProperty().parentModelNode(), view->rootModelNode());
    QVERIFY(view->rootModelNode().allDirectSubModelNodes().contains(textNode));

    textNode.setId("Text_1");
    textNode.variantProperty("x").setValue(10);
    textNode.variantProperty("y").setValue(10);
    textNode.variantProperty("width").setValue(50);
    textNode.variantProperty("height").setValue(20);

    textNode.variantProperty("x").setValue(30);
    textNode.variantProperty("y").setValue(30);

    {
        NodeInstance textInstance = nodeInstanceView->instanceForNode(textNode);
        QVERIFY(textInstance.isValid());
        QVERIFY(textInstance.parent() == nodeInstanceView->instanceForNode(view->rootModelNode()));
        QCOMPARE(textInstance.position().x(), 30.0);
        QCOMPARE(textInstance.position().y(), 30.0);
        QCOMPARE(textInstance.size().width(), 50.0);
        QCOMPARE(textInstance.size().height(), 20.0);
    }

    textNode.setParentProperty(rectNode, "data");
    QCOMPARE(textNode.parentProperty().parentModelNode(), rectNode);
    QVERIFY(rectNode.allDirectSubModelNodes().contains(textNode));

    {
        NodeInstance textInstance = nodeInstanceView->instanceForNode(textNode);
        QVERIFY(textInstance.isValid());
        QVERIFY(textInstance.parent() == nodeInstanceView->instanceForNode(rectNode));
        QCOMPARE(textInstance.position().x(), 30.0);
        QCOMPARE(textInstance.position().y(), 30.0);
        QCOMPARE(textInstance.size().width(), 50.0);
        QCOMPARE(textInstance.size().height(), 20.0);
    }

    textNode.setParentProperty(view->rootModelNode(), "data");
    QCOMPARE(textNode.parentProperty().parentModelNode(), view->rootModelNode());
    QVERIFY(view->rootModelNode().allDirectSubModelNodes().contains(textNode));

    {
        NodeInstance textInstance = nodeInstanceView->instanceForNode(textNode);
        QVERIFY(textInstance.isValid());
        QVERIFY(textInstance.parent() == nodeInstanceView->instanceForNode(view->rootModelNode()));
        QCOMPARE(textInstance.position().x(), 30.0);
        QCOMPARE(textInstance.position().y(), 30.0);
        QCOMPARE(textInstance.size().width(), 50.0);
        QCOMPARE(textInstance.size().height(), 20.0);
    }

    textNode.setParentProperty(rectNode, "data");
    QCOMPARE(textNode.parentProperty().parentModelNode(), rectNode);
    QVERIFY(rectNode.allDirectSubModelNodes().contains(textNode));

    {
        NodeInstance textInstance = nodeInstanceView->instanceForNode(textNode);
        QVERIFY(textInstance.isValid());
        QVERIFY(textInstance.parent() == nodeInstanceView->instanceForNode(rectNode));
        QCOMPARE(textInstance.position().x(), 30.0);
        QCOMPARE(textInstance.position().y(), 30.0);
        QCOMPARE(textInstance.size().width(), 50.0);
        QCOMPARE(textInstance.size().height(), 20.0);
    }

    {
        NodeInstance textInstance = nodeInstanceView->instanceForNode(textNode);
        QVERIFY(textInstance.isValid());
        QVERIFY(textInstance.parent() == nodeInstanceView->instanceForNode(rectNode));
        QCOMPARE(textInstance.position().x(), 30.0);
        QCOMPARE(textInstance.position().y(), 30.0);
        QCOMPARE(textInstance.size().width(), 50.0);
        QCOMPARE(textInstance.size().height(), 20.0);
    }

    transaction.commit();

    if(!textNode.isValid())
        QFAIL("textnode node is not valid anymore");
    else
        QCOMPARE(textNode.parentProperty().parentModelNode(), rectNode);
    QVERIFY(rectNode.allDirectSubModelNodes().contains(textNode));

    model->detachView(nodeInstanceView);
}

void TestCore::testModelReorderSiblings()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    NodeInstanceView *nodeInstanceView = new NodeInstanceView(model.data());
    model->attachView(nodeInstanceView);

    ModelNode rootModelNode = view->rootModelNode();
    QVERIFY(rootModelNode.isValid());

    ModelNode a = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    QVERIFY(a.isValid());
    ModelNode b = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    QVERIFY(b.isValid());
    ModelNode c = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    QVERIFY(c.isValid());

    {
        QVERIFY(nodeInstanceView->instanceForNode(a).parent() == nodeInstanceView->instanceForNode(rootModelNode));
        QVERIFY(nodeInstanceView->instanceForNode(b).parent() == nodeInstanceView->instanceForNode(rootModelNode));
        QVERIFY(nodeInstanceView->instanceForNode(c).parent() == nodeInstanceView->instanceForNode(rootModelNode));
    }

    NodeListProperty listProperty(rootModelNode.nodeListProperty("data"));

    listProperty.slide(listProperty.toModelNodeList().indexOf(a), 2); //a.slideToIndex(2);

    QVERIFY(a.isValid()); QCOMPARE(listProperty.toModelNodeList().indexOf(a), 2);
    QVERIFY(b.isValid()); QCOMPARE(listProperty.toModelNodeList().indexOf(b), 0);
    QVERIFY(c.isValid()); QCOMPARE(listProperty.toModelNodeList().indexOf(c), 1);

    listProperty.slide(listProperty.toModelNodeList().indexOf(c), 0); //c.slideToIndex(0);

    QVERIFY(a.isValid()); QCOMPARE(listProperty.toModelNodeList().indexOf(a), 2);
    QVERIFY(b.isValid()); QCOMPARE(listProperty.toModelNodeList().indexOf(b), 1);
    QVERIFY(c.isValid()); QCOMPARE(listProperty.toModelNodeList().indexOf(c), 0);

    {
        QVERIFY(nodeInstanceView->instanceForNode(a).parent() == nodeInstanceView->instanceForNode(rootModelNode));
        QVERIFY(nodeInstanceView->instanceForNode(b).parent() == nodeInstanceView->instanceForNode(rootModelNode));
        QVERIFY(nodeInstanceView->instanceForNode(c).parent() == nodeInstanceView->instanceForNode(rootModelNode));
    }

    model->detachView(nodeInstanceView);
}

void TestCore::testModelRootNode()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    try {
        ModelNode rootModelNode = view->rootModelNode();
        QVERIFY(rootModelNode.isValid());
        QVERIFY(rootModelNode.isRootNode());
        ModelNode topChildNode = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
        QVERIFY(topChildNode.isValid());
        QVERIFY(rootModelNode.isRootNode());
        ModelNode childNode = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
        QVERIFY(childNode.isValid());
        QVERIFY(rootModelNode.isValid());
        QVERIFY(rootModelNode.isRootNode());
        childNode.setParentProperty(topChildNode, "data");
        QVERIFY(topChildNode.isValid());
        QVERIFY(childNode.isValid());
        QVERIFY(rootModelNode.isValid());
        QVERIFY(rootModelNode.isRootNode());
    } catch (const QmlDesigner::Exception &exception) {
        QString errorMsg = tr("Exception: %1 %2 %3:%4").arg(exception.type(), exception.function(), exception.file()).arg(exception.line());
        QFAIL(errorMsg.toLatin1().constData());
    }
}

void TestCore::reparentingNodeInModificationGroup()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode childNode = view->rootModelNode().addChildNode("Qt/Rectangle", 4, 6, "data");
    ModelNode childNode2 = view->rootModelNode().addChildNode("Qt/Item", 4, 6, "data");
    childNode.variantProperty("x").setValue(10);
    childNode.variantProperty("y").setValue(10);

    QCOMPARE(childNode.parentProperty().parentModelNode(), view->rootModelNode());
    QCOMPARE(childNode2.parentProperty().parentModelNode(), view->rootModelNode());
    QVERIFY(view->rootModelNode().allDirectSubModelNodes().contains(childNode));
    QVERIFY(view->rootModelNode().allDirectSubModelNodes().contains(childNode2));

//    ModificationGroupToken token = model->beginModificationGroup();
    childNode.variantProperty("x").setValue(20);
    childNode.variantProperty("y").setValue(20);
    childNode.setParentProperty(childNode2, "data");
    childNode.variantProperty("x").setValue(30);
    childNode.variantProperty("y").setValue(30);
    childNode.variantProperty("x").setValue(1000000);
    childNode.variantProperty("y").setValue(1000000);
//    model->endModificationGroup(token);

    QCOMPARE(childNode.parentProperty().parentModelNode(), childNode2);
    QVERIFY(childNode2.allDirectSubModelNodes().contains(childNode));
    QVERIFY(!view->rootModelNode().allDirectSubModelNodes().contains(childNode));
    QVERIFY(view->rootModelNode().allDirectSubModelNodes().contains(childNode2));

    childNode.setParentProperty(view->rootModelNode(), "data");
    QVERIFY(childNode.isValid());
    QCOMPARE(childNode2.parentProperty().parentModelNode(), view->rootModelNode());

    childNode2.setParentProperty(childNode, "data");
    QCOMPARE(childNode2.parentProperty().parentModelNode(), childNode);
    QVERIFY(childNode2.isValid());
    QVERIFY(childNode.allDirectSubModelNodes().contains(childNode2));

    QCOMPARE(childNode2.parentProperty().parentModelNode(), childNode);
    childNode2.setParentProperty(view->rootModelNode(), "data");
    QCOMPARE(childNode2.parentProperty().parentModelNode(), view->rootModelNode());
    QVERIFY(childNode2.isValid());
    QVERIFY(view->rootModelNode().allDirectSubModelNodes().contains(childNode2));
}

void TestCore::testModelAddAndRemoveProperty()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode node = view->rootModelNode();
    QVERIFY(node.isValid());

    NodeInstanceView *nodeInstanceView = new NodeInstanceView(model.data());
    model->attachView(nodeInstanceView);

    node.variantProperty("blah").setValue(-1);
    QCOMPARE(node.variantProperty("blah").value().toInt(), -1);
    QVERIFY(node.hasProperty("blah"));
    QCOMPARE(node.variantProperty("blah").value().toInt(), -1);

    {
        NodeInstance nodeInstance = nodeInstanceView->instanceForNode(node);
//        QCOMPARE(nodeInstance.property("blah").toInt(), -1);
    }

    node.variantProperty("customValue").setValue(42);
    QCOMPARE(node.variantProperty("customValue").value().toInt(), 42);

    {
        NodeInstance nodeInstance = nodeInstanceView->instanceForNode(node);
//        QCOMPARE(!nodeInstance.property("customValue").toInt(), 42);
    }

    node.variantProperty("x").setValue(42);
    QCOMPARE(node.variantProperty("x").value().toInt(), 42);

    {
        NodeInstance nodeInstance = nodeInstanceView->instanceForNode(node);
        QCOMPARE(nodeInstance.property("x").toInt(), 42);
    }

    node.removeProperty("customValue");
    QVERIFY(!node.hasProperty("customValue"));

    node.variantProperty("foo").setValue("bar");
    QVERIFY(node.hasProperty("foo"));
    QCOMPARE(node.variantProperty("foo").value().toString(), QString("bar"));

    model->detachView(nodeInstanceView);
}

void TestCore::testModelViewNotification()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view1(new TestView);
    QVERIFY(view1.data());

    QScopedPointer<TestView> view2(new TestView);
    QVERIFY(view2.data());

    model->attachView(view2.data());
    model->attachView(view1.data());

    QCOMPARE(view1->methodCalls().size(), 1);
    QCOMPARE(view1->methodCalls().at(0).name,QString("modelAttached"));
    QCOMPARE(view2->methodCalls().size(), 1);
    QCOMPARE(view2->methodCalls().at(0).name,QString("modelAttached"));

    QList<TestView::MethodCall> expectedCalls;
    expectedCalls << TestView::MethodCall("modelAttached", QStringList() << QString::number(reinterpret_cast<long>(model.data())));
    QCOMPARE(view1->methodCalls(), expectedCalls);
    QCOMPARE(view2->methodCalls(), expectedCalls);

    ModelNode childNode = view2->rootModelNode().addChildNode("Qt/Rectangle", 4, 6, "data");
    expectedCalls << TestView::MethodCall("nodeCreated", QStringList() << "");
    expectedCalls << TestView::MethodCall("nodeReparented", QStringList() << "" << "data" << "" << "PropertiesAdded");
    QCOMPARE(view1->methodCalls(), expectedCalls);
    QCOMPARE(view2->methodCalls(), expectedCalls);

    childNode.setId("supernode");
    expectedCalls << TestView::MethodCall("nodeIdChanged", QStringList() << "supernode" << "supernode" << "");
    QCOMPARE(view1->methodCalls(), expectedCalls);
    QCOMPARE(view2->methodCalls(), expectedCalls);

    childNode.variantProperty("visible").setValue(false);
    expectedCalls << TestView::MethodCall("variantPropertiesChanged", QStringList() << "visible" << "PropertiesAdded");
    QCOMPARE(view1->methodCalls(), expectedCalls);
    QCOMPARE(view2->methodCalls(), expectedCalls);

    childNode.setId("supernode2");
    expectedCalls << TestView::MethodCall("nodeIdChanged", QStringList() << "supernode2" << "supernode2" << "supernode");
    QCOMPARE(view1->methodCalls(), expectedCalls);
    QCOMPARE(view2->methodCalls(), expectedCalls);

    childNode.variantProperty("visible").setValue(false); // its tae some value so no notification should be sent
    QCOMPARE(view1->methodCalls(), expectedCalls);
    QCOMPARE(view2->methodCalls(), expectedCalls);

    childNode.variantProperty("visible").setValue(true);
    expectedCalls << TestView::MethodCall("variantPropertiesChanged", QStringList() << "visible" << "");
    QCOMPARE(view1->methodCalls(), expectedCalls);
    QCOMPARE(view2->methodCalls(), expectedCalls);

    childNode.bindingProperty("visible").setExpression("false");
    expectedCalls << TestView::MethodCall("propertiesAboutToBeRemoved", QStringList() << "visible");
    expectedCalls << TestView::MethodCall("bindingPropertiesChanged", QStringList() << "visible" << "PropertiesAdded");
    QCOMPARE(view1->methodCalls(), expectedCalls);
    QCOMPARE(view2->methodCalls(), expectedCalls);

    childNode.destroy();
    expectedCalls << TestView::MethodCall("nodeAboutToBeRemoved", QStringList() << "supernode2");
    expectedCalls << TestView::MethodCall("nodeRemoved", QStringList() << QString() << "data" << "EmptyPropertiesRemoved");
    QCOMPARE(view1->methodCalls(), expectedCalls);
    QCOMPARE(view2->methodCalls(), expectedCalls);

    model->detachView(view1.data());
    expectedCalls << TestView::MethodCall("modelAboutToBeDetached", QStringList() << QString::number(reinterpret_cast<long>(model.data())));
    QCOMPARE(view1->methodCalls(), expectedCalls);
}


void TestCore::testRewriterTransaction()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    RewriterTransaction transaction = view->beginRewriterTransaction();
    QVERIFY(transaction.isValid());

    ModelNode childNode = view->rootModelNode().addChildNode("Qt/Rectangle", 4, 6, "data");
    QVERIFY(childNode.isValid());

    childNode.destroy();
    QVERIFY(!childNode.isValid());

    {
        RewriterTransaction transaction2 = view->beginRewriterTransaction();
        QVERIFY(transaction2.isValid());

        ModelNode childNode = view->rootModelNode().addChildNode("Qt/Rectangle", 4, 6, "data");
        QVERIFY(childNode.isValid());

        childNode.destroy();
        QVERIFY(!childNode.isValid());

        RewriterTransaction transaction3(transaction2);
        QVERIFY(!transaction2.isValid());
        QVERIFY(transaction3.isValid());

        transaction2 = transaction3;
        QVERIFY(transaction2.isValid());
        QVERIFY(!transaction3.isValid());

        transaction3 = transaction3;
        transaction2 = transaction2;
        QVERIFY(transaction2.isValid());
        QVERIFY(!transaction3.isValid());
    }

    QVERIFY(transaction.isValid());
    transaction.commit();
    QVERIFY(!transaction.isValid());
}

void TestCore::testRewriterId()
{
    char qmlString[] = "import Qt 4.6\n"
                       "Rectangle {\n"
                       "}\n";

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);


    model->attachView(testRewriterView.data());
    QCOMPARE(rootModelNode.type(), QString("Qt/Rectangle"));

    QVERIFY(rootModelNode.isValid());

    ModelNode newNode(view->createModelNode("Qt/Rectangle", 4, 6));
    newNode.setId("testId");

    rootModelNode.nodeListProperty("data").reparentHere(newNode);

    const QLatin1String expected("import Qt 4.6\n"
                                  "Rectangle {\n"
                                  "    Rectangle {\n"
                                  "        id: testId\n"
                                  "    }\n"
                                  "}\n");

    QCOMPARE(textEdit.toPlainText(), expected);
}

void TestCore::testRewriterNodeReparentingTransaction1()
{
     char qmlString[] = "import Qt 4.6\n"
                       "Rectangle {\n"
                       "}\n";

//    QSKIP("another asserting test", SkipAll);

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.type(), QString("Qt/Item"));
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);

    model->attachView(testRewriterView.data());

    QVERIFY(rootModelNode.isValid());

    ModelNode childNode1 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    ModelNode childNode2 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    ModelNode childNode3 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    ModelNode childNode4 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");

    ModelNode reparentNode = childNode1.addChildNode("Qt/Rectangle", 4, 6, "data");

    RewriterTransaction rewriterTransaction = view->beginRewriterTransaction();

    childNode2.nodeListProperty("data").reparentHere(reparentNode);
    childNode3.nodeListProperty("data").reparentHere(reparentNode);
    childNode4.nodeListProperty("data").reparentHere(reparentNode);

    reparentNode.destroy();

    rewriterTransaction.commit();
}

void TestCore::testRewriterNodeReparentingTransaction2()
{
     char qmlString[] = "import Qt 4.6\n"
                       "Rectangle {\n"
                       "}\n";

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.type(), QString("Qt/Item"));
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);

    model->attachView(testRewriterView.data());

    QVERIFY(rootModelNode.isValid());

    ModelNode childNode1 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    ModelNode childNode2 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");

    childNode2.variantProperty("x") = 200;
    childNode2.variantProperty("y") = 50;
    childNode2.variantProperty("color") = QColor(Qt::red);
    childNode2.setId("childNode2");

    childNode1.variantProperty("x") = 100;
    childNode1.variantProperty("y") = 10;
    childNode1.variantProperty("color") = QColor(Qt::blue);
    childNode1.setId("childNode1");

    RewriterTransaction rewriterTransaction = view->beginRewriterTransaction();

    childNode1.nodeListProperty("data").reparentHere(childNode2);
    childNode2.variantProperty("x") = 300;
    childNode2.variantProperty("y") = 150;

    rewriterTransaction.commit();

    rewriterTransaction = view->beginRewriterTransaction();

    rootModelNode.nodeListProperty("data").reparentHere(childNode2);
    childNode2.variantProperty("x") = 100;
    childNode2.variantProperty("y") = 200;

    rewriterTransaction.commit();

    rewriterTransaction = view->beginRewriterTransaction();

    rootModelNode.nodeListProperty("data").reparentHere(childNode2);
    childNode2.variantProperty("x") = 150;
    childNode2.variantProperty("y") = 250;
    childNode1.nodeListProperty("data").reparentHere(childNode2);

    rewriterTransaction.commit();
}

void TestCore::testRewriterNodeReparentingTransaction3()
{
    char qmlString[] = "import Qt 4.6\n"
                      "Rectangle {\n"
                      "}\n";

   QPlainTextEdit textEdit;
   textEdit.setPlainText(qmlString);
   PlainTextEditModifier textModifier(&textEdit);

   QScopedPointer<Model> model(Model::create("Qt/Item"));
   QVERIFY(model.data());

   QScopedPointer<TestView> view(new TestView);
   QVERIFY(view.data());
   model->attachView(view.data());

   ModelNode rootModelNode(view->rootModelNode());
   QVERIFY(rootModelNode.isValid());
   QCOMPARE(rootModelNode.type(), QString("Qt/Item"));
   QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
   testRewriterView->setTextModifier(&textModifier);

   model->attachView(testRewriterView.data());

   QVERIFY(rootModelNode.isValid());

   ModelNode childNode1 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
   ModelNode childNode2 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
   ModelNode childNode3 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
   ModelNode childNode4 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");

   RewriterTransaction rewriterTransaction = view->beginRewriterTransaction();

   childNode1.nodeListProperty("data").reparentHere(childNode4);
   childNode4.variantProperty("x") = 151;
   childNode4.variantProperty("y") = 251;
   childNode2.nodeListProperty("data").reparentHere(childNode4);
   childNode4.variantProperty("x") = 152;
   childNode4.variantProperty("y") = 252;
   childNode3.nodeListProperty("data").reparentHere(childNode4);
   childNode4.variantProperty("x") = 153;
   childNode4.variantProperty("y") = 253;
   rootModelNode.nodeListProperty("data").reparentHere(childNode4);
   childNode4.variantProperty("x") = 154;
   childNode4.variantProperty("y") = 254;

   rewriterTransaction.commit();
}

void TestCore::testRewriterNodeReparentingTransaction4()
{
    char qmlString[] = "import Qt 4.6\n"
                      "Rectangle {\n"
                      "}\n";

   QPlainTextEdit textEdit;
   textEdit.setPlainText(qmlString);
   PlainTextEditModifier textModifier(&textEdit);

   QScopedPointer<Model> model(Model::create("Qt/Item"));
   QVERIFY(model.data());

   QScopedPointer<TestView> view(new TestView);
   QVERIFY(view.data());
   model->attachView(view.data());

   ModelNode rootModelNode(view->rootModelNode());
   QVERIFY(rootModelNode.isValid());
   QCOMPARE(rootModelNode.type(), QString("Qt/Item"));
   QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
   testRewriterView->setTextModifier(&textModifier);

   model->attachView(testRewriterView.data());

   QVERIFY(rootModelNode.isValid());

   ModelNode childNode1 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
   ModelNode childNode2 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
   ModelNode childNode3 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
   ModelNode childNode4 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
   ModelNode childNode5 = childNode2.addChildNode("Qt/Rectangle", 4, 6, "data");

   RewriterTransaction rewriterTransaction = view->beginRewriterTransaction();

   childNode1.nodeListProperty("data").reparentHere(childNode4);
   childNode4.variantProperty("x") = 151;
   childNode4.variantProperty("y") = 251;
   childNode2.nodeListProperty("data").reparentHere(childNode4);
   childNode4.variantProperty("x") = 152;
   childNode4.variantProperty("y") = 252;
   childNode3.nodeListProperty("data").reparentHere(childNode4);
   childNode4.variantProperty("x") = 153;
   childNode4.variantProperty("y") = 253;
   childNode5.nodeListProperty("data").reparentHere(childNode4);
   childNode4.variantProperty("x") = 154;
   childNode4.variantProperty("y") = 254;

   rewriterTransaction.commit();
}

void TestCore::testRewriterAddNodeTransaction()
{
    char qmlString[] = "import Qt 4.6\n"
                       "Rectangle {\n"
                       "}\n";

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.type(), QString("Qt/Item"));
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);

    model->attachView(testRewriterView.data());

    QVERIFY(rootModelNode.isValid());


    ModelNode childNode = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");

    RewriterTransaction rewriterTransaction = view->beginRewriterTransaction();

    ModelNode newNode = view->createModelNode("Qt/Rectangle", 4, 6);
    newNode.variantProperty("x") = 100;
    newNode.variantProperty("y") = 100;

    rootModelNode.nodeListProperty("data").reparentHere(newNode);
    childNode.nodeListProperty("data").reparentHere(newNode);

    rewriterTransaction.commit();
}

void TestCore::testRewriterComponentId()
{
    char qmlString[] = "import Qt 4.6\n"
        "Rectangle {\n"
        "   Component {\n"
        "       id: testComponent\n"
        "       Item {\n"
        "       }\n"
        "   }\n"
        "}\n";

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.type(), QString("Qt/Rectangle"));

    ModelNode component(rootModelNode.allDirectSubModelNodes().first());
    QVERIFY(component.isValid());
    QCOMPARE(component.type(), QString("Qt/Component"));
    QCOMPARE(component.id(), QString("testComponent"));
}

void TestCore::testRewriterTransactionRewriter()
{
    char qmlString[] = "import Qt 4.6\n"
                       "Rectangle {\n"
                       "}\n";

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.type(), QString("Qt/Item"));
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);

    model->attachView(testRewriterView.data());

    QVERIFY(rootModelNode.isValid());

    ModelNode childNode1;
    ModelNode childNode2;

    {
        RewriterTransaction transaction = view->beginRewriterTransaction();
        childNode1 = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
        childNode1.variantProperty("x") = "10";
        childNode1.variantProperty("y") = "10";
    }

    QVERIFY(childNode1.isValid());
    QCOMPARE(childNode1.variantProperty("x").value(), QVariant(10));
    QCOMPARE(childNode1.variantProperty("y").value(), QVariant(10));


    {
        RewriterTransaction transaction = view->beginRewriterTransaction();
        childNode2 = childNode1.addChildNode("Qt/Rectangle", 4, 6, "data");
        childNode2.destroy();
    }

    QVERIFY(!childNode2.isValid());
    QVERIFY(childNode1.isValid());
}

void TestCore::testRewriterPropertyDeclarations()
{
//    QSKIP("Work in progress. See task https://qtrequirements.europe.nokia.com/browse/BAUHAUS-170",
//          SkipAll);

    //
    // test properties defined in qml
    //
    // [default] property <type> <name>[: defaultValue]
    //
    // where type is (int | bool | double | real | string | url | color | date | var | variant)
    //

    // Unsupported:
    //  property var varProperty2: boolProperty
    //  property var myArray: [ Rectangle {} ]
    //  property var someGradient: Gradient {}

    char qmlString[] = "import Qt 4.6\n"
        "Item {\n"
        "   property int intProperty\n"
        "   property bool boolProperty: true\n"
        "   property var varProperty1\n"
        "   property variant varProperty2: boolProperty\n"
        "   default property url urlProperty\n"
        "   intProperty: 2\n"
        "}\n";

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    //
    // parsing
    //
    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.type(), QString("Qt/Item"));

    QCOMPARE(rootModelNode.properties().size(), 5);

    VariantProperty intProperty = rootModelNode.property(QLatin1String("intProperty")).toVariantProperty();
    QVERIFY(intProperty.isValid());
    QVERIFY(intProperty.isVariantProperty());

    VariantProperty boolProperty = rootModelNode.property(QLatin1String("boolProperty")).toVariantProperty();
    QVERIFY(boolProperty.isValid());
    QVERIFY(boolProperty.isVariantProperty());
    QCOMPARE(boolProperty.value(), QVariant(true));

    VariantProperty varProperty1 = rootModelNode.property(QLatin1String("varProperty1")).toVariantProperty();
    QVERIFY(varProperty1.isValid());
    QVERIFY(varProperty1.isVariantProperty());
    QCOMPARE(varProperty1.value(), QVariant());

    VariantProperty urlProperty = rootModelNode.property(QLatin1String("urlProperty")).toVariantProperty();
    QVERIFY(urlProperty.isValid());
    QVERIFY(urlProperty.isVariantProperty());
    QCOMPARE(urlProperty.value(), QVariant());
}

void TestCore::testRewriterPropertyAliases()
{
    //
    // test property aliases defined in qml, i.e.
    //
    // [default] property alias <name>: <alias reference>
    //
    // where type is (int | bool | double | real | string | url | color | date | var | variant)
    //

    char qmlString[] = "import Qt 4.6\n"
        "Item {\n"
        "   property alias theText: t.text\n"
        "   default alias property yPos: t.y\n"
        "   Text { id: t; text: \"zoo\"}\n"
        "}\n";

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.type(), QString("Qt/Item"));

    QList<AbstractProperty> properties  = rootModelNode.properties();
    QCOMPARE(properties.size(), 0); // TODO: How to represent alias properties? As Bindings?
}

void TestCore::testRewriterPositionAndOffset()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "    id: Root\n"
                                  "    x: 10;\n"
                                  "    y: 10;\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle1\n"
                                  "        x: 10;\n"
                                  "        y: 10;\n"
                                  "    }\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle2\n"
                                  "        x: 100;\n"
                                  "        y: 100;\n"
                                  "        anchors.fill: Root\n"
                                  "    }\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle3\n"
                                  "        x: 140;\n"
                                  "        y: 180;\n"
                                  "        gradient: Gradient {\n"
                                  "             GradientStop {\n"
                                  "                 position: 0\n"
                                  "                 color: \"white\"\n"
                                  "             }\n"
                                  "             GradientStop {\n"
                                  "                  position: 1\n"
                                  "                  color: \"black\"\n"
                                  "             }\n"
                                  "        }\n"
                                  "    }\n"
                                  "}");

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QLatin1String("Qt/Rectangle"));

    QString string;
        string = "";
    for (int i=testRewriterView->nodeOffset(rootNode); i < testRewriterView->nodeOffset(rootNode) + testRewriterView->nodeLength(rootNode);i++)
        string +=QString(qmlString)[i];

       const QString qmlExpected0("Rectangle {\n"
                                  "    id: Root\n"
                                  "    x: 10;\n"
                                  "    y: 10;\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle1\n"
                                  "        x: 10;\n"
                                  "        y: 10;\n"
                                  "    }\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle2\n"
                                  "        x: 100;\n"
                                  "        y: 100;\n"
                                  "        anchors.fill: Root\n"
                                  "    }\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle3\n"
                                  "        x: 140;\n"
                                  "        y: 180;\n"
                                  "        gradient: Gradient {\n"
                                  "             GradientStop {\n"
                                  "                 position: 0\n"
                                  "                 color: \"white\"\n"
                                  "             }\n"
                                  "             GradientStop {\n"
                                  "                  position: 1\n"
                                  "                  color: \"black\"\n"
                                  "             }\n"
                                  "        }\n"
                                  "    }\n"
                                  "}");

    QCOMPARE(string, qmlExpected0);

    ModelNode lastRectNode = rootNode.allDirectSubModelNodes().last();
    ModelNode gradientNode = lastRectNode.allDirectSubModelNodes().first();
    ModelNode gradientStop = gradientNode.allDirectSubModelNodes().first();

    string = "";
    for (int i=testRewriterView->nodeOffset(gradientNode); i < testRewriterView->nodeOffset(gradientNode) + testRewriterView->nodeLength(gradientNode);i++)
        string +=QString(qmlString)[i];

    const QString qmlExpected1(     "Gradient {\n"
                               "             GradientStop {\n"
                               "                 position: 0\n"
                               "                 color: \"white\"\n"
                               "             }\n"
                               "             GradientStop {\n"
                               "                  position: 1\n"
                               "                  color: \"black\"\n"
                               "             }\n"
                               "        }");

    QCOMPARE(string, qmlExpected1);

    string = "";
    for (int i=testRewriterView->nodeOffset(gradientStop); i < testRewriterView->nodeOffset(gradientStop) + testRewriterView->nodeLength(gradientStop);i++)
        string +=QString(qmlString)[i];

    const QString qmlExpected2(             "GradientStop {\n"
                               "                 position: 0\n"
                               "                 color: \"white\"\n"
                               "             }");

    QCOMPARE(string, qmlExpected2);
}

void TestCore::testRewriterComponentTextModifier()
{
    const QString qmlString("import Qt 4.6\n"
                            "Rectangle {\n"
                            "    id: Root\n"
                            "    x: 10;\n"
                            "    y: 10;\n"
                            "    Rectangle {\n"
                            "        id: Rectangle1\n"
                            "        x: 10;\n"
                            "        y: 10;\n"
                            "    }\n"
                            "    Component {\n"
                            "        id: RectangleComponent\n"
                            "        Rectangle {\n"
                            "            x: 100;\n"
                            "            y: 100;\n"
                            "        }\n"
                            "    }\n"
                            "}");

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QLatin1String("Qt/Rectangle"));

    ModelNode componentNode  = rootNode.allDirectSubModelNodes().last();

    int componentStartOffset = testRewriterView->nodeOffset(componentNode);
    int componentEndOffset = componentStartOffset + testRewriterView->nodeLength(componentNode);

    int rootStartOffset = testRewriterView->nodeOffset(rootNode);

    ComponentTextModifier componentTextModifier(&textModifier, componentStartOffset, componentEndOffset, rootStartOffset);

     const QString qmlExpected("import Qt 4.6\n"
                            "            "
                            "             "
                            "           "
                            "           "
                            "                "
                            "                       "
                            "               "
                            "               "
                            "      "
                            "    Component {\n"
                            "        id: RectangleComponent\n"
                            "        Rectangle {\n"
                            "            x: 100;\n"
                            "            y: 100;\n"
                            "        }\n"
                            "    } "
                            " ");

    QCOMPARE(componentTextModifier.text(), qmlExpected);

    QScopedPointer<Model> componentModel(Model::create("Qt/Item", 4, 6));
    QScopedPointer<TestRewriterView> testRewriterViewComponent(new TestRewriterView());
    testRewriterViewComponent->setTextModifier(&componentTextModifier);
    componentModel->attachView(testRewriterViewComponent.data());

    ModelNode componentRootNode = testRewriterViewComponent->rootModelNode();
    QVERIFY(componentRootNode.isValid());
    QCOMPARE(componentRootNode.type(), QLatin1String("Qt/Component"));
}

void TestCore::testRewriterPreserveType()
{
    const QString qmlString("import Qt 4.6\n"
                            "Rectangle {\n"
                            "    id: root\n"
                            "    Text {\n"
                            "        font.bold: true\n"
                            "        font.pointSize: 14\n"
                            "    }\n"
                            "}");

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QLatin1String("Qt/Rectangle"));

    ModelNode textNode = rootNode.allDirectSubModelNodes().first();
    QCOMPARE(QVariant::Bool, textNode.variantProperty("font.bold").value().type());
    QCOMPARE(QVariant::Double, textNode.variantProperty("font.pointSize").value().type());
    textNode.variantProperty("font.bold") = QVariant(false);
    textNode.variantProperty("font.bold") = QVariant(true);
    textNode.variantProperty("font.pointSize") = QVariant(13.0);

    ModelNode newTextNode = rootNode.addChildNode("Qt/Text", 4, 6, "data");

    newTextNode.variantProperty("font.bold") = QVariant(true);
    newTextNode.variantProperty("font.pointSize") = QVariant(13.0);

    QCOMPARE(QVariant::Bool, newTextNode.variantProperty("font.bold").value().type());
    QCOMPARE(QVariant::Double, newTextNode.variantProperty("font.pointSize").value().type());
}

void TestCore::testRewriterForArrayMagic()
{
    const QLatin1String qmlString("import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "    states: State {\n"
                                  "        name: \"s1\"\n"
                                  "    }\n"
                                  "}\n");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QString("Qt/Rectangle"));

    QmlItemNode rootItem(rootNode);
    QVERIFY(rootItem.isValid());

    QmlModelState state1(rootItem.states().addState("s2"));
    QCOMPARE(state1.modelNode().type(), QString("Qt/State"));

    const QLatin1String expected("import Qt 4.6\n"
                                 "\n"
                                 "Rectangle {\n"
                                 "    states: [\n"
                                 "    State {\n"
                                 "        name: \"s1\"\n"
                                 "    },\n"
                                 "    State {\n"
                                 "        name: \"s2\"\n"
                                 "    }\n"
                                 "    ]\n"
                                 "}\n");
    QCOMPARE(textEdit.toPlainText(), expected);
}

void TestCore::testRewriterWithSignals()
{
    const QLatin1String qmlString("import Qt 4.6\n"
                                  "\n"
                                  "TextEdit {\n"
                                  "    onTextChanged: { print(\"foo\"); }\n"
                                  "}\n");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QString("Qt/TextEdit"));

    QmlItemNode rootItem(rootNode);
    QVERIFY(rootItem.isValid());

    QCOMPARE(rootNode.properties().count(), 0);
}

void TestCore::testRewriterNodeSliding()
{
    const QLatin1String qmlString("import Qt 4.6\n"
                                  "Rectangle {\n"
                                  "    id: root\n"
                                  "    Rectangle {\n"
                                  "        id: rectangle1\n"
                                  "        x: 10;\n"
                                  "        y: 10;\n"
                                  "    }\n"
                                  "\n"
                                  "    Rectangle {\n"
                                  "        id: rectangle2\n"
                                  "        x: 100;\n"
                                  "        y: 100;\n"
                                  "    }\n"
                                  "}");

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QLatin1String("Qt/Rectangle"));
    QCOMPARE(rootNode.id(), QLatin1String("root"));

    QCOMPARE(rootNode.nodeListProperty(QLatin1String("data")).toModelNodeList().at(0).id(), QLatin1String("rectangle1"));
    QCOMPARE(rootNode.nodeListProperty(QLatin1String("data")).toModelNodeList().at(1).id(), QLatin1String("rectangle2"));

    rootNode.nodeListProperty(QLatin1String("data")).slide(0, 1);

    QCOMPARE(rootNode.nodeListProperty(QLatin1String("data")).toModelNodeList().at(0).id(), QLatin1String("rectangle2"));
    QCOMPARE(rootNode.nodeListProperty(QLatin1String("data")).toModelNodeList().at(1).id(), QLatin1String("rectangle1"));

    rootNode.nodeListProperty(QLatin1String("data")).slide(1, 0);

    QCOMPARE(rootNode.nodeListProperty(QLatin1String("data")).toModelNodeList().at(0).id(), QLatin1String("rectangle1"));
    QCOMPARE(rootNode.nodeListProperty(QLatin1String("data")).toModelNodeList().at(1).id(), QLatin1String("rectangle2"));
}

void TestCore::testRewriterFirstDefinitionInside()
{
    const QString qmlString("import Qt 4.6\n"
                            "Rectangle {\n"
                            "    id: Root\n"
                            "    x: 10;\n"
                            "    y: 10;\n"
                            "    Rectangle {\n"
                            "        id: Rectangle1\n"
                            "        x: 10;\n"
                            "        y: 10;\n"
                            "    }\n"
                            "    Component {\n"
                            "        id: RectangleComponent\n"
                            "        Rectangle {\n"
                            "            x: 100;\n"
                            "            y: 100;\n"
                            "        }\n"
                            "    }\n"
                            "}");


    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QLatin1String("Qt/Rectangle"));

    ModelNode componentNode  = rootNode.allDirectSubModelNodes().last();

    QString string = "";
    for (int i = testRewriterView->firstDefinitionInsideOffset(componentNode); i < testRewriterView->firstDefinitionInsideOffset(componentNode) +  testRewriterView->firstDefinitionInsideLength(componentNode);i++)
        string +=QString(qmlString)[i];

    const QString qmlExpected("Rectangle {\n"
                              "            x: 100;\n"
                              "            y: 100;\n"
                              "        }");
    QCOMPARE(string, qmlExpected);

}

void TestCore::testCopyModelRewriter1()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "    id: Root\n"
                                  "    x: 10;\n"
                                  "    y: 10;\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle1\n"
                                  "        x: 10;\n"
                                  "        y: 10;\n"
                                  "    }\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle2\n"
                                  "        x: 100;\n"
                                  "        y: 100;\n"
                                  "        anchors.fill: Root\n"
                                  "    }\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle3\n"
                                  "        x: 140;\n"
                                  "        y: 180;\n"
                                  "        gradient: Gradient {\n"
                                  "             GradientStop {\n"
                                  "                 position: 0\n"
                                  "                 color: \"white\"\n"
                                  "             }\n"
                                  "             GradientStop {\n"
                                  "                  position: 1\n"
                                  "                  color: \"black\"\n"
                                  "             }\n"
                                  "        }\n"
                                  "    }\n"
                                  "}");

    QPlainTextEdit textEdit1;
    textEdit1.setPlainText(qmlString);
    PlainTextEditModifier textModifier1(&textEdit1);

    QScopedPointer<Model> model1(Model::create("Qt/Item", 4, 6));
    QVERIFY(model1.data());

    QScopedPointer<TestView> view1(new TestView);
    model1->attachView(view1.data());

    // read in 1
    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&textModifier1);
    model1->attachView(testRewriterView1.data());

    ModelNode rootNode1 = view1->rootModelNode();
    QVERIFY(rootNode1.isValid());
    QCOMPARE(rootNode1.type(), QLatin1String("Qt/Rectangle"));

    QPlainTextEdit textEdit2;
    textEdit2.setPlainText(qmlString);
    PlainTextEditModifier textModifier2(&textEdit2);

    QScopedPointer<Model> model2(Model::create("Qt/Item", 4, 6));
    QVERIFY(model2.data());

    QScopedPointer<TestView> view2(new TestView);
    model2->attachView(view2.data());

    // read in 2 
    QScopedPointer<TestRewriterView> testRewriterView2(new TestRewriterView());
    testRewriterView2->setTextModifier(&textModifier2);
    model2->attachView(testRewriterView2.data());

    ModelNode rootNode2 = view2->rootModelNode();
    QVERIFY(rootNode2.isValid());
    QCOMPARE(rootNode2.type(), QLatin1String("Qt/Rectangle"));


    // 

    ModelNode childNode(rootNode1.allDirectSubModelNodes().first());
    QVERIFY(childNode.isValid());

    RewriterTransaction transaction(view1->beginRewriterTransaction());

    ModelMerger merger(view1.data());

    ModelNode insertedNode(merger.insertModel(rootNode2));
    transaction.commit();

    QCOMPARE(insertedNode.id(), QString("Root1"));

    QVERIFY(insertedNode.isValid());
    childNode.nodeListProperty("data").reparentHere(insertedNode);



    const QLatin1String expected(

        "\n"
        "import Qt 4.6\n"
        "\n"
        "Rectangle {\n"
        "    id: Root\n"
        "    x: 10;\n"
        "    y: 10;\n"
        "    Rectangle {\n"
        "        id: Rectangle1\n"
        "        x: 10;\n"
        "        y: 10;\n"
        "\n"
        "        Rectangle {\n"
        "            id: Root1\n"
        "            x: 10\n"
        "            y: 10\n"
        "            Rectangle {\n"
        "                id: Rectangle11\n"
        "                x: 10\n"
        "                y: 10\n"
        "            }\n"
        "\n"
        "            Rectangle {\n"
        "                id: Rectangle21\n"
        "                x: 100\n"
        "                y: 100\n"
        "                anchors.fill: Root1\n"
        "            }\n"
        "\n"
        "            Rectangle {\n"
        "                id: Rectangle31\n"
        "                x: 140\n"
        "                y: 180\n"
        "                gradient: Gradient {\n"
        "                    GradientStop {\n"
        "                        position: 0\n"
        "                        color: \"#ffffff\"\n"
        "                    }\n"
        "\n"
        "                    GradientStop {\n"
        "                        position: 1\n"
        "                        color: \"#000000\"\n"
        "                    }\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    Rectangle {\n"
        "        id: Rectangle2\n"
        "        x: 100;\n"
        "        y: 100;\n"
        "        anchors.fill: Root\n"
        "    }\n"
        "    Rectangle {\n"
        "        id: Rectangle3\n"
        "        x: 140;\n"
        "        y: 180;\n"
        "        gradient: Gradient {\n"
        "             GradientStop {\n"
        "                 position: 0\n"
        "                 color: \"white\"\n"
        "             }\n"
        "             GradientStop {\n"
        "                  position: 1\n"
        "                  color: \"black\"\n"
        "             }\n"
        "        }\n"
        "    }\n"
        "}");

    QCOMPARE(textEdit1.toPlainText(), expected);
}

void TestCore::testCopyModelRewriter2()
{
   const QLatin1String qmlString1("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "    id: Root\n"
                                  "    x: 10\n"
                                  "    y: 10\n"
                                  "\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle1\n"
                                  "        x: 10\n"
                                  "        y: 10\n"
                                  "    }\n"
                                  "\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle2\n"
                                  "        x: 100\n"
                                  "        y: 100\n"
                                  "        anchors.fill: Rectangle1\n"
                                  "    }\n"
                                  "\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle3\n"
                                  "        x: 140\n"
                                  "        y: 180\n"
                                  "        gradient: Gradient {\n"
                                  "            GradientStop {\n"
                                  "                position: 0\n"
                                  "                color: \"#ffffff\"\n"
                                  "            }\n"
                                  "\n"
                                  "            GradientStop {\n"
                                  "                position: 1\n"
                                  "                color: \"#000000\"\n"
                                  "            }\n"
                                  "        }\n"
                                  "    }\n"
                                  "}");


    const QLatin1String qmlString2("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "}");

    QPlainTextEdit textEdit1;
    textEdit1.setPlainText(qmlString1);
    PlainTextEditModifier textModifier1(&textEdit1);

    QScopedPointer<Model> model1(Model::create("Qt/Item", 4, 6));
    QVERIFY(model1.data());

    QScopedPointer<TestView> view1(new TestView);
    model1->attachView(view1.data());

    // read in 1
    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&textModifier1);
    model1->attachView(testRewriterView1.data());

    ModelNode rootNode1 = view1->rootModelNode();
    QVERIFY(rootNode1.isValid());
    QCOMPARE(rootNode1.type(), QLatin1String("Qt/Rectangle"));


        // read in 2 

    QPlainTextEdit textEdit2;
    textEdit2.setPlainText(qmlString2);
    PlainTextEditModifier textModifier2(&textEdit2);

    QScopedPointer<Model> model2(Model::create("Qt/Item", 4, 6));
    QVERIFY(model2.data());

    QScopedPointer<TestView> view2(new TestView);
    model2->attachView(view2.data());

    QScopedPointer<TestRewriterView> testRewriterView2(new TestRewriterView());
    testRewriterView2->setTextModifier(&textModifier2);
    model2->attachView(testRewriterView2.data());

    ModelNode rootNode2 = view2->rootModelNode();
    QVERIFY(rootNode2.isValid());
    QCOMPARE(rootNode2.type(), QLatin1String("Qt/Rectangle"));


    //

    ModelMerger merger(view2.data());

    merger.replaceModel(rootNode1);
    QVERIFY(rootNode2.isValid());
    QCOMPARE(rootNode2.type(), QLatin1String("Qt/Rectangle"));

    QCOMPARE(textEdit2.toPlainText(), qmlString1);
}


void TestCore::loadQml()
{
char qmlString[] = "import Qt 4.6\n"
                       "Rectangle {\n"
                            "id: root;\n"
                            "width: 200;\n"
                            "height: 200;\n"
                            "color: \"white\";\n"
                            "Text {\n"
                                "id: text1\n"
                                "text: \"Hello World\"\n"
                                "anchors.centerIn: parent\n"
                                "Item {\n"
                                    "id: item\n"
                                 "}\n"
                            "}\n"
                            "Rectangle {\n"
                                "id: rectangle;\n"
                                "gradient: Gradient {\n"
                                    "GradientStop {\n"
                                        "position: 0\n"
                                        "color: \"white\"\n"
                                     "}\n"
                                     "GradientStop {\n"
                                        "position: 1\n"
                                        "color: \"black\"\n"
                                     "}\n"
                                "}\n"
                            "}\n"
                             "Text {\n"
                                "text: \"text\"\n"
                                "x: 66\n"
                                "y: 43\n"
                                "width: 80\n"
                                "height: 20\n"
                                "id: text2\n"
                            "}\n"
                       "}\n";

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.type(), QString("Qt/Item"));
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);

    model->attachView(testRewriterView.data());

    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.type(), QString("Qt/Rectangle"));
    QCOMPARE(rootModelNode.id(), QString("root"));
    QCOMPARE(rootModelNode.variantProperty("width").value().toInt(), 200);
    QCOMPARE(rootModelNode.variantProperty("height").value().toInt(), 200);
    QVERIFY(rootModelNode.hasProperty("data"));
    QVERIFY(rootModelNode.property("data").isNodeListProperty());
    QVERIFY(!rootModelNode.nodeListProperty("data").toModelNodeList().isEmpty());
    QCOMPARE(rootModelNode.nodeListProperty("data").toModelNodeList().count(), 3);
    ModelNode textNode1 = rootModelNode.nodeListProperty("data").toModelNodeList().first();
    QVERIFY(textNode1.isValid());
    QCOMPARE(textNode1.type(), QString("Qt/Text"));
    QCOMPARE(textNode1.id(), QString("text1"));
    QCOMPARE(textNode1.variantProperty("text").value().toString(), QString("Hello World"));
    QVERIFY(textNode1.hasProperty("anchors.centerIn"));
    QVERIFY(textNode1.property("anchors.centerIn").isBindingProperty());
    QCOMPARE(textNode1.bindingProperty("anchors.centerIn").expression(), QString("parent"));
    QVERIFY(textNode1.hasProperty("data"));
    QVERIFY(textNode1.property("data").isNodeListProperty());
    QVERIFY(!textNode1.nodeListProperty("data").toModelNodeList().isEmpty());
    ModelNode itemNode = textNode1.nodeListProperty("data").toModelNodeList().first();
    QVERIFY(itemNode.isValid());
    QCOMPARE(itemNode.id(), QString("item"));
    QVERIFY(!itemNode.hasProperty("data"));

    ModelNode rectNode = rootModelNode.nodeListProperty("data").toModelNodeList().at(1);
    QVERIFY(rectNode.isValid());
    QCOMPARE(rectNode.type(), QString("Qt/Rectangle"));
    QCOMPARE(rectNode.id(), QString("rectangle"));
    QVERIFY(rectNode.hasProperty("gradient"));
    QVERIFY(rectNode.property("gradient").isNodeProperty());
    ModelNode gradientNode = rectNode.nodeProperty("gradient").modelNode();
    QVERIFY(gradientNode.isValid());
    QCOMPARE(gradientNode.type(), QString("Qt/Gradient"));
    QVERIFY(gradientNode.hasProperty("stops"));
    QVERIFY(gradientNode.property("stops").isNodeListProperty());
    QCOMPARE(gradientNode.nodeListProperty("stops").toModelNodeList().count(), 2);

    ModelNode stop1 = gradientNode.nodeListProperty("stops").toModelNodeList().first();
    ModelNode stop2 = gradientNode.nodeListProperty("stops").toModelNodeList().last();

    QVERIFY(stop1.isValid());
    QVERIFY(stop2.isValid());

    QCOMPARE(stop1.type(), QString("Qt/GradientStop"));
    QCOMPARE(stop2.type(), QString("Qt/GradientStop"));

    QCOMPARE(stop1.variantProperty("position").value().toInt(), 0);
    QCOMPARE(stop2.variantProperty("position").value().toInt(), 1);

    ModelNode textNode2 = rootModelNode.nodeListProperty("data").toModelNodeList().last();
    QVERIFY(textNode2.isValid());
    QCOMPARE(textNode2.type(), QString("Qt/Text"));
    QCOMPARE(textNode2.id(), QString("text2"));
    QCOMPARE(textNode2.variantProperty("width").value().toInt(), 80);
    QCOMPARE(textNode2.variantProperty("height").value().toInt(), 20);
    QCOMPARE(textNode2.variantProperty("x").value().toInt(), 66);
    QCOMPARE(textNode2.variantProperty("y").value().toInt(), 43);
    QCOMPARE(textNode2.variantProperty("text").value().toString(), QString("text"));
}

void TestCore::testMetaInfo()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QVERIFY(model->metaInfo().hasNodeMetaInfo("Qt/Item"));
    QVERIFY(model->metaInfo().hasNodeMetaInfo("Qt/QtObject"));
    QVERIFY(model->metaInfo().isSubclassOf("Qt/Text", "Qt/QtObject"));
    QVERIFY(model->metaInfo().isSubclassOf("Qt/Text", "Qt/Item"));
    QVERIFY(model->metaInfo().isSubclassOf("Qt/Rectangle", "Qt/QtObject"));

    QVERIFY(!model->metaInfo().isSubclassOf("Fooo", "Qt/QtObject"));

    QVERIFY(model->metaInfo().nodeMetaInfo("Qt/Item", 4, 6).isContainer());

    QCOMPARE(view->rootModelNode().metaInfo().typeName(), QString("Qt/Item"));
    QVERIFY(view->rootModelNode().metaInfo().hasProperty("x"));
    QVERIFY(!view->rootModelNode().metaInfo().hasProperty("blah"));
    QVERIFY(!view->rootModelNode().metaInfo().property("blah").isValid());
    QVERIFY(view->rootModelNode().metaInfo().isContainer());

    ModelNode rectNode = view->rootModelNode().addChildNode("Qt/Rectangle", 4, 6, "data");

    QVERIFY(rectNode.metaInfo().isSubclassOf("Qt/QtObject"));
    QVERIFY(rectNode.metaInfo().isSubclassOf("Qt/Item"));
    QVERIFY(!rectNode.metaInfo().isSubclassOf("Qt/Text"));
    QVERIFY(rectNode.metaInfo().hasProperty("color"));
    QVERIFY(rectNode.metaInfo().hasProperty("x"));
    QVERIFY(!rectNode.metaInfo().hasProperty("blah"));
    QVERIFY(rectNode.metaInfo().isContainer());

    ModelNode textNode = view->rootModelNode().addChildNode("Qt/TextEdit", 4, 6, "data");
    NodeMetaInfo textNodeMetaInfo = textNode.metaInfo();
    QVERIFY(textNodeMetaInfo.hasProperty("text"));
    QVERIFY(textNodeMetaInfo.property("text").isValid());
    QVERIFY(textNodeMetaInfo.property("text").isReadable());
    QVERIFY(textNodeMetaInfo.property("text").isWriteable());
    QVERIFY(textNodeMetaInfo.property("x").isReadable());
    QVERIFY(textNodeMetaInfo.property("x").isWriteable());
    QVERIFY(textNodeMetaInfo.isSubclassOf("Qt/Item", 4, 6));
    QVERIFY(!textNodeMetaInfo.isSubclassOf("Blah"));
    QVERIFY(textNodeMetaInfo.isQmlGraphicsItem());
    QVERIFY(textNodeMetaInfo.isGraphicsObject());
    QVERIFY(!textNodeMetaInfo.isGraphicsWidget());

    // test types declared with EXTENDED_OBJECT
    NodeMetaInfo graphicsWidgetInfo = model->metaInfo().nodeMetaInfo("Qt/QGraphicsWidget", 4, 6);
    QVERIFY(graphicsWidgetInfo.isValid());
    QVERIFY(graphicsWidgetInfo.hasProperty("layout")); // from QGraphicsWidgetDeclarativeUI
    QVERIFY(graphicsWidgetInfo.hasProperty("font")); // from QGraphicsWidget
    QVERIFY(graphicsWidgetInfo.hasProperty("enabled")); // from QGraphicsItem
}

void TestCore::testMetaInfoDotProperties()
{
    QScopedPointer<Model> model(Model::create("Qt/Text"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QVERIFY(model->metaInfo().hasNodeMetaInfo("Qt/Text"));
    QCOMPARE(view->rootModelNode().metaInfo().typeName(), QString("Qt/Text"));
    QVERIFY(!view->rootModelNode().metaInfo().property("text").isValueType());
    QVERIFY(view->rootModelNode().metaInfo().hasProperty("font"));
    QVERIFY(view->rootModelNode().metaInfo().property("font").isValueType());

    QVERIFY(view->rootModelNode().metaInfo().hasProperty("font.bold", true));
    QVERIFY(view->rootModelNode().metaInfo().properties(true).keys().contains("font.bold"));
    QVERIFY(!view->rootModelNode().metaInfo().properties().keys().contains("font.bold"));
    QVERIFY(view->rootModelNode().metaInfo().properties(true).keys().contains("font.pointSize"));
    QVERIFY(view->rootModelNode().metaInfo().hasProperty("font.pointSize", true));
    QVERIFY(view->rootModelNode().metaInfo().property("font.pointSize", true).isValid());

    ModelNode rectNode(view->rootModelNode().addChildNode("Qt/Rectangle", 4, 6, "data"));

    
    QVERIFY(rectNode.metaInfo().properties(true).keys().contains("pos.x"));
    QVERIFY(!rectNode.metaInfo().properties().keys().contains("pos.x"));    
    QVERIFY(rectNode.metaInfo().properties(true).keys().contains("pos.y"));
    QVERIFY(!rectNode.metaInfo().properties().keys().contains("pos.y"));
    QVERIFY(rectNode.metaInfo().properties(true).keys().contains("border.width"));
    QVERIFY(rectNode.metaInfo().hasProperty("border"));
    QVERIFY(!rectNode.metaInfo().property("border").isValueType());
    QVERIFY(rectNode.metaInfo().hasProperty("border.width", true));
    QVERIFY(rectNode.metaInfo().property("border.width", true).isValid());
}

void TestCore::testMetaInfoListProperties()
{
     QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QVERIFY(model->metaInfo().hasNodeMetaInfo("Qt/Item"));
    QCOMPARE(view->rootModelNode().metaInfo().typeName(), QString("Qt/Item"));

    QVERIFY(view->rootModelNode().metaInfo().hasProperty("states"));
    QVERIFY(view->rootModelNode().metaInfo().property("states").isListProperty());
    QVERIFY(view->rootModelNode().metaInfo().hasProperty("children"));
    QVERIFY(view->rootModelNode().metaInfo().property("children").isListProperty());
    QVERIFY(view->rootModelNode().metaInfo().hasProperty("data"));
    QVERIFY(view->rootModelNode().metaInfo().property("data").isListProperty());
    QVERIFY(view->rootModelNode().metaInfo().hasProperty("resources"));
    QVERIFY(view->rootModelNode().metaInfo().property("resources").isListProperty());
    QVERIFY(view->rootModelNode().metaInfo().hasProperty("transitions"));
    QVERIFY(view->rootModelNode().metaInfo().property("transitions").isListProperty());
    QVERIFY(view->rootModelNode().metaInfo().hasProperty("transform"));
    QVERIFY(view->rootModelNode().metaInfo().property("transform").isListProperty());

    QVERIFY(view->rootModelNode().metaInfo().hasProperty("effect"));
    QVERIFY(!view->rootModelNode().metaInfo().property("effect").isListProperty());
    QVERIFY(view->rootModelNode().metaInfo().hasProperty("parent"));
    QVERIFY(!view->rootModelNode().metaInfo().property("parent").isListProperty());
}

void TestCore::subItemMetaInfo()
{
    QSKIP("no rewriter anymore", SkipAll);
//    QFile file(QCoreApplication::applicationDirPath() + "/../tests/data/fx/topitem.qml");
//    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
//
//    QList<QmlError> errors;
//    QScopedPointer<ByteArrayModifier> modifier1(ByteArrayModifier::create(QString(file.readAll())));
//    QScopedPointer<Model> model(Model::create(modifier1.data(), QUrl::fromLocalFile(file.fileName()), &errors));
//    QVERIFY(errors.isEmpty());
//    QVERIFY(model.data());
//
//    ModelNode firstChild = view->rootModelNode().childNodes().at(0);
//    QVERIFY(firstChild.isValid());
//    QCOMPARE(firstChild.type(), QString("SubItem"));
//    NodeMetaInfo firstMetaInfo = firstChild.metaInfo();
//    QVERIFY(firstMetaInfo.isValid());
//    QList<NodeMetaInfo> superClasses = firstMetaInfo.directSuperClasses();
//    QVERIFY(superClasses.contains(model->metaInfo().nodeMetaInfo("Qt/Rectangle", 4, 6, "data")));
//    QVERIFY(model->metaInfo().nodeMetaInfo("Qt/Rectangle", 4, 6, "data").property("x").isValid());
//    QVERIFY(firstMetaInfo.property("x").isValid());
//
//#ifdef CUSTOM_PROPERTIES_SHOW_UP_IN_THE_DOM
//    QVERIFY(firstMetaInfo.property("text").isValid());
//    QCOMPARE(firstMetaInfo.property("text").variantTypeId(), QVariant::String);
//#endif // CUSTOM_PROPERTIES_SHOW_UP_IN_THE_DOM
}

void TestCore::testStatesRewriter()
{
    QPlainTextEdit textEdit;
    textEdit.setPlainText("import Qt 4.6; Item {}\n");
    PlainTextEditModifier modifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());
    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    TestRewriterView *testRewriterView = new TestRewriterView(model.data());
    testRewriterView->setTextModifier(&modifier);
    model->attachView(testRewriterView);

    QVERIFY(testRewriterView->errors().isEmpty());

    ModelNode rootModelNode(view->rootModelNode());

    QVERIFY(rootModelNode.isValid());

    QVERIFY(QmlItemNode(rootModelNode).isValid());

    QVERIFY(QmlItemNode(rootModelNode).states().allStates().isEmpty());

    QmlModelState state1 = QmlItemNode(rootModelNode).states().addState("state 1");
    QVERIFY(state1.isValid());
    QVERIFY(!QmlItemNode(rootModelNode).states().allStates().isEmpty());
    QCOMPARE(QmlItemNode(rootModelNode).states().allStates().count(), 1);
    
    QmlModelState state2 = QmlItemNode(rootModelNode).states().addState("state 2");
    QVERIFY(state2.isValid());

    QCOMPARE(QmlItemNode(rootModelNode).states().allStates().count(), 2);

    QmlItemNode(rootModelNode).states().removeState("state 1");
    QVERIFY(!state1.isValid());

    QmlItemNode(rootModelNode).states().removeState("state 2");
    QVERIFY(!state2.isValid());

    QVERIFY(QmlItemNode(rootModelNode).states().allStates().isEmpty());

}


void TestCore::testGradientsRewriter()
{
//    QSKIP("this crashes in the rewriter", SkipAll);

    QPlainTextEdit textEdit;
    textEdit.setPlainText("\nimport Qt 4.6\n\nItem {\n}\n");
    PlainTextEditModifier modifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());
    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    TestRewriterView *testRewriterView = new TestRewriterView(model.data());
    testRewriterView->setTextModifier(&modifier);
    model->attachView(testRewriterView);

    QVERIFY(testRewriterView->errors().isEmpty());

    ModelNode rootModelNode(view->rootModelNode());

    QVERIFY(rootModelNode.isValid());

    ModelNode rectNode(rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data"));

    const QLatin1String expected1("\nimport Qt 4.6\n"
                                  "\n"
                                  "Item {\n"
                                  "    Rectangle {\n"
                                  "    }\n"
                                  "}\n");
    QCOMPARE(textEdit.toPlainText(), expected1);

    ModelNode gradientNode(rectNode.addChildNode("Qt/Gradient", 4, 6, "gradient"));

    QVERIFY(rectNode.hasNodeProperty("gradient"));

    const QLatin1String expected2("\nimport Qt 4.6\n"
                                  "\n"
                                  "Item {\n"
                                  "    Rectangle {\n"
                                  "        gradient: Gradient {\n"
                                  "        }\n"
                                  "    }\n"
                                  "}\n");
    QCOMPARE(textEdit.toPlainText(), expected2);

    NodeListProperty stops(gradientNode.nodeListProperty("stops"));

    QList<QPair<QString, QVariant> > propertyList;

    propertyList.append(qMakePair(QString("position"), QVariant::fromValue(0)));
    propertyList.append(qMakePair(QString("color"), QVariant::fromValue(QColor(Qt::red))));

    ModelNode gradientStop1(gradientNode.view()->createModelNode("Qt/GradientStop", 4, 6, propertyList));
    QVERIFY(gradientStop1.isValid());
    stops.reparentHere(gradientStop1);

    const QLatin1String expected3("\nimport Qt 4.6\n"
                                  "\n"
                                  "Item {\n"
                                  "    Rectangle {\n"
                                  "        gradient: Gradient {\n"
                                  "            GradientStop {\n"
                                  "                position: 0\n"
                                  "                color: \"#ff0000\"\n"
                                  "            }\n"
                                  "        }\n"
                                  "    }\n"
                                  "}\n");

    QCOMPARE(textEdit.toPlainText(), expected3);

    propertyList.clear();
    propertyList.append(qMakePair(QString("position"), QVariant::fromValue(0.5)));
    propertyList.append(qMakePair(QString("color"), QVariant::fromValue(QColor(Qt::blue))));

    ModelNode gradientStop2(gradientNode.view()->createModelNode("Qt/GradientStop", 4, 6, propertyList));
    QVERIFY(gradientStop2.isValid());
    stops.reparentHere(gradientStop2);

    const QLatin1String expected4("\nimport Qt 4.6\n"
                                  "\n"
                                  "Item {\n"
                                  "    Rectangle {\n"
                                  "        gradient: Gradient {\n"
                                  "            GradientStop {\n"
                                  "                position: 0\n"
                                  "                color: \"#ff0000\"\n"
                                  "            }\n"
                                  "\n"
                                  "            GradientStop {\n"
                                  "                position: 0.5\n"
                                  "                color: \"#0000ff\"\n"
                                  "            }\n"
                                  "        }\n"
                                  "    }\n"
                                  "}\n");

    QCOMPARE(textEdit.toPlainText(), expected4);

    propertyList.clear();
    propertyList.append(qMakePair(QString("position"), QVariant::fromValue(0.8)));
    propertyList.append(qMakePair(QString("color"), QVariant::fromValue(QColor(Qt::yellow))));

    ModelNode gradientStop3(gradientNode.view()->createModelNode("Qt/GradientStop", 4, 6, propertyList));
    QVERIFY(gradientStop3.isValid());
    stops.reparentHere(gradientStop3);

    const QLatin1String expected5("\nimport Qt 4.6\n"
                                  "\n"
                                  "Item {\n"
                                  "    Rectangle {\n"
                                  "        gradient: Gradient {\n"
                                  "            GradientStop {\n"
                                  "                position: 0\n"
                                  "                color: \"#ff0000\"\n"
                                  "            }\n"
                                  "\n"
                                  "            GradientStop {\n"
                                  "                position: 0.5\n"
                                  "                color: \"#0000ff\"\n"
                                  "            }\n"
                                  "\n"
                                  "            GradientStop {\n"
                                  "                position: 0.8\n"
                                  "                color: \"#ffff00\"\n"
                                  "            }\n"
                                  "        }\n"
                                  "    }\n"
                                  "}\n");

    QCOMPARE(textEdit.toPlainText(), expected5);

    gradientNode.removeProperty("stops");

    const QLatin1String expected6("\nimport Qt 4.6\n"
                                  "\n"
                                  "Item {\n"
                                  "    Rectangle {\n"
                                  "        gradient: Gradient {\n"
                                  "        }\n"
                                  "    }\n"
                                  "}\n");

    QCOMPARE(textEdit.toPlainText(), expected6);

    QVERIFY(!gradientNode.hasProperty(QLatin1String("stops")));

    propertyList.clear();
    propertyList.append(qMakePair(QString("position"), QVariant::fromValue(0)));
    propertyList.append(qMakePair(QString("color"), QVariant::fromValue(QColor(Qt::blue))));

    gradientStop1 = gradientNode.view()->createModelNode("Qt/GradientStop", 4, 6, propertyList);
    QVERIFY(gradientStop1.isValid());

    stops.reparentHere(gradientStop1);
}

void TestCore::testQmlModelStates()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());
    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());



    ModelNode rootModelNode(view->rootModelNode());

    QVERIFY(rootModelNode.isValid());

    QVERIFY(QmlItemNode(rootModelNode).isValid());

    QVERIFY(QmlItemNode(rootModelNode).states().allStates().isEmpty());

    QmlModelState newState = QmlItemNode(rootModelNode).states().addState("testState");
    QVERIFY(newState.isValid());
    QVERIFY(!QmlItemNode(rootModelNode).states().allStates().isEmpty());
    QCOMPARE(QmlItemNode(rootModelNode).states().allStates().count(), 1);

    QCOMPARE(newState.name(), QString("testState"));
    QCOMPARE(newState, QmlItemNode(rootModelNode).states().state("testState"));

    QmlItemNode(rootModelNode).states().removeState("testState");
    QVERIFY(!newState.isValid());
    QVERIFY(QmlItemNode(rootModelNode).states().allStates().isEmpty());

}

void TestCore::testInstancesStates()
{
//
//    import Qt 4.6
//
//    Rectangle {
//      Text {
//        id: targetObject
//        text: "base state"
//      }
//
//      states: [
//        State {
//          name: "state1"
//          PropertyChanges {
//            target: targetObj
//            x: 10
//            text: "state1"
//          }
//        }
//        State {
//          name: "state2"
//          PropertyChanges {
//            target: targetObj
//            text: "state2"
//          }
//        }
//      ]
//    }
//

    QScopedPointer<Model> model(Model::create("Qt/Rectangle", 4, 6));
    QVERIFY(model.data());
    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    //
    // build up model
    //
    ModelNode rootNode = view->rootModelNode();

    ModelNode textNode = view->createModelNode("Qt/Text", 4, 6);
    textNode.setId("targetObject");
    textNode.variantProperty("text").setValue("base state");

    rootNode.nodeListProperty("data").reparentHere(textNode);

    ModelNode propertyChanges1Node = view->createModelNode("Qt/PropertyChanges", 4, 6);
    propertyChanges1Node.bindingProperty("target").setExpression("targetObject");
    propertyChanges1Node.variantProperty("x").setValue(10);
    propertyChanges1Node.variantProperty("text").setValue("state1");

    ModelNode state1Node = view->createModelNode("Qt/State", 4, 6);
    state1Node.variantProperty("name").setValue("state1");
    state1Node.nodeListProperty("changes").reparentHere(propertyChanges1Node);

    rootNode.nodeListProperty("states").reparentHere(state1Node);

    ModelNode propertyChanges2Node = view->createModelNode("Qt/PropertyChanges", 4, 6);
    propertyChanges2Node.bindingProperty("target").setExpression("targetObject");
    propertyChanges2Node.variantProperty("text").setValue("state2");

    ModelNode state2Node = view->createModelNode("Qt/State", 4, 6);
    state2Node.variantProperty("name").setValue("state2");
    state2Node.nodeListProperty("changes").reparentHere(propertyChanges2Node);

    rootNode.nodeListProperty("states").reparentHere(state2Node);

    //
    // load into instance view
    //
    QScopedPointer<NodeInstanceView> instanceView(new NodeInstanceView);

    model->attachView(instanceView.data());

    //
    // check that list of actions is not empty (otherwise the CustomParser has not been run properly)
    //
    NodeInstance state1Instance = instanceView->instanceForNode(state1Node);
    QVERIFY(state1Instance.isValid());
    QmlState *state1 = const_cast<QmlState*>(qobject_cast<const QmlState*>(state1Instance.testHandle()));
    QVERIFY(state1);
    QCOMPARE(state1->changes()->count(), 1);
    QCOMPARE(state1->changes()->at(0)->actions().size(), 2);

    NodeInstance state2Instance = instanceView->instanceForNode(state2Node);
    QVERIFY(state2Instance.isValid());
    QmlState *state2 = const_cast<QmlState*>(qobject_cast<const QmlState*>(state1Instance.testHandle()));
    QVERIFY(state2);
    QCOMPARE(state2->changes()->count(), 1);
    QCOMPARE(state2->changes()->at(0)->actions().size(), 2);

    NodeInstance textInstance = instanceView->instanceForNode(textNode);

    //
    // State switching
    //

    // base state
    QVERIFY(textInstance.isValid());
    QCOMPARE(state1Instance.property("__activateState").toBool(), false);
    QCOMPARE(state2Instance.property("__activateState").toBool(), false);
    QCOMPARE(textInstance.property("x").toInt(), 0);
    QCOMPARE(textInstance.property("text").toString(), QString("base state"));

    // base state -> state 1
    state1Instance.setPropertyVariant("__activateState", true);
    QCOMPARE(state1Instance.property("__activateState").toBool(), true);
    QCOMPARE(state2Instance.property("__activateState").toBool(), false);
    QCOMPARE(textInstance.property("x").toInt(), 10);
    QCOMPARE(textInstance.property("text").toString(), QString("state1"));

    // state 1 -> state 2
    state2Instance.setPropertyVariant("__activateState", true);
    QCOMPARE(state1Instance.property("__activateState").toBool(), false);
    QCOMPARE(state2Instance.property("__activateState").toBool(), true);
    QCOMPARE(textInstance.property("x").toInt(), 0);
    QCOMPARE(textInstance.property("text").toString(), QString("state2"));

    // state 1 -> base state
    state2Instance.setPropertyVariant("__activateState", false);
    QCOMPARE(state1Instance.property("__activateState").toBool(), false);
    QCOMPARE(state2Instance.property("__activateState").toBool(), false);
    QCOMPARE(textInstance.property("x").toInt(), 0);
    QCOMPARE(textInstance.property("text").toString(), QString("base state"));

    //
    // Add/Change/Remove properties in current state
    //
    state1Instance.setPropertyVariant("__activateState", true);

    propertyChanges1Node.variantProperty("x").setValue(20);
    QCOMPARE(textInstance.property("x").toInt(), 20);
    propertyChanges1Node.variantProperty("x").setValue(10);  // undo
    QCOMPARE(textInstance.property("x").toInt(), 10);

    QCOMPARE(textInstance.property("y").toInt(), 0);
    propertyChanges1Node.variantProperty("y").setValue(50);
    QCOMPARE(textInstance.property("y").toInt(), 50);
    propertyChanges1Node.removeProperty("y");
    QCOMPARE(textInstance.property("y").toInt(), 0);

    QCOMPARE(textInstance.property("text").toString(), QString("state1"));
    propertyChanges1Node.removeProperty("text");
    QCOMPARE(textInstance.property("text").toString(), QString("base state"));
    propertyChanges1Node.variantProperty("text").setValue("state1");   // undo
    QCOMPARE(textInstance.property("text").toString(), QString("state1"));

////    Following is not supported. We work around this
////    by _always_ changing to the base state before doing any changes to the
////    state structure.

    //
    // Reparenting state actions (while state is active)
    //

//    state1Instance.setPropertyVariant("__activateState", true);
//    // move property changes of current state out of state
//    ModelNode state3Node = view->createModelNode("Qt/State", 4, 6);
//    state3Node.nodeListProperty("changes").reparentHere(propertyChanges1Node);
//
//    QCOMPARE(state1->changes()->count(), 0);
//    QCOMPARE(textInstance.property("text").toString(), QString("base state"));
//
//    // undo
//    state1Node.nodeListProperty("changes").reparentHere(propertyChanges1Node);
//    QCOMPARE(state1->changes()->count(), 1);
//    QCOMPARE(textInstance.property("text").toString(), QString("state 1"));

    //
    // Removing state actions (while state is active)
    //

//    state1Instance.setPropertyVariant("__activateState", true);
//    QCOMPARE(textInstance.property("text").toString(), QString("state1"));
//    propertyChanges1Node.destroy();
//    QCOMPARE(state1->changes()->count(), 0);
//    QCOMPARE(textInstance.property("text").toString(), QString("base state"));

    //
    // Removing state (while active)
    //

//    state2Instance.setPropertyVariant("__activateState", true);
//    QCOMPARE(textInstance.property("text").toString(), QString("state2"));
//    state2Node.destroy();
//    QCOMPARE(textInstance.property("text").toString(), QString("base state"));
}

void TestCore::testStates()
{
//
//    import Qt 4.6
//
//    Rectangle {
//      Text {
//        id: targetObject
//        text: "base state"
//      }
//      states: [
//        State {
//          name: "state 1"
//          PropertyChanges {
//            target: targetObj
//            text: "state 1"
//          }
//        }
//      ]
//    }
//

    QScopedPointer<Model> model(Model::create("Qt/Rectangle", 4, 6));
    QVERIFY(model.data());
    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    // build up model
    ModelNode rootNode = view->rootModelNode();

    ModelNode textNode = view->createModelNode("Qt/Text", 4, 6);
    textNode.setId("targetObject");
    textNode.variantProperty("text").setValue("base state");

    rootNode.nodeListProperty("data").reparentHere(textNode);

    QVERIFY(rootNode.isValid());
    QVERIFY(textNode.isValid());

    QmlItemNode rootItem(rootNode);
    QmlItemNode textItem(textNode);

    QVERIFY(rootItem.isValid());
    QVERIFY(textItem.isValid());

    QmlModelState state1(rootItem.states().addState("state 1")); //add state "state 1"
    QCOMPARE(state1.modelNode().type(), QString("Qt/State"));

    QVERIFY(view->currentState().isBaseState());

    NodeInstance textInstance = view->nodeInstanceView()->instanceForNode(textNode);
    QVERIFY(textInstance.isValid());

    NodeInstance state1Instance = view->nodeInstanceView()->instanceForNode(state1);
    QVERIFY(state1Instance.isValid());
    QCOMPARE(state1Instance.property("__activateState").toBool(), false);
    QCOMPARE(state1Instance.property("name").toString(), QString("state 1"));

    view->setCurrentState(state1); //set currentState "state 1"

    QCOMPARE(view->currentState(), state1);

    QCOMPARE(state1Instance.property("__activateState").toBool(), true);

    QVERIFY(!textItem.propertyAffectedByCurrentState("text"));

    textItem.setVariantProperty("text", QVariant("state 1")); //set text in state !

    QmlState *qmlState1 = const_cast<QmlState*>(qobject_cast<const QmlState*>(state1Instance.testHandle()));
    QVERIFY(qmlState1);
    QCOMPARE(qmlState1->changes()->count(), 1);
    QCOMPARE(qmlState1->changes()->at(0)->actions().size(), 1);

    QmlPropertyChanges changes(state1.propertyChanges(textNode));
    QVERIFY(changes.modelNode().hasProperty("text"));
    QCOMPARE(changes.modelNode().variantProperty("text").value(), QVariant("state 1"));
    QCOMPARE(changes.modelNode().bindingProperty("target").expression(), QString("targetObject"));
    QCOMPARE(changes.target(), textNode);
    QCOMPARE(changes.modelNode().type(), QString("Qt/PropertyChanges"));

    QCOMPARE(changes.modelNode().parentProperty().name(), QString("changes"));
    QCOMPARE(changes.modelNode().parentProperty().parentModelNode(), state1.modelNode());

    QCOMPARE(state1Instance.property("__activateState").toBool(), true);

    QVERIFY(textItem.propertyAffectedByCurrentState("text"));

    QCOMPARE(textInstance.property("text").toString(), QString("state 1"));
    QCOMPARE(textItem.instanceValue("text"), QVariant("state 1"));

    QVERIFY(textNode.hasProperty("text"));
    QCOMPARE(textNode.variantProperty("text").value(), QVariant("base state"));
}

void TestCore::testStatesBaseState()
{
//
//    import Qt 4.6
//
//    Rectangle {
//      Text {
//        id: targetObject
//        text: "base state"
//      }
//      states: [
//        State {
//          name: "state 1"
//          PropertyChanges {
//            target: targetObj
//            text: "state 1"
//          }
//        }
//      ]
//    }
//

    QScopedPointer<Model> model(Model::create("Qt/Rectangle", 4, 6));
    QVERIFY(model.data());
    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    // build up model
    ModelNode rootNode = view->rootModelNode();

    ModelNode textNode = view->createModelNode("Qt/Text", 4, 6);
    textNode.setId("targetObject");
    textNode.variantProperty("text").setValue("base state");

    rootNode.nodeListProperty("data").reparentHere(textNode);

    QVERIFY(rootNode.isValid());
    QVERIFY(textNode.isValid());

    QmlItemNode rootItem(rootNode);
    QmlItemNode textItem(textNode);

    QVERIFY(rootItem.isValid());
    QVERIFY(textItem.isValid());

    QmlModelState state1(rootItem.states().addState("state 1")); //add state "state 1"
    QCOMPARE(state1.modelNode().type(), QString("Qt/State"));

    QVERIFY(view->currentState().isBaseState());

    view->setCurrentState(state1); //set currentState "state 1"
    QCOMPARE(view->currentState(), state1);
    textItem.setVariantProperty("text", QVariant("state 1")); //set text in state !
    QVERIFY(textItem.propertyAffectedByCurrentState("text"));
    QCOMPARE(textItem.instanceValue("text"), QVariant("state 1"));

    view->setCurrentState(view->baseState()); //set currentState base state
    QVERIFY(view->currentState().isBaseState());

    textNode.variantProperty("text").setValue("base state changed");

    view->setCurrentState(state1); //set currentState "state 1"
    QCOMPARE(view->currentState(), state1);
    QCOMPARE(textItem.instanceValue("text"), QVariant("state 1"));

    view->setCurrentState(view->baseState()); //set currentState base state
    QVERIFY(view->currentState().isBaseState());

    QCOMPARE(textItem.instanceValue("text"), QVariant("base state changed"));
}


void TestCore::testInstancesIdResolution()
{
    QScopedPointer<Model> model(Model::create("Qt/Rectangle", 4, 6));
    QVERIFY(model.data());
    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    //    import Qt 4.6
    //
    //    Rectangle {
    //      id: root
    //      width: 100
    //      height: 100
    //      Rectangle {
    //        id: item1
    //        width: root.width
    //        height: item2.height
    //      }
    //    }

    ModelNode rootNode = view->rootModelNode();
    rootNode.setId("root");
    rootNode.variantProperty("width").setValue(100);
    rootNode.variantProperty("height").setValue(100);

    ModelNode item1Node = view->createModelNode("Qt/Rectangle", 4, 6);
    item1Node.setId("item1");
    item1Node.bindingProperty("width").setExpression("root.width");
    item1Node.bindingProperty("height").setExpression("item2.height");

    rootNode.nodeListProperty("data").reparentHere(item1Node);

    NodeInstance item1Instance = view->instanceForModelNode(item1Node);
    QVERIFY(item1Instance.isValid());

    QCOMPARE(item1Instance.property("width").toInt(), 100);
    QCOMPARE(item1Instance.property("height").toInt(), 0); // item2 is still unknown

    // Add item2:
    //      Rectangle {
    //        id: item2
    //        width: root.width / 2
    //        height: root.height
    //      }

    ModelNode item2Node = view->createModelNode("Qt/Rectangle", 4, 6);
    item2Node.setId("item2");
    item2Node.bindingProperty("width").setExpression("root.width / 2");
    item2Node.bindingProperty("height").setExpression("root.height");

    rootNode.nodeListProperty("data").reparentHere(item2Node);

    NodeInstance item2Instance = view->instanceForModelNode(item2Node);
    QVERIFY(item2Instance.isValid());

    QCOMPARE(item2Instance.property("width").toInt(), 50);
    QCOMPARE(item2Instance.property("height").toInt(), 100);
    QCOMPARE(item1Instance.property("height").toInt(), 100);

    // Remove item2 again
    item2Node.destroy();
    QVERIFY(!item2Instance.isValid());
    QVERIFY(!item2Instance.testHandle());

    // Add item3:
    //      Rectangle {
    //        id: item3
    //        height: 80
    //      }

    ModelNode item3Node = view->createModelNode("Qt/Rectangle", 4, 6);
    item3Node.setId("item3");
    item3Node.variantProperty("height").setValue(80);
    rootNode.nodeListProperty("data").reparentHere(item3Node);

    // Change item1.height: item3.height

    item1Node.bindingProperty("height").setExpression("item3.height");
    QCOMPARE(item1Instance.property("height").toInt(), 80);
}


void TestCore::testInstancesNotInScene()
{
    //
    // test whether deleting an instance which is not in the scene crashes
    //

    QScopedPointer<Model> model(Model::create("Qt/Rectangle", 4, 6));
    QVERIFY(model.data());
    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode node1 = view->createModelNode("Qt/Item", 4, 6);
    node1.setId("node1");

    ModelNode node2 = view->createModelNode("Qt/Item", 4, 6);
    node2.setId("node2");

    node1.nodeListProperty("children").reparentHere(node2);

    node1.destroy();
}

void TestCore::testQmlModelStatesInvalidForRemovedNodes()
{
    QScopedPointer<Model> model(Model::create("Qt/Rectangle", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QVERIFY(QmlItemNode(rootModelNode).isValid());

    rootModelNode.setId("rootModelNode");

    QmlModelState state1 = QmlItemNode(rootModelNode).states().addState("state1");
    QVERIFY(state1.isValid());
    QCOMPARE(state1.name(), QString("state1"));

    ModelNode childNode = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    QVERIFY(childNode.isValid());
    childNode.setId("childNode");

    ModelNode subChildNode = childNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    QVERIFY(subChildNode.isValid());
    subChildNode.setId("subChildNode");

    QVERIFY(!state1.hasPropertyChanges(subChildNode));
    QmlPropertyChanges changeSet = state1.propertyChanges(subChildNode);
    QVERIFY(changeSet.isValid());

    QmlItemNode(childNode).destroy();
    QVERIFY(!childNode.isValid());
    QVERIFY(!subChildNode.isValid());
    QVERIFY(!changeSet.isValid());
}

void TestCore::testInstancesAttachToExistingModel()
{
    //
    // Test attaching nodeinstanceview to an existing model
    //

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootNode = view->rootModelNode();
    ModelNode rectangleNode = rootNode.addChildNode("Qt/Rectangle", 4, 6, "data");

    rectangleNode.variantProperty("width").setValue(100);

    QVERIFY(rectangleNode.isValid());
    QVERIFY(rectangleNode.hasProperty("width"));

    // Attach NodeInstanceView

    QScopedPointer<NodeInstanceView> instanceView(new NodeInstanceView);
    QVERIFY(instanceView.data());
    model->attachView(instanceView.data());

    NodeInstance rootInstance = instanceView->instanceForNode(rootNode);
    NodeInstance rectangleInstance = instanceView->instanceForNode(rectangleNode);
    QVERIFY(rootInstance.isValid());
    QVERIFY(rectangleInstance.isValid());
    QCOMPARE(QVariant(100), rectangleInstance.property("width"));
    QVERIFY(rootInstance.testHandle());
    QVERIFY(rectangleInstance.testHandle());
    QCOMPARE(rootInstance.testHandle(), rectangleInstance.testHandle()->parent());
}

void TestCore::testQmlModelAddMultipleStates()
{
    QScopedPointer<Model> model(Model::create("Qt/Rectangle", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QVERIFY(QmlItemNode(rootModelNode).isValid());

    QmlModelState state1 = QmlItemNode(rootModelNode).states().addState("state1");
    QVERIFY(state1.isValid());
    QCOMPARE(state1.name(), QString("state1"));

    QmlModelState state2 = QmlItemNode(rootModelNode).states().addState("state2");
    QVERIFY(state2.isValid());
    QCOMPARE(state1.name(), QString("state1"));
    QCOMPARE(state2.name(), QString("state2"));

    QmlModelState state3 = QmlItemNode(rootModelNode).states().addState("state3");
    QVERIFY(state3.isValid());
    QCOMPARE(state1.name(), QString("state1"));
    QCOMPARE(state2.name(), QString("state2"));
    QCOMPARE(state3.name(), QString("state3"));

    QCOMPARE(QmlItemNode(rootModelNode).states().allStates().count(), 3);
}

void TestCore::testQmlModelRemoveStates()
{
    QScopedPointer<Model> model(Model::create("Qt/Rectangle", 4, 6));

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QVERIFY(QmlItemNode(rootModelNode).isValid());

    QmlModelState state1 = QmlItemNode(rootModelNode).states().addState("state1");
    QmlModelState state2 = QmlItemNode(rootModelNode).states().addState("state2");
    QmlModelState state3 = QmlItemNode(rootModelNode).states().addState("state3");

    QCOMPARE(QmlItemNode(rootModelNode).states().allStates().count(), 3);

    QmlItemNode(rootModelNode).states().removeState("state2");
    QVERIFY(!state2.isValid());
    QCOMPARE(QmlItemNode(rootModelNode).states().allStates().count(), 2);

    QmlItemNode(rootModelNode).states().removeState("state3");
    QVERIFY(!state3.isValid());
    state1.modelNode().destroy();
    QVERIFY(!state1.isValid());
    QCOMPARE(QmlItemNode(rootModelNode).states().allStates().count(), 0);
}

void TestCore::testQmlModelStateWithName()
{
    QPlainTextEdit textEdit1;
    textEdit1.setPlainText("import Qt 4.6; Rectangle { id: theRect; width: 100; states: [ State { name: \"a\"; PropertyChanges { target: theRect; width: 200; } } ] }\n");
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model1(Model::create("Qt/Item"));

    TestRewriterView *testRewriterView1 = new TestRewriterView(model1.data());
    testRewriterView1->setTextModifier(&modifier1);
    model1->attachView(testRewriterView1);

    QVERIFY(testRewriterView1->errors().isEmpty());

    TestView *view = new TestView(model1.data());
    model1->attachView(view);
    QmlItemNode rootNode = view->rootModelNode();
    QCOMPARE(rootNode.states().allStates().size(), 1);

    QmlModelState modelState = rootNode.states().allStates().at(0);
    QVERIFY(!modelState.isBaseState());
    QCOMPARE(rootNode.instanceValue("width").toInt(), 100);

    QVERIFY(rootNode.isInBaseState());
    QVERIFY(rootNode.propertyAffectedByCurrentState("width"));
    view->setCurrentState(rootNode.states().allStates().at(0));
    rootNode.setVariantProperty("width", 112);

    const QLatin1String expected1("import Qt 4.6; Rectangle { id: theRect; width: 100; states: [ State { name: \"a\"; PropertyChanges { target: theRect; width: 112 } } ] }\n");
    QCOMPARE(textEdit1.toPlainText(), expected1);

    QVERIFY(!rootNode.isInBaseState());
    QCOMPARE(rootNode.instanceValue("width").toInt(), 112);
    
    view->setCurrentState(view->baseState());
    QCOMPARE(rootNode.instanceValue("width").toInt(), 100);

    view->setCurrentState(rootNode.states().allStates().at(0));
    QCOMPARE(rootNode.instanceValue("width").toInt(), 112);


    modelState.destroy();
    QCOMPARE(rootNode.states().allStates().size(), 0);
}

void TestCore::testRewriterAutomaticSemicolonAfterChangedProperty()
{
    QPlainTextEdit textEdit1;
    textEdit1.setPlainText("import Qt 4.6; Rectangle {\n    width: 640\n    height: 480\n}\n");
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model1(Model::create("Qt/Item"));

    TestRewriterView *testRewriterView1 = new TestRewriterView(model1.data());
    testRewriterView1->setTextModifier(&modifier1);
    model1->attachView(testRewriterView1);

    QVERIFY(testRewriterView1->errors().isEmpty());

    ModelNode rootModelNode = testRewriterView1->rootModelNode();
    rootModelNode.variantProperty("height").setValue(1480);
    QCOMPARE(rootModelNode.variantProperty("height").value().toInt(), 1480);

    QVERIFY(testRewriterView1->errors().isEmpty());
}

void TestCore::attributeChangeSynchronizer()
{
    QSKIP("no rewriter anymore", SkipAll);
//    ByteArrayModifier* modifier;
//    Model* model = 0;
//    const QString originalQml = contentsTemplate.arg("text", "");
//    load(originalQml, model, modifier);
//    QVERIFY(model);
//    QCOMPARE(view->rootModelNode().childNodes().size(), 1);
//    ModelNode textChild = view->rootModelNode().childNodes().first();
//    QVERIFY(textChild.id() == "textChild");
//    QVERIFY(textChild.property("text").value() == "text");
//
//    reload(contentsTemplate.arg("changed text", ""), modifier);
//
//    QVERIFY2(textChild.property("text").value() == "changed text", QString("text was: \"%1\", expected \"changed text\"").arg(textChild.property("text").value().toString()).toLatin1().data());
//
//    textChild = view->rootModelNode().childNodes().first();
//    QVERIFY(textChild.property("text").value() == "changed text");
//
//    delete model;
//    delete modifier;
}

void TestCore::attributeAdditionSynchronizer()
{
        QSKIP("no rewriter anymore", SkipAll);
//    ByteArrayModifier* modifier;
//    Model* model = 0;
//    load(contentsTemplate.arg("text", ""), model, modifier);
//    QVERIFY(model);
//
//    ModelNode textChild = view->rootModelNode().childNodes().first();
//    QVERIFY(textChild.id() == "textChild");
//    QVERIFY(textChild.property("text").value() == "text");
//    QVERIFY(!textChild.propertyNames().contains("width"));
//
//    reload(contentsTemplate.arg("text", "width: 100;"),  modifier);
//
//    QVERIFY(textChild.property("text").value() == "text");
//    QVERIFY(textChild.property("width").value() == "100");
//
//    delete model;
//    delete modifier;
}

void TestCore::attributeRemovalSynchronizer()
{
    QSKIP("no rewriter anymore", SkipAll);
//    ByteArrayModifier* modifier;
//    Model* model = 0;
//    load(contentsTemplate.arg("text", "width: 50;"), model, modifier);
//    QVERIFY(model);
//
//    ModelNode textChild = view->rootModelNode().childNodes().first();
//    QVERIFY(textChild.id() == "textChild");
//    QVERIFY(textChild.property("text").value() == "text");
//    QVERIFY(textChild.property("width").value() == "50");
//
//    reload(contentsTemplate.arg("text", ""), modifier);
//
//    QVERIFY(textChild.property("text").value() == "text");
//    QVERIFY(!textChild.propertyNames().contains("width"));
//
//    delete model;
//    delete modifier;
}

void TestCore::childAddedSynchronizer()
{
    QSKIP("no rewriter anymore", SkipAll);
//    ByteArrayModifier* modifier;
//    Model* model = 0;
//    load(bareTemplate.arg(""), model, modifier);
//    QVERIFY(model);
//    QVERIFY(view->rootModelNode().childNodes().isEmpty());
//
//    reload(contentsTemplate.arg("text", ""), modifier);
//
//    QVERIFY(view->rootModelNode().childNodes().size() == 1);
//    QVERIFY(view->rootModelNode().childNodes().first().id() == "textChild");
//
//    delete model;
//    delete modifier;
}

void TestCore::childRemovedSynchronizer()
{
    QSKIP("no rewriter anymore", SkipAll);
//    ByteArrayModifier* modifier;
//    Model* model = 0;
//    load(contentsTemplate.arg("text", ""), model, modifier);
//    QVERIFY(model);
//    QVERIFY(view->rootModelNode().childNodes().size() == 1);
//    QVERIFY(view->rootModelNode().childNodes().first().id() == "textChild");
//
//    reload(bareTemplate.arg(""), modifier);
//
//    QVERIFY(view->rootModelNode().childNodes().isEmpty());
//
//    delete model;
//    delete modifier;
}

void TestCore::childReplacedSynchronizer()
{
        QSKIP("no rewriter anymore", SkipAll);
//    ByteArrayModifier* modifier;
//    Model *model = 0;
//    load(contentsTemplate.arg("text", ""), model, modifier);
//    QVERIFY(model);
//    QVERIFY(view->rootModelNode().childNodes().size() == 1);
//    QVERIFY(view->rootModelNode().childNodes().first().id() == "textChild");
//
//    reload(bareTemplate.arg("Item { id:\"someOtherItem\"; }"), modifier);
//
//    QVERIFY(view->rootModelNode().childNodes().size() == 1);
//    QVERIFY(view->rootModelNode().childNodes().first().id() == "someOtherItem");
//
//    delete model;
//    delete modifier;
}

void TestCore::defaultPropertyValues()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QCOMPARE(view->rootModelNode().variantProperty("x").value().toDouble(), 0.0);
    QCOMPARE(view->rootModelNode().variantProperty("width").value().toDouble(), 0.0);

    ModelNode rectNode(view->rootModelNode().addChildNode("Qt/Rectangle", 4, 6, "data"));

    QCOMPARE(rectNode.variantProperty("y").value().toDouble(), 0.0);
    QCOMPARE(rectNode.variantProperty("width").value().toDouble(), 0.0);

    ModelNode imageNode(view->rootModelNode().addChildNode("Qt/Image", 4, 6, "data"));

    QCOMPARE(imageNode.variantProperty("y").value().toDouble(), 0.0);
    QCOMPARE(imageNode.variantProperty("width").value().toDouble(), 0.0);
}

void TestCore::testModelPropertyValueTypes()
{
    QPlainTextEdit textEdit1;
    textEdit1.setPlainText("import Qt 4.6; Rectangle { width: 100; radius: 1.5; color: \"red\"; }");
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model1(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model1->attachView(testRewriterView1.data());

    QVERIFY(testRewriterView1->errors().isEmpty());

    ModelNode rootModelNode(testRewriterView1->rootModelNode());
    QVERIFY(rootModelNode.isValid());

    QCOMPARE(rootModelNode.variantProperty("width").value().type(), QVariant::Double);
    QCOMPARE(rootModelNode.variantProperty("radius").value().type(), QVariant::Double);
    QCOMPARE(rootModelNode.variantProperty("color").value().type(), QVariant::Color);
}

void TestCore::testModelNodeInHierarchy()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QVERIFY(view->rootModelNode().isInHierarchy());
    ModelNode node1 = view->rootModelNode().addChildNode("Qt/Item", 4, 6, "data");
    QVERIFY(node1.isInHierarchy());
    ModelNode node2 = view->createModelNode("Qt/Item", 4, 6);
    QVERIFY(!node2.isInHierarchy());
    node2.nodeListProperty("data").reparentHere(node1);
    QVERIFY(!node2.isInHierarchy());
    view->rootModelNode().nodeListProperty("data").reparentHere(node2);
    QVERIFY(node1.isInHierarchy());
    QVERIFY(node2.isInHierarchy());
}

void TestCore::testModelNodeIsAncestorOf()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    //
    //  import Qt 4.6
    //  Item {
    //    Item {
    //      id: item2
    //    }
    //    Item {
    //      id: item3
    //      Item {
    //        id: item4
    //      }
    //    }
    //  }
    //
    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    view->rootModelNode().setId("item1");
    ModelNode item2 = view->rootModelNode().addChildNode("Qt/Item", 4, 6, "data");
    item2.setId("item2");
    ModelNode item3 = view->rootModelNode().addChildNode("Qt/Item", 4, 6, "data");
    item3.setId("item3");
    ModelNode item4 = item3.addChildNode("Qt/Item", 4, 6, "data");
    item4.setId("item4");

    QVERIFY(view->rootModelNode().isAncestorOf(item2));
    QVERIFY(view->rootModelNode().isAncestorOf(item3));
    QVERIFY(view->rootModelNode().isAncestorOf(item4));
    QVERIFY(!item2.isAncestorOf(view->rootModelNode()));
    QVERIFY(!item2.isAncestorOf(item4));
    QVERIFY(item3.isAncestorOf(item4));
}

void TestCore::testModelDefaultProperties()
{
    QScopedPointer<Model> model(Model::create("Qt/Rectangle"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());

    QCOMPARE(rootModelNode.metaInfo().defaultProperty(), QString("data"));
}

void TestCore::loadAnchors()
{
    QPlainTextEdit textEdit1;
    textEdit1.setPlainText("import Qt 4.6; Item { width: 100; height: 100; Rectangle { anchors.left: parent.left; anchors.horizontalCenter: parent.horizontalCenter; anchors.rightMargin: 20; }}");
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model->attachView(testRewriterView1.data());

    TestView *testView = new TestView(model.data());
    model->attachView(testView);

    QmlItemNode rootQmlItemNode(testView->rootQmlItemNode());
    QmlItemNode anchoredNode(rootQmlItemNode.children().first());
    QVERIFY(anchoredNode.isValid());

    QmlAnchors anchors(anchoredNode.anchors());
    QVERIFY(anchors.instanceHasAnchor(AnchorLine::Left));
    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::Top));
    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::Right));
    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::Bottom));
    QVERIFY(anchors.instanceHasAnchor(AnchorLine::HorizontalCenter));
    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::VerticalCenter));

    QCOMPARE(anchors.instanceMargin(AnchorLine::Right), 20.0);


    QCOMPARE(anchoredNode.instancePosition().x(), 0.0);
    QCOMPARE(anchoredNode.instancePosition().y(), 0.0);
    QCOMPARE(anchoredNode.instanceBoundingRect().width(), 100.0);
    QCOMPARE(anchoredNode.instanceBoundingRect().height(), 0.0);


    model->detachView(testView);
}

void TestCore::changeAnchors()
{
    QPlainTextEdit textEdit1;
    textEdit1.setPlainText("import Qt 4.6; Item { width: 100; height: 100; Rectangle { anchors.left: parent.left; anchors.horizontalCenter: parent.horizontalCenter; anchors.rightMargin: 20; }}");
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model->attachView(testRewriterView1.data());

    TestView *testView = new TestView(model.data());
    model->attachView(testView);

    QmlItemNode rootQmlItemNode(testView->rootQmlItemNode());
    QmlItemNode anchoredNode(rootQmlItemNode.children().first());
    QVERIFY(anchoredNode.isValid());

    QmlAnchors anchors(anchoredNode.anchors());
    QVERIFY(anchors.instanceHasAnchor(AnchorLine::Left));
    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::Top));
    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::Right));
    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::Bottom));
    QVERIFY(anchors.instanceHasAnchor(AnchorLine::HorizontalCenter));
    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::VerticalCenter));
    QCOMPARE(anchors.instanceMargin(AnchorLine::Right), 20.0);


    QCOMPARE(anchoredNode.instancePosition().x(), 0.0);
    QCOMPARE(anchoredNode.instancePosition().y(), 0.0);
    QCOMPARE(anchoredNode.instanceBoundingRect().width(), 100.0);
    QCOMPARE(anchoredNode.instanceBoundingRect().height(), 0.0);


    anchors.removeAnchor(AnchorLine::Top);

    anchoredNode.setBindingProperty("anchors.bottom", "parent.bottom");
    QVERIFY(anchors.instanceHasAnchor(AnchorLine::Bottom));
    anchors.setAnchor(AnchorLine::Right, rootQmlItemNode, AnchorLine::Right);
    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::Right));
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Right).type(), AnchorLine::Invalid);
    QVERIFY(!anchors.instanceAnchor(AnchorLine::Right).qmlItemNode().isValid());

    anchors.setMargin(AnchorLine::Right, 10.0);
    QCOMPARE(anchors.instanceMargin(AnchorLine::Right), 10.0);

    anchors.setMargin(AnchorLine::Right, 0.0);
    QCOMPARE(anchors.instanceMargin(AnchorLine::Right), 0.0);

    anchors.setMargin(AnchorLine::Right, 20.0);
    QCOMPARE(anchors.instanceMargin(AnchorLine::Right), 20.0);

    QCOMPARE(anchoredNode.instancePosition().x(), 0.0);
    QCOMPARE(anchoredNode.instancePosition().y(), 100.0);
    QCOMPARE(anchoredNode.instanceBoundingRect().width(), 100.0);
    QCOMPARE(anchoredNode.instanceBoundingRect().height(), 0.0);

    model->detachView(testView);
}

void TestCore::anchorToSibling()
{
    QPlainTextEdit textEdit1;
    textEdit1.setPlainText("import Qt 4.6; Item { Rectangle {} Rectangle { id: secondChild } }");
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model->attachView(testRewriterView1.data());

    TestView *testView = new TestView(model.data());
    model->attachView(testView);

    QmlItemNode rootQmlItemNode(testView->rootQmlItemNode());
    QmlItemNode anchoredNode(rootQmlItemNode.children().first());
    QVERIFY(anchoredNode.isValid());

    QmlAnchors anchors(anchoredNode.anchors());

    QmlItemNode firstChild(rootQmlItemNode.children().first());
    QVERIFY(firstChild.isValid());
    QCOMPARE(firstChild.validId(), QString("rectangle1"));

    QmlItemNode secondChild(rootQmlItemNode.children().at(1));
    QVERIFY(secondChild.isValid());
    QCOMPARE(secondChild.validId(), QString("secondChild"));

    secondChild.anchors().setAnchor(AnchorLine::Top, firstChild, AnchorLine::Bottom);

    QmlItemNode secondChildTopAnchoredNode = secondChild.anchors().instanceAnchor(AnchorLine::Top).qmlItemNode();
    QVERIFY(secondChildTopAnchoredNode.isValid());
    QVERIFY2(secondChildTopAnchoredNode == firstChild, QString("expected %1 got %2").arg(firstChild.id(), secondChildTopAnchoredNode.id()).toLatin1().data());

    secondChild.anchors().setMargin(AnchorLine::Top, 10.0);
    QCOMPARE(secondChild.anchors().instanceMargin(AnchorLine::Top), 10.0);

    QVERIFY(firstChild.anchors().possibleAnchorLines(AnchorLine::Bottom, secondChild) == AnchorLine::Invalid);
    QVERIFY(firstChild.anchors().possibleAnchorLines(AnchorLine::Left, secondChild) == AnchorLine::HorizontalMask);
    QVERIFY(secondChild.anchors().possibleAnchorLines(AnchorLine::Top, firstChild) == AnchorLine::VerticalMask);
    QVERIFY(secondChild.anchors().possibleAnchorLines(AnchorLine::Left, firstChild) == AnchorLine::HorizontalMask);
}

void TestCore::anchorsAndStates()
{
    QSKIP("no anchor support anymore", SkipAll);

//    QScopedPointer<Model> model(create("import Qt 4.6; Item { id:rootModelNode }"));
//    QVERIFY(model.data());
//
//    ModelNode childNode(view->rootModelNode().addChildNode("Qt/Rectangle", 4, 6, "data"));
//    childNode.setId("childNode");
//    ModelState modelState(model->createModelState("anchoreteststate"));
//
//    childNode.setId("childNode");
//    NodeState nodeState(modelState.nodeState(childNode));
//    NodeAnchors(nodeState).setAnchor(AnchorLine::Top, view->rootModelNode(), AnchorLine::Top);
}

void TestCore::removeFillAnchorByDetaching()
{
    QPlainTextEdit textEdit1;
    textEdit1.setPlainText("import Qt 4.6; Item { width: 100; height: 100; Rectangle { id: child; anchors.fill: parent } }");
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model->attachView(testRewriterView1.data());

    TestView *testView = new TestView(model.data());
    model->attachView(testView);

    QmlItemNode rootQmlItemNode(testView->rootQmlItemNode());

    QmlItemNode firstChild(rootQmlItemNode.children().first());
    QVERIFY(firstChild.isValid());
    QCOMPARE(firstChild.id(), QString("child"));

    // Verify the synthesized anchors:
    QmlAnchors anchors(firstChild);
    QVERIFY(anchors.instanceHasAnchor(AnchorLine::Left));
    QVERIFY(anchors.instanceAnchor(AnchorLine::Left).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Left).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Left).type(), AnchorLine::Left);

    QVERIFY(anchors.instanceAnchor(AnchorLine::Top).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Top).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Top).type(), AnchorLine::Top);

    QVERIFY(anchors.instanceAnchor(AnchorLine::Right).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Right).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Right).type(), AnchorLine::Right);

    QVERIFY(anchors.instanceAnchor(AnchorLine::Bottom).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Bottom).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Bottom).type(), AnchorLine::Bottom);

    QVERIFY(!anchors.instanceAnchor(AnchorLine::HorizontalCenter).isValid());
    QVERIFY(!anchors.instanceAnchor(AnchorLine::VerticalCenter).isValid());

    QCOMPARE(firstChild.instancePosition().x(), 0.0);
    QCOMPARE(firstChild.instancePosition().y(), 0.0);
    QCOMPARE(firstChild.instanceBoundingRect().width(), 100.0);
    QCOMPARE(firstChild.instanceBoundingRect().height(), 100.0);

    // Remove 2 anchors:
    RewriterTransaction transaction = testView->beginRewriterTransaction();
    anchors.removeAnchor(AnchorLine::Bottom);
    anchors.removeAnchor(AnchorLine::Top);
    transaction.commit();

    // Verify again:
    QVERIFY(anchors.instanceAnchor(AnchorLine::Left).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Left).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Left).type(), AnchorLine::Left);

    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::Top));
    QVERIFY(!anchors.instanceAnchor(AnchorLine::Top).isValid());

    QVERIFY(anchors.instanceAnchor(AnchorLine::Right).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Right).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Right).type(), AnchorLine::Right);

    QVERIFY(!anchors.instanceAnchor(AnchorLine::Bottom).isValid());

    QVERIFY(!anchors.instanceAnchor(AnchorLine::HorizontalCenter).isValid());
    QVERIFY(!anchors.instanceAnchor(AnchorLine::VerticalCenter).isValid());

    QCOMPARE(firstChild.instancePosition().x(), 0.0);
    QCOMPARE(firstChild.instancePosition().y(), 0.0);
    QCOMPARE(firstChild.instanceBoundingRect().width(), 100.0);
    QSKIP("is this intended", SkipAll);
    QCOMPARE(firstChild.instanceBoundingRect().height(), 0.0);


    model->detachView(testView);
}

void TestCore::removeFillAnchorByChanging()
{
    QPlainTextEdit textEdit1;
    textEdit1.setPlainText("import Qt 4.6; Item { width: 100; height: 100; Rectangle { id: child; anchors.fill: parent } }");
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model->attachView(testRewriterView1.data());

    TestView *testView = new TestView(model.data());
    model->attachView(testView);

    QmlItemNode rootQmlItemNode(testView->rootQmlItemNode());

    QmlItemNode firstChild(rootQmlItemNode.children().first());
    QVERIFY(firstChild.isValid());
    QCOMPARE(firstChild.id(), QString("child"));

    // Verify the synthesized anchors:
    QmlAnchors anchors(firstChild);
    QVERIFY(anchors.instanceAnchor(AnchorLine::Left).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Left).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Left).type(), AnchorLine::Left);

    QVERIFY(anchors.instanceAnchor(AnchorLine::Top).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Top).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Top).type(), AnchorLine::Top);

    QVERIFY(anchors.instanceAnchor(AnchorLine::Right).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Right).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Right).type(), AnchorLine::Right);

    QVERIFY(anchors.instanceAnchor(AnchorLine::Bottom).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Bottom).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Bottom).type(), AnchorLine::Bottom);

    QVERIFY(!anchors.instanceAnchor(AnchorLine::HorizontalCenter).isValid());
    QVERIFY(!anchors.instanceAnchor(AnchorLine::VerticalCenter).isValid());

    QCOMPARE(firstChild.instancePosition().x(), 0.0);
    QCOMPARE(firstChild.instancePosition().y(), 0.0);
    QCOMPARE(firstChild.instanceBoundingRect().width(), 100.0);
    QCOMPARE(firstChild.instanceBoundingRect().height(), 100.0);

    // Change 2 anchors:
    anchors.setAnchor(AnchorLine::Bottom, rootQmlItemNode, AnchorLine::Top);
    anchors.setAnchor(AnchorLine::Top, rootQmlItemNode, AnchorLine::Bottom);

    // Verify again:
    QVERIFY(anchors.instanceAnchor(AnchorLine::Left).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Left).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Left).type(), AnchorLine::Left);

    QVERIFY(anchors.instanceAnchor(AnchorLine::Top).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Top).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Top).type(), AnchorLine::Bottom);

    QVERIFY(anchors.instanceAnchor(AnchorLine::Right).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Right).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Right).type(), AnchorLine::Right);

    QVERIFY(anchors.instanceAnchor(AnchorLine::Bottom).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Bottom).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::Bottom).type(), AnchorLine::Top);

    QVERIFY(!anchors.instanceAnchor(AnchorLine::HorizontalCenter).isValid());
    QVERIFY(!anchors.instanceAnchor(AnchorLine::VerticalCenter).isValid());


    QCOMPARE(firstChild.instancePosition().x(), 0.0);
    QCOMPARE(firstChild.instancePosition().y(), 100.0);
    QCOMPARE(firstChild.instanceBoundingRect().width(), 100.0);
    QCOMPARE(firstChild.instanceBoundingRect().height(), -100.0);


    model->detachView(testView);
}

void TestCore::testModelBindings()
{
    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    NodeInstanceView *nodeInstanceView = new NodeInstanceView(model.data());
    model->attachView(nodeInstanceView);

    ModelNode rootModelNode = nodeInstanceView->rootModelNode();
    QCOMPARE(rootModelNode.allDirectSubModelNodes().count(), 0);
    NodeInstance rootInstance = nodeInstanceView->instanceForNode(rootModelNode);

    QCOMPARE(rootInstance.size().width(), 0.0);
    QCOMPARE(rootInstance.size().height(), 0.0);

    rootModelNode.variantProperty("width") = 200;
    rootModelNode.variantProperty("height") = 100;

    QCOMPARE(rootInstance.size().width(), 200.0);
    QCOMPARE(rootInstance.size().height(), 100.0);

    ModelNode childNode = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");

    childNode.variantProperty("width") = 100;
    childNode.variantProperty("height") = 100;

    childNode.variantProperty("x") = 10;
    childNode.variantProperty("y") = 10;

    NodeInstance childInstance = nodeInstanceView->instanceForNode(childNode);

    QCOMPARE(childInstance.size().width(), 100.0);
    QCOMPARE(childInstance.size().height(), 100.0);

    QCOMPARE(childInstance.position().x(), 10.0);
    QCOMPARE(childInstance.position().y(), 10.0);

    childNode.bindingProperty("width").setExpression("parent.width");

    QCOMPARE(childInstance.size().width(), 200.0);

    rootModelNode.setId("root");
    QCOMPARE(rootModelNode.id(), QString("root"));
    QCOMPARE(rootInstance.testHandle()->objectName(), QString("root"));

    childNode.setId("child");
    QCOMPARE(childNode.id(), QString("child"));
    QCOMPARE(childInstance.testHandle()->objectName(), QString("child"));
    childNode.variantProperty("width") = 100;

    QCOMPARE(childInstance.size().width(), 100.0);

    childNode.bindingProperty("width").setExpression("child.height");
    QCOMPARE(childInstance.size().width(), 100.0);

    childNode.bindingProperty("width").setExpression("root.width");
    QCOMPARE(childInstance.size().width(), 200.0);
}

void TestCore::testDynamicProperties()
{
    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    TestView *testView = new TestView(model.data());
    model->attachView(testView);        

    QmlItemNode rootQmlItemNode(testView->rootQmlItemNode());
    ModelNode rootModelNode = rootQmlItemNode.modelNode();

    rootModelNode.variantProperty("x") = 10;
    rootModelNode.variantProperty("myDouble") = qMakePair(QString("real"), QVariant(10));
    rootModelNode.variantProperty("myColor").setDynamicTypeNameAndValue("color", Qt::red);

    QVERIFY(!rootModelNode.property("x").isDynamic());
    QVERIFY(rootModelNode.property("myColor").isDynamic());
    QVERIFY(rootModelNode.property("myDouble").isDynamic());

    QCOMPARE(rootModelNode.property("myColor").dynamicTypeName(), QString("color"));
    QCOMPARE(rootModelNode.variantProperty("myColor").value(), QVariant(Qt::red));
    //QCOMPARE(rootQmlItemNode.instanceValue("myColor"), QVariant(Qt::red)); //not working yet
    QCOMPARE(rootModelNode.property("myDouble").dynamicTypeName(), QString("real"));
    QCOMPARE(rootModelNode.variantProperty("myDouble").value(), QVariant(10));
    //QCOMPARE(rootQmlItemNode.instanceValue("myDouble"), QVariant(10)); //not working yet

    QCOMPARE(rootModelNode.property("x").dynamicTypeName(), QString());

    rootModelNode.variantProperty("myDouble") = QVariant(10);
    QVERIFY(!rootModelNode.property("myDouble").isDynamic());

    rootModelNode.bindingProperty("myBindingDouble") = qMakePair(QString("real"), QString("myDouble"));
    rootModelNode.bindingProperty("myBindingColor").setDynamicTypeNameAndExpression("color", "myColor");

    QCOMPARE(rootModelNode.property("myBindingColor").dynamicTypeName(), QString("color"));
    QCOMPARE(rootModelNode.property("myBindingDouble").dynamicTypeName(), QString("real"));

    QVERIFY(rootModelNode.property("myBindingDouble").isDynamic());
    QVERIFY(rootModelNode.property("myBindingColor").isDynamic());

    QCOMPARE(rootModelNode.bindingProperty("myBindingDouble").expression(), QString("myDouble"));
    QCOMPARE(rootModelNode.bindingProperty("myBindingColor").expression(), QString("myColor"));
}

void TestCore::sideIndex()
{
    QSKIP("not anymore needed", SkipAll);
//    QList<QmlError> errors;
//
//    char qmlString[] = "import Qt 4.6\n"
//                       "Rectangle {\n"
//                       "    width: 200\n"
//                       "    height: 200\n"
//                       "    color: \"white\"\n"
//                       "Text {\n"
//                       "    text: \"Hello World\"\n"
//                       "    anchors.centerIn: parent\n"
//                       "}\n"
//                       " Text {\n"
//                       "   text: \"text\"\n"
//                       "    x: 66\n"
//                       "    y: 43\n"
//                       "    width: 80\n"
//                       "    height: 20\n"
//                       "    id: Text1;\n"
//                       "}\n"
//                       "  Text {\n"
//                       "    text: \"text\"\n"
//                       "    width: 80\n"
//                       "    height: 20\n"
//                       "    x: 66\n"
//                       "    y: 141\n"
//                       "    id: Text2;\n"
//                       "}\n"
//                       "   Text {\n"
//                       "    height: 20\n"
//                       "    text: \"text\"\n"
//                       "    x: 74\n"
//                       "    width: 80\n"
//                       "    y: 68\n"
//                       "    id: Text3;\n"
//                       "}\n"
//                       "}\n";
//
//    QScopedPointer<ByteArrayModifier> modifier1(ByteArrayModifier::create(QLatin1String(qmlString)));
//    QScopedPointer<Model> model(Model::create(modifier1.data(), QUrl(), &errors));
//    foreach(QmlError error, errors) {
//        QFAIL((QString("Line %1: %2").arg(error.line()).arg(error.description())).toLatin1());
//    }
//
//    QVERIFY(model.data());
//
//    QCOMPARE(view->rootModelNode().childNodes().at(0).property("text").value().toString(), QString("Hello World"));
//    QCOMPARE(view->rootModelNode().childNodes().at(1).id(), QString("Text1"));
//    QCOMPARE(view->rootModelNode().childNodes().at(2).id(), QString("Text2"));
//    QCOMPARE(view->rootModelNode().childNodes().at(3).id(), QString("Text3"));
//
//
//    ModelNode childModelNode(view->rootModelNode().childNodes().at(2));
//    ModelNode childModelNode2(view->rootModelNode().childNodes().at(1));
//
//    QCOMPARE(childModelNode.id(), QString("Text2"));
//    QCOMPARE(childModelNode2.id(), QString("Text1"));
//
//    childModelNode.slideToIndex(0);
//    QCOMPARE(childModelNode.id(), QString("Text2"));
//    QCOMPARE(childModelNode2.id(), QString("Text1"));
//    QCOMPARE(view->rootModelNode().childNodes().at(0).id(), QString("Text2"));
//    QCOMPARE(view->rootModelNode().childNodes().at(1).property("text").value().toString(), QString("Hello World"));
//    QCOMPARE(view->rootModelNode().childNodes().at(2).id(), QString("Text1"));
//    QCOMPARE(view->rootModelNode().childNodes().at(3).id(), QString("Text3"));

}

void TestCore::testModelSliding()
{
    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());

    ModelNode rect00(rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data"));
    ModelNode rect01(rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data"));
    ModelNode rect02(rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data"));
    ModelNode rect03(rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data"));

    QVERIFY(rect00.isValid());
    QVERIFY(rect01.isValid());
    QVERIFY(rect02.isValid());
    QVERIFY(rect03.isValid());

    NodeListProperty listProperty(rootModelNode.nodeListProperty("data"));
    QVERIFY(!listProperty.isEmpty());

    QList<ModelNode> nodeList(listProperty.toModelNodeList());

    QCOMPARE(rect00, nodeList.at(0));
    QCOMPARE(rect01, nodeList.at(1));
    QCOMPARE(rect02, nodeList.at(2));
    QCOMPARE(rect03, nodeList.at(3));

    listProperty.slide(3,0);

    nodeList = listProperty.toModelNodeList();

    QCOMPARE(rect03, nodeList.at(0));
    QCOMPARE(rect00, nodeList.at(1));
    QCOMPARE(rect01, nodeList.at(2));
    QCOMPARE(rect02, nodeList.at(3));

    NodeListProperty childrenProperty(rootModelNode.nodeListProperty("children"));

    childrenProperty.reparentHere(rect00);
    childrenProperty.reparentHere(rect01);
    childrenProperty.reparentHere(rect02);
    childrenProperty.reparentHere(rect03);

    QVERIFY(listProperty.isEmpty());

    QVERIFY(!childrenProperty.isEmpty());

    nodeList = childrenProperty.toModelNodeList();

    QCOMPARE(rect00, nodeList.at(0));
    QCOMPARE(rect01, nodeList.at(1));
    QCOMPARE(rect02, nodeList.at(2));
    QCOMPARE(rect03, nodeList.at(3));

    childrenProperty.slide(0,3);

    nodeList = childrenProperty.toModelNodeList();

    QCOMPARE(rect01, nodeList.at(0));
    QCOMPARE(rect02, nodeList.at(1));
    QCOMPARE(rect03, nodeList.at(2));
    QCOMPARE(rect00, nodeList.at(3));
}

void TestCore::testRewriterChangeId()
{
    const char* qmlString = "import Qt 4.6\nRectangle { }";

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.id(), QString());

    rootModelNode.setId("rectId");

    QCOMPARE(rootModelNode.id(), QString("rectId"));

    QString expected = "import Qt 4.6\n"
                       "Rectangle {\n"
                       "    id: rectId\n"
                       " }";

    QCOMPARE(textEdit.toPlainText(), expected);

    // change id for node outside of hierarchy
    ModelNode node = view->createModelNode("Qt/Item", 4, 6);
    node.setId("myId");
}

void TestCore::testRewriterChangeValueProperty()
{
    const char* qmlString = "import Qt 4.6\nRectangle { x: 10; y: 10 }";

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());

    QCOMPARE(rootModelNode.property("x").toVariantProperty().value().toInt(), 10);

    rootModelNode.property("x").toVariantProperty().setValue(2);

    QCOMPARE(textEdit.toPlainText(), QString(QLatin1String(qmlString)).replace("x: 10;", "x: 2;"));

    // change property for node outside of hierarchy
    PropertyListType properties;
    properties.append(QPair<QString,QVariant>("x", 10));
    ModelNode node = view->createModelNode("Qt/Item", 4, 6, properties);
    node.variantProperty("x").setValue(20);
}

void TestCore::testRewriterRemoveValueProperty()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "Rectangle {\n"
                                  "  x: 10\n"
                                  "  y: 10;\n"
                                  "}\n");

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());

    QCOMPARE(rootModelNode.property("x").toVariantProperty().value().toInt(), 10);

    rootModelNode.property("x").toVariantProperty().setValue(2);

    rootModelNode.removeProperty("x");

    const QLatin1String expected("\n"
                                 "import Qt 4.6\n"
                                 "Rectangle {\n"
                                 "  y: 10;\n"
                                 "}\n");
    QCOMPARE(textEdit.toPlainText(), expected);

    // remove property for node outside of hierarchy
    PropertyListType properties;
    properties.append(QPair<QString,QVariant>("x", 10));
    ModelNode node = view->createModelNode("Qt/Item", 4, 6, properties);
    node.removeProperty("x");
}

void TestCore::testRewriterSignalProperty()
{
    const char* qmlString = "import Qt 4.6\nRectangle { onColorChanged: {} }";

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());

    // Signal properties are ignored for the time being
    QCOMPARE(rootModelNode.properties().size(), 0);
}

void TestCore::testRewriterObjectTypeProperty()
{
    const char* qmlString = "import Qt 4.6\nRectangle { x: 10; y: 10 }";

    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    QVERIFY(view.data());
    model->attachView(view.data());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootModelNode(view->rootModelNode());
    QVERIFY(rootModelNode.isValid());

    QCOMPARE(rootModelNode.type(), QLatin1String("Qt/Rectangle"));

    rootModelNode.changeType(QLatin1String("Qt/Text"), 4, 6);

    QCOMPARE(rootModelNode.type(), QLatin1String("Qt/Text"));

    // change type outside of hierarchy
    ModelNode node = view->createModelNode("Qt/Rectangle", 4, 6);
    node.changeType(QLatin1String("Qt/Item"), 4, 6);
}

void TestCore::testRewriterPropertyChanges()
{
    try {
        // PropertyChanges uses a custom parser

        // Use a slightly more complicated example so that target properties are not resolved in default scope
        const char* qmlString
                = "import Qt 4.6\n"
                  "Rectangle {\n"
                  "  Text {\n"
                  "    id: targetObj\n"
                  "    text: \"base State\"\n"
                  "  }\n"
                  "  states: [\n"
                  "    State {\n"
                  "      PropertyChanges {\n"
                  "        target: targetObj\n"
                  "        text: \"State 1\"\n"
                  "      }\n"
                  "    }\n"
                  "  ]\n"
                  "}\n";

        QPlainTextEdit textEdit;
        textEdit.setPlainText(qmlString);
        PlainTextEditModifier textModifier(&textEdit);

        QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
        QVERIFY(model.data());

        QScopedPointer<TestView> view(new TestView);
        model->attachView(view.data());

        // read in
        QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
        testRewriterView->setTextModifier(&textModifier);
        model->attachView(testRewriterView.data());

        ModelNode rootNode = view->rootModelNode();
        QVERIFY(rootNode.isValid());
        QCOMPARE(rootNode.type(), QString("Qt/Rectangle"));
        QVERIFY(rootNode.propertyNames().contains(QLatin1String("data")));
        QVERIFY(rootNode.propertyNames().contains(QLatin1String("states")));
        QCOMPARE(rootNode.propertyNames().count(), 2);

        NodeListProperty statesProperty = rootNode.nodeListProperty("states");
        QVERIFY(statesProperty.isValid());
        QCOMPARE(statesProperty.toModelNodeList().size(), 1);

        ModelNode stateNode = statesProperty.toModelNodeList().first();
        QVERIFY(stateNode.isValid());
        QCOMPARE(stateNode.type(), QString("Qt/State"));
        QCOMPARE(stateNode.propertyNames(), QStringList("changes"));

        NodeListProperty stateChangesProperty = stateNode.property("changes").toNodeListProperty();
        QVERIFY(stateChangesProperty.isValid());

        ModelNode propertyChangesNode = stateChangesProperty.toModelNodeList().first();
        QVERIFY(propertyChangesNode.isValid());
        QCOMPARE(propertyChangesNode.properties().size(), 2);

        BindingProperty targetProperty = propertyChangesNode.bindingProperty("target");
        QVERIFY(targetProperty.isValid());

        VariantProperty textProperty = propertyChangesNode.variantProperty("text");
        QVERIFY(textProperty.isValid());
        QCOMPARE(textProperty.value().toString(), QLatin1String("State 1"));
    } catch (Exception &e) {
        QFAIL(e.description().toAscii().data());
    }
}

void TestCore::testRewriterListModel()
{
    QSKIP("See BAUHAUS-157", SkipAll);

    try {
        // ListModel uses a custom parser
        const char* qmlString = "import Qt 4.6; ListModel {\n ListElement {\n age: 12\n} \n}";

        QPlainTextEdit textEdit;
        textEdit.setPlainText(qmlString);
        PlainTextEditModifier textModifier(&textEdit);

        QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
        QVERIFY(model.data());

        QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
        testRewriterView->setTextModifier(&textModifier);
        model->attachView(testRewriterView.data());

        QScopedPointer<TestView> view(new TestView);
        model->attachView(view.data());

        ModelNode listModelNode = view->rootModelNode();
        QCOMPARE(listModelNode.propertyNames(), QStringList() << "__elements"); // TODO: what should be the name?

        NodeListProperty elementListProperty = listModelNode.nodeListProperty("__elements");
        QCOMPARE(elementListProperty.toModelNodeList().size(), 1);

        ModelNode elementNode = elementListProperty.toModelNodeList().at(0);
        QVERIFY(elementNode.isValid());
        QCOMPARE(elementNode.properties().size(), 1);
        QVERIFY(elementNode.variantProperty("age").isValid());
        QCOMPARE(elementNode.variantProperty("age").value().toInt(), 12);
    } catch (Exception &e) {
        QFAIL(qPrintable(e.description()));
    }
}

void TestCore::testRewriterAddProperty()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "}");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QString("Qt/Rectangle"));

    rootNode.variantProperty(QLatin1String("x")).setValue(123);

    QVERIFY(rootNode.hasProperty(QLatin1String("x")));
    QVERIFY(rootNode.property(QLatin1String("x")).isVariantProperty());
    QCOMPARE(rootNode.variantProperty(QLatin1String("x")).value(), QVariant(123));

    const QLatin1String expected("\n"
                                 "import Qt 4.6\n"
                                 "\n"
                                 "Rectangle {\n"
                                 "    x: 123\n"
                                 "}");
    QCOMPARE(textEdit.toPlainText(), expected);
}

void TestCore::testRewriterAddPropertyInNestedObject()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "    Rectangle {\n"
                                  "        id: Rectangle1\n"
                                  "    }\n"
                                  "}");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QLatin1String("Qt/Rectangle"));

    ModelNode childNode = rootNode.nodeListProperty(QLatin1String("data")).toModelNodeList().at(0);
    QVERIFY(childNode.isValid());
    QCOMPARE(childNode.type(), QLatin1String("Qt/Rectangle"));
    QCOMPARE(childNode.id(), QLatin1String("Rectangle1"));

    childNode.variantProperty(QLatin1String("x")).setValue(10);
    childNode.variantProperty(QLatin1String("y")).setValue(10);

    const QLatin1String expected("\n"
                                 "import Qt 4.6\n"
                                 "\n"
                                 "Rectangle {\n"
                                 "    Rectangle {\n"
                                 "        id: Rectangle1\n"
                                 "        x: 10\n"
                                 "        y: 10\n"
                                 "    }\n"
                                 "}");
    QCOMPARE(textEdit.toPlainText(), expected);
}

void TestCore::testRewriterAddObjectDefinition()
{
    const QLatin1String qmlString("import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "}");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QString("Qt/Rectangle"));

    ModelNode childNode = view->createModelNode("Qt/MouseRegion", 4, 6);
    rootNode.nodeAbstractProperty(QLatin1String("data")).reparentHere(childNode);

    QCOMPARE(rootNode.nodeProperty(QLatin1String("data")).toNodeListProperty().toModelNodeList().size(), 1);
    childNode = rootNode.nodeProperty(QLatin1String("data")).toNodeListProperty().toModelNodeList().at(0);
    QCOMPARE(childNode.type(), QString(QLatin1String("Qt/MouseRegion")));
}

void TestCore::testRewriterAddStatesArray()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "}");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QString("Qt/Rectangle"));

    ModelNode stateNode = view->createModelNode("Qt/State", 4, 6);
    rootNode.nodeListProperty(QLatin1String("states")).reparentHere(stateNode);

    const QString expected1 = QLatin1String("\n"
                                           "import Qt 4.6\n"
                                           "\n"
                                           "Rectangle {\n"
                                           "    states: [\n"
                                           "        State {\n"
                                           "        }\n"
                                           "    ]\n"
                                           "}");
    QCOMPARE(textEdit.toPlainText(), expected1);

    ModelNode stateNode2 = view->createModelNode("Qt/State", 4, 6);
    rootNode.nodeListProperty(QLatin1String("states")).reparentHere(stateNode2);

    const QString expected2 = QLatin1String("\n"
                                           "import Qt 4.6\n"
                                           "\n"
                                           "Rectangle {\n"
                                           "    states: [\n"
                                           "        State {\n"
                                           "        },\n"
                                           "        State {\n"
                                           "        }\n"
                                           "    ]\n"
                                           "}");
    QCOMPARE(textEdit.toPlainText(), expected2);
}

void TestCore::testRewriterRemoveStates()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "    states: [\n"
                                  "        State {\n"
                                  "        },\n"
                                  "        State {\n"
                                  "        }\n"
                                  "    ]\n"
                                  "}");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QString("Qt/Rectangle"));

    NodeListProperty statesProperty = rootNode.nodeListProperty(QLatin1String("states"));
    QVERIFY(statesProperty.isValid());
    QCOMPARE(statesProperty.toModelNodeList().size(), 2);

    ModelNode state = statesProperty.toModelNodeList().at(1);
    state.destroy();

    const QLatin1String expected1("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "    states: [\n"
                                  "        State {\n"
                                  "        }\n"
                                  "    ]\n"
                                  "}");
    QCOMPARE(textEdit.toPlainText(), expected1);

    state = statesProperty.toModelNodeList().at(0);
    state.destroy();

    const QLatin1String expected2("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "}");
    QCOMPARE(textEdit.toPlainText(), expected2);
}

void TestCore::testRewriterRemoveObjectDefinition()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "  MouseRegion {\n"
                                  "  }\n"
                                  "  MouseRegion {\n"
                                  "  } // some comment here\n"
                                  "}");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QString("Qt/Rectangle"));

    QCOMPARE(rootNode.nodeProperty(QLatin1String("data")).toNodeListProperty().toModelNodeList().size(), 2);
    ModelNode childNode = rootNode.nodeProperty(QLatin1String("data")).toNodeListProperty().toModelNodeList().at(1);
    QCOMPARE(childNode.type(), QString(QLatin1String("Qt/MouseRegion")));

    childNode.destroy();

    QCOMPARE(rootNode.nodeProperty(QLatin1String("data")).toNodeListProperty().toModelNodeList().size(), 1);
    childNode = rootNode.nodeProperty(QLatin1String("data")).toNodeListProperty().toModelNodeList().at(0);
    QCOMPARE(childNode.type(), QString(QLatin1String("Qt/MouseRegion")));

    childNode.destroy();

    QVERIFY(!rootNode.hasProperty(QLatin1String("data")));

    const QString expected = QLatin1String("\n"
                                           "import Qt 4.6\n"
                                           "\n"
                                           "Rectangle {\n"
                                           "  // some comment here\n"
                                           "}");
    QCOMPARE(textEdit.toPlainText(), expected);

    // don't crash when deleting nodes not in any hierarchy
    ModelNode node1 = view->createModelNode("Qt/Rectangle", 4, 6);
    ModelNode node2 = node1.addChildNode("Qt/Item", 4, 6, "data");
    ModelNode node3 = node2.addChildNode("Qt/Item", 4, 6, "data");

    node3.destroy();
    node1.destroy();
}

void TestCore::testRewriterRemoveScriptBinding()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "   x: 10; // some comment\n"
                                  "  y: 20\n"
                                  "}");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QString("Qt/Rectangle"));

    QCOMPARE(rootNode.properties().size(), 2);
    QVERIFY(rootNode.hasProperty(QLatin1String("x")));
    QVERIFY(rootNode.hasProperty(QLatin1String("y")));

    rootNode.removeProperty(QLatin1String("y"));

    QCOMPARE(rootNode.properties().size(), 1);
    QVERIFY(rootNode.hasProperty(QLatin1String("x")));
    QVERIFY(!rootNode.hasProperty(QLatin1String("y")));

    rootNode.removeProperty(QLatin1String("x"));

    QCOMPARE(rootNode.properties().size(), 0);

    const QString expected = QLatin1String("\n"
                                           "import Qt 4.6\n"
                                           "\n"
                                           "Rectangle {\n"
                                           "   // some comment\n"
                                           "}");
    QCOMPARE(textEdit.toPlainText(), expected);
}

void TestCore::testRewriterNodeReparenting()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "  Item {\n"
                                  "    MouseRegion {\n"
                                  "    }\n"
                                  "  }\n"
                                  "}");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QString("Qt/Rectangle"));

    ModelNode itemNode = rootNode.nodeListProperty("data").toModelNodeList().at(0);
    QVERIFY(itemNode.isValid());
    QCOMPARE(itemNode.type(), QLatin1String("Qt/Item"));

    ModelNode mouseRegion = itemNode.nodeListProperty("data").toModelNodeList().at(0);
    QVERIFY(mouseRegion.isValid());
    QCOMPARE(mouseRegion.type(), QLatin1String("Qt/MouseRegion"));

    rootNode.nodeListProperty("data").reparentHere(mouseRegion);

    QVERIFY(mouseRegion.isValid());
    QCOMPARE(rootNode.nodeListProperty("data").toModelNodeList().size(), 2);

    QString expected =  "\n"
                        "import Qt 4.6\n"
                        "\n"
                        "Rectangle {\n"
                        "  Item {\n"
                        "  }\n"
                        "\n"
                        "MouseRegion {\n"
                        "    }\n"
                        "}";
    QCOMPARE(textEdit.toPlainText(), expected);

    // reparenting outside of the hierarchy
    ModelNode node1 = view->createModelNode("Qt/Rectangle", 4, 6);
    ModelNode node2 = view->createModelNode("Qt/Item", 4, 6);
    ModelNode node3 = view->createModelNode("Qt/Item", 4, 6);
    node2.nodeListProperty("data").reparentHere(node3);
    node1.nodeListProperty("data").reparentHere(node2);

    // reparent into the hierarchy
    rootNode.nodeListProperty("data").reparentHere(node1);

    expected =  "\n"
                "import Qt 4.6\n"
                "\n"
                "Rectangle {\n"
                "  Item {\n"
                "  }\n"
                "\n"
                "MouseRegion {\n"
                "    }\n"
                "\n"
                "    Rectangle {\n"
                "        Item {\n"
                "            Item {\n"
                "            }\n"
                "        }\n"
                "    }\n"
                "}";

    QCOMPARE(textEdit.toPlainText(), expected);

    // reparent out of the hierarchy
    ModelNode node4 = view->createModelNode("Qt/Rectangle", 4, 6);
    node4.nodeListProperty("data").reparentHere(node1);

    expected =  "\n"
                "import Qt 4.6\n"
                "\n"
                "Rectangle {\n"
                "  Item {\n"
                "  }\n"
                "\n"
                "MouseRegion {\n"
                "    }\n"
                "}";

    QCOMPARE(textEdit.toPlainText(), expected);
}

void TestCore::testRewriterNodeReparentingWithTransaction()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "  id: rootItem\n"
                                  "  Item {\n"
                                  "    id: firstItem\n"
                                  "    x: 10\n"
                                  "  }\n"
                                  "  Item {\n"
                                  "    id: secondItem\n"
                                  "    x: 20\n"
                                  "  }\n"
                                  "}");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QLatin1String("Qt/Rectangle"));
    QCOMPARE(rootNode.id(), QLatin1String("rootItem"));

    ModelNode item1Node = rootNode.nodeListProperty("data").toModelNodeList().at(0);
    QVERIFY(item1Node.isValid());
    QCOMPARE(item1Node.type(), QLatin1String("Qt/Item"));
    QCOMPARE(item1Node.id(), QLatin1String("firstItem"));

    ModelNode item2Node = rootNode.nodeListProperty("data").toModelNodeList().at(1);
    QVERIFY(item2Node.isValid());
    QCOMPARE(item2Node.type(), QLatin1String("Qt/Item"));
    QCOMPARE(item2Node.id(), QLatin1String("secondItem"));

    RewriterTransaction transaction = testRewriterView->beginRewriterTransaction();

    item1Node.nodeListProperty(QLatin1String("data")).reparentHere(item2Node);
    item2Node.variantProperty(QLatin1String("x")).setValue(0);

    transaction.commit();

    const QLatin1String expected("\n"
                                 "import Qt 4.6\n"
                                 "\n"
                                 "Rectangle {\n"
                                 "  id: rootItem\n"
                                 "  Item {\n"
                                 "    id: firstItem\n"
                                 "    x: 10\n"
                                 "\n"
                                 "Item {\n"
                                 "    id: secondItem\n"
                                 "    x: 0\n"
                                 "  }\n"
                                 "  }\n"
                                 "}");
    QCOMPARE(textEdit.toPlainText(), expected);
}
void TestCore::testRewriterMovingInOut()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "}");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QString("Qt/Rectangle"));

    ModelNode newNode = view->createModelNode("Qt/MouseRegion", 4, 6);
    rootNode.nodeListProperty(QLatin1String("data")).reparentHere(newNode);

    const QLatin1String expected1("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "    MouseRegion {\n"
                                  "    }\n"
                                  "}");
    QCOMPARE(textEdit.toPlainText(), expected1);

#define move(node, x, y) {\
    node.variantProperty(QLatin1String("x")).setValue(x);\
    node.variantProperty(QLatin1String("y")).setValue(y);\
}
    move(newNode, 1, 1);
    move(newNode, 2, 2);
    move(newNode, 3, 3);
#undef move
    newNode.destroy();

    const QLatin1String expected2("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "}");
    QCOMPARE(textEdit.toPlainText(), expected2);
}

void TestCore::testRewriterMovingInOutWithTransaction()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "}");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QString("Qt/Rectangle"));

    RewriterTransaction transaction = view->beginRewriterTransaction();

    ModelNode newNode = view->createModelNode("Qt/MouseRegion", 4, 6);
    rootNode.nodeListProperty(QLatin1String("data")).reparentHere(newNode);

#define move(node, x, y) {\
    node.variantProperty(QLatin1String("x")).setValue(x);\
    node.variantProperty(QLatin1String("y")).setValue(y);\
}
    move(newNode, 1, 1);
    move(newNode, 2, 2);
    move(newNode, 3, 3);
#undef move
    newNode.destroy();

    transaction.commit();

    const QLatin1String expected2("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "}");
    QCOMPARE(textEdit.toPlainText(), expected2);
}

void TestCore::testRewriterComplexMovingInOut()
{
    const QLatin1String qmlString("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "  Item {\n"
                                  "  }\n"
                                  "}");
    QPlainTextEdit textEdit;
    textEdit.setPlainText(qmlString);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    // read in
    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());

    ModelNode rootNode = view->rootModelNode();
    QVERIFY(rootNode.isValid());
    QCOMPARE(rootNode.type(), QString("Qt/Rectangle"));
    ModelNode itemNode = rootNode.nodeListProperty(QLatin1String("data")).toModelNodeList().at(0);

    ModelNode newNode = view->createModelNode("Qt/MouseRegion", 4, 6);
    rootNode.nodeListProperty(QLatin1String("data")).reparentHere(newNode);

    const QLatin1String expected1("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "  Item {\n"
                                  "  }\n"
                                  "\n"
                                  "  MouseRegion {\n"
                                  "  }\n"
                                  "}");
    QCOMPARE(textEdit.toPlainText(), expected1);

#define move(node, x, y) {\
    node.variantProperty(QLatin1String("x")).setValue(x);\
    node.variantProperty(QLatin1String("y")).setValue(y);\
}
    move(newNode, 1, 1);
    move(newNode, 2, 2);
    move(newNode, 3, 3);

    const QLatin1String expected2("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "  Item {\n"
                                  "  }\n"
                                  "\n"
                                  "  MouseRegion {\n"
                                  "      x: 3\n"
                                  "      y: 3\n"
                                  "  }\n"
                                  "}");
    QCOMPARE(textEdit.toPlainText(), expected2);

    itemNode.nodeListProperty(QLatin1String("data")).reparentHere(newNode);

    const QLatin1String expected3("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "  Item {\n"
                                  "MouseRegion {\n"
                                  "      x: 3\n"
                                  "      y: 3\n"
                                  "  }\n"
                                  "  }\n"
                                  "}");
    QCOMPARE(textEdit.toPlainText(), expected3);

    move(newNode, 0, 0);
    move(newNode, 1, 1);
    move(newNode, 2, 2);
    move(newNode, 3, 3);
#undef move
    newNode.destroy();

    const QLatin1String expected4("\n"
                                  "import Qt 4.6\n"
                                  "\n"
                                  "Rectangle {\n"
                                  "  Item {\n"
                                  "  }\n"
                                  "}");
    QCOMPARE(textEdit.toPlainText(), expected4);
}

void TestCore::removeCenteredInAnchorByDetaching()
{
    QPlainTextEdit textEdit1;
    textEdit1.setPlainText("import Qt 4.6; Item { Rectangle { id: child; anchors.centerIn: parent } }");
    PlainTextEditModifier modifier1(&textEdit1);

    QScopedPointer<Model> model(Model::create("Qt/Item"));

    QScopedPointer<TestRewriterView> testRewriterView1(new TestRewriterView());
    testRewriterView1->setTextModifier(&modifier1);
    model->attachView(testRewriterView1.data());

    TestView *testView = new TestView(model.data());
    model->attachView(testView);

    QmlItemNode rootQmlItemNode(testView->rootQmlItemNode());


    QmlItemNode firstChild(rootQmlItemNode.children().first());
    QVERIFY(firstChild.isValid());
    QCOMPARE(firstChild.id(), QString("child"));

    // Verify the synthesized anchors:
    QmlAnchors anchors(firstChild);
    QVERIFY(!anchors.instanceAnchor(AnchorLine::Left).isValid());
    QVERIFY(!anchors.instanceAnchor(AnchorLine::Top).isValid());
    QVERIFY(!anchors.instanceAnchor(AnchorLine::Right).isValid());
    QVERIFY(!anchors.instanceAnchor(AnchorLine::Bottom).isValid());

    QVERIFY(anchors.instanceAnchor(AnchorLine::HorizontalCenter).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::HorizontalCenter).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::HorizontalCenter).type(), AnchorLine::HorizontalCenter);

    QVERIFY(anchors.instanceAnchor(AnchorLine::VerticalCenter).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::VerticalCenter).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::VerticalCenter).type(), AnchorLine::VerticalCenter);
    QSKIP("transaction are crashing", SkipAll);
    // Remove horizontal anchor:
    anchors.removeAnchor(AnchorLine::HorizontalCenter);

    // Verify again:
    QVERIFY(!anchors.instanceAnchor(AnchorLine::Left).isValid());
    QVERIFY(!anchors.instanceAnchor(AnchorLine::Top).isValid());
    QVERIFY(!anchors.instanceAnchor(AnchorLine::Right).isValid());
    QVERIFY(!anchors.instanceAnchor(AnchorLine::Bottom).isValid());

    QVERIFY(!anchors.instanceAnchor(AnchorLine::HorizontalCenter).isValid());

    QVERIFY(anchors.instanceAnchor(AnchorLine::VerticalCenter).isValid());
    QCOMPARE(anchors.instanceAnchor(AnchorLine::VerticalCenter).qmlItemNode(), rootQmlItemNode);
    QCOMPARE(anchors.instanceAnchor(AnchorLine::VerticalCenter).type(), AnchorLine::VerticalCenter);
}

void TestCore::loadPropertyBinding()
{
    QSKIP("property bindings are disabled", SkipAll);
//    QScopedPointer<Model> model(Model::create("Qt/Item"));
//    QVERIFY(model.data());
//    ModelNode rootModelNode(view->rootModelNode());
//
//
//    ModelNode firstChild = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
//    firstChild.addProperty("width", QString("parent.width"));
//    QVERIFY(firstChild.isValid());
//    QVERIFY(firstChild.hasProperty("width"));
//    QVERIFY(firstChild.property("width").isValueAPropertyBinding());
//
//    AbstractProperty width = firstChild.property("width");
//    QVERIFY(width.isValueAPropertyBinding());
//
//    QVERIFY(width.value().isValid() && !width.value().isNull());
//    QCOMPARE(width.value().type(), QVariant::UserType);
//    QCOMPARE(width.value().userType(), qMetaTypeId<QmlDesigner::PropertyBinding>());
//
//    PropertyBinding widthBinding = width.valueToPropertyBinding();
//    QVERIFY(widthBinding.isValid());
//    QCOMPARE(widthBinding.value(), QString("parent.width"));
}

void TestCore::changePropertyBinding()
{

    QScopedPointer<Model> model(Model::create("Qt/Item"));
    QVERIFY(model.data());

    QScopedPointer<TestView> view(new TestView);
    model->attachView(view.data());

    ModelNode rootModelNode(view->rootModelNode());
    rootModelNode.variantProperty("width") = 20;

    ModelNode firstChild = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
    firstChild.bindingProperty("width").setExpression(QString("parent.width"));
    firstChild.variantProperty("height")=  10;
    QVERIFY(firstChild.isValid());

    {
        QVERIFY(firstChild.hasProperty("width"));
        
        BindingProperty widthBinding = firstChild.bindingProperty("width");
        QVERIFY(widthBinding.isValid());
        QCOMPARE(widthBinding.expression(), QString("parent.width"));
        QVERIFY(firstChild.variantProperty("height").value().toInt() == 10);
    }

    firstChild.variantProperty("width") =  400;
    firstChild.bindingProperty("height").setExpression("parent.width / 2");

    {
        QVERIFY(firstChild.hasProperty("width"));
        QVariant width = firstChild.variantProperty("width").value();
        QVERIFY(width.isValid() && !width.isNull());
        QVERIFY(width == 400);
        QVERIFY(firstChild.hasProperty("height"));
        BindingProperty heightBinding = firstChild.bindingProperty("height");
        QVERIFY(heightBinding.isValid());
        QCOMPARE(heightBinding.expression(), QString("parent.width / 2"));
    }
}

void TestCore::loadObjectPropertyBinding()
{
    QSKIP("will be changed", SkipAll);

//    QList<QmlError> errors;
//    QScopedPointer<ByteArrayModifier> modifier1(ByteArrayModifier::create(QString("import Qt 4.6; Item { ListView { delegate:componentId } Component { id:componentId;Rectangle {} } }")));
//
//    QVERIFY(model.data());
//    ModelNode rootModelNode(view->rootModelNode());
//
//
//    QVERIFY(rootModelNode.childNodes().at(1).id() == "componentId");
//    QVERIFY(rootModelNode.childNodes().first().property("delegate").value().canConvert<ObjectPropertyBinding>());
}

void TestCore::loadWelcomeScreen()
{
    QSKIP("there is no rewriter anymore", SkipAll);
//    QFile file(QCoreApplication::applicationDirPath() + "/../shared/welcomescreen.qml");
//    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
//
//    QList<QmlError> errors;
//    QScopedPointer<ByteArrayModifier> modifier1(ByteArrayModifier::create(QString(file.readAll())));
//    QScopedPointer<Model> model1(Model::create(modifier1.data(), QUrl::fromLocalFile(file.fileName()), &errors));
//    if (!errors.isEmpty()) {
//        printErrors(errors, file.fileName());
//        QVERIFY(errors.isEmpty());
//    }
//    QVERIFY(model1.data());
}

static QString rectWithGradient = "import Qt 4.6\n"
                                  "Rectangle {\n"
                                  "    gradient: Gradient {\n"
                                  "        id: pGradient\n"
                                  "        GradientStop { id: pOne; position: 0.0; color: \"lightsteelblue\" }\n"
                                  "        GradientStop { id: pTwo; position: 1.0; color: \"blue\" }\n"
                                  "    }\n"
                                  "    Gradient {\n"
                                  "        id: secondGradient\n"
                                  "        GradientStop { id: nOne; position: 0.0; color: \"blue\" }\n"
                                  "        GradientStop { id: nTwo; position: 1.0; color: \"lightsteelblue\" }\n"
                                  "    }\n"
                                  "}";

void TestCore::loadGradient()
{
    QPlainTextEdit textEdit;
    textEdit.setPlainText(rectWithGradient);
    PlainTextEditModifier textModifier(&textEdit);

    QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
    QVERIFY(model.data());

    QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
    testRewriterView->setTextModifier(&textModifier);
    model->attachView(testRewriterView.data());


    QVERIFY(model.data());
    ModelNode rootModelNode(testRewriterView->rootModelNode());
    QVERIFY(rootModelNode.isValid());
    QCOMPARE(rootModelNode.allDirectSubModelNodes().size(), 2);

    {
        QVERIFY(rootModelNode.hasProperty("gradient"));
        AbstractProperty gradientProperty = rootModelNode.property("gradient");
        QVERIFY(gradientProperty.isNodeProperty());
        ModelNode gradientPropertyModelNode = gradientProperty.toNodeProperty().modelNode();
        QVERIFY(gradientPropertyModelNode.isValid());
        QCOMPARE(gradientPropertyModelNode.type(), QString("Qt/Gradient"));
        QCOMPARE(gradientPropertyModelNode.allDirectSubModelNodes().size(), 2);

        AbstractProperty stopsProperty = gradientPropertyModelNode.property("stops");
        QVERIFY(stopsProperty.isValid());

        QVERIFY(stopsProperty.isNodeListProperty());
        QList<ModelNode>  stops = stopsProperty.toNodeListProperty().toModelNodeList();
        QCOMPARE(stops.size(), 2);

        ModelNode pOne = stops.first();
        ModelNode pTwo = stops.last();

        QCOMPARE(pOne.type(), QString("Qt/GradientStop"));
        QCOMPARE(pOne.id(), QString("pOne"));
        QCOMPARE(pOne.allDirectSubModelNodes().size(), 0);
        QCOMPARE(pOne.propertyNames().size(), 2);
        QCOMPARE(pOne.variantProperty("position").value().type(), QVariant::Double);
        QCOMPARE(pOne.variantProperty("position").value().toDouble(), 0.0);
        QCOMPARE(pOne.variantProperty("color").value().type(), QVariant::Color);
        QCOMPARE(pOne.variantProperty("color").value().value<QColor>(), QColor("lightsteelblue"));

        QCOMPARE(pTwo.type(), QString("Qt/GradientStop"));
        QCOMPARE(pTwo.id(), QString("pTwo"));
        QCOMPARE(pTwo.allDirectSubModelNodes().size(), 0);
        QCOMPARE(pTwo.propertyNames().size(), 2);
        QCOMPARE(pTwo.variantProperty("position").value().type(), QVariant::Double);
        QCOMPARE(pTwo.variantProperty("position").value().toDouble(), 1.0);
        QCOMPARE(pTwo.variantProperty("color").value().type(), QVariant::Color);
        QCOMPARE(pTwo.variantProperty("color").value().value<QColor>(), QColor("blue"));
    }

    {
        ModelNode gradientNode = rootModelNode.allDirectSubModelNodes().last();
        QVERIFY(gradientNode.isValid());
        QVERIFY(!gradientNode.metaInfo().isQmlGraphicsItem());
        QCOMPARE(gradientNode.type(), QString("Qt/Gradient"));
        QCOMPARE(gradientNode.id(), QString("secondGradient"));
        QCOMPARE(gradientNode.allDirectSubModelNodes().size(), 2);

        AbstractProperty stopsProperty = gradientNode.property("stops");
        QVERIFY(stopsProperty.isValid());

        QVERIFY(stopsProperty.isNodeListProperty());
        QList<ModelNode>  stops = stopsProperty.toNodeListProperty().toModelNodeList();
        QCOMPARE(stops.size(), 2);

        ModelNode nOne = stops.first();
        ModelNode nTwo = stops.last();

        QCOMPARE(nOne.type(), QString("Qt/GradientStop"));
        QCOMPARE(nOne.id(), QString("nOne"));
        QCOMPARE(nOne.allDirectSubModelNodes().size(), 0);
        QCOMPARE(nOne.propertyNames().size(), 2);
        QCOMPARE(nOne.variantProperty("position").value().type(), QVariant::Double);
        QCOMPARE(nOne.variantProperty("position").value().toDouble(), 0.0);
        QCOMPARE(nOne.variantProperty("color").value().type(), QVariant::Color);
        QCOMPARE(nOne.variantProperty("color").value().value<QColor>(), QColor("blue"));

        QCOMPARE(nTwo.type(), QString("Qt/GradientStop"));
        QCOMPARE(nTwo.id(), QString("nTwo"));
        QCOMPARE(nTwo.allDirectSubModelNodes().size(), 0);
        QCOMPARE(nTwo.propertyNames().size(), 2);
        QCOMPARE(nTwo.variantProperty("position").value().type(), QVariant::Double);
        QCOMPARE(nTwo.variantProperty("position").value().toDouble(), 1.0);
        QCOMPARE(nTwo.variantProperty("color").value().type(), QVariant::Color);
        QCOMPARE(nTwo.variantProperty("color").value().value<QColor>(), QColor("lightsteelblue"));
    }
}

void TestCore::changeGradientId()
{
    try {
        QPlainTextEdit textEdit;
        textEdit.setPlainText(rectWithGradient);
        PlainTextEditModifier textModifier(&textEdit);

        QScopedPointer<Model> model(Model::create("Qt/Item", 4, 6));
        QVERIFY(model.data());

        QScopedPointer<TestRewriterView> testRewriterView(new TestRewriterView());
        testRewriterView->setTextModifier(&textModifier);
        model->attachView(testRewriterView.data());

        QVERIFY(model.data());
        ModelNode rootModelNode(testRewriterView->rootModelNode());
        QVERIFY(rootModelNode.isValid());
        QCOMPARE(rootModelNode.allDirectSubModelNodes().size(), 2);

        AbstractProperty gradientProperty = rootModelNode.property("gradient");
        QVERIFY(gradientProperty.isNodeProperty());
        ModelNode gradientNode = gradientProperty.toNodeProperty().modelNode();        
        QVERIFY(gradientNode.isValid());
        QCOMPARE(gradientNode.id(), QString("pGradient"));

        gradientNode.setId("firstGradient");
        QCOMPARE(gradientNode.id(), QString("firstGradient"));

        AbstractProperty stopsProperty = gradientNode.property("stops");
        QVERIFY(stopsProperty.isValid());

        QVERIFY(stopsProperty.isNodeListProperty());
        QList<ModelNode>  stops = stopsProperty.toNodeListProperty().toModelNodeList();
        QCOMPARE(stops.size(), 2);

        ModelNode firstStop = stops.at(0);
        QVERIFY(firstStop.isValid());
        firstStop.destroy();
        QVERIFY(!firstStop.isValid());

        ModelNode gradientStop  = gradientNode.addChildNode("Qt/GradientStop", 4, 6, "stops");
        gradientStop.variantProperty("position") = 0.5;
        gradientStop.variantProperty("color") = QColor("yellow");

        gradientStop.setId("newGradientStop");

        QCOMPARE(gradientNode.allDirectSubModelNodes().size(), 2);
        QCOMPARE(gradientNode.nodeListProperty("stops").toModelNodeList().size(), 2);
        QCOMPARE(gradientStop.id(), QString("newGradientStop"));
        QCOMPARE(gradientStop.variantProperty("position").value().toDouble(), 0.5);
        QCOMPARE(gradientStop.variantProperty("color").value().value<QColor>(), QColor("yellow"));

    } catch (Exception &e) {
        qDebug() << e;
        QFAIL(e.description().toLatin1().data());
    }
}

void TestCore::copyAndPasteInSingleModel()
{
    QSKIP("no anchor support anymore", SkipAll);

//    try {
//        QScopedPointer<Model> model(create("import Qt 4.6; Item { width: 100; height: 200; Rectangle { x: 10; y: 20; width: 30; height: 40; color: \"red\"}}"));
//        QVERIFY(model.data());
//        ModelNode rootModelNode(view->rootModelNode());
//        QVERIFY(rootModelNode.isValid());
//        ModelNode rectNode(rootModelNode.childNodes().at(0));
//        QVERIFY(rectNode.isValid());
//
//        QMimeData *transfer = model->copy(QList<NodeState>() << rectNode);
//        QVERIFY(rectNode.isValid());
//        QCOMPARE(rectNode.childNodes().size(), 0);
//
//        QVERIFY(transfer != 0);
//        model->paste(transfer, rectNode);
//        QVERIFY(rectNode.isValid());
//        QCOMPARE(rectNode.childNodes().size(), 1);
//
//        QCOMPARE(rectNode.property("x").value().toDouble(), 10.0);
//        QCOMPARE(rectNode.property("y").value().toDouble(), 20.0);
//        QCOMPARE(rectNode.property("width").value().toDouble(), 30.0);
//        QCOMPARE(rectNode.property("height").value().toDouble(), 40.0);
//        QCOMPARE(rectNode.property("color").value().value<QColor>(), QColor("red"));
//
//        ModelNode rectChild = rectNode.childNodes().at(0);
//        QCOMPARE(rectChild.property("x").value().toDouble(), 10.0);
//        QCOMPARE(rectChild.property("y").value().toDouble(), 20.0);
//        QCOMPARE(rectChild.property("width").value().toDouble(), 30.0);
//        QCOMPARE(rectChild.property("height").value().toDouble(), 40.0);
//        QCOMPARE(rectChild.property("color").value().value<QColor>(), QColor("red"));
//    } catch (Exception &e) {
//        qDebug() << e;
//        QFAIL(e.description().toLatin1().data());
//    }
}

void TestCore::copyAndPasteBetweenModels()
{
    QSKIP("no anchor support anymore", SkipAll);

//    try {
//        QScopedPointer<Model> model1(create("import Qt 4.6; Item { width: 100; height: 200; Rectangle { x: 10; y: 20; width: 30; height: 40; color: \"red\"}}"));
//        QVERIFY(model1.data());
//        ModelNode rootModelNode1(model1->rootModelNode());
//        QVERIFY(rootModelNode1.isValid());
//        ModelNode rectNode1(rootModelNode1.childNodes().at(0));
//        QVERIFY(rectNode1.isValid());
//
//        QScopedPointer<Model> model2(create("import Qt 4.6; Item { width: 100; height: 200}"));
//        QVERIFY(model2.data());
//        ModelNode rootModelNode2(model2->rootModelNode());
//        QVERIFY(rootModelNode2.isValid());
//        QCOMPARE(rootModelNode2.childNodes().size(), 0);
//
//        QMimeData *transfer = model1->copy(QList<NodeState>() << rectNode1);
//        QVERIFY(rectNode1.isValid());
//
//        QVERIFY(transfer != 0);
//        model2->paste(transfer, rootModelNode2);
//        QCOMPARE(rootModelNode2.childNodes().size(), 1);
//
//        ModelNode rectNode2 = rootModelNode2.childNodes().at(0);
//        QCOMPARE(rectNode2.property("x").value().toDouble(), 10.0);
//        QCOMPARE(rectNode2.property("y").value().toDouble(), 20.0);
//        QCOMPARE(rectNode2.property("width").value().toDouble(), 30.0);
//        QCOMPARE(rectNode2.property("height").value().toDouble(), 40.0);
//        QCOMPARE(rectNode2.property("color").value().value<QColor>(), QColor("red"));
//    } catch (Exception &e) {
//        qDebug() << e;
//        QFAIL(e.description().toLatin1().data());
//    }
}

void TestCore::changeSubModel()
{
    QSKIP("must be refactored", SkipAll);
//     // create the orignal model:
//    QScopedPointer<Model> model1(create("import Qt 4.6; Rectangle { id: rootRect; Rectangle { id: rect2 }}"));
//    QVERIFY(model1.data());
//
//    // get the model node for the component from the "original" model
//    ModelNode modelNode = model1->rootModelNode().childNodes().at(0);
//    QVERIFY(modelNode.isValid());
//    QCOMPARE(modelNode.id(), QString("rect2"));
//
//    QList<QmlError> errors;
//    // Create a model on top of the component node:
//    TextModifier *subModifier = model1->createTextModifier(modelNode);
//    QScopedPointer<Model> subModel(Model::create(subModifier, model1->fileUrl(), &errors));
//    // Handle errors:
//    if (!errors.isEmpty()) {
//        printErrors(errors, "<sub model>");
//        qDebug() << "Data:";
//        qDebug() << subModifier->text();
//        QVERIFY(errors.isEmpty());
//    }
//
//    // Check that changes in the model for the Component are reflected in both the model for the component, and the original model:
//    ModelNode subModelNode = subview->rootModelNode();
//    Q_ASSERT(subModelNode.isValid());
//    subModelNode.setId("AnotherId");
//    QCOMPARE(subModelNode.id(), QString("AnotherId"));
//    QCOMPARE(modelNode.id(), QString("AnotherId"));
//
//    // Check that changes in the original model are reflected in the model for the component:
//    modelNode.setId("YetAnotherId");
//    QCOMPARE(subModelNode.id(), QString("YetAnotherId"));
//    QCOMPARE(modelNode.id(), QString("YetAnotherId"));
}


void TestCore::changeInlineComponent()
{
    // create the orignal model:
        QSKIP("must be refactored", SkipAll);
//    QScopedPointer<Model> model1(create("import Qt 4.6; Rectangle { id: rootRect; Component { id: MyComponent; Rectangle { id: myComponentRect }}}"));
//    QVERIFY(model1.data());
//
//    // get the model node for the component from the "original" model
//    ModelNode componentNode = model1->rootModelNode().childNodes().at(0);
//    QVERIFY(componentNode.isValid());
//    QCOMPARE(componentNode.id(), QString("MyComponent"));
//    QVERIFY(!componentNode.hasChildNodes()); // top model shouldn't go into subcomponents
//
//    QScopedPointer<Model> componentModel(model1->createComponentModel(componentNode));
//
//    ModelNode componentRootNode = componentview->rootModelNode();
//    QVERIFY(componentRootNode.isValid());
//    QCOMPARE(componentRootNode.type(), QString("Qt/Rectangle"));
//
//    ModificationGroupToken token = componentModel->beginModificationGroup();
//    ModelNode childNode(componentRootNode.addChildNode("Qt/Rectangle", 4, 6, "data"));
//    QVERIFY(childNode.isValid());
//    componentModel->endModificationGroup(token);
//    QVERIFY(childNode.isValid());
//    QVERIFY(!componentRootNode.childNodes().isEmpty());
//    QVERIFY(componentRootNode.childNodes().first() == childNode);

}

void TestCore::changeImports()
{
    QSKIP("imports are disabled", SkipAll);
//    QScopedPointer<Model> model(Model::create("Qt/Rectangle"));
//    QVERIFY(model.data());
//    QVERIFY(view->rootModelNode().isValid());
//
//    QSet<Import> imports = model->imports();
//    QCOMPARE(imports.size(), 1);
//    const Import import = imports.toModelNodeList().at(0);
//    QVERIFY(import.isLibraryImport());
//    QCOMPARE(import.url(), QUrl("Qt"));
//    QVERIFY(import.file().isEmpty());
//    QCOMPARE(import.version(), QString("4.6"));
//    QVERIFY(import.alias().isEmpty());
//
//    Import newImport = Import::createFileImport("Namespaces_the_final_frontier", QString(), "MySpace");
//    model->addImport(newImport);
//    imports = model->imports();
//    QCOMPARE(imports.size(), 2);
//    QVERIFY(imports.contains(import));
//    QVERIFY(imports.contains(newImport));
//
//    model->removeImport(newImport);
//    imports = model->imports();
//    QCOMPARE(imports.size(), 1);
//    QVERIFY(imports.contains(import));
//    QVERIFY(!imports.contains(newImport));
}

void TestCore::testIfChangePropertyIsRemoved()
{
    QSKIP("Fix me!!! See task BAUHAUS-139", SkipAll);
//    QScopedPointer<Model> model(create("import Qt 4.6; Item { Rectangle { x: 10; y: 10; } }"));
//    QVERIFY(model.data());
//    ModelNode rootModelNode(view->rootModelNode());
//    QVERIFY(rootModelNode.isValid());
//    ModelNode rectNode(rootModelNode.childNodes().at(0));
//    QVERIFY(rectNode.isValid());
//    ModelState newState = model->createModelState("new state");
//    QVERIFY(newState.isValid());
//    NodeState rectNodeState = newState.nodeState(rectNode);
//    QVERIFY(newState.isValid());
//    try {
//        rectNodeState.setPropertyValue("x", 100);
//        QVERIFY(rectNodeState.hasProperty("x"));
//    } catch (Exception &e) {
//        // ok.
//    }
//    rectNode.setPropertyValue("x", 15);
//    QVERIFY(newState.isValid());
//    NodeState rectNodeState2 = newState.nodeState(rectNode);
//    QVERIFY(rectNodeState2.isValid());
//    QVERIFY(rectNodeState.isValid());
//
//    rectNode.setId("rect");
//    QVERIFY(rectNodeState2.isValid());
//    QVERIFY(rectNodeState.isValid());
//
//    rectNodeState.setPropertyValue("x", 105);
//
//    QVERIFY(rectNodeState2.isValid());
//    QVERIFY(rectNodeState.isValid());
//
//    QCOMPARE(rectNodeState.property("x").value().toInt(), 105);
//
//    rectNode.destroy();
//
//    QVERIFY(!rectNodeState2.isValid());
//    QVERIFY(!rectNodeState.isValid());
//    QVERIFY(!rectNode.isValid());
//
//    rectNode = rootModelNode.addChildNode("Qt/Rectangle", 4, 6, "data");
//    rectNode.setId("rect");
//    rectNodeState = newState.nodeState(rectNode);
//    rectNode.setPropertyValue("x", 10);
//
//    QCOMPARE(rectNodeState.property("x").value().toInt(), 10);
}

void  TestCore::testAnchorsAndStates()
{
    QSKIP("no states anymore", SkipAll);
//    QScopedPointer<Model> model(create("import Qt 4.6; Item { Rectangle { x: 10; y: 10; } }"));
//    QVERIFY(model.data());
//    ModelNode rootModelNode(view->rootModelNode());
//    QVERIFY(rootModelNode.isValid());
//    ModelNode rectNode(rootModelNode.childNodes().at(0));
//    QVERIFY(rectNode.isValid());
//
//    NodeAnchors anchors(rectNode);
//    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::Left));
//    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::Top));
//    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::Right));
//    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::Bottom));
//    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::HorizontalCenter));
//    QVERIFY(!anchors.instanceHasAnchor(AnchorLine::VerticalCenter));
//
//    anchors.setAnchor(AnchorLine::Right, rootModelNode, AnchorLine::Right);
//    QVERIFY(anchors.instanceHasAnchor(AnchorLine::Right));
//    QCOMPARE(anchors.instanceAnchor(AnchorLine::Right).type(), AnchorLine::Right);
//    QCOMPARE(anchors.instanceAnchor(AnchorLine::Right).modelNode(), rootModelNode);
//
//    anchors.setMargin(AnchorLine::Right, 10.0);
//    QCOMPARE(anchors.instanceMargin(AnchorLine::Right), 10.0);
//
//    anchors.setMargin(AnchorLine::Right, 0.0);
//    QCOMPARE(anchors.instanceMargin(AnchorLine::Right), 0.0);
//
//    anchors.setMargin(AnchorLine::Right, 20.0);
//
//    QCOMPARE(1, rectNode.nodeStates().count());
//
//    ModelState newState = model->createModelState("new state");
//
//    QCOMPARE(2, rectNode.nodeStates().count());
//
//    QCOMPARE(2, model->baseModelState().nodeStates().count());
//    QCOMPARE(2, newState.nodeStates().count());
//
//    QVERIFY(newState.isValid());
//    QVERIFY(rectNode.isValid());
//    NodeState rectNodeState = newState.nodeState(rectNode);
//    QVERIFY(rectNodeState.isValid());
}


void TestCore::testStatesWithAnonymousTargets()
{    
    QSKIP("no states anymore", SkipAll);
    //QScopedPointer<Model> model(create("import Qt 4.6; Rectangle { states:[State{name:\"s1\";PropertyChanges{target:;x:20}}]Rectangle{x:10} } "));
//    ByteArrayModifier* modifier = 0;
//    Model *model_p = 0;
//    load("import Qt 4.6; Rectangle { states:[State{name:\"s1\";PropertyChanges{target:;x:20}}]Rectangle{x:10} } ",model_p,modifier);
//
//    QScopedPointer<Model> model(model_p);
//    QVERIFY(model.data());
//    ModelNode rootModelNode(view->rootModelNode());
//    QVERIFY(rootModelNode.isValid());
//    ModelNode rectNode(rootModelNode.childNodes().at(0));
//    QVERIFY(rectNode.isValid());
//    // check item is anonymous
//    QVERIFY(rectNode.id().isEmpty());
//
//    NodeState newState = model->modelStates().at(1).nodeStates().at(1);
//    QVERIFY(newState.isValid());
//    // check target is anonymous
//    QVERIFY(newState.modelNode().id().isEmpty());
//    // check base state has x=0
//    QCOMPARE(model->modelStates().at(0).nodeStates().at(1).property("x").value().toInt(),10);
//
//    // now we "move" the rectangle, the model should force an id and generate a new propertychange
//    NodeStateChangeSet change( newState );
//    change.setPropertyValue("x",QVariant(30));
//    QList <NodeStateChangeSet> changeList;
//    changeList.append(change);
//    model->changeNodeStates(changeList);
//
//    QCOMPARE(newState.modelNode().id(),QString("Rectangle1"));
//    QCOMPARE(rectNode.id(),QString("Rectangle1"));
//    QCOMPARE(newState.property("x").value().toInt(),30);
//
//    // now we manually erase the new id and target
//    QString Text = modifier->text();
//    Text.remove(Text.indexOf(QString("id: Rectangle1;")),QString("id: Rectangle1;").length());
//    Text.remove(Text.indexOf(QString("Rectangle1")),QString("Rectangle1").length());
//
//    // text2model again
//    reload(Text,modifier);
//
//    // repeat checks
//    QVERIFY(model.data());
//    QVERIFY(rootModelNode.isValid());
//    QVERIFY(rectNode.isValid());
//    QVERIFY(rectNode.id().isEmpty());
//    QCOMPARE(model->modelStates().at(0).nodeStates().at(1).property("x").value().toInt(),10);
//
//    // see what happened with the new state
//    QVERIFY(newState.isValid());
//    QVERIFY(newState.modelNode().id().isEmpty());

}

QTEST_MAIN(TestCore);
