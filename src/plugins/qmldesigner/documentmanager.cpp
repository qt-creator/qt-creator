// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "documentmanager.h"
#include "qmldesignerplugin.h"

#include <bindingproperty.h>
#include <model/modelutils.h>
#include <modelnode.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <qmldesignerprojectmanager.h>
#include <qmlitemnode.h>
#include <variantproperty.h>

#include <utils/qtcassert.h>
#include <utils/textfileformat.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>

#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>

#include <QMessageBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmlDesigner {

Q_LOGGING_CATEGORY(documentManagerLog, "qtc.qtquickdesigner.documentmanager", QtWarningMsg)

inline static QmlDesigner::DesignDocument *designDocument()
{
    return QmlDesigner::QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
}

inline static QHash<PropertyName, QVariant> getProperties(const ModelNode &node)
{
    QHash<PropertyName, QVariant> propertyHash;
    if (QmlObjectNode::isValidQmlObjectNode(node)) {
        const QList<AbstractProperty> abstractProperties = node.properties();
        for (const AbstractProperty &abstractProperty : abstractProperties) {
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

    return propertyHash;
}

inline static void applyProperties(ModelNode &node, const QHash<PropertyName, QVariant> &propertyHash)
{
    const auto auxiliaryData = node.auxiliaryData(AuxiliaryDataType::NodeInstancePropertyOverwrite);

    for (const auto &element : auxiliaryData)
        node.removeAuxiliaryData(AuxiliaryDataType::NodeInstancePropertyOverwrite, element.first);

    for (auto propertyIterator = propertyHash.cbegin(), end = propertyHash.cend();
              propertyIterator != end;
              ++propertyIterator) {
        const PropertyName propertyName = propertyIterator.key();
        if (propertyName == "width" || propertyName == "height") {
            node.setAuxiliaryData(AuxiliaryDataType::NodeInstancePropertyOverwrite,
                                  propertyIterator.key(),
                                  propertyIterator.value());
        }
    }
}

static void openFileComponentForFile(const QString &fileName)
{
    QmlDesignerPlugin::instance()->viewManager().nextFileIsCalledInternally();
    Core::EditorManager::openEditor(FilePath::fromString(fileName),
                                    Utils::Id(),
                                    Core::EditorManager::DoNotMakeVisible);
}

static void openFileComponent(const ModelNode &modelNode)
{
    openFileComponentForFile(ModelUtils::componentFilePath(modelNode));
}

static void openFileComponentForDelegate(const ModelNode &modelNode)
{
    openFileComponent(modelNode.nodeProperty("delegate").modelNode());
}

static void openComponentSourcePropertyOfLoader(const ModelNode &modelNode)
{
    QmlDesignerPlugin::instance()->viewManager().nextFileIsCalledInternally();

    ModelNode componentModelNode;

    if (modelNode.hasNodeProperty("sourceComponent")) {
        componentModelNode = modelNode.nodeProperty("sourceComponent").modelNode();
    } else if (modelNode.hasNodeListProperty("component")) {

     /*
     * The component property should be a NodeProperty, but currently is a NodeListProperty, because
     * the default property is always implcitly a NodeListProperty. This is something that has to be fixed.
     */

        componentModelNode = modelNode.nodeListProperty("component").toModelNodeList().constFirst();
    }

    Core::EditorManager::openEditor(FilePath::fromString(
                                        componentModelNode.metaInfo().componentFileName()),
                                    Utils::Id(),
                                    Core::EditorManager::DoNotMakeVisible);
}

static void openSourcePropertyOfLoader(const ModelNode &modelNode)
{
    QmlDesignerPlugin::instance()->viewManager().nextFileIsCalledInternally();

    QString componentFileName = modelNode.variantProperty("source").value().toString();

    QFileInfo fileInfo(modelNode.model()->fileUrl().toLocalFile());
    Core::EditorManager::openEditor(FilePath::fromString(fileInfo.absolutePath())
                                        / componentFileName,
                                    Utils::Id(),
                                    Core::EditorManager::DoNotMakeVisible);
}


static void handleComponent(const ModelNode &modelNode)
{
    if (modelNode.nodeSourceType() == ModelNode::NodeWithComponentSource)
        designDocument()->changeToSubComponent(modelNode);
}

static void handleDelegate(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isView()
            && modelNode.hasNodeProperty("delegate")
            && modelNode.nodeProperty("delegate").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource)
        designDocument()->changeToSubComponent(modelNode.nodeProperty("delegate").modelNode());
}

static void handleTabComponent(const ModelNode &modelNode)
{
    if (modelNode.hasNodeProperty("component")
            && modelNode.nodeProperty("component").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource) {
        designDocument()->changeToSubComponent(modelNode.nodeProperty("component").modelNode());
    }
}

inline static void openInlineComponent(const ModelNode &modelNode)
{
    if (!modelNode.metaInfo().isValid())
        return;

    handleComponent(modelNode);
    handleDelegate(modelNode);
    handleTabComponent(modelNode);
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
    if (modelNode.isValid() && modelNode.metaInfo().isQtQuickLoader()) {
        if (modelNode.hasNodeProperty("sourceComponent"))
            return true;
        if (modelNode.hasNodeListProperty("component"))
            return true;
    }

    return false;
}

static bool hasSourceWithFileComponent(const ModelNode &modelNode)
{
    if (modelNode.isValid() && modelNode.metaInfo().isQtQuickLoader()
        && modelNode.hasVariantProperty("source"))
        return true;

    return false;
}

void DocumentManager::setCurrentDesignDocument(Core::IEditor *editor)
{
    if (editor) {
        auto found = m_designDocuments.find(editor);
        if (found == m_designDocuments.end()) {
            auto &inserted = m_designDocuments[editor] = std::make_unique<DesignDocument>(
                m_projectManager.projectStorageDependencies(), m_externalDependencies);
            m_currentDesignDocument = inserted.get();
            m_currentDesignDocument->setEditor(editor);
        } else {
            m_currentDesignDocument = found->second.get();
        }
    } else if (m_currentDesignDocument) {
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
    return !m_currentDesignDocument.isNull();
}

void DocumentManager::removeEditors(const QList<Core::IEditor *> &editors)
{
    for (Core::IEditor *editor : editors)
        m_designDocuments.erase(editor);
}

void DocumentManager::resetPossibleImports()
{
    for (const auto &[key, value] : m_designDocuments) {
        if (RewriterView *view = value->rewriterView())
            view->resetPossibleImports();
    }
}

bool DocumentManager::goIntoComponent(const ModelNode &modelNode)
{
    QImage image = QmlDesignerPlugin::instance()->viewManager().takeFormEditorScreenshot();
    const QPoint offset = image.offset();
    image.setOffset(offset - QmlItemNode(modelNode).instancePosition().toPoint());
    if (modelNode.isValid() && modelNode.isComponent() && designDocument()) {
        QmlDesignerPlugin::instance()->viewManager().setComponentNode(modelNode);
        QHash<PropertyName, QVariant> oldProperties = getProperties(modelNode);
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

        ModelNode rootModelNode = designDocument()->rewriterView()->rootModelNode();
        applyProperties(rootModelNode, oldProperties);

        rootModelNode.setAuxiliaryData(AuxiliaryDataType::Temporary, "contextImage", image);

        return true;
    }

    return false;
}

void DocumentManager::goIntoComponent(const QString &fileName)
{
    openFileComponentForFile(fileName);
}

bool DocumentManager::createFile(const QString &filePath, const QString &contents)
{
    Utils::TextFileFormat textFileFormat;
    textFileFormat.codec = Core::EditorManager::defaultTextCodec();
    QString errorMessage;

    return textFileFormat.writeFile(Utils::FilePath::fromString(filePath), contents, &errorMessage);
}

void DocumentManager::addFileToVersionControl(const QString &directoryPath, const QString &newFilePath)
{
    Core::IVersionControl *versionControl =
        Core::VcsManager::findVersionControlForDirectory(FilePath::fromString(directoryPath));
    if (versionControl && versionControl->supportsOperation(Core::IVersionControl::AddOperation)) {
        const QMessageBox::StandardButton button
            = QMessageBox::question(Core::ICore::dialogParent(),
                                    Core::VcsManager::msgAddToVcsTitle(),
                                    Core::VcsManager::msgPromptToAddToVcs(QStringList(newFilePath),
                                                                          versionControl),
                                    QMessageBox::Yes | QMessageBox::No);
        if (button == QMessageBox::Yes && !versionControl->vcsAdd(FilePath::fromString(newFilePath))) {
            Core::AsynchronousMessageBox::warning(Core::VcsManager::msgAddToVcsFailedTitle(),
                                                   Core::VcsManager::msgToAddToVcsFailed(QStringList(newFilePath), versionControl));
        }
    }
}

Utils::FilePath DocumentManager::currentFilePath()
{
    if (!QmlDesignerPlugin::instance()->currentDesignDocument())
        return {};

    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument()->fileName();
}

Utils::FilePath DocumentManager::currentProjectDirPath()
{
    QTC_ASSERT(QmlDesignerPlugin::instance(), return {});

    if (!QmlDesignerPlugin::instance()->currentDesignDocument())
        return {};

    Utils::FilePath qmlFileName = QmlDesignerPlugin::instance()->currentDesignDocument()->fileName();

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::projectForFile(qmlFileName);
    if (project)
        return project->projectDirectory();

    const QList projects = ProjectExplorer::ProjectManager::projects();
    for (auto p : projects) {
        if (qmlFileName.startsWith(p->projectDirectory().toString()))
            return p->projectDirectory();
    }

    return {};
}

QStringList DocumentManager::isoIconsQmakeVariableValue(const QString &proPath)
{
    ProjectExplorer::Node *node = ProjectExplorer::ProjectTree::nodeForFile(Utils::FilePath::fromString(proPath));
    if (!node) {
        qCWarning(documentManagerLog) << "No node for .pro:" << proPath;
        return {};
    }

    ProjectExplorer::Node *parentNode = node->parentFolderNode();
    if (!parentNode) {
        qCWarning(documentManagerLog) << "No parent node for node at" << proPath;
        return {};
    }

    auto proNode = dynamic_cast<QmakeProjectManager::QmakeProFileNode*>(parentNode);
    if (!proNode) {
        qCWarning(documentManagerLog) << "Parent node for node at" << proPath << "could not be converted to a QmakeProFileNode";
        return {};
    }

    return proNode->variableValue(QmakeProjectManager::Variable::IsoIcons);
}

bool DocumentManager::setIsoIconsQmakeVariableValue(const QString &proPath, const QStringList &value)
{
    ProjectExplorer::Node *node = ProjectExplorer::ProjectTree::nodeForFile(Utils::FilePath::fromString(proPath));
    if (!node) {
        qCWarning(documentManagerLog) << "No node for .pro:" << proPath;
        return false;
    }

    ProjectExplorer::Node *parentNode = node->parentFolderNode();
    if (!parentNode) {
        qCWarning(documentManagerLog) << "No parent node for node at" << proPath;
        return false;
    }

    auto proNode = dynamic_cast<QmakeProjectManager::QmakeProFileNode*>(parentNode);
    if (!proNode) {
        qCWarning(documentManagerLog) << "Node for" << proPath << "could not be converted to a QmakeProFileNode";
        return false;
    }

    int flags = QmakeProjectManager::Internal::ProWriter::ReplaceValues | QmakeProjectManager::Internal::ProWriter::MultiLine;
    QmakeProjectManager::QmakeProFile *pro = proNode->proFile();
    if (pro)
        return pro->setProVariable("ISO_ICONS", value, QString(), flags);
    return false;
}

void DocumentManager::findPathToIsoProFile(bool *iconResourceFileAlreadyExists, QString *resourceFilePath,
    QString *resourceFileProPath, const QString &isoIconsQrcFile)
{
    Utils::FilePath qmlFileName = QmlDesignerPlugin::instance()->currentDesignDocument()->fileName();
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::projectForFile(qmlFileName);
    ProjectExplorer::Node *node = ProjectExplorer::ProjectTree::nodeForFile(qmlFileName)->parentFolderNode();
    ProjectExplorer::Node *iconQrcFileNode = nullptr;

    while (node && !iconQrcFileNode) {
        qCDebug(documentManagerLog) << "Checking" << node->displayName() << "(" << node << ")";

        if (node->isVirtualFolderType() && node->displayName() == "Resources") {
            ProjectExplorer::FolderNode *virtualFolderNode = node->asFolderNode();
            if (QTC_GUARD(virtualFolderNode)) {
                QList<FolderNode *> folderNodes;
                virtualFolderNode->forEachFolderNode([&](FolderNode *fn) { folderNodes.append(fn); });
                for (int subFolderIndex = 0; subFolderIndex < folderNodes.size() && !iconQrcFileNode; ++subFolderIndex) {
                    ProjectExplorer::FolderNode *subFolderNode = folderNodes.at(subFolderIndex);

                    qCDebug(documentManagerLog) << "Checking if" << subFolderNode->displayName() << "("
                        << subFolderNode << ") is" << isoIconsQrcFile;

                    if (subFolderNode->isFolderNodeType() && subFolderNode->displayName() == isoIconsQrcFile) {
                        qCDebug(documentManagerLog) << "Found" << isoIconsQrcFile << "in" << virtualFolderNode->filePath();

                        iconQrcFileNode = subFolderNode;
                        *resourceFileProPath = iconQrcFileNode->parentProjectNode()->filePath().toString();
                    }
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
        if (project)
            *resourceFilePath = project->projectDirectory().toString() + "/" + isoIconsQrcFile;

        // We assume that the .pro containing the QML file is an acceptable place to add the .qrc file.
        ProjectExplorer::ProjectNode *projectNode = ProjectExplorer::ProjectTree::nodeForFile(qmlFileName)->parentProjectNode();
        *resourceFileProPath = projectNode->filePath().toString();
    } else {
        // We found the QRC file that we want.
        QString projectDirectory = ProjectExplorer::ProjectTree::projectForNode(iconQrcFileNode)->projectDirectory().toString();
        *resourceFilePath = projectDirectory + "/" + isoIconsQrcFile;
    }

    *iconResourceFileAlreadyExists = iconQrcFileNode != nullptr;
}

bool DocumentManager::isoProFileSupportsAddingExistingFiles(const QString &resourceFileProPath)
{
    ProjectExplorer::Node *node = ProjectExplorer::ProjectTree::nodeForFile(Utils::FilePath::fromString(resourceFileProPath));
    if (!node || !node->parentFolderNode())
        return false;
    ProjectExplorer::ProjectNode *projectNode = node->parentFolderNode()->asProjectNode();
    if (!projectNode)
        return false;
    if (!projectNode->supportsAction(ProjectExplorer::AddExistingFile, projectNode)) {
        qCWarning(documentManagerLog) << "Project" << projectNode->displayName() << "does not support adding existing files";
        return false;
    }

    return true;
}

bool DocumentManager::addResourceFileToIsoProject(const QString &resourceFileProPath, const QString &resourceFilePath)
{
    ProjectExplorer::Node *node = ProjectExplorer::ProjectTree::nodeForFile(Utils::FilePath::fromString(resourceFileProPath));
    if (!node || !node->parentFolderNode())
        return false;
    ProjectExplorer::ProjectNode *projectNode = node->parentFolderNode()->asProjectNode();
    if (!projectNode)
        return false;

    if (!projectNode->addFiles({Utils::FilePath::fromString(resourceFilePath)})) {
        qCWarning(documentManagerLog) << "Failed to add resource file to" << projectNode->displayName();
        return false;
    }
    return true;
}

bool DocumentManager::belongsToQmakeProject()
{
    QTC_ASSERT(QmlDesignerPlugin::instance(), return false);

    if (!QmlDesignerPlugin::instance()->currentDesignDocument())
        return false;

    Utils::FilePath qmlFileName = QmlDesignerPlugin::instance()->currentDesignDocument()->fileName();
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::projectForFile(qmlFileName);
    if (!project)
        return false;

    ProjectExplorer::Node *rootNode = project->rootProjectNode();
    auto proNode = dynamic_cast<QmakeProjectManager::QmakeProFileNode*>(rootNode);
    return proNode;
}

Utils::FilePath DocumentManager::currentResourcePath()
{
    Utils::FilePath resourcePath = currentProjectDirPath();

    if (resourcePath.isEmpty())
        return currentFilePath().absolutePath();

    FilePath contentFilePath = resourcePath.pathAppended("content");
    if (contentFilePath.exists())
        return contentFilePath;

    return resourcePath;
}

} // namespace QmlDesigner
