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
    void setModelController(qmt::ModelController *modelController);
    void updateFilePaths();
    void visitMComponent(qmt::MComponent *component);

private:
    bool haveMatchingStereotypes(const qmt::MObject *object1, const qmt::MObject *object2);
    QStringList findFilePathOfComponent(const qmt::MComponent *component);
    void collectElementPaths(const ProjectExplorer::FolderNode *folderNode, QMultiHash<QString,
                             Node> *filePathsMap);
    qmt::MComponent *findComponentFromFilePath(const QString &filePath);
    bool haveDependency(const qmt::MObject *source, const qmt::MObject *target,
                        bool inverted = false);
    bool haveDependency(const qmt::MObject *source, const QList<qmt::MPackage *> &targets);

private:
    qmt::ModelController *m_modelController = nullptr;
    QMultiHash<QString, Node> m_filePaths;
};

void UpdateIncludeDependenciesVisitor::setModelController(qmt::ModelController *modelController)
{
    m_modelController = modelController;
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
                qmt::MComponent *includeComponent = findComponentFromFilePath(includeFilePath);
                if (includeComponent && includeComponent != component) {
                    // add dependency between components
                    if (!haveDependency(component, includeComponent)) {
                        auto dependency = new qmt::MDependency;
                        dependency->setFlags(qmt::MElement::ReverseEngineered);
                        dependency->setStereotypes(QStringList() << "include");
                        dependency->setDirection(qmt::MDependency::AToB);
                        dependency->setSource(component->uid());
                        dependency->setTarget(includeComponent->uid());
                        m_modelController->addRelation(component, dependency);
                    }

                    // search ancestors sharing a common owner
                    QList<qmt::MPackage *> componentAncestors;
                    auto ancestor = dynamic_cast<qmt::MPackage *>(component->owner());
                    while (ancestor) {
                        componentAncestors.append(ancestor);
                        ancestor = dynamic_cast<qmt::MPackage *>(ancestor->owner());
                    }
                    QList<qmt::MPackage *> includeComponentAncestors;
                    ancestor = dynamic_cast<qmt::MPackage *>(includeComponent->owner());
                    while (ancestor) {
                        includeComponentAncestors.append(ancestor);
                        ancestor = dynamic_cast<qmt::MPackage *>(ancestor->owner());
                    }

                    int componentHighestAncestorIndex = componentAncestors.size() - 1;
                    int includeComponentHighestAncestorIndex = includeComponentAncestors.size() - 1;
                    QMT_ASSERT(componentAncestors.at(componentHighestAncestorIndex) == includeComponentAncestors.at(includeComponentHighestAncestorIndex), return);
                    while (componentHighestAncestorIndex > 0 && includeComponentHighestAncestorIndex > 0) {
                        if (componentAncestors.at(componentHighestAncestorIndex) != includeComponentAncestors.at(includeComponentHighestAncestorIndex))
                            break;
                        --componentHighestAncestorIndex;
                        --includeComponentHighestAncestorIndex;
                    }

                    // add dependency between parent packages with same stereotype
                    int index1 = 0;
                    int includeComponentLowestIndex = 0;
                    while (index1 <= componentHighestAncestorIndex
                           && includeComponentLowestIndex <= includeComponentHighestAncestorIndex) {
                        if (!componentAncestors.at(index1)->stereotypes().isEmpty()) {
                            int index2 = includeComponentLowestIndex;
                            while (index2 <= includeComponentHighestAncestorIndex) {
                                if (haveMatchingStereotypes(componentAncestors.at(index1), includeComponentAncestors.at(index2))) {
                                    if (!haveDependency(componentAncestors.at(index1), includeComponentAncestors.at(index2))) {
                                        auto dependency = new qmt::MDependency;
                                        dependency->setFlags(qmt::MElement::ReverseEngineered);
                                        // TODO set stereotype for testing purpose
                                        dependency->setStereotypes(QStringList() << "same stereotype");
                                        dependency->setDirection(qmt::MDependency::AToB);
                                        dependency->setSource(componentAncestors.at(index1)->uid());
                                        dependency->setTarget(includeComponentAncestors.at(index2)->uid());
                                        m_modelController->addRelation(componentAncestors.at(index1), dependency);
                                    }
                                    includeComponentLowestIndex = index2 + 1;
                                    break;
                                }
                                ++index2;
                            }
                        }
                        ++index1;
                    }

                    // add dependency between topmost packages with common owner
                    if (componentAncestors.at(componentHighestAncestorIndex) != includeComponentAncestors.at(includeComponentHighestAncestorIndex)) {
                        if (!haveDependency(componentAncestors.at(componentHighestAncestorIndex), includeComponentAncestors)) {
                            auto dependency = new qmt::MDependency;
                            dependency->setFlags(qmt::MElement::ReverseEngineered);
                            // TODO set stereotype for testing purpose
                            dependency->setStereotypes(QStringList() << "ancestor");
                            dependency->setDirection(qmt::MDependency::AToB);
                            dependency->setSource(componentAncestors.at(componentHighestAncestorIndex)->uid());
                            dependency->setTarget(includeComponentAncestors.at(includeComponentHighestAncestorIndex)->uid());
                            m_modelController->addRelation(componentAncestors.at(componentHighestAncestorIndex), dependency);
                        }
                    }

                    // add dependency between parent packages
                    if (componentHighestAncestorIndex > 0 && includeComponentHighestAncestorIndex > 0) { // check for valid parents
                        if (componentAncestors.at(0) != includeComponentAncestors.at(0)) {
                            if (!haveDependency(componentAncestors.at(0), includeComponentAncestors)) {
                                auto dependency = new qmt::MDependency;
                                dependency->setFlags(qmt::MElement::ReverseEngineered);
                                // TODO set stereotype for testing purpose
                                dependency->setStereotypes(QStringList() << "parents");
                                dependency->setDirection(qmt::MDependency::AToB);
                                dependency->setSource(componentAncestors.at(0)->uid());
                                dependency->setTarget(includeComponentAncestors.at(0)->uid());
                                m_modelController->addRelation(componentAncestors.at(0), dependency);
                            }
                        }
                    }
                }
            }
        }
    }
    visitMObject(component);
}

bool UpdateIncludeDependenciesVisitor::haveMatchingStereotypes(const qmt::MObject *object1,
                                                               const qmt::MObject *object2)
{
    return !(object1->stereotypes().toSet() & object2->stereotypes().toSet()).isEmpty();
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
    FindComponentFromFilePath visitor;
    visitor.setFilePath(filePath);
    m_modelController->rootPackage()->accept(&visitor);
    return visitor.component();
}

bool UpdateIncludeDependenciesVisitor::haveDependency(const qmt::MObject *source,
                                                      const qmt::MObject *target, bool inverted)
{
    qmt::MDependency::Direction aToB = qmt::MDependency::AToB;
    qmt::MDependency::Direction bToA = qmt::MDependency::BToA;
    if (inverted) {
        aToB = qmt::MDependency::BToA;
        bToA = qmt::MDependency::AToB;
    }
    foreach (const qmt::Handle<qmt::MRelation> &handle, source->relations()) {
        if (auto dependency = dynamic_cast<qmt::MDependency *>(handle.target())) {
            if (dependency->source() == source->uid()
                    && dependency->target() == target->uid()
                    && (dependency->direction() == aToB
                        || dependency->direction() == qmt::MDependency::Bidirectional)) {
                return true;
            }
            if (dependency->source() == target->uid()
                    && dependency->target() == source->uid()
                    && (dependency->direction() == bToA
                        || dependency->direction() == qmt::MDependency::Bidirectional)) {
                return true;
            }
        }
    }
    if (!inverted)
        return haveDependency(target, source, true);
    return false;
}

bool UpdateIncludeDependenciesVisitor::haveDependency(const qmt::MObject *source,
                                                      const QList<qmt::MPackage *> &targets)
{
    foreach (const qmt::MObject *target, targets) {
        if (haveDependency(source, target))
            return true;
    }
    return false;
}

class ComponentViewController::ComponentViewControllerPrivate {
public:
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

void ComponentViewController::setPxNodeUtilties(PxNodeUtilities *pxnodeUtilities)
{
    d->pxnodeUtilities = pxnodeUtilities;
}

void ComponentViewController::setDiagramSceneController(qmt::DiagramSceneController *diagramSceneController)
{
    d->diagramSceneController = diagramSceneController;
}

void ComponentViewController::createComponentModel(const ProjectExplorer::FolderNode *folderNode,
                                                   qmt::MDiagram *diagram,
                                                   const QString anchorFolder)
{
    d->diagramSceneController->modelController()->startResetModel();
    doCreateComponentModel(folderNode, diagram, anchorFolder);
    d->diagramSceneController->modelController()->finishResetModel(true);
}

void ComponentViewController::updateIncludeDependencies(qmt::MPackage *rootPackage)
{
    d->diagramSceneController->modelController()->startResetModel();
    UpdateIncludeDependenciesVisitor visitor;
    visitor.setModelController(d->diagramSceneController->modelController());
    visitor.updateFilePaths();
    rootPackage->accept(&visitor);
    d->diagramSceneController->modelController()->finishResetModel(true);
}

void ComponentViewController::doCreateComponentModel(const ProjectExplorer::FolderNode *folderNode, qmt::MDiagram *diagram, const QString anchorFolder)
{
    foreach (const ProjectExplorer::FileNode *fileNode, folderNode->fileNodes()) {
        QString componentName = qmt::NameController::convertFileNameToElementName(fileNode->filePath().toString());
        qmt::MComponent *component = nullptr;
        bool isSource = false;
        CppTools::ProjectFile::Kind kind = CppTools::ProjectFile::classify(fileNode->filePath().toString());
        switch (kind) {
        case CppTools::ProjectFile::AmbiguousHeader:
        case CppTools::ProjectFile::CHeader:
        case CppTools::ProjectFile::CSource:
        case CppTools::ProjectFile::CXXHeader:
        case CppTools::ProjectFile::CXXSource:
        case CppTools::ProjectFile::ObjCSource:
        case CppTools::ProjectFile::ObjCHeader:
        case CppTools::ProjectFile::ObjCXXSource:
        case CppTools::ProjectFile::ObjCXXHeader:
        case CppTools::ProjectFile::CudaSource:
        case CppTools::ProjectFile::OpenCLSource:
            isSource = true;
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
                        d->pxnodeUtilities->calcRelativePath(fileNode, anchorFolder), false);
            if (d->pxnodeUtilities->findSameObject(relativeElements, component)) {
                delete component;
            } else {
                qmt::MPackage *requestedRootPackage = d->diagramSceneController->findSuitableParentPackage(nullptr, diagram);
                qmt::MPackage *bestParentPackage = d->pxnodeUtilities->createBestMatchingPackagePath(requestedRootPackage, relativeElements);
                d->diagramSceneController->modelController()->addObject(bestParentPackage, component);
            }
        }
    }

    foreach (const ProjectExplorer::FolderNode *subNode, folderNode->folderNodes())
        doCreateComponentModel(subNode, diagram, anchorFolder);
    auto containerNode = dynamic_cast<const ProjectExplorer::ContainerNode *>(folderNode);
    if (containerNode)
        doCreateComponentModel(containerNode->rootProjectNode(), diagram, anchorFolder);
}

} // namespace Internal
} // namespace ModelEditor
