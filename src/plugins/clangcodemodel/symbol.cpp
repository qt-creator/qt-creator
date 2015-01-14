/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "symbol.h"

#include <cplusplus/Icons.h>

using namespace ClangCodeModel;

Symbol::Symbol()
    : m_kind(Unknown)
{}

Symbol::Symbol(const QString &name,
               const QString &qualification,
               Kind type,
               const SourceLocation &location)
    : m_name(name)
    , m_qualification(qualification)
    , m_location(location)
    , m_kind(type)
{}

QIcon Symbol::iconForSymbol() const
{
    CPlusPlus::Icons icons;
    switch (m_kind) {
    case Enum:
        return icons.iconForType(CPlusPlus::Icons::EnumIconType);
    case Class:
        return icons.iconForType(CPlusPlus::Icons::ClassIconType);
    case Method:
    case Function:
    case Declaration:
    case Constructor:
    case Destructor:
        return icons.iconForType(CPlusPlus::Icons::FuncPublicIconType);
    default:
        return icons.iconForType(CPlusPlus::Icons::UnknownIconType);
    }
}

namespace ClangCodeModel {

QDataStream &operator<<(QDataStream &stream, const Symbol &symbol)
{
    stream << symbol.m_name
           << symbol.m_qualification
           << symbol.m_location.fileName()
           << (quint32)symbol.m_location.line()
           << (quint16)symbol.m_location.column()
           << (quint32)symbol.m_location.offset()
           << (qint8)symbol.m_kind;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, Symbol &symbol)
{
    QString fileName;
    quint32 line;
    quint16 column;
    quint32 offset;
    quint8 kind;
    stream >> symbol.m_name
           >> symbol.m_qualification
           >> fileName
           >> line
           >> column
           >> offset
           >> kind;
    symbol.m_location = SourceLocation(fileName, line, column, offset);
    symbol.m_kind = Symbol::Kind(kind);

    return stream;
}

bool operator==(const Symbol &a, const Symbol &b)
{
    return a.m_name == b.m_name
            && a.m_qualification == b.m_qualification
            && a.m_location == b.m_location
            && a.m_kind == b.m_kind;
}

bool operator!=(const Symbol &a, const Symbol &b)
{
    return !(a == b);
}

} // ClangCodeModel
