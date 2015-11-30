/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "id.h"

#include <utils/qtcassert.h>

#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QHash>
#include <QVariant>

#include <string.h>

namespace Core {

/*!
    \class Core::Id

    \brief The Id class encapsulates an identifier that is unique
    within a specific running \QC process.

    \c{Core::Id} is used as facility to identify objects of interest
    in a more typesafe and faster manner than a plain \c QString or
    \c QByteArray would provide.

    An id is internally represented as a 32 bit integer (its \c UID)
    and associated with a plain 7-bit-clean ASCII name used
    for display and persistency.

    Each plugin that is distributed as part of \QC has a
    private range of 10000 UIDs that are guaranteed to be unique.

    Third party plugins are advised to construct ids from their
    string representation.

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
            length = n = static_cast<int>(strlen(s));
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
    return sh1.h == sh2.h && sh1.str && sh2.str && strcmp(sh1.str, sh2.str) == 0;
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


static int firstUnusedId = Id::IdsPerPlugin * Id::ReservedPlugins;

static QHash<int, StringHolder> stringFromId;
static IdCache idFromString;

static int theId(const char *str, int n = 0)
{
    QTC_ASSERT(str && *str, return 0);
    StringHolder sh(str, n);
    int res = idFromString.value(sh, 0);
    if (res == 0) {
        res = firstUnusedId++;
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

/*!
    \fn Core::Id::Id(int uid)

    Constructs an id given \a UID.

    The UID is an integer value that is unique within the running
    \QC process.

    It is the callers responsibility to ensure the uniqueness of
    the passed integer. The recommended approach is to use
    \c{registerId()} with an value taken from the plugin's
    private range.

    \sa registerId()

*/

/*!
    Constructs an id given its associated \a name. The internal
    representation will be unspecified, but consistent within a
    \QC process.

*/
Id::Id(const char *name)
    : m_id(theId(name, 0))
{}

/*!
  Returns an internal representation of the id.
*/

QByteArray Id::name() const
{
    return stringFromId.value(m_id).str;
}

/*!
  Returns a string representation of the id suitable
  for UI display.

  This should not be used to create a persistent version
  of the Id, use \c{toSetting()} instead.

  \sa fromString(), toSetting()
*/

QString Id::toString() const
{
    return QString::fromUtf8(stringFromId.value(m_id).str);
}

/*!
  Creates an id from a string representation.

  This should not be used to handle a persistent version
  of the Id, use \c{fromSetting()} instead.

  \deprecated

  \sa toString(), fromSetting()
*/

Id Id::fromString(const QString &name)
{
    return Id(theId(name.toUtf8()));
}

/*!
  Creates an id from a string representation.

  This should not be used to handle a persistent version
  of the Id, use \c{fromSetting()} instead.

  \deprecated

  \sa toString(), fromSetting()
*/

Id Id::fromName(const QByteArray &name)
{
    return Id(theId(name));
}

/*!
  Returns a persistent value representing the id which is
  suitable to be stored in QSettings.

  \sa fromSetting()
*/

QVariant Id::toSetting() const
{
    return QVariant(QString::fromUtf8(stringFromId.value(m_id).str));
}

/*!
  Reconstructs an id from a persistent value.

  \sa toSetting()
*/

Id Id::fromSetting(const QVariant &variant)
{
    const QByteArray ba = variant.toString().toUtf8();
    if (ba.isEmpty())
        return Id();
    return Id(theId(ba));
}

/*!
  Constructs a derived id.

  This can be used to construct groups of ids logically
  belonging together. The associated internal name
  will be generated by appending \a suffix.
*/

Id Id::withSuffix(int suffix) const
{
    const QByteArray ba = name() + QByteArray::number(suffix);
    return Id(ba.constData());
}

/*!
  \overload
*/

Id Id::withSuffix(const char *suffix) const
{
    const QByteArray ba = name() + suffix;
    return Id(ba.constData());
}

/*!
  \overload

  \sa stringSuffix()
*/

Id Id::withSuffix(const QString &suffix) const
{
    const QByteArray ba = name() + suffix.toUtf8();
    return Id(ba.constData());
}

/*!
  Constructs a derived id.

  This can be used to construct groups of ids logically
  belonging together. The associated internal name
  will be generated by prepending \a prefix.
*/

Id Id::withPrefix(const char *prefix) const
{
    const QByteArray ba = prefix + name();
    return Id(ba.constData());
}


bool Id::operator==(const char *name) const
{
    const char *string = stringFromId.value(m_id).str;
    if (string && name)
        return strcmp(string, name) == 0;
    else
        return false;
}

// For debugging purposes
CORE_EXPORT const char *nameForId(int id)
{
    return stringFromId.value(id).str;
}

bool Id::alphabeticallyBefore(Id other) const
{
    return toString().compare(other.toString(), Qt::CaseInsensitive) < 0;
}


/*!
  Extracts a part of the id string
  representation. This function can be used to split off the base
  part specified by \a baseId used when generating an id with \c{withSuffix()}.

  \sa withSuffix()
*/

QString Id::suffixAfter(Id baseId) const
{
    const QByteArray b = baseId.name();
    const QByteArray n = name();
    return n.startsWith(b) ? QString::fromUtf8(n.mid(b.size())) : QString();
}

} // namespace Core

QT_BEGIN_NAMESPACE

QDataStream &operator<<(QDataStream &ds, Core::Id id)
{
    return ds << id.name();
}

QDataStream &operator>>(QDataStream &ds, Core::Id &id)
{
    QByteArray ba;
    ds >> ba;
    id = Core::Id::fromName(ba);
    return ds;
}

QDebug operator<<(QDebug dbg, const Core::Id &id)
{
    return dbg << id.name();
}

QT_END_NAMESPACE
