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
#include "qmt/model/mconnection.h"
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
    explicit ItemFactory(TreeModel *treeModel)
        : m_treeModel(treeModel)
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
        QMT_ASSERT(m_item, return);
        m_item->setEditable(false);
    }

    void visitMPackage(const MPackage *package)
    {
        QMT_CHECK(!m_item);

        static QIcon icon(":/modelinglib/48x48/package.png");
        m_item = new ModelItem(icon, m_treeModel->createObjectLabel(package));
        m_item->setData(TreeModel::Package, TreeModel::RoleItemType);
        visitMObject(package);
    }

    void visitMClass(const MClass *klass)
    {
        QMT_CHECK(!m_item);

        QIcon icon = m_treeModel->createIcon(StereotypeIcon::ElementClass, StyleEngine::TypeClass, klass->stereotypes(),
                                             ":/modelinglib/48x48/class.png");
        m_item = new ModelItem(icon, m_treeModel->createObjectLabel(klass));
        m_item->setData(TreeModel::Element, TreeModel::RoleItemType);
        m_item->setStereotypes(klass->stereotypes());
        visitMObject(klass);
    }

    void visitMComponent(const MComponent *component)
    {
        QMT_CHECK(!m_item);

        QIcon icon = m_treeModel->createIcon(StereotypeIcon::ElementComponent, StyleEngine::TypeComponent,
                                             component->stereotypes(),
                                             ":/modelinglib/48x48/component.png");
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

        static QIcon icon(":/modelinglib/48x48/canvas-diagram.png");
        m_item = new ModelItem(icon, m_treeModel->createObjectLabel(diagram));
        visitMDiagram(diagram);
    }

    void visitMItem(const MItem *item)
    {
        QMT_CHECK(!m_item);

        QList<QString> stereotypes = item->stereotypes() << item->variety();
        QIcon icon = m_treeModel->createIcon(StereotypeIcon::ElementItem, StyleEngine::TypeItem, stereotypes,
                                             ":/modelinglib/48x48/item.png");
        m_item = new ModelItem(icon, m_treeModel->createObjectLabel(item));
        m_item->setData(TreeModel::Element, TreeModel::RoleItemType);
        m_item->setStereotypes(stereotypes);
        visitMObject(item);
    }

    void visitMRelation(const MRelation *relation)
    {
        Q_UNUSED(relation);
        QMT_ASSERT(m_item, return);
        m_item->setEditable(false);
        m_item->setData(TreeModel::Relation, TreeModel::RoleItemType);
    }

    void visitMDependency(const MDependency *dependency)
    {
        QMT_CHECK(!m_item);

        static QIcon icon(":/modelinglib/48x48/dependency.png");
        m_item = new ModelItem(icon, m_treeModel->createRelationLabel(dependency));
        visitMRelation(dependency);
    }

    void visitMInheritance(const MInheritance *inheritance)
    {
        QMT_CHECK(!m_item);

        static QIcon icon(":/modelinglib/48x48/inheritance.png");
        m_item = new ModelItem(icon, m_treeModel->createRelationLabel(inheritance));
        visitMRelation(inheritance);
    }

    void visitMAssociation(const MAssociation *association)
    {
        QMT_CHECK(!m_item);

        static QIcon icon(":/modelinglib/48x48/association.png");
        m_item = new ModelItem(icon, m_treeModel->createRelationLabel(association));
        visitMRelation(association);
    }

    void visitMConnection(const MConnection *connection)
    {
        QMT_CHECK(!m_item);

        static QIcon icon(":modelinglib/48x48/connection.ong");
        m_item = new ModelItem(icon, m_treeModel->createRelationLabel(connection));
        visitMRelation(connection);
    }

private:
    TreeModel *m_treeModel = nullptr;
    TreeModel::ModelItem *m_item = nullptr;
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
            QIcon icon = m_treeModel->createIcon(StereotypeIcon::ElementClass, StyleEngine::TypeClass,
                                                 klass->stereotypes(), ":/modelinglib/48x48/class.png");
            m_item->setIcon(icon);
            m_item->setStereotypes(klass->stereotypes());
        }
        visitMObject(klass);
    }

    void visitMComponent(const MComponent *component)
    {
        if (component->stereotypes() != m_item->stereotypes()) {
            QIcon icon = m_treeModel->createIcon(StereotypeIcon::ElementComponent, StyleEngine::TypeComponent,
                                                 component->stereotypes(),
                                                 ":/modelinglib/48x48/component.png");
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
            QIcon icon = m_treeModel->createIcon(StereotypeIcon::ElementItem, StyleEngine::TypeItem, stereotypes,
                                                 ":/modelinglib/48x48/item.png");
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

    void visitMConnection(const MConnection *connection)
    {
        visitMRelation(connection);
    }

private:
    void updateObjectLabel(const MObject *object);
    void updateRelationLabel(const MRelation *relation);

    TreeModel *m_treeModel = nullptr;
    TreeModel::ModelItem *m_item = nullptr;
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

TreeModel::TreeModel(QObject *parent) : QStandardItemModel(parent)
{
    connect(this, &QAbstractItemModel::dataChanged,
            this, &TreeModel::onModelDataChanged);
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
            disconnect(m_modelController, nullptr, this, nullptr);
        m_modelController = modelController;
        if (m_modelController) {
            connect(m_modelController, &ModelController::beginResetModel,
                    this, &TreeModel::onBeginResetModel);
            connect(m_modelController, &ModelController::endResetModel,
                    this, &TreeModel::onEndResetModel);

            connect(m_modelController, &ModelController::beginInsertObject,
                    this, &TreeModel::onBeginInsertObject);
            connect(m_modelController, &ModelController::endInsertObject,
                    this, &TreeModel::onEndInsertObject);
            connect(m_modelController, &ModelController::beginUpdateObject,
                    this, &TreeModel::onBeginUpdateObject);
            connect(m_modelController, &ModelController::endUpdateObject,
                    this, &TreeModel::onEndUpdateObject);
            connect(m_modelController, &ModelController::beginRemoveObject,
                    this, &TreeModel::onBeginRemoveObject);
            connect(m_modelController, &ModelController::endRemoveObject,
                    this, &TreeModel::onEndRemoveObject);
            connect(m_modelController, &ModelController::beginMoveObject,
                    this, &TreeModel::onBeginMoveObject);
            connect(m_modelController, &ModelController::endMoveObject,
                    this, &TreeModel::onEndMoveObject);

            connect(m_modelController, &ModelController::beginInsertRelation,
                    this, &TreeModel::onBeginInsertRelation);
            connect(m_modelController, &ModelController::endInsertRelation,
                    this, &TreeModel::onEndInsertRelation);
            connect(m_modelController, &ModelController::beginUpdateRelation,
                    this, &TreeModel::onBeginUpdateRelation);
            connect(m_modelController, &ModelController::endUpdateRelation,
                    this, &TreeModel::onEndUpdateRelation);
            connect(m_modelController, &ModelController::beginRemoveRelation,
                    this, &TreeModel::onBeginRemoveRelation);
            connect(m_modelController, &ModelController::endRemoveRelation,
                    this, &TreeModel::onEndRemoveRelation);
            connect(m_modelController, &ModelController::beginMoveRelation,
                    this, &TreeModel::onBeginMoveRelation);
            connect(m_modelController, &ModelController::endMoveRelation,
                    this, &TreeModel::onEndMoveRelation);

            connect(m_modelController, &ModelController::relationEndChanged,
                    this, &TreeModel::onRelationEndChanged);
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

    MElement *element = nullptr;
    QStandardItem *item = itemFromIndex(index);
    if (item) {
        if (item->parent()) {
            auto parentModelItem = dynamic_cast<ModelItem *>(item->parent());
            QMT_ASSERT(parentModelItem, return nullptr);
            const MObject *parentObject = m_itemToObjectMap.value(parentModelItem);
            QMT_ASSERT(parentObject, return nullptr);
            if (parentObject) {
                if (index.row() >= 0 && index.row() < parentObject->children().size()) {
                    element = parentObject->children().at(index.row());
                    QMT_ASSERT(element, return nullptr);
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
    if (auto object = dynamic_cast<const MObject *>(element)) {
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
    } else if (auto relation = dynamic_cast<const MRelation *>(element)) {
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
    return QStringList(QString("text/model-elements"));
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
    MPackage *rootPackage = m_modelController ? m_modelController->rootPackage() : nullptr;
    if (rootPackage) {
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
        auto object = dynamic_cast<MObject *>(element);
        if (object) {
            auto item = dynamic_cast<ModelItem *>(itemFromIndex(elementIndex));
            QMT_ASSERT(item, return);
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
    QMT_ASSERT(parentItem, return);
    MObject *object = parent->children().at(row);
    ModelItem *item = createItem(object);
    parentItem->insertRow(row, item);
    createChildren(object, item);
    m_busyState = NotBusy;
}

void TreeModel::onBeginRemoveObject(int row, const MObject *parent)
{
    QMT_CHECK(m_busyState == NotBusy);
    QMT_ASSERT(parent, return);
    m_busyState = RemoveElement;
    MObject *object = parent->children().at(row);
    if (object)
        removeObjectFromItemMap(object);
    ModelItem *parentItem = m_objectToItemMap.value(parent);
    QMT_ASSERT(parentItem, return);
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
    QMT_ASSERT(formerOwner, return);
    m_busyState = MoveElement;
    MObject *object = formerOwner->children().at(formerRow);
    if (object)
        removeObjectFromItemMap(object);
    ModelItem *parentItem = m_objectToItemMap.value(formerOwner);
    QMT_ASSERT(parentItem, return);
    parentItem->removeRow(formerRow);
}

void TreeModel::onEndMoveObject(int row, const MObject *owner)
{
    QMT_CHECK(m_busyState == MoveElement);
    ModelItem *parentItem =m_objectToItemMap.value(owner);
    QMT_ASSERT(parentItem, return);
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
    QMT_ASSERT(parent, return);
    QMT_CHECK(m_busyState == UpdateRelation);

    QMT_CHECK(m_objectToItemMap.contains(parent));
    ModelItem *parentItem = m_objectToItemMap.value(parent);
    QMT_ASSERT(parentItem, return);
    QModelIndex parentIndex = indexFromItem(parentItem);

    // reflect updated relation in standard item
    row += parent->children().size();
    QModelIndex elementIndex = QStandardItemModel::index(row, 0, parentIndex);
    MElement *element = TreeModel::element(elementIndex);
    if (element) {
        auto relation = dynamic_cast<MRelation *>(element);
        if (relation) {
            auto item = dynamic_cast<ModelItem *>(itemFromIndex(elementIndex));
            QMT_ASSERT(item, return);
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
    QMT_ASSERT(parent, return);
    QMT_CHECK(m_busyState == InsertRelation);
    ModelItem *parentItem =m_objectToItemMap.value(parent);
    QMT_ASSERT(parentItem, return);
    MRelation *relation = parent->relations().at(row);
    ModelItem *item = createItem(relation);
    parentItem->insertRow(parent->children().size() + row, item);
    m_busyState = NotBusy;
}

void TreeModel::onBeginRemoveRelation(int row, const MObject *parent)
{
    QMT_ASSERT(parent, return);
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = RemoveRelation;
    QMT_CHECK(parent->relations().at(row));
    ModelItem *parentItem = m_objectToItemMap.value(parent);
    QMT_ASSERT(parentItem, return);
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
    QMT_ASSERT(formerOwner, return);
    m_busyState = MoveElement;
    QMT_CHECK(formerOwner->relations().at(formerRow));
    ModelItem *parentItem = m_objectToItemMap.value(formerOwner);
    QMT_ASSERT(parentItem, return);
    parentItem->removeRow(formerOwner->children().size() + formerRow);
}

void TreeModel::onEndMoveRelation(int row, const MObject *owner)
{
    QMT_ASSERT(owner, return);
    QMT_CHECK(m_busyState == MoveElement);
    ModelItem *parentItem =m_objectToItemMap.value(owner);
    QMT_ASSERT(parentItem, return);
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
    QMT_ASSERT(parent, return);
    QMT_CHECK(m_objectToItemMap.contains(parent));
    ModelItem *parentItem = m_objectToItemMap.value(parent);
    QMT_ASSERT(parentItem, return);
    QModelIndex parentIndex = indexFromItem(parentItem);

    int row = parent->children().size() + relation->owner()->relations().indexOf(relation);
    QModelIndex elementIndex = QStandardItemModel::index(row, 0, parentIndex);
    QMT_CHECK(elementIndex.isValid());

    auto item = dynamic_cast<ModelItem *>(itemFromIndex(elementIndex));
    QMT_ASSERT(item, return);

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
    m_rootItem = nullptr;
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
    QMT_ASSERT(object, return);
    QMT_CHECK(m_objectToItemMap.contains(object));
    ModelItem *item = m_objectToItemMap.value(object);
    QMT_ASSERT(item, return);
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
    QMT_ASSERT(object, return QString());

    if (object->name().isEmpty()) {
        if (auto item = dynamic_cast<const MItem *>(object)) {
            if (!item->variety().isEmpty())
                return QString("[%1]").arg(item->variety());
        }
        return tr("[unnamed]");
    }

    if (auto klass = dynamic_cast<const MClass *>(object)) {
        if (!klass->umlNamespace().isEmpty())
            return QString("%1 [%2]").arg(klass->name()).arg(klass->umlNamespace());
    }
    return object->name();
}

QString TreeModel::createRelationLabel(const MRelation *relation)
{
    QString name;
    if (!relation->name().isEmpty()) {
        name += relation->name();
        name += ": ";
    }
    if (MObject *endA = m_modelController->findObject(relation->endAUid()))
        name += createObjectLabel(endA);
    name += " - ";
    if (MObject *endB = m_modelController->findObject(relation->endBUid()))
        name += createObjectLabel(endB);
    return name;
}

QIcon TreeModel::createIcon(StereotypeIcon::Element stereotypeIconElement, StyleEngine::ElementType styleElementType,
                            const QStringList &stereotypes, const QString &defaultIconPath)
{
    const Style *style = m_styleController->adaptStyle(styleElementType);
    return m_stereotypeController->createIcon(stereotypeIconElement, stereotypes, defaultIconPath, style,
                                              QSize(48, 48), QMarginsF(3.0, 2.0, 3.0, 4.0));
}

} // namespace qmt
