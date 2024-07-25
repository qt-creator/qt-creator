// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "id.h"

#include "algorithm.h"
#include "qtcassert.h"

#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QHash>
#include <QReadWriteLock>
#include <QUuid>
#include <QVariant>

namespace Utils {

/*!
    \class Utils::Id
    \inheaderfile utils/id.h
    \inmodule QtCreator

    \brief The Id class encapsulates an identifier that is unique
    within a specific running \QC process.

    \c{Utils::Id} is used as facility to identify objects of interest
    in a more typesafe and faster manner than a plain QString or
    QByteArray would provide.

    An id is associated with a plain 7-bit-clean ASCII name used
    for display and persistency.

*/

class StringHolder
{
public:
    StringHolder() = default;

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

    friend auto qHash(const StringHolder &sh)
    {
        return QT_PREPEND_NAMESPACE(qHash)(sh.h, 0);
    }

    friend bool operator==(const StringHolder &sh1, const StringHolder &sh2)
    {
        // sh.n is unlikely to discriminate better than the hash.
        return sh1.h == sh2.h && sh1.str && sh2.str && strcmp(sh1.str, sh2.str) == 0;
    }

    int n = 0;
    const char *str = nullptr;
    quintptr h;
};

struct IdCache : public QHash<StringHolder, quintptr>
{
#ifndef QTC_ALLOW_STATIC_LEAKS
    ~IdCache()
    {
        for (IdCache::iterator it = begin(); it != end(); ++it)
            delete[](const_cast<char *>(it.key().str));
    }
#endif
};

static QHash<quintptr, StringHolder> stringFromId;
static IdCache idFromString;
static QReadWriteLock s_cacheMutex;

static quintptr theId(const char *str, int n)
{
    QTC_ASSERT(str && *str, return 0);
    StringHolder sh(str, n);
    int res = 0;
    {
        QReadLocker lock(&s_cacheMutex); // Try quick read locker first
        res = idFromString.value(sh, 0);
    }
    if (res == 0) {
        QWriteLocker lock(&s_cacheMutex);
        res = idFromString.value(sh, 0); // Some other thread could have added it to the cache
                                         // in meantime, after read lock was released and before
                                         // write lock was acquired. Re-read it again.
        if (res == 0) {
            static quintptr firstUnusedId = 10 * 1000 * 1000;
            res = firstUnusedId++;
            sh.str = qstrdup(sh.str);
            idFromString[sh] = res;
            stringFromId[res] = sh;
        }
    }
    return res;
}

/*!
    \fn Utils::Id::Id(quintptr uid)
    \internal

    Constructs an id given \a UID.

    The UID is an integer value that is unique within the running
    \QC process.

*/

/*!
    Constructs an id given its associated \a name. The internal
    representation will be unspecified, but consistent within a
    \QC process.

*/
Id::Id(const char *s, size_t len)
    : m_id(theId(s, len))
{}

Id Id::generate()
{
    const QByteArray ba = QUuid::createUuid().toByteArray();
    return Id(theId(ba.data(), ba.size()));
}

/*!
  Returns an internal representation of the id.
*/

QByteArrayView Id::name() const
{
    QReadLocker lock(&s_cacheMutex);
    StringHolder holder = stringFromId.value(m_id);
    return QByteArrayView(holder.str, holder.n);
}

/*! \internal */
QByteArray Id::toByteArray() const
{
    return name().toByteArray();
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
    return QString::fromUtf8(name());
}

/*! \internal */
Key Id::toKey() const
{
    return name();
}

/*!
  Creates an id from a string representation.

  This should not be used to handle a persistent version
  of the Id, use \c{fromSetting()} instead.

  \deprecated

  \sa toString(), fromSetting()
*/

Id Id::fromString(const QStringView name)
{
    if (name.isEmpty())
        return Id();
    const QByteArray ba = name.toUtf8();
    return Id(theId(ba.data(), ba.size()));
}

/*!
  Creates an id from a string representation.

  This should not be used to handle a persistent version
  of the Id, use \c{fromSetting()} instead.

  \deprecated

  \sa toString(), fromSetting()
*/

Id Id::fromName(const QByteArrayView name)
{
    return Id(theId(name.data(), name.size()));
}

/*!
  Returns a persistent value representing the id which is
  suitable to be stored in QSettings.

  \sa fromSetting()
*/

QVariant Id::toSetting() const
{
    QReadLocker lock(&s_cacheMutex);
    return QVariant(QString::fromUtf8(stringFromId.value(m_id).str));
}

/*!
  Reconstructs an id from the persistent value \a variant.

  \sa toSetting()
*/

Id Id::fromSetting(const QVariant &variant)
{
    const QByteArray ba = variant.toString().toUtf8();
    if (ba.isEmpty())
        return Id();
    return Id(theId(ba.data(), ba.size()));
}

QSet<Id> Id::fromStringList(const QStringList &list)
{
    return transform<QSet<Id>>(list, &Id::fromString);
}

QStringList Id::toStringList(const QSet<Id> &ids)
{
    return transform(sorted(toList(ids)), &Id::toString);
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
    return Id(theId(ba.data(), ba.size()));
}

/*!
  \overload
*/
Id Id::withSuffix(qsizetype suffix) const
{
    return withSuffix(int(suffix));
}

/*!
  \overload
*/
Id Id::withSuffix(const char suffix) const
{
    const QByteArray ba = name() + suffix;
    return Id(theId(ba.data(), ba.size()));
}

/*!
  \overload
*/

Id Id::withSuffix(const char *suffix) const
{
    const QByteArray ba = name() + suffix;
    return Id(theId(ba.data(), ba.size()));
}

/*!
  \overload
*/

Id Id::withSuffix(const QStringView suffix) const
{
    const QByteArray ba = name() + suffix.toUtf8();
    return Id(theId(ba.data(), ba.size()));
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
    return Id(theId(ba.data(), ba.size()));
}

bool Id::operator==(const char *name) const
{
    QReadLocker lock(&s_cacheMutex);
    const char *string = stringFromId.value(m_id).str;
    if (string && name)
        return strcmp(string, name) == 0;
    else
        return false;
}

// For debugging purposes
QTCREATOR_UTILS_EXPORT const char *nameForId(quintptr id)
{
    QReadLocker lock(&s_cacheMutex);
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
    const QByteArrayView b = baseId.name();
    const QByteArrayView n = name();
    return n.startsWith(b) ? QString::fromUtf8(n).mid(b.size()) : QString();
}

QDataStream &operator<<(QDataStream &ds, Id id)
{
    return ds << id.name().toByteArray();
}

QDataStream &operator>>(QDataStream &ds, Id &id)
{
    QByteArray ba;
    ds >> ba;
    id = Utils::Id::fromName(ba);
    return ds;
}

QDebug operator<<(QDebug dbg, const Id &id)
{
    return dbg << id.name();
}

} // namespace Utils
