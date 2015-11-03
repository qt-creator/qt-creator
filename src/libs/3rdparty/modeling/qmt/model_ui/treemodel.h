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

#ifndef QMT_TREEMODEL_H
#define QMT_TREEMODEL_H

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


class QMT_EXPORT TreeModel :
        public QStandardItemModel
{
    Q_OBJECT

    class ModelItem;
    class ItemFactory;
    class ItemUpdater;

public:

    enum ItemType {
        PACKAGE,
        DIAGRAM,
        ELEMENT,
        RELATION
    };

    enum Roles {
        ROLE_ITEM_TYPE = Qt::UserRole + 1
    };

public:
    explicit TreeModel(QObject *parent = 0);

    ~TreeModel();

public:

    ModelController *getModelController() const { return m_modelController; }

    void setModelController(ModelController *model_controller);

    StereotypeController *getStereotypeController() const { return m_stereotypeController; }

    void setStereotypeController(StereotypeController *stereotype_controller);

    StyleController *getStyleController() const { return m_styleController; }

    void setStyleController(StyleController *style_controller);

    MElement *getElement(const QModelIndex &index) const;

    QModelIndex getIndex(const MElement *element) const;

    QIcon getIcon(const QModelIndex &index) const;

public:

    Qt::DropActions supportedDropActions() const;

    QStringList mimeTypes() const;

private slots:

    void onBeginResetModel();

    void onEndResetModel();

    void onBeginUpdateObject(int row, const MObject *parent);

    void onEndUpdateObject(int row, const MObject *parent);

    void onBeginInsertObject(int row, const MObject *parent);

    void onEndInsertObject(int row, const MObject *parent);

    void onBeginRemoveObject(int row, const MObject *parent);

    void onEndRemoveObject(int row, const MObject *parent);

    void onBeginMoveObject(int former_row, const MObject *former_owner);

    void onEndMoveObject(int row, const MObject *owner);

    void onBeginUpdateRelation(int row, const MObject *parent);

    void onEndUpdateRelation(int row, const MObject *parent);

    void onBeginInsertRelation(int row, const MObject *parent);

    void onEndInsertRelation(int row, const MObject *parent);

    void onBeginRemoveRelation(int row, const MObject *parent);

    void onEndRemoveRelation(int row, const MObject *parent);

    void onBeginMoveRelation(int former_row, const MObject *former_owner);

    void onEndMoveRelation(int row, const MObject *owner);

    void onRelationEndChanged(MRelation *relation, MObject *end_object);

    void onModelDataChanged(const QModelIndex &topleft, const QModelIndex &bottomright);

private:

    void clear();

    ModelItem *createItem(const MElement *element);

    void createChildren(const MObject *parent_object, ModelItem *parent_item);

    void removeObjectFromItemMap(const MObject *object);

    QString createObjectLabel(const MObject *object);

    QString createRelationLabel(const MRelation *relation);

    QIcon createIcon(StereotypeIcon::Element stereotype_icon_element, StyleEngine::ElementType style_element_type, const QStringList &stereotypes, const QString &default_icon_path);

private:
    enum Busy {
        NOT_BUSY,
        RESET_MODEL,
        UPDATE_ELEMENT,
        INSERT_ELEMENT,
        REMOVE_ELEMENT,
        MOVE_ELEMENT,
        UPDATE_DIAGRAM,
        INSERT_DIAGRAM,
        REMOVE_DIAGRAM,
        MOVE_DIAGRAM,
        UPDATE_RELATION,
        INSERT_RELATION,
        REMOVE_RELATION,
        MOVE_RELATION
    };

private:

    ModelController *m_modelController;

    StereotypeController *m_stereotypeController;

    StyleController *m_styleController;

    ModelItem *m_rootItem;

    QHash<const MObject *, ModelItem *> m_objectToItemMap;

    QHash<ModelItem *, const MObject *> m_itemToObjectMap;

    Busy m_busy;
};

}

#endif // QMT_TREEMODEL_H
