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

    typedef QList<Handle<T> > type;

    typedef typename type::iterator iterator;

    typedef typename type::const_iterator const_iterator;

public:

    explicit Handles(bool take_ownership = false) : _take_ownership(take_ownership) { }

    Handles(const Handles<T> &rhs)
        : _handle_list(rhs._handle_list),
          _take_ownership(false)
    {
    }

    Handles(const Handles<T> &rhs, bool take_ownership)
        : _handle_list(rhs._handle_list),
          _take_ownership(take_ownership)
    {
        if (_take_ownership && rhs._take_ownership) {
            const_cast<Handles<T> &>(rhs)._handle_list.clear();
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
            _handle_list = rhs._handle_list;
            if (_take_ownership && rhs._take_ownership) {
                const_cast<Handles<T> &>(rhs)._handle_list.clear();
            }
        }
        return *this;
    }

public:

    bool takesOwnership() const { return _take_ownership; }

    bool isEmpty() const { return _handle_list.empty(); }

    int size() const { return _handle_list.size(); }

    bool contains(const Uid &uid) const
    {
        foreach (const Handle<T> &handle, _handle_list) {
            if (handle.getUid() == uid) {
                return true;
            }
        }
        return false;
    }

    bool contains(const T *t) const
    {
        QMT_CHECK(t);
        return contains(t->getUid());
    }

    T *find(const Uid &uid) const
    {
        foreach (const Handle<T> &handle, _handle_list) {
            if (handle.getUid() == uid) {
                return handle.getTarget();
            }
        }
        return 0;
    }

    T *at(int index) const
    {
        QMT_CHECK(index >= 0 && index < _handle_list.size());
        return _handle_list.at(index).getTarget();
    }

    T *at(int index)
    {
        QMT_CHECK(index >= 0 && index < _handle_list.size());
        return _handle_list.at(index);
    }

    int indexOf(const Uid &uid) const
    {
        int index = 0;
        foreach (const Handle<T> &handle, _handle_list) {
            if (handle.getUid() == uid) {
                return index;
            }
            ++index;
        }
        return -1;
    }

    int indexOf(const T *t) const
    {
        QMT_CHECK(t);
        return indexOf(t->getUid());
    }

public:

    const type &get() const { return _handle_list; }

    type take()
    {
        type handles = _handle_list;
        _handle_list.clear();
        return handles;
    }

    void set(const type &handles) {
        reset();
        _handle_list = handles;
    }

    void reset()
    {
        if (_take_ownership) {
            foreach (const Handle<T> &handle, _handle_list) {
                delete handle.getTarget();
            }
        }
        _handle_list.clear();
    }

public:
    iterator begin() { return _handle_list.begin(); }

    iterator end() { return _handle_list.end(); }

    const_iterator begin() const { return _handle_list.begin(); }

    const_iterator end() const { return _handle_list.end(); }

public:

    void add(const Uid &uid)
    {
        QMT_CHECK(uid.isValid());
        _handle_list.append(Handle<T>(uid));
    }

    void add(T *t)
    {
        QMT_CHECK(t);
        _handle_list.append(Handle<T>(t));
    }

    void insert(int before_index, const Uid &uid)
    {
        QMT_CHECK(before_index >= 0 && before_index <= _handle_list.size());
        QMT_CHECK(uid.isValid());
        _handle_list.insert(before_index, Handle<T>(uid));
    }

    void insert(int before_index, T *t)
    {
        QMT_CHECK(before_index >= 0 && before_index <= _handle_list.size());
        QMT_CHECK(t);
        _handle_list.insert(before_index, Handle<T>(t));
    }

    void remove(int index)
    {
        QMT_CHECK(index >= 0 && index < size());
        if (_take_ownership) {
            T *t = _handle_list.at(index).getTarget();
            _handle_list.removeAt(index);
            delete t;
        } else {
            _handle_list.removeAt(index);
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
        T *t = _handle_list.at(index).getTarget();
        _handle_list.removeAt(index);
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

    type _handle_list;

    bool _take_ownership;
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
