/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

// TODO this class is experimental and not finished. Code needs fixes and to be cleaned up!

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
    qmt::MComponent *m_bestComponent = 0;
};

void FindComponentFromFilePath::setFilePath(const QString &filePath)
{
    m_elementName = qmt::NameController::convertFileNameToElementName(filePath);
    QFileInfo fileInfo(filePath);
    m_elementsPath = qmt::NameController::buildElementsPath(fileInfo.path(), false);
}

void FindComponentFromFilePath::visitMComponent(qmt::MComponent *component)
{
    if (component->getName() == m_elementName) {
        QStringList elementPath;
        const qmt::MObject *ancestor = component->getOwner();
        while (ancestor) {
            elementPath.prepend(ancestor->getName());
            ancestor = ancestor->getOwner();
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
    qmt::ModelController *m_modelController = 0;
};

void UpdateIncludeDependenciesVisitor::setModelController(qmt::ModelController *modelController)
{
    m_modelController = modelController;
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
                        dependency->setFlags(qmt::MElement::REVERSE_ENGINEERED);
                        dependency->setStereotypes(QStringList() << QStringLiteral("include"));
                        dependency->setDirection(qmt::MDependency::A_TO_B);
                        dependency->setSource(component->getUid());
                        dependency->setTarget(includeComponent->getUid());
                        m_modelController->addRelation(component, dependency);
                    }

                    // search ancestors sharing a common owner
                    QList<qmt::MPackage *> componentAncestors;
                    auto ancestor = dynamic_cast<qmt::MPackage *>(component->getOwner());
                    while (ancestor) {
                        componentAncestors.append(ancestor);
                        ancestor = dynamic_cast<qmt::MPackage *>(ancestor->getOwner());
                    }
                    QList<qmt::MPackage *> includeComponentAncestors;
                    ancestor = dynamic_cast<qmt::MPackage *>(includeComponent->getOwner());
                    while (ancestor) {
                        includeComponentAncestors.append(ancestor);
                        ancestor = dynamic_cast<qmt::MPackage *>(ancestor->getOwner());
                    }

                    int componentHighestAncestorIndex = componentAncestors.size() - 1;
                    int includeComponentHighestAncestorIndex = includeComponentAncestors.size() - 1;
                    QTC_ASSERT(componentAncestors.at(componentHighestAncestorIndex) == includeComponentAncestors.at(includeComponentHighestAncestorIndex), return);
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
                        if (!componentAncestors.at(index1)->getStereotypes().isEmpty()) {
                            int index2 = includeComponentLowestIndex;
                            while (index2 <= includeComponentHighestAncestorIndex) {
                                if (haveMatchingStereotypes(componentAncestors.at(index1), includeComponentAncestors.at(index2))) {
                                    if (!haveDependency(componentAncestors.at(index1), includeComponentAncestors.at(index2))) {
                                        auto dependency = new qmt::MDependency;
                                        dependency->setFlags(qmt::MElement::REVERSE_ENGINEERED);
                                        dependency->setStereotypes(QStringList() << QStringLiteral("same stereotype"));
                                        dependency->setDirection(qmt::MDependency::A_TO_B);
                                        dependency->setSource(componentAncestors.at(index1)->getUid());
                                        dependency->setTarget(includeComponentAncestors.at(index2)->getUid());
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
                            dependency->setFlags(qmt::MElement::REVERSE_ENGINEERED);
                            dependency->setStereotypes(QStringList() << QStringLiteral("ancestor"));
                            dependency->setDirection(qmt::MDependency::A_TO_B);
                            dependency->setSource(componentAncestors.at(componentHighestAncestorIndex)->getUid());
                            dependency->setTarget(includeComponentAncestors.at(includeComponentHighestAncestorIndex)->getUid());
                            m_modelController->addRelation(componentAncestors.at(componentHighestAncestorIndex), dependency);
                        }
                    }

                    // add dependency between parent packages
                    if (componentHighestAncestorIndex > 0 && includeComponentHighestAncestorIndex > 0) { // check for valid parents
                        if (componentAncestors.at(0) != includeComponentAncestors.at(0)) {
                            if (!haveDependency(componentAncestors.at(0), includeComponentAncestors)) {
                                auto dependency = new qmt::MDependency;
                                dependency->setFlags(qmt::MElement::REVERSE_ENGINEERED);
                                dependency->setStereotypes(QStringList() << QStringLiteral("parents"));
                                dependency->setDirection(qmt::MDependency::A_TO_B);
                                dependency->setSource(componentAncestors.at(0)->getUid());
                                dependency->setTarget(includeComponentAncestors.at(0)->getUid());
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
    return !(object1->getStereotypes().toSet() & object2->getStereotypes().toSet()).isEmpty();
}

QStringList UpdateIncludeDependenciesVisitor::findFilePathOfComponent(const qmt::MComponent *component)
{
    QMultiHash<QString, Node> filePaths;
    foreach (const ProjectExplorer::Project *project, ProjectExplorer::SessionManager::projects()) {
        ProjectExplorer::ProjectNode *projectNode = project->rootProjectNode();
        if (projectNode)
            collectElementPaths(projectNode, &filePaths);
    }
    QStringList elementPath;
    const qmt::MObject *ancestor = component->getOwner();
    while (ancestor) {
        elementPath.prepend(ancestor->getName());
        ancestor = ancestor->getOwner();
    }
    QStringList bestFilePaths;
    int maxPathLength = 1;
    foreach (const Node &node, filePaths.values(component->getName())) {
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
        QFileInfo fileInfo(fileNode->filePath().toString());
        QString nodePath = fileInfo.path();
        QStringList elementsPath = qmt::NameController::buildElementsPath(nodePath, false);
        filePathsMap->insertMulti(elementName, Node(fileNode->filePath().toString(), elementsPath));
    }
    foreach (const ProjectExplorer::FolderNode *subNode, folderNode->subFolderNodes())
        collectElementPaths(subNode, filePathsMap);
}

qmt::MComponent *UpdateIncludeDependenciesVisitor::findComponentFromFilePath(const QString &filePath)
{
    FindComponentFromFilePath visitor;
    visitor.setFilePath(filePath);
    m_modelController->getRootPackage()->accept(&visitor);
    return visitor.component();
}

bool UpdateIncludeDependenciesVisitor::haveDependency(const qmt::MObject *source,
                                                      const qmt::MObject *target, bool inverted)
{
    qmt::MDependency::Direction aToB = qmt::MDependency::A_TO_B;
    qmt::MDependency::Direction bToA = qmt::MDependency::B_TO_A;
    if (inverted) {
        aToB = qmt::MDependency::B_TO_A;
        bToA = qmt::MDependency::A_TO_B;
    }
    foreach (const qmt::Handle<qmt::MRelation> &handle, source->getRelations()) {
        if (auto dependency = dynamic_cast<qmt::MDependency *>(handle.getTarget())) {
            if (dependency->getSource() == source->getUid()
                    && dependency->getTarget() == target->getUid()
                    && (dependency->getDirection() == aToB
                        || dependency->getDirection() == qmt::MDependency::BIDIRECTIONAL)) {
                return true;
            }
            if (dependency->getSource() == target->getUid()
                    && dependency->getTarget() == source->getUid()
                    && (dependency->getDirection() == bToA
                        || dependency->getDirection() == qmt::MDependency::BIDIRECTIONAL)) {
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
    PxNodeUtilities *pxnodeUtilities = 0;
    qmt::DiagramSceneController *diagramSceneController = 0;
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
    foreach (const ProjectExplorer::FileNode *fileNode, folderNode->fileNodes()) {
        QString componentName = qmt::NameController::convertFileNameToElementName(fileNode->filePath().toString());
        qmt::MComponent *component = 0;
        bool isSource = false;
        CppTools::ProjectFile::Kind kind = CppTools::ProjectFile::classify(fileNode->filePath().toString());
        switch (kind) {
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
            isSource = false;
            break;
        }
        if (isSource) {
            component = new qmt::MComponent;
            component->setFlags(qmt::MElement::REVERSE_ENGINEERED);
            component->setName(componentName);
        }
        if (component) {
            QStringList relativeElements = qmt::NameController::buildElementsPath(
                        d->pxnodeUtilities->calcRelativePath(fileNode, anchorFolder), false);
            if (d->pxnodeUtilities->findSameObject(relativeElements, component)) {
                delete component;
            } else {
                qmt::MPackage *requestedRootPackage = d->diagramSceneController->findSuitableParentPackage(0, diagram);
                qmt::MPackage *bestParentPackage = d->pxnodeUtilities->createBestMatchingPackagePath(requestedRootPackage, relativeElements);
                d->diagramSceneController->getModelController()->addObject(bestParentPackage, component);
            }
        }
    }

    foreach (const ProjectExplorer::FolderNode *subNode, folderNode->subFolderNodes())
        createComponentModel(subNode, diagram, anchorFolder);
}

void ComponentViewController::updateIncludeDependencies(qmt::MPackage *rootPackage)
{
    UpdateIncludeDependenciesVisitor visitor;
    visitor.setModelController(d->diagramSceneController->getModelController());
    rootPackage->accept(&visitor);
}

} // namespace Internal
} // namespace ModelEditor
