/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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
