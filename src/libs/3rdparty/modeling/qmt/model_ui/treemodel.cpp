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

#include "treemodel.h"

#include "qmt/model_controller/modelcontroller.h"

#include "qmt/model/mobject.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model/mitem.h"
#include "qmt/model/mrelation.h"
#include "qmt/model/massociation.h"
#include "qmt/model/mdependency.h"
#include "qmt/model/minheritance.h"

#include "qmt/model/mconstvisitor.h"

#include "qmt/stereotype/shapepaintvisitor.h"
#include "qmt/stereotype/stereotypecontroller.h"

#include "qmt/style/style.h"
#include "qmt/style/stylecontroller.h"

#include <QStandardItem>
#include <QDebug>


namespace qmt {

class TreeModel::ModelItem :
        public QStandardItem
{
public:
    ModelItem(const QIcon &icon, const QString &text)
        : QStandardItem(icon, text)
    {
    }

    QList<QString> getStereotypes() const { return _stereotypes; }

    void setStereotypes(const QList<QString> &stereotypes) { _stereotypes = stereotypes; }

private:

    QList<QString> _stereotypes;
};


class TreeModel::ItemFactory :
        public MConstVisitor
{
public:
    ItemFactory(TreeModel *tree_model)
        : _tree_model(tree_model),
          _item(0)
    {
        QMT_CHECK(_tree_model);
    }

public:

    ModelItem *getProduct() const { return _item; }

public:

    void visitMElement(const MElement *element)
    {
        Q_UNUSED(element);
        QMT_CHECK(false);
    }

    void visitMObject(const MObject *object)
    {
        Q_UNUSED(object);
        QMT_CHECK(_item);
        _item->setEditable(false);
    }

    void visitMPackage(const MPackage *package)
    {
        QMT_CHECK(!_item);

        static QIcon icon(QStringLiteral(":/modelinglib/48x48/package.png"));
        _item = new ModelItem(icon, _tree_model->createObjectLabel(package));
        _item->setData(TreeModel::PACKAGE, TreeModel::ROLE_ITEM_TYPE);
        visitMObject(package);
    }

    void visitMClass(const MClass *klass)
    {
        QMT_CHECK(!_item);

        QIcon icon = _tree_model->createIcon(StereotypeIcon::ELEMENT_CLASS, StyleEngine::TYPE_CLASS, klass->getStereotypes(), QStringLiteral(":/modelinglib/48x48/class.png"));
        _item = new ModelItem(icon, _tree_model->createObjectLabel(klass));
        _item->setData(TreeModel::ELEMENT, TreeModel::ROLE_ITEM_TYPE);
        _item->setStereotypes(klass->getStereotypes());
        visitMObject(klass);
    }

    void visitMComponent(const MComponent *component)
    {
        QMT_CHECK(!_item);

        QIcon icon = _tree_model->createIcon(StereotypeIcon::ELEMENT_COMPONENT, StyleEngine::TYPE_COMPONENT, component->getStereotypes(), QStringLiteral(":/modelinglib/48x48/component.png"));
        _item = new ModelItem(icon, _tree_model->createObjectLabel(component));
        _item->setData(TreeModel::ELEMENT, TreeModel::ROLE_ITEM_TYPE);
        _item->setStereotypes(component->getStereotypes());
        visitMObject(component);
    }

    void visitMDiagram(const MDiagram *diagram)
    {
        visitMObject(diagram);
        _item->setData(TreeModel::DIAGRAM, TreeModel::ROLE_ITEM_TYPE);
    }

    void visitMCanvasDiagram(const MCanvasDiagram *diagram)
    {
        QMT_CHECK(!_item);

        static QIcon icon(QStringLiteral(":/modelinglib/48x48/canvas-diagram.png"));
        _item = new ModelItem(icon, _tree_model->createObjectLabel(diagram));
        visitMDiagram(diagram);
    }

    void visitMItem(const MItem *item)
    {
        QMT_CHECK(!_item);

        QList<QString> stereotypes = item->getStereotypes() << item->getVariety();
        QIcon icon = _tree_model->createIcon(StereotypeIcon::ELEMENT_ITEM, StyleEngine::TYPE_ITEM, stereotypes, QStringLiteral(":/modelinglib/48x48/item.png"));
        _item = new ModelItem(icon, _tree_model->createObjectLabel(item));
        _item->setData(TreeModel::ELEMENT, TreeModel::ROLE_ITEM_TYPE);
        _item->setStereotypes(stereotypes);
        visitMObject(item);
    }

    void visitMRelation(const MRelation *relation)
    {
        Q_UNUSED(relation);
        QMT_CHECK(_item);
        _item->setEditable(false);
        _item->setData(TreeModel::RELATION, TreeModel::ROLE_ITEM_TYPE);
    }

    void visitMDependency(const MDependency *dependency)
    {
        QMT_CHECK(!_item);

        static QIcon icon(QStringLiteral(":/modelinglib/48x48/dependency.png"));
        _item = new ModelItem(icon, _tree_model->createRelationLabel(dependency));
        visitMRelation(dependency);
    }

    void visitMInheritance(const MInheritance *inheritance)
    {
        QMT_CHECK(!_item);

        static QIcon icon(QStringLiteral(":/modelinglib/48x48/inheritance.png"));
        _item = new ModelItem(icon, _tree_model->createRelationLabel(inheritance));
        visitMRelation(inheritance);
    }

    void visitMAssociation(const MAssociation *association)
    {
        QMT_CHECK(!_item);

        static QIcon icon(QStringLiteral(":/modelinglib/48x48/association.png"));
        _item = new ModelItem(icon, _tree_model->createRelationLabel(association));
        visitMRelation(association);
    }

private:

    TreeModel *_tree_model;

    ModelItem *_item;
};



class TreeModel::ItemUpdater :
        public MConstVisitor
{
public:
    ItemUpdater(TreeModel *tree_model, ModelItem *item)
        : _tree_model(tree_model),
          _item(item)
    {
        QMT_CHECK(_tree_model);
        QMT_CHECK(_item);
    }

public:

    void visitMElement(const MElement *element)
    {
        Q_UNUSED(element);
        QMT_CHECK(false);
    }

    void visitMObject(const MObject *object)
    {
        updateObjectLabel(object);
    }

    void visitMPackage(const MPackage *package)
    {
        visitMObject(package);
    }

    void visitMClass(const MClass *klass)
    {
        if (klass->getStereotypes() != _item->getStereotypes()) {
            QIcon icon = _tree_model->createIcon(StereotypeIcon::ELEMENT_CLASS, StyleEngine::TYPE_CLASS, klass->getStereotypes(), QStringLiteral(":/modelinglib/48x48/class.png"));
            _item->setIcon(icon);
            _item->setStereotypes(klass->getStereotypes());
        }
        visitMObject(klass);
    }

    void visitMComponent(const MComponent *component)
    {
        if (component->getStereotypes() != _item->getStereotypes()) {
            QIcon icon = _tree_model->createIcon(StereotypeIcon::ELEMENT_COMPONENT, StyleEngine::TYPE_COMPONENT, component->getStereotypes(), QStringLiteral(":/modelinglib/48x48/component.png"));
            _item->setIcon(icon);
            _item->setStereotypes(component->getStereotypes());
        }
        visitMObject(component);
    }

    void visitMDiagram(const MDiagram *diagram)
    {
        visitMObject(diagram);
    }

    void visitMCanvasDiagram(const MCanvasDiagram *diagram)
    {
        visitMDiagram(diagram);
    }

    void visitMItem(const MItem *item)
    {
        QList<QString> stereotypes = item->getStereotypes() << item->getVariety();
        if (stereotypes != _item->getStereotypes()) {
            QIcon icon = _tree_model->createIcon(StereotypeIcon::ELEMENT_ITEM, StyleEngine::TYPE_ITEM, stereotypes, QStringLiteral(":/modelinglib/48x48/item.png"));
            _item->setIcon(icon);
            _item->setStereotypes(stereotypes);
        }
        visitMObject(item);
    }

    void visitMRelation(const MRelation *relation)
    {
        updateRelationLabel(relation);
    }

    void visitMDependency(const MDependency *dependency)
    {
        visitMRelation(dependency);
    }

    void visitMInheritance(const MInheritance *inheritance)
    {
        visitMRelation(inheritance);
    }

    void visitMAssociation(const MAssociation *association)
    {
        visitMRelation(association);
    }

private:

    void updateObjectLabel(const MObject *object);

    void updateRelationLabel(const MRelation *relation);

private:

    TreeModel *_tree_model;

    ModelItem *_item;
};

void TreeModel::ItemUpdater::updateObjectLabel(const MObject *object)
{
    QString label = _tree_model->createObjectLabel(object);
    if (_item->text() != label) {
        _item->setText(label);
    }
}

void TreeModel::ItemUpdater::updateRelationLabel(const MRelation *relation)
{
    QString label = _tree_model->createRelationLabel(relation);
    if (_item->text() != label) {
        _item->setText(label);
    }
}


TreeModel::TreeModel(QObject *parent)
    : QStandardItemModel(parent),
      _model_controller(0),
      _stereotype_controller(0),
      _style_controller(0),
      _root_item(0),
      _busy(NOT_BUSY)
{
    connect(this, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(onModelDataChanged(QModelIndex,QModelIndex)));
}

TreeModel::~TreeModel()
{
    QMT_CHECK(_busy == NOT_BUSY);
    disconnect();
    clear();
}

void TreeModel::setModelController(ModelController *model_controller)
{
    if (_model_controller != model_controller) {
        if (_model_controller) {
            disconnect(_model_controller, 0, this, 0);
        }
        _model_controller = model_controller;
        if (_model_controller) {
            connect(_model_controller, SIGNAL(beginResetModel()), this, SLOT(onBeginResetModel()));
            connect(_model_controller, SIGNAL(endResetModel()), this, SLOT(onEndResetModel()));

            connect(_model_controller, SIGNAL(beginInsertObject(int,const MObject*)), this, SLOT(onBeginInsertObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(endInsertObject(int,const MObject*)), this, SLOT(onEndInsertObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(beginUpdateObject(int,const MObject*)), this, SLOT(onBeginUpdateObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(endUpdateObject(int,const MObject*)), this, SLOT(onEndUpdateObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(beginRemoveObject(int,const MObject*)), this, SLOT(onBeginRemoveObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(endRemoveObject(int,const MObject*)), this, SLOT(onEndRemoveObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(beginMoveObject(int,const MObject*)), this, SLOT(onBeginMoveObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(endMoveObject(int,const MObject*)), this, SLOT(onEndMoveObject(int,const MObject*)));

            connect(_model_controller, SIGNAL(beginInsertRelation(int,const MObject*)), this, SLOT(onBeginInsertRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(endInsertRelation(int,const MObject*)), this, SLOT(onEndInsertRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(beginUpdateRelation(int,const MObject*)), this, SLOT(onBeginUpdateRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(endUpdateRelation(int,const MObject*)), this, SLOT(onEndUpdateRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(beginRemoveRelation(int,const MObject*)), this, SLOT(onBeginRemoveRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(endRemoveRelation(int,const MObject*)), this, SLOT(onEndRemoveRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(beginMoveRelation(int,const MObject*)), this, SLOT(onBeginMoveRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(endMoveRelation(int,const MObject*)), this, SLOT(onEndMoveRelation(int,const MObject*)));

            connect(_model_controller, SIGNAL(relationEndChanged(MRelation*,MObject*)), this, SLOT(onRelationEndChanged(MRelation*,MObject*)));
        }
    }
}

void TreeModel::setStereotypeController(StereotypeController *stereotype_controller)
{
    _stereotype_controller = stereotype_controller;
}

void TreeModel::setStyleController(StyleController *style_controller)
{
    _style_controller = style_controller;
}

MElement *TreeModel::getElement(const QModelIndex &index) const
{
    QMT_CHECK(index.isValid());

    MElement *element = 0;
    QStandardItem *item = itemFromIndex(index);
    if (item) {
        if (item->parent()) {
            ModelItem *parent_model_item = dynamic_cast<ModelItem *>(item->parent());
            QMT_CHECK(parent_model_item);
            const MObject *parent_object = _item_to_object_map.value(parent_model_item);
            QMT_CHECK(parent_object);
            if (parent_object) {
                if (index.row() >= 0 && index.row() < parent_object->getChildren().size()) {
                    element = parent_object->getChildren().at(index.row());
                    QMT_CHECK(element);
                } else if (index.row() >= parent_object->getChildren().size()
                           && index.row() < parent_object->getChildren().size() + parent_object->getRelations().size()) {
                    element = parent_object->getRelations().at(index.row() - parent_object->getChildren().size());
                    QMT_CHECK(element);
                } else {
                    QMT_CHECK(false);
                }
            }
        } else if (index.row() == 0) {
            element = _model_controller->getRootPackage();
        } else {
            QMT_CHECK(false);
        }
    }
    return element;
}

QModelIndex TreeModel::getIndex(const MElement *element) const
{
    if (const MObject *object = dynamic_cast<const MObject *>(element)) {
        if (!object->getOwner()) {
            QMT_CHECK(element == _model_controller->getRootPackage());
            return index(0, 0);
        }
        MObject *parent_object = object->getOwner();
        ModelItem *item = _object_to_item_map.value(parent_object);
        if (!item) {
            QMT_CHECK(false);
            return QModelIndex();
        }
        QModelIndex parent_index = indexFromItem(item);
        int row = parent_object->getChildren().indexOf(object);
        return index(row, 0, parent_index);
    } else if (const MRelation *relation = dynamic_cast<const MRelation *>(element)) {
        QMT_CHECK(relation->getOwner());
        MObject *owner = relation->getOwner();
        ModelItem *item = _object_to_item_map.value(owner);
        if (!item) {
            QMT_CHECK(false);
            return QModelIndex();
        }
        QModelIndex parent_index = indexFromItem(item);
        int row = owner->getChildren().size() + owner->getRelations().indexOf(relation);
        return index(row, 0, parent_index);
    }
    return QModelIndex();
}

QIcon TreeModel::getIcon(const QModelIndex &index) const
{
    QStandardItem *item = itemFromIndex(index);
    if (item) {
        return item->icon();
    }
    return QIcon();
}

Qt::DropActions TreeModel::supportedDropActions() const
{
    return Qt::DropActions() | Qt::CopyAction | Qt::MoveAction;
}

QStringList TreeModel::mimeTypes() const
{
    return QStringList() << QStringLiteral("text/model-elements");
}

void TreeModel::onBeginResetModel()
{
    QMT_CHECK(_busy == NOT_BUSY);
    _busy = RESET_MODEL;
    QStandardItemModel::beginResetModel();
}

void TreeModel::onEndResetModel()
{
    QMT_CHECK(_busy == RESET_MODEL);
    clear();
    MPackage *root_package = _model_controller->getRootPackage();
    if (_model_controller && root_package) {
        _root_item = createItem(root_package);
        appendRow(_root_item);
        createChildren(root_package, _root_item);
        QStandardItemModel::endResetModel();
    }
    _busy = NOT_BUSY;
}

void TreeModel::onBeginUpdateObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    QMT_CHECK(_busy == NOT_BUSY);
    _busy = UPDATE_ELEMENT;
}

void TreeModel::onEndUpdateObject(int row, const MObject *parent)
{
    QMT_CHECK(_busy == UPDATE_ELEMENT);
    QModelIndex parent_index;
    if (parent) {
        QMT_CHECK(_object_to_item_map.contains(parent));
        ModelItem  *parent_item = _object_to_item_map.value(parent);
        QMT_CHECK(parent_item);
        parent_index = indexFromItem(parent_item);
    }
    // reflect updated element in standard item
    QModelIndex element_index = this->index(row, 0, parent_index);
    MElement *element = getElement(element_index);
    if (element) {
        MObject *object = dynamic_cast<MObject *>(element);
        if (object) {
            ModelItem *item = dynamic_cast<ModelItem *>(itemFromIndex(element_index));
            QMT_CHECK(item);
            ItemUpdater visitor(this, item);
            element->accept(&visitor);
        }
    }
    _busy = NOT_BUSY;
    emit dataChanged(this->index(row, 0, parent_index), this->index(row, 0, parent_index));
}

void TreeModel::onBeginInsertObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    QMT_CHECK(_busy == NOT_BUSY);
    _busy = INSERT_ELEMENT;
}

void TreeModel::onEndInsertObject(int row, const MObject *parent)
{
    QMT_CHECK(_busy == INSERT_ELEMENT);
    ModelItem *parent_item =_object_to_item_map.value(parent);
    QMT_CHECK(parent_item);
    MObject *object = parent->getChildren().at(row);
    ModelItem *item = createItem(object);
    parent_item->insertRow(row, item);
    createChildren(object, item);
    _busy = NOT_BUSY;
}

void TreeModel::onBeginRemoveObject(int row, const MObject *parent)
{
    QMT_CHECK(_busy == NOT_BUSY);
    _busy = REMOVE_ELEMENT;
    QMT_CHECK(parent);
    MObject *object = parent->getChildren().at(row);
    if (object) {
        removeObjectFromItemMap(object);
    }
    ModelItem *parent_item = _object_to_item_map.value(parent);
    QMT_CHECK(parent_item);
    parent_item->removeRow(row);
}

void TreeModel::onEndRemoveObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    QMT_CHECK(_busy == REMOVE_ELEMENT);
    _busy = NOT_BUSY;
}

void TreeModel::onBeginMoveObject(int former_row, const MObject *former_owner)
{
    QMT_CHECK(_busy == NOT_BUSY);
    _busy = MOVE_ELEMENT;
    QMT_CHECK(former_owner);
    MObject *object = former_owner->getChildren().at(former_row);
    if (object) {
        removeObjectFromItemMap(object);
    }
    ModelItem *parent_item = _object_to_item_map.value(former_owner);
    QMT_CHECK(parent_item);
    parent_item->removeRow(former_row);
}

void TreeModel::onEndMoveObject(int row, const MObject *owner)
{
    QMT_CHECK(_busy == MOVE_ELEMENT);
    ModelItem *parent_item =_object_to_item_map.value(owner);
    QMT_CHECK(parent_item);
    MObject *object = owner->getChildren().at(row);
    ModelItem *item = createItem(object);
    parent_item->insertRow(row, item);
    createChildren(object, item);
    _busy = NOT_BUSY;
}

void TreeModel::onBeginUpdateRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    QMT_CHECK(_busy == NOT_BUSY);
    _busy = UPDATE_RELATION;
}

void TreeModel::onEndUpdateRelation(int row, const MObject *parent)
{
    QMT_CHECK(parent);
    QMT_CHECK(_busy == UPDATE_RELATION);

    QMT_CHECK(_object_to_item_map.contains(parent));
    ModelItem *parent_item = _object_to_item_map.value(parent);
    QMT_CHECK(parent_item);
    QModelIndex parent_index = indexFromItem(parent_item);

    // reflect updated relation in standard item
    row += parent->getChildren().size();
    QModelIndex element_index = this->index(row, 0, parent_index);
    MElement *element = getElement(element_index);
    if (element) {
        MRelation *relation = dynamic_cast<MRelation *>(element);
        if (relation) {
            ModelItem *item = dynamic_cast<ModelItem *>(itemFromIndex(element_index));
            QMT_CHECK(item);
            ItemUpdater visitor(this, item);
            element->accept(&visitor);
        }
    }
    _busy = NOT_BUSY;
    emit dataChanged(this->index(row, 0, parent_index), this->index(row, 0, parent_index));
}

void TreeModel::onBeginInsertRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    QMT_CHECK(_busy == NOT_BUSY);
    _busy = INSERT_RELATION;
}

void TreeModel::onEndInsertRelation(int row, const MObject *parent)
{
    QMT_CHECK(parent);
    QMT_CHECK(_busy == INSERT_RELATION);
    ModelItem *parent_item =_object_to_item_map.value(parent);
    QMT_CHECK(parent_item);
    MRelation *relation = parent->getRelations().at(row);
    ModelItem *item = createItem(relation);
    parent_item->insertRow(parent->getChildren().size() + row, item);
    _busy = NOT_BUSY;
}

void TreeModel::onBeginRemoveRelation(int row, const MObject *parent)
{
    QMT_CHECK(parent);
    QMT_CHECK(_busy == NOT_BUSY);
    _busy = REMOVE_RELATION;
    QMT_CHECK(parent->getRelations().at(row));
    ModelItem *parent_item = _object_to_item_map.value(parent);
    QMT_CHECK(parent_item);
    parent_item->removeRow(parent->getChildren().size() + row);
}

void TreeModel::onEndRemoveRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    QMT_CHECK(_busy == REMOVE_RELATION);
    _busy = NOT_BUSY;
}

void TreeModel::onBeginMoveRelation(int former_row, const MObject *former_owner)
{
    QMT_CHECK(_busy == NOT_BUSY);
    _busy = MOVE_ELEMENT;
    QMT_CHECK(former_owner);
    QMT_CHECK(former_owner->getRelations().at(former_row));
    ModelItem *parent_item = _object_to_item_map.value(former_owner);
    QMT_CHECK(parent_item);
    parent_item->removeRow(former_owner->getChildren().size() + former_row);
}

void TreeModel::onEndMoveRelation(int row, const MObject *owner)
{
    QMT_CHECK(owner);
    QMT_CHECK(_busy == MOVE_ELEMENT);
    ModelItem *parent_item =_object_to_item_map.value(owner);
    QMT_CHECK(parent_item);
    MRelation *relation = owner->getRelations().at(row);
    ModelItem *item = createItem(relation);
    parent_item->insertRow(owner->getChildren().size() + row, item);
    _busy = NOT_BUSY;
}

void TreeModel::onRelationEndChanged(MRelation *relation, MObject *end_object)
{
    Q_UNUSED(end_object);
    QMT_CHECK(_busy == NOT_BUSY);

    MObject *parent = relation->getOwner();
    QMT_CHECK(parent);
    QMT_CHECK(_object_to_item_map.contains(parent));
    ModelItem *parent_item = _object_to_item_map.value(parent);
    QMT_CHECK(parent_item);
    QModelIndex parent_index = indexFromItem(parent_item);

    int row = parent->getChildren().size() + relation->getOwner()->getRelations().indexOf(relation);
    QModelIndex element_index = this->index(row, 0, parent_index);
    QMT_CHECK(element_index.isValid());

    ModelItem *item = dynamic_cast<ModelItem *>(itemFromIndex(element_index));
    QMT_CHECK(item);

    QString label = createRelationLabel(relation);
    if (item->text() != label) {
        item->setText(label);
    }

    emit dataChanged(this->index(row, 0, parent_index), this->index(row, 0, parent_index));
}

void TreeModel::onModelDataChanged(const QModelIndex &topleft, const QModelIndex &bottomright)
{
    Q_UNUSED(topleft);
    Q_UNUSED(bottomright);
    // TODO fix editing object name in model tree
    // item->text() no longer returns a simple object name
    // classes contains namespace label
    // Possible solution?: When label gets focus morph label into simple object name
}

void TreeModel::clear()
{
    QStandardItemModel::clear();
    _root_item = 0;
    _object_to_item_map.clear();
    _item_to_object_map.clear();
}

TreeModel::ModelItem *TreeModel::createItem(const MElement *element)
{
    ItemFactory visitor(this);
    element->accept(&visitor);
    QMT_CHECK(visitor.getProduct());
    return visitor.getProduct();
}

void TreeModel::createChildren(const MObject *parent_object, ModelItem *parent_item)
{
    QMT_CHECK(!_object_to_item_map.contains(parent_object));
    _object_to_item_map.insert(parent_object, parent_item);
    QMT_CHECK(!_item_to_object_map.contains(parent_item));
    _item_to_object_map.insert(parent_item, parent_object);
    foreach (const Handle<MObject> &object, parent_object->getChildren()) {
        if (object.hasTarget()) {
            ModelItem *item = createItem(object.getTarget());
            parent_item->appendRow(item);
            createChildren(object.getTarget(), item);
        }
    }
    foreach (const Handle<MRelation> &handle, parent_object->getRelations()) {
        if (handle.hasTarget()) {
            MRelation *relation = handle.getTarget();
            ModelItem *item = createItem(relation);
            parent_item->appendRow(item);
        }
    }
}

void TreeModel::removeObjectFromItemMap(const MObject *object)
{
    QMT_CHECK(object);
    QMT_CHECK(_object_to_item_map.contains(object));
    ModelItem *item = _object_to_item_map.value(object);
    QMT_CHECK(item);
    QMT_CHECK(_item_to_object_map.contains(item));
    _item_to_object_map.remove(item);
    _object_to_item_map.remove(object);
    foreach (const Handle<MObject> &child, object->getChildren()) {
        if (child.hasTarget()) {
            removeObjectFromItemMap(child.getTarget());
        }
    }
}

QString TreeModel::createObjectLabel(const MObject *object)
{
    QMT_CHECK(object);

    if (object->getName().isEmpty()) {
        if (const MItem *item = dynamic_cast<const MItem *>(object)) {
            if (!item->getVariety().isEmpty()) {
                return QString(QStringLiteral("[%1]")).arg(item->getVariety());
            }
        }
        return tr("[unnamed]");
    }

    if (const MClass *klass = dynamic_cast<const MClass *>(object)) {
        if (!klass->getNamespace().isEmpty()) {
            return QString(QStringLiteral("%1 [%2]")).arg(klass->getName()).arg(klass->getNamespace());
        }
    }
    return object->getName();
}

QString TreeModel::createRelationLabel(const MRelation *relation)
{
    QString name;
    if (!relation->getName().isEmpty()) {
        name += relation->getName();
        name += QStringLiteral(": ");
    }
    if (MObject *end_a = _model_controller->findObject(relation->getEndA())) {
        name += createObjectLabel(end_a);
    }
    name += QStringLiteral(" - ");
    if (MObject *end_b = _model_controller->findObject(relation->getEndB())) {
        name += createObjectLabel(end_b);
    }
    return name;
}

QIcon TreeModel::createIcon(StereotypeIcon::Element stereotype_icon_element, StyleEngine::ElementType style_element_type, const QStringList &stereotypes, const QString &default_icon_path)
{
    const Style *style = _style_controller->adaptStyle(style_element_type);
    return _stereotype_controller->createIcon(stereotype_icon_element, stereotypes, default_icon_path, style,
                                              QSize(48, 48), QMarginsF(3.0, 2.0, 3.0, 4.0));
}

}
