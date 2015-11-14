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

namespace qmt {

class TreeModel::ModelItem : public QStandardItem
{
public:
    ModelItem(const QIcon &icon, const QString &text)
        : QStandardItem(icon, text)
    {
    }

    QList<QString> stereotypes() const { return m_stereotypes; }
    void setStereotypes(const QList<QString> &stereotypes) { m_stereotypes = stereotypes; }

private:
    QList<QString> m_stereotypes;
};

class TreeModel::ItemFactory : public MConstVisitor
{
public:
    ItemFactory(TreeModel *treeModel)
        : m_treeModel(treeModel),
          m_item(0)
    {
        QMT_CHECK(m_treeModel);
    }

    ModelItem *product() const { return m_item; }

    void visitMElement(const MElement *element)
    {
        Q_UNUSED(element);
        QMT_CHECK(false);
    }

    void visitMObject(const MObject *object)
    {
        Q_UNUSED(object);
        QMT_CHECK(m_item);
        m_item->setEditable(false);
    }

    void visitMPackage(const MPackage *package)
    {
        QMT_CHECK(!m_item);

        static QIcon icon(QStringLiteral(":/modelinglib/48x48/package.png"));
        m_item = new ModelItem(icon, m_treeModel->createObjectLabel(package));
        m_item->setData(TreeModel::Package, TreeModel::RoleItemType);
        visitMObject(package);
    }

    void visitMClass(const MClass *klass)
    {
        QMT_CHECK(!m_item);

        QIcon icon = m_treeModel->createIcon(StereotypeIcon::ElementClass, StyleEngine::TypeClass, klass->stereotypes(), QStringLiteral(":/modelinglib/48x48/class.png"));
        m_item = new ModelItem(icon, m_treeModel->createObjectLabel(klass));
        m_item->setData(TreeModel::Element, TreeModel::RoleItemType);
        m_item->setStereotypes(klass->stereotypes());
        visitMObject(klass);
    }

    void visitMComponent(const MComponent *component)
    {
        QMT_CHECK(!m_item);

        QIcon icon = m_treeModel->createIcon(StereotypeIcon::ElementComponent, StyleEngine::TypeComponent, component->stereotypes(), QStringLiteral(":/modelinglib/48x48/component.png"));
        m_item = new ModelItem(icon, m_treeModel->createObjectLabel(component));
        m_item->setData(TreeModel::Element, TreeModel::RoleItemType);
        m_item->setStereotypes(component->stereotypes());
        visitMObject(component);
    }

    void visitMDiagram(const MDiagram *diagram)
    {
        visitMObject(diagram);
        m_item->setData(TreeModel::Diagram, TreeModel::RoleItemType);
    }

    void visitMCanvasDiagram(const MCanvasDiagram *diagram)
    {
        QMT_CHECK(!m_item);

        static QIcon icon(QStringLiteral(":/modelinglib/48x48/canvas-diagram.png"));
        m_item = new ModelItem(icon, m_treeModel->createObjectLabel(diagram));
        visitMDiagram(diagram);
    }

    void visitMItem(const MItem *item)
    {
        QMT_CHECK(!m_item);

        QList<QString> stereotypes = item->stereotypes() << item->variety();
        QIcon icon = m_treeModel->createIcon(StereotypeIcon::ElementItem, StyleEngine::TypeItem, stereotypes, QStringLiteral(":/modelinglib/48x48/item.png"));
        m_item = new ModelItem(icon, m_treeModel->createObjectLabel(item));
        m_item->setData(TreeModel::Element, TreeModel::RoleItemType);
        m_item->setStereotypes(stereotypes);
        visitMObject(item);
    }

    void visitMRelation(const MRelation *relation)
    {
        Q_UNUSED(relation);
        QMT_CHECK(m_item);
        m_item->setEditable(false);
        m_item->setData(TreeModel::Relation, TreeModel::RoleItemType);
    }

    void visitMDependency(const MDependency *dependency)
    {
        QMT_CHECK(!m_item);

        static QIcon icon(QStringLiteral(":/modelinglib/48x48/dependency.png"));
        m_item = new ModelItem(icon, m_treeModel->createRelationLabel(dependency));
        visitMRelation(dependency);
    }

    void visitMInheritance(const MInheritance *inheritance)
    {
        QMT_CHECK(!m_item);

        static QIcon icon(QStringLiteral(":/modelinglib/48x48/inheritance.png"));
        m_item = new ModelItem(icon, m_treeModel->createRelationLabel(inheritance));
        visitMRelation(inheritance);
    }

    void visitMAssociation(const MAssociation *association)
    {
        QMT_CHECK(!m_item);

        static QIcon icon(QStringLiteral(":/modelinglib/48x48/association.png"));
        m_item = new ModelItem(icon, m_treeModel->createRelationLabel(association));
        visitMRelation(association);
    }

private:
    TreeModel *m_treeModel;
    ModelItem *m_item;
};

class TreeModel::ItemUpdater : public MConstVisitor
{
public:
    ItemUpdater(TreeModel *treeModel, ModelItem *item)
        : m_treeModel(treeModel),
          m_item(item)
    {
        QMT_CHECK(m_treeModel);
        QMT_CHECK(m_item);
    }

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
        if (klass->stereotypes() != m_item->stereotypes()) {
            QIcon icon = m_treeModel->createIcon(StereotypeIcon::ElementClass, StyleEngine::TypeClass, klass->stereotypes(), QStringLiteral(":/modelinglib/48x48/class.png"));
            m_item->setIcon(icon);
            m_item->setStereotypes(klass->stereotypes());
        }
        visitMObject(klass);
    }

    void visitMComponent(const MComponent *component)
    {
        if (component->stereotypes() != m_item->stereotypes()) {
            QIcon icon = m_treeModel->createIcon(StereotypeIcon::ElementComponent, StyleEngine::TypeComponent, component->stereotypes(), QStringLiteral(":/modelinglib/48x48/component.png"));
            m_item->setIcon(icon);
            m_item->setStereotypes(component->stereotypes());
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
        QList<QString> stereotypes = item->stereotypes() << item->variety();
        if (stereotypes != m_item->stereotypes()) {
            QIcon icon = m_treeModel->createIcon(StereotypeIcon::ElementItem, StyleEngine::TypeItem, stereotypes, QStringLiteral(":/modelinglib/48x48/item.png"));
            m_item->setIcon(icon);
            m_item->setStereotypes(stereotypes);
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

    TreeModel *m_treeModel;
    ModelItem *m_item;
};

void TreeModel::ItemUpdater::updateObjectLabel(const MObject *object)
{
    QString label = m_treeModel->createObjectLabel(object);
    if (m_item->text() != label)
        m_item->setText(label);
}

void TreeModel::ItemUpdater::updateRelationLabel(const MRelation *relation)
{
    QString label = m_treeModel->createRelationLabel(relation);
    if (m_item->text() != label)
        m_item->setText(label);
}

TreeModel::TreeModel(QObject *parent)
    : QStandardItemModel(parent),
      m_modelController(0),
      m_stereotypeController(0),
      m_styleController(0),
      m_rootItem(0),
      m_busyState(NotBusy)
{
    connect(this, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(onModelDataChanged(QModelIndex,QModelIndex)));
}

TreeModel::~TreeModel()
{
    QMT_CHECK(m_busyState == NotBusy);
    disconnect();
    clear();
}

void TreeModel::setModelController(ModelController *modelController)
{
    if (m_modelController != modelController) {
        if (m_modelController)
            disconnect(m_modelController, 0, this, 0);
        m_modelController = modelController;
        if (m_modelController) {
            connect(m_modelController, SIGNAL(beginResetModel()), this, SLOT(onBeginResetModel()));
            connect(m_modelController, SIGNAL(endResetModel()), this, SLOT(onEndResetModel()));

            connect(m_modelController, SIGNAL(beginInsertObject(int,const MObject*)), this, SLOT(onBeginInsertObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(endInsertObject(int,const MObject*)), this, SLOT(onEndInsertObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(beginUpdateObject(int,const MObject*)), this, SLOT(onBeginUpdateObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(endUpdateObject(int,const MObject*)), this, SLOT(onEndUpdateObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(beginRemoveObject(int,const MObject*)), this, SLOT(onBeginRemoveObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(endRemoveObject(int,const MObject*)), this, SLOT(onEndRemoveObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(beginMoveObject(int,const MObject*)), this, SLOT(onBeginMoveObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(endMoveObject(int,const MObject*)), this, SLOT(onEndMoveObject(int,const MObject*)));

            connect(m_modelController, SIGNAL(beginInsertRelation(int,const MObject*)), this, SLOT(onBeginInsertRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(endInsertRelation(int,const MObject*)), this, SLOT(onEndInsertRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(beginUpdateRelation(int,const MObject*)), this, SLOT(onBeginUpdateRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(endUpdateRelation(int,const MObject*)), this, SLOT(onEndUpdateRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(beginRemoveRelation(int,const MObject*)), this, SLOT(onBeginRemoveRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(endRemoveRelation(int,const MObject*)), this, SLOT(onEndRemoveRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(beginMoveRelation(int,const MObject*)), this, SLOT(onBeginMoveRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(endMoveRelation(int,const MObject*)), this, SLOT(onEndMoveRelation(int,const MObject*)));

            connect(m_modelController, SIGNAL(relationEndChanged(MRelation*,MObject*)), this, SLOT(onRelationEndChanged(MRelation*,MObject*)));
        }
    }
}

void TreeModel::setStereotypeController(StereotypeController *stereotypeController)
{
    m_stereotypeController = stereotypeController;
}

void TreeModel::setStyleController(StyleController *styleController)
{
    m_styleController = styleController;
}

MElement *TreeModel::element(const QModelIndex &index) const
{
    QMT_CHECK(index.isValid());

    MElement *element = 0;
    QStandardItem *item = itemFromIndex(index);
    if (item) {
        if (item->parent()) {
            ModelItem *parentModelItem = dynamic_cast<ModelItem *>(item->parent());
            QMT_CHECK(parentModelItem);
            const MObject *parentObject = m_itemToObjectMap.value(parentModelItem);
            QMT_CHECK(parentObject);
            if (parentObject) {
                if (index.row() >= 0 && index.row() < parentObject->children().size()) {
                    element = parentObject->children().at(index.row());
                    QMT_CHECK(element);
                } else if (index.row() >= parentObject->children().size()
                           && index.row() < parentObject->children().size() + parentObject->relations().size()) {
                    element = parentObject->relations().at(index.row() - parentObject->children().size());
                    QMT_CHECK(element);
                } else {
                    QMT_CHECK(false);
                }
            }
        } else if (index.row() == 0) {
            element = m_modelController->rootPackage();
        } else {
            QMT_CHECK(false);
        }
    }
    return element;
}

QModelIndex TreeModel::indexOf(const MElement *element) const
{
    if (const MObject *object = dynamic_cast<const MObject *>(element)) {
        if (!object->owner()) {
            QMT_CHECK(element == m_modelController->rootPackage());
            return QStandardItemModel::index(0, 0);
        }
        MObject *parentObject = object->owner();
        ModelItem *item = m_objectToItemMap.value(parentObject);
        if (!item) {
            QMT_CHECK(false);
            return QModelIndex();
        }
        QModelIndex parentIndex = indexFromItem(item);
        int row = parentObject->children().indexOf(object);
        return QStandardItemModel::index(row, 0, parentIndex);
    } else if (const MRelation *relation = dynamic_cast<const MRelation *>(element)) {
        QMT_CHECK(relation->owner());
        MObject *owner = relation->owner();
        ModelItem *item = m_objectToItemMap.value(owner);
        if (!item) {
            QMT_CHECK(false);
            return QModelIndex();
        }
        QModelIndex parentIndex = indexFromItem(item);
        int row = owner->children().size() + owner->relations().indexOf(relation);
        return QStandardItemModel::index(row, 0, parentIndex);
    }
    return QModelIndex();
}

QIcon TreeModel::icon(const QModelIndex &index) const
{
    QStandardItem *item = itemFromIndex(index);
    if (item)
        return item->icon();
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
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = ResetModel;
    QStandardItemModel::beginResetModel();
}

void TreeModel::onEndResetModel()
{
    QMT_CHECK(m_busyState == ResetModel);
    clear();
    MPackage *rootPackage = m_modelController->rootPackage();
    if (m_modelController && rootPackage) {
        m_rootItem = createItem(rootPackage);
        appendRow(m_rootItem);
        createChildren(rootPackage, m_rootItem);
        QStandardItemModel::endResetModel();
    }
    m_busyState = NotBusy;
}

void TreeModel::onBeginUpdateObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = UpdateElement;
}

void TreeModel::onEndUpdateObject(int row, const MObject *parent)
{
    QMT_CHECK(m_busyState == UpdateElement);
    QModelIndex parentIndex;
    if (parent) {
        QMT_CHECK(m_objectToItemMap.contains(parent));
        ModelItem  *parentItem = m_objectToItemMap.value(parent);
        QMT_CHECK(parentItem);
        parentIndex = indexFromItem(parentItem);
    }
    // reflect updated element in standard item
    QModelIndex elementIndex = this->QStandardItemModel::index(row, 0, parentIndex);
    MElement *element = TreeModel::element(elementIndex);
    if (element) {
        MObject *object = dynamic_cast<MObject *>(element);
        if (object) {
            ModelItem *item = dynamic_cast<ModelItem *>(itemFromIndex(elementIndex));
            QMT_CHECK(item);
            ItemUpdater visitor(this, item);
            element->accept(&visitor);
        }
    }
    m_busyState = NotBusy;
    emit dataChanged(QStandardItemModel::index(row, 0, parentIndex), QStandardItemModel::index(row, 0, parentIndex));
}

void TreeModel::onBeginInsertObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = InsertElement;
}

void TreeModel::onEndInsertObject(int row, const MObject *parent)
{
    QMT_CHECK(m_busyState == InsertElement);
    ModelItem *parentItem =m_objectToItemMap.value(parent);
    QMT_CHECK(parentItem);
    MObject *object = parent->children().at(row);
    ModelItem *item = createItem(object);
    parentItem->insertRow(row, item);
    createChildren(object, item);
    m_busyState = NotBusy;
}

void TreeModel::onBeginRemoveObject(int row, const MObject *parent)
{
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = RemoveElement;
    QMT_CHECK(parent);
    MObject *object = parent->children().at(row);
    if (object)
        removeObjectFromItemMap(object);
    ModelItem *parentItem = m_objectToItemMap.value(parent);
    QMT_CHECK(parentItem);
    parentItem->removeRow(row);
}

void TreeModel::onEndRemoveObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    QMT_CHECK(m_busyState == RemoveElement);
    m_busyState = NotBusy;
}

void TreeModel::onBeginMoveObject(int formerRow, const MObject *formerOwner)
{
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = MoveElement;
    QMT_CHECK(formerOwner);
    MObject *object = formerOwner->children().at(formerRow);
    if (object)
        removeObjectFromItemMap(object);
    ModelItem *parentItem = m_objectToItemMap.value(formerOwner);
    QMT_CHECK(parentItem);
    parentItem->removeRow(formerRow);
}

void TreeModel::onEndMoveObject(int row, const MObject *owner)
{
    QMT_CHECK(m_busyState == MoveElement);
    ModelItem *parentItem =m_objectToItemMap.value(owner);
    QMT_CHECK(parentItem);
    MObject *object = owner->children().at(row);
    ModelItem *item = createItem(object);
    parentItem->insertRow(row, item);
    createChildren(object, item);
    m_busyState = NotBusy;
}

void TreeModel::onBeginUpdateRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = UpdateRelation;
}

void TreeModel::onEndUpdateRelation(int row, const MObject *parent)
{
    QMT_CHECK(parent);
    QMT_CHECK(m_busyState == UpdateRelation);

    QMT_CHECK(m_objectToItemMap.contains(parent));
    ModelItem *parentItem = m_objectToItemMap.value(parent);
    QMT_CHECK(parentItem);
    QModelIndex parentIndex = indexFromItem(parentItem);

    // reflect updated relation in standard item
    row += parent->children().size();
    QModelIndex elementIndex = QStandardItemModel::index(row, 0, parentIndex);
    MElement *element = TreeModel::element(elementIndex);
    if (element) {
        MRelation *relation = dynamic_cast<MRelation *>(element);
        if (relation) {
            ModelItem *item = dynamic_cast<ModelItem *>(itemFromIndex(elementIndex));
            QMT_CHECK(item);
            ItemUpdater visitor(this, item);
            element->accept(&visitor);
        }
    }
    m_busyState = NotBusy;
    emit dataChanged(QStandardItemModel::index(row, 0, parentIndex), QStandardItemModel::index(row, 0, parentIndex));
}

void TreeModel::onBeginInsertRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = InsertRelation;
}

void TreeModel::onEndInsertRelation(int row, const MObject *parent)
{
    QMT_CHECK(parent);
    QMT_CHECK(m_busyState == InsertRelation);
    ModelItem *parentItem =m_objectToItemMap.value(parent);
    QMT_CHECK(parentItem);
    MRelation *relation = parent->relations().at(row);
    ModelItem *item = createItem(relation);
    parentItem->insertRow(parent->children().size() + row, item);
    m_busyState = NotBusy;
}

void TreeModel::onBeginRemoveRelation(int row, const MObject *parent)
{
    QMT_CHECK(parent);
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = RemoveRelation;
    QMT_CHECK(parent->relations().at(row));
    ModelItem *parentItem = m_objectToItemMap.value(parent);
    QMT_CHECK(parentItem);
    parentItem->removeRow(parent->children().size() + row);
}

void TreeModel::onEndRemoveRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    QMT_CHECK(m_busyState == RemoveRelation);
    m_busyState = NotBusy;
}

void TreeModel::onBeginMoveRelation(int formerRow, const MObject *formerOwner)
{
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = MoveElement;
    QMT_CHECK(formerOwner);
    QMT_CHECK(formerOwner->relations().at(formerRow));
    ModelItem *parentItem = m_objectToItemMap.value(formerOwner);
    QMT_CHECK(parentItem);
    parentItem->removeRow(formerOwner->children().size() + formerRow);
}

void TreeModel::onEndMoveRelation(int row, const MObject *owner)
{
    QMT_CHECK(owner);
    QMT_CHECK(m_busyState == MoveElement);
    ModelItem *parentItem =m_objectToItemMap.value(owner);
    QMT_CHECK(parentItem);
    MRelation *relation = owner->relations().at(row);
    ModelItem *item = createItem(relation);
    parentItem->insertRow(owner->children().size() + row, item);
    m_busyState = NotBusy;
}

void TreeModel::onRelationEndChanged(MRelation *relation, MObject *endObject)
{
    Q_UNUSED(endObject);
    QMT_CHECK(m_busyState == NotBusy);

    MObject *parent = relation->owner();
    QMT_CHECK(parent);
    QMT_CHECK(m_objectToItemMap.contains(parent));
    ModelItem *parentItem = m_objectToItemMap.value(parent);
    QMT_CHECK(parentItem);
    QModelIndex parentIndex = indexFromItem(parentItem);

    int row = parent->children().size() + relation->owner()->relations().indexOf(relation);
    QModelIndex elementIndex = QStandardItemModel::index(row, 0, parentIndex);
    QMT_CHECK(elementIndex.isValid());

    ModelItem *item = dynamic_cast<ModelItem *>(itemFromIndex(elementIndex));
    QMT_CHECK(item);

    QString label = createRelationLabel(relation);
    if (item->text() != label)
        item->setText(label);

    emit dataChanged(QStandardItemModel::index(row, 0, parentIndex), QStandardItemModel::index(row, 0, parentIndex));
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
    m_rootItem = 0;
    m_objectToItemMap.clear();
    m_itemToObjectMap.clear();
}

TreeModel::ModelItem *TreeModel::createItem(const MElement *element)
{
    ItemFactory visitor(this);
    element->accept(&visitor);
    QMT_CHECK(visitor.product());
    return visitor.product();
}

void TreeModel::createChildren(const MObject *parentObject, ModelItem *parentItem)
{
    QMT_CHECK(!m_objectToItemMap.contains(parentObject));
    m_objectToItemMap.insert(parentObject, parentItem);
    QMT_CHECK(!m_itemToObjectMap.contains(parentItem));
    m_itemToObjectMap.insert(parentItem, parentObject);
    foreach (const Handle<MObject> &object, parentObject->children()) {
        if (object.hasTarget()) {
            ModelItem *item = createItem(object.target());
            parentItem->appendRow(item);
            createChildren(object.target(), item);
        }
    }
    foreach (const Handle<MRelation> &handle, parentObject->relations()) {
        if (handle.hasTarget()) {
            MRelation *relation = handle.target();
            ModelItem *item = createItem(relation);
            parentItem->appendRow(item);
        }
    }
}

void TreeModel::removeObjectFromItemMap(const MObject *object)
{
    QMT_CHECK(object);
    QMT_CHECK(m_objectToItemMap.contains(object));
    ModelItem *item = m_objectToItemMap.value(object);
    QMT_CHECK(item);
    QMT_CHECK(m_itemToObjectMap.contains(item));
    m_itemToObjectMap.remove(item);
    m_objectToItemMap.remove(object);
    foreach (const Handle<MObject> &child, object->children()) {
        if (child.hasTarget())
            removeObjectFromItemMap(child.target());
    }
}

QString TreeModel::createObjectLabel(const MObject *object)
{
    QMT_CHECK(object);

    if (object->name().isEmpty()) {
        if (const MItem *item = dynamic_cast<const MItem *>(object)) {
            if (!item->variety().isEmpty())
                return QString(QStringLiteral("[%1]")).arg(item->variety());
        }
        return tr("[unnamed]");
    }

    if (const MClass *klass = dynamic_cast<const MClass *>(object)) {
        if (!klass->umlNamespace().isEmpty())
            return QString(QStringLiteral("%1 [%2]")).arg(klass->name()).arg(klass->umlNamespace());
    }
    return object->name();
}

QString TreeModel::createRelationLabel(const MRelation *relation)
{
    QString name;
    if (!relation->name().isEmpty()) {
        name += relation->name();
        name += QStringLiteral(": ");
    }
    if (MObject *endA = m_modelController->findObject(relation->endAUid()))
        name += createObjectLabel(endA);
    name += QStringLiteral(" - ");
    if (MObject *endB = m_modelController->findObject(relation->endBUid()))
        name += createObjectLabel(endB);
    return name;
}

QIcon TreeModel::createIcon(StereotypeIcon::Element stereotypeIconElement, StyleEngine::ElementType styleElementType, const QStringList &stereotypes, const QString &defaultIconPath)
{
    const Style *style = m_styleController->adaptStyle(styleElementType);
    return m_stereotypeController->createIcon(stereotypeIconElement, stereotypes, defaultIconPath, style,
                                              QSize(48, 48), QMarginsF(3.0, 2.0, 3.0, 4.0));
}

} // namespace qmt
