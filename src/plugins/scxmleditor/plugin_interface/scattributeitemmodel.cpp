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

#include "scattributeitemmodel.h"
#include "mytypes.h"

#include <QBrush>

using namespace ScxmlEditor::PluginInterface;

SCAttributeItemModel::SCAttributeItemModel(QObject *parent)
    : AttributeItemModel(parent)
{
}

QVariant SCAttributeItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return section == 0 ? tr("Name") : tr("Value");

    return QVariant();
}

bool SCAttributeItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || !m_tag)
        return false;

    bool bEditable = m_tag->tagType() <= MetadataItem;

    if (index.row() >= 0 && m_document != 0) {
        if (!bEditable) {
            if (index.row() < m_tag->info()->n_attributes)
                m_document->setValue(m_tag, index.row(), value.toString());
        } else {
            if (index.column() == 0) {
                m_tag->setAttributeName(index.row(), value.toString());
                m_document->setValue(m_tag, value.toString(), m_tag->attribute(value.toString()));
            } else
                m_document->setValue(m_tag, m_tag->attributeName(index.row()), value.toString());
        }
        emit dataChanged(index, index);
        emit layoutChanged();
        return true;
    }

    return false;
}

QVariant SCAttributeItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_tag)
        return QVariant();

    if (index.row() < 0)
        return QVariant();

    bool bEditable = m_tag->tagType() <= MetadataItem;

    if (!bEditable && index.row() >= m_tag->info()->n_attributes)
        return QVariant();

    if (bEditable && index.row() > m_tag->attributeCount())
        return QVariant();

    bool bExtraRow = bEditable && m_tag->attributeCount() == index.row();

    switch (role) {
    case Qt::DisplayRole:
        if (bExtraRow)
            return index.column() == 0 ? tr("- name -") : tr(" - value -");
    case Qt::EditRole: {
        if (index.column() == 0) {
            if (bEditable) {
                return m_tag->attributeName(index.row());
            } else {
                if (m_tag->info()->attributes[index.row()].required)
                    return QString::fromLatin1("*%1").arg(QLatin1String(m_tag->info()->attributes[index.row()].name));
                else
                    return m_tag->info()->attributes[index.row()].name;
            }
        } else {
            if (bEditable) {
                if (m_tag->tagType() > MetadataItem && m_tag->info()->attributes[index.row()].datatype == QVariant::StringList)
                    return QString::fromLatin1(m_tag->info()->attributes[index.row()].value).split(";");
                else
                    return m_tag->attribute(index.row());
            } else {
                return m_tag->attribute(QLatin1String(m_tag->info()->attributes[index.row()].name));
            }
        }
    }
    case Qt::TextAlignmentRole:
        if (bExtraRow)
            return Qt::AlignHCenter;
        else
            break;
    case Qt::ForegroundRole:
        return bExtraRow ? QBrush(Qt::gray) : QBrush(Qt::black);
    case DataTypeRole: {
        if (m_tag->tagType() == Metadata || m_tag->tagType() == MetadataItem)
            return (int)QVariant::String;
        else if (index.column() == 1 && m_tag->info()->n_attributes > 0)
            return m_tag->info()->attributes[index.row()].datatype;
        else
            return QVariant::Invalid;
    }
    case DataRole: {
        if (m_tag->info()->n_attributes > 0)
            return m_tag->info()->attributes[index.row()].value;
        else
            return QVariant();
    }
    default:
        break;
    }

    return QVariant();
}

Qt::ItemFlags SCAttributeItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || !m_tag)
        return Qt::NoItemFlags;

    if (m_tag->tagType() <= MetadataItem || (index.column() == 1 && m_tag->info()->n_attributes > 0 && m_tag->info()->attributes[index.row()].editable))
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;

    return Qt::NoItemFlags;
}

int SCAttributeItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}

int SCAttributeItemModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (m_tag) {
        if (m_tag->tagType() <= MetadataItem)
            return m_tag->attributeCount() + 1;
        else
            return m_tag->info()->n_attributes;
    }

    return 0;
}
