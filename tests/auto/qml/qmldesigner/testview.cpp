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

#include "testview.h"

#include <QDebug>
#include <QUrl>
#include <qtestcase.h>
#include <abstractproperty.h>
#include <bindingproperty.h>
#include <variantproperty.h>
#include <signalhandlerproperty.h>
#include <nodelistproperty.h>
#include <nodeinstanceview.h>
#include <model.h>
#include <nodeinstanceview.h>

TestView::TestView(QmlDesigner::Model *model)
    : QmlDesigner::AbstractView(model)
{
    /*
    QmlDesigner::NodeInstanceView *nodeInstanceView = new QmlDesigner::NodeInstanceView(model, QmlDesigner::NodeInstanceServerInterface::TestModus);

    if (model)
        model->setNodeInstanceView(nodeInstanceView);
    */
}

void TestView::modelAttached(QmlDesigner::Model *model)
{
    QmlDesigner::AbstractView::modelAttached(model);
    m_methodCalls += MethodCall("modelAttached", QStringList() << QString::number(reinterpret_cast<long>(model)));
}

void TestView::modelAboutToBeDetached(QmlDesigner::Model *model)
{
    QmlDesigner::AbstractView::modelAboutToBeDetached(model);
    m_methodCalls += MethodCall("modelAboutToBeDetached", QStringList() << QString::number(reinterpret_cast<long>(model)));
}

void TestView::nodeIdChanged(const QmlDesigner::ModelNode &node, const QString& newId, const QString &oldId)
{
    m_methodCalls += MethodCall("nodeIdChanged", QStringList() << node.id() << newId << oldId);
}

void TestView::rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
    m_methodCalls += MethodCall("rootNodeTypeChanged", QStringList() << rootModelNode().id() << type << QString::number(majorVersion) << QString::number(minorVersion));
}

void TestView::fileUrlChanged(const QUrl & oldBaseUrl, const QUrl &newBaseUrl)
{
    m_methodCalls += MethodCall("fileUrlChanged", QStringList() << oldBaseUrl.toString() << newBaseUrl.toString());
}

void TestView::propertiesAboutToBeRemoved(const QList<QmlDesigner::AbstractProperty>& propertyList)
{
    QStringList propertyNames;
    foreach (const QmlDesigner::AbstractProperty &property, propertyList)
        propertyNames += QString::fromUtf8(property.name());
    m_methodCalls += MethodCall("propertiesAboutToBeRemoved", QStringList() << propertyNames.join(", "));
}

void TestView::propertiesRemoved(const QList<QmlDesigner::AbstractProperty> &propertyList)
{
    QStringList propertyNames;
    foreach (const QmlDesigner::AbstractProperty &property, propertyList)
        propertyNames += QString::fromUtf8(property.name());
    m_methodCalls += MethodCall("propertiesRemoved", QStringList() << propertyNames.join(", "));
}

void TestView::signalHandlerPropertiesChanged(const QVector<QmlDesigner::SignalHandlerProperty> &propertyList, PropertyChangeFlags )
{
    QStringList propertyNames;
    foreach (const QmlDesigner::AbstractProperty &property, propertyList)
        propertyNames += QString::fromUtf8(property.name());
    m_methodCalls += MethodCall("signalHandlerPropertiesChanged", QStringList() << propertyNames.join(", "));
}

void TestView::importsChanged(const QList<QmlDesigner::Import> &, const QList<QmlDesigner::Import> &)
{

}

void TestView::nodeCreated(const QmlDesigner::ModelNode &createdNode)
{
    m_methodCalls += MethodCall("nodeCreated", QStringList() << createdNode.id());
}

void TestView::nodeAboutToBeRemoved(const QmlDesigner::ModelNode &removedNode)
{
    m_methodCalls += MethodCall("nodeAboutToBeRemoved", QStringList() << removedNode.id());
}

void TestView::nodeRemoved(const QmlDesigner::ModelNode &removedNode, const QmlDesigner::NodeAbstractProperty &parentProperty, AbstractView::PropertyChangeFlags propertyChange)
{
    const QString parentPropertyName = parentProperty.isValid() ? QString::fromUtf8(parentProperty.name()) : QLatin1String("");

    m_methodCalls += MethodCall("nodeRemoved", QStringList() << QString::number(removedNode.internalId()) << parentPropertyName << serialize(propertyChange));
}


void TestView::nodeReparented(const QmlDesigner::ModelNode & node, const QmlDesigner::NodeAbstractProperty &newPropertyParent, const QmlDesigner::NodeAbstractProperty & oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
    m_methodCalls += MethodCall("nodeReparented", QStringList() << node.id() << QString::fromUtf8(newPropertyParent.name()) << QString::fromUtf8(oldPropertyParent.name()) << serialize(propertyChange));
}

void TestView::nodeAboutToBeReparented(const QmlDesigner::ModelNode &node, const QmlDesigner::NodeAbstractProperty &newPropertyParent, const QmlDesigner::NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
    m_methodCalls += MethodCall("nodeAboutToBeReparented", QStringList() << node.id() << QString::fromUtf8(newPropertyParent.name()) << QString::fromUtf8(oldPropertyParent.name()) << serialize(propertyChange));
}

void TestView::bindingPropertiesChanged(const QList<QmlDesigner::BindingProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    QStringList propertyNames;
    foreach (const QmlDesigner::BindingProperty &property, propertyList)
        propertyNames += QString::fromUtf8(property.name());
    m_methodCalls += MethodCall("bindingPropertiesChanged", QStringList() << propertyNames.join(", ") << serialize(propertyChange));
}

void TestView::variantPropertiesChanged(const QList<QmlDesigner::VariantProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    QStringList propertyNames;
    foreach (const QmlDesigner::VariantProperty &property, propertyList)
        propertyNames += QString::fromUtf8(property.name());

    m_methodCalls += MethodCall("variantPropertiesChanged", QStringList() << propertyNames.join(", ") << serialize(propertyChange));
}

void TestView::selectedNodesChanged(const QList<QmlDesigner::ModelNode> &selectedNodeList,
                       const QList<QmlDesigner::ModelNode> &lastSelectedNodeList)
{
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
    m_methodCalls += MethodCall("nodeOrderChanged", QStringList() << QString::fromUtf8(listProperty.name()) << movedNode.id() << QString::number(oldIndex));
}

void TestView::instancePropertyChanged(const QList<QPair<QmlDesigner::ModelNode, QmlDesigner::PropertyName> > &)
{

}

void TestView::instancesCompleted(const QVector<QmlDesigner::ModelNode> &)
{

}

void TestView::instanceInformationsChanged(const QMultiHash<QmlDesigner::ModelNode, QmlDesigner::InformationName> &)
{

}

void TestView::instancesRenderImageChanged(const QVector<QmlDesigner::ModelNode> &)
{

}

void TestView::instancesPreviewImageChanged(const QVector<QmlDesigner::ModelNode> &)
{

}

void TestView::instancesChildrenChanged(const QVector<QmlDesigner::ModelNode> &)
{

}

void TestView::instancesToken(const QString &, int, const QVector<QmlDesigner::ModelNode> &)
{

}

void TestView::nodeSourceChanged(const QmlDesigner::ModelNode &, const QString &)
{

}

void TestView::scriptFunctionsChanged(const QmlDesigner::ModelNode &, const QStringList &)
{

}

void TestView::currentStateChanged(const QmlDesigner::ModelNode &node)
{
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
    return QmlDesigner::AbstractView::nodeInstanceView();

}

QmlDesigner::QmlObjectNode TestView::rootQmlObjectNode() const
{
    return rootModelNode();
}

void TestView::setCurrentState(const QmlDesigner::QmlModelState &node)
{
    setCurrentStateNode(node.modelNode());
}

QmlDesigner::QmlItemNode TestView::rootQmlItemNode() const { return rootModelNode(); }

QmlDesigner::QmlModelState TestView::baseState() const { return rootModelNode(); }

QmlDesigner::QmlObjectNode TestView::createQmlObjectNode(const QmlDesigner::TypeName &typeName,
                                                         int majorVersion,
                                                         int minorVersion,
                                                         const QmlDesigner::PropertyListType &propertyList)

{
    return createModelNode(typeName, majorVersion, minorVersion, propertyList);
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

