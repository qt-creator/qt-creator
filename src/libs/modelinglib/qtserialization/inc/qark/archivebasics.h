// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "flag.h"

#include <QVariant>
#include <QString>
#include <QHash>

namespace qark {

class ArchiveBasics
{
public:
    void setFlag(const Flag &flag) { m_flags |= flag.mask(); }
    void clearFlag(const Flag &flag) { m_flags &= ~flag.mask(); }
    bool hasFlag(const Flag &flag) const { return (m_flags & flag.mask()) != 0; }
    bool takeFlag(const Flag &flag)
    {
        bool f = (m_flags & flag.mask()) != 0;
        m_flags &= ~flag.mask();
        return f;
    }

    bool hasUserData(const QString &key)
    {
        return m_userData.contains(key);
    }

    template<typename T>
    T userData(const QString &key)
    {
        return m_userData.value(key).value<T>();
    }

    template<typename T>
    T userData(const QString &key, const T &defaultValue)
    {
        // gcc 4.8.2 fails to compile if the following 2 statements are written in one expression
        //return m_userData.value(key, data).value<T>();
        QVariant v = m_userData.value(key, defaultValue);
        return v.value<T>();
    }

    template<class T>
    void setUserData(const QString &key, const T &data)
    {
        m_userData.insert(key, data);
    }

    void removeUserData(const QString &key)
    {
        m_userData.remove(key);
    }

private:
    Flag::MaskType m_flags = 0;
    QHash<QString, QVariant> m_userData;
};

} // namespace qark
