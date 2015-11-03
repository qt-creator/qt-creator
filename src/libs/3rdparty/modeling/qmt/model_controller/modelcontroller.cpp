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

struct ModelController::Clone {
    Clone();

    ElementType m_elementType;
    Uid m_elementKey;
    Uid m_ownerKey;
    int m_indexOfElement;
    MElement *m_clonedElement;
};

ModelController::Clone::Clone()
    : m_elementType(TYPE_UNKNOWN),
      m_indexOfElement(-1),
      m_clonedElement(0)
{
}


class ModelController::UpdateObjectCommand :
        public UndoCommand
{
public:
    UpdateObjectCommand(ModelController *model_controller, MObject *object)
        : UndoCommand(tr("Change Object")),
          m_modelController(model_controller),
          m_object(0)
    {
        MCloneVisitor visitor;
        object->accept(&visitor);
        m_object = dynamic_cast<MObject *>(visitor.getCloned());
        QMT_CHECK(m_object);
    }

    ~UpdateObjectCommand()
    {
        delete m_object;
    }

    bool mergeWith(const UndoCommand *other)
    {
        const UpdateObjectCommand *update_command = dynamic_cast<const UpdateObjectCommand *>(other);
        if (!update_command) {
            return false;
        }
        if (m_object->getUid() != update_command->m_object->getUid()) {
            return false;
        }
        // the last update is a complete update of all changes...
        return true;
    }

    void redo()
    {
        if (canRedo()) {
            assign();
            UndoCommand::redo();
        }
    }

    void undo()
    {
        assign();
        UndoCommand::undo();
    }

private:
    void assign()
    {
        MObject *object = m_modelController->findObject<MObject>(m_object->getUid());
        QMT_CHECK(object);
        int row = 0;
        MObject *parent = object->getOwner();
        if (!parent) {
            QMT_CHECK(object == m_modelController->m_rootPackage);
        } else {
            row = object->getOwner()->getChildren().indexOf(object);
        }
        emit m_modelController->beginUpdateObject(row, parent);
        MCloneVisitor clone_visitor;
        object->accept(&clone_visitor);
        MObject *new_object = dynamic_cast<MObject *>(clone_visitor.getCloned());
        QMT_CHECK(new_object);
        MFlatAssignmentVisitor assign_visitor(object);
        m_object->accept(&assign_visitor);
        delete m_object;
        m_object = new_object;
        emit m_modelController->endUpdateObject(row, parent);
        emit m_modelController->modified();
        m_modelController->verifyModelIntegrity();
    }

private:

    ModelController *m_modelController;

    MObject *m_object;
};


class ModelController::UpdateRelationCommand :
        public UndoCommand
{
public:
    UpdateRelationCommand(ModelController *model_controller, MRelation *relation)
        : UndoCommand(tr("Change Relation")),
          m_modelController(model_controller),
          m_relation(0)
    {
        MCloneVisitor visitor;
        relation->accept(&visitor);
        m_relation = dynamic_cast<MRelation *>(visitor.getCloned());
        QMT_CHECK(m_relation);
    }

    ~UpdateRelationCommand()
    {
        delete m_relation;
    }

    bool mergeWith(const UndoCommand *other)
    {
        const UpdateRelationCommand *update_command = dynamic_cast<const UpdateRelationCommand *>(other);
        if (!update_command) {
            return false;
        }
        if (m_relation->getUid() != update_command->m_relation->getUid()) {
            return false;
        }
        // the last update is a complete update of all changes...
        return true;
    }

    void redo()
    {
        if (canRedo()) {
            assign();
            UndoCommand::redo();
        }
    }

    void undo()
    {
        assign();
        UndoCommand::undo();
    }

private:
    void assign()
    {
        MRelation *relation = m_modelController->findRelation<MRelation>(m_relation->getUid());
        QMT_CHECK(relation);
        MObject *owner = relation->getOwner();
        QMT_CHECK(owner);
        int row = owner->getRelations().indexOf(relation);
        emit m_modelController->beginUpdateRelation(row, owner);
        MCloneVisitor clone_visitor;
        relation->accept(&clone_visitor);
        MRelation *new_relation = dynamic_cast<MRelation *>(clone_visitor.getCloned());
        QMT_CHECK(new_relation);
        MFlatAssignmentVisitor assign_visitor(relation);
        m_relation->accept(&assign_visitor);
        delete m_relation;
        m_relation = new_relation;
        emit m_modelController->endUpdateRelation(row, owner);
        emit m_modelController->modified();
        m_modelController->verifyModelIntegrity();
    }

private:

    ModelController *m_modelController;

    MRelation *m_relation;
};


class ModelController::AddElementsCommand :
        public UndoCommand
{
public:
    AddElementsCommand(ModelController *model_controller, const QString &command_label)
        : UndoCommand(command_label),
          m_modelController(model_controller)
    {
    }

    ~AddElementsCommand()
    {
        foreach (const Clone &clone, m_clonedElements) {
            delete clone.m_clonedElement;
        }
    }

    void add(ElementType elements_type, const Uid &object_key, const Uid &owner_key)
    {
        Clone clone;
        clone.m_elementType = elements_type;
        clone.m_elementKey = object_key;
        clone.m_ownerKey = owner_key;
        clone.m_indexOfElement = -1;
        m_clonedElements.append(clone);
    }

    void redo()
    {
        if (canRedo()) {
            bool inserted = false;
            for (int i = m_clonedElements.count() - 1; i >= 0; --i) {
                Clone &clone = m_clonedElements[i];
                QMT_CHECK(clone.m_clonedElement);
                QMT_CHECK(clone.m_clonedElement->getUid() == clone.m_elementKey);
                MObject *owner = m_modelController->findObject<MObject>(clone.m_ownerKey);
                QMT_CHECK(owner);
                QMT_CHECK(clone.m_indexOfElement >= 0);
                switch (clone.m_elementType) {
                case TYPE_OBJECT:
                {
                    emit m_modelController->beginInsertObject(clone.m_indexOfElement, owner);
                    MObject *object = dynamic_cast<MObject *>(clone.m_clonedElement);
                    QMT_CHECK(object);
                    m_modelController->mapObject(object);
                    owner->insertChild(clone.m_indexOfElement, object);
                    clone.m_clonedElement = 0;
                    emit m_modelController->endInsertObject(clone.m_indexOfElement, owner);
                    inserted = true;
                    break;
                }
                case TYPE_RELATION:
                {
                    emit m_modelController->beginInsertRelation(clone.m_indexOfElement, owner);
                    MRelation *relation = dynamic_cast<MRelation *>(clone.m_clonedElement);
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
            if (inserted) {
                emit m_modelController->modified();
            }
            m_modelController->verifyModelIntegrity();
            UndoCommand::redo();
        }
    }

    void undo()
    {
        bool removed = false;
        for (int i = 0; i < m_clonedElements.count(); ++i) {
            Clone &clone = m_clonedElements[i];
            QMT_CHECK(!clone.m_clonedElement);
            MObject *owner = m_modelController->findObject<MObject>(clone.m_ownerKey);
            QMT_CHECK(owner);
            switch (clone.m_elementType) {
            case TYPE_OBJECT:
            {
                MObject *object = m_modelController->findObject<MObject>(clone.m_elementKey);
                QMT_CHECK(object);
                clone.m_indexOfElement = owner->getChildren().indexOf(object);
                QMT_CHECK(clone.m_indexOfElement >= 0);
                emit m_modelController->beginRemoveObject(clone.m_indexOfElement, owner);
                MCloneDeepVisitor visitor;
                object->accept(&visitor);
                clone.m_clonedElement = visitor.getCloned();
                m_modelController->unmapObject(object);
                owner->removeChild(object);
                emit m_modelController->endRemoveObject(clone.m_indexOfElement, owner);
                removed = true;
                break;
            }
            case TYPE_RELATION:
            {
                MRelation *relation = m_modelController->findRelation<MRelation>(clone.m_elementKey);
                QMT_CHECK(relation);
                clone.m_indexOfElement = owner->getRelations().indexOf(relation);
                QMT_CHECK(clone.m_indexOfElement >= 0);
                emit m_modelController->beginRemoveRelation(clone.m_indexOfElement, owner);
                MCloneDeepVisitor visitor;
                relation->accept(&visitor);
                clone.m_clonedElement = visitor.getCloned();
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
        if (removed) {
            emit m_modelController->modified();
        }
        m_modelController->verifyModelIntegrity();
        UndoCommand::undo();
    }

private:

    ModelController *m_modelController;

    QList<Clone> m_clonedElements;
};


class ModelController::RemoveElementsCommand :
        public UndoCommand
{
public:
    RemoveElementsCommand(ModelController *model_controller, const QString &command_label)
        : UndoCommand(command_label),
          m_modelController(model_controller)
    {
    }

    ~RemoveElementsCommand()
    {
        foreach (const Clone &clone, m_clonedElements) {
            delete clone.m_clonedElement;
        }
    }

    void add(MElement *element, MObject *owner)
    {
        struct Clone clone;

        clone.m_elementKey = element->getUid();
        clone.m_ownerKey = owner->getUid();
        if (MObject *object = dynamic_cast<MObject *>(element)) {
            clone.m_elementType = TYPE_OBJECT;
            clone.m_indexOfElement = owner->getChildren().indexOf(object);
            QMT_CHECK(clone.m_indexOfElement >= 0);
        } else if (MRelation *relation = dynamic_cast<MRelation *>(element)) {
            clone.m_elementType = TYPE_RELATION;
            clone.m_indexOfElement = owner->getRelations().indexOf(relation);
            QMT_CHECK(clone.m_indexOfElement >= 0);
        } else {
            QMT_CHECK(false);
        }
        MCloneDeepVisitor visitor;
        element->accept(&visitor);
        clone.m_clonedElement = visitor.getCloned();
        QMT_CHECK(clone.m_clonedElement);
        m_clonedElements.append(clone);
    }

    void redo()
    {
        if (canRedo()) {
            bool removed = false;
            for (int i = 0; i < m_clonedElements.count(); ++i) {
                Clone &clone = m_clonedElements[i];
                QMT_CHECK(!clone.m_clonedElement);
                MObject *owner = m_modelController->findObject<MObject>(clone.m_ownerKey);
                QMT_CHECK(owner);
                switch (clone.m_elementType) {
                case TYPE_OBJECT:
                {
                    MObject *object = m_modelController->findObject<MObject>(clone.m_elementKey);
                    QMT_CHECK(object);
                    clone.m_indexOfElement = owner->getChildren().indexOf(object);
                    QMT_CHECK(clone.m_indexOfElement >= 0);
                    emit m_modelController->beginRemoveObject(clone.m_indexOfElement, owner);
                    MCloneDeepVisitor visitor;
                    object->accept(&visitor);
                    clone.m_clonedElement = visitor.getCloned();
                    m_modelController->unmapObject(object);
                    owner->removeChild(object);
                    emit m_modelController->endRemoveObject(clone.m_indexOfElement, owner);
                    removed = true;
                    break;
                }
                case TYPE_RELATION:
                {
                    MRelation *relation = m_modelController->findRelation<MRelation>(clone.m_elementKey);
                    QMT_CHECK(relation);
                    clone.m_indexOfElement = owner->getRelations().indexOf(relation);
                    QMT_CHECK(clone.m_indexOfElement >= 0);
                    emit m_modelController->beginRemoveRelation(clone.m_indexOfElement, owner);
                    MCloneDeepVisitor visitor;
                    relation->accept(&visitor);
                    clone.m_clonedElement = visitor.getCloned();
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
            if (removed) {
                emit m_modelController->modified();
            }
            m_modelController->verifyModelIntegrity();
            UndoCommand::redo();
        }
    }

    void undo()
    {
        bool inserted = false;
        for (int i = m_clonedElements.count() - 1; i >= 0; --i) {
            Clone &clone = m_clonedElements[i];
            QMT_CHECK(clone.m_clonedElement);
            MObject *owner = m_modelController->findObject<MObject>(clone.m_ownerKey);
            QMT_CHECK(owner);
            QMT_CHECK(clone.m_indexOfElement >= 0);
            switch (clone.m_elementType) {
            case TYPE_OBJECT:
            {
                emit m_modelController->beginInsertObject(clone.m_indexOfElement, owner);
                MObject *object = dynamic_cast<MObject *>(clone.m_clonedElement);
                QMT_CHECK(object);
                m_modelController->mapObject(object);
                owner->insertChild(clone.m_indexOfElement, object);
                clone.m_clonedElement = 0;
                emit m_modelController->endInsertObject(clone.m_indexOfElement, owner);
                inserted = true;
                break;
            }
            case TYPE_RELATION:
            {
                emit m_modelController->beginInsertRelation(clone.m_indexOfElement, owner);
                MRelation *relation = dynamic_cast<MRelation *>(clone.m_clonedElement);
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
        if (inserted) {
            emit m_modelController->modified();
        }
        m_modelController->verifyModelIntegrity();
        UndoCommand::undo();
    }

private:

    ModelController *m_modelController;

    QList<Clone> m_clonedElements;
};


class ModelController::MoveObjectCommand :
        public UndoCommand
{
public:
    MoveObjectCommand(ModelController *model_controller, MObject *object)
        : UndoCommand(tr("Move Object")),
          m_modelController(model_controller),
          m_objectKey(object->getUid()),
          m_ownerKey(object->getOwner()->getUid()),
          m_indexOfElement(object->getOwner()->getChildren().indexOf(object))
    {
    }

    ~MoveObjectCommand()
    {
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
        MObject *object = m_modelController->findObject(m_objectKey);
        QMT_CHECK(object);
        MObject *former_owner = object->getOwner();
        int former_row = former_owner->getChildren().indexOf(object);
        emit m_modelController->beginMoveObject(former_row, former_owner);
        former_owner->decontrolChild(object);
        MObject *new_owner = m_modelController->findObject(m_ownerKey);
        new_owner->insertChild(m_indexOfElement, object);
        int new_row = m_indexOfElement;
        m_ownerKey = former_owner->getUid();
        m_indexOfElement = former_row;
        emit m_modelController->endMoveObject(new_row, new_owner);
        emit m_modelController->modified();
        m_modelController->verifyModelIntegrity();
    }

private:

    ModelController *m_modelController;

    Uid m_objectKey;

    Uid m_ownerKey;

    int m_indexOfElement;

};


class ModelController::MoveRelationCommand :
        public UndoCommand
{
public:
    MoveRelationCommand(ModelController *model_controller, MRelation *relation)
        : UndoCommand(tr("Move Relation")),
          m_modelController(model_controller),
          m_relationKey(relation->getUid()),
          m_ownerKey(relation->getOwner()->getUid()),
          m_indexOfElement(relation->getOwner()->getRelations().indexOf(relation))
    {
    }

    ~MoveRelationCommand()
    {
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
        MRelation *relation = m_modelController->findRelation(m_relationKey);
        QMT_CHECK(relation);
        MObject *former_owner = relation->getOwner();
        int former_row = former_owner->getRelations().indexOf(relation);
        emit m_modelController->beginMoveRelation(former_row, former_owner);
        former_owner->decontrolRelation(relation);
        MObject *new_owner = m_modelController->findObject(m_ownerKey);
        new_owner->insertRelation(m_indexOfElement, relation);
        int new_row = m_indexOfElement;
        m_ownerKey = former_owner->getUid();
        m_indexOfElement = former_row;
        emit m_modelController->endMoveRelation(new_row, new_owner);
        emit m_modelController->modified();
        m_modelController->verifyModelIntegrity();
    }

private:

    ModelController *m_modelController;

    Uid m_relationKey;

    Uid m_ownerKey;

    int m_indexOfElement;

};


ModelController::ModelController(QObject *parent)
    : QObject(parent),
      m_rootPackage(0),
      m_undoController(0),
      m_resettingModel(false)
{
}

ModelController::~ModelController()
{
    delete m_rootPackage;
}

void ModelController::setRootPackage(MPackage *root_package)
{
    startResetModel();
    unmapObject(m_rootPackage);
    m_rootPackage = root_package;
    mapObject(m_rootPackage);
    finishResetModel(false);
}

void ModelController::setUndoController(UndoController *undo_controller)
{
    m_undoController = undo_controller;
}

Uid ModelController::getOwnerKey(const MElement *element) const
{
    QMT_CHECK(element);
    MObject *owner = element->getOwner();
    if (!owner) {
        return Uid();
    }
    return owner->getUid();
}

MElement *ModelController::findElement(const Uid &key)
{
    if (MObject *object = findObject(key)) {
        return object;
    } else if (MRelation *relation = findRelation(key)) {
        return relation;
    }
    return 0;
}

void ModelController::startResetModel()
{
    QMT_CHECK(!m_resettingModel);
    m_resettingModel = true;
    emit beginResetModel();
    QMT_CHECK(m_resettingModel);
}

void ModelController::finishResetModel(bool modified)
{
    QMT_CHECK(m_resettingModel);
    emit endResetModel();
    if (modified) {
        emit this->modified();
    }
    QMT_CHECK(m_resettingModel);
    m_resettingModel = false;
}

MObject *ModelController::getObject(int row, const MObject *owner) const
{
    if (!owner) {
        QMT_CHECK(row == 0);
        return m_rootPackage;
    }
    QMT_CHECK(row >= 0 && row < owner->getChildren().size());
    return owner->getChildren().at(row);
}

MObject *ModelController::findObject(const Uid &key) const
{
    return m_objectsMap.value(key);
}

void ModelController::addObject(MPackage *parent_package, MObject *object)
{
    QMT_CHECK(parent_package);
    QMT_CHECK(object);
    int row = parent_package->getChildren().size();
    if (!m_resettingModel) {
        emit beginInsertObject(row, parent_package);
    }
    mapObject(object);
    if (m_undoController) {
        AddElementsCommand *undo_command = new AddElementsCommand(this, tr("Add Object"));
        m_undoController->push(undo_command);
        undo_command->add(TYPE_OBJECT, object->getUid(), parent_package->getUid());
    }
    parent_package->addChild(object);
    if (!m_resettingModel) {
        emit endInsertObject(row, parent_package);
        emit modified();
    }
    verifyModelIntegrity();
}

void ModelController::removeObject(MObject *object)
{
    QMT_CHECK(object);
    if (m_undoController) {
        m_undoController->beginMergeSequence(tr("Delete Object"));
    }
    removeRelatedRelations(object);
    // remove object
    QMT_CHECK(object->getOwner());
    int row = object->getOwner()->getChildren().indexOf(object);
    MObject *owner = object->getOwner();
    if (!m_resettingModel) {
        emit beginRemoveObject(row, owner);
    }
    if (m_undoController) {
        RemoveElementsCommand *undo_command = new RemoveElementsCommand(this, tr("Delete Object"));
        m_undoController->push(undo_command);
        undo_command->add(object, object->getOwner());
    }
    unmapObject(object);
    owner->removeChild(object);
    if (!m_resettingModel) {
        emit endRemoveObject(row, owner);
        emit modified();
    }
    if (m_undoController) {
        m_undoController->endMergeSequence();
    }
    verifyModelIntegrity();
}

void ModelController::startUpdateObject(MObject *object)
{
    QMT_CHECK(object);
    int row = 0;
    MObject *parent = object->getOwner();
    if (!parent) {
        QMT_CHECK(object == m_rootPackage);
    } else {
        row = parent->getChildren().indexOf(object);
    }
    if (MPackage *package = dynamic_cast<MPackage *>(object)) {
        m_oldPackageName = package->getName();
    }
    if (!m_resettingModel) {
        emit beginUpdateObject(row, parent);
    }
    if (m_undoController) {
        m_undoController->push(new UpdateObjectCommand(this, object));
    }
}

void ModelController::finishUpdateObject(MObject *object, bool cancelled)
{
    QMT_CHECK(object);

    int row = 0;
    MObject *parent = object->getOwner();
    if (!parent) {
        QMT_CHECK(object == m_rootPackage);
    } else {
        row = parent->getChildren().indexOf(object);
    }
    if (!m_resettingModel) {
        emit endUpdateObject(row, parent);
        if (!cancelled) {
            QList<MRelation *> relations = findRelationsOfObject(object);
            foreach (MRelation *relation, relations) {
                emit relationEndChanged(relation, object);
            }
            if (MPackage *package = dynamic_cast<MPackage *>(object)) {
                if (m_oldPackageName != package->getName()) {
                    emit packageNameChanged(package, m_oldPackageName);
                }
            }
            emit modified();
        }
    }
    verifyModelIntegrity();
}

void ModelController::moveObject(MPackage *new_owner, MObject *object)
{
    QMT_CHECK(new_owner);
    QMT_CHECK(object);
    QMT_CHECK(object != m_rootPackage);

    if (new_owner != object->getOwner()) {
        int former_row = 0;
        MObject *former_owner = object->getOwner();
        QMT_CHECK(former_owner);
        former_row = former_owner->getChildren().indexOf(object);
        if (!m_resettingModel) {
            emit beginMoveObject(former_row, former_owner);
        }
        if (m_undoController) {
            MoveObjectCommand *undo_command = new MoveObjectCommand(this, object);
            m_undoController->push(undo_command);
        }
        former_owner->decontrolChild(object);
        new_owner->addChild(object);
        int row = new_owner->getChildren().indexOf(object);
        if (!m_resettingModel) {
            emit endMoveObject(row, new_owner);
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
    QMT_CHECK(findObject(relation->getEndA()));
    QMT_CHECK(findObject(relation->getEndB()));

    int row = owner->getRelations().size();
    if (!m_resettingModel) {
        emit beginInsertRelation(row, owner);
    }
    mapRelation(relation);
    if (m_undoController) {
        AddElementsCommand *undo_command = new AddElementsCommand(this, tr("Add Relation"));
        m_undoController->push(undo_command);
        undo_command->add(TYPE_RELATION, relation->getUid(), owner->getUid());
    }
    owner->addRelation(relation);
    if (!m_resettingModel) {
        emit endInsertRelation(row, owner);
        emit modified();
    }
    verifyModelIntegrity();
}

void ModelController::removeRelation(MRelation *relation)
{
    QMT_CHECK(relation);
    MObject *owner = relation->getOwner();
    QMT_CHECK(owner);
    int row = owner->getRelations().indexOf(relation);
    if (!m_resettingModel) {
        emit beginRemoveRelation(row, owner);
    }
    if (m_undoController) {
        RemoveElementsCommand *undo_command = new RemoveElementsCommand(this, tr("Delete Relation"));
        m_undoController->push(undo_command);
        undo_command->add(relation, owner);
    }
    unmapRelation(relation);
    owner->removeRelation(relation);
    if (!m_resettingModel) {
        emit endRemoveRelation(row, owner);
        emit modified();
    }
    verifyModelIntegrity();
}

void ModelController::startUpdateRelation(MRelation *relation)
{
    QMT_CHECK(relation);
    MObject *owner = relation->getOwner();
    QMT_CHECK(owner);
    if (!m_resettingModel) {
        emit beginUpdateRelation(owner->getRelations().indexOf(relation), owner);
    }
    if (m_undoController) {
        m_undoController->push(new UpdateRelationCommand(this, relation));
    }
}

void ModelController::finishUpdateRelation(MRelation *relation, bool cancelled)
{
    QMT_CHECK(relation);
    QMT_CHECK(findObject(relation->getEndA()));
    QMT_CHECK(findObject(relation->getEndB()));
    MObject *owner = relation->getOwner();
    QMT_CHECK(owner);
    if (!m_resettingModel) {
        emit endUpdateRelation(owner->getRelations().indexOf(relation), owner);
        if (!cancelled) {
            emit modified();
        }
    }
    verifyModelIntegrity();
}

void ModelController::moveRelation(MObject *new_owner, MRelation *relation)
{
    QMT_CHECK(new_owner);
    QMT_CHECK(relation);

    if (new_owner != relation->getOwner()) {
        int former_row = 0;
        MObject *former_owner = relation->getOwner();
        QMT_CHECK(former_owner);
        former_row = former_owner->getRelations().indexOf(relation);
        if (!m_resettingModel) {
            emit beginMoveRelation(former_row, former_owner);
        }
        if (m_undoController) {
            MoveRelationCommand *undo_command = new MoveRelationCommand(this, relation);
            m_undoController->push(undo_command);
        }
        former_owner->decontrolRelation(relation);
        new_owner->addRelation(relation);
        int row = new_owner->getRelations().indexOf(relation);
        if (!m_resettingModel) {
            emit endMoveRelation(row, new_owner);
            emit modified();
        }
    }
    verifyModelIntegrity();
}

QList<MRelation *> ModelController::findRelationsOfObject(const MObject *object) const
{
    QMT_CHECK(object);
    return m_objectRelationsMap.values(object->getUid());
}

MContainer ModelController::cutElements(const MSelection &model_selection)
{
    // PERFORM avoid duplicate call of simplify(model_selection)
    MContainer copied_elements = copyElements(model_selection);
    deleteElements(model_selection, tr("Cut"));
    return copied_elements;
}

MContainer ModelController::copyElements(const MSelection &model_selection)
{
    MReferences simplified_selection = simplify(model_selection);
    MContainer copied_elements;
    foreach (MElement *element, simplified_selection.getElements()) {
        MCloneDeepVisitor visitor;
        element->accept(&visitor);
        MElement *cloned_element = visitor.getCloned();
        copied_elements.submit(cloned_element);
    }
    return copied_elements;
}

void ModelController::pasteElements(MObject *owner, const MContainer &model_container)
{
    // clone all elements and renew their keys
    QHash<Uid, Uid> renewed_keys;
    QList<MElement *> cloned_elements;
    foreach (MElement *element, model_container.getElements()) {
        MCloneDeepVisitor visitor;
        element->accept(&visitor);
        MElement *cloned_element = visitor.getCloned();
        renewElementKey(cloned_element, &renewed_keys);
        cloned_elements.append(cloned_element);
    }
    // fix all keys referencing between pasting elements
    foreach (MElement *cloned_element, cloned_elements) {
        updateRelationKeys(cloned_element, renewed_keys);
    }
    if (m_undoController) {
        m_undoController->beginMergeSequence(tr("Paste"));
    }
    // insert all elements
    bool added = false;
    foreach (MElement *cloned_element, cloned_elements) {
        if (MObject *object = dynamic_cast<MObject *>(cloned_element)) {
            MObject *object_owner = owner;
            if (!dynamic_cast<MPackage*>(owner)) {
                object_owner = owner->getOwner();
            }
            QMT_CHECK(dynamic_cast<MPackage*>(object_owner));
            int row = object_owner->getChildren().size();
            emit beginInsertObject(row, object_owner);
            mapObject(object);
            if (m_undoController) {
                AddElementsCommand *undo_command = new AddElementsCommand(this, tr("Paste"));
                m_undoController->push(undo_command);
                undo_command->add(TYPE_OBJECT, object->getUid(), object_owner->getUid());
            }
            object_owner->insertChild(row, object);
            emit endInsertObject(row, object_owner);
            added = true;
        } else if (MRelation *relation = dynamic_cast<MRelation *>(cloned_element)) {
            int row = owner->getRelations().size();
            emit beginInsertRelation(row, owner);
            mapRelation(relation);
            if (m_undoController) {
                AddElementsCommand *undo_command = new AddElementsCommand(this, tr("Paste"));
                m_undoController->push(undo_command);
                undo_command->add(TYPE_RELATION, relation->getUid(), owner->getUid());
            }
            owner->addRelation(relation);
            emit endInsertRelation(row, owner);
            added = true;
        }
    }
    if (added) {
        emit modified();
    }
    verifyModelIntegrity();
    if (m_undoController) {
        m_undoController->endMergeSequence();
    }
}

void ModelController::deleteElements(const MSelection &model_selection)
{
    deleteElements(model_selection, tr("Delete"));
}

void ModelController::deleteElements(const MSelection &model_selection, const QString &command_label)
{
    MReferences simplified_selection = simplify(model_selection);
    if (simplified_selection.getElements().isEmpty()) {
        return;
    }
    if (m_undoController) {
        m_undoController->beginMergeSequence(command_label);
    }
    bool removed = false;
    foreach (MElement *element, simplified_selection.getElements()) {
        // element may have been deleted indirectly by predecessor element in loop
        if ((element = findElement(element->getUid()))) {
            if (MObject *object = dynamic_cast<MObject *>(element)) {
                removeRelatedRelations(object);
                MObject *owner = object->getOwner();
                int row = owner->getChildren().indexOf(object);
                emit beginRemoveObject(row, owner);
                if (m_undoController) {
                    RemoveElementsCommand *cut_command = new RemoveElementsCommand(this, command_label);
                    m_undoController->push(cut_command);
                    cut_command->add(element, owner);
                }
                unmapObject(object);
                owner->removeChild(object);
                emit endRemoveObject(row, owner);
                removed = true;
            } else if (MRelation *relation = dynamic_cast<MRelation *>(element)) {
                MObject *owner = relation->getOwner();
                int row = owner->getRelations().indexOf(relation);
                emit beginRemoveRelation(row, owner);
                if (m_undoController) {
                    RemoveElementsCommand *cut_command = new RemoveElementsCommand(this, command_label);
                    m_undoController->push(cut_command);
                    cut_command->add(element, owner);
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
    if (removed) {
        emit modified();
    }
    verifyModelIntegrity();
    if (m_undoController) {
        m_undoController->endMergeSequence();
    }
}

void ModelController::removeRelatedRelations(MObject *object)
{
    foreach (MRelation *relation, m_objectRelationsMap.values(object->getUid())) {
        removeRelation(relation);
    }
    QMT_CHECK(m_objectRelationsMap.values(object->getUid()).isEmpty());
}

void ModelController::renewElementKey(MElement *element, QHash<Uid, Uid> *renewed_keys)
{
    if (element) {
        MElement *other_element = findObject(element->getUid());
        if (other_element) {
            QMT_CHECK(other_element != element);
        }
        if (m_objectsMap.contains(element->getUid()) || m_relationsMap.contains(element->getUid())) {
            Uid old_key = element->getUid();
            element->renewUid();
            Uid new_key = element->getUid();
            renewed_keys->insert(old_key, new_key);
        }
        MObject *object = dynamic_cast<MObject *>(element);
        if (object) {
            foreach (const Handle<MObject> &child, object->getChildren()) {
                renewElementKey(child.getTarget(), renewed_keys);
            }
            foreach (const Handle<MRelation> &relation, object->getRelations()) {
                renewElementKey(relation.getTarget(), renewed_keys);
            }
        }
    }
}

void ModelController::updateRelationKeys(MElement *element, const QHash<Uid, Uid> &renewed_keys)
{
    if (MObject *object = dynamic_cast<MObject *>(element)) {
        foreach (const Handle<MRelation> &handle, object->getRelations()) {
            updateRelationEndKeys(handle.getTarget(), renewed_keys);
        }
        foreach (const Handle<MObject> &child, object->getChildren()) {
            updateRelationKeys(child.getTarget(), renewed_keys);
        }
    } else if (MRelation *relation = dynamic_cast<MRelation *>(element)) {
        updateRelationEndKeys(relation, renewed_keys);
    }
}

void ModelController::updateRelationEndKeys(MRelation *relation, const QHash<Uid, Uid> &renewed_keys)
{
    if (relation) {
        Uid new_end_a_key = renewed_keys.value(relation->getEndA(), Uid::getInvalidUid());
        if (new_end_a_key.isValid()) {
            relation->setEndA(new_end_a_key);
        }
        Uid new_end_b_key = renewed_keys.value(relation->getEndB(), Uid::getInvalidUid());
        if (new_end_b_key.isValid()) {
            relation->setEndB(new_end_b_key);
        }
    }
}

void ModelController::mapObject(MObject *object)
{
    if (object) {
        QMT_CHECK(!m_objectsMap.contains(object->getUid()));
        m_objectsMap.insert(object->getUid(), object);
        foreach (const Handle<MObject> &child, object->getChildren()) {
            mapObject(child.getTarget());
        }
        foreach (const Handle<MRelation> &relation, object->getRelations()) {
            mapRelation(relation.getTarget());
        }
    }
}

void ModelController::unmapObject(MObject *object)
{
    if (object) {
        QMT_CHECK(m_objectsMap.contains(object->getUid()));
        foreach (const Handle<MRelation> &relation, object->getRelations()) {
            unmapRelation(relation.getTarget());
        }
        foreach (const Handle<MObject> &child, object->getChildren()) {
            unmapObject(child.getTarget());
        }
        m_objectsMap.remove(object->getUid());
    }
}

void ModelController::mapRelation(MRelation *relation)
{
    if (relation) {
        QMT_CHECK(!m_relationsMap.contains(relation->getUid()));
        m_relationsMap.insert(relation->getUid(), relation);
        QMT_CHECK(!m_objectRelationsMap.contains(relation->getEndA(), relation));
        m_objectRelationsMap.insert(relation->getEndA(), relation);
        if (relation->getEndA() != relation->getEndB()) {
            QMT_CHECK(!m_objectRelationsMap.contains(relation->getEndB(), relation));
            m_objectRelationsMap.insert(relation->getEndB(), relation);
        }
    }
}

void ModelController::unmapRelation(MRelation *relation)
{
    if (relation) {
        QMT_CHECK(m_relationsMap.contains(relation->getUid()));
        m_relationsMap.remove(relation->getUid());
        QMT_CHECK(m_objectRelationsMap.contains(relation->getEndA(), relation));
        m_objectRelationsMap.remove(relation->getEndA(), relation);
        if (relation->getEndA() != relation->getEndB()) {
            QMT_CHECK(m_objectRelationsMap.contains(relation->getEndB(), relation));
            m_objectRelationsMap.remove(relation->getEndB(), relation);
        }
    }
}

MReferences ModelController::simplify(const MSelection &model_selection)
{
    // PERFORM improve performance by using a set of Uid build from model_selection
    MReferences references;
    foreach (const MSelection::Index &index, model_selection.getIndices()) {
        MElement *element = findElement(index.getElementKey());
        QMT_CHECK(element);
        // if any (grand-)parent of element is in model_selection then ignore element
        bool ignore = false;
        MObject *owner = element->getOwner();
        while (owner) {
            Uid owner_key = owner->getUid();
            foreach (const MSelection::Index &index, model_selection.getIndices()) {
                if (index.getElementKey() == owner_key) {
                    ignore = true;
                    break;
                }
            }
            if (ignore) {
                break;
            }
            owner = owner->getOwner();
        }
        if (!ignore) {
            references.append(element);
        }
    }
    return references;
}

void ModelController::verifyModelIntegrity() const
{
#if 0
#ifndef QT_NO_DEBUG
    QMT_CHECK(m_rootPackage);

    QHash<Uid, const MObject *> objects_map;
    QHash<Uid, const MRelation *> relations_map;
    QMultiHash<Uid, MRelation *> object_relations_map;
    verifyModelIntegrity(m_rootPackage, &objects_map, &relations_map, &object_relations_map);

    QMT_CHECK(objects_map.size() == m_objectsMap.size());
    foreach (const MObject *object, m_objectsMap) {
        QMT_CHECK(object);
        QMT_CHECK(m_objectsMap.contains(object->getUid()));
        QMT_CHECK(objects_map.contains(object->getUid()));
    }
    QMT_CHECK(relations_map.size() == m_relationsMap.size());
    foreach (const MRelation *relation, m_relationsMap) {
        QMT_CHECK(relation);
        QMT_CHECK(m_relationsMap.contains(relation->getUid()));
        QMT_CHECK(relations_map.contains(relation->getUid()));
    }
    QMT_CHECK(object_relations_map.size() == m_objectRelationsMap.size());
    for (QMultiHash<Uid, MRelation *>::const_iterator it = m_objectRelationsMap.cbegin(); it != m_objectRelationsMap.cend(); ++it) {
        QMT_CHECK(object_relations_map.contains(it.key(), it.value()));
    }
#endif
#endif
}

void ModelController::verifyModelIntegrity(const MObject *object, QHash<Uid, const MObject *> *objects_map,
                                           QHash<Uid, const MRelation *> *relations_map,
                                           QMultiHash<Uid, MRelation *> *object_relations_map) const
{
#ifndef QT_NO_DEBUG
    QMT_CHECK(object);
    QMT_CHECK(!objects_map->contains(object->getUid()));
    objects_map->insert(object->getUid(), object);
    foreach (const Handle<MRelation> &handle, object->getRelations()) {
        MRelation *relation = handle.getTarget();
        if (relation) {
            QMT_CHECK(!relations_map->contains(relation->getUid()));
            relations_map->insert(relation->getUid(), relation);
            QMT_CHECK(findObject(relation->getEndA()));
            QMT_CHECK(findObject(relation->getEndB()));
            QMT_CHECK(!object_relations_map->contains(relation->getEndA(), relation));
            object_relations_map->insert(relation->getEndA(), relation);
            QMT_CHECK(!object_relations_map->contains(relation->getEndB(), relation));
            object_relations_map->insert(relation->getEndB(), relation);
        }
    }
    foreach (const Handle<MObject> &handle, object->getChildren()) {
        MObject *child_object = handle.getTarget();
        if (child_object) {
            verifyModelIntegrity(child_object, objects_map, relations_map, object_relations_map);
        }
    }
#else
    Q_UNUSED(object);
    Q_UNUSED(objects_map);
    Q_UNUSED(relations_map);
    Q_UNUSED(object_relations_map);
#endif
}

}
