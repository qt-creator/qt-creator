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

#include <QDebug>

namespace qmt {

class DiagramController::Clone
{
public:
    Uid m_elementKey;
    int m_indexOfElement = -1;
    DElement *m_clonedElement = 0;
};

class DiagramController::DiagramUndoCommand : public UndoCommand
{
public:
    DiagramUndoCommand(DiagramController *diagramController, const Uid &diagramKey, const QString &text)
        : UndoCommand(text),
          m_diagramController(diagramController),
          m_diagramKey(diagramKey)
    {
    }

protected:
    DiagramController *diagramController() const
    {
        return m_diagramController;
    }

    Uid diagramKey() const { return m_diagramKey; }

    MDiagram *diagram() const
    {
        MDiagram *diagram = m_diagramController->findDiagram(m_diagramKey);
        QMT_CHECK(diagram);
        return diagram;
    }

private:
    DiagramController *m_diagramController = 0;
    Uid m_diagramKey;
};

class DiagramController::UpdateElementCommand : public DiagramUndoCommand
{
public:
    UpdateElementCommand(DiagramController *diagramController, const Uid &diagramKey, DElement *element,
                         DiagramController::UpdateAction updateAction)
        : DiagramUndoCommand(diagramController, diagramKey, tr("Change")),
          m_updateAction(updateAction)
    {
        DCloneVisitor visitor;
        element->accept(&visitor);
        m_clonedElements.insert(visitor.cloned()->uid(), visitor.cloned());
    }

    ~UpdateElementCommand() override
    {
        qDeleteAll(m_clonedElements);
    }

    bool mergeWith(const UndoCommand *other) override
    {
        auto otherUpdateCommand = dynamic_cast<const UpdateElementCommand *>(other);
        if (!otherUpdateCommand)
            return false;
        if (diagramKey() != otherUpdateCommand->diagramKey())
            return false;
        if (m_updateAction == DiagramController::UpdateMajor
                || otherUpdateCommand->m_updateAction == DiagramController::UpdateMajor
                || m_updateAction != otherUpdateCommand->m_updateAction) {
            return false;
        }
        // join other elements into this command
        foreach (const DElement *otherElement, otherUpdateCommand->m_clonedElements.values()) {
            if (!m_clonedElements.contains(otherElement->uid())) {
                DCloneVisitor visitor;
                otherElement->accept(&visitor);
                m_clonedElements.insert(visitor.cloned()->uid(), visitor.cloned());
            }
        }
        // the last update is a complete update of all changes...
        return true;
    }

    void redo() override
    {
        if (canRedo()) {
            swap();
            UndoCommand::redo();
        }
    }

    void undo() override
    {
        swap();
        UndoCommand::undo();
    }

private:
    void swap()
    {
        DiagramController *diagramController = this->diagramController();
        MDiagram *diagram = this->diagram();
        foreach (DElement *clonedElement, m_clonedElements) {
            DElement *activeElement = diagramController->findElement(clonedElement->uid(), diagram);
            QMT_CHECK(activeElement);
            int row = diagram->diagramElements().indexOf(activeElement);
            emit diagramController->beginUpdateElement(row, diagram);
            // clone active element
            DCloneVisitor cloneVisitor;
            activeElement->accept(&cloneVisitor);
            DElement *newElement = cloneVisitor.cloned();
            // reset active element to cloned element
            DFlatAssignmentVisitor visitor(activeElement);
            clonedElement->accept(&visitor);
            // replace stored element with new cloned active element
            QMT_CHECK(clonedElement->uid() == newElement->uid());
            m_clonedElements.insert(newElement->uid(), newElement);
            delete clonedElement;
            emit diagramController->endUpdateElement(row, diagram);
        }
        diagramController->diagramModified(diagram);
        diagramController->verifyDiagramsIntegrity();
    }

    DiagramController::UpdateAction m_updateAction = DiagramController::UpdateMajor;
    QHash<Uid, DElement *> m_clonedElements;
};

class DiagramController::AbstractAddRemCommand : public DiagramUndoCommand
{
protected:
    AbstractAddRemCommand(DiagramController *diagramController, const Uid &diagramKey, const QString &commandLabel)
        : DiagramUndoCommand(diagramController, diagramKey, commandLabel)
    {
    }

    ~AbstractAddRemCommand() override
    {
        foreach (const Clone &clone, m_clonedElements)
            delete clone.m_clonedElement;
    }

    void remove()
    {
        DiagramController *diagramController = this->diagramController();
        MDiagram *diagram = this->diagram();
        bool removed = false;
        for (int i = 0; i < m_clonedElements.count(); ++i) {
            Clone &clone = m_clonedElements[i];
            QMT_CHECK(!clone.m_clonedElement);
            DElement *activeElement = diagramController->findElement(clone.m_elementKey, diagram);
            QMT_CHECK(activeElement);
            clone.m_indexOfElement = diagram->diagramElements().indexOf(activeElement);
            QMT_CHECK(clone.m_indexOfElement >= 0);
            emit diagramController->beginRemoveElement(clone.m_indexOfElement, diagram);
            DCloneDeepVisitor cloneVisitor;
            activeElement->accept(&cloneVisitor);
            clone.m_clonedElement = cloneVisitor.cloned();
            diagram->removeDiagramElement(activeElement);
            emit diagramController->endRemoveElement(clone.m_indexOfElement, diagram);
            removed = true;
        }
        if (removed)
            diagramController->diagramModified(diagram);
        diagramController->verifyDiagramsIntegrity();
    }

    void insert()
    {
        DiagramController *diagramController = this->diagramController();
        MDiagram *diagram = this->diagram();
        bool inserted = false;
        for (int i = m_clonedElements.count() - 1; i >= 0; --i) {
            Clone &clone = m_clonedElements[i];
            QMT_CHECK(clone.m_clonedElement);
            QMT_CHECK(clone.m_clonedElement->uid() == clone.m_elementKey);
            emit diagramController->beginInsertElement(clone.m_indexOfElement, diagram);
            diagram->insertDiagramElement(clone.m_indexOfElement, clone.m_clonedElement);
            clone.m_clonedElement = 0;
            emit diagramController->endInsertElement(clone.m_indexOfElement, diagram);
            inserted = true;
        }
        if (inserted)
            diagramController->diagramModified(diagram);
        diagramController->verifyDiagramsIntegrity();
    }

    QList<DiagramController::Clone> m_clonedElements;
};

class DiagramController::AddElementsCommand : public AbstractAddRemCommand
{
public:
    AddElementsCommand(DiagramController *diagramController, const Uid &diagramKey, const QString &commandLabel)
        : AbstractAddRemCommand(diagramController, diagramKey, commandLabel)
    {
    }

    void add(const Uid &elementKey)
    {
        Clone clone;

        clone.m_elementKey = elementKey;
        m_clonedElements.append(clone);
    }

    void redo() override
    {
        if (canRedo()) {
            insert();
            UndoCommand::redo();
        }
    }

    void undo() override
    {
        remove();
        UndoCommand::undo();
    }
};

class DiagramController::RemoveElementsCommand : public AbstractAddRemCommand
{
public:
    RemoveElementsCommand(DiagramController *diagramController, const Uid &diagramKey, const QString &commandLabel)
        : AbstractAddRemCommand(diagramController, diagramKey, commandLabel)
    {
    }

    void add(DElement *element)
    {
        Clone clone;

        MDiagram *diagram = this->diagram();
        clone.m_elementKey = element->uid();
        clone.m_indexOfElement = diagram->diagramElements().indexOf(element);
        QMT_CHECK(clone.m_indexOfElement >= 0);
        DCloneDeepVisitor visitor;
        element->accept(&visitor);
        clone.m_clonedElement = visitor.cloned();
        QMT_CHECK(clone.m_clonedElement);
        m_clonedElements.append(clone);
    }

    void redo() override
    {
        if (canRedo()) {
            remove();
            UndoCommand::redo();
        }
    }

    void undo() override
    {
        insert();
        UndoCommand::undo();
    }
};

class DiagramController::FindDiagramsVisitor : public MChildrenVisitor
{
public:
    FindDiagramsVisitor(QList<MDiagram *> *allDiagrams)
        : m_allDiagrams(allDiagrams)
    {
    }

    void visitMDiagram(MDiagram *diagram) override
    {
        m_allDiagrams->append(diagram);
        MChildrenVisitor::visitMDiagram(diagram);
    }

private:
    QList<MDiagram *> *m_allDiagrams = 0;
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

void DiagramController::setModelController(ModelController *modelController)
{
    if (m_modelController) {
        disconnect(m_modelController, 0, this, 0);
        m_modelController = 0;
    }
    if (modelController) {
        m_modelController = modelController;
        connect(modelController, &ModelController::beginResetModel,
                this, &DiagramController::onBeginResetModel);
        connect(modelController, &ModelController::endResetModel,
                this, &DiagramController::onEndResetModel);

        connect(modelController, &ModelController::beginUpdateObject,
                this, &DiagramController::onBeginUpdateObject);
        connect(modelController, &ModelController::endUpdateObject,
                this, &DiagramController::onEndUpdateObject);
        connect(modelController, &ModelController::beginInsertObject,
                this, &DiagramController::onBeginInsertObject);
        connect(modelController, &ModelController::endInsertObject,
                this, &DiagramController::onEndInsertObject);
        connect(modelController, &ModelController::beginRemoveObject,
                this, &DiagramController::onBeginRemoveObject);
        connect(modelController, &ModelController::endRemoveObject,
                this, &DiagramController::onEndRemoveObject);
        connect(modelController, &ModelController::beginMoveObject,
                this, &DiagramController::onBeginMoveObject);
        connect(modelController, &ModelController::endMoveObject,
                this, &DiagramController::onEndMoveObject);

        connect(modelController, &ModelController::beginUpdateRelation,
                this, &DiagramController::onBeginUpdateRelation);
        connect(modelController, &ModelController::endUpdateRelation,
                this, &DiagramController::onEndUpdateRelation);
        connect(modelController, &ModelController::beginRemoveRelation,
                this, &DiagramController::onBeginRemoveRelation);
        connect(modelController, &ModelController::endRemoveRelation,
                this, &DiagramController::onEndRemoveRelation);
        connect(modelController, &ModelController::beginMoveRelation,
                this, &DiagramController::onBeginMoveRelation);
        connect(modelController, &ModelController::endMoveRelation,
                this, &DiagramController::onEndMoveRelation);
    }
}

void DiagramController::setUndoController(UndoController *undoController)
{
    m_undoController = undoController;
}

MDiagram *DiagramController::findDiagram(const Uid &diagramKey) const
{
    return dynamic_cast<MDiagram *>(m_modelController->findObject(diagramKey));
}

void DiagramController::addElement(DElement *element, MDiagram *diagram)
{
    int row = diagram->diagramElements().count();
    emit beginInsertElement(row, diagram);
    updateElementFromModel(element, diagram, false);
    if (m_undoController) {
        auto undoCommand = new AddElementsCommand(this, diagram->uid(), tr("Add Object"));
        m_undoController->push(undoCommand);
        undoCommand->add(element->uid());
    }
    diagram->addDiagramElement(element);
    emit endInsertElement(row, diagram);
    diagramModified(diagram);
    verifyDiagramsIntegrity();
}

void DiagramController::removeElement(DElement *element, MDiagram *diagram)
{
    removeRelations(element, diagram);
    int row = diagram->diagramElements().indexOf(element);
    emit beginRemoveElement(row, diagram);
    if (m_undoController) {
        auto undoCommand = new RemoveElementsCommand(this, diagram->uid(), tr("Remove Object"));
        m_undoController->push(undoCommand);
        undoCommand->add(element);
    }
    diagram->removeDiagramElement(element);
    emit endRemoveElement(row, diagram);
    diagramModified(diagram);
    verifyDiagramsIntegrity();
}

DElement *DiagramController::findElement(const Uid &key, const MDiagram *diagram) const
{
    QMT_CHECK(diagram);

    return diagram->findDiagramElement(key);
}

bool DiagramController::hasDelegate(const MElement *modelElement, const MDiagram *diagram) const
{
    // PERFORM smarter implementation after map is introduced
    return findDelegate(modelElement, diagram) != 0;
}

DElement *DiagramController::findDelegate(const MElement *modelElement, const MDiagram *diagram) const
{
    // PERFORM use map to increase performance
    foreach (DElement *diagramElement, diagram->diagramElements()) {
        if (diagramElement->modelUid().isValid() && diagramElement->modelUid() == modelElement->uid())
            return diagramElement;
    }
    return 0;
}

void DiagramController::startUpdateElement(DElement *element, MDiagram *diagram, UpdateAction updateAction)
{
    emit beginUpdateElement(diagram->diagramElements().indexOf(element), diagram);
    if (m_undoController)
        m_undoController->push(new UpdateElementCommand(this, diagram->uid(), element, updateAction));
}

void DiagramController::finishUpdateElement(DElement *element, MDiagram *diagram, bool cancelled)
{
    if (!cancelled)
        updateElementFromModel(element, diagram, false);
    emit endUpdateElement(diagram->diagramElements().indexOf(element), diagram);
    if (!cancelled)
        diagramModified(diagram);
    verifyDiagramsIntegrity();
}

void DiagramController::breakUndoChain()
{
    m_undoController->doNotMerge();
}

DContainer DiagramController::cutElements(const DSelection &diagramSelection, MDiagram *diagram)
{
    DContainer copiedElements = copyElements(diagramSelection, diagram);
    deleteElements(diagramSelection, diagram, tr("Cut"));
    return copiedElements;
}

DContainer DiagramController::copyElements(const DSelection &diagramSelection, const MDiagram *diagram)
{
    QMT_CHECK(diagram);

    DReferences simplifiedSelection = simplify(diagramSelection, diagram);
    DContainer copiedElements;
    foreach (const DElement *element, simplifiedSelection.elements()) {
        DCloneDeepVisitor visitor;
        element->accept(&visitor);
        DElement *clonedElement = visitor.cloned();
        copiedElements.submit(clonedElement);
    }
    return copiedElements;
}

void DiagramController::pasteElements(const DContainer &diagramContainer, MDiagram *diagram)
{
    QMT_CHECK(diagram);

    // clone all elements and renew their keys
    QHash<Uid, Uid> renewedKeys;
    QList<DElement *> clonedElements;
    foreach (const DElement *element, diagramContainer.elements()) {
        if (!isDelegatedElementOnDiagram(element, diagram)) {
            DCloneDeepVisitor visitor;
            element->accept(&visitor);
            DElement *clonedElement = visitor.cloned();
            renewElementKey(clonedElement, &renewedKeys);
            clonedElements.append(clonedElement);
        }
    }
    // fix all keys referencing between pasting elements
    foreach(DElement *clonedElement, clonedElements) {
        auto relation = dynamic_cast<DRelation *>(clonedElement);
        if (relation)
            updateRelationKeys(relation, renewedKeys);
    }
    if (m_undoController)
        m_undoController->beginMergeSequence(tr("Paste"));
    // insert all elements
    bool added = false;
    foreach (DElement *clonedElement, clonedElements) {
        if (!dynamic_cast<DRelation *>(clonedElement)) {
            int row = diagram->diagramElements().size();
            emit beginInsertElement(row, diagram);
            if (m_undoController) {
                auto undoCommand = new AddElementsCommand(this, diagram->uid(), tr("Paste"));
                m_undoController->push(undoCommand);
                undoCommand->add(clonedElement->uid());
            }
            diagram->addDiagramElement(clonedElement);
            emit endInsertElement(row, diagram);
            added = true;
        }
    }
    foreach (DElement *clonedElement, clonedElements) {
        auto clonedRelation = dynamic_cast<DRelation *>(clonedElement);
        if (clonedRelation && areRelationEndsOnDiagram(clonedRelation, diagram)) {
            int row = diagram->diagramElements().size();
            emit beginInsertElement(row, diagram);
            if (m_undoController) {
                auto undoCommand = new AddElementsCommand(this, diagram->uid(), tr("Paste"));
                m_undoController->push(undoCommand);
                undoCommand->add(clonedElement->uid());
            }
            diagram->addDiagramElement(clonedElement);
            emit endInsertElement(row, diagram);
            added = true;
        }
    }
    if (added)
        diagramModified(diagram);
    if (m_undoController)
        m_undoController->endMergeSequence();
    verifyDiagramsIntegrity();
}

void DiagramController::deleteElements(const DSelection &diagramSelection, MDiagram *diagram)
{
    deleteElements(diagramSelection, diagram, tr("Delete"));
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
        foreach (DElement *element, diagram->diagramElements()) {
            if (element->modelUid().isValid()) {
                MElement *modelElement = m_modelController->findElement(element->modelUid());
                if (!modelElement)
                    removeElement(element, diagram);
            }
        }
        // update all remaining elements from model
        foreach (DElement *element, diagram->diagramElements())
            updateElementFromModel(element, diagram, false);
    }
    emit endResetAllDiagrams();
    verifyDiagramsIntegrity();
}

void DiagramController::onBeginUpdateObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);

    // nothing to do
}

void DiagramController::onEndUpdateObject(int row, const MObject *parent)
{
    MObject *modelObject = m_modelController->object(row, parent);
    QMT_CHECK(modelObject);
    auto modelPackage = dynamic_cast<MPackage *>(modelObject);
    foreach (MDiagram *diagram, m_allDiagrams) {
        DObject *object = findDelegate<DObject>(modelObject, diagram);
        if (object) {
            updateElementFromModel(object, diagram, true);
        }
        if (modelPackage) {
            // update each element that has the updated object as its owner (for context changes)
            foreach (DElement *diagramElement, diagram->diagramElements()) {
                if (diagramElement->modelUid().isValid()) {
                    MObject *mobject = m_modelController->findObject(diagramElement->modelUid());
                    if (mobject && mobject->owner() == modelPackage)
                        updateElementFromModel(diagramElement, diagram, true);
                }
            }
        }
    }
    verifyDiagramsIntegrity();
}

void DiagramController::onBeginInsertObject(int row, const MObject *owner)
{
    Q_UNUSED(row);
    Q_UNUSED(owner);
}

void DiagramController::onEndInsertObject(int row, const MObject *owner)
{
    QMT_CHECK(owner);

    MObject *modelObject = m_modelController->object(row, owner);
    if (auto modelDiagram = dynamic_cast<MDiagram *>(modelObject)) {
        QMT_CHECK(!m_allDiagrams.contains(modelDiagram));
        m_allDiagrams.append(modelDiagram);
    }
    verifyDiagramsIntegrity();
}

void DiagramController::onBeginRemoveObject(int row, const MObject *parent)
{
    QMT_CHECK(parent);

    MObject *modelObject = m_modelController->object(row, parent);
    removeObjects(modelObject);
}

void DiagramController::onEndRemoveObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void DiagramController::onBeginMoveObject(int formerRow, const MObject *formerOwner)
{
    Q_UNUSED(formerRow);
    Q_UNUSED(formerOwner);
}

void DiagramController::onEndMoveObject(int row, const MObject *owner)
{
    onEndUpdateObject(row, owner);

    // if diagram was moved update all elements because of changed context
    MObject *modelObject = m_modelController->object(row, owner);
    QMT_CHECK(modelObject);
    auto modelDiagram = dynamic_cast<MDiagram *>(modelObject);
    if (modelDiagram) {
        emit beginResetDiagram(modelDiagram);
        foreach (DElement *diagramElement, modelDiagram->diagramElements())
            updateElementFromModel(diagramElement, modelDiagram, false);
        emit endResetDiagram(modelDiagram);
    }
    verifyDiagramsIntegrity();
}

void DiagramController::onBeginUpdateRelation(int row, const MObject *owner)
{
    Q_UNUSED(row);
    Q_UNUSED(owner);

    // nothing to do
}

void DiagramController::onEndUpdateRelation(int row, const MObject *owner)
{
    MRelation *modelRelation = owner->relations().at(row);
    foreach (MDiagram *diagram, m_allDiagrams) {
        DRelation *relation = findDelegate<DRelation>(modelRelation, diagram);
        if (relation) {
            updateElementFromModel(relation, diagram, true);
        }
    }
    verifyDiagramsIntegrity();
}

void DiagramController::onBeginRemoveRelation(int row, const MObject *owner)
{
    QMT_CHECK(owner);

    MRelation *modelRelation = owner->relations().at(row);
    removeRelations(modelRelation);
}

void DiagramController::onEndRemoveRelation(int row, const MObject *owner)
{
    Q_UNUSED(row);
    Q_UNUSED(owner);
}

void DiagramController::onBeginMoveRelation(int formerRow, const MObject *formerOwner)
{
    Q_UNUSED(formerRow);
    Q_UNUSED(formerOwner);

    // nothing to do
}

void DiagramController::onEndMoveRelation(int row, const MObject *owner)
{
    onEndUpdateRelation(row, owner);
}

void DiagramController::deleteElements(const DSelection &diagramSelection, MDiagram *diagram,
                                       const QString &commandLabel)
{
    QMT_CHECK(diagram);

    DReferences simplifiedSelection = simplify(diagramSelection, diagram);
    if (simplifiedSelection.elements().isEmpty())
        return;
    if (m_undoController)
        m_undoController->beginMergeSequence(commandLabel);
    bool removed = false;
    foreach (DElement *element, simplifiedSelection.elements()) {
        // element may have been deleted indirectly by predecessor element in loop
        if ((element = findElement(element->uid(), diagram))) {
            removeRelations(element, diagram);
            int row = diagram->diagramElements().indexOf(element);
            emit beginRemoveElement(diagram->diagramElements().indexOf(element), diagram);
            if (m_undoController) {
                auto cutCommand = new RemoveElementsCommand(this, diagram->uid(), commandLabel);
                m_undoController->push(cutCommand);
                cutCommand->add(element);
            }
            diagram->removeDiagramElement(element);
            emit endRemoveElement(row, diagram);
            removed = true;
        }
    }
    if (removed)
        diagramModified(diagram);
    if (m_undoController)
        m_undoController->endMergeSequence();
    verifyDiagramsIntegrity();
}

DElement *DiagramController::findElementOnAnyDiagram(const Uid &uid)
{
    foreach (MDiagram *diagram, m_allDiagrams) {
        DElement *element = findElement(uid, diagram);
        if (element)
            return element;
    }
    return 0;
}

void DiagramController::removeObjects(MObject *modelObject)
{
    foreach (MDiagram *diagram, m_allDiagrams) {
        DElement *diagramElement = findDelegate(modelObject, diagram);
        if (diagramElement)
            removeElement(diagramElement, diagram);
        foreach (const Handle<MRelation> &relation, modelObject->relations()) {
            DElement *diagramElement = findDelegate(relation.target(), diagram);
            if (diagramElement)
                removeElement(diagramElement, diagram);
        }
    }
    foreach (const Handle<MObject> &object, modelObject->children()) {
        if (object.hasTarget())
            removeObjects(object.target());
    }
    if (auto diagram = dynamic_cast<MDiagram *>(modelObject)) {
        emit diagramAboutToBeRemoved(diagram);
        QMT_CHECK(m_allDiagrams.contains(diagram));
        m_allDiagrams.removeOne(diagram);
        QMT_CHECK(!m_allDiagrams.contains(diagram));
        // PERFORM increase performace
        while (!diagram->diagramElements().isEmpty()) {
            DElement *element = diagram->diagramElements().first();
            removeElement(element, diagram);
        }
    }
    verifyDiagramsIntegrity();
}

void DiagramController::removeRelations(MRelation *modelRelation)
{
    foreach (MDiagram *diagram, m_allDiagrams) {
        DElement *diagramElement = findDelegate(modelRelation, diagram);
        if (diagramElement)
            removeElement(diagramElement, diagram);
    }
    verifyDiagramsIntegrity();
}

void DiagramController::removeRelations(DElement *element, MDiagram *diagram)
{
    auto diagramObject = dynamic_cast<DObject *>(element);
    if (diagramObject) {
        foreach (DElement *diagramElement, diagram->diagramElements()) {
            if (auto diagramRelation = dynamic_cast<DRelation *>(diagramElement)) {
                if (diagramRelation->endAUid() == diagramObject->uid()
                        || diagramRelation->endBUid() == diagramObject->uid()) {
                    removeElement(diagramRelation, diagram);
                }
            }
        }
        verifyDiagramsIntegrity();
    }
}

void DiagramController::renewElementKey(DElement *element, QHash<Uid, Uid> *renewedKeys)
{
    QMT_CHECK(renewedKeys);

    if (element) {
        DElement *existingElementOnDiagram = findElementOnAnyDiagram(element->uid());
        if (existingElementOnDiagram) {
            QMT_CHECK(existingElementOnDiagram != element);
            Uid oldKey = element->uid();
            element->renewUid();
            Uid newKey = element->uid();
            renewedKeys->insert(oldKey, newKey);
        }
    }
}

void DiagramController::updateRelationKeys(DRelation *relation, const QHash<Uid, Uid> &renewedKeys)
{
    Uid newEndAKey = renewedKeys.value(relation->endAUid(), Uid::invalidUid());
    if (newEndAKey.isValid())
        relation->setEndAUid(newEndAKey);
    Uid newEndBKey = renewedKeys.value(relation->endBUid(), Uid::invalidUid());
    if (newEndBKey.isValid())
        relation->setEndBUid(newEndBKey);
}

void DiagramController::updateElementFromModel(DElement *element, const MDiagram *diagram, bool emitUpdateSignal)
{
    if (!element->modelUid().isValid())
        return;

    DUpdateVisitor visitor(element, diagram);

    MElement *melement = m_modelController->findElement(element->modelUid());
    QMT_CHECK(melement);

    if (emitUpdateSignal) {
        visitor.setCheckNeedsUpdate(true);
        melement->accept(&visitor);
        if (visitor.isUpdateNeeded()) {
            int row = diagram->diagramElements().indexOf(element);
            emit beginUpdateElement(row, diagram);
            visitor.setCheckNeedsUpdate(false);
            melement->accept(&visitor);
            emit endUpdateElement(row, diagram);
        }
    } else {
        melement->accept(&visitor);
    }
    verifyDiagramsIntegrity();
}

void DiagramController::diagramModified(MDiagram *diagram)
{
    // the modification date is updated intentionally without signalling model controller
    // avoiding recursive change updates
    diagram->setLastModifiedToNow();
    emit modified(diagram);
}

DReferences DiagramController::simplify(const DSelection &diagramSelection, const MDiagram *diagram)
{
    DReferences references;
    foreach (const DSelection::Index &index, diagramSelection.indices()) {
        DElement *element = findElement(index.elementKey(), diagram);
        if (element)
            references.append(element);
    }
    return references;
}

MElement *DiagramController::delegatedElement(const DElement *element) const
{
    if (!element->modelUid().isValid())
        return 0;
    return m_modelController->findElement(element->modelUid());
}

bool DiagramController::isDelegatedElementOnDiagram(const DElement *element, const MDiagram *diagram) const
{
    MElement *modelElement = delegatedElement(element);
    if (!modelElement)
        return false;
    return hasDelegate(modelElement, diagram);
}

bool DiagramController::areRelationEndsOnDiagram(const DRelation *relation, const MDiagram *diagram) const
{
    return findElement(relation->endAUid(), diagram) && findElement(relation->endBUid(), diagram);
}

void DiagramController::updateAllDiagramsList()
{
    m_allDiagrams.clear();
    if (m_modelController && m_modelController->rootPackage()) {
        FindDiagramsVisitor visitor(&m_allDiagrams);
        m_modelController->rootPackage()->accept(&visitor);
    }
}

void DiagramController::verifyDiagramsIntegrity()
{
    static const bool debugDiagramsIntegrity = false;
    if (debugDiagramsIntegrity) {
        QList<MDiagram *> allDiagrams;
        if (m_modelController && m_modelController->rootPackage()) {
            FindDiagramsVisitor visitor(&allDiagrams);
            m_modelController->rootPackage()->accept(&visitor);
        }
        QMT_CHECK(allDiagrams == m_allDiagrams);
        foreach (const MDiagram *diagram, allDiagrams)
            verifyDiagramIntegrity(diagram);
    }
}

void DiagramController::verifyDiagramIntegrity(const MDiagram *diagram)
{
    QHash<Uid, const DElement *> delementsMap;
    foreach (const DElement *delement, diagram->diagramElements()) {
        delementsMap.insert(delement->uid(), delement);
        if (dynamic_cast<const DObject *>(delement) != 0 || dynamic_cast<const DRelation *>(delement) != 0) {
            QMT_CHECK(delement->modelUid().isValid());
            QMT_CHECK(m_modelController->findElement(delement->modelUid()) != 0);
            if (!delement->modelUid().isValid() || m_modelController->findElement(delement->modelUid()) == 0) {
                if (const DObject *dobject = dynamic_cast<const DObject *>(delement))
                    qWarning() << "Diagram" << diagram->name() << diagram->uid().toString() << ": object" << dobject->name() << dobject->uid().toString() << "has invalid reference to model element.";
                else if (const DRelation *drelation = dynamic_cast<const DRelation *>(delement))
                    qWarning() << "Diagram" << diagram->name() << diagram->uid().toString() << ": relation" << drelation->uid().toString() << "has invalid refeference to model element.";
            }
        } else {
            QMT_CHECK(!delement->modelUid().isValid());
        }
    }
    foreach (const DElement *delement, diagram->diagramElements()) {
        if (const DRelation *drelation = dynamic_cast<const DRelation *>(delement)) {
            QMT_CHECK(drelation->endAUid().isValid());
            QMT_CHECK(delementsMap.contains(drelation->endAUid()));
            if (!drelation->endAUid().isValid() || !delementsMap.contains(drelation->endAUid()))
                qWarning() << "Diagram" << diagram->name() << diagram->uid().toString() << ": relation" << drelation->uid().toString() << "has invalid end A.";
            QMT_CHECK(drelation->endBUid().isValid());
            QMT_CHECK(delementsMap.contains(drelation->endBUid()));
            if (!drelation->endBUid().isValid() || !delementsMap.contains(drelation->endBUid()))
                qWarning() << "Diagram" << diagram->name() << diagram->uid().toString() << ": relation" << drelation->uid().toString() << "has invalid end B.";
        }
    }
}

} // namespace qmt
