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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "uniqueidmanager.h"
#include "coreconstants.h"

using namespace Core;

UniqueIDManager *UniqueIDManager::m_instance = 0;

UniqueIDManager::UniqueIDManager()
{
    m_instance = this;
}

UniqueIDManager::~UniqueIDManager()
{
    m_instance = 0;
}

bool UniqueIDManager::hasUniqueIdentifier(const Id &id) const
{
    return m_uniqueIdentifiers.contains(id);
}

int UniqueIDManager::uniqueIdentifier(const Id &id)
{
    if (hasUniqueIdentifier(id))
        return m_uniqueIdentifiers.value(id);

    int uid = m_uniqueIdentifiers.count() + 1;
    m_uniqueIdentifiers.insert(id, uid);
    return uid;
}

QString UniqueIDManager::stringForUniqueIdentifier(int uid)
{
    return m_uniqueIdentifiers.key(uid);
}

// FIXME: Move to some better place.
#include "icontext.h"

static int toId(const char *id)
{
    return UniqueIDManager::instance()->uniqueIdentifier(id);
}

Context::Context(const char *id, int offset)
{
    d.append(UniqueIDManager::instance()
        -> uniqueIdentifier(Id(QString(id) + QString::number(offset))));
}

void Context::add(const char *id)
{
    d.append(toId(id));
}

bool Context::contains(const char *id) const
{
    return d.contains(toId(id));
}
