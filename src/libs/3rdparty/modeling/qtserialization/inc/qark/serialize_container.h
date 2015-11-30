/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#ifndef QARK_SERIALIZE_CONTAINER_H
#define QARK_SERIALIZE_CONTAINER_H

#include "parameters.h"

#include <QList>
#include <QHash>

namespace qark {

static Flag ENFORCE_REFERENCED_ITEMS;

// QList

template<class Archive, class T>
inline void save(Archive &archive, const QList<T> &list, const Parameters &)
{
    archive << tag("qlist");
    foreach (const T &t, list)
        archive << attr(QStringLiteral("item"), t);
    archive << end;
}

template<class Archive, class T>
inline void save(Archive &archive, const QList<T *> &list, const Parameters &parameters)
{
    archive << tag("qlist");
    if (parameters.hasFlag(ENFORCE_REFERENCED_ITEMS)) {
        foreach (const T *t, list)
            archive << ref(QStringLiteral("item"), t);
    } else {
        foreach (const T *t, list)
            archive << attr(QStringLiteral("item"), t);
    }
    archive << end;
}

template<class Archive, class T>
inline void load(Archive &archive, QList<T> &list, const Parameters &)
{
    archive >> tag(QStringLiteral("qlist"));
    archive >> attr<QList<T>, const T &>(QStringLiteral("item"), list, &QList<T>::append);
    archive >> end;
}

template<class Archive, class T>
inline void load(Archive &archive, QList<T *> &list, const Parameters &parameters)
{
    archive >> tag(QStringLiteral("qlist"));
    if (parameters.hasFlag(ENFORCE_REFERENCED_ITEMS)) {
        // why does the following line not compile but the line below selects the correct function?
        //archive >> ref<QList<T *>, T * const &>("item", list, &QList<T *>::append);
        archive >> ref(QStringLiteral("item"), list, &QList<T *>::append);
    } else {
        archive >> attr<QList<T *>, T * const &>(QStringLiteral("item"), list, &QList<T *>::append);
    }
    archive >> end;
}

// QSet

template<class Archive, class T>
inline void save(Archive &archive, const QSet<T> &set, const Parameters &)
{
    archive << tag("qset");
    foreach (const T &t, set)
        archive << attr(QStringLiteral("item"), t);
    archive << end;
}

template<class Archive, class T>
inline void save(Archive &archive, const QSet<T *> &set, const Parameters &parameters)
{
    archive << tag("qset");
    if (parameters.hasFlag(ENFORCE_REFERENCED_ITEMS)) {
        foreach (const T *t, set)
            archive << ref(QStringLiteral("item"), t);
    } else {
        foreach (const T *t, set)
            archive << attr(QStringLiteral("item"), t);
    }
    archive << end;
}

namespace impl {

template<typename T>
void insertIntoSet(QSet<T> &set, const T &t) {
    set.insert(t);
}

} // namespace impl

template<class Archive, class T>
inline void load(Archive &archive, QSet<T> &set, const Parameters &)
{
    archive >> tag(QStringLiteral("qset"));
    archive >> attr<QSet<T>, const T &>(QStringLiteral("item"), set, &impl::insertIntoSet<T>);
    archive >> end;
}

template<class Archive, class T>
inline void load(Archive &archive, QSet<T *> &set, const Parameters &parameters)
{
    archive >> tag(QStringLiteral("qset"));
    if (parameters.hasFlag(ENFORCE_REFERENCED_ITEMS))
        archive >> ref(QStringLiteral("item"), set, &impl::insertIntoSet<T *>);
    else
        archive >> attr<QSet<T *>, T * const &>(QStringLiteral("item"), set,
                                                &impl::insertIntoSet<T *>);
    archive >> end;
}

// QHash

namespace impl {

template<typename KEY, typename VALUE>
class KeyValuePair
{
public:
    KeyValuePair() { }
    explicit KeyValuePair(const KEY &key, const VALUE &value) : m_key(key), m_value(value) { }

    KEY m_key;
    VALUE m_value;
};

} // namespace impl

template<class Archive, class KEY, class VALUE>
inline void save(Archive &archive, const impl::KeyValuePair<KEY, VALUE> &pair, const Parameters &)
{
    archive << tag(QStringLiteral("pair"))
            << attr(QStringLiteral("key"), pair.m_key)
            << attr(QStringLiteral("value"), pair.m_value)
            << end;
}

template<class Archive, class KEY, class VALUE>
inline void load(Archive &archive, impl::KeyValuePair<KEY, VALUE> &pair, const Parameters &)
{
    archive >> tag(QStringLiteral("pair"))
            >> attr(QStringLiteral("key"), pair.m_key)
            >> attr(QStringLiteral("value"), pair.m_value)
            >> end;
}

template<class Archive, class KEY, class VALUE>
inline void save(Archive &archive, const QHash<KEY, VALUE> &hash, const Parameters &)
{
    archive << tag(QStringLiteral("qhash"));
    for (auto it = hash.begin(); it != hash.end(); ++it) {
        impl::KeyValuePair<KEY, VALUE> pair(it.key(), it.value());
        archive << attr("item", pair);
    }
    archive << end;
}

namespace impl {

template<class KEY, class VALUE>
inline void keyValuePairInsert(QHash<KEY, VALUE> &hash, const KeyValuePair<KEY, VALUE> &pair)
{
    hash.insert(pair.m_key, pair.m_value);
}

} // namespace impl

template<class Archive, class KEY, class VALUE>
inline void load(Archive &archive, QHash<KEY, VALUE> &hash, const Parameters &)
{
    archive >> tag(QStringLiteral("qhash"));
    archive >> attr(QStringLiteral("item"), hash, &impl::keyValuePairInsert<KEY, VALUE>);
    archive >> end;
}

} // namespace qark

#endif // QARK_SERIALIZE_CONTAINER_H
