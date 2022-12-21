// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "attributeitemmodel.h"

using namespace ScxmlEditor::PluginInterface;

AttributeItemModel::AttributeItemModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void AttributeItemModel::setTag(ScxmlTag *tag)
{
    beginResetModel();
    m_tag = tag;
    m_document = m_tag ? m_tag->document() : nullptr;
    endResetModel();
    emit layoutChanged();
    emit dataChanged(QModelIndex(), QModelIndex());
}

ScxmlTag *AttributeItemModel::tag() const
{
    return m_tag;
}
