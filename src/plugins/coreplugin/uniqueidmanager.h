/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef UNIQUEIDMANAGER_H
#define UNIQUEIDMANAGER_H

#include "core_global.h"

#include <QtCore/QString>
#include <QtCore/QHash>

namespace Core {

// FIXME: The intention is to use this class instead of the
// generic QString to identify actions.

class CORE_EXPORT Id
{
public:
    Id() {}
    Id(const char *name) : m_name(QLatin1String(name)) {}
    // FIXME: Replace with QByteArray
    Id(const QString &name) : m_name(name) {}
    // FIXME: Remove.
    operator QString() const { return m_name; }
    // FIXME: Replace with QByteArray
    QString name() const { return m_name; }
    bool isValid() const { return !m_name.isEmpty(); }
    bool operator==(const Id &id) const { return m_name == id.m_name; }
    bool operator!=(const Id &id) const { return m_name != id.m_name; }

private:
    // Intentionally unimplemented
    Id(const QLatin1String &);
    // FIXME: Replace with QByteArray
    QString m_name;
};

inline uint qHash(const Id &id)
{
    return qHash(id.name());
}

class CORE_EXPORT UniqueIDManager
{
public:
    UniqueIDManager();
    ~UniqueIDManager();

    static UniqueIDManager *instance() { return m_instance; }

    bool hasUniqueIdentifier(const Id &id) const;
    int uniqueIdentifier(const Id &id);
    QString stringForUniqueIdentifier(int uid);

private:
    QHash<Id, int> m_uniqueIdentifiers;
    static UniqueIDManager *m_instance;
};

} // namespace Core

#endif // UNIQUEIDMANAGER_H
