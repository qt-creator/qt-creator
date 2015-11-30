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

#include "pxnodecontroller.h"

#include "pxnodeutilities.h"
#include "componentviewcontroller.h"
#include "classviewcontroller.h"

#include "qmt/model/mpackage.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/controller/namecontroller.h"
#include "qmt/controller/undocontroller.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <projectexplorer/projectnodes.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QMenu>
#include <QQueue>

namespace ModelEditor {
namespace Internal {

class PxNodeController::MenuAction :
        public QAction
{
public:
    enum Type {
        TYPE_ADD_COMPONENT,
        TYPE_ADD_CLASS,
        TYPE_ADD_PACKAGE,
        TYPE_ADD_PACKAGE_AND_DIAGRAM,
        TYPE_ADD_PACKAGE_MODEL,
        TYPE_ADD_COMPONENT_MODEL,
        TYPE_ADD_CLASS_MODEL
    };

public:
    MenuAction(const QString &text, const QString &elementName, Type type, int index,
               QObject *parent)
        : QAction(text, parent),
          elementName(elementName),
          type(type),
          index(index)
    {
    }

    MenuAction(const QString &text, const QString &elementName, Type type, QObject *parent)
        : QAction(text, parent),
          elementName(elementName),
          type(type),
          index(-1)
    {
    }

    QString elementName;
    int type;
    int index;
    QString className;
    QString packageStereotype;
};

class PxNodeController::PxNodeControllerPrivate
{
public:
    PxNodeUtilities *pxnodeUtilities = 0;
    ComponentViewController *componentViewController = 0;
    ClassViewController *classViewController = 0;
    qmt::DiagramSceneController *diagramSceneController = 0;
    QString anchorFolder;
};

PxNodeController::PxNodeController(QObject *parent)
    : QObject(parent),
      d(new PxNodeControllerPrivate)
{
    d->pxnodeUtilities = new PxNodeUtilities(this);
    d->componentViewController = new ComponentViewController(this);
    d->componentViewController->setPxNodeUtilties(d->pxnodeUtilities);
    d->classViewController = new ClassViewController(this);
}

PxNodeController::~PxNodeController()
{
    delete d;
}

void PxNodeController::setDiagramSceneController(
        qmt::DiagramSceneController *diagramSceneController)
{
    d->diagramSceneController = diagramSceneController;
    d->pxnodeUtilities->setDiagramSceneController(diagramSceneController);
    d->componentViewController->setDiagramSceneController(diagramSceneController);
}

void PxNodeController::setAnchorFolder(const QString &anchorFolder)
{
    d->anchorFolder = anchorFolder;
}

void PxNodeController::addExplorerNode(const ProjectExplorer::Node *node,
                                       qmt::DElement *topMostElementAtPos, const QPointF &pos,
                                       qmt::MDiagram *diagram)
{
    QTC_ASSERT(node, return);
    QTC_ASSERT(diagram, return);

    QString elementName = qmt::NameController::convertFileNameToElementName(
                node->filePath().toString());

    switch (node->nodeType()) {
    case ProjectExplorer::FileNodeType:
    {
        QStringList classNames = d->classViewController->findClassDeclarations(
                    node->filePath().toString()).toList();
        auto menu = new QMenu;
        menu->addAction(new MenuAction(tr("Add Component %1").arg(elementName), elementName,
                                       MenuAction::TYPE_ADD_COMPONENT, menu));
        menu->addSeparator();
        int index = 0;
        foreach (const QString &className, classNames) {
            auto action = new MenuAction(tr("Add Class %1").arg(className), elementName,
                                         MenuAction::TYPE_ADD_CLASS, index, menu);
            action->className = className;
            menu->addAction(action);
            ++index;
        }
        connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
        connect(menu, &QMenu::triggered, this, [=](QAction *action) {
            // TODO potential risk if node, topMostElementAtPos or diagram is deleted in between
            onMenuActionTriggered(static_cast<MenuAction *>(action), node, topMostElementAtPos,
                                  pos, diagram);
        });
        menu->popup(QCursor::pos());
        break;
    }
    case ProjectExplorer::FolderNodeType:
    case ProjectExplorer::VirtualFolderNodeType:
    case ProjectExplorer::ProjectNodeType:
    {
        QString stereotype;
        switch (node->nodeType()) {
        case ProjectExplorer::VirtualFolderNodeType:
            stereotype = QStringLiteral("virtual folder");
            break;
        case ProjectExplorer::ProjectNodeType:
            stereotype = QStringLiteral("project");
            break;
        default:
            break;
        }
        auto menu = new QMenu;
        auto action = new MenuAction(tr("Add Package %1").arg(elementName), elementName,
                                     MenuAction::TYPE_ADD_PACKAGE, menu);
        action->packageStereotype = stereotype;
        menu->addAction(action);
        action = new MenuAction(tr("Add Package and Diagram %1").arg(elementName), elementName,
                                MenuAction::TYPE_ADD_PACKAGE_AND_DIAGRAM, menu);
        action->packageStereotype = stereotype;
        menu->addAction(action);
        action = new MenuAction(tr("Add Component Model"), elementName,
                                MenuAction::TYPE_ADD_COMPONENT_MODEL, menu);
        action->packageStereotype = stereotype;
        menu->addAction(action);
        connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
        connect(menu, &QMenu::triggered, this, [=](QAction *action) {
            onMenuActionTriggered(static_cast<MenuAction *>(action), node, topMostElementAtPos,
                                  pos, diagram);
        });
        menu->popup(QCursor::pos());
        break;
    }
    case ProjectExplorer::SessionNodeType:
        break;
    }
}

bool PxNodeController::hasDiagramForExplorerNode(const ProjectExplorer::Node *node)
{
    return findDiagramForExplorerNode(node) != 0;
}

qmt::MDiagram *PxNodeController::findDiagramForExplorerNode(const ProjectExplorer::Node *node)
{
    QStringList relativeElements = qmt::NameController::buildElementsPath(
                d->pxnodeUtilities->calcRelativePath(node, d->anchorFolder), false);

    QQueue<qmt::MPackage *> roots;
    roots.append(d->diagramSceneController->modelController()->rootPackage());

    while (!roots.isEmpty()) {
        qmt::MPackage *package = roots.takeFirst();

        // append all sub-packages of the same level as next root packages
        foreach (const qmt::Handle<qmt::MObject> &handle, package->children()) {
            if (handle.hasTarget()) {
                if (auto childPackage = dynamic_cast<qmt::MPackage *>(handle.target()))
                    roots.append(childPackage);
            }
        }

        // goto into sub-packages to find complete chain of names
        int relativeIndex = 0;
        bool found = true;
        while (found && relativeIndex < relativeElements.size()) {
            QString relativeSearchId = qmt::NameController::calcElementNameSearchId(
                        relativeElements.at(relativeIndex));
            found = false;
            foreach (const qmt::Handle<qmt::MObject> &handle, package->children()) {
                if (handle.hasTarget()) {
                    if (auto childPackage = dynamic_cast<qmt::MPackage *>(handle.target())) {
                        if (qmt::NameController::calcElementNameSearchId(childPackage->name()) == relativeSearchId) {
                            package = childPackage;
                            ++relativeIndex;
                            found = true;
                            break;
                        }
                    }
                }
            }
        }

        if (found) {
            QTC_ASSERT(relativeIndex >= relativeElements.size(), return 0);
            // complete package chain found so check for appropriate diagram within deepest package
            qmt::MDiagram *diagram = d->diagramSceneController->findDiagramBySearchId(
                        package, package->name());
            if (diagram)
                return diagram;
            // find first diagram within deepest package
            foreach (const qmt::Handle<qmt::MObject> &handle, package->children()) {
                if (handle.hasTarget()) {
                    if (auto diagram = dynamic_cast<qmt::MDiagram *>(handle.target()))
                        return diagram;
                }
            }
        }
    }

    // complete sub-package structure scanned but did not found the desired object
    return 0;
}

void PxNodeController::onMenuActionTriggered(PxNodeController::MenuAction *action,
                                             const ProjectExplorer::Node *node,
                                             qmt::DElement *topMostElementAtPos,
                                             const QPointF &pos, qmt::MDiagram *diagram)
{
    qmt::MObject *newObject = 0;
    qmt::MDiagram *newDiagramInObject = 0;

    switch (action->type) {
    case MenuAction::TYPE_ADD_COMPONENT:
    {
        auto component = new qmt::MComponent();
        component->setFlags(qmt::MElement::ReverseEngineered);
        component->setName(action->elementName);
        newObject = component;
        break;
    }
    case MenuAction::TYPE_ADD_CLASS:
    {
        // TODO handle template classes
        auto klass = new qmt::MClass();
        klass->setFlags(qmt::MElement::ReverseEngineered);
        QString qualifiedName = action->className;
        int i = qualifiedName.lastIndexOf(QStringLiteral("::"));
        if (i >= 0) {
            klass->setUmlNamespace(qualifiedName.left(i));
            klass->setName(qualifiedName.mid(i + 2));
        } else {
            klass->setName(qualifiedName);
        }
        newObject = klass;
        break;
    }
    case MenuAction::TYPE_ADD_PACKAGE:
    case MenuAction::TYPE_ADD_PACKAGE_AND_DIAGRAM:
    {
            auto package = new qmt::MPackage();
            package->setFlags(qmt::MElement::ReverseEngineered);
            package->setName(action->elementName);
            if (!action->packageStereotype.isEmpty())
                package->setStereotypes(QStringList() << action->packageStereotype);
            newObject = package;
            if (action->type == MenuAction::TYPE_ADD_PACKAGE_AND_DIAGRAM) {
                auto diagram = new qmt::MCanvasDiagram();
                diagram->setName(action->elementName);
                newDiagramInObject = diagram;
            }
        break;
    }
    case MenuAction::TYPE_ADD_COMPONENT_MODEL:
    {
        auto package = new qmt::MPackage();
        package->setFlags(qmt::MElement::ReverseEngineered);
        package->setName(action->elementName);
        if (!action->packageStereotype.isEmpty())
            package->setStereotypes(QStringList() << action->packageStereotype);
        auto folderNode = dynamic_cast<const ProjectExplorer::FolderNode *>(node);
        QTC_CHECK(folderNode);
        if (folderNode) {
            d->diagramSceneController->modelController()->undoController()->beginMergeSequence(tr("Create Component Model"));
            QStringList relativeElements = qmt::NameController::buildElementsPath(
                        d->pxnodeUtilities->calcRelativePath(folderNode, d->anchorFolder), true);
            if (qmt::MObject *existingObject = d->pxnodeUtilities->findSameObject(relativeElements, package)) {
                delete package;
                package = dynamic_cast<qmt::MPackage *>(existingObject);
                QTC_ASSERT(package, return);
                d->diagramSceneController->addExistingModelElement(package->uid(), pos, diagram);
            } else {
                qmt::MPackage *requestedRootPackage = d->diagramSceneController->findSuitableParentPackage(topMostElementAtPos, diagram);
                qmt::MPackage *bestParentPackage = d->pxnodeUtilities->createBestMatchingPackagePath(requestedRootPackage, relativeElements);
                d->diagramSceneController->dropNewModelElement(package, bestParentPackage, pos, diagram);
            }
            d->componentViewController->createComponentModel(folderNode, diagram, d->anchorFolder);
            d->componentViewController->updateIncludeDependencies(package);
            d->diagramSceneController->modelController()->undoController()->endMergeSequence();
        }
        break;
    }
    }

    if (newObject) {
        d->diagramSceneController->modelController()->undoController()->beginMergeSequence(tr("Drop Node"));
        qmt::MObject *parentForDiagram = 0;
        QStringList relativeElements = qmt::NameController::buildElementsPath(
                    d->pxnodeUtilities->calcRelativePath(node, d->anchorFolder),
                    dynamic_cast<qmt::MPackage *>(newObject) != 0);
        if (qmt::MObject *existingObject = d->pxnodeUtilities->findSameObject(relativeElements, newObject)) {
            delete newObject;
            newObject = 0;
            d->diagramSceneController->addExistingModelElement(existingObject->uid(), pos, diagram);
            parentForDiagram = existingObject;
        } else {
            qmt::MPackage *requestedRootPackage = d->diagramSceneController->findSuitableParentPackage(topMostElementAtPos, diagram);
            qmt::MPackage *bestParentPackage = d->pxnodeUtilities->createBestMatchingPackagePath(requestedRootPackage, relativeElements);
            d->diagramSceneController->dropNewModelElement(newObject, bestParentPackage, pos, diagram);
            parentForDiagram = newObject;
        }

        // if requested and not existing then create new diagram in package
        if (newDiagramInObject) {
            auto package = dynamic_cast<qmt::MPackage *>(parentForDiagram);
            QTC_ASSERT(package, return);
            if (d->diagramSceneController->findDiagramBySearchId(package, newDiagramInObject->name()))
                delete newDiagramInObject;
            else
                d->diagramSceneController->modelController()->addObject(package, newDiagramInObject);
        }
        d->diagramSceneController->modelController()->undoController()->endMergeSequence();
    }
}

} // namespace Internal
} // namespace ModelEditor
