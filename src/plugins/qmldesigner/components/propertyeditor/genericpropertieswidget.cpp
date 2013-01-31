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

#include <QSet>
#include <QDebug>
#include <QWidget>
#include <QApplication>

#include "qteditorfactory.h"
#include "qtpropertymanager.h"
#include "qttreepropertybrowser.h"
#include "qtvariantproperty.h"

#include "genericpropertieswidget.h"
#include "nodemetainfo.h"
#include "propertymetainfo.h"

#include "modelutilities.h"

using namespace QmlDesigner;

GenericPropertiesWidget::GenericPropertiesWidget(QWidget* parent):
    AbstractView(parent),
    editor(new QtTreePropertyBrowser()),
    variantManager(new QtVariantPropertyManager()),
    enumManager(new QtEnumPropertyManager())
{
}

QWidget* GenericPropertiesWidget::createPropertiesPage()
{
    editor->setRootIsDecorated(false);

    QtEnumEditorFactory *enumFactory = new QtEnumEditorFactory();
    editor->setFactoryForManager(enumManager, enumFactory);

    QtVariantEditorFactory *variantFactory = new QtVariantEditorFactory();
    editor->setFactoryForManager(variantManager, variantFactory);

    reconnectEditor();

    return editor;
}

void GenericPropertiesWidget::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                                   const QList<ModelNode> &lastSelectedNodeList)
{
    Q_UNUSED(lastSelectedNodeList);

    if (selectedNode.isValid() && selectedNodeList.contains(selectedNode))
        return;

    if (selectedNodeList.isEmpty())
        select(ModelNode());
    else
        select(selectedNodeList.at(0));
}

void GenericPropertiesWidget::select(const ModelNode& node)
{
    if (!node.isValid() || (selectedNode.isValid() && node == selectedNode))
        return;

    selectedNode = node;

    disconnectEditor();

    editor->setUpdatesEnabled(false);
    editor->clear();
    buildPropertyEditorItems();
    editor->setUpdatesEnabled(true);

    reconnectEditor();
}

void GenericPropertiesWidget::disconnectEditor()
{
    disconnect(variantManager, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(propertyChanged(QtProperty*)));
    disconnect(enumManager, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(propertyChanged(QtProperty*)));
}

void GenericPropertiesWidget::reconnectEditor()
{
    connect(enumManager, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(propertyChanged(QtProperty*)));
    connect(variantManager, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(propertyChanged(QtProperty*)));
}

QtProperty* GenericPropertiesWidget::addVariantProperty(const PropertyMetaInfo& propertyMetaInfo,
                                                        const QHash<QString, NodeProperty>& propertiesWithValues,
                                                        const NodeInstance& instance)
{
//    qDebug() << "\t\tAdding variant property" << propertyMetaInfo.name() << "...";

    if (variantManager->isPropertyTypeSupported(propertyMetaInfo.variantTypeId())) {
        QtVariantProperty* item = variantManager->addProperty(propertyMetaInfo.variantTypeId(), propertyMetaInfo.name());

        if (propertiesWithValues.contains(propertyMetaInfo.name())) {
            NodeProperty nodeProperty(propertiesWithValues[propertyMetaInfo.name()]);

            if (nodeProperty.isValid()) {
                item->setValue(nodeProperty.value());
                item->setModified(true);
            }
        }

        if (!item->isModified())
            item->setValue(instance.property(propertyMetaInfo.name())); //TODO fix this

        return item;
    } else {
//        qDebug() << "\t\t"
//                 << "Property type not supported for property"
//                 << propertyMetaInfo.name()
//                 << ", type name:" << propertyMetaInfo.variantTypeId()
//                 << "(" << propertyMetaInfo.type() << ")";
        return 0;
    }
}

QtProperty* GenericPropertiesWidget::addEnumProperty(const PropertyMetaInfo& propertyMetaInfo,
                                                     const QHash<QString, NodeProperty>& propertiesWithValues,
                                                     const NodeInstance& instance)
{
//    qDebug() << "\t\tAdding enum property" << propertyMetaInfo.name() << "...";

    QtProperty* item = enumManager->addProperty(propertyMetaInfo.name());

    QList<QString> elementNames(propertyMetaInfo.enumerator().elementNames());
    enumManager->setEnumNames(item, elementNames);

    if (propertiesWithValues.contains(propertyMetaInfo.name())) {
        NodeProperty nodeProperty(propertiesWithValues[propertyMetaInfo.name()]);

        int selectionIndex = elementNames.indexOf(nodeProperty.value().toString());
        if (selectionIndex != -1) {
            enumManager->setValue(item, selectionIndex);
            item->setModified(true);
        }
    }

    if (!item->isModified()) {
        int selectionIndex = elementNames.indexOf(instance.property(propertyMetaInfo.name()).toString()); // TODO Fix this
        if (selectionIndex != -1)
            enumManager->setValue(item, selectionIndex);
    }

    return item;
}

QtProperty* GenericPropertiesWidget::addFlagProperty(const PropertyMetaInfo &/*propertyMetaInfo*/,
                                                     const QHash<QString, NodeProperty> &/*propertiesWithValues*/,
                                                     const NodeInstance &/*instance*/)
{
//    qDebug() << "\t\tFlags are not yet supported. (Property: " << propertyMetaInfo.name() << ")";

    return 0;
}

QtProperty* GenericPropertiesWidget::addProperties(const NodeMetaInfo& nodeMetaInfo,
                                                   const QHash<QString, NodeProperty>& propertiesWithValues,
                                                   const NodeInstance& instance)
{
//    qDebug() << "\tAdding" << nodeMetaInfo.properties().size()
//             << "properties for (super)node with type" << nodeMetaInfo.className() << "...";

    QtProperty* groupItem = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                                        nodeMetaInfo.typeName());

    foreach (PropertyMetaInfo propMetaInfo, nodeMetaInfo.properties()) {
        if (!propMetaInfo.isVisibleToPropertyEditor())
            continue;

        QtProperty* property = 0;

        if (propMetaInfo.isEnumType())
            property = addEnumProperty(propMetaInfo, propertiesWithValues, instance);
        else if (propMetaInfo.isFlagType())
            property = addFlagProperty(propMetaInfo, propertiesWithValues, instance);
        else
            property = addVariantProperty(propMetaInfo, propertiesWithValues, instance);

        if (property)
            groupItem->addSubProperty(property);
    }

    return groupItem;
}

void GenericPropertiesWidget::buildPropertyEditorItems()
{
    if (!selectedNode.isValid())
        return;

//    qDebug() << "buildPropertyEditorItems for node" << selectedNode.name() << "...";

    QList<NodeMetaInfo> allClasses;
    allClasses.append(selectedNode.metaInfo());
    allClasses += selectedNode.metaInfo().superClasses();
//    qDebug() << "\tNode has" << allClasses.size() << "(super) classes";

    QHash<QString, NodeProperty> propertiesWithValues;
    foreach (const NodeProperty & nodeProperty, selectedNode.properties()) {
        propertiesWithValues[nodeProperty.name()] = nodeProperty;
    }

//    qDebug() << "\tNode has" << propertiesWithValues.size() << "properties set.";

//    NodeInstance instance = ModelUtilities::instanceForNode(selectedNode);
//
//    foreach (const NodeMetaInfo &info, allClasses) {
//        // FIXME: the add property is quite (too) expensive!
//        editor->addProperty(addProperties(info, propertiesWithValues, instance));
//    }
}

static bool nodeHasProperty(const ModelNode& node, const QString& name)
{
    if (node.metaInfo().hasProperty(name))
        return true;

    foreach (const NodeMetaInfo& info, node.metaInfo().superClasses()) {
        if (info.hasProperty(name))
            return true;
    }

    return false;
}

void GenericPropertiesWidget::propertyChanged(QtProperty* property)
{
//    qDebug() << "property" << property->propertyName() << "changed...";

    QtVariantProperty* variantProperty = dynamic_cast<QtVariantProperty*>(property);

    if (variantProperty) {
//        qDebug() << "\tnew value: " << variantProperty->value();

        if (selectedNode.property(property->propertyName()).value() == variantProperty->value()) {
//            qDebug() << "\twhich is the same as the old one, so we'll forget about it.";
        } else {
            if (nodeHasProperty(selectedNode, property->propertyName())) {
                selectedNode.setPropertyValue(property->propertyName(), variantProperty->value());

                if (!property->isModified())
                    property->setModified(true);
            } else {
//                qDebug() << "--- property " << property->propertyName() << "ignored: it doesn't exist in the metadata.";
            }
        }
    } else {
//        qDebug("\tEep: changed property is not a variant property");
    }
}

void GenericPropertiesWidget::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
}

void GenericPropertiesWidget::nodeCreated(const ModelNode&)
{
}

void GenericPropertiesWidget::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (selectedNode.isValid() && removedNode.isValid() && selectedNode == removedNode)
        select(selectedNode.parentNode());
}

void GenericPropertiesWidget::propertyAdded(const NodeState& state, const NodeProperty&)
{
    if (selectedNode.isValid() && state.isValid() && selectedNode == state.modelNode())
        select(state.modelNode());
}

void GenericPropertiesWidget::propertyAboutToBeRemoved(const NodeState& /* state */, const NodeProperty&)
{
}

void GenericPropertiesWidget::nodeReparented(const ModelNode &node, const ModelNode &oldParent, const ModelNode &newParent)
{
    Q_UNUSED(node);
    Q_UNUSED(oldParent);
    Q_UNUSED(newParent);
}

void GenericPropertiesWidget::propertyValueChanged(const NodeState& state, const NodeProperty& property,
                              const QVariant&, const QVariant& )
{
    if (!selectedNode.isValid() || selectedNode != state.modelNode())
        return;

    disconnectEditor();

    ModelNode node(state.modelNode());

    foreach (QtProperty* qtProperty, enumManager->properties()) {
        if (qtProperty->propertyName() == property.name()) {
            QList<QString> elementNames = node.property(property.name()).metaInfo().enumerator().elementNames();
            int selectionIndex = elementNames.indexOf(property.value().toString());
            if (selectionIndex != -1) {
                enumManager->setValue(qtProperty, selectionIndex);
                qtProperty->setModified(true);
            }

            reconnectEditor();
            return;
        }
    }

    foreach (QtProperty* qtProperty, variantManager->properties()) {
        if (qtProperty->propertyName() == property.name()) {
            (dynamic_cast<QtVariantProperty*>(qtProperty))->setValue(property.value());
            qtProperty->setModified(true);

            reconnectEditor();
            return;
        }
    }

    reconnectEditor();
}

void GenericPropertiesWidget::modelStateAboutToBeRemoved(const ModelState &/*modelState*/)
{
//    TODO: implement
}

void GenericPropertiesWidget::modelStateAdded(const ModelState &/*modelState*/)
{
//    TODO: implement
}

void GenericPropertiesWidget::nodeStatesAboutToBeRemoved(const QList<NodeState> &/*nodeStateList*/)
{
//    TODO: implement
}

void GenericPropertiesWidget::nodeStatesAdded(const QList<NodeState> &/*nodeStateList*/)
{
//    TODO: implement
}

void GenericPropertiesWidget::anchorsChanged(const NodeState &/*nodeState*/)
{
//    TODO: implement
}
