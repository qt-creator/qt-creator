/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "id.h"
#include "coreconstants.h"

#include <utils/qtcassert.h>

#include <QByteArray>
#include <QHash>
#include <QVector>

#include <string.h>

namespace Core {

/*!
    \class Core::Id

    \brief The class Id encapsulates an identifier. It is used as a type-safe
    helper class instead of a \c QString or \c QByteArray. The internal
    representation of the id is assumed to be plain 7-bit-clean ASCII.

*/

class StringHolder
{
public:
    StringHolder()
        : n(0), str(0)
    {}

    StringHolder(const char *s, int length)
        : n(length), str(s)
    {
        if (!n)
            length = n = strlen(s);
        h = 0;
        while (length--) {
            h = (h << 4) + *s++;
            h ^= (h & 0xf0000000) >> 23;
            h &= 0x0fffffff;
        }
    }
    int n;
    const char *str;
    uint h;
};

static bool operator==(const StringHolder &sh1, const StringHolder &sh2)
{
    // sh.n is unlikely to discriminate better than the hash.
    return sh1.h == sh2.h && strcmp(sh1.str, sh1.str) == 0;
}


static uint qHash(const StringHolder &sh)
{
    return sh.h;
}

struct IdCache : public QHash<StringHolder, int>
{
#ifndef QTC_ALLOW_STATIC_LEAKS
    ~IdCache()
    {
        for (IdCache::iterator it = begin(); it != end(); ++it)
            delete[](const_cast<char *>(it.key().str));
    }
#endif
};


static int lastUid = 1000 * 1000;
static QHash<int, StringHolder> stringFromId;
static IdCache idFromString;

static int theId(const char *str, int n = 0)
{
    QTC_ASSERT(str && *str, return 0);
    StringHolder sh(str, n);
    int res = idFromString.value(sh, 0);
    if (res == 0) {
        res = ++lastUid;
        sh.str = qstrdup(sh.str);
        idFromString[sh] = res;
        stringFromId[res] = sh;
    }
    return res;
}

static int theId(const QByteArray &ba)
{
    return theId(ba.constData(), ba.size());
}

Id::Id(const char *name)
    : m_id(theId(name, 0))
{}

Id::Id(const QByteArray &name)
   : m_id(theId(name))
{}

Id::Id(const QString &name)
   : m_id(theId(name.toUtf8()))
{}

QByteArray Id::name() const
{
    return stringFromId.value(m_id).str;
}

QString Id::toString() const
{
    return QString::fromUtf8(stringFromId.value(m_id).str);
}

void Id::registerId(int uid, const char *name)
{
    StringHolder sh(name, 0);
    idFromString[sh] = uid;
    stringFromId[uid] = sh;
}

bool Id::operator==(const char *name) const
{
    return strcmp(stringFromId.value(m_id).str, name) == 0;
}

// For debugging purposes
CORE_EXPORT const char *nameForId(int id)
{
    return stringFromId.value(id).str;
}

} // namespace Core
