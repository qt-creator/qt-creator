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

#pragma once

#include "core_global.h"

#include <QMetaType>
#include <QString>

QT_BEGIN_NAMESPACE
class QDataStream;
class QVariant;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT Id
{
public:
    Id() : m_id(0) {}
    Id(const char *name); // Good to use.

    Id withSuffix(int suffix) const;
    Id withSuffix(const char *suffix) const;
    Id withSuffix(const QString &suffix) const;
    Id withPrefix(const char *prefix) const;

    QByteArray name() const;
    QString toString() const; // Avoid.
    QVariant toSetting() const; // Good to use.
    QString suffixAfter(Id baseId) const;
    bool isValid() const { return m_id; }
    bool operator==(Id id) const { return m_id == id.m_id; }
    bool operator==(const char *name) const;
    bool operator!=(Id id) const { return m_id != id.m_id; }
    bool operator!=(const char *name) const { return !operator==(name); }
    bool operator<(Id id) const { return m_id < id.m_id; }
    bool operator>(Id id) const { return m_id > id.m_id; }
    bool alphabeticallyBefore(Id other) const;

    quintptr uniqueIdentifier() const { return m_id; } // Avoid.
    static Id fromString(const QString &str); // FIXME: avoid.
    static Id fromName(const QByteArray &ba); // FIXME: avoid.
    static Id fromSetting(const QVariant &variant); // Good to use.

    static Id versionedId(const QByteArray &prefix, int major, int minor = -1);

    static QSet<Id> fromStringList(const QStringList &list);
    static QStringList toStringList(const QSet<Id> &ids);

private:
    // Intentionally unimplemented
    Id(const QLatin1String &) = delete;
    explicit Id(quintptr uid) : m_id(uid) {}

    quintptr m_id;
};

inline uint qHash(Id id) { return static_cast<uint>(id.uniqueIdentifier()); }

} // namespace Core

Q_DECLARE_METATYPE(Core::Id)
Q_DECLARE_METATYPE(QList<Core::Id>)

QT_BEGIN_NAMESPACE
QDataStream &operator<<(QDataStream &ds, Core::Id id);
QDataStream &operator>>(QDataStream &ds, Core::Id &id);
CORE_EXPORT QDebug operator<<(QDebug dbg, const Core::Id &id);
QT_END_NAMESPACE
