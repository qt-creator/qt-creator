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

#include "id.h"
#include "coreconstants.h"
#include "icontext.h"

#include <QtCore/QHash>

namespace Core {

/*!
    \class Core::Id

    \brief The class Id encapsulates an identifier. It is used as a type-safe
    helper class instead of a \c QString or \c QByteArray. The internal
    representation of the id is assumed to be plain 7-bit-clean ASCII.

*/

uint qHash(const Core::Id &id) { return qHash(id.name()); }

static QHash<Core::Id, int> &theUniqueIdentifiers()
{
    static QHash<Core::Id, int> data;
    return data;
}

int Id::uniqueIdentifier() const
{
    if (theUniqueIdentifiers().contains(*this))
        return theUniqueIdentifiers().value(*this);

    const int uid = theUniqueIdentifiers().count() + 1;
    theUniqueIdentifiers().insert(*this, uid);
    return uid;
}

Id Id::fromUniqueIdentifier(int uid)
{
    return theUniqueIdentifiers().key(uid);
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
