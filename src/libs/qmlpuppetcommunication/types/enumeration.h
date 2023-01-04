// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <QDataStream>
#include <QMetaType>
#include <QVariant>

#include <QString>
#include <QList>
#include <QtDebug>

namespace QmlDesigner {

using EnumerationName = QByteArray;

class Enumeration
{
    friend bool operator ==(const Enumeration &first, const Enumeration &second);
    friend bool operator <(const Enumeration &first, const Enumeration &second);
    friend QDataStream &operator>>(QDataStream &in, Enumeration &enumeration);

public:
    Enumeration() = default;
    Enumeration(const EnumerationName &enumerationName)
        : m_enumerationName(enumerationName)
    {
    }
    Enumeration(const QString &enumerationName)
        : m_enumerationName(enumerationName.toUtf8())
    {
    }
    Enumeration(const QString &scope, const QString &name)
    {
        QString enumerationString = scope + QLatin1Char('.') + name;
        m_enumerationName = enumerationString.toUtf8();
    }

    EnumerationName scope() const
    {
        return m_enumerationName.split('.').constFirst();
    }
    EnumerationName name() const
    {
        return m_enumerationName.split('.').last();
    }
    EnumerationName toEnumerationName() const
    {
        return m_enumerationName;
    }
    QString toString() const
    {
        return QString::fromUtf8(m_enumerationName);
    }
    QString nameToString() const
    {
        return QString::fromUtf8(name());
    }

private:
    EnumerationName m_enumerationName;
};

inline QDataStream &operator<<(QDataStream &out, const Enumeration &enumeration){
    out << enumeration.toEnumerationName();
    return out;
}

inline QDataStream &operator>>(QDataStream &in, Enumeration &enumeration)
{
    in >> enumeration.m_enumerationName;
    return in;
}


inline bool operator==(const Enumeration &first, const Enumeration &second)
{
    return first.m_enumerationName == second.m_enumerationName;
}

inline bool operator<(const Enumeration &first, const Enumeration &second)
{
    return first.m_enumerationName < second.m_enumerationName;
}

inline QDebug operator <<(QDebug debug, const Enumeration &enumeration)
{
    debug.nospace() << "Enumeration("
                    << enumeration.toString()
                    << ")";
    return debug;
}

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::Enumeration)
