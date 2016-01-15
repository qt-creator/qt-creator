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

#include "columndefinition.h"

namespace Internal {

void ColumnDefinition::setName(const Utf8String &name)
{
    name_ = name;
}

const Utf8String &ColumnDefinition::name() const
{
    return name_;
}

void ColumnDefinition::setType(ColumnType type)
{
    type_ = type;
}

ColumnType ColumnDefinition::type() const
{
    return type_;
}

Utf8String ColumnDefinition::typeString() const
{
    switch (type_) {
        case ColumnType::None: return Utf8String();
        case ColumnType::Numeric: return Utf8StringLiteral("NUMERIC");
        case ColumnType::Integer: return Utf8StringLiteral("INTEGER");
        case ColumnType::Real: return Utf8StringLiteral("REAL");
        case ColumnType::Text: return Utf8StringLiteral("TEXT");
    }

    Q_UNREACHABLE();
}

void ColumnDefinition::setIsPrimaryKey(bool isPrimaryKey)
{
    isPrimaryKey_ = isPrimaryKey;
}

bool ColumnDefinition::isPrimaryKey() const
{
    return isPrimaryKey_;
}

}
