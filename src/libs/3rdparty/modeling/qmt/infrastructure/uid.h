/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#ifndef QMT_UID_H
#define QMT_UID_H

#include "qmt_global.h"

#include <QDataStream>
#include <QUuid>
#include <QMetaType>


namespace qmt {

class QMT_EXPORT Uid
{
public:

    static const Uid getInvalidUid() { return Uid(QUuid()); }

public:
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

}

Q_DECLARE_METATYPE(qmt::Uid)

#endif // QMT_UID_H
