// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStandardItemModel>
#include "qmt/infrastructure/qmt_global.h"

#include <qmt/stereotype/stereotypeicon.h>
#include <qmt/style/styleengine.h>

#include <QScopedPointer>
#include <QHash>

QT_BEGIN_NAMESPACE
class QIcon;
QT_END_NAMESPACE

namespace qmt {

class MElement;
class MObject;
class MDiagram;
class MRelation;
class ModelController;
class StereotypeController;
class StyleController;

class QMT_EXPORT TreeModel : public QStandardItemModel
{
    Q_OBJECT

    class ModelItem;
    class ItemFactory;
    class ItemUpdater;

public:
    enum ItemType {
        Package,
        Diagram,
        Element,
        Relation
    };

    enum Roles {
        RoleItemType = Qt::UserRole + 1
    };

    explicit TreeModel(QObject *parent = nullptr);
    ~TreeModel() override;

    ModelController *modelController() const { return m_modelController; }
    void setModelController(ModelController *modelController);
    StereotypeController *stereotypeController() const { return m_stereotypeController; }
    void setStereotypeController(StereotypeController *stereotypeController);
    StyleController *styleController() const { return m_styleController; }
    void setStyleController(StyleController *styleController);

    MElement *element(const QModelIndex &index) const;
    QModelIndex indexOf(const MElement *element) const;
    QIcon icon(const QModelIndex &index) const;

    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;

private:
    void onBeginResetModel();
    void onEndResetModel();
    void onBeginUpdateObject(int row, const MObject *parent);
    void onEndUpdateObject(int row, const MObject *parent);
    void onBeginInsertObject(int row, const MObject *parent);
    void onEndInsertObject(int row, const MObject *parent);
    void onBeginRemoveObject(int row, const MObject *parent);
    void onEndRemoveObject(int row, const MObject *parent);
    void onBeginMoveObject(int formerRow, const MObject *formerOwner);
    void onEndMoveObject(int row, const MObject *owner);
    void onBeginUpdateRelation(int row, const MObject *parent);
    void onEndUpdateRelation(int row, const MObject *parent);
    void onBeginInsertRelation(int row, const MObject *parent);
    void onEndInsertRelation(int row, const MObject *parent);
    void onBeginRemoveRelation(int row, const MObject *parent);
    void onEndRemoveRelation(int row, const MObject *parent);
    void onBeginMoveRelation(int formerRow, const MObject *formerOwner);
    void onEndMoveRelation(int row, const MObject *owner);
    void onRelationEndChanged(MRelation *relation, MObject *endObject);
    void onModelDataChanged(const QModelIndex &topleft, const QModelIndex &bottomright);

    void clear();
    ModelItem *createItem(const MElement *element);
    void createChildren(const MObject *parentObject, ModelItem *parentItem);
    void removeObjectFromItemMap(const MObject *object);
    QString filterLabel(const QString &label) const;
    QString createObjectLabel(const MObject *object);
    QString createRelationLabel(const MRelation *relation);
    QIcon createIcon(StereotypeIcon::Element stereotypeIconElement,
                     StyleEngine::ElementType styleElementType, const QStringList &stereotypes,
                     const QString &defaultIconPath);

    enum Busy {
        NotBusy,
        ResetModel,
        UpdateElement,
        InsertElement,
        RemoveElement,
        MoveElement,
        UpdateDiagram,
        InsertDiagram,
        RemoveDiagram,
        MoveDiagram,
        UpdateRelation,
        InsertRelation,
        RemoveRelation,
        MoveRelation
    };

    ModelController *m_modelController = nullptr;
    StereotypeController *m_stereotypeController = nullptr;
    StyleController *m_styleController = nullptr;
    ModelItem *m_rootItem = nullptr;
    QHash<const MObject *, ModelItem *> m_objectToItemMap;
    QHash<ModelItem *, const MObject *> m_itemToObjectMap;
    Busy m_busyState = NotBusy;
};

} // namespace qmt
