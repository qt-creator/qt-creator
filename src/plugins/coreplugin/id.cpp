/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

    \brief The class Id encapsulates an identifier that is unique
    within a specific running Qt Creator process.

    \c{Core::Id} is used as facility to identify objects of interest
    in a more typesafe and faster manner than a plain \c QString or
    \c QByteArray would provide.

    An id is internally represented as a 32 bit integer (its \c UID)
    and associated with a be plain 7-bit-clean ASCII name used
    for display and persistency.

    Each plugin that is distributed as part of Qt Creator has a
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
    \fn Core::Id(int uid)

    \brief Constructs an id given a UID.

    The UID is an integer value that is unique within the running
    Qt Creator process.

    It is the callers responsibility to ensure the uniqueness of
    the passed integer. The recommended approach is to use
    \c{registerId()} with an value taken from the plugin's
    private range.

    \sa registerId()

*/

/*!
    Constructs an id given its associated name. The internal
    representation will be unspecified, but consistent within a
    Qt Creator process.

*/
Id::Id(const char *name)
    : m_id(theId(name, 0))
{}

/*!
    \overload

*/
Id::Id(const QByteArray &name)
   : m_id(theId(name))
{}

/*!
    \overload
    \deprecated
*/
Id::Id(const QString &name)
   : m_id(theId(name.toUtf8()))
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
  will be generated by appending \c{suffix}.
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
  Constructs a derived id.

  This can be used to construct groups of ids logically
  belonging together. The associated internal name
  will be generated by prepending \c{prefix}.
*/

Id Id::withPrefix(const char *prefix) const
{
    const QByteArray ba = prefix + name();
    return Id(ba.constData());
}


/*!
  Associates a id with its uid and its string
  representation.

  The uid should be taken from the plugin's private range.

  \sa fromSetting()
*/

void Id::registerId(int uid, const char *name)
{
    StringHolder sh(name, 0);
    idFromString[sh] = uid;
    stringFromId[uid] = sh;
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

} // namespace Core
