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

#include "diagramcontroller.h"

#include "dselection.h"
#include "dcontainer.h"
#include "dreferences.h"
#include "dflatassignmentvisitor.h"
#include "dclonevisitor.h"
#include "dupdatevisitor.h"

#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model_controller/mchildrenvisitor.h"

#include "qmt/controller/undocontroller.h"
#include "qmt/controller/undocommand.h"

#include "qmt/diagram/dobject.h"
#include "qmt/diagram/drelation.h"

#include "qmt/model/mobject.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/mrelation.h"


namespace qmt {

struct DiagramController::Clone {
    Clone();
    Uid _element_key;
    int _index_of_element;
    DElement *_cloned_element;
};

DiagramController::Clone::Clone()
    : _index_of_element(-1),
      _cloned_element(0)
{
}

class DiagramController::DiagramUndoCommand :
        public UndoCommand
{
public:

    DiagramUndoCommand(DiagramController *diagram_controller, const Uid &diagram_key, const QString &text)
        : UndoCommand(text),
          _diagram_controller(diagram_controller),
          _diagram_key(diagram_key)
    {
    }

protected:

    DiagramController *getDiagramController() const
    {
        return _diagram_controller;
    }

    Uid getDiagramKey() const { return _diagram_key; }

    MDiagram *getDiagram() const
    {
        MDiagram *diagram = _diagram_controller->findDiagram(_diagram_key);
        QMT_CHECK(diagram);
        return diagram;
    }

private:

    DiagramController *_diagram_controller;

    Uid _diagram_key;
};


class DiagramController::UpdateElementCommand :
        public DiagramUndoCommand
{
public:
    UpdateElementCommand(DiagramController *diagram_controller, const Uid &diagram_key, DElement *element,
                         DiagramController::UpdateAction update_action)
        : DiagramUndoCommand(diagram_controller, diagram_key, tr("Change")),
          _update_action(update_action)
    {
        DCloneVisitor visitor;
        element->accept(&visitor);
        _cloned_elements.insert(visitor.getCloned()->getUid(), visitor.getCloned());
    }

    ~UpdateElementCommand()
    {
        qDeleteAll(_cloned_elements);
    }

    bool mergeWith(const UndoCommand *other)
    {
        const UpdateElementCommand *other_update_command = dynamic_cast<const UpdateElementCommand *>(other);
        if (!other_update_command) {
            return false;
        }
        if (getDiagramKey() != other_update_command->getDiagramKey()) {
            return false;
        }
        if (_update_action == DiagramController::UPDATE_MAJOR || other_update_command->_update_action == DiagramController::UPDATE_MAJOR
                || _update_action != other_update_command->_update_action) {
            return false;
        }
        // join other elements into this command
        foreach (const DElement *other_element, other_update_command->_cloned_elements.values()) {
            if (!_cloned_elements.contains(other_element->getUid())) {
                DCloneVisitor visitor;
                other_element->accept(&visitor);
                _cloned_elements.insert(visitor.getCloned()->getUid(), visitor.getCloned());
            }
        }
        // the last update is a complete update of all changes...
        return true;
    }

    void redo()
    {
        if (canRedo()) {
            swap();
            UndoCommand::redo();
        }
    }

    void undo()
    {
        swap();
        UndoCommand::undo();
    }

private:
    void swap()
    {
        DiagramController *diagram_controller = getDiagramController();
        MDiagram *diagram = getDiagram();
        foreach (DElement *cloned_element, _cloned_elements) {
            DElement *active_element = diagram_controller->findElement(cloned_element->getUid(), diagram);
            QMT_CHECK(active_element);
            int row = diagram->getDiagramElements().indexOf(active_element);
            emit diagram_controller->beginUpdateElement(row, diagram);
            // clone active element
            DCloneVisitor clone_visitor;
            active_element->accept(&clone_visitor);
            DElement *new_element = clone_visitor.getCloned();
            // reset active element to cloned element
            DFlatAssignmentVisitor visitor(active_element);
            cloned_element->accept(&visitor);
            // replace stored element with new cloned active element
            QMT_CHECK(cloned_element->getUid() == new_element->getUid());
            _cloned_elements.insert(new_element->getUid(), new_element);
            delete cloned_element;
            emit diagram_controller->endUpdateElement(row, diagram);
        }
        diagram_controller->diagramModified(diagram);
    }

private:

    DiagramController::UpdateAction _update_action;

    QHash<Uid, DElement *> _cloned_elements;
};


class DiagramController::AbstractAddRemCommand :
        public DiagramUndoCommand
{
protected:
    AbstractAddRemCommand(DiagramController *diagram_controller, const Uid &diagram_key, const QString &command_label)
        : DiagramUndoCommand(diagram_controller, diagram_key, command_label)
    {
    }

    ~AbstractAddRemCommand()
    {
        foreach (const Clone &clone, _cloned_elements) {
            delete clone._cloned_element;
        }
    }

    void remove()
    {
        DiagramController *diagram_controller = getDiagramController();
        MDiagram *diagram = getDiagram();
        bool removed = false;
        for (int i = 0; i < _cloned_elements.count(); ++i) {
            Clone &clone = _cloned_elements[i];
            QMT_CHECK(!clone._cloned_element);
            DElement *active_element = diagram_controller->findElement(clone._element_key, diagram);
            QMT_CHECK(active_element);
            clone._index_of_element = diagram->getDiagramElements().indexOf(active_element);
            QMT_CHECK(clone._index_of_element >= 0);
            emit diagram_controller->beginRemoveElement(clone._index_of_element, diagram);
            DCloneDeepVisitor clone_visitor;
            active_element->accept(&clone_visitor);
            clone._cloned_element = clone_visitor.getCloned();
            diagram->removeDiagramElement(active_element);
            emit diagram_controller->endRemoveElement(clone._index_of_element, diagram);
            removed = true;
        }
        if (removed) {
            diagram_controller->diagramModified(diagram);
        }
    }

    void insert()
    {
        DiagramController *diagram_controller = getDiagramController();
        MDiagram *diagram = getDiagram();
        bool inserted = false;
        for (int i = _cloned_elements.count() - 1; i >= 0; --i) {
            Clone &clone = _cloned_elements[i];
            QMT_CHECK(clone._cloned_element);
            QMT_CHECK(clone._cloned_element->getUid() == clone._element_key);
            emit diagram_controller->beginInsertElement(clone._index_of_element, diagram);
            diagram->insertDiagramElement(clone._index_of_element, clone._cloned_element);
            clone._cloned_element = 0;
            emit diagram_controller->endInsertElement(clone._index_of_element, diagram);
            inserted = true;
        }
        if (inserted) {
            diagram_controller->diagramModified(diagram);
        }

    }

protected:
    QList<Clone> _cloned_elements;

};

class DiagramController::AddElementsCommand :
        public AbstractAddRemCommand
{
public:
    AddElementsCommand(DiagramController *diagram_controller, const Uid &diagram_key, const QString &command_label)
        : AbstractAddRemCommand(diagram_controller, diagram_key, command_label)
    {
    }

    void add(const Uid &element_key)
    {
        Clone clone;

        clone._element_key = element_key;
        _cloned_elements.append(clone);
    }

    void redo()
    {
        if (canRedo()) {
            insert();
            UndoCommand::redo();
        }
    }

    void undo()
    {
        remove();
        UndoCommand::undo();
    }
};


class DiagramController::RemoveElementsCommand :
        public AbstractAddRemCommand
{
public:
    RemoveElementsCommand(DiagramController *diagram_controller, const Uid &diagram_key, const QString &command_label)
        : AbstractAddRemCommand(diagram_controller, diagram_key, command_label)
    {
    }

    void add(DElement *element)
    {
        Clone clone;

        MDiagram *diagram = getDiagram();
        clone._element_key = element->getUid();
        clone._index_of_element = diagram->getDiagramElements().indexOf(element);
        QMT_CHECK(clone._index_of_element >= 0);
        DCloneDeepVisitor visitor;
        element->accept(&visitor);
        clone._cloned_element = visitor.getCloned();
        QMT_CHECK(clone._cloned_element);
        _cloned_elements.append(clone);
    }

    void redo()
    {
        if (canRedo()) {
            remove();
            UndoCommand::redo();
        }
    }

    void undo()
    {
        insert();
        UndoCommand::undo();
    }
};


class DiagramController::FindDiagramsVisitor :
        public MChildrenVisitor
{
public:

    FindDiagramsVisitor(QList<MDiagram *> *all_diagrams)
        : _all_diagrams(all_diagrams)
    {
    }

    void visitMDiagram(MDiagram *diagram)
    {
        _all_diagrams->append(diagram);
        MChildrenVisitor::visitMDiagram(diagram);
    }

private:

    QList<MDiagram *> *_all_diagrams;
};


DiagramController::DiagramController(QObject *parent)
    : QObject(parent),
      _model_controller(0),
      _undo_controller(0)
{
}

DiagramController::~DiagramController()
{
}

void DiagramController::setModelController(ModelController *model_controller)
{
    if (_model_controller) {
        disconnect(_model_controller, 0, this, 0);
        _model_controller = 0;
    }
    if (model_controller) {
        _model_controller = model_controller;
        connect(model_controller, SIGNAL(beginResetModel()), this, SLOT(onBeginResetModel()));
        connect(model_controller, SIGNAL(endResetModel()), this, SLOT(onEndResetModel()));

        connect(model_controller, SIGNAL(beginUpdateObject(int,const MObject*)), this, SLOT(onBeginUpdateObject(int,const MObject*)));
        connect(model_controller, SIGNAL(endUpdateObject(int,const MObject*)), this, SLOT(onEndUpdateObject(int,const MObject*)));
        connect(model_controller, SIGNAL(beginInsertObject(int,const MObject*)), this, SLOT(onBeginInsertObject(int,const MObject*)));
        connect(model_controller, SIGNAL(endInsertObject(int,const MObject*)), this, SLOT(onEndInsertObject(int,const MObject*)));
        connect(model_controller, SIGNAL(beginRemoveObject(int,const MObject*)), this, SLOT(onBeginRemoveObject(int,const MObject*)));
        connect(model_controller, SIGNAL(endRemoveObject(int,const MObject*)), this, SLOT(onEndRemoveObject(int,const MObject*)));
        connect(model_controller, SIGNAL(beginMoveObject(int,const MObject*)), this, SLOT(onBeginMoveObject(int,const MObject*)));
        connect(model_controller, SIGNAL(endMoveObject(int,const MObject*)), this, SLOT(onEndMoveObject(int,const MObject*)));

        connect(model_controller, SIGNAL(beginUpdateRelation(int,const MObject*)), this, SLOT(onBeginUpdateRelation(int,const MObject*)));
        connect(model_controller, SIGNAL(endUpdateRelation(int,const MObject*)), this, SLOT(onEndUpdateRelation(int,const MObject*)));
        connect(model_controller, SIGNAL(beginRemoveRelation(int,const MObject*)), this, SLOT(onBeginRemoveRelation(int,const MObject*)));
        connect(model_controller, SIGNAL(endRemoveRelation(int,const MObject*)), this, SLOT(onEndRemoveRelation(int,const MObject*)));
        connect(model_controller, SIGNAL(beginMoveRelation(int,const MObject*)), this, SLOT(onBeginMoveRelation(int,const MObject*)));
        connect(model_controller, SIGNAL(endMoveRelation(int,const MObject*)), this, SLOT(onEndMoveRelation(int,const MObject*)));
    }
}

void DiagramController::setUndoController(UndoController *undo_controller)
{
    _undo_controller = undo_controller;
}

MDiagram *DiagramController::findDiagram(const Uid &diagram_key) const
{
    return dynamic_cast<MDiagram *>(_model_controller->findObject(diagram_key));
}

void DiagramController::addElement(DElement *element, MDiagram *diagram)
{
    int row = diagram->getDiagramElements().count();
    emit beginInsertElement(row, diagram);
    updateElementFromModel(element, diagram, false);
    if (_undo_controller) {
        AddElementsCommand *undo_command = new AddElementsCommand(this, diagram->getUid(), tr("Add Object"));
        _undo_controller->push(undo_command);
        undo_command->add(element->getUid());
    }
    diagram->addDiagramElement(element);
    emit endInsertElement(row, diagram);
    diagramModified(diagram);
}

void DiagramController::removeElement(DElement *element, MDiagram *diagram)
{
    removeRelations(element, diagram);
    int row = diagram->getDiagramElements().indexOf(element);
    emit beginRemoveElement(row, diagram);
    if (_undo_controller) {
        RemoveElementsCommand *undo_command = new RemoveElementsCommand(this, diagram->getUid(), tr("Remove Object"));
        _undo_controller->push(undo_command);
        undo_command->add(element);
    }
    diagram->removeDiagramElement(element);
    emit endRemoveElement(row, diagram);
    diagramModified(diagram);
}

DElement *DiagramController::findElement(const Uid &key, const MDiagram *diagram) const
{
    QMT_CHECK(diagram);

    return diagram->findDiagramElement(key);
}

bool DiagramController::hasDelegate(const MElement *model_element, const MDiagram *diagram) const
{
    // PERFORM smarter implementation after map is introduced
    return findDelegate(model_element, diagram) != 0;
}

DElement *DiagramController::findDelegate(const MElement *model_element, const MDiagram *diagram) const
{
    Q_UNUSED(diagram);
    // PERFORM use map to increase performance
    foreach (DElement *diagram_element, diagram->getDiagramElements()) {
        if (diagram_element->getModelUid().isValid() && diagram_element->getModelUid() == model_element->getUid()) {
            return diagram_element;
        }
    }
    return 0;
}

void DiagramController::startUpdateElement(DElement *element, MDiagram *diagram, UpdateAction update_action)
{
    emit beginUpdateElement(diagram->getDiagramElements().indexOf(element), diagram);
    if (_undo_controller) {
        _undo_controller->push(new UpdateElementCommand(this, diagram->getUid(), element, update_action));
    }
}

void DiagramController::finishUpdateElement(DElement *element, MDiagram *diagram, bool cancelled)
{
    if (!cancelled) {
        updateElementFromModel(element, diagram, false);
    }
    emit endUpdateElement(diagram->getDiagramElements().indexOf(element), diagram);
    if (!cancelled) {
        diagramModified(diagram);
    }
}

void DiagramController::breakUndoChain()
{
    _undo_controller->doNotMerge();
}

DContainer DiagramController::cutElements(const DSelection &diagram_selection, MDiagram *diagram)
{
    DContainer copied_elements = copyElements(diagram_selection, diagram);
    deleteElements(diagram_selection, diagram, tr("Cut"));
    return copied_elements;
}

DContainer DiagramController::copyElements(const DSelection &diagram_selection, const MDiagram *diagram)
{
    QMT_CHECK(diagram);

    DReferences simplified_selection = simplify(diagram_selection, diagram);
    DContainer copied_elements;
    foreach (const DElement *element, simplified_selection.getElements()) {
        DCloneDeepVisitor visitor;
        element->accept(&visitor);
        DElement *cloned_element = visitor.getCloned();
        copied_elements.submit(cloned_element);
    }
    return copied_elements;
}

void DiagramController::pasteElements(const DContainer &diagram_container, MDiagram *diagram)
{
    QMT_CHECK(diagram);

    // clone all elements and renew their keys
    QHash<Uid, Uid> renewed_keys;
    QList<DElement *> cloned_elements;
    foreach (const DElement *element, diagram_container.getElements()) {
        if (!isDelegatedElementOnDiagram(element, diagram)) {
            DCloneDeepVisitor visitor;
            element->accept(&visitor);
            DElement *cloned_element = visitor.getCloned();
            renewElementKey(cloned_element, &renewed_keys);
            cloned_elements.append(cloned_element);
        }
    }
    // fix all keys referencing between pasting elements
    foreach(DElement *cloned_element, cloned_elements) {
        DRelation *relation = dynamic_cast<DRelation *>(cloned_element);
        if (relation) {
            updateRelationKeys(relation, renewed_keys);
        }
    }
    if (_undo_controller) {
        _undo_controller->beginMergeSequence(tr("Paste"));
    }
    // insert all elements
    bool added = false;
    foreach (DElement *cloned_element, cloned_elements) {
        if (!dynamic_cast<DRelation *>(cloned_element)) {
            int row = diagram->getDiagramElements().size();
            emit beginInsertElement(row, diagram);
            if (_undo_controller) {
                AddElementsCommand *undo_command = new AddElementsCommand(this, diagram->getUid(), tr("Paste"));
                _undo_controller->push(undo_command);
                undo_command->add(cloned_element->getUid());
            }
            diagram->addDiagramElement(cloned_element);
            emit endInsertElement(row, diagram);
            added = true;
        }
    }
    foreach (DElement *cloned_element, cloned_elements) {
        DRelation *cloned_relation = dynamic_cast<DRelation *>(cloned_element);
        if (cloned_relation && areRelationEndsOnDiagram(cloned_relation, diagram)) {
            int row = diagram->getDiagramElements().size();
            emit beginInsertElement(row, diagram);
            if (_undo_controller) {
                AddElementsCommand *undo_command = new AddElementsCommand(this, diagram->getUid(), tr("Paste"));
                _undo_controller->push(undo_command);
                undo_command->add(cloned_element->getUid());
            }
            diagram->addDiagramElement(cloned_element);
            emit endInsertElement(row, diagram);
            added = true;
        }
    }
    if (added) {
        diagramModified(diagram);
    }
    if (_undo_controller) {
        _undo_controller->endMergeSequence();
    }
}

void DiagramController::deleteElements(const DSelection &diagram_selection, MDiagram *diagram)
{
    deleteElements(diagram_selection, diagram, tr("Delete"));
}

void DiagramController::onBeginResetModel()
{
    _all_diagrams.clear();
    emit beginResetAllDiagrams();
}

void DiagramController::onEndResetModel()
{
    updateAllDiagramsList();
    foreach (MDiagram *diagram, _all_diagrams) {
        // remove all elements which are not longer part of the model
        foreach (DElement *element, diagram->getDiagramElements()) {
            if (element->getModelUid().isValid()) {
                MElement *model_element = _model_controller->findElement(element->getModelUid());
                if (!model_element) {
                    removeElement(element, diagram);
                }
            }
        }
        // update all remaining elements from model
        foreach (DElement *element, diagram->getDiagramElements()) {
            updateElementFromModel(element, diagram, false);
        }
    }
    emit endResetAllDiagrams();
}

void DiagramController::onBeginUpdateObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);

    // nothing to do
}

void DiagramController::onEndUpdateObject(int row, const MObject *parent)
{
    MObject *model_object = _model_controller->getObject(row, parent);
    QMT_CHECK(model_object);
    MPackage *model_package = dynamic_cast<MPackage *>(model_object);
    foreach (MDiagram *diagram, _all_diagrams) {
        DObject *object = findDelegate<DObject>(model_object, diagram);
        if (object) {
            updateElementFromModel(object, diagram, true);
        }
        if (model_package) {
            // update each element that has the updated object as its owner (for context changes)
            foreach (DElement *diagram_element, diagram->getDiagramElements()) {
                if (diagram_element->getModelUid().isValid()) {
                    MObject *mobject = _model_controller->findObject(diagram_element->getModelUid());
                    if (mobject && mobject->getOwner() == model_package) {
                        updateElementFromModel(diagram_element, diagram, true);
                    }
                }
            }
        }
    }
}

void DiagramController::onBeginInsertObject(int row, const MObject *owner)
{
    Q_UNUSED(row);
    Q_UNUSED(owner);
}

void DiagramController::onEndInsertObject(int row, const MObject *owner)
{
    QMT_CHECK(owner);

    MObject *model_object = _model_controller->getObject(row, owner);
    if (MDiagram *model_diagram = dynamic_cast<MDiagram *>(model_object)) {
        QMT_CHECK(!_all_diagrams.contains(model_diagram));
        _all_diagrams.append(model_diagram);
    }
}

void DiagramController::onBeginRemoveObject(int row, const MObject *parent)
{
    QMT_CHECK(parent);

    MObject *model_object = _model_controller->getObject(row, parent);
    removeObjects(model_object);
}

void DiagramController::onEndRemoveObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void DiagramController::onBeginMoveObject(int former_row, const MObject *former_owner)
{
    Q_UNUSED(former_row);
    Q_UNUSED(former_owner);

}

void DiagramController::onEndMoveObject(int row, const MObject *owner)
{
    onEndUpdateObject(row, owner);

    // if diagram was moved update all elements because of changed context
    MObject *model_object = _model_controller->getObject(row, owner);
    QMT_CHECK(model_object);
    MDiagram *model_diagram = dynamic_cast<MDiagram *>(model_object);
    if (model_diagram) {
        emit beginResetDiagram(model_diagram);
        foreach (DElement *diagram_element, model_diagram->getDiagramElements()) {
            updateElementFromModel(diagram_element, model_diagram, false);
        }
        emit endResetDiagram(model_diagram);
    }
}

void DiagramController::onBeginUpdateRelation(int row, const MObject *owner)
{
    Q_UNUSED(row);
    Q_UNUSED(owner);

    // nothing to do
}

void DiagramController::onEndUpdateRelation(int row, const MObject *owner)
{
    MRelation *model_relation = owner->getRelations().at(row);
    foreach (MDiagram *diagram, _all_diagrams) {
        DRelation *relation = findDelegate<DRelation>(model_relation, diagram);
        if (relation) {
            updateElementFromModel(relation, diagram, true);
        }
    }
}

void DiagramController::onBeginRemoveRelation(int row, const MObject *owner)
{
    QMT_CHECK(owner);

    MRelation *model_relation = owner->getRelations().at(row);
    removeRelations(model_relation);
}

void DiagramController::onEndRemoveRelation(int row, const MObject *owner)
{
    Q_UNUSED(row);
    Q_UNUSED(owner);
}

void DiagramController::onBeginMoveRelation(int former_row, const MObject *former_owner)
{
    Q_UNUSED(former_row);
    Q_UNUSED(former_owner);

    // nothing to do
}

void DiagramController::onEndMoveRelation(int row, const MObject *owner)
{
    onEndUpdateRelation(row, owner);
}

void DiagramController::deleteElements(const DSelection &diagram_selection, MDiagram *diagram, const QString &command_label)
{
    QMT_CHECK(diagram);

    DReferences simplified_selection = simplify(diagram_selection, diagram);
    if (simplified_selection.getElements().isEmpty()) {
        return;
    }
    if (_undo_controller) {
        _undo_controller->beginMergeSequence(command_label);
    }
    bool removed = false;
    foreach (DElement *element, simplified_selection.getElements()) {
        // element may have been deleted indirectly by predecessor element in loop
        if ((element = findElement(element->getUid(), diagram))) {
            removeRelations(element, diagram);
            int row = diagram->getDiagramElements().indexOf(element);
            emit beginRemoveElement(diagram->getDiagramElements().indexOf(element), diagram);
            if (_undo_controller) {
                RemoveElementsCommand *cut_command = new RemoveElementsCommand(this, diagram->getUid(), command_label);
                _undo_controller->push(cut_command);
                cut_command->add(element);
            }
            diagram->removeDiagramElement(element);
            emit endRemoveElement(row, diagram);
            removed = true;
        }
    }
    if (removed) {
        diagramModified(diagram);
    }
    if (_undo_controller) {
        _undo_controller->endMergeSequence();
    }
}

DElement *DiagramController::findElementOnAnyDiagram(const Uid &uid)
{
    foreach (MDiagram *diagram, _all_diagrams) {
        DElement *element = findElement(uid, diagram);
        if (element) {
            return element;
        }
    }
    return 0;
}

void DiagramController::removeObjects(MObject *model_object)
{
    foreach (MDiagram *diagram, _all_diagrams) {
        DElement *diagram_element = findDelegate(model_object, diagram);
        if (diagram_element) {
            removeElement(diagram_element, diagram);
        }
        foreach (const Handle<MRelation> &relation, model_object->getRelations()) {
            DElement *diagram_element = findDelegate(relation.getTarget(), diagram);
            if (diagram_element) {
                removeElement(diagram_element, diagram);
            }
        }
    }
    foreach (const Handle<MObject> &object, model_object->getChildren()) {
        if (object.hasTarget()) {
            removeObjects(object.getTarget());
        }
    }
    if (MDiagram *diagram = dynamic_cast<MDiagram *>(model_object)) {
        emit diagramAboutToBeRemoved(diagram);
        QMT_CHECK(_all_diagrams.contains(diagram));
        _all_diagrams.removeOne(diagram);
        QMT_CHECK(!_all_diagrams.contains(diagram));
        // PERFORM increase performace
        while (!diagram->getDiagramElements().isEmpty()) {
            DElement *element = diagram->getDiagramElements().first();
            removeElement(element, diagram);
        }
    }
}

void DiagramController::removeRelations(MRelation *model_relation)
{
    foreach (MDiagram *diagram, _all_diagrams) {
        DElement *diagram_element = findDelegate(model_relation, diagram);
        if (diagram_element) {
            removeElement(diagram_element, diagram);
        }
    }
}

void DiagramController::removeRelations(DElement *element, MDiagram *diagram)
{
    DObject *diagram_object = dynamic_cast<DObject *>(element);
    if (diagram_object) {
        foreach (DElement *diagram_element, diagram->getDiagramElements()) {
            if (DRelation *diagram_relation = dynamic_cast<DRelation *>(diagram_element)) {
                if (diagram_relation->getEndA() == diagram_object->getUid() || diagram_relation->getEndB() == diagram_object->getUid()) {
                    removeElement(diagram_relation, diagram);
                }
            }
        }
    }
}

void DiagramController::renewElementKey(DElement *element, QHash<Uid, Uid> *renewed_keys)
{
    QMT_CHECK(renewed_keys);

    if (element) {
        DElement *existing_element_on_diagram = findElementOnAnyDiagram(element->getUid());
        if (existing_element_on_diagram) {
            QMT_CHECK(existing_element_on_diagram != element);
            Uid old_key = element->getUid();
            element->renewUid();
            Uid new_key = element->getUid();
            renewed_keys->insert(old_key, new_key);
        }
    }
}

void DiagramController::updateRelationKeys(DRelation *relation, const QHash<Uid, Uid> &renewed_keys)
{
    Uid new_end_a_key = renewed_keys.value(relation->getEndA(), Uid::getInvalidUid());
    if (new_end_a_key.isValid()) {
        relation->setEndA(new_end_a_key);
    }
    Uid new_end_b_key = renewed_keys.value(relation->getEndB(), Uid::getInvalidUid());
    if (new_end_b_key.isValid()) {
        relation->setEndB(new_end_b_key);
    }
}

void DiagramController::updateElementFromModel(DElement *element, const MDiagram *diagram, bool emit_update_signal)
{
    if (!element->getModelUid().isValid()) {
        return;
    }

    DUpdateVisitor visitor(element, diagram);

    MElement *melement = _model_controller->findElement(element->getModelUid());
    QMT_CHECK(melement);

    if (emit_update_signal) {
        visitor.setCheckNeedsUpdate(true);
        melement->accept(&visitor);
        if (visitor.updateNeeded()) {
            int row = diagram->getDiagramElements().indexOf(element);
            emit beginUpdateElement(row, diagram);
            visitor.setCheckNeedsUpdate(false);
            melement->accept(&visitor);
            emit endUpdateElement(row, diagram);
        }
    } else {
        melement->accept(&visitor);
    }
}

void DiagramController::diagramModified(MDiagram *diagram)
{
    // the modification date is updated intentionally without signalling model controller avoiding recursive change updates
    diagram->setLastModifiedToNow();
    emit modified(diagram);
}

DReferences DiagramController::simplify(const DSelection &diagram_selection, const MDiagram *diagram)
{
    DReferences references;
    foreach (const DSelection::Index &index, diagram_selection.getIndices()) {
        DElement *element = findElement(index.getElementKey(), diagram);
        if (element) {
            references.append(element);
        }
    }
    return references;
}

MElement *DiagramController::getDelegatedElement(const DElement *element) const
{
    if (!element->getModelUid().isValid()) {
        return 0;
    }
    return _model_controller->findElement(element->getModelUid());
}

bool DiagramController::isDelegatedElementOnDiagram(const DElement *element, const MDiagram *diagram) const
{
    MElement *model_element = getDelegatedElement(element);
    if (!model_element) {
        return false;
    }
    return hasDelegate(model_element, diagram);
}

bool DiagramController::areRelationEndsOnDiagram(const DRelation *relation, const MDiagram *diagram) const
{
    return findElement(relation->getEndA(), diagram) && findElement(relation->getEndB(), diagram);
}

void DiagramController::updateAllDiagramsList()
{
    _all_diagrams.clear();
    if (_model_controller && _model_controller->getRootPackage()) {
        FindDiagramsVisitor visitor(&_all_diagrams);
        _model_controller->getRootPackage()->accept(&visitor);
    }
}

}
