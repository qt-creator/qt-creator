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

#include "modelcontroller.h"

#include "mcontainer.h"
#include "mselection.h"
#include "mreferences.h"
#include "mclonevisitor.h"
#include "mflatassignmentvisitor.h"

#include "qmt/controller/undocommand.h"
#include "qmt/controller/undocontroller.h"

#include "qmt/model/mobject.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/mrelation.h"

#include <QDebug>

namespace qmt {

class ModelController::Clone
{
public:
    ModelController::ElementType m_elementType = ModelController::TypeUnknown;
    Uid m_elementKey;
    Uid m_ownerKey;
    int m_indexOfElement = -1;
    MElement *m_clonedElement = 0;
};

class ModelController::UpdateObjectCommand : public UndoCommand
{
public:
    UpdateObjectCommand(ModelController *modelController, MObject *object)
        : UndoCommand(tr("Change Object")),
          m_modelController(modelController)
    {
        MCloneVisitor visitor;
        object->accept(&visitor);
        m_object = dynamic_cast<MObject *>(visitor.cloned());
        QMT_CHECK(m_object);
    }

    ~UpdateObjectCommand() override
    {
        delete m_object;
    }

    bool mergeWith(const UndoCommand *other) override
    {
        auto updateCommand = dynamic_cast<const UpdateObjectCommand *>(other);
        if (!updateCommand)
            return false;
        if (m_object->uid() != updateCommand->m_object->uid())
            return false;
        // the last update is a complete update of all changes...
        return true;
    }

    void redo() override
    {
        if (canRedo()) {
            assign();
            UndoCommand::redo();
        }
    }

    void undo() override
    {
        assign();
        UndoCommand::undo();
    }

private:
    void assign()
    {
        MObject *object = m_modelController->findObject<MObject>(m_object->uid());
        QMT_CHECK(object);
        int row = 0;
        MObject *parent = object->owner();
        if (!parent) {
            QMT_CHECK(object == m_modelController->m_rootPackage);
        } else {
            row = object->owner()->children().indexOf(object);
        }
        emit m_modelController->beginUpdateObject(row, parent);
        MCloneVisitor cloneVisitor;
        object->accept(&cloneVisitor);
        auto newObject = dynamic_cast<MObject *>(cloneVisitor.cloned());
        QMT_CHECK(newObject);
        MFlatAssignmentVisitor assignVisitor(object);
        m_object->accept(&assignVisitor);
        delete m_object;
        m_object = newObject;
        emit m_modelController->endUpdateObject(row, parent);
        emit m_modelController->modified();
        m_modelController->verifyModelIntegrity();
    }

    ModelController *m_modelController = 0;
    MObject *m_object = 0;
};

class ModelController::UpdateRelationCommand :
        public UndoCommand
{
public:
    UpdateRelationCommand(ModelController *modelController, MRelation *relation)
        : UndoCommand(tr("Change Relation")),
          m_modelController(modelController)
    {
        MCloneVisitor visitor;
        relation->accept(&visitor);
        m_relation = dynamic_cast<MRelation *>(visitor.cloned());
        QMT_CHECK(m_relation);
    }

    ~UpdateRelationCommand() override
    {
        delete m_relation;
    }

    bool mergeWith(const UndoCommand *other) override
    {
        auto updateCommand = dynamic_cast<const UpdateRelationCommand *>(other);
        if (!updateCommand)
            return false;
        if (m_relation->uid() != updateCommand->m_relation->uid())
            return false;
        // the last update is a complete update of all changes...
        return true;
    }

    void redo() override
    {
        if (canRedo()) {
            assign();
            UndoCommand::redo();
        }
    }

    void undo() override
    {
        assign();
        UndoCommand::undo();
    }

private:
    void assign()
    {
        MRelation *relation = m_modelController->findRelation<MRelation>(m_relation->uid());
        QMT_CHECK(relation);
        MObject *owner = relation->owner();
        QMT_CHECK(owner);
        int row = owner->relations().indexOf(relation);
        emit m_modelController->beginUpdateRelation(row, owner);
        MCloneVisitor cloneVisitor;
        relation->accept(&cloneVisitor);
        auto newRelation = dynamic_cast<MRelation *>(cloneVisitor.cloned());
        QMT_CHECK(newRelation);
        MFlatAssignmentVisitor assignVisitor(relation);
        m_relation->accept(&assignVisitor);
        delete m_relation;
        m_relation = newRelation;
        emit m_modelController->endUpdateRelation(row, owner);
        emit m_modelController->modified();
        m_modelController->verifyModelIntegrity();
    }

    ModelController *m_modelController = 0;
    MRelation *m_relation = 0;
};

class ModelController::AddElementsCommand : public UndoCommand
{
public:
    AddElementsCommand(ModelController *modelController, const QString &commandLabel)
        : UndoCommand(commandLabel),
          m_modelController(modelController)
    {
    }

    ~AddElementsCommand() override
    {
        foreach (const Clone &clone, m_clonedElements)
            delete clone.m_clonedElement;
    }

    void add(ElementType elementsType, const Uid &objectKey, const Uid &ownerKey)
    {
        Clone clone;
        clone.m_elementType = elementsType;
        clone.m_elementKey = objectKey;
        clone.m_ownerKey = ownerKey;
        clone.m_indexOfElement = -1;
        m_clonedElements.append(clone);
    }

    void redo() override
    {
        if (canRedo()) {
            bool inserted = false;
            for (int i = m_clonedElements.count() - 1; i >= 0; --i) {
                Clone &clone = m_clonedElements[i];
                QMT_CHECK(clone.m_clonedElement);
                QMT_CHECK(clone.m_clonedElement->uid() == clone.m_elementKey);
                MObject *owner = m_modelController->findObject<MObject>(clone.m_ownerKey);
                QMT_CHECK(owner);
                QMT_CHECK(clone.m_indexOfElement >= 0);
                switch (clone.m_elementType) {
                case TypeObject:
                {
                    emit m_modelController->beginInsertObject(clone.m_indexOfElement, owner);
                    auto object = dynamic_cast<MObject *>(clone.m_clonedElement);
                    QMT_CHECK(object);
                    m_modelController->mapObject(object);
                    owner->insertChild(clone.m_indexOfElement, object);
                    clone.m_clonedElement = 0;
                    emit m_modelController->endInsertObject(clone.m_indexOfElement, owner);
                    inserted = true;
                    break;
                }
                case TypeRelation:
                {
                    emit m_modelController->beginInsertRelation(clone.m_indexOfElement, owner);
                    auto relation = dynamic_cast<MRelation *>(clone.m_clonedElement);
                    QMT_CHECK(relation);
                    m_modelController->mapRelation(relation);
                    owner->insertRelation(clone.m_indexOfElement, relation);
                    clone.m_clonedElement = 0;
                    emit m_modelController->endInsertRelation(clone.m_indexOfElement, owner);
                    inserted = true;
                    break;
                }
                default:
                    QMT_CHECK(false);
                    break;
                }
            }
            if (inserted)
                emit m_modelController->modified();
            m_modelController->verifyModelIntegrity();
            UndoCommand::redo();
        }
    }

    void undo() override
    {
        bool removed = false;
        for (int i = 0; i < m_clonedElements.count(); ++i) {
            Clone &clone = m_clonedElements[i];
            QMT_CHECK(!clone.m_clonedElement);
            MObject *owner = m_modelController->findObject<MObject>(clone.m_ownerKey);
            QMT_CHECK(owner);
            switch (clone.m_elementType) {
            case TypeObject:
            {
                MObject *object = m_modelController->findObject<MObject>(clone.m_elementKey);
                QMT_CHECK(object);
                clone.m_indexOfElement = owner->children().indexOf(object);
                QMT_CHECK(clone.m_indexOfElement >= 0);
                emit m_modelController->beginRemoveObject(clone.m_indexOfElement, owner);
                MCloneDeepVisitor visitor;
                object->accept(&visitor);
                clone.m_clonedElement = visitor.cloned();
                m_modelController->unmapObject(object);
                owner->removeChild(object);
                emit m_modelController->endRemoveObject(clone.m_indexOfElement, owner);
                removed = true;
                break;
            }
            case TypeRelation:
            {
                MRelation *relation = m_modelController->findRelation<MRelation>(clone.m_elementKey);
                QMT_CHECK(relation);
                clone.m_indexOfElement = owner->relations().indexOf(relation);
                QMT_CHECK(clone.m_indexOfElement >= 0);
                emit m_modelController->beginRemoveRelation(clone.m_indexOfElement, owner);
                MCloneDeepVisitor visitor;
                relation->accept(&visitor);
                clone.m_clonedElement = visitor.cloned();
                m_modelController->unmapRelation(relation);
                owner->removeRelation(relation);
                emit m_modelController->endRemoveRelation(clone.m_indexOfElement, owner);
                removed = true;
                break;
            }
            default:
                QMT_CHECK(false);
                break;
            }
        }
        if (removed)
            emit m_modelController->modified();
        m_modelController->verifyModelIntegrity();
        UndoCommand::undo();
    }

private:
    ModelController *m_modelController = 0;
    QList<ModelController::Clone> m_clonedElements;
};

class ModelController::RemoveElementsCommand : public UndoCommand
{
public:
    RemoveElementsCommand(ModelController *modelController, const QString &commandLabel)
        : UndoCommand(commandLabel),
          m_modelController(modelController)
    {
    }

    ~RemoveElementsCommand() override
    {
        foreach (const Clone &clone, m_clonedElements)
            delete clone.m_clonedElement;
    }

    void add(MElement *element, MObject *owner)
    {
        ModelController::Clone clone;

        clone.m_elementKey = element->uid();
        clone.m_ownerKey = owner->uid();
        if (auto object = dynamic_cast<MObject *>(element)) {
            clone.m_elementType = TypeObject;
            clone.m_indexOfElement = owner->children().indexOf(object);
            QMT_CHECK(clone.m_indexOfElement >= 0);
        } else if (auto relation = dynamic_cast<MRelation *>(element)) {
            clone.m_elementType = TypeRelation;
            clone.m_indexOfElement = owner->relations().indexOf(relation);
            QMT_CHECK(clone.m_indexOfElement >= 0);
        } else {
            QMT_CHECK(false);
        }
        MCloneDeepVisitor visitor;
        element->accept(&visitor);
        clone.m_clonedElement = visitor.cloned();
        QMT_CHECK(clone.m_clonedElement);
        m_clonedElements.append(clone);
    }

    void redo() override
    {
        if (canRedo()) {
            bool removed = false;
            for (int i = 0; i < m_clonedElements.count(); ++i) {
                Clone &clone = m_clonedElements[i];
                QMT_CHECK(!clone.m_clonedElement);
                MObject *owner = m_modelController->findObject<MObject>(clone.m_ownerKey);
                QMT_CHECK(owner);
                switch (clone.m_elementType) {
                case TypeObject:
                {
                    MObject *object = m_modelController->findObject<MObject>(clone.m_elementKey);
                    QMT_CHECK(object);
                    clone.m_indexOfElement = owner->children().indexOf(object);
                    QMT_CHECK(clone.m_indexOfElement >= 0);
                    emit m_modelController->beginRemoveObject(clone.m_indexOfElement, owner);
                    MCloneDeepVisitor visitor;
                    object->accept(&visitor);
                    clone.m_clonedElement = visitor.cloned();
                    m_modelController->unmapObject(object);
                    owner->removeChild(object);
                    emit m_modelController->endRemoveObject(clone.m_indexOfElement, owner);
                    removed = true;
                    break;
                }
                case TypeRelation:
                {
                    MRelation *relation = m_modelController->findRelation<MRelation>(clone.m_elementKey);
                    QMT_CHECK(relation);
                    clone.m_indexOfElement = owner->relations().indexOf(relation);
                    QMT_CHECK(clone.m_indexOfElement >= 0);
                    emit m_modelController->beginRemoveRelation(clone.m_indexOfElement, owner);
                    MCloneDeepVisitor visitor;
                    relation->accept(&visitor);
                    clone.m_clonedElement = visitor.cloned();
                    m_modelController->unmapRelation(relation);
                    owner->removeRelation(relation);
                    emit m_modelController->endRemoveRelation(clone.m_indexOfElement, owner);
                    removed = true;
                    break;
                }
                default:
                    QMT_CHECK(false);
                    break;
                }
            }
            if (removed)
                emit m_modelController->modified();
            m_modelController->verifyModelIntegrity();
            UndoCommand::redo();
        }
    }

    void undo() override
    {
        bool inserted = false;
        for (int i = m_clonedElements.count() - 1; i >= 0; --i) {
            Clone &clone = m_clonedElements[i];
            QMT_CHECK(clone.m_clonedElement);
            MObject *owner = m_modelController->findObject<MObject>(clone.m_ownerKey);
            QMT_CHECK(owner);
            QMT_CHECK(clone.m_indexOfElement >= 0);
            switch (clone.m_elementType) {
            case TypeObject:
            {
                emit m_modelController->beginInsertObject(clone.m_indexOfElement, owner);
                auto object = dynamic_cast<MObject *>(clone.m_clonedElement);
                QMT_CHECK(object);
                m_modelController->mapObject(object);
                owner->insertChild(clone.m_indexOfElement, object);
                clone.m_clonedElement = 0;
                emit m_modelController->endInsertObject(clone.m_indexOfElement, owner);
                inserted = true;
                break;
            }
            case TypeRelation:
            {
                emit m_modelController->beginInsertRelation(clone.m_indexOfElement, owner);
                auto relation = dynamic_cast<MRelation *>(clone.m_clonedElement);
                QMT_CHECK(relation);
                m_modelController->mapRelation(relation);
                owner->insertRelation(clone.m_indexOfElement, relation);
                clone.m_clonedElement = 0;
                emit m_modelController->endInsertRelation(clone.m_indexOfElement, owner);
                inserted = true;
                break;
            }
            default:
                QMT_CHECK(false);
                break;
            }
        }
        if (inserted)
            emit m_modelController->modified();
        m_modelController->verifyModelIntegrity();
        UndoCommand::undo();
    }

private:
    ModelController *m_modelController = 0;
    QList<ModelController::Clone> m_clonedElements;
};

class ModelController::MoveObjectCommand : public UndoCommand
{
public:
    MoveObjectCommand(ModelController *modelController, MObject *object)
        : UndoCommand(tr("Move Object")),
          m_modelController(modelController),
          m_objectKey(object->uid()),
          m_ownerKey(object->owner()->uid()),
          m_indexOfElement(object->owner()->children().indexOf(object))
    {
    }

    ~MoveObjectCommand() override
    {
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
        MObject *object = m_modelController->findObject(m_objectKey);
        QMT_CHECK(object);
        MObject *formerOwner = object->owner();
        int formerRow = formerOwner->children().indexOf(object);
        emit m_modelController->beginMoveObject(formerRow, formerOwner);
        formerOwner->decontrolChild(object);
        MObject *newOwner = m_modelController->findObject(m_ownerKey);
        newOwner->insertChild(m_indexOfElement, object);
        int newRow = m_indexOfElement;
        m_ownerKey = formerOwner->uid();
        m_indexOfElement = formerRow;
        emit m_modelController->endMoveObject(newRow, newOwner);
        emit m_modelController->modified();
        m_modelController->verifyModelIntegrity();
    }

    ModelController *m_modelController = 0;
    Uid m_objectKey;
    Uid m_ownerKey;
    int m_indexOfElement = -1;
};

class ModelController::MoveRelationCommand : public UndoCommand
{
public:
    MoveRelationCommand(ModelController *modelController, MRelation *relation)
        : UndoCommand(tr("Move Relation")),
          m_modelController(modelController),
          m_relationKey(relation->uid()),
          m_ownerKey(relation->owner()->uid()),
          m_indexOfElement(relation->owner()->relations().indexOf(relation))
    {
    }

    ~MoveRelationCommand() override
    {
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
        MRelation *relation = m_modelController->findRelation(m_relationKey);
        QMT_CHECK(relation);
        MObject *formerOwner = relation->owner();
        int formerRow = formerOwner->relations().indexOf(relation);
        emit m_modelController->beginMoveRelation(formerRow, formerOwner);
        formerOwner->decontrolRelation(relation);
        MObject *newOwner = m_modelController->findObject(m_ownerKey);
        newOwner->insertRelation(m_indexOfElement, relation);
        int newRow = m_indexOfElement;
        m_ownerKey = formerOwner->uid();
        m_indexOfElement = formerRow;
        emit m_modelController->endMoveRelation(newRow, newOwner);
        emit m_modelController->modified();
        m_modelController->verifyModelIntegrity();
    }

    ModelController *m_modelController = 0;
    Uid m_relationKey;
    Uid m_ownerKey;
    int m_indexOfElement = -1;
};

ModelController::ModelController(QObject *parent)
    : QObject(parent),
      m_rootPackage(0),
      m_undoController(0),
      m_isResettingModel(false)
{
}

ModelController::~ModelController()
{
    delete m_rootPackage;
}

void ModelController::setRootPackage(MPackage *rootPackage)
{
    startResetModel();
    unmapObject(m_rootPackage);
    m_rootPackage = rootPackage;
    mapObject(m_rootPackage);
    finishResetModel(false);
}

void ModelController::setUndoController(UndoController *undoController)
{
    m_undoController = undoController;
}

Uid ModelController::ownerKey(const MElement *element) const
{
    QMT_CHECK(element);
    MObject *owner = element->owner();
    if (!owner)
        return Uid();
    return owner->uid();
}

MElement *ModelController::findElement(const Uid &key)
{
    if (MObject *object = findObject(key))
        return object;
    else if (MRelation *relation = findRelation(key))
        return relation;
    return 0;
}

void ModelController::startResetModel()
{
    QMT_CHECK(!m_isResettingModel);
    m_isResettingModel = true;
    emit beginResetModel();
    QMT_CHECK(m_isResettingModel);
}

void ModelController::finishResetModel(bool modified)
{
    QMT_CHECK(m_isResettingModel);
    emit endResetModel();
    if (modified)
        emit this->modified();
    QMT_CHECK(m_isResettingModel);
    m_isResettingModel = false;
}

MObject *ModelController::object(int row, const MObject *owner) const
{
    if (!owner) {
        QMT_CHECK(row == 0);
        return m_rootPackage;
    }
    QMT_CHECK(row >= 0 && row < owner->children().size());
    return owner->children().at(row);
}

MObject *ModelController::findObject(const Uid &key) const
{
    return m_objectsMap.value(key);
}

void ModelController::addObject(MPackage *parentPackage, MObject *object)
{
    QMT_CHECK(parentPackage);
    QMT_CHECK(object);
    int row = parentPackage->children().size();
    if (!m_isResettingModel)
        emit beginInsertObject(row, parentPackage);
    mapObject(object);
    if (m_undoController) {
        auto undoCommand = new AddElementsCommand(this, tr("Add Object"));
        m_undoController->push(undoCommand);
        undoCommand->add(TypeObject, object->uid(), parentPackage->uid());
    }
    parentPackage->addChild(object);
    if (!m_isResettingModel) {
        emit endInsertObject(row, parentPackage);
        emit modified();
    }
    verifyModelIntegrity();
}

void ModelController::removeObject(MObject *object)
{
    QMT_CHECK(object);
    if (m_undoController)
        m_undoController->beginMergeSequence(tr("Delete Object"));
    removeRelatedRelations(object);
    // remove object
    QMT_CHECK(object->owner());
    int row = object->owner()->children().indexOf(object);
    MObject *owner = object->owner();
    if (!m_isResettingModel)
        emit beginRemoveObject(row, owner);
    if (m_undoController) {
        auto undoCommand = new RemoveElementsCommand(this, tr("Delete Object"));
        m_undoController->push(undoCommand);
        undoCommand->add(object, object->owner());
    }
    unmapObject(object);
    owner->removeChild(object);
    if (!m_isResettingModel) {
        emit endRemoveObject(row, owner);
        emit modified();
    }
    if (m_undoController)
        m_undoController->endMergeSequence();
    verifyModelIntegrity();
}

void ModelController::startUpdateObject(MObject *object)
{
    QMT_CHECK(object);
    int row = 0;
    MObject *parent = object->owner();
    if (!parent) {
        QMT_CHECK(object == m_rootPackage);
    } else {
        row = parent->children().indexOf(object);
    }
    if (auto package = dynamic_cast<MPackage *>(object))
        m_oldPackageName = package->name();
    if (!m_isResettingModel)
        emit beginUpdateObject(row, parent);
    if (m_undoController)
        m_undoController->push(new UpdateObjectCommand(this, object));
}

void ModelController::finishUpdateObject(MObject *object, bool cancelled)
{
    QMT_CHECK(object);

    int row = 0;
    MObject *parent = object->owner();
    if (!parent) {
        QMT_CHECK(object == m_rootPackage);
    } else {
        row = parent->children().indexOf(object);
    }
    if (!m_isResettingModel) {
        emit endUpdateObject(row, parent);
        if (!cancelled) {
            QList<MRelation *> relations = findRelationsOfObject(object);
            foreach (MRelation *relation, relations)
                emit relationEndChanged(relation, object);
            if (auto package = dynamic_cast<MPackage *>(object)) {
                if (m_oldPackageName != package->name())
                    emit packageNameChanged(package, m_oldPackageName);
            }
            emit modified();
        }
    }
    verifyModelIntegrity();
}

void ModelController::moveObject(MPackage *newOwner, MObject *object)
{
    QMT_CHECK(newOwner);
    QMT_CHECK(object);
    QMT_CHECK(object != m_rootPackage);

    if (newOwner != object->owner()) {
        int formerRow = 0;
        MObject *formerOwner = object->owner();
        QMT_CHECK(formerOwner);
        formerRow = formerOwner->children().indexOf(object);
        if (!m_isResettingModel)
            emit beginMoveObject(formerRow, formerOwner);
        if (m_undoController) {
            auto undoCommand = new MoveObjectCommand(this, object);
            m_undoController->push(undoCommand);
        }
        formerOwner->decontrolChild(object);
        newOwner->addChild(object);
        int row = newOwner->children().indexOf(object);
        if (!m_isResettingModel) {
            emit endMoveObject(row, newOwner);
            emit modified();
        }
    }
    verifyModelIntegrity();
}

MRelation *ModelController::findRelation(const Uid &key) const
{
    return m_relationsMap.value(key);
}

void ModelController::addRelation(MObject *owner, MRelation *relation)
{
    QMT_CHECK(owner);
    QMT_CHECK(relation);
    QMT_CHECK(findObject(relation->endAUid()));
    QMT_CHECK(findObject(relation->endBUid()));

    int row = owner->relations().size();
    if (!m_isResettingModel)
        emit beginInsertRelation(row, owner);
    mapRelation(relation);
    if (m_undoController) {
        auto undoCommand = new AddElementsCommand(this, tr("Add Relation"));
        m_undoController->push(undoCommand);
        undoCommand->add(TypeRelation, relation->uid(), owner->uid());
    }
    owner->addRelation(relation);
    if (!m_isResettingModel) {
        emit endInsertRelation(row, owner);
        emit modified();
    }
    verifyModelIntegrity();
}

void ModelController::removeRelation(MRelation *relation)
{
    QMT_CHECK(relation);
    MObject *owner = relation->owner();
    QMT_CHECK(owner);
    int row = owner->relations().indexOf(relation);
    if (!m_isResettingModel)
        emit beginRemoveRelation(row, owner);
    if (m_undoController) {
        auto undoCommand = new RemoveElementsCommand(this, tr("Delete Relation"));
        m_undoController->push(undoCommand);
        undoCommand->add(relation, owner);
    }
    unmapRelation(relation);
    owner->removeRelation(relation);
    if (!m_isResettingModel) {
        emit endRemoveRelation(row, owner);
        emit modified();
    }
    verifyModelIntegrity();
}

void ModelController::startUpdateRelation(MRelation *relation)
{
    QMT_CHECK(relation);
    MObject *owner = relation->owner();
    QMT_CHECK(owner);
    if (!m_isResettingModel)
        emit beginUpdateRelation(owner->relations().indexOf(relation), owner);
    if (m_undoController)
        m_undoController->push(new UpdateRelationCommand(this, relation));
}

void ModelController::finishUpdateRelation(MRelation *relation, bool cancelled)
{
    QMT_CHECK(relation);
    QMT_CHECK(findObject(relation->endAUid()));
    QMT_CHECK(findObject(relation->endBUid()));
    MObject *owner = relation->owner();
    QMT_CHECK(owner);
    if (!m_isResettingModel) {
        emit endUpdateRelation(owner->relations().indexOf(relation), owner);
        if (!cancelled)
            emit modified();
    }
    verifyModelIntegrity();
}

void ModelController::moveRelation(MObject *newOwner, MRelation *relation)
{
    QMT_CHECK(newOwner);
    QMT_CHECK(relation);

    if (newOwner != relation->owner()) {
        int formerRow = 0;
        MObject *formerOwner = relation->owner();
        QMT_CHECK(formerOwner);
        formerRow = formerOwner->relations().indexOf(relation);
        if (!m_isResettingModel)
            emit beginMoveRelation(formerRow, formerOwner);
        if (m_undoController) {
            auto undoCommand = new MoveRelationCommand(this, relation);
            m_undoController->push(undoCommand);
        }
        formerOwner->decontrolRelation(relation);
        newOwner->addRelation(relation);
        int row = newOwner->relations().indexOf(relation);
        if (!m_isResettingModel) {
            emit endMoveRelation(row, newOwner);
            emit modified();
        }
    }
    verifyModelIntegrity();
}

QList<MRelation *> ModelController::findRelationsOfObject(const MObject *object) const
{
    QMT_CHECK(object);
    return m_objectRelationsMap.values(object->uid());
}

MContainer ModelController::cutElements(const MSelection &modelSelection)
{
    // PERFORM avoid duplicate call of simplify(modelSelection)
    MContainer copiedElements = copyElements(modelSelection);
    deleteElements(modelSelection, tr("Cut"));
    return copiedElements;
}

MContainer ModelController::copyElements(const MSelection &modelSelection)
{
    MReferences simplifiedSelection = simplify(modelSelection);
    MContainer copiedElements;
    foreach (MElement *element, simplifiedSelection.elements()) {
        MCloneDeepVisitor visitor;
        element->accept(&visitor);
        MElement *clonedElement = visitor.cloned();
        copiedElements.submit(clonedElement);
    }
    return copiedElements;
}

void ModelController::pasteElements(MObject *owner, const MContainer &modelContainer)
{
    // clone all elements and renew their keys
    QHash<Uid, Uid> renewedKeys;
    QList<MElement *> clonedElements;
    foreach (MElement *element, modelContainer.elements()) {
        MCloneDeepVisitor visitor;
        element->accept(&visitor);
        MElement *clonedElement = visitor.cloned();
        renewElementKey(clonedElement, &renewedKeys);
        clonedElements.append(clonedElement);
    }
    // fix all keys referencing between pasting elements
    foreach (MElement *clonedElement, clonedElements)
        updateRelationKeys(clonedElement, renewedKeys);
    if (m_undoController)
        m_undoController->beginMergeSequence(tr("Paste"));
    // insert all elements
    bool added = false;
    foreach (MElement *clonedElement, clonedElements) {
        if (auto object = dynamic_cast<MObject *>(clonedElement)) {
            MObject *objectOwner = owner;
            if (!dynamic_cast<MPackage*>(owner))
                objectOwner = owner->owner();
            QMT_CHECK(dynamic_cast<MPackage*>(objectOwner));
            int row = objectOwner->children().size();
            emit beginInsertObject(row, objectOwner);
            mapObject(object);
            if (m_undoController) {
                auto undoCommand = new AddElementsCommand(this, tr("Paste"));
                m_undoController->push(undoCommand);
                undoCommand->add(TypeObject, object->uid(), objectOwner->uid());
            }
            objectOwner->insertChild(row, object);
            emit endInsertObject(row, objectOwner);
            added = true;
        } else if (auto relation = dynamic_cast<MRelation *>(clonedElement)) {
            int row = owner->relations().size();
            emit beginInsertRelation(row, owner);
            mapRelation(relation);
            if (m_undoController) {
                auto undoCommand = new AddElementsCommand(this, tr("Paste"));
                m_undoController->push(undoCommand);
                undoCommand->add(TypeRelation, relation->uid(), owner->uid());
            }
            owner->addRelation(relation);
            emit endInsertRelation(row, owner);
            added = true;
        }
    }
    if (added)
        emit modified();
    verifyModelIntegrity();
    if (m_undoController)
        m_undoController->endMergeSequence();
}

void ModelController::deleteElements(const MSelection &modelSelection)
{
    deleteElements(modelSelection, tr("Delete"));
}

void ModelController::deleteElements(const MSelection &modelSelection, const QString &commandLabel)
{
    MReferences simplifiedSelection = simplify(modelSelection);
    if (simplifiedSelection.elements().isEmpty())
        return;
    if (m_undoController)
        m_undoController->beginMergeSequence(commandLabel);
    bool removed = false;
    foreach (MElement *element, simplifiedSelection.elements()) {
        // element may have been deleted indirectly by predecessor element in loop
        if ((element = findElement(element->uid()))) {
            if (auto object = dynamic_cast<MObject *>(element)) {
                removeRelatedRelations(object);
                MObject *owner = object->owner();
                int row = owner->children().indexOf(object);
                emit beginRemoveObject(row, owner);
                if (m_undoController) {
                    auto cutCommand = new RemoveElementsCommand(this, commandLabel);
                    m_undoController->push(cutCommand);
                    cutCommand->add(element, owner);
                }
                unmapObject(object);
                owner->removeChild(object);
                emit endRemoveObject(row, owner);
                removed = true;
            } else if (auto relation = dynamic_cast<MRelation *>(element)) {
                MObject *owner = relation->owner();
                int row = owner->relations().indexOf(relation);
                emit beginRemoveRelation(row, owner);
                if (m_undoController) {
                    auto cutCommand = new RemoveElementsCommand(this, commandLabel);
                    m_undoController->push(cutCommand);
                    cutCommand->add(element, owner);
                }
                unmapRelation(relation);
                owner->removeRelation(relation);
                emit endRemoveRelation(row, owner);
                removed = true;
            } else {
                QMT_CHECK(false);
            }
        }
    }
    if (removed)
        emit modified();
    verifyModelIntegrity();
    if (m_undoController)
        m_undoController->endMergeSequence();
}

void ModelController::removeRelatedRelations(MObject *object)
{
    foreach (MRelation *relation, m_objectRelationsMap.values(object->uid()))
        removeRelation(relation);
    QMT_CHECK(m_objectRelationsMap.values(object->uid()).isEmpty());
}

void ModelController::renewElementKey(MElement *element, QHash<Uid, Uid> *renewedKeys)
{
    if (element) {
        MElement *otherElement = findObject(element->uid());
        if (otherElement) {
            QMT_CHECK(otherElement != element);
        }
        if (m_objectsMap.contains(element->uid()) || m_relationsMap.contains(element->uid())) {
            Uid oldKey = element->uid();
            element->renewUid();
            Uid newKey = element->uid();
            renewedKeys->insert(oldKey, newKey);
        }
        auto object = dynamic_cast<MObject *>(element);
        if (object) {
            foreach (const Handle<MObject> &child, object->children())
                renewElementKey(child.target(), renewedKeys);
            foreach (const Handle<MRelation> &relation, object->relations())
                renewElementKey(relation.target(), renewedKeys);
        }
    }
}

void ModelController::updateRelationKeys(MElement *element, const QHash<Uid, Uid> &renewedKeys)
{
    if (auto object = dynamic_cast<MObject *>(element)) {
        foreach (const Handle<MRelation> &handle, object->relations())
            updateRelationEndKeys(handle.target(), renewedKeys);
        foreach (const Handle<MObject> &child, object->children())
            updateRelationKeys(child.target(), renewedKeys);
    } else if (auto relation = dynamic_cast<MRelation *>(element)) {
        updateRelationEndKeys(relation, renewedKeys);
    }
}

void ModelController::updateRelationEndKeys(MRelation *relation, const QHash<Uid, Uid> &renewedKeys)
{
    if (relation) {
        Uid newEndAKey = renewedKeys.value(relation->endAUid(), Uid::invalidUid());
        if (newEndAKey.isValid())
            relation->setEndAUid(newEndAKey);
        Uid newEndBKey = renewedKeys.value(relation->endBUid(), Uid::invalidUid());
        if (newEndBKey.isValid())
            relation->setEndBUid(newEndBKey);
    }
}

void ModelController::mapObject(MObject *object)
{
    if (object) {
        QMT_CHECK(!m_objectsMap.contains(object->uid()));
        m_objectsMap.insert(object->uid(), object);
        foreach (const Handle<MObject> &child, object->children())
            mapObject(child.target());
        foreach (const Handle<MRelation> &relation, object->relations())
            mapRelation(relation.target());
    }
}

void ModelController::unmapObject(MObject *object)
{
    if (object) {
        QMT_CHECK(m_objectsMap.contains(object->uid()));
        foreach (const Handle<MRelation> &relation, object->relations())
            unmapRelation(relation.target());
        foreach (const Handle<MObject> &child, object->children())
            unmapObject(child.target());
        m_objectsMap.remove(object->uid());
    }
}

void ModelController::mapRelation(MRelation *relation)
{
    if (relation) {
        QMT_CHECK(!m_relationsMap.contains(relation->uid()));
        m_relationsMap.insert(relation->uid(), relation);
        QMT_CHECK(!m_objectRelationsMap.contains(relation->endAUid(), relation));
        m_objectRelationsMap.insert(relation->endAUid(), relation);
        if (relation->endAUid() != relation->endBUid()) {
            QMT_CHECK(!m_objectRelationsMap.contains(relation->endBUid(), relation));
            m_objectRelationsMap.insert(relation->endBUid(), relation);
        }
    }
}

void ModelController::unmapRelation(MRelation *relation)
{
    if (relation) {
        QMT_CHECK(m_relationsMap.contains(relation->uid()));
        m_relationsMap.remove(relation->uid());
        QMT_CHECK(m_objectRelationsMap.contains(relation->endAUid(), relation));
        m_objectRelationsMap.remove(relation->endAUid(), relation);
        if (relation->endAUid() != relation->endBUid()) {
            QMT_CHECK(m_objectRelationsMap.contains(relation->endBUid(), relation));
            m_objectRelationsMap.remove(relation->endBUid(), relation);
        }
    }
}

MReferences ModelController::simplify(const MSelection &modelSelection)
{
    // PERFORM improve performance by using a set of Uid build from modelSelection
    MReferences references;
    foreach (const MSelection::Index &index, modelSelection.indices()) {
        MElement *element = findElement(index.elementKey());
        QMT_CHECK(element);
        // if any (grand-)parent of element is in modelSelection then ignore element
        bool ignore = false;
        MObject *owner = element->owner();
        while (owner) {
            Uid ownerKey = owner->uid();
            foreach (const MSelection::Index &index, modelSelection.indices()) {
                if (index.elementKey() == ownerKey) {
                    ignore = true;
                    break;
                }
            }
            if (ignore)
                break;
            owner = owner->owner();
        }
        if (!ignore)
            references.append(element);
    }
    return references;
}

void ModelController::verifyModelIntegrity() const
{
    static const bool debugModelIntegrity = false;
    if (debugModelIntegrity) {
        QMT_CHECK(m_rootPackage);

        QHash<Uid, const MObject *> objectsMap;
        QHash<Uid, const MRelation *> relationsMap;
        QMultiHash<Uid, MRelation *> objectRelationsMap;
        verifyModelIntegrity(m_rootPackage, &objectsMap, &relationsMap, &objectRelationsMap);

        QMT_CHECK(objectsMap.size() == m_objectsMap.size());
        foreach (const MObject *object, m_objectsMap) {
            QMT_CHECK(object);
            QMT_CHECK(m_objectsMap.contains(object->uid()));
            QMT_CHECK(objectsMap.contains(object->uid()));
        }
        QMT_CHECK(relationsMap.size() == m_relationsMap.size());
        foreach (const MRelation *relation, m_relationsMap) {
            QMT_CHECK(relation);
            QMT_CHECK(m_relationsMap.contains(relation->uid()));
            QMT_CHECK(relationsMap.contains(relation->uid()));
        }
        QMT_CHECK(objectRelationsMap.size() == m_objectRelationsMap.size());
        for (auto it = m_objectRelationsMap.cbegin(); it != m_objectRelationsMap.cend(); ++it) {
            QMT_CHECK(objectRelationsMap.contains(it.key(), it.value()));
        }
    }
}

void ModelController::verifyModelIntegrity(const MObject *object, QHash<Uid, const MObject *> *objectsMap,
                                           QHash<Uid, const MRelation *> *relationsMap,
                                           QMultiHash<Uid, MRelation *> *objectRelationsMap) const
{
    QMT_CHECK(object);
    QMT_CHECK(!objectsMap->contains(object->uid()));
    objectsMap->insert(object->uid(), object);
    foreach (const Handle<MRelation> &handle, object->relations()) {
        MRelation *relation = handle.target();
        if (relation) {
            QMT_CHECK(!relationsMap->contains(relation->uid()));
            relationsMap->insert(relation->uid(), relation);
            QMT_CHECK(findObject(relation->endAUid()));
            QMT_CHECK(findObject(relation->endBUid()));
            QMT_CHECK(!objectRelationsMap->contains(relation->endAUid(), relation));
            objectRelationsMap->insert(relation->endAUid(), relation);
            QMT_CHECK(!objectRelationsMap->contains(relation->endBUid(), relation));
            objectRelationsMap->insert(relation->endBUid(), relation);
        }
    }
    foreach (const Handle<MObject> &handle, object->children()) {
        MObject *childObject = handle.target();
        if (childObject)
            verifyModelIntegrity(childObject, objectsMap, relationsMap, objectRelationsMap);
    }
}

} // namespace qmt
