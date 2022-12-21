// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "attributeitemdelegate.h"

using namespace ScxmlEditor::PluginInterface;

AttributeItemDelegate::AttributeItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void AttributeItemDelegate::setTag(ScxmlTag *tag)
{
    m_tag = tag;
}

ScxmlTag *AttributeItemDelegate::tag() const
{
    return m_tag;
}
