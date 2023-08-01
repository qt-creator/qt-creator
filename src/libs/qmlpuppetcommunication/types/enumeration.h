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
using EnumerationNameView = QByteArrayView;

class Enumeration
{
public:
    Enumeration() = default;
    Enumeration(EnumerationName enumerationName)
        : m_enumerationName{std::move(enumerationName)}
    {}

    Enumeration(const char *text)
        : m_enumerationName{text, static_cast<qsizetype>(std::strlen(text))}
    {}

    Enumeration(const QString &enumerationName)
        : m_enumerationName(enumerationName.toUtf8())
    {}

    Enumeration(const EnumerationName &scope, const EnumerationName &name)
    {
        m_enumerationName.reserve(scope.size() + 1 + name.size());
        m_enumerationName.append(scope);
        m_enumerationName.append('.');
        m_enumerationName.append(name);
    }

    EnumerationNameView scope() const
    {
        auto found = std::find(m_enumerationName.begin(), m_enumerationName.end(), '.');
        return {m_enumerationName.begin(), found};
    }

    EnumerationNameView toScope() const { return scope().toByteArray(); }

    EnumerationNameView name() const
    {
        auto found = std::find(m_enumerationName.begin(), m_enumerationName.end(), '.');
        if (found != m_enumerationName.end())
            return {std::next(found), m_enumerationName.end()};

        return {m_enumerationName.end(), m_enumerationName.end()};
    }

    EnumerationName toName() const { return name().toByteArray(); }

    EnumerationName toEnumerationName() const { return m_enumerationName; }

    QString toString() const { return QString::fromUtf8(m_enumerationName); }
    QString nameToString() const { return QString::fromUtf8(name()); }

    friend bool operator==(const Enumeration &first, const Enumeration &second)
    {
        return first.m_enumerationName == second.m_enumerationName;
    }

    friend bool operator<(const Enumeration &first, const Enumeration &second)
    {
        return first.m_enumerationName < second.m_enumerationName;
    }

    friend QDataStream &operator<<(QDataStream &out, const Enumeration &enumeration)
    {
        return out << enumeration.m_enumerationName;
    }

    friend QDataStream &operator>>(QDataStream &in, Enumeration &enumeration)
    {
        return in >> enumeration.m_enumerationName;
    }

private:
    EnumerationName m_enumerationName;
};

inline QDebug operator <<(QDebug debug, const Enumeration &enumeration)
{
    debug.nospace() << "Enumeration("
                    << enumeration.toString()
                    << ")";
    return debug;
}

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::Enumeration)
