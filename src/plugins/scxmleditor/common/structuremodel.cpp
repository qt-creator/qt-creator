/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "structuremodel.h"
#include "scxmldocument.h"
#include "scxmltag.h"

#include <QMimeData>
#include <QUndoStack>

using namespace ScxmlEditor::PluginInterface;
using namespace ScxmlEditor::Common;

StructureModel::StructureModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_icons.addIcon(State, QIcon(":/scxmleditor/images/state.png"));
    m_icons.addIcon(Parallel, QIcon(":/scxmleditor/images/parallel.png"));
    m_icons.addIcon(Initial, QIcon(":/scxmleditor/images/initial.png"));
    m_icons.addIcon(Final, QIcon(":/scxmleditor/images/final.png"));
}

void StructureModel::setDocument(ScxmlDocument *document)
{
    beginResetModel();
    if (m_document)
        disconnect(m_document, 0, this, 0);

    m_document = document;
    if (m_document) {
        connect(m_document.data(), &ScxmlDocument::beginTagChange, this, &StructureModel::beginTagChange);
        connect(m_document.data(), &ScxmlDocument::endTagChange, this, &StructureModel::endTagChange);
    }
    endResetModel();
}

int StructureModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

bool StructureModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || value.toString().isEmpty())
        return false;

    ScxmlTag *tag = getItem(index);
    if (!tag || tag->tagType() > MetadataItem)
        return false;

    tag->setTagName(value.toString());
    emit dataChanged(index, index);
    m_document->setCurrentTag(tag);

    return true;
}

QVariant StructureModel::data(const QModelIndex &index, int role) const
{
    ScxmlTag *tag = getItem(index);
    if (!tag)
        return QVariant();

    switch (role) {
    case TagTypeRole:
        return tag->tagType();
    case Qt::DecorationRole:
        return m_icons.icon(tag->tagType());
    case Qt::DisplayRole: {
        switch (tag->tagType()) {
        case State:
        case Parallel:
        case Initial:
        case Final:
            if (tag->hasAttribute("id"))
                return tag->attribute("id");
            break;
        case Transition:
            if (tag->hasAttribute("event"))
                return tag->attribute("event");
            break;
        default:
            break;
        }

        return tag->tagName();
    }
    case Qt::EditRole:
        return tag->tagName(false);

    default:
        break;
    }

    return QVariant();
}

ScxmlTag *StructureModel::getItem(const QModelIndex &parent, int source_row) const
{
    const ScxmlTag *tag = getItem(parent);
    if (tag)
        return tag->child(source_row);

    return nullptr;
}

ScxmlTag *StructureModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        auto tag = static_cast<ScxmlTag*>(index.internalPointer());
        if (tag)
            return tag;
    }
    return m_document ? m_document->rootTag() : nullptr;
}

QModelIndex StructureModel::index(int row, int column, const QModelIndex &index) const
{
    if (!index.isValid() && m_document)
        return createIndex(0, 0, m_document->rootTag());

    const ScxmlTag *tag = getItem(index);
    if (tag) {
        ScxmlTag *childTag = tag->child(row);
        if (childTag)
            return createIndex(row, column, childTag);
    }

    return QModelIndex();
}

QModelIndex StructureModel::parent(const QModelIndex &index) const
{
    if (!m_document)
        return QModelIndex();

    if (!index.isValid())
        return QModelIndex();

    const ScxmlTag *child = getItem(index);
    if (child && child != m_document->rootTag()) {
        ScxmlTag *parentTag = child->parentTag();
        if (parentTag)
            return createIndex(parentTag->index(), 0, parentTag);
    }

    return QModelIndex();
}

int StructureModel::rowCount(const QModelIndex &index) const
{
    if (!index.isValid())
        return m_document ? 1 : 0;

    const ScxmlTag *tag = getItem(index);

    if (tag)
        return tag->childCount();

    return 0;
}

Qt::DropActions StructureModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList StructureModel::mimeTypes() const
{
    return QAbstractItemModel::mimeTypes();
}

QMimeData *StructureModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.count() == 1)
        m_dragTag = getItem(indexes.first());

    return QAbstractItemModel::mimeData(indexes);
}

bool StructureModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &p) const
{
    Q_UNUSED(data)
    Q_UNUSED(action)
    Q_UNUSED(row)
    Q_UNUSED(column)

    const ScxmlTag *tag = getItem(p);
    return tag && m_dragTag && (tag->tagType() == State || tag->tagType() == Parallel || tag->tagType() == Scxml);
}

bool StructureModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &p)
{
    Q_UNUSED(data)
    Q_UNUSED(action)
    Q_UNUSED(row)
    Q_UNUSED(column)

    ScxmlTag *tag = getItem(p);
    if (tag && m_dragTag && tag != m_dragTag && (tag->tagType() == State || tag->tagType() == Parallel || tag->tagType() == Scxml)) {
        m_document->undoStack()->beginMacro(tr("Change parent"));
        m_document->changeParent(m_dragTag, tag);
        m_document->undoStack()->endMacro();
        m_dragTag = nullptr;
        return true;
    }

    m_dragTag = nullptr;
    return false;
}

Qt::ItemFlags StructureModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    const ScxmlTag *tag = getItem(index);
    if (index.isValid() && tag) {
        switch (tag->tagType()) {
        case State:
        case Parallel:
        case Initial:
        case Final:
        case History:
            defaultFlags |= Qt::ItemIsDragEnabled;
        case Scxml:
            defaultFlags |= Qt::ItemIsDropEnabled;
            break;
        default:
            break;
        }
    }

    if (tag) {
        if (tag->tagType() == UnknownTag || tag->tagType() == MetadataItem)
            defaultFlags |= Qt::ItemIsEditable;
    }

    return defaultFlags;
}

void StructureModel::updateData()
{
    emit dataChanged(QModelIndex(), QModelIndex());
}

void StructureModel::beginTagChange(ScxmlDocument::TagChange change, ScxmlTag *tag, const QVariant &value)
{
    switch (change) {
    case ScxmlDocument::TagAddChild:
    case ScxmlDocument::TagChangeParentAddChild:
        beginInsertRows(createIndex(tag->index(), 0, tag), value.toInt(), value.toInt());
        break;
    case ScxmlDocument::TagRemoveChild:
    case ScxmlDocument::TagChangeParentRemoveChild:
        beginRemoveRows(createIndex(tag->index(), 0, tag), value.toInt(), value.toInt());
        break;
    case ScxmlDocument::TagChangeOrder: {
        int r1 = tag->index();
        int r2 = value.toInt();
        if (r2 > r1)
            r2++;

        QModelIndex ind = createIndex(r1, 0, tag);
        beginMoveRows(ind, r1, r1, ind, r2);
        break;
    }
    default:
        break;
    }
}

void StructureModel::endTagChange(ScxmlDocument::TagChange change, ScxmlTag *tag, const QVariant &value)
{
    switch (change) {
    case ScxmlDocument::TagAttributesChanged: {
        emit dataChanged(QModelIndex(), QModelIndex());
        break;
    }
    case ScxmlDocument::TagAddChild:
    case ScxmlDocument::TagChangeParentAddChild: {
        endInsertRows();
        emit childAdded(createIndex(0, 0, tag->child(value.toInt())));
        break;
    }
    case ScxmlDocument::TagChangeParentRemoveChild: {
        endRemoveRows();
        break;
    }
    case ScxmlDocument::TagRemoveChild: {
        endRemoveRows();
        break;
    }
    case ScxmlDocument::TagChangeOrder: {
        endMoveRows();
        break;
    }
    case ScxmlDocument::TagCurrentChanged: {
        if (tag)
            emit selectIndex(createIndex(tag->index(), 0, tag));
        break;
    }
    default:
        break;
    }
}
