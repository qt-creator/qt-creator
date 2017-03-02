/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "utils_global.h"

#include <QPointer>
#include <QList>

/*!
    \class ObjectPool

    The ObjectPool class template provides parts of the functionality of the
    global PluginManager object pool but is intented to be used with smaller
    set of objects, typically with same base type (e.g. factories) only.

    The ObjectPool takes ownership of add items if and only if the item
    does not have a QObject parent.

    Items owned by the ObjectPool are destructed when the pool is destructed,
    the other items are taken care of by their QObject parent according to
    the usual parent/child behavior.

*/

namespace Utils {

template <typename T>
class ObjectPool
{
public:
    ObjectPool() {}

    ~ObjectPool()
    {
        for (const QPointer<T> &ptr : m_objects) {
            if (!ptr.isNull() && !ptr->parent())
                delete ptr.data();
        }
    }

    int size() const
    {
        int res = 0;
        for (const QPointer<T> &ptr : m_objects)
            res += ptr.data() != nullptr;
        return res;
    }

    void addObject(T *t)
    {
        m_objects.append(t);
    }

    template <typename Func>
    void forEachObject(const Func &func) const
    {
        for (const QPointer<T> &ptr : m_objects) {
            if (!ptr.isNull())
                func(ptr.data());
        }
    }

    template <typename Func>
    T *findObject(const Func &func) const
    {
        for (const QPointer<T> &ptr : m_objects) {
            if (!ptr.isNull() && func(ptr.data()))
                return ptr.data();
        }
        return nullptr;
    }

private:
    ObjectPool(const ObjectPool &) = delete;
    ObjectPool &operator=(const ObjectPool &) = delete;

    QList<QPointer<T>> m_objects;
};

} // namespace Utils
