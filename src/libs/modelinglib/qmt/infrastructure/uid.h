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

#include "qmt_global.h"

#include <QDataStream>
#include <QUuid>
#include <QMetaType>

namespace qmt {

class QMT_EXPORT Uid
{
public:
    static const Uid invalidUid() { return Uid(QUuid()); }

    Uid() : m_uuid(QUuid::createUuid()) { }
    explicit Uid(const QUuid &uuid) : m_uuid(uuid) { }

    bool isValid() const { return !m_uuid.isNull(); }
    QUuid get() const { return m_uuid; }
    void setUuid(const QUuid &uuid) { m_uuid = uuid; }

    void clear() { m_uuid = QUuid(); }
    void renew() { m_uuid = QUuid::createUuid(); }

    QString toString() const { return m_uuid.toString(); }
    void fromString(const QString &s) { m_uuid = QUuid(s); }

private:
    QUuid m_uuid;
};

inline uint qHash(const Uid &uid)
{
    return qHash(uid.get());
}

inline bool operator==(const Uid &lhs, const Uid &rhs)
{
    return lhs.get() == rhs.get();
}

inline bool operator!=(const Uid &lhs, const Uid &rhs)
{
    return !operator==(lhs, rhs);
}

inline QDataStream &operator<<(QDataStream &stream, const Uid &uid)
{
    return stream << uid.get();
}

inline QDataStream &operator>>(QDataStream &stream, Uid &uid)
{
    QUuid uuid;
    stream >> uuid;
    uid.setUuid(uuid);
    return stream;
}

} // namespace qmt

Q_DECLARE_METATYPE(qmt::Uid)
