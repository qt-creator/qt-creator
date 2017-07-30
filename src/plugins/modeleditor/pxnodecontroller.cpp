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
    PxNodeUtilities *pxnodeUtilities = nullptr;
    ComponentViewController *componentViewController = nullptr;
    ClassViewController *classViewController = nullptr;
    qmt::DiagramSceneController *diagramSceneController = nullptr;
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

ComponentViewController *PxNodeController::componentViewController() const
{
    return d->componentViewController;
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
    QMT_ASSERT(node, return);
    QMT_ASSERT(diagram, return);

    QString elementName = qmt::NameController::convertFileNameToElementName(
                node->filePath().toString());

    switch (node->nodeType()) {
    case ProjectExplorer::NodeType::File:
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
    case ProjectExplorer::NodeType::Folder:
    case ProjectExplorer::NodeType::VirtualFolder:
    case ProjectExplorer::NodeType::Project:
    {
        QString stereotype;
        switch (node->nodeType()) {
        case ProjectExplorer::NodeType::VirtualFolder:
            stereotype = "virtual folder";
            break;
        case ProjectExplorer::NodeType::Project:
            stereotype = "project";
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
    }
}

bool PxNodeController::hasDiagramForExplorerNode(const ProjectExplorer::Node *node)
{
    return findDiagramForExplorerNode(node) != nullptr;
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
            QMT_ASSERT(relativeIndex >= relativeElements.size(), return nullptr);
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
    return nullptr;
}

void PxNodeController::onMenuActionTriggered(PxNodeController::MenuAction *action,
                                             const ProjectExplorer::Node *node,
                                             qmt::DElement *topMostElementAtPos,
                                             const QPointF &pos, qmt::MDiagram *diagram)
{
    qmt::MObject *newObject = nullptr;
    qmt::MDiagram *newDiagramInObject = nullptr;

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
        parseFullClassName(klass, action->className);
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
        if (QTC_GUARD(folderNode)) {
            d->diagramSceneController->modelController()->undoController()->beginMergeSequence(tr("Create Component Model"));
            QStringList relativeElements = qmt::NameController::buildElementsPath(
                        d->pxnodeUtilities->calcRelativePath(folderNode, d->anchorFolder), true);
            if (qmt::MObject *existingObject = d->pxnodeUtilities->findSameObject(relativeElements, package)) {
                delete package;
                package = dynamic_cast<qmt::MPackage *>(existingObject);
                QMT_ASSERT(package, return);
                d->diagramSceneController->addExistingModelElement(package->uid(), pos, diagram);
            } else {
                qmt::MPackage *requestedRootPackage = d->diagramSceneController->findSuitableParentPackage(topMostElementAtPos, diagram);
                qmt::MPackage *bestParentPackage = d->pxnodeUtilities->createBestMatchingPackagePath(requestedRootPackage, relativeElements);
                d->diagramSceneController->dropNewModelElement(package, bestParentPackage, pos, diagram);
            }
            d->componentViewController->createComponentModel(folderNode, diagram, d->anchorFolder);
            d->componentViewController->updateIncludeDependencies(package);
            d->diagramSceneController->modelController()->undoController()->endMergeSequence();
        } else {
            delete package;
        }
        break;
    }
    }

    if (newObject) {
        d->diagramSceneController->modelController()->undoController()->beginMergeSequence(tr("Drop Node"));
        qmt::MObject *parentForDiagram = nullptr;
        QStringList relativeElements = qmt::NameController::buildElementsPath(
                    d->pxnodeUtilities->calcRelativePath(node, d->anchorFolder),
                    dynamic_cast<qmt::MPackage *>(newObject) != nullptr);
        if (qmt::MObject *existingObject = d->pxnodeUtilities->findSameObject(relativeElements, newObject)) {
            delete newObject;
            newObject = nullptr;
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
            QMT_ASSERT(package, return);
            if (d->diagramSceneController->findDiagramBySearchId(package, newDiagramInObject->name()))
                delete newDiagramInObject;
            else
                d->diagramSceneController->modelController()->addObject(package, newDiagramInObject);
        }
        d->diagramSceneController->modelController()->undoController()->endMergeSequence();
    }
}

void PxNodeController::parseFullClassName(qmt::MClass *klass, const QString &fullClassName)
{
    QString umlNamespace;
    QString className;
    QStringList templateParameters;

    if (qmt::NameController::parseClassName(fullClassName, &umlNamespace, &className, &templateParameters)) {
        klass->setName(className);
        klass->setUmlNamespace(umlNamespace);
        klass->setTemplateParameters(templateParameters);
    } else {
        klass->setName(fullClassName);
    }
}

} // namespace Internal
} // namespace ModelEditor
