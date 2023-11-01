// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "componentviewcontroller.h"

#include "modelutilities.h"
#include "packageviewcontroller.h"
#include "pxnodeutilities.h"

#include "qmt/controller/namecontroller.h"
#include "qmt/model_controller/mchildrenvisitor.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mdependency.h"
#include "qmt/model/mpackage.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <cppeditor/cppmodelmanager.h>
#include <cplusplus/CppDocument.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>

#include <utils/qtcassert.h>

#include <QFileInfo>

// TODO implement removing include dependencies that are not longer used
// TODO refactor add/remove relations between ancestor packages into extra controller class

using namespace ProjectExplorer;
using namespace Utils;

namespace ModelEditor {
namespace Internal {

class FindComponentFromFilePath :
        public qmt::MChildrenVisitor
{
public:
    void setFilePath(const QString &filePath);
    qmt::MComponent *component() const { return m_bestComponent; }
    void visitMComponent(qmt::MComponent *component) final;

private:
    QString m_elementName;
    QStringList m_elementsPath;
    int m_maxPathLength = 0;
    qmt::MComponent *m_bestComponent = nullptr;
};

void FindComponentFromFilePath::setFilePath(const QString &filePath)
{
    m_elementName = qmt::NameController::convertFileNameToElementName(filePath);
    QFileInfo fileInfo(filePath);
    m_elementsPath = qmt::NameController::buildElementsPath(fileInfo.path(), false);
}

void FindComponentFromFilePath::visitMComponent(qmt::MComponent *component)
{
    if (component->name() == m_elementName) {
        QStringList elementPath;
        const qmt::MObject *ancestor = component->owner();
        while (ancestor) {
            elementPath.prepend(ancestor->name());
            ancestor = ancestor->owner();
        }
        int i = elementPath.size() - 1;
        int j = m_elementsPath.size() - 1;
        while (i >= 0 && j >= 0 && elementPath.at(i) == m_elementsPath.at(j)) {
            --i;
            --j;
        }
        int pathLength = elementPath.size() - 1 - i;
        if (pathLength > m_maxPathLength) {
            m_maxPathLength = pathLength;
            m_bestComponent = component;
        }
    }
    visitMObject(component);
}

class UpdateIncludeDependenciesVisitor :
        public qmt::MChildrenVisitor
{
    class Node
    {
    public:
        Node() = default;
        Node(const QString &filePath, const QStringList &elementPath)
            : m_filePath(filePath),
              m_elementPath(elementPath)
        {
        }

        QString m_filePath;
        QStringList m_elementPath;
    };

public:
    void setPackageViewController(PackageViewController *packageViewController);
    void setModelController(qmt::ModelController *modelController);
    void setModelUtilities(ModelUtilities *modelUtilities);
    void updateFilePaths();
    void visitMComponent(qmt::MComponent *component) final;

private:
    QStringList findFilePathOfComponent(const qmt::MComponent *component);
    void collectElementPaths(const ProjectExplorer::FolderNode *folderNode, QMultiHash<QString,
                             Node> *filePathsMap);
    qmt::MComponent *findComponentFromFilePath(const QString &filePath);

private:
    PackageViewController *m_packageViewController = nullptr;
    qmt::ModelController *m_modelController = nullptr;
    ModelUtilities *m_modelUtilities = nullptr;
    QMultiHash<QString, Node> m_filePaths;
    QHash<QString, qmt::MComponent *> m_filePathComponentsMap;
};

void UpdateIncludeDependenciesVisitor::setPackageViewController(PackageViewController *packageViewController)
{
    m_packageViewController = packageViewController;
}

void UpdateIncludeDependenciesVisitor::setModelController(qmt::ModelController *modelController)
{
    m_modelController = modelController;
}

void UpdateIncludeDependenciesVisitor::setModelUtilities(ModelUtilities *modelUtilities)
{
    m_modelUtilities = modelUtilities;
}

void UpdateIncludeDependenciesVisitor::updateFilePaths()
{
    m_filePaths.clear();
    for (const ProjectExplorer::Project *project : ProjectExplorer::ProjectManager::projects()) {
        ProjectExplorer::ProjectNode *projectNode = project->rootProjectNode();
        if (projectNode)
            collectElementPaths(projectNode, &m_filePaths);
    }
}

void UpdateIncludeDependenciesVisitor::visitMComponent(qmt::MComponent *component)
{
    CPlusPlus::Snapshot snapshot = CppEditor::CppModelManager::snapshot();

    const QStringList filePaths = findFilePathOfComponent(component);
    for (const QString &filePath : filePaths) {
        CPlusPlus::Document::Ptr document = snapshot.document(FilePath::fromString(filePath));
        if (document) {
            const QList<CPlusPlus::Document::Include> includes = document->resolvedIncludes();
            for (const CPlusPlus::Document::Include &include : includes) {
                Utils::FilePath includeFilePath = include.resolvedFileName();
                // replace proxy header with real one
                CPlusPlus::Document::Ptr includeDocument = snapshot.document(includeFilePath);
                if (includeDocument) {
                    QList<CPlusPlus::Document::Include> includes = includeDocument->resolvedIncludes();
                    if (includes.count() == 1 &&
                            includes.at(0).resolvedFileName().fileName() == includeFilePath.fileName())
                    {
                        includeFilePath = includes.at(0).resolvedFileName();
                    }
                }
                qmt::MComponent *includeComponent = findComponentFromFilePath(includeFilePath.toString());
                if (includeComponent && includeComponent != component) {
                    // add dependency between components
                    if (!m_modelUtilities->haveDependency(component, includeComponent)) {
                        auto dependency = new qmt::MDependency;
                        dependency->setFlags(qmt::MElement::ReverseEngineered);
                        dependency->setStereotypes({"include"});
                        dependency->setDirection(qmt::MDependency::AToB);
                        dependency->setSource(component->uid());
                        dependency->setTarget(includeComponent->uid());
                        m_modelController->addRelation(component, dependency);
                    }
                    m_packageViewController->createAncestorDependencies(component, includeComponent);
                }
            }
        }
    }
    visitMObject(component);
}

QStringList UpdateIncludeDependenciesVisitor::findFilePathOfComponent(const qmt::MComponent *component)
{
    QStringList elementPath;
    const qmt::MObject *ancestor = component->owner();
    while (ancestor) {
        elementPath.prepend(ancestor->name());
        ancestor = ancestor->owner();
    }
    QStringList bestFilePaths;
    int maxPathLength = 1;
    const QList<Node> nodes = m_filePaths.values(component->name());
    for (const Node &node : nodes) {
        int i = elementPath.size() - 1;
        int j = node.m_elementPath.size() - 1;
        while (i >= 0 && j >= 0 && elementPath.at(i) == node.m_elementPath.at(j)) {
            --i;
            --j;
        }
        int pathLength = elementPath.size() - 1 - i;
        if (pathLength > maxPathLength)
            bestFilePaths.clear();
        if (pathLength >= maxPathLength) {
            maxPathLength = pathLength;
            bestFilePaths.append(node.m_filePath);
        }
    }
    return bestFilePaths;
}

void UpdateIncludeDependenciesVisitor::collectElementPaths(const ProjectExplorer::FolderNode *folderNode,
                                                           QMultiHash<QString, Node> *filePathsMap)
{
    folderNode->forEachFileNode([&](FileNode *fileNode) {
        QString elementName = qmt::NameController::convertFileNameToElementName(fileNode->filePath().toString());
        QFileInfo fileInfo = fileNode->filePath().toFileInfo();
        QString nodePath = fileInfo.path();
        QStringList elementsPath = qmt::NameController::buildElementsPath(nodePath, false);
        filePathsMap->insert(elementName, Node(fileNode->filePath().toString(), elementsPath));
    });
    folderNode->forEachFolderNode([&](FolderNode *subNode) {
        collectElementPaths(subNode, filePathsMap);
    });
}

qmt::MComponent *UpdateIncludeDependenciesVisitor::findComponentFromFilePath(const QString &filePath)
{
    const auto it = m_filePathComponentsMap.constFind(filePath);
    if (it != m_filePathComponentsMap.cend())
        return it.value();

    FindComponentFromFilePath visitor;
    visitor.setFilePath(filePath);
    m_modelController->rootPackage()->accept(&visitor);
    qmt::MComponent *component = visitor.component();
    m_filePathComponentsMap.insert(filePath, component);
    return component;
}

class ComponentViewController::ComponentViewControllerPrivate {
public:
    ModelUtilities *modelUtilities = nullptr;
    PackageViewController *packageViewController = nullptr;
    PxNodeUtilities *pxnodeUtilities = nullptr;
    qmt::DiagramSceneController *diagramSceneController = nullptr;
};

ComponentViewController::ComponentViewController(QObject *parent)
    : QObject(parent),
      d(new ComponentViewControllerPrivate)
{
}

ComponentViewController::~ComponentViewController()
{
    delete d;
}

void ComponentViewController::setModelUtilities(ModelUtilities *modelUtilities)
{
    d->modelUtilities = modelUtilities;
}

void ComponentViewController::setPackageViewController(PackageViewController *packageViewController)
{
    d->packageViewController = packageViewController;
}

void ComponentViewController::setPxNodeUtilties(PxNodeUtilities *pxnodeUtilities)
{
    d->pxnodeUtilities = pxnodeUtilities;
}

void ComponentViewController::setDiagramSceneController(qmt::DiagramSceneController *diagramSceneController)
{
    d->diagramSceneController = diagramSceneController;
}

void ComponentViewController::createComponentModel(const QString &filePath,
                                                   qmt::MDiagram *diagram,
                                                   const QString &anchorFolder)
{
    d->diagramSceneController->modelController()->startResetModel();
    doCreateComponentModel(filePath, diagram, anchorFolder, false);
    doCreateComponentModel(filePath, diagram, anchorFolder, true);
    d->diagramSceneController->modelController()->finishResetModel(true);
}

void ComponentViewController::updateIncludeDependencies(qmt::MPackage *rootPackage)
{
    d->diagramSceneController->modelController()->startResetModel();
    UpdateIncludeDependenciesVisitor visitor;
    visitor.setPackageViewController(d->packageViewController);
    visitor.setModelController(d->diagramSceneController->modelController());
    visitor.setModelUtilities(d->modelUtilities);
    visitor.updateFilePaths();
    rootPackage->accept(&visitor);
    d->diagramSceneController->modelController()->finishResetModel(true);
}

void ComponentViewController::doCreateComponentModel(const QString &filePath, qmt::MDiagram *diagram,
                                                     const QString &anchorFolder, bool scanHeaders)
{
    for (const QString &fileName : QDir(filePath).entryList(QDir::Files)) {
        QString file = filePath + "/" + fileName;
        QString componentName = qmt::NameController::convertFileNameToElementName(file);
        qmt::MComponent *component = nullptr;
        bool isSource = false;
        CppEditor::ProjectFile::Kind kind = CppEditor::ProjectFile::classify(file);
        switch (kind) {
        case CppEditor::ProjectFile::CSource:
        case CppEditor::ProjectFile::CXXSource:
        case CppEditor::ProjectFile::ObjCSource:
        case CppEditor::ProjectFile::ObjCXXSource:
        case CppEditor::ProjectFile::CudaSource:
        case CppEditor::ProjectFile::OpenCLSource:
            isSource = !scanHeaders;
            break;
        case CppEditor::ProjectFile::AmbiguousHeader:
        case CppEditor::ProjectFile::CHeader:
        case CppEditor::ProjectFile::CXXHeader:
        case CppEditor::ProjectFile::ObjCHeader:
        case CppEditor::ProjectFile::ObjCXXHeader:
            isSource = scanHeaders && !d->pxnodeUtilities->isProxyHeader(file);
            break;
        case CppEditor::ProjectFile::Unclassified:
        case CppEditor::ProjectFile::Unsupported:
            isSource = false;
            break;
        }
        if (isSource) {
            component = new qmt::MComponent;
            component->setFlags(qmt::MElement::ReverseEngineered);
            component->setName(componentName);
        }
        if (component) {
            QStringList relativeElements = qmt::NameController::buildElementsPath(
                        d->pxnodeUtilities->calcRelativePath(file, anchorFolder), false);
            if (d->pxnodeUtilities->findSameObject(relativeElements, component)) {
                delete component;
            } else {
                qmt::MPackage *requestedRootPackage = d->diagramSceneController->findSuitableParentPackage(nullptr, diagram);
                qmt::MPackage *bestParentPackage = d->pxnodeUtilities->createBestMatchingPackagePath(requestedRootPackage, relativeElements);
                d->diagramSceneController->modelController()->addObject(bestParentPackage, component);
            }
        }
    }
    for (const QString &fileName : QDir(filePath).entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
        QString file = filePath + "/" + fileName;
        doCreateComponentModel(file, diagram, anchorFolder, scanHeaders);
    }
}

} // namespace Internal
} // namespace ModelEditor
