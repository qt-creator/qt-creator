/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "uid.h"
#include "qmtassert.h"

namespace qmt {

template<class T>
class Handle
{
public:
    Handle() = default;
    explicit Handle(const Uid &uid) : m_uid(uid) { }
    explicit Handle(T *t) : m_uid(t ? t->uid() : Uid()), m_target(t) { }
    Handle(const Handle &rhs) : m_uid(rhs.m_uid), m_target(rhs.m_target) { }
    template<class U>
    Handle(const Handle<U> &rhs) : m_uid(rhs.uid()), m_target(rhs.target()) { }

    bool isNull() const { return !m_uid.isValid(); }
    bool hasTarget() const { return m_target != nullptr; }
    Uid uid() const { return m_uid; }
    T *target() const { return m_target; }

    void setUid(const Uid &uid)
    {
        QMT_CHECK(m_target ? (m_target->uid() == uid) : true);
        m_uid = uid;
    }

    void setTarget(T *t)
    {
        m_uid = t ? t->uid() : Uid();
        m_target = t;
    }

    void clear()
    {
        m_uid = Uid();
        m_target = nullptr;
    }

    void clearTarget() { m_target = nullptr; }

private:
    Uid m_uid;
    T *m_target = nullptr;
};

template<class T>
inline int qHash(const Handle<T> &handle)
{
    return qHash(handle.uid());
}

template<class T, class U>
bool operator==(const Handle<T> &lhs, const Handle<U> &rhs)
{
    return lhs.uid() == rhs.uid();
}

template<class T>
Handle<T> makeHandle(T *t)
{
    return Handle<T>(t);
}

template<class T, class U>
Handle<T> handle_dynamic_cast(const Handle<U> &handle)
{
    if (!handle.hasTarget())
        return Handle<T>(handle.uid());
    return Handle<T>(dynamic_cast<T *>(handle.target()));
}

} // namespace qmt
