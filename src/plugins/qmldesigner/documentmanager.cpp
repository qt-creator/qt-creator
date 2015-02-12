/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "documentmanager.h"
#include "qmldesignerplugin.h"

#include <modelnode.h>
#include <qmlitemnode.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <bindingproperty.h>
#include <variantproperty.h>

#include <utils/textfileformat.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <QMessageBox>

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

static void openFileComponent(const ModelNode &modelNode)
{
    QmlDesignerPlugin::instance()->viewManager().nextFileIsCalledInternally();

    QHash<PropertyName, QVariant> propertyHash;

    getProperties(modelNode, propertyHash);
    Core::EditorManager::openEditor(modelNode.metaInfo().componentFileName(), Core::Id(), Core::EditorManager::DoNotMakeVisible);

    ModelNode rootModelNode = currentDesignDocument()->rewriterView()->rootModelNode();
    applyProperties(rootModelNode, propertyHash);
}

static void openFileComponentForDelegate(const ModelNode &modelNode)
{
    openFileComponent(modelNode.nodeProperty("delegate").modelNode());
}

static void openComponentSourcePropertyOfLoader(const ModelNode &modelNode)
{
    QmlDesignerPlugin::instance()->viewManager().nextFileIsCalledInternally();

    QHash<PropertyName, QVariant> propertyHash;

    getProperties(modelNode, propertyHash);

    ModelNode componentModelNode;

    if (modelNode.hasNodeProperty("sourceComponent")) {
        componentModelNode = modelNode.nodeProperty("sourceComponent").modelNode();
    } else if (modelNode.hasNodeListProperty("component")) {

     /*
     * The component property should be a NodeProperty, but currently is a NodeListProperty, because
     * the default property is always implcitly a NodeListProperty. This is something that has to be fixed.
     */

        componentModelNode = modelNode.nodeListProperty("component").toModelNodeList().first();
    }

    Core::EditorManager::openEditor(componentModelNode.metaInfo().componentFileName(), Core::Id(), Core::EditorManager::DoNotMakeVisible);

    ModelNode rootModelNode = currentDesignDocument()->rewriterView()->rootModelNode();
    applyProperties(rootModelNode, propertyHash);
}

static void openSourcePropertyOfLoader(const ModelNode &modelNode)
{
    QmlDesignerPlugin::instance()->viewManager().nextFileIsCalledInternally();

    QHash<PropertyName, QVariant> propertyHash;

    QString componentFileName = modelNode.variantProperty("source").value().toString();
    QString componentFilePath = modelNode.model()->fileUrl().resolved(QUrl::fromLocalFile(componentFileName)).toLocalFile();

    getProperties(modelNode, propertyHash);
    Core::EditorManager::openEditor(componentFilePath, Core::Id(), Core::EditorManager::DoNotMakeVisible);

    ModelNode rootModelNode = currentDesignDocument()->rewriterView()->rootModelNode();
    applyProperties(rootModelNode, propertyHash);
}


static void handleComponent(const ModelNode &modelNode)
{
    if (modelNode.nodeSourceType() == ModelNode::NodeWithComponentSource)
        currentDesignDocument()->changeToSubComponent(modelNode);
}

static void handleDelegate(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isView()
            && modelNode.hasNodeProperty("delegate")
            && modelNode.nodeProperty("delegate").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource)
        currentDesignDocument()->changeToSubComponent(modelNode.nodeProperty("delegate").modelNode());
}

static void handleTabComponent(const ModelNode &modelNode)
{
    if (modelNode.hasNodeProperty("component")
            && modelNode.nodeProperty("component").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource) {
        currentDesignDocument()->changeToSubComponent(modelNode.nodeProperty("component").modelNode());
    }
}

static inline void openInlineComponent(const ModelNode &modelNode)
{

    if (!modelNode.isValid() || !modelNode.metaInfo().isValid())
        return;

    if (!currentDesignDocument())
        return;

    QHash<PropertyName, QVariant> propertyHash;

    getProperties(modelNode, propertyHash);

    handleComponent(modelNode);
    handleDelegate(modelNode);
    handleTabComponent(modelNode);

    ModelNode rootModelNode = currentDesignDocument()->rewriterView()->rootModelNode();
    applyProperties(rootModelNode, propertyHash);
}

static bool isFileComponent(const ModelNode &node)
{
    if (node.isValid()
            && node.metaInfo().isValid()
            && node.metaInfo().isFileComponent())
        return true;

    return false;
}

static bool hasDelegateWithFileComponent(const ModelNode &node)
{
    if (node.isValid()
            && node.metaInfo().isValid()
            && node.metaInfo().isView()
            && node.hasNodeProperty("delegate")
            && node.nodeProperty("delegate").modelNode().metaInfo().isFileComponent())
        return true;

    return false;
}

static bool isLoaderWithSourceComponent(const ModelNode &modelNode)
{
    if (modelNode.isValid()
            && modelNode.metaInfo().isValid()
            && modelNode.metaInfo().isSubclassOf("QtQuick.Loader", -1, -1)) {

        if (modelNode.hasNodeProperty("sourceComponent"))
            return true;
        if (modelNode.hasNodeListProperty("component"))
            return true;
    }

    return false;

}

static bool hasSourceWithFileComponent(const ModelNode &modelNode)
{
    if (modelNode.isValid()
            && modelNode.metaInfo().isValid()
            && modelNode.metaInfo().isSubclassOf("QtQuick.Loader", -1, -1)
            && modelNode.hasVariantProperty("source"))
        return true;

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
            openFileComponent(modelNode);
        else if (hasDelegateWithFileComponent(modelNode))
            openFileComponentForDelegate(modelNode);
        else if (hasSourceWithFileComponent(modelNode))
            openSourcePropertyOfLoader(modelNode);
        else if (isLoaderWithSourceComponent(modelNode))
            openComponentSourcePropertyOfLoader(modelNode);
        else
            openInlineComponent(modelNode);
    }
}

bool DocumentManager::createFile(const QString &filePath, const QString &contents)
{
    Utils::TextFileFormat textFileFormat;
    textFileFormat.codec = Core::EditorManager::defaultTextCodec();
    QString errorMessage;

    return textFileFormat.writeFile(filePath, contents, &errorMessage);
}

void DocumentManager::addFileToVersionControl(const QString &directoryPath, const QString &newFilePath)
{
    Core::IVersionControl *versionControl = Core::VcsManager::findVersionControlForDirectory(directoryPath);
    if (versionControl && versionControl->supportsOperation(Core::IVersionControl::AddOperation)) {
        const QMessageBox::StandardButton button =
                QMessageBox::question(Core::ICore::mainWindow(),
                                      Core::VcsManager::msgAddToVcsTitle(),
                                      Core::VcsManager::msgPromptToAddToVcs(QStringList(newFilePath), versionControl),
                                      QMessageBox::Yes | QMessageBox::No);
        if (button == QMessageBox::Yes && !versionControl->vcsAdd(newFilePath)) {
            Core::AsynchronousMessageBox::warning(Core::VcsManager::msgAddToVcsFailedTitle(),
                                                   Core::VcsManager::msgToAddToVcsFailed(QStringList(newFilePath), versionControl));
        }
    }
}


} // namespace QmlDesigner
