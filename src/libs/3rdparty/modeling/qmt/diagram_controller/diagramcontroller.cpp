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
    Uid m_elementKey;
    int m_indexOfElement;
    DElement *m_clonedElement;
};

DiagramController::Clone::Clone()
    : m_indexOfElement(-1),
      m_clonedElement(0)
{
}

class DiagramController::DiagramUndoCommand :
        public UndoCommand
{
public:

    DiagramUndoCommand(DiagramController *diagram_controller, const Uid &diagram_key, const QString &text)
        : UndoCommand(text),
          m_diagramController(diagram_controller),
          m_diagramKey(diagram_key)
    {
    }

protected:

    DiagramController *getDiagramController() const
    {
        return m_diagramController;
    }

    Uid getDiagramKey() const { return m_diagramKey; }

    MDiagram *getDiagram() const
    {
        MDiagram *diagram = m_diagramController->findDiagram(m_diagramKey);
        QMT_CHECK(diagram);
        return diagram;
    }

private:

    DiagramController *m_diagramController;

    Uid m_diagramKey;
};


class DiagramController::UpdateElementCommand :
        public DiagramUndoCommand
{
public:
    UpdateElementCommand(DiagramController *diagram_controller, const Uid &diagram_key, DElement *element,
                         DiagramController::UpdateAction update_action)
        : DiagramUndoCommand(diagram_controller, diagram_key, tr("Change")),
          m_updateAction(update_action)
    {
        DCloneVisitor visitor;
        element->accept(&visitor);
        m_clonedElements.insert(visitor.getCloned()->getUid(), visitor.getCloned());
    }

    ~UpdateElementCommand()
    {
        qDeleteAll(m_clonedElements);
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
        if (m_updateAction == DiagramController::UPDATE_MAJOR || other_update_command->m_updateAction == DiagramController::UPDATE_MAJOR
                || m_updateAction != other_update_command->m_updateAction) {
            return false;
        }
        // join other elements into this command
        foreach (const DElement *other_element, other_update_command->m_clonedElements.values()) {
            if (!m_clonedElements.contains(other_element->getUid())) {
                DCloneVisitor visitor;
                other_element->accept(&visitor);
                m_clonedElements.insert(visitor.getCloned()->getUid(), visitor.getCloned());
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
        foreach (DElement *cloned_element, m_clonedElements) {
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
            m_clonedElements.insert(new_element->getUid(), new_element);
            delete cloned_element;
            emit diagram_controller->endUpdateElement(row, diagram);
        }
        diagram_controller->diagramModified(diagram);
    }

private:

    DiagramController::UpdateAction m_updateAction;

    QHash<Uid, DElement *> m_clonedElements;
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
        foreach (const Clone &clone, m_clonedElements) {
            delete clone.m_clonedElement;
        }
    }

    void remove()
    {
        DiagramController *diagram_controller = getDiagramController();
        MDiagram *diagram = getDiagram();
        bool removed = false;
        for (int i = 0; i < m_clonedElements.count(); ++i) {
            Clone &clone = m_clonedElements[i];
            QMT_CHECK(!clone.m_clonedElement);
            DElement *active_element = diagram_controller->findElement(clone.m_elementKey, diagram);
            QMT_CHECK(active_element);
            clone.m_indexOfElement = diagram->getDiagramElements().indexOf(active_element);
            QMT_CHECK(clone.m_indexOfElement >= 0);
            emit diagram_controller->beginRemoveElement(clone.m_indexOfElement, diagram);
            DCloneDeepVisitor clone_visitor;
            active_element->accept(&clone_visitor);
            clone.m_clonedElement = clone_visitor.getCloned();
            diagram->removeDiagramElement(active_element);
            emit diagram_controller->endRemoveElement(clone.m_indexOfElement, diagram);
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
        for (int i = m_clonedElements.count() - 1; i >= 0; --i) {
            Clone &clone = m_clonedElements[i];
            QMT_CHECK(clone.m_clonedElement);
            QMT_CHECK(clone.m_clonedElement->getUid() == clone.m_elementKey);
            emit diagram_controller->beginInsertElement(clone.m_indexOfElement, diagram);
            diagram->insertDiagramElement(clone.m_indexOfElement, clone.m_clonedElement);
            clone.m_clonedElement = 0;
            emit diagram_controller->endInsertElement(clone.m_indexOfElement, diagram);
            inserted = true;
        }
        if (inserted) {
            diagram_controller->diagramModified(diagram);
        }

    }

protected:
    QList<Clone> m_clonedElements;

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

        clone.m_elementKey = element_key;
        m_clonedElements.append(clone);
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
        clone.m_elementKey = element->getUid();
        clone.m_indexOfElement = diagram->getDiagramElements().indexOf(element);
        QMT_CHECK(clone.m_indexOfElement >= 0);
        DCloneDeepVisitor visitor;
        element->accept(&visitor);
        clone.m_clonedElement = visitor.getCloned();
        QMT_CHECK(clone.m_clonedElement);
        m_clonedElements.append(clone);
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
        : m_allDiagrams(all_diagrams)
    {
    }

    void visitMDiagram(MDiagram *diagram)
    {
        m_allDiagrams->append(diagram);
        MChildrenVisitor::visitMDiagram(diagram);
    }

private:

    QList<MDiagram *> *m_allDiagrams;
};


DiagramController::DiagramController(QObject *parent)
    : QObject(parent),
      m_modelController(0),
      m_undoController(0)
{
}

DiagramController::~DiagramController()
{
}

void DiagramController::setModelController(ModelController *model_controller)
{
    if (m_modelController) {
        disconnect(m_modelController, 0, this, 0);
        m_modelController = 0;
    }
    if (model_controller) {
        m_modelController = model_controller;
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
    m_undoController = undo_controller;
}

MDiagram *DiagramController::findDiagram(const Uid &diagram_key) const
{
    return dynamic_cast<MDiagram *>(m_modelController->findObject(diagram_key));
}

void DiagramController::addElement(DElement *element, MDiagram *diagram)
{
    int row = diagram->getDiagramElements().count();
    emit beginInsertElement(row, diagram);
    updateElementFromModel(element, diagram, false);
    if (m_undoController) {
        AddElementsCommand *undo_command = new AddElementsCommand(this, diagram->getUid(), tr("Add Object"));
        m_undoController->push(undo_command);
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
    if (m_undoController) {
        RemoveElementsCommand *undo_command = new RemoveElementsCommand(this, diagram->getUid(), tr("Remove Object"));
        m_undoController->push(undo_command);
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
    if (m_undoController) {
        m_undoController->push(new UpdateElementCommand(this, diagram->getUid(), element, update_action));
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
    m_undoController->doNotMerge();
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
    if (m_undoController) {
        m_undoController->beginMergeSequence(tr("Paste"));
    }
    // insert all elements
    bool added = false;
    foreach (DElement *cloned_element, cloned_elements) {
        if (!dynamic_cast<DRelation *>(cloned_element)) {
            int row = diagram->getDiagramElements().size();
            emit beginInsertElement(row, diagram);
            if (m_undoController) {
                AddElementsCommand *undo_command = new AddElementsCommand(this, diagram->getUid(), tr("Paste"));
                m_undoController->push(undo_command);
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
            if (m_undoController) {
                AddElementsCommand *undo_command = new AddElementsCommand(this, diagram->getUid(), tr("Paste"));
                m_undoController->push(undo_command);
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
    if (m_undoController) {
        m_undoController->endMergeSequence();
    }
}

void DiagramController::deleteElements(const DSelection &diagram_selection, MDiagram *diagram)
{
    deleteElements(diagram_selection, diagram, tr("Delete"));
}

void DiagramController::onBeginResetModel()
{
    m_allDiagrams.clear();
    emit beginResetAllDiagrams();
}

void DiagramController::onEndResetModel()
{
    updateAllDiagramsList();
    foreach (MDiagram *diagram, m_allDiagrams) {
        // remove all elements which are not longer part of the model
        foreach (DElement *element, diagram->getDiagramElements()) {
            if (element->getModelUid().isValid()) {
                MElement *model_element = m_modelController->findElement(element->getModelUid());
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
    MObject *model_object = m_modelController->getObject(row, parent);
    QMT_CHECK(model_object);
    MPackage *model_package = dynamic_cast<MPackage *>(model_object);
    foreach (MDiagram *diagram, m_allDiagrams) {
        DObject *object = findDelegate<DObject>(model_object, diagram);
        if (object) {
            updateElementFromModel(object, diagram, true);
        }
        if (model_package) {
            // update each element that has the updated object as its owner (for context changes)
            foreach (DElement *diagram_element, diagram->getDiagramElements()) {
                if (diagram_element->getModelUid().isValid()) {
                    MObject *mobject = m_modelController->findObject(diagram_element->getModelUid());
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

    MObject *model_object = m_modelController->getObject(row, owner);
    if (MDiagram *model_diagram = dynamic_cast<MDiagram *>(model_object)) {
        QMT_CHECK(!m_allDiagrams.contains(model_diagram));
        m_allDiagrams.append(model_diagram);
    }
}

void DiagramController::onBeginRemoveObject(int row, const MObject *parent)
{
    QMT_CHECK(parent);

    MObject *model_object = m_modelController->getObject(row, parent);
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
    MObject *model_object = m_modelController->getObject(row, owner);
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
    foreach (MDiagram *diagram, m_allDiagrams) {
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
    if (m_undoController) {
        m_undoController->beginMergeSequence(command_label);
    }
    bool removed = false;
    foreach (DElement *element, simplified_selection.getElements()) {
        // element may have been deleted indirectly by predecessor element in loop
        if ((element = findElement(element->getUid(), diagram))) {
            removeRelations(element, diagram);
            int row = diagram->getDiagramElements().indexOf(element);
            emit beginRemoveElement(diagram->getDiagramElements().indexOf(element), diagram);
            if (m_undoController) {
                RemoveElementsCommand *cut_command = new RemoveElementsCommand(this, diagram->getUid(), command_label);
                m_undoController->push(cut_command);
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
    if (m_undoController) {
        m_undoController->endMergeSequence();
    }
}

DElement *DiagramController::findElementOnAnyDiagram(const Uid &uid)
{
    foreach (MDiagram *diagram, m_allDiagrams) {
        DElement *element = findElement(uid, diagram);
        if (element) {
            return element;
        }
    }
    return 0;
}

void DiagramController::removeObjects(MObject *model_object)
{
    foreach (MDiagram *diagram, m_allDiagrams) {
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
        QMT_CHECK(m_allDiagrams.contains(diagram));
        m_allDiagrams.removeOne(diagram);
        QMT_CHECK(!m_allDiagrams.contains(diagram));
        // PERFORM increase performace
        while (!diagram->getDiagramElements().isEmpty()) {
            DElement *element = diagram->getDiagramElements().first();
            removeElement(element, diagram);
        }
    }
}

void DiagramController::removeRelations(MRelation *model_relation)
{
    foreach (MDiagram *diagram, m_allDiagrams) {
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

    MElement *melement = m_modelController->findElement(element->getModelUid());
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
    return m_modelController->findElement(element->getModelUid());
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
    m_allDiagrams.clear();
    if (m_modelController && m_modelController->getRootPackage()) {
        FindDiagramsVisitor visitor(&m_allDiagrams);
        m_modelController->getRootPackage()->accept(&visitor);
    }
}

}
