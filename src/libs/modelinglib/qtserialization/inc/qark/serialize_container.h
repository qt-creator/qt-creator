// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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
    for (const T &t : list)
        archive << attr("item", t);
    archive << end;
}

template<class Archive, class T>
inline void save(Archive &archive, const QList<T *> &list, const Parameters &parameters)
{
    archive << tag("qlist");
    if (parameters.hasFlag(ENFORCE_REFERENCED_ITEMS)) {
        for (const T *t : list)
            archive << ref("item", t);
    } else {
        for (const T *t : list)
            archive << attr("item", t);
    }
    archive << end;
}

template<class Archive, class T>
inline void load(Archive &archive, QList<T> &list, const Parameters &)
{
    archive >> tag("qlist");
    void (QList<T>::*appendMethod)(const T &) = &QList<T>::append;
    archive >> attr("item", list, appendMethod);
    archive >> end;
}

template<class Archive, class T>
inline void load(Archive &archive, QList<T *> &list, const Parameters &parameters)
{
    archive >> tag("qlist");
    if (parameters.hasFlag(ENFORCE_REFERENCED_ITEMS)) {
        // why does the following line not compile but the line below selects the correct function?
        //archive >> ref<QList<T *>, T * const &>("item", list, &QList<T *>::append);
        archive >> ref("item", list, &QList<T *>::append);
    } else {
        using ParameterType = typename QList<T *>::parameter_type;
        void (QList<T *>::*appendMethod)(ParameterType) = &QList<T *>::append;
        archive >> attr("item", list, appendMethod);
    }
    archive >> end;
}

// QSet

template<class Archive, class T>
inline void save(Archive &archive, const QSet<T> &set, const Parameters &)
{
    archive << tag("qset");
    for (const T &t : set)
        archive << attr("item", t);
    archive << end;
}

template<class Archive, class T>
inline void save(Archive &archive, const QSet<T *> &set, const Parameters &parameters)
{
    archive << tag("qset");
    if (parameters.hasFlag(ENFORCE_REFERENCED_ITEMS)) {
        for (const T *t : set)
            archive << ref("item", t);
    } else {
        for (const T *t : set)
            archive << attr("item", t);
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
    archive >> tag("qset");
    archive >> attr("item", set, &impl::insertIntoSet<T>);
    archive >> end;
}

template<class Archive, class T>
inline void load(Archive &archive, QSet<T *> &set, const Parameters &parameters)
{
    archive >> tag("qset");
    if (parameters.hasFlag(ENFORCE_REFERENCED_ITEMS))
        archive >> ref("item", set, &impl::insertIntoSet<T *>);
    else
        archive >> attr("item", set, &impl::insertIntoSet<T *>);
    archive >> end;
}

// QHash

namespace impl {

template<typename KEY, typename VALUE>
class KeyValuePair
{
public:
    KeyValuePair() = default;
    KeyValuePair(const KEY &key, const VALUE &value) : m_key(key), m_value(value) { }

    KEY m_key;
    VALUE m_value;
};

} // namespace impl

template<class Archive, class KEY, class VALUE>
inline void save(Archive &archive, const impl::KeyValuePair<KEY, VALUE> &pair, const Parameters &)
{
    archive << tag("pair")
            << attr("key", pair.m_key)
            << attr("value", pair.m_value)
            << end;
}

template<class Archive, class KEY, class VALUE>
inline void load(Archive &archive, impl::KeyValuePair<KEY, VALUE> &pair, const Parameters &)
{
    archive >> tag("pair")
            >> attr("key", pair.m_key)
            >> attr("value", pair.m_value)
            >> end;
}

template<class Archive, class KEY, class VALUE>
inline void save(Archive &archive, const QHash<KEY, VALUE> &hash, const Parameters &)
{
    archive << tag("qhash");
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
    archive >> tag("qhash");
    archive >> attr("item", hash, &impl::keyValuePairInsert<KEY, VALUE>);
    archive >> end;
}

} // namespace qark
