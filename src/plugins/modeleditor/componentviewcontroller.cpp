/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include <cpptools/cppmodelmanager.h>
#include <cplusplus/CppDocument.h>

#include <projectexplorer/session.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/project.h>

#include <utils/qtcassert.h>

#include <QFileInfo>

// TODO implement removing include dependencies that are not longer used
// TODO refactor add/remove relations between ancestor packages into extra controller class

namespace ModelEditor {
namespace Internal {

class FindComponentFromFilePath :
        public qmt::MChildrenVisitor
{
public:
    void setFilePath(const QString &filePath);
    qmt::MComponent *component() const { return m_bestComponent; }
    void visitMComponent(qmt::MComponent *component);

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
    void visitMComponent(qmt::MComponent *component);

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
    for (const ProjectExplorer::Project *project : ProjectExplorer::SessionManager::projects()) {
        ProjectExplorer::ProjectNode *projectNode = project->rootProjectNode();
        if (projectNode)
            collectElementPaths(projectNode, &m_filePaths);
    }
}

void UpdateIncludeDependenciesVisitor::visitMComponent(qmt::MComponent *component)
{
    CppTools::CppModelManager *cppModelManager = CppTools::CppModelManager::instance();
    CPlusPlus::Snapshot snapshot = cppModelManager->snapshot();

    QStringList filePaths = findFilePathOfComponent(component);
    foreach (const QString &filePath, filePaths) {
        CPlusPlus::Document::Ptr document = snapshot.document(filePath);
        if (document) {
            foreach (const CPlusPlus::Document::Include &include, document->resolvedIncludes()) {
                QString includeFilePath = include.resolvedFileName();
                // replace proxy header with real one
                CPlusPlus::Document::Ptr includeDocument = snapshot.document(includeFilePath);
                if (includeDocument) {
                    QList<CPlusPlus::Document::Include> includes = includeDocument->resolvedIncludes();
                    if (includes.count() == 1 &&
                            QFileInfo(includes.at(0).resolvedFileName()).fileName() == QFileInfo(includeFilePath).fileName())
                    {
                        includeFilePath = includes.at(0).resolvedFileName();
                    }
                }
                qmt::MComponent *includeComponent = findComponentFromFilePath(includeFilePath);
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
    foreach (const Node &node, m_filePaths.values(component->name())) {
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
    foreach (const ProjectExplorer::FileNode *fileNode, folderNode->fileNodes()) {
        QString elementName = qmt::NameController::convertFileNameToElementName(fileNode->filePath().toString());
        QFileInfo fileInfo = fileNode->filePath().toFileInfo();
        QString nodePath = fileInfo.path();
        QStringList elementsPath = qmt::NameController::buildElementsPath(nodePath, false);
        filePathsMap->insertMulti(elementName, Node(fileNode->filePath().toString(), elementsPath));
    }
    foreach (const ProjectExplorer::FolderNode *subNode, folderNode->folderNodes())
        collectElementPaths(subNode, filePathsMap);
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
        CppTools::ProjectFile::Kind kind = CppTools::ProjectFile::classify(file);
        switch (kind) {
        case CppTools::ProjectFile::CSource:
        case CppTools::ProjectFile::CXXSource:
        case CppTools::ProjectFile::ObjCSource:
        case CppTools::ProjectFile::ObjCXXSource:
        case CppTools::ProjectFile::CudaSource:
        case CppTools::ProjectFile::OpenCLSource:
            isSource = !scanHeaders;
            break;
        case CppTools::ProjectFile::AmbiguousHeader:
        case CppTools::ProjectFile::CHeader:
        case CppTools::ProjectFile::CXXHeader:
        case CppTools::ProjectFile::ObjCHeader:
        case CppTools::ProjectFile::ObjCXXHeader:
            isSource = scanHeaders && !d->pxnodeUtilities->isProxyHeader(file);
            break;
        case CppTools::ProjectFile::Unclassified:
        case CppTools::ProjectFile::Unsupported:
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
