/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CORE_ID_H
#define CORE_ID_H

#include "core_global.h"

#include <QtCore/QMetaType>
#include <QtCore/QString>

namespace Core {

// FIXME: Make this a wrapper around an int, not around a QByteArray.
class CORE_EXPORT Id
{
public:
    Id() {}
    Id(const char *name) : m_name(name) {}
    // FIXME: Replace with QByteArray
    Id(const QString &name) : m_name(name.toLatin1()) {}
    QByteArray name() const { return m_name; }
    QString toString() const { return QString::fromLatin1(m_name); }
    bool isValid() const { return !m_name.isEmpty(); }
    bool operator==(const Id &id) const { return m_name == id.m_name; }
    bool operator!=(const Id &id) const { return m_name != id.m_name; }
    int uniqueIdentifier() const;
    static Id fromUniqueIdentifier(int uid);

private:
    // Intentionally unimplemented
    Id(const QLatin1String &);
    QByteArray m_name;
};

CORE_EXPORT uint qHash(const Id &id);

} // namespace Core

Q_DECLARE_METATYPE(Core::Id);

#endif // CORE_ID_H
