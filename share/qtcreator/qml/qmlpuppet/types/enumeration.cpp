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

#include "enumeration.h"

#include <QString>
#include <QList>
#include <QtDebug>

namespace QmlDesigner {

Enumeration::Enumeration()
{
}

Enumeration::Enumeration(const EnumerationName &enumerationName)
    : m_enumerationName(enumerationName)
{
}

Enumeration::Enumeration(const QString &enumerationName)
    : m_enumerationName(enumerationName.toUtf8())
{
}

Enumeration::Enumeration(const QString &scope, const QString &name)
{
    QString enumerationString = scope + QLatin1Char('.') + name;

    m_enumerationName = enumerationString.toUtf8();
}

QmlDesigner::EnumerationName QmlDesigner::Enumeration::scope() const
{
    return m_enumerationName.split('.').first();
}

EnumerationName Enumeration::name() const
{
    return m_enumerationName.split('.').last();
}

EnumerationName Enumeration::toEnumerationName() const
{
    return m_enumerationName;
}

QString Enumeration::toString() const
{
    return QString::fromUtf8(m_enumerationName);
}

QString Enumeration::nameToString() const
{
    return QString::fromUtf8(name());
}

QDataStream &operator<<(QDataStream &out, const Enumeration &enumeration)
{
    out << enumeration.toEnumerationName();

    return out;
}

QDataStream &operator>>(QDataStream &in, Enumeration &enumeration)
{
    in >> enumeration.m_enumerationName;

    return in;
}


bool operator ==(const Enumeration &first, const Enumeration &second)
{
    return first.m_enumerationName == second.m_enumerationName;
}

bool operator <(const Enumeration &first, const Enumeration &second)
{
    return first.m_enumerationName < second.m_enumerationName;
}

QDebug operator <<(QDebug debug, const Enumeration &enumeration)
{
    debug.nospace() << "Enumeration("
                    << enumeration.toString()
                    << ")";

    return debug;
}


} // namespace QmlDesigner



