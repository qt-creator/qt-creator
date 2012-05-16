/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <utils/qtcassert.h>

#include <QByteArray>
#include <QHash>
#include <QVector>

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
    explicit StringHolder(const char *s)
        : str(s)
    {
        n = strlen(s);
        int m = n;
        h = 0;
        while (m--) {
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
            free(const_cast<char *>(it.key().str));
    }
#endif
};


static int lastUid = 0;
static QVector<QByteArray> stringFromId;
static IdCache idFromString;

static int theId(const char *str)
{
    QTC_ASSERT(str && *str, return 0);
    StringHolder sh(str);
    int res = idFromString.value(sh, 0);
    if (res == 0) {
        if (lastUid == 0)
            stringFromId.append(QByteArray());
        res = ++lastUid;
        sh.str = qstrdup(sh.str);
        idFromString[sh] = res;
        stringFromId.append(QByteArray::fromRawData(sh.str, sh.n));
    }
    return res;
}

Id::Id(const char *name)
    : m_id(theId(name))
{}

Id::Id(const QString &name)
   : m_id(theId(name.toUtf8()))
{}

QByteArray Id::name() const
{
    return stringFromId.at(m_id);
}

QString Id::toString() const
{
    return QString::fromUtf8(stringFromId[m_id]);
}

} // namespace Core
