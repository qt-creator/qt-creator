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

#include "documentcontroller.h"

#include "qmt/config/configcontroller.h"
#include "qmt/controller/namecontroller.h"
#include "qmt/controller/undocontroller.h"
#include "qmt/diagram_controller/dcontainer.h"
#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram_controller/dselection.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_ui/diagramsmanager.h"
#include "qmt/diagram_ui/sceneinspector.h"
#include "qmt/model_controller/mcontainer.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model_controller/mselection.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mpackage.h"
#include "qmt/model_ui/sortedtreemodel.h"
#include "qmt/model_ui/treemodel.h"
#include "qmt/project_controller/projectcontroller.h"
#include "qmt/project/project.h"
#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/style/defaultstyleengine.h"
#include "qmt/style/defaultstyle.h"
#include "qmt/style/relationstarterstyle.h"
#include "qmt/style/stylecontroller.h"
#include "qmt/tasks/diagramscenecontroller.h"
#include "qmt/tasks/findrootdiagramvisitor.h"

#include <QFileInfo>

namespace qmt {

DocumentController::DocumentController(QObject *parent) :
    QObject(parent),
    m_projectController(new ProjectController(this)),
    m_undoController(new UndoController(this)),
    m_modelController(new ModelController(this)),
    m_diagramController(new DiagramController(this)),
    m_diagramSceneController(new DiagramSceneController(this)),
    m_styleController(new StyleController(this)),
    m_stereotypeController(new StereotypeController(this)),
    m_configController(new ConfigController(this)),
    m_treeModel(new TreeModel(this)),
    m_sortedTreeModel(new SortedTreeModel(this)),
    m_diagramsManager(new DiagramsManager(this)),
    m_sceneInspector(new SceneInspector(this)),
    m_modelClipboard(new MContainer()),
    m_diagramClipboard(new DContainer())
{
    // project controller
    QObject::connect(m_projectController, SIGNAL(changed()), this, SIGNAL(changed()));

    // model controller
    m_modelController->setUndoController(m_undoController);
    QObject::connect(m_modelController, SIGNAL(modified()), m_projectController, SLOT(setModified()));

    // diagram controller
    m_diagramController->setModelController(m_modelController);
    m_diagramController->setUndoController(m_undoController);
    QObject::connect(m_diagramController, SIGNAL(modified(const MDiagram*)), m_projectController, SLOT(setModified()));

    // diagram scene controller
    m_diagramSceneController->setModelController(m_modelController);
    m_diagramSceneController->setDiagramController(m_diagramController);

    // config controller
    m_configController->setStereotypeController(m_stereotypeController);

    // tree model
    m_treeModel->setModelController(m_modelController);
    m_treeModel->setStereotypeController(m_stereotypeController);
    m_treeModel->setStyleController(m_styleController);

    // sorted tree model
    m_sortedTreeModel->setTreeModel(m_treeModel);

    // diagrams manager
    m_diagramsManager->setModel(m_treeModel);
    m_diagramsManager->setDiagramController(m_diagramController);
    m_diagramsManager->setDiagramSceneController(m_diagramSceneController);
    m_diagramsManager->setStyleController(m_styleController);
    m_diagramsManager->setStereotypeController(m_stereotypeController);

    // scene inspector
    m_sceneInspector->setDiagramsManager(m_diagramsManager);

    // diagram scene controller (2)
    m_diagramSceneController->setSceneInspector(m_sceneInspector);
}

DocumentController::~DocumentController()
{
    // manually delete objects to ensure correct reverse order of creation
    delete m_sceneInspector;
    delete m_diagramsManager;
    delete m_sortedTreeModel;
    delete m_treeModel;
    delete m_configController;
    delete m_stereotypeController;
    delete m_styleController;
    delete m_diagramSceneController;
    delete m_diagramController;
    delete m_modelController;
    delete m_undoController;
    delete m_projectController;
}

bool DocumentController::isModelClipboardEmpty() const
{
    return m_modelClipboard->isEmpty();
}

bool DocumentController::isDiagramClipboardEmpty() const
{
    return m_diagramClipboard->isEmpty();
}

bool DocumentController::hasDiagramSelection(const MDiagram *diagram) const
{
    return m_diagramsManager->getDiagramSceneModel(diagram)->hasSelection();
}

void DocumentController::cutFromModel(const MSelection &selection)
{
    *m_modelClipboard = m_modelController->cutElements(selection);
    emit modelClipboardChanged(isModelClipboardEmpty());
}

void DocumentController::cutFromDiagram(MDiagram *diagram)
{
    *m_diagramClipboard = m_diagramController->cutElements(m_diagramsManager->getDiagramSceneModel(diagram)->getSelectedElements(), diagram);
    emit diagramClipboardChanged(isDiagramClipboardEmpty());
}

void DocumentController::copyFromModel(const MSelection &selection)
{
    *m_modelClipboard = m_modelController->copyElements(selection);
    emit modelClipboardChanged(isModelClipboardEmpty());
}

void DocumentController::copyFromDiagram(const qmt::MDiagram *diagram)
{
    *m_diagramClipboard = m_diagramController->copyElements(m_diagramsManager->getDiagramSceneModel(diagram)->getSelectedElements(), diagram);
    emit diagramClipboardChanged(isDiagramClipboardEmpty());
}

void DocumentController::copyDiagram(const MDiagram *diagram)
{
    m_diagramsManager->getDiagramSceneModel(diagram)->copyToClipboard();
}

void DocumentController::pasteIntoModel(MObject *modelObject)
{
    if (modelObject) {
        m_modelController->pasteElements(modelObject, *m_modelClipboard);
    }
}

void DocumentController::pasteIntoDiagram(MDiagram *diagram)
{
    m_diagramController->pasteElements(*m_diagramClipboard, diagram);
}

void DocumentController::deleteFromModel(const MSelection &selection)
{
    m_modelController->deleteElements(selection);
}

void DocumentController::deleteFromDiagram(MDiagram *diagram)
{
    if (m_diagramsManager->getDiagramSceneModel(diagram)->hasSelection()) {
        DSelection dselection = m_diagramsManager->getDiagramSceneModel(diagram)->getSelectedElements();
        m_diagramSceneController->deleteFromDiagram(dselection, diagram);
    }
}

void DocumentController::removeFromDiagram(MDiagram *diagram)
{
    m_diagramController->deleteElements(m_diagramsManager->getDiagramSceneModel(diagram)->getSelectedElements(), diagram);
}

void DocumentController::selectAllOnDiagram(MDiagram *diagram)
{
    m_diagramsManager->getDiagramSceneModel(diagram)->selectAllElements();
}

MPackage *DocumentController::createNewPackage(MPackage *parent)
{
    MPackage *newPackage = new MPackage();
    newPackage->setName(tr("New Package"));
    m_modelController->addObject(parent, newPackage);
    return newPackage;
}

MClass *DocumentController::createNewClass(MPackage *parent)
{
    MClass *newClass = new MClass();
    newClass->setName(tr("New Class"));
    m_modelController->addObject(parent, newClass);
    return newClass;
}

MComponent *DocumentController::createNewComponent(MPackage *parent)
{
    MComponent *newComponent = new MComponent();
    newComponent->setName(tr("New Component"));
    m_modelController->addObject(parent, newComponent);
    return newComponent;
}

MCanvasDiagram *DocumentController::createNewCanvasDiagram(MPackage *parent)
{
    MCanvasDiagram *newDiagram = new MCanvasDiagram();
    if (!m_diagramSceneController->findDiagramBySearchId(parent, parent->getName())) {
        newDiagram->setName(parent->getName());
    } else {
        newDiagram->setName(tr("New Diagram"));
    }
    m_modelController->addObject(parent, newDiagram);
    return newDiagram;
}

MDiagram *DocumentController::findRootDiagram()
{
    FindRootDiagramVisitor visitor;
    m_modelController->getRootPackage()->accept(&visitor);
    MDiagram *rootDiagram = visitor.getDiagram();
    return rootDiagram;
}

MDiagram *DocumentController::findOrCreateRootDiagram()
{
    MDiagram *rootDiagram = findRootDiagram();
    if (!rootDiagram) {
        rootDiagram = createNewCanvasDiagram(m_modelController->getRootPackage());
        m_modelController->startUpdateObject(rootDiagram);
        if (m_projectController->getProject()->hasFileName()) {
           rootDiagram->setName(NameController::convertFileNameToElementName(m_projectController->getProject()->getFileName()));
        }
        m_modelController->finishUpdateObject(rootDiagram, false);
    }
    return rootDiagram;
}

void DocumentController::createNewProject(const QString &fileName)
{
    m_diagramsManager->removeAllDiagrams();
    m_treeModel->setModelController(0);
    m_modelController->setRootPackage(0);
    m_undoController->reset();

    m_projectController->newProject(fileName);

    m_treeModel->setModelController(m_modelController);
    m_modelController->setRootPackage(m_projectController->getProject()->getRootPackage());
}

void DocumentController::loadProject(const QString &fileName)
{
    m_diagramsManager->removeAllDiagrams();
    m_treeModel->setModelController(0);
    m_modelController->setRootPackage(0);
    m_undoController->reset();

    m_projectController->newProject(fileName);
    m_projectController->load();

    m_treeModel->setModelController(m_modelController);
    m_modelController->setRootPackage(m_projectController->getProject()->getRootPackage());
}

}
