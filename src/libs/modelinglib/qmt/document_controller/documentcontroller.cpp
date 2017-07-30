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
    connect(m_projectController, &ProjectController::changed, this, &DocumentController::changed);

    // model controller
    m_modelController->setUndoController(m_undoController);
    connect(m_modelController, &ModelController::modified,
            m_projectController, &ProjectController::setModified);

    // diagram controller
    m_diagramController->setModelController(m_modelController);
    m_diagramController->setUndoController(m_undoController);
    connect(m_diagramController, &DiagramController::modified,
            m_projectController, &ProjectController::setModified);

    // diagram scene controller
    m_diagramSceneController->setModelController(m_modelController);
    m_diagramSceneController->setDiagramController(m_diagramController);
    m_diagramSceneController->setStereotypeController(m_stereotypeController);

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
    return m_diagramsManager->diagramSceneModel(diagram)->hasSelection();
}

void DocumentController::cutFromModel(const MSelection &selection)
{
    *m_modelClipboard = m_modelController->cutElements(selection);
    emit modelClipboardChanged(isModelClipboardEmpty());
}

void DocumentController::cutFromDiagram(MDiagram *diagram)
{
    *m_diagramClipboard = m_diagramController->cutElements(m_diagramsManager->diagramSceneModel(diagram)->selectedElements(), diagram);
    emit diagramClipboardChanged(isDiagramClipboardEmpty());
}

void DocumentController::copyFromModel(const MSelection &selection)
{
    *m_modelClipboard = m_modelController->copyElements(selection);
    emit modelClipboardChanged(isModelClipboardEmpty());
}

void DocumentController::copyFromDiagram(const qmt::MDiagram *diagram)
{
    m_diagramsManager->diagramSceneModel(diagram)->copyToClipboard();
    *m_diagramClipboard = m_diagramController->copyElements(m_diagramsManager->diagramSceneModel(diagram)->selectedElements(), diagram);
    emit diagramClipboardChanged(isDiagramClipboardEmpty());
}

void DocumentController::copyDiagram(const MDiagram *diagram)
{
    m_diagramsManager->diagramSceneModel(diagram)->copyToClipboard();
}

void DocumentController::pasteIntoModel(MObject *modelObject)
{
    if (modelObject)
        m_modelController->pasteElements(modelObject, *m_modelClipboard);
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
    if (m_diagramsManager->diagramSceneModel(diagram)->hasSelection()) {
        DSelection dselection = m_diagramsManager->diagramSceneModel(diagram)->selectedElements();
        m_diagramSceneController->deleteFromDiagram(dselection, diagram);
    }
}

void DocumentController::removeFromDiagram(MDiagram *diagram)
{
    m_diagramController->deleteElements(m_diagramsManager->diagramSceneModel(diagram)->selectedElements(), diagram);
}

void DocumentController::selectAllOnDiagram(MDiagram *diagram)
{
    m_diagramsManager->diagramSceneModel(diagram)->selectAllElements();
}

MPackage *DocumentController::createNewPackage(MPackage *parent)
{
    auto newPackage = new MPackage();
    newPackage->setName(tr("New Package"));
    m_modelController->addObject(parent, newPackage);
    return newPackage;
}

MClass *DocumentController::createNewClass(MPackage *parent)
{
    auto newClass = new MClass();
    newClass->setName(tr("New Class"));
    m_modelController->addObject(parent, newClass);
    return newClass;
}

MComponent *DocumentController::createNewComponent(MPackage *parent)
{
    auto newComponent = new MComponent();
    newComponent->setName(tr("New Component"));
    m_modelController->addObject(parent, newComponent);
    return newComponent;
}

MCanvasDiagram *DocumentController::createNewCanvasDiagram(MPackage *parent)
{
    auto newDiagram = new MCanvasDiagram();
    if (!m_diagramSceneController->findDiagramBySearchId(parent, parent->name()))
        newDiagram->setName(parent->name());
    else
        newDiagram->setName(tr("New Diagram"));
    m_modelController->addObject(parent, newDiagram);
    return newDiagram;
}

MDiagram *DocumentController::findRootDiagram()
{
    FindRootDiagramVisitor visitor;
    m_modelController->rootPackage()->accept(&visitor);
    MDiagram *rootDiagram = visitor.diagram();
    return rootDiagram;
}

MDiagram *DocumentController::findOrCreateRootDiagram()
{
    MDiagram *rootDiagram = findRootDiagram();
    if (!rootDiagram) {
        rootDiagram = createNewCanvasDiagram(m_modelController->rootPackage());
        m_modelController->startUpdateObject(rootDiagram);
        if (m_projectController->project()->hasFileName())
           rootDiagram->setName(NameController::convertFileNameToElementName(m_projectController->project()->fileName()));
        m_modelController->finishUpdateObject(rootDiagram, false);
    }
    return rootDiagram;
}

void DocumentController::createNewProject(const QString &fileName)
{
    m_diagramsManager->removeAllDiagrams();
    m_treeModel->setModelController(nullptr);
    m_modelController->setRootPackage(nullptr);
    m_undoController->reset();

    m_projectController->newProject(fileName);

    m_treeModel->setModelController(m_modelController);
    m_modelController->setRootPackage(m_projectController->project()->rootPackage());
}

void DocumentController::loadProject(const QString &fileName)
{
    m_diagramsManager->removeAllDiagrams();
    m_treeModel->setModelController(nullptr);
    m_modelController->setRootPackage(nullptr);
    m_undoController->reset();

    m_projectController->newProject(fileName);
    m_projectController->load();

    m_treeModel->setModelController(m_modelController);
    m_modelController->setRootPackage(m_projectController->project()->rootPackage());
}

} // namespace qmt
