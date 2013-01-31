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

#include "testview.h"

#include <QDebug>
#include <QUrl>
#include <qtestcase.h>
#include <abstractproperty.h>
#include <bindingproperty.h>
#include <variantproperty.h>
#include <nodelistproperty.h>
#include <nodeinstanceview.h>
#include <model.h>

TestView::TestView(QmlDesigner::Model *model)
    : QmlDesigner::QmlModelView(model)
{
    QmlDesigner::NodeInstanceView *nodeInstanceView = new QmlDesigner::NodeInstanceView(model, QmlDesigner::NodeInstanceServerInterface::TestModus);

    model->attachView(nodeInstanceView);
}

void TestView::modelAttached(QmlDesigner::Model *model)
{
    QmlDesigner::QmlModelView::modelAttached(model);
    m_methodCalls += MethodCall("modelAttached", QStringList() << QString::number(reinterpret_cast<long>(model)));
}

void TestView::modelAboutToBeDetached(QmlDesigner::Model *model)
{
    QmlDesigner::QmlModelView::modelAboutToBeDetached(model);
    m_methodCalls += MethodCall("modelAboutToBeDetached", QStringList() << QString::number(reinterpret_cast<long>(model)));
}

void TestView::nodeIdChanged(const QmlDesigner::ModelNode &node, const QString& newId, const QString &oldId)
{
    QmlDesigner::QmlModelView::nodeIdChanged(node, newId, oldId);
    m_methodCalls += MethodCall("nodeIdChanged", QStringList() << node.id() << newId << oldId);
}

void TestView::rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
    QmlDesigner::QmlModelView::rootNodeTypeChanged(type, majorVersion, minorVersion);
    m_methodCalls += MethodCall("rootNodeTypeChanged", QStringList() << rootModelNode().id() << type << QString::number(majorVersion) << QString::number(minorVersion));
}

void TestView::fileUrlChanged(const QUrl & oldBaseUrl, const QUrl &newBaseUrl)
{
    QmlDesigner::QmlModelView::fileUrlChanged(oldBaseUrl, newBaseUrl);
    m_methodCalls += MethodCall("fileUrlChanged", QStringList() << oldBaseUrl.toString() << newBaseUrl.toString());
}

void TestView::propertiesAboutToBeRemoved(const QList<QmlDesigner::AbstractProperty>& propertyList)
{
    QmlDesigner::QmlModelView::propertiesAboutToBeRemoved(propertyList);
    QStringList propertyNames;
    foreach (const QmlDesigner::AbstractProperty &property, propertyList)
        propertyNames += property.name();
    m_methodCalls += MethodCall("propertiesAboutToBeRemoved", QStringList() << propertyNames.join(", "));
}

void TestView::nodeCreated(const QmlDesigner::ModelNode &createdNode)
{
    QmlDesigner::QmlModelView::nodeCreated(createdNode);
    m_methodCalls += MethodCall("nodeCreated", QStringList() << createdNode.id());
}

void TestView::nodeAboutToBeRemoved(const QmlDesigner::ModelNode &removedNode)
{
    QmlDesigner::QmlModelView::nodeAboutToBeRemoved(removedNode);
    m_methodCalls += MethodCall("nodeAboutToBeRemoved", QStringList() << removedNode.id());
}

void TestView::nodeRemoved(const QmlDesigner::ModelNode &removedNode, const QmlDesigner::NodeAbstractProperty &parentProperty, AbstractView::PropertyChangeFlags propertyChange)
{
    QmlDesigner::QmlModelView::nodeRemoved(removedNode, parentProperty, propertyChange);
    const QString parentPropertyName = parentProperty.isValid() ? parentProperty.name() : "";

    m_methodCalls += MethodCall("nodeRemoved", QStringList() << QString() << parentPropertyName << serialize(propertyChange));
}


void TestView::nodeReparented(const QmlDesigner::ModelNode & node, const QmlDesigner::NodeAbstractProperty &newPropertyParent, const QmlDesigner::NodeAbstractProperty & oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
    QmlDesigner::QmlModelView::nodeReparented(node, newPropertyParent, oldPropertyParent, propertyChange);
    m_methodCalls += MethodCall("nodeReparented", QStringList() << node.id() << newPropertyParent.name() << oldPropertyParent.name() << serialize(propertyChange));
}

void TestView::bindingPropertiesChanged(const QList<QmlDesigner::BindingProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    QmlDesigner::QmlModelView::bindingPropertiesChanged(propertyList, propertyChange);
    QStringList propertyNames;
    foreach (const QmlDesigner::BindingProperty &property, propertyList)
        propertyNames += property.name();
    m_methodCalls += MethodCall("bindingPropertiesChanged", QStringList() << propertyNames.join(", ") << serialize(propertyChange));
}

void TestView::variantPropertiesChanged(const QList<QmlDesigner::VariantProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    QmlDesigner::QmlModelView::variantPropertiesChanged(propertyList, propertyChange);
    QStringList propertyNames;
    foreach (const QmlDesigner::VariantProperty &property, propertyList)
        propertyNames += property.name();

    m_methodCalls += MethodCall("variantPropertiesChanged", QStringList() << propertyNames.join(", ") << serialize(propertyChange));
}

void TestView::selectedNodesChanged(const QList<QmlDesigner::ModelNode> &selectedNodeList,
                       const QList<QmlDesigner::ModelNode> &lastSelectedNodeList)
{
    QmlDesigner::QmlModelView::selectedNodesChanged(selectedNodeList, lastSelectedNodeList);
    QStringList selectedNodes;
    foreach (const QmlDesigner::ModelNode &node, selectedNodeList)
        selectedNodes += node.id();
    QStringList lastSelectedNodes;
    foreach (const QmlDesigner::ModelNode &node, lastSelectedNodeList)
        lastSelectedNodes += node.id();
    m_methodCalls += MethodCall("selectedNodesChanged", QStringList() << selectedNodes.join(", ") << lastSelectedNodes.join(", "));
}


void TestView::nodeOrderChanged(const QmlDesigner::NodeListProperty &listProperty, const QmlDesigner::ModelNode &movedNode, int oldIndex)
{
    QmlDesigner::QmlModelView::nodeOrderChanged(listProperty, movedNode, oldIndex);
    m_methodCalls += MethodCall("nodeOrderChanged", QStringList() << listProperty.name() << movedNode.id() << QString::number(oldIndex));
}

void TestView::actualStateChanged(const QmlDesigner::ModelNode &node)
{
    QmlDesigner::QmlModelView::actualStateChanged(node);
    m_methodCalls += MethodCall("actualStateChanged", QStringList() << node.id());
}

QList<TestView::MethodCall> &TestView::methodCalls()
{
    return m_methodCalls;
}

QString TestView::lastFunction() const
{
    return m_methodCalls.last().name;
}

QmlDesigner::NodeInstanceView *TestView::nodeInstanceView() const
{
    return QmlDesigner::QmlModelView::nodeInstanceView();

}

QmlDesigner::NodeInstance TestView::instanceForModelNode(const QmlDesigner::ModelNode &modelNode)
{
    return QmlDesigner::QmlModelView::instanceForModelNode(modelNode);
}


QString TestView::serialize(AbstractView::PropertyChangeFlags change)
{
    QStringList tokenList;

    if (change.testFlag(PropertiesAdded))
        tokenList.append(QLatin1String("PropertiesAdded"));

    if (change.testFlag(EmptyPropertiesRemoved))
        tokenList.append(QLatin1String("EmptyPropertiesRemoved"));

    return tokenList.join(" ");
}


bool operator==(TestView::MethodCall call1, TestView::MethodCall call2)
{
    if (call1.name != call2.name)
        return false;
//    if (call1.arguments != call2.arguments)
//        return false;
    if (call1.arguments.size() != call2.arguments.size()) {
        qWarning() << "different method names:" << call1.arguments.size() << call2.arguments.size();
        return false;
    }
    for (int i = 0; i < call1.arguments.size(); ++i)
        if (call1.arguments.at(i) != call2.arguments.at(i)) {
            qDebug() << "different argument values:" << call1.arguments.at(i) << call2.arguments.at(i);
            return false;
        }
    return true;
}

QDebug operator<<(QDebug debug, TestView::MethodCall call)
{
    return debug.nospace() << call.name << "(" << call.arguments.join(", ") << ")";
}

