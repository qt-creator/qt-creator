/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "id.h"
#include "coreconstants.h"
#include "icontext.h"

#include <utils/qtcassert.h>

#include <QtCore/QHash>

namespace Core {

/*!
    \class Core::Id

    \brief The class Id encapsulates an identifier. It is used as a type-safe
    helper class instead of a \c QString or \c QByteArray. The internal
    representation of the id is assumed to be plain 7-bit-clean ASCII.

*/

static int lastUid = 0;
static QVector<QByteArray> stringFromId;
static QHash<QByteArray, int> idFromString;

static int theId(const QByteArray &ba)
{
    QTC_ASSERT(!ba.isEmpty(), /**/);
    int res = idFromString.value(ba);
    if (res == 0) {
        if (lastUid == 0)
            stringFromId.append(QByteArray());
        res = ++lastUid;
        idFromString[ba] = res;
        stringFromId.append(ba);
    }
    return res;
}

Id::Id(const char *name)
    : m_id(theId(name))
{}

Id::Id(const QString &name)
   : m_id(theId(name.toLatin1()))
{}

QByteArray Id::name() const
{
    return stringFromId.at(m_id);
}

QString Id::toString() const
{
    return QString::fromLatin1(stringFromId[m_id]);
}

Context::Context(const char *id, int offset)
{
    d.append(Id(QString(id) + QString::number(offset)).uniqueIdentifier());
}

void Context::add(const char *id)
{
    d.append(Id(id).uniqueIdentifier());
}

bool Context::contains(const char *id) const
{
    return d.contains(Id(id).uniqueIdentifier());
}

} // namespace Core
