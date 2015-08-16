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
    _project_controller(new ProjectController(this)),
    _undo_controller(new UndoController(this)),
    _model_controller(new ModelController(this)),
    _diagram_controller(new DiagramController(this)),
    _diagram_scene_controller(new DiagramSceneController(this)),
    _style_controller(new StyleController(this)),
    _stereotype_controller(new StereotypeController(this)),
    _config_controller(new ConfigController(this)),
    _tree_model(new TreeModel(this)),
    _sorted_tree_model(new SortedTreeModel(this)),
    _diagrams_manager(new DiagramsManager(this)),
    _scene_inspector(new SceneInspector(this)),
    _model_clipboard(new MContainer()),
    _diagram_clipboard(new DContainer())
{
    // project controller
    QObject::connect(_project_controller, SIGNAL(changed()), this, SIGNAL(changed()));

    // model controller
    _model_controller->setUndoController(_undo_controller);
    QObject::connect(_model_controller, SIGNAL(modified()), _project_controller, SLOT(setModified()));

    // diagram controller
    _diagram_controller->setModelController(_model_controller);
    _diagram_controller->setUndoController(_undo_controller);
    QObject::connect(_diagram_controller, SIGNAL(modified(const MDiagram*)), _project_controller, SLOT(setModified()));

    // diagram scene controller
    _diagram_scene_controller->setModelController(_model_controller);
    _diagram_scene_controller->setDiagramController(_diagram_controller);

    // config controller
    _config_controller->setStereotypeController(_stereotype_controller);

    // tree model
    _tree_model->setModelController(_model_controller);
    _tree_model->setStereotypeController(_stereotype_controller);
    _tree_model->setStyleController(_style_controller);

    // sorted tree model
    _sorted_tree_model->setTreeModel(_tree_model);

    // diagrams manager
    _diagrams_manager->setModel(_tree_model);
    _diagrams_manager->setDiagramController(_diagram_controller);
    _diagrams_manager->setDiagramSceneController(_diagram_scene_controller);
    _diagrams_manager->setStyleController(_style_controller);
    _diagrams_manager->setStereotypeController(_stereotype_controller);

    // scene inspector
    _scene_inspector->setDiagramsManager(_diagrams_manager);

    // diagram scene controller (2)
    _diagram_scene_controller->setSceneInspector(_scene_inspector);
}

DocumentController::~DocumentController()
{
    // manually delete objects to ensure correct reverse order of creation
    delete _scene_inspector;
    delete _diagrams_manager;
    delete _sorted_tree_model;
    delete _tree_model;
    delete _config_controller;
    delete _stereotype_controller;
    delete _style_controller;
    delete _diagram_scene_controller;
    delete _diagram_controller;
    delete _model_controller;
    delete _undo_controller;
    delete _project_controller;
}

bool DocumentController::isModelClipboardEmpty() const
{
    return _model_clipboard->isEmpty();
}

bool DocumentController::isDiagramClipboardEmpty() const
{
    return _diagram_clipboard->isEmpty();
}

bool DocumentController::hasCurrentDiagramSelection() const
{
    QMT_CHECK(_diagrams_manager->getCurrentDiagram());
    return _diagrams_manager->getDiagramSceneModel(_diagrams_manager->getCurrentDiagram())->hasSelection();
}

void DocumentController::cutFromModel(const MSelection &selection)
{
    *_model_clipboard = _model_controller->cutElements(selection);
    emit modelClipboardChanged(isModelClipboardEmpty());
}

void DocumentController::cutFromCurrentDiagram()
{
    MDiagram *diagram = _diagrams_manager->getCurrentDiagram();
    QMT_CHECK(diagram);
    *_diagram_clipboard = _diagram_controller->cutElements(_diagrams_manager->getDiagramSceneModel(diagram)->getSelectedElements(), diagram);
    emit diagramClipboardChanged(isDiagramClipboardEmpty());
}

void DocumentController::copyFromModel(const MSelection &selection)
{
    *_model_clipboard = _model_controller->copyElements(selection);
    emit modelClipboardChanged(isModelClipboardEmpty());
}

void DocumentController::copyFromCurrentDiagram()
{
    MDiagram *diagram = _diagrams_manager->getCurrentDiagram();
    QMT_CHECK(diagram);
    *_diagram_clipboard = _diagram_controller->copyElements(_diagrams_manager->getDiagramSceneModel(diagram)->getSelectedElements(), diagram);
    emit diagramClipboardChanged(isDiagramClipboardEmpty());
}

void DocumentController::copyCurrentDiagram()
{
    QMT_CHECK(_diagrams_manager->getCurrentDiagram());
    _diagrams_manager->getDiagramSceneModel(_diagrams_manager->getCurrentDiagram())->copyToClipboard();
}

void DocumentController::pasteIntoModel(MObject *model_object)
{
    if (model_object) {
        _model_controller->pasteElements(model_object, *_model_clipboard);
    }
}

void DocumentController::pasteIntoCurrentDiagram()
{
    QMT_CHECK(_diagrams_manager->getCurrentDiagram());
    _diagram_controller->pasteElements(*_diagram_clipboard, _diagrams_manager->getCurrentDiagram());
}

void DocumentController::deleteFromModel(const MSelection &selection)
{
    _model_controller->deleteElements(selection);
}

void DocumentController::deleteFromCurrentDiagram()
{
    MDiagram *current_diagram = _diagrams_manager->getCurrentDiagram();
    QMT_CHECK(current_diagram);
    if (_diagrams_manager->getDiagramSceneModel(current_diagram)->hasSelection()) {
        DSelection dselection = _diagrams_manager->getDiagramSceneModel(current_diagram)->getSelectedElements();
        _diagram_scene_controller->deleteFromDiagram(dselection, current_diagram);
    }
}

void DocumentController::removeFromCurrentDiagram()
{
    MDiagram *diagram = _diagrams_manager->getCurrentDiagram();
    QMT_CHECK(diagram);
    _diagram_controller->deleteElements(_diagrams_manager->getDiagramSceneModel(diagram)->getSelectedElements(), diagram);
}

void DocumentController::selectAllOnCurrentDiagram()
{
    QMT_CHECK(_diagrams_manager->getCurrentDiagram());
    _diagrams_manager->getDiagramSceneModel(_diagrams_manager->getCurrentDiagram())->selectAllElements();
}

MPackage *DocumentController::createNewPackage(MPackage *parent)
{
    MPackage *new_package = new MPackage();
    new_package->setName(tr("New Package"));
    _model_controller->addObject(parent, new_package);
    return new_package;
}

MClass *DocumentController::createNewClass(MPackage *parent)
{
    MClass *new_class = new MClass();
    new_class->setName(tr("New Class"));
    _model_controller->addObject(parent, new_class);
    return new_class;
}

MComponent *DocumentController::createNewComponent(MPackage *parent)
{
    MComponent *new_component = new MComponent();
    new_component->setName(tr("New Component"));
    _model_controller->addObject(parent, new_component);
    return new_component;
}

MCanvasDiagram *DocumentController::createNewCanvasDiagram(MPackage *parent)
{
    MCanvasDiagram *new_diagram = new MCanvasDiagram();
    if (!_diagram_scene_controller->findDiagramBySearchId(parent, parent->getName())) {
        new_diagram->setName(parent->getName());
    } else {
        new_diagram->setName(tr("New Diagram"));
    }
    _model_controller->addObject(parent, new_diagram);
    return new_diagram;
}

MDiagram *DocumentController::findRootDiagram()
{
    FindRootDiagramVisitor visitor;
    _model_controller->getRootPackage()->accept(&visitor);
    MDiagram *root_diagram = visitor.getDiagram();
    return root_diagram;
}

MDiagram *DocumentController::findOrCreateRootDiagram()
{
    MDiagram *root_diagram = findRootDiagram();
    if (!root_diagram) {
        root_diagram = createNewCanvasDiagram(_model_controller->getRootPackage());
        _model_controller->startUpdateObject(root_diagram);
        if (_project_controller->getProject()->hasFileName()) {
           root_diagram->setName(NameController::convertFileNameToElementName(_project_controller->getProject()->getFileName()));
        }
        _model_controller->finishUpdateObject(root_diagram, false);
    }
    return root_diagram;
}

void DocumentController::createNewProject(const QString &file_name)
{
    _diagrams_manager->removeAllDiagrams();
    _tree_model->setModelController(0);
    _model_controller->setRootPackage(0);
    _undo_controller->reset();

    _project_controller->newProject(file_name);

    _tree_model->setModelController(_model_controller);
    _model_controller->setRootPackage(_project_controller->getProject()->getRootPackage());
}

void DocumentController::loadProject(const QString &file_name)
{
    _diagrams_manager->removeAllDiagrams();
    _tree_model->setModelController(0);
    _model_controller->setRootPackage(0);
    _undo_controller->reset();

    _project_controller->newProject(file_name);
    _project_controller->load();

    _tree_model->setModelController(_model_controller);
    _model_controller->setRootPackage(_project_controller->getProject()->getRootPackage());
}

}
