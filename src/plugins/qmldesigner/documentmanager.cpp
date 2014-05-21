/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "documentmanager.h"
#include "qmldesignerplugin.h"

#include <modelnode.h>
#include <qmlitemnode.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <bindingproperty.h>

namespace QmlDesigner {

static inline DesignDocument* currentDesignDocument()
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
}

static inline void getProperties(const ModelNode &node, QHash<PropertyName, QVariant> &propertyHash)
{
    if (QmlObjectNode::isValidQmlObjectNode(node)) {
        foreach (const AbstractProperty &abstractProperty, node.properties()) {
            if (abstractProperty.isVariantProperty()
                    || (abstractProperty.isBindingProperty()
                        && !abstractProperty.name().contains("anchors.")))
                propertyHash.insert(abstractProperty.name(), QmlObjectNode(node).instanceValue(abstractProperty.name()));
        }

        if (QmlItemNode::isValidQmlItemNode(node)) {
            QmlItemNode itemNode(node);

            propertyHash.insert("width", itemNode.instanceValue("width"));
            propertyHash.insert("height", itemNode.instanceValue("height"));
            propertyHash.remove("x");
            propertyHash.remove("y");
            propertyHash.remove("rotation");
            propertyHash.remove("opacity");
        }
    }
}

static inline void applyProperties(ModelNode &node, const QHash<PropertyName, QVariant> &propertyHash)
{
    QHash<PropertyName, QVariant> auxiliaryData  = node.auxiliaryData();

    foreach (const PropertyName &propertyName, auxiliaryData.keys()) {
        if (node.hasAuxiliaryData(propertyName))
            node.setAuxiliaryData(propertyName, QVariant());
    }

    QHashIterator<PropertyName, QVariant> propertyIterator(propertyHash);
    while (propertyIterator.hasNext()) {
        propertyIterator.next();
        const PropertyName propertyName = propertyIterator.key();
        if (propertyName == "width" || propertyName == "height") {
            node.setAuxiliaryData(propertyIterator.key(), propertyIterator.value());
        } else if (node.property(propertyIterator.key()).isDynamic() &&
                   node.property(propertyIterator.key()).dynamicTypeName() == "alias" &&
                   node.property(propertyIterator.key()).isBindingProperty()) {
            AbstractProperty targetProperty = node.bindingProperty(propertyIterator.key()).resolveToProperty();
            if (targetProperty.isValid())
                targetProperty.parentModelNode().setAuxiliaryData(targetProperty.name() + "@NodeInstance", propertyIterator.value());
        } else {
            node.setAuxiliaryData(propertyIterator.key() + "@NodeInstance", propertyIterator.value());
        }
    }
}

static inline void openFileForComponent(const ModelNode &node)
{
    QmlDesignerPlugin::instance()->viewManager().nextFileIsCalledInternally();

    //int width = 0;
    //int height = 0;
    QHash<PropertyName, QVariant> propertyHash;
    if (node.metaInfo().isFileComponent()) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        Core::EditorManager::openEditor(node.metaInfo().componentFileName(), Core::Id(), Core::EditorManager::DoNotMakeVisible);
    } else if (node.metaInfo().isView() &&
               node.hasNodeProperty("delegate") &&
               node.nodeProperty("delegate").modelNode().metaInfo().isFileComponent()) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        Core::EditorManager::openEditor(node.nodeProperty("delegate").modelNode().metaInfo().componentFileName(),
                                        Core::Id(), Core::EditorManager::DoNotMakeVisible);
    }
    ModelNode rootModelNode = currentDesignDocument()->rewriterView()->rootModelNode();
    applyProperties(rootModelNode, propertyHash);
    //rootModelNode.setAuxiliaryData("width", width);
    //rootModelNode.setAuxiliaryData("height", height);
}

static inline void openInlineComponent(const ModelNode &node)
{

    if (!node.isValid() || !node.metaInfo().isValid())
        return;

    if (!currentDesignDocument())
        return;

    QHash<PropertyName, QVariant> propertyHash;

    if (node.nodeSourceType() == ModelNode::NodeWithComponentSource) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        currentDesignDocument()->changeToSubComponent(node);
    } else if (node.metaInfo().isView()
               && node.hasNodeProperty("delegate")
               && node.nodeProperty("delegate").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        currentDesignDocument()->changeToSubComponent(node.nodeProperty("delegate").modelNode());
    }

    ModelNode rootModelNode = currentDesignDocument()->rewriterView()->rootModelNode();
    applyProperties(rootModelNode, propertyHash);
    //rootModelNode.setAuxiliaryData("width", width);
    //rootModelNode.setAuxiliaryData("height", height);
}

static inline bool isFileComponent(const ModelNode &node)
{
    if (!node.isValid() || !node.metaInfo().isValid())
        return false;

    if (node.metaInfo().isFileComponent())
        return true;

    if (node.metaInfo().isView() &&
        node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().metaInfo().isFileComponent())
            return true;
    }

    return false;
}

DocumentManager::DocumentManager()
    : QObject()
{
}

DocumentManager::~DocumentManager()
{
    foreach (const QPointer<DesignDocument> &designDocument, m_designDocumentHash)
        delete designDocument.data();
}

void DocumentManager::setCurrentDesignDocument(Core::IEditor *editor)
{
    if (editor) {
        m_currentDesignDocument = m_designDocumentHash.value(editor);
        if (m_currentDesignDocument == 0) {
            m_currentDesignDocument = new DesignDocument;
            m_designDocumentHash.insert(editor, m_currentDesignDocument);
            m_currentDesignDocument->setEditor(editor);
        }
    } else {
        m_currentDesignDocument->resetToDocumentModel();
        m_currentDesignDocument.clear();
    }
}

DesignDocument *DocumentManager::currentDesignDocument() const
{
    return m_currentDesignDocument.data();
}

bool DocumentManager::hasCurrentDesignDocument() const
{
    return m_currentDesignDocument.data();
}

void DocumentManager::removeEditors(QList<Core::IEditor *> editors)
{
    foreach (Core::IEditor *editor, editors)
        delete m_designDocumentHash.take(editor).data();
}

void DocumentManager::goIntoComponent(const ModelNode &modelNode)
{
    if (modelNode.isValid() && modelNode.isComponent()) {
        QmlDesignerPlugin::instance()->viewManager().setComponentNode(modelNode);
        if (isFileComponent(modelNode))
            openFileForComponent(modelNode);
        else
            openInlineComponent(modelNode);
    }
}


} // namespace QmlDesigner
