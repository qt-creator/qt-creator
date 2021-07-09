/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#pragma once

#include "annotation.h"

#include "modelnode.h"

#include <utils/itemviews.h>

#include <QWidget>
#include <QStandardItemModel>
#include <QListView>
#include <QStyledItemDelegate>

#include <tuple>
#include <vector>


namespace QmlDesigner {

class AnnotationListView;

//structure is used as a storage for the model data
//id, annotationname and annotation are storages, that will be submited into node on save
struct AnnotationListEntry {
    QString id;
    QString annotationName;
    Annotation annotation;

    ModelNode node;

    AnnotationListEntry() = default;
    AnnotationListEntry(const ModelNode &modelnode);
    AnnotationListEntry(const QString &argId, const QString &argAnnotationName,
                        const Annotation &argAnnotation, const ModelNode &argNode);
};

class AnnotationListModel final : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum ColumnRoles {
        IdRow = Qt::UserRole,
        NameRow,
        AnnotationsCountRow
    };

    AnnotationListModel(ModelNode rootNode, AnnotationListView *view = nullptr);
    ~AnnotationListModel() = default;

    void setRootNode(ModelNode rootNode);

    void resetModel();

    ModelNode getModelNode(int id) const;
    AnnotationListEntry getStoredAnnotation(int id) const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void storeChanges(int row, const QString &customId, const Annotation &annotation);
    void saveChanges();

private:
    void fillModel();

private:
    QListView *m_annotationView = nullptr;

    ModelNode m_rootNode;
    std::vector<AnnotationListEntry> m_annoList;

    const int m_numberOfColumns = 3;
};

class AnnotationListView final : public Utils::ListView
{
    Q_OBJECT
public:
    explicit AnnotationListView(ModelNode rootNode, QWidget *parent = nullptr);
    ~AnnotationListView() = default;

    void setRootNode(ModelNode rootNode);

    ModelNode getModelNodeByAnnotationId(int id) const;
    AnnotationListEntry getStoredAnnotationById(int id) const;

    void selectRow(int row);
    int rowCount() const;

    void storeChangesInModel(int row, const QString &customId, const Annotation &annotation);
    void saveChangesFromModel();

private:
    AnnotationListModel *m_model = nullptr;
};

class AnnotationListDelegate final : public QStyledItemDelegate
{
    Q_OBJECT

public:
    AnnotationListDelegate(QObject *parent = nullptr);

private:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

}

