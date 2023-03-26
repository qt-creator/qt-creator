// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    friend auto qHash(const Uid &uid) { return qHash(uid.get()); }

    friend bool operator==(const Uid &lhs, const Uid &rhs) { return lhs.get() == rhs.get(); }
    friend bool operator!=(const Uid &lhs, const Uid &rhs) { return !operator==(lhs, rhs); }

    friend QDataStream &operator<<(QDataStream &stream, const Uid &uid)
    {
        return stream << uid.get();
    }

    friend QDataStream &operator>>(QDataStream &stream, Uid &uid)
    {
        QUuid uuid;
        stream >> uuid;
        uid.setUuid(uuid);
        return stream;
    }

private:
    QUuid m_uuid;
};

} // namespace qmt

Q_DECLARE_METATYPE(qmt::Uid)
