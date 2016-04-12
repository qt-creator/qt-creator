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
#include <projectexplorer/session.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/project.h>

#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>

#include <QMessageBox>

namespace QmlDesigner {

Q_LOGGING_CATEGORY(documentManagerLog, "qtc.qtquickdesigner.documentmanager")

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
            && modelNode.metaInfo().isSubclassOf("QtQuick.Loader")) {

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
            && modelNode.metaInfo().isSubclassOf("QtQuick.Loader")
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
    } else if (!m_currentDesignDocument.isNull()) {
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

Utils::FileName DocumentManager::currentFilePath()
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument()->fileName();
}

QStringList DocumentManager::isoIconsQmakeVariableValue(const QString &proPath)
{
    ProjectExplorer::Node *node = ProjectExplorer::SessionManager::nodeForFile(Utils::FileName::fromString(proPath));
    if (!node) {
        qCWarning(documentManagerLog) << "No node for .pro:" << proPath;
        return QStringList();
    }

    ProjectExplorer::Node *parentNode = node->parentFolderNode();
    if (!parentNode) {
        qCWarning(documentManagerLog) << "No parent node for node at" << proPath;
        return QStringList();
    }

    QmakeProjectManager::QmakeProFileNode *proNode = dynamic_cast<QmakeProjectManager::QmakeProFileNode*>(parentNode);
    if (!proNode) {
        qCWarning(documentManagerLog) << "Parent node for node at" << proPath << "could not be converted to a QmakeProFileNode";
        return QStringList();
    }

    return proNode->variableValue(QmakeProjectManager::IsoIconsVar);
}

bool DocumentManager::setIsoIconsQmakeVariableValue(const QString &proPath, const QStringList &value)
{
    ProjectExplorer::Node *node = ProjectExplorer::SessionManager::nodeForFile(Utils::FileName::fromString(proPath));
    if (!node) {
        qCWarning(documentManagerLog) << "No node for .pro:" << proPath;
        return false;
    }

    ProjectExplorer::Node *parentNode = node->parentFolderNode();
    if (!parentNode) {
        qCWarning(documentManagerLog) << "No parent node for node at" << proPath;
        return false;
    }

    QmakeProjectManager::QmakeProFileNode *proNode = dynamic_cast<QmakeProjectManager::QmakeProFileNode*>(parentNode);
    if (!proNode) {
        qCWarning(documentManagerLog) << "Node for" << proPath << "could not be converted to a QmakeProFileNode";
        return false;
    }

    int flags = QmakeProjectManager::Internal::ProWriter::ReplaceValues | QmakeProjectManager::Internal::ProWriter::MultiLine;
    return proNode->setProVariable("ISO_ICONS", value, QString(), flags);
}

void DocumentManager::findPathToIsoProFile(bool *iconResourceFileAlreadyExists, QString *resourceFilePath,
    QString *resourceFileProPath, const QString &isoIconsQrcFile)
{
    Utils::FileName qmlFileName = QmlDesignerPlugin::instance()->currentDesignDocument()->fileName();
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::projectForFile(qmlFileName);
    ProjectExplorer::Node *node = ProjectExplorer::SessionManager::nodeForFile(qmlFileName)->parentFolderNode();
    ProjectExplorer::Node *iconQrcFileNode = 0;

    while (node && !iconQrcFileNode) {
        qCDebug(documentManagerLog) << "Checking" << node->displayName() << "(" << node << node->nodeType() << ")";

        if (node->nodeType() == ProjectExplorer::VirtualFolderNodeType && node->displayName() == "Resources") {
            ProjectExplorer::VirtualFolderNode *virtualFolderNode = dynamic_cast<ProjectExplorer::VirtualFolderNode*>(node);

            for (int subFolderIndex = 0; subFolderIndex < virtualFolderNode->subFolderNodes().size() && !iconQrcFileNode; ++subFolderIndex) {
                ProjectExplorer::FolderNode *subFolderNode = virtualFolderNode->subFolderNodes().at(subFolderIndex);

                qCDebug(documentManagerLog) << "Checking if" << subFolderNode->displayName() << "("
                    << subFolderNode << subFolderNode->nodeType() << ") is" << isoIconsQrcFile;

                if (subFolderNode->nodeType() == ProjectExplorer::FolderNodeType
                    && subFolderNode->displayName() == isoIconsQrcFile) {
                    qCDebug(documentManagerLog) << "Found" << isoIconsQrcFile << "in" << virtualFolderNode->filePath();

                    iconQrcFileNode = subFolderNode;
                    *resourceFileProPath = iconQrcFileNode->projectNode()->filePath().toString();
                }
            }
        }

        if (!iconQrcFileNode) {
            qCDebug(documentManagerLog) << "Didn't find" << isoIconsQrcFile
                << "in" << node->displayName() << "; checking parent";
            node = node->parentFolderNode();
        }
    }

    if (!iconQrcFileNode) {
        // The QRC file that we want doesn't exist or is not listed under RESOURCES in the .pro.
        *resourceFilePath = project->projectDirectory().toString() + "/" + isoIconsQrcFile;

        // We assume that the .pro containing the QML file is an acceptable place to add the .qrc file.
        ProjectExplorer::ProjectNode *projectNode = ProjectExplorer::SessionManager::nodeForFile(qmlFileName)->projectNode();
        *resourceFileProPath = projectNode->filePath().toString();
    } else {
        // We found the QRC file that we want.
        QString projectDirectory = ProjectExplorer::SessionManager::projectForNode(iconQrcFileNode)->projectDirectory().toString();
        *resourceFilePath = projectDirectory + "/" + isoIconsQrcFile;
    }

    *iconResourceFileAlreadyExists = iconQrcFileNode != 0;
}

bool DocumentManager::isoProFileSupportsAddingExistingFiles(const QString &resourceFileProPath)
{
    ProjectExplorer::Node *node = ProjectExplorer::SessionManager::nodeForFile(Utils::FileName::fromString(resourceFileProPath));
    ProjectExplorer::ProjectNode *projectNode = dynamic_cast<ProjectExplorer::ProjectNode*>(node->parentFolderNode());

    if (!projectNode->supportedActions(projectNode).contains(ProjectExplorer::AddExistingFile)) {
        qCWarning(documentManagerLog) << "Project" << projectNode->displayName() << "does not support adding existing files";
        return false;
    }

    return true;
}

bool DocumentManager::addResourceFileToIsoProject(const QString &resourceFileProPath, const QString &resourceFilePath)
{
    ProjectExplorer::Node *node = ProjectExplorer::SessionManager::nodeForFile(Utils::FileName::fromString(resourceFileProPath));
    ProjectExplorer::ProjectNode *projectNode = dynamic_cast<ProjectExplorer::ProjectNode*>(node->parentFolderNode());

    if (!projectNode->addFiles(QStringList() << resourceFilePath)) {
        qCWarning(documentManagerLog) << "Failed to add resource file to" << projectNode->displayName();
        return false;
    }
    return true;
}


} // namespace QmlDesigner
