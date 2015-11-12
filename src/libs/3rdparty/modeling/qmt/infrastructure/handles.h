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

#ifndef QMT_HANDLES_H
#define QMT_HANDLES_H

#include "handle.h"
#include "qmt/infrastructure/qmtassert.h"

#include <QList>
#include <QHash>


namespace qmt {

template<typename T>
class Handles
{
public:

    typedef QList<Handle<T> > value_type;

    typedef typename value_type::iterator iterator;

    typedef typename value_type::const_iterator const_iterator;

public:

    explicit Handles(bool takeOwnership = false) : m_takesOwnership(takeOwnership) { }

    Handles(const Handles<T> &rhs)
        : m_handleList(rhs.m_handleList),
          m_takesOwnership(false)
    {
    }

    Handles(const Handles<T> &rhs, bool takeOwnership)
        : m_handleList(rhs.m_handleList),
          m_takesOwnership(takeOwnership)
    {
        if (m_takesOwnership && rhs.m_takesOwnership) {
            const_cast<Handles<T> &>(rhs).m_handleList.clear();
        }
    }

    ~Handles()
    {
        reset();
    }

public:

    Handles<T> operator=(const Handles<T> &rhs)
    {
        if (this != &rhs) {
            m_handleList = rhs.m_handleList;
            if (m_takesOwnership && rhs.m_takesOwnership) {
                const_cast<Handles<T> &>(rhs).m_handleList.clear();
            }
        }
        return *this;
    }

public:

    bool takesOwnership() const { return m_takesOwnership; }

    bool isEmpty() const { return m_handleList.empty(); }

    int size() const { return m_handleList.size(); }

    bool contains(const Uid &uid) const
    {
        foreach (const Handle<T> &handle, m_handleList) {
            if (handle.uid() == uid) {
                return true;
            }
        }
        return false;
    }

    bool contains(const T *t) const
    {
        QMT_CHECK(t);
        return contains(t->uid());
    }

    T *find(const Uid &uid) const
    {
        foreach (const Handle<T> &handle, m_handleList) {
            if (handle.uid() == uid) {
                return handle.target();
            }
        }
        return 0;
    }

    T *at(int index) const
    {
        QMT_CHECK(index >= 0 && index < m_handleList.size());
        return m_handleList.at(index).target();
    }

    T *at(int index)
    {
        QMT_CHECK(index >= 0 && index < m_handleList.size());
        return m_handleList.at(index);
    }

    int indexOf(const Uid &uid) const
    {
        int index = 0;
        foreach (const Handle<T> &handle, m_handleList) {
            if (handle.uid() == uid) {
                return index;
            }
            ++index;
        }
        return -1;
    }

    int indexOf(const T *t) const
    {
        QMT_CHECK(t);
        return indexOf(t->uid());
    }

public:

    const value_type &get() const { return m_handleList; }

    value_type take()
    {
        value_type handles = m_handleList;
        m_handleList.clear();
        return handles;
    }

    void set(const value_type &handles) {
        reset();
        m_handleList = handles;
    }

    void reset()
    {
        if (m_takesOwnership) {
            foreach (const Handle<T> &handle, m_handleList) {
                delete handle.target();
            }
        }
        m_handleList.clear();
    }

public:
    iterator begin() { return m_handleList.begin(); }

    iterator end() { return m_handleList.end(); }

    const_iterator begin() const { return m_handleList.begin(); }

    const_iterator end() const { return m_handleList.end(); }

public:

    void add(const Uid &uid)
    {
        QMT_CHECK(uid.isValid());
        m_handleList.append(Handle<T>(uid));
    }

    void add(T *t)
    {
        QMT_CHECK(t);
        m_handleList.append(Handle<T>(t));
    }

    void insert(int beforeIndex, const Uid &uid)
    {
        QMT_CHECK(beforeIndex >= 0 && beforeIndex <= m_handleList.size());
        QMT_CHECK(uid.isValid());
        m_handleList.insert(beforeIndex, Handle<T>(uid));
    }

    void insert(int beforeIndex, T *t)
    {
        QMT_CHECK(beforeIndex >= 0 && beforeIndex <= m_handleList.size());
        QMT_CHECK(t);
        m_handleList.insert(beforeIndex, Handle<T>(t));
    }

    void remove(int index)
    {
        QMT_CHECK(index >= 0 && index < size());
        if (m_takesOwnership) {
            T *t = m_handleList.at(index).target();
            m_handleList.removeAt(index);
            delete t;
        } else {
            m_handleList.removeAt(index);
        }
    }

    void remove(const Uid &uid)
    {
        remove(indexOf(uid));
    }

    void remove(T *t)
    {
        QMT_CHECK(t);
        remove(indexOf(t));
    }

    T * take(int index)
    {
        QMT_CHECK(index >= 0 && index < size());
        T *t = m_handleList.at(index).target();
        m_handleList.removeAt(index);
        return t;
    }

    T *take(const Uid &uid)
    {
        return take(indexOf(uid));
    }

    T *take(T *t)
    {
        QMT_CHECK(t);
        return take(indexOf(t));
    }

private:

    value_type m_handleList;

    bool m_takesOwnership;
};

template<typename T>
bool operator==(const Handles<T> &lhs, const Handles<T> &rhs)
{
    return lhs.get() == rhs.get();
}

template<typename T>
bool operator!=(const Handles<T> &lhs, const Handles<T> &rhs) { return !(lhs == rhs); }

}

#endif // QMT_HANDLES_H
