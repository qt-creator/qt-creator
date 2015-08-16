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

    ElementType _element_type;
    Uid _element_key;
    Uid _owner_key;
    int _index_of_element;
    MElement *_cloned_element;
};

ModelController::Clone::Clone()
    : _element_type(TYPE_UNKNOWN),
      _index_of_element(-1),
      _cloned_element(0)
{
}


class ModelController::UpdateObjectCommand :
        public UndoCommand
{
public:
    UpdateObjectCommand(ModelController *model_controller, MObject *object)
        : UndoCommand(tr("Change Object")),
          _model_controller(model_controller),
          _object(0)
    {
        MCloneVisitor visitor;
        object->accept(&visitor);
        _object = dynamic_cast<MObject *>(visitor.getCloned());
        QMT_CHECK(_object);
    }

    ~UpdateObjectCommand()
    {
        delete _object;
    }

    bool mergeWith(const UndoCommand *other)
    {
        const UpdateObjectCommand *update_command = dynamic_cast<const UpdateObjectCommand *>(other);
        if (!update_command) {
            return false;
        }
        if (_object->getUid() != update_command->_object->getUid()) {
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
        MObject *object = _model_controller->findObject<MObject>(_object->getUid());
        QMT_CHECK(object);
        int row = 0;
        MObject *parent = object->getOwner();
        if (!parent) {
            QMT_CHECK(object == _model_controller->_root_package);
        } else {
            row = object->getOwner()->getChildren().indexOf(object);
        }
        emit _model_controller->beginUpdateObject(row, parent);
        MCloneVisitor clone_visitor;
        object->accept(&clone_visitor);
        MObject *new_object = dynamic_cast<MObject *>(clone_visitor.getCloned());
        QMT_CHECK(new_object);
        MFlatAssignmentVisitor assign_visitor(object);
        _object->accept(&assign_visitor);
        delete _object;
        _object = new_object;
        emit _model_controller->endUpdateObject(row, parent);
        emit _model_controller->modified();
        _model_controller->verifyModelIntegrity();
    }

private:

    ModelController *_model_controller;

    MObject *_object;
};


class ModelController::UpdateRelationCommand :
        public UndoCommand
{
public:
    UpdateRelationCommand(ModelController *model_controller, MRelation *relation)
        : UndoCommand(tr("Change Relation")),
          _model_controller(model_controller),
          _relation(0)
    {
        MCloneVisitor visitor;
        relation->accept(&visitor);
        _relation = dynamic_cast<MRelation *>(visitor.getCloned());
        QMT_CHECK(_relation);
    }

    ~UpdateRelationCommand()
    {
        delete _relation;
    }

    bool mergeWith(const UndoCommand *other)
    {
        const UpdateRelationCommand *update_command = dynamic_cast<const UpdateRelationCommand *>(other);
        if (!update_command) {
            return false;
        }
        if (_relation->getUid() != update_command->_relation->getUid()) {
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
        MRelation *relation = _model_controller->findRelation<MRelation>(_relation->getUid());
        QMT_CHECK(relation);
        MObject *owner = relation->getOwner();
        QMT_CHECK(owner);
        int row = owner->getRelations().indexOf(relation);
        emit _model_controller->beginUpdateRelation(row, owner);
        MCloneVisitor clone_visitor;
        relation->accept(&clone_visitor);
        MRelation *new_relation = dynamic_cast<MRelation *>(clone_visitor.getCloned());
        QMT_CHECK(new_relation);
        MFlatAssignmentVisitor assign_visitor(relation);
        _relation->accept(&assign_visitor);
        delete _relation;
        _relation = new_relation;
        emit _model_controller->endUpdateRelation(row, owner);
        emit _model_controller->modified();
        _model_controller->verifyModelIntegrity();
    }

private:

    ModelController *_model_controller;

    MRelation *_relation;
};


class ModelController::AddElementsCommand :
        public UndoCommand
{
public:
    AddElementsCommand(ModelController *model_controller, const QString &command_label)
        : UndoCommand(command_label),
          _model_controller(model_controller)
    {
    }

    ~AddElementsCommand()
    {
        foreach (const Clone &clone, _cloned_elements) {
            delete clone._cloned_element;
        }
    }

    void add(ElementType elements_type, const Uid &object_key, const Uid &owner_key)
    {
        Clone clone;
        clone._element_type = elements_type;
        clone._element_key = object_key;
        clone._owner_key = owner_key;
        clone._index_of_element = -1;
        _cloned_elements.append(clone);
    }

    void redo()
    {
        if (canRedo()) {
            bool inserted = false;
            for (int i = _cloned_elements.count() - 1; i >= 0; --i) {
                Clone &clone = _cloned_elements[i];
                QMT_CHECK(clone._cloned_element);
                QMT_CHECK(clone._cloned_element->getUid() == clone._element_key);
                MObject *owner = _model_controller->findObject<MObject>(clone._owner_key);
                QMT_CHECK(owner);
                QMT_CHECK(clone._index_of_element >= 0);
                switch (clone._element_type) {
                case TYPE_OBJECT:
                {
                    emit _model_controller->beginInsertObject(clone._index_of_element, owner);
                    MObject *object = dynamic_cast<MObject *>(clone._cloned_element);
                    QMT_CHECK(object);
                    _model_controller->mapObject(object);
                    owner->insertChild(clone._index_of_element, object);
                    clone._cloned_element = 0;
                    emit _model_controller->endInsertObject(clone._index_of_element, owner);
                    inserted = true;
                    break;
                }
                case TYPE_RELATION:
                {
                    emit _model_controller->beginInsertRelation(clone._index_of_element, owner);
                    MRelation *relation = dynamic_cast<MRelation *>(clone._cloned_element);
                    QMT_CHECK(relation);
                    _model_controller->mapRelation(relation);
                    owner->insertRelation(clone._index_of_element, relation);
                    clone._cloned_element = 0;
                    emit _model_controller->endInsertRelation(clone._index_of_element, owner);
                    inserted = true;
                    break;
                }
                default:
                    QMT_CHECK(false);
                    break;
                }
            }
            if (inserted) {
                emit _model_controller->modified();
            }
            _model_controller->verifyModelIntegrity();
            UndoCommand::redo();
        }
    }

    void undo()
    {
        bool removed = false;
        for (int i = 0; i < _cloned_elements.count(); ++i) {
            Clone &clone = _cloned_elements[i];
            QMT_CHECK(!clone._cloned_element);
            MObject *owner = _model_controller->findObject<MObject>(clone._owner_key);
            QMT_CHECK(owner);
            switch (clone._element_type) {
            case TYPE_OBJECT:
            {
                MObject *object = _model_controller->findObject<MObject>(clone._element_key);
                QMT_CHECK(object);
                clone._index_of_element = owner->getChildren().indexOf(object);
                QMT_CHECK(clone._index_of_element >= 0);
                emit _model_controller->beginRemoveObject(clone._index_of_element, owner);
                MCloneDeepVisitor visitor;
                object->accept(&visitor);
                clone._cloned_element = visitor.getCloned();
                _model_controller->unmapObject(object);
                owner->removeChild(object);
                emit _model_controller->endRemoveObject(clone._index_of_element, owner);
                removed = true;
                break;
            }
            case TYPE_RELATION:
            {
                MRelation *relation = _model_controller->findRelation<MRelation>(clone._element_key);
                QMT_CHECK(relation);
                clone._index_of_element = owner->getRelations().indexOf(relation);
                QMT_CHECK(clone._index_of_element >= 0);
                emit _model_controller->beginRemoveRelation(clone._index_of_element, owner);
                MCloneDeepVisitor visitor;
                relation->accept(&visitor);
                clone._cloned_element = visitor.getCloned();
                _model_controller->unmapRelation(relation);
                owner->removeRelation(relation);
                emit _model_controller->endRemoveRelation(clone._index_of_element, owner);
                removed = true;
                break;
            }
            default:
                QMT_CHECK(false);
                break;
            }
        }
        if (removed) {
            emit _model_controller->modified();
        }
        _model_controller->verifyModelIntegrity();
        UndoCommand::undo();
    }

private:

    ModelController *_model_controller;

    QList<Clone> _cloned_elements;
};


class ModelController::RemoveElementsCommand :
        public UndoCommand
{
public:
    RemoveElementsCommand(ModelController *model_controller, const QString &command_label)
        : UndoCommand(command_label),
          _model_controller(model_controller)
    {
    }

    ~RemoveElementsCommand()
    {
        foreach (const Clone &clone, _cloned_elements) {
            delete clone._cloned_element;
        }
    }

    void add(MElement *element, MObject *owner)
    {
        struct Clone clone;

        clone._element_key = element->getUid();
        clone._owner_key = owner->getUid();
        if (MObject *object = dynamic_cast<MObject *>(element)) {
            clone._element_type = TYPE_OBJECT;
            clone._index_of_element = owner->getChildren().indexOf(object);
            QMT_CHECK(clone._index_of_element >= 0);
        } else if (MRelation *relation = dynamic_cast<MRelation *>(element)) {
            clone._element_type = TYPE_RELATION;
            clone._index_of_element = owner->getRelations().indexOf(relation);
            QMT_CHECK(clone._index_of_element >= 0);
        } else {
            QMT_CHECK(false);
        }
        MCloneDeepVisitor visitor;
        element->accept(&visitor);
        clone._cloned_element = visitor.getCloned();
        QMT_CHECK(clone._cloned_element);
        _cloned_elements.append(clone);
    }

    void redo()
    {
        if (canRedo()) {
            bool removed = false;
            for (int i = 0; i < _cloned_elements.count(); ++i) {
                Clone &clone = _cloned_elements[i];
                QMT_CHECK(!clone._cloned_element);
                MObject *owner = _model_controller->findObject<MObject>(clone._owner_key);
                QMT_CHECK(owner);
                switch (clone._element_type) {
                case TYPE_OBJECT:
                {
                    MObject *object = _model_controller->findObject<MObject>(clone._element_key);
                    QMT_CHECK(object);
                    clone._index_of_element = owner->getChildren().indexOf(object);
                    QMT_CHECK(clone._index_of_element >= 0);
                    emit _model_controller->beginRemoveObject(clone._index_of_element, owner);
                    MCloneDeepVisitor visitor;
                    object->accept(&visitor);
                    clone._cloned_element = visitor.getCloned();
                    _model_controller->unmapObject(object);
                    owner->removeChild(object);
                    emit _model_controller->endRemoveObject(clone._index_of_element, owner);
                    removed = true;
                    break;
                }
                case TYPE_RELATION:
                {
                    MRelation *relation = _model_controller->findRelation<MRelation>(clone._element_key);
                    QMT_CHECK(relation);
                    clone._index_of_element = owner->getRelations().indexOf(relation);
                    QMT_CHECK(clone._index_of_element >= 0);
                    emit _model_controller->beginRemoveRelation(clone._index_of_element, owner);
                    MCloneDeepVisitor visitor;
                    relation->accept(&visitor);
                    clone._cloned_element = visitor.getCloned();
                    _model_controller->unmapRelation(relation);
                    owner->removeRelation(relation);
                    emit _model_controller->endRemoveRelation(clone._index_of_element, owner);
                    removed = true;
                    break;
                }
                default:
                    QMT_CHECK(false);
                    break;
                }
            }
            if (removed) {
                emit _model_controller->modified();
            }
            _model_controller->verifyModelIntegrity();
            UndoCommand::redo();
        }
    }

    void undo()
    {
        bool inserted = false;
        for (int i = _cloned_elements.count() - 1; i >= 0; --i) {
            Clone &clone = _cloned_elements[i];
            QMT_CHECK(clone._cloned_element);
            MObject *owner = _model_controller->findObject<MObject>(clone._owner_key);
            QMT_CHECK(owner);
            QMT_CHECK(clone._index_of_element >= 0);
            switch (clone._element_type) {
            case TYPE_OBJECT:
            {
                emit _model_controller->beginInsertObject(clone._index_of_element, owner);
                MObject *object = dynamic_cast<MObject *>(clone._cloned_element);
                QMT_CHECK(object);
                _model_controller->mapObject(object);
                owner->insertChild(clone._index_of_element, object);
                clone._cloned_element = 0;
                emit _model_controller->endInsertObject(clone._index_of_element, owner);
                inserted = true;
                break;
            }
            case TYPE_RELATION:
            {
                emit _model_controller->beginInsertRelation(clone._index_of_element, owner);
                MRelation *relation = dynamic_cast<MRelation *>(clone._cloned_element);
                QMT_CHECK(relation);
                _model_controller->mapRelation(relation);
                owner->insertRelation(clone._index_of_element, relation);
                clone._cloned_element = 0;
                emit _model_controller->endInsertRelation(clone._index_of_element, owner);
                inserted = true;
                break;
            }
            default:
                QMT_CHECK(false);
                break;
            }
        }
        if (inserted) {
            emit _model_controller->modified();
        }
        _model_controller->verifyModelIntegrity();
        UndoCommand::undo();
    }

private:

    ModelController *_model_controller;

    QList<Clone> _cloned_elements;
};


class ModelController::MoveObjectCommand :
        public UndoCommand
{
public:
    MoveObjectCommand(ModelController *model_controller, MObject *object)
        : UndoCommand(tr("Move Object")),
          _model_controller(model_controller),
          _object_key(object->getUid()),
          _owner_key(object->getOwner()->getUid()),
          _index_of_element(object->getOwner()->getChildren().indexOf(object))
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
        MObject *object = _model_controller->findObject(_object_key);
        QMT_CHECK(object);
        MObject *former_owner = object->getOwner();
        int former_row = former_owner->getChildren().indexOf(object);
        emit _model_controller->beginMoveObject(former_row, former_owner);
        former_owner->decontrolChild(object);
        MObject *new_owner = _model_controller->findObject(_owner_key);
        new_owner->insertChild(_index_of_element, object);
        int new_row = _index_of_element;
        _owner_key = former_owner->getUid();
        _index_of_element = former_row;
        emit _model_controller->endMoveObject(new_row, new_owner);
        emit _model_controller->modified();
        _model_controller->verifyModelIntegrity();
    }

private:

    ModelController *_model_controller;

    Uid _object_key;

    Uid _owner_key;

    int _index_of_element;

};


class ModelController::MoveRelationCommand :
        public UndoCommand
{
public:
    MoveRelationCommand(ModelController *model_controller, MRelation *relation)
        : UndoCommand(tr("Move Relation")),
          _model_controller(model_controller),
          _relation_key(relation->getUid()),
          _owner_key(relation->getOwner()->getUid()),
          _index_of_element(relation->getOwner()->getRelations().indexOf(relation))
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
        MRelation *relation = _model_controller->findRelation(_relation_key);
        QMT_CHECK(relation);
        MObject *former_owner = relation->getOwner();
        int former_row = former_owner->getRelations().indexOf(relation);
        emit _model_controller->beginMoveRelation(former_row, former_owner);
        former_owner->decontrolRelation(relation);
        MObject *new_owner = _model_controller->findObject(_owner_key);
        new_owner->insertRelation(_index_of_element, relation);
        int new_row = _index_of_element;
        _owner_key = former_owner->getUid();
        _index_of_element = former_row;
        emit _model_controller->endMoveRelation(new_row, new_owner);
        emit _model_controller->modified();
        _model_controller->verifyModelIntegrity();
    }

private:

    ModelController *_model_controller;

    Uid _relation_key;

    Uid _owner_key;

    int _index_of_element;

};


ModelController::ModelController(QObject *parent)
    : QObject(parent),
      _root_package(0),
      _undo_controller(0),
      _resetting_model(false)
{
}

ModelController::~ModelController()
{
    delete _root_package;
}

void ModelController::setRootPackage(MPackage *root_package)
{
    startResetModel();
    unmapObject(_root_package);
    _root_package = root_package;
    mapObject(_root_package);
    finishResetModel(false);
}

void ModelController::setUndoController(UndoController *undo_controller)
{
    _undo_controller = undo_controller;
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
    QMT_CHECK(!_resetting_model);
    _resetting_model = true;
    emit beginResetModel();
    QMT_CHECK(_resetting_model);
}

void ModelController::finishResetModel(bool modified)
{
    QMT_CHECK(_resetting_model);
    emit endResetModel();
    if (modified) {
        emit this->modified();
    }
    QMT_CHECK(_resetting_model);
    _resetting_model = false;
}

MObject *ModelController::getObject(int row, const MObject *owner) const
{
    if (!owner) {
        QMT_CHECK(row == 0);
        return _root_package;
    }
    QMT_CHECK(row >= 0 && row < owner->getChildren().size());
    return owner->getChildren().at(row);
}

MObject *ModelController::findObject(const Uid &key) const
{
    return _objects_map.value(key);
}

void ModelController::addObject(MPackage *parent_package, MObject *object)
{
    QMT_CHECK(parent_package);
    QMT_CHECK(object);
    int row = parent_package->getChildren().size();
    if (!_resetting_model) {
        emit beginInsertObject(row, parent_package);
    }
    mapObject(object);
    if (_undo_controller) {
        AddElementsCommand *undo_command = new AddElementsCommand(this, tr("Add Object"));
        _undo_controller->push(undo_command);
        undo_command->add(TYPE_OBJECT, object->getUid(), parent_package->getUid());
    }
    parent_package->addChild(object);
    if (!_resetting_model) {
        emit endInsertObject(row, parent_package);
        emit modified();
    }
    verifyModelIntegrity();
}

void ModelController::removeObject(MObject *object)
{
    QMT_CHECK(object);
    if (_undo_controller) {
        _undo_controller->beginMergeSequence(tr("Delete Object"));
    }
    removeRelatedRelations(object);
    // remove object
    QMT_CHECK(object->getOwner());
    int row = object->getOwner()->getChildren().indexOf(object);
    MObject *owner = object->getOwner();
    if (!_resetting_model) {
        emit beginRemoveObject(row, owner);
    }
    if (_undo_controller) {
        RemoveElementsCommand *undo_command = new RemoveElementsCommand(this, tr("Delete Object"));
        _undo_controller->push(undo_command);
        undo_command->add(object, object->getOwner());
    }
    unmapObject(object);
    owner->removeChild(object);
    if (!_resetting_model) {
        emit endRemoveObject(row, owner);
        emit modified();
    }
    if (_undo_controller) {
        _undo_controller->endMergeSequence();
    }
    verifyModelIntegrity();
}

void ModelController::startUpdateObject(MObject *object)
{
    QMT_CHECK(object);
    int row = 0;
    MObject *parent = object->getOwner();
    if (!parent) {
        QMT_CHECK(object == _root_package);
    } else {
        row = parent->getChildren().indexOf(object);
    }
    if (MPackage *package = dynamic_cast<MPackage *>(object)) {
        _old_package_name = package->getName();
    }
    if (!_resetting_model) {
        emit beginUpdateObject(row, parent);
    }
    if (_undo_controller) {
        _undo_controller->push(new UpdateObjectCommand(this, object));
    }
}

void ModelController::finishUpdateObject(MObject *object, bool cancelled)
{
    QMT_CHECK(object);

    int row = 0;
    MObject *parent = object->getOwner();
    if (!parent) {
        QMT_CHECK(object == _root_package);
    } else {
        row = parent->getChildren().indexOf(object);
    }
    if (!_resetting_model) {
        emit endUpdateObject(row, parent);
        if (!cancelled) {
            QList<MRelation *> relations = findRelationsOfObject(object);
            foreach (MRelation *relation, relations) {
                emit relationEndChanged(relation, object);
            }
            if (MPackage *package = dynamic_cast<MPackage *>(object)) {
                if (_old_package_name != package->getName()) {
                    emit packageNameChanged(package, _old_package_name);
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
    QMT_CHECK(object != _root_package);

    if (new_owner != object->getOwner()) {
        int former_row = 0;
        MObject *former_owner = object->getOwner();
        QMT_CHECK(former_owner);
        former_row = former_owner->getChildren().indexOf(object);
        if (!_resetting_model) {
            emit beginMoveObject(former_row, former_owner);
        }
        if (_undo_controller) {
            MoveObjectCommand *undo_command = new MoveObjectCommand(this, object);
            _undo_controller->push(undo_command);
        }
        former_owner->decontrolChild(object);
        new_owner->addChild(object);
        int row = new_owner->getChildren().indexOf(object);
        if (!_resetting_model) {
            emit endMoveObject(row, new_owner);
            emit modified();
        }
    }
    verifyModelIntegrity();
}

MRelation *ModelController::findRelation(const Uid &key) const
{
    return _relations_map.value(key);
}

void ModelController::addRelation(MObject *owner, MRelation *relation)
{
    QMT_CHECK(owner);
    QMT_CHECK(relation);
    QMT_CHECK(findObject(relation->getEndA()));
    QMT_CHECK(findObject(relation->getEndB()));

    int row = owner->getRelations().size();
    if (!_resetting_model) {
        emit beginInsertRelation(row, owner);
    }
    mapRelation(relation);
    if (_undo_controller) {
        AddElementsCommand *undo_command = new AddElementsCommand(this, tr("Add Relation"));
        _undo_controller->push(undo_command);
        undo_command->add(TYPE_RELATION, relation->getUid(), owner->getUid());
    }
    owner->addRelation(relation);
    if (!_resetting_model) {
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
    if (!_resetting_model) {
        emit beginRemoveRelation(row, owner);
    }
    if (_undo_controller) {
        RemoveElementsCommand *undo_command = new RemoveElementsCommand(this, tr("Delete Relation"));
        _undo_controller->push(undo_command);
        undo_command->add(relation, owner);
    }
    unmapRelation(relation);
    owner->removeRelation(relation);
    if (!_resetting_model) {
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
    if (!_resetting_model) {
        emit beginUpdateRelation(owner->getRelations().indexOf(relation), owner);
    }
    if (_undo_controller) {
        _undo_controller->push(new UpdateRelationCommand(this, relation));
    }
}

void ModelController::finishUpdateRelation(MRelation *relation, bool cancelled)
{
    QMT_CHECK(relation);
    QMT_CHECK(findObject(relation->getEndA()));
    QMT_CHECK(findObject(relation->getEndB()));
    MObject *owner = relation->getOwner();
    QMT_CHECK(owner);
    if (!_resetting_model) {
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
        if (!_resetting_model) {
            emit beginMoveRelation(former_row, former_owner);
        }
        if (_undo_controller) {
            MoveRelationCommand *undo_command = new MoveRelationCommand(this, relation);
            _undo_controller->push(undo_command);
        }
        former_owner->decontrolRelation(relation);
        new_owner->addRelation(relation);
        int row = new_owner->getRelations().indexOf(relation);
        if (!_resetting_model) {
            emit endMoveRelation(row, new_owner);
            emit modified();
        }
    }
    verifyModelIntegrity();
}

QList<MRelation *> ModelController::findRelationsOfObject(const MObject *object) const
{
    QMT_CHECK(object);
    return _object_relations_map.values(object->getUid());
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
    if (_undo_controller) {
        _undo_controller->beginMergeSequence(tr("Paste"));
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
            if (_undo_controller) {
                AddElementsCommand *undo_command = new AddElementsCommand(this, tr("Paste"));
                _undo_controller->push(undo_command);
                undo_command->add(TYPE_OBJECT, object->getUid(), object_owner->getUid());
            }
            object_owner->insertChild(row, object);
            emit endInsertObject(row, object_owner);
            added = true;
        } else if (MRelation *relation = dynamic_cast<MRelation *>(cloned_element)) {
            int row = owner->getRelations().size();
            emit beginInsertRelation(row, owner);
            mapRelation(relation);
            if (_undo_controller) {
                AddElementsCommand *undo_command = new AddElementsCommand(this, tr("Paste"));
                _undo_controller->push(undo_command);
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
    if (_undo_controller) {
        _undo_controller->endMergeSequence();
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
    if (_undo_controller) {
        _undo_controller->beginMergeSequence(command_label);
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
                if (_undo_controller) {
                    RemoveElementsCommand *cut_command = new RemoveElementsCommand(this, command_label);
                    _undo_controller->push(cut_command);
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
                if (_undo_controller) {
                    RemoveElementsCommand *cut_command = new RemoveElementsCommand(this, command_label);
                    _undo_controller->push(cut_command);
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
    if (_undo_controller) {
        _undo_controller->endMergeSequence();
    }
}

void ModelController::removeRelatedRelations(MObject *object)
{
    foreach (MRelation *relation, _object_relations_map.values(object->getUid())) {
        removeRelation(relation);
    }
    QMT_CHECK(_object_relations_map.values(object->getUid()).isEmpty());
}

void ModelController::renewElementKey(MElement *element, QHash<Uid, Uid> *renewed_keys)
{
    if (element) {
        MElement *other_element = findObject(element->getUid());
        if (other_element) {
            QMT_CHECK(other_element != element);
        }
        if (_objects_map.contains(element->getUid()) || _relations_map.contains(element->getUid())) {
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
        QMT_CHECK(!_objects_map.contains(object->getUid()));
        _objects_map.insert(object->getUid(), object);
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
        QMT_CHECK(_objects_map.contains(object->getUid()));
        foreach (const Handle<MRelation> &relation, object->getRelations()) {
            unmapRelation(relation.getTarget());
        }
        foreach (const Handle<MObject> &child, object->getChildren()) {
            unmapObject(child.getTarget());
        }
        _objects_map.remove(object->getUid());
    }
}

void ModelController::mapRelation(MRelation *relation)
{
    if (relation) {
        QMT_CHECK(!_relations_map.contains(relation->getUid()));
        _relations_map.insert(relation->getUid(), relation);
        QMT_CHECK(!_object_relations_map.contains(relation->getEndA(), relation));
        _object_relations_map.insert(relation->getEndA(), relation);
        if (relation->getEndA() != relation->getEndB()) {
            QMT_CHECK(!_object_relations_map.contains(relation->getEndB(), relation));
            _object_relations_map.insert(relation->getEndB(), relation);
        }
    }
}

void ModelController::unmapRelation(MRelation *relation)
{
    if (relation) {
        QMT_CHECK(_relations_map.contains(relation->getUid()));
        _relations_map.remove(relation->getUid());
        QMT_CHECK(_object_relations_map.contains(relation->getEndA(), relation));
        _object_relations_map.remove(relation->getEndA(), relation);
        if (relation->getEndA() != relation->getEndB()) {
            QMT_CHECK(_object_relations_map.contains(relation->getEndB(), relation));
            _object_relations_map.remove(relation->getEndB(), relation);
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
    QMT_CHECK(_root_package);

    QHash<Uid, const MObject *> objects_map;
    QHash<Uid, const MRelation *> relations_map;
    QMultiHash<Uid, MRelation *> object_relations_map;
    verifyModelIntegrity(_root_package, &objects_map, &relations_map, &object_relations_map);

    QMT_CHECK(objects_map.size() == _objects_map.size());
    foreach (const MObject *object, _objects_map) {
        QMT_CHECK(object);
        QMT_CHECK(_objects_map.contains(object->getUid()));
        QMT_CHECK(objects_map.contains(object->getUid()));
    }
    QMT_CHECK(relations_map.size() == _relations_map.size());
    foreach (const MRelation *relation, _relations_map) {
        QMT_CHECK(relation);
        QMT_CHECK(_relations_map.contains(relation->getUid()));
        QMT_CHECK(relations_map.contains(relation->getUid()));
    }
    QMT_CHECK(object_relations_map.size() == _object_relations_map.size());
    for (QMultiHash<Uid, MRelation *>::const_iterator it = _object_relations_map.cbegin(); it != _object_relations_map.cend(); ++it) {
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
