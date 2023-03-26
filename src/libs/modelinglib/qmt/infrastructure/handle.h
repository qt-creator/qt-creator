// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    Handle &operator=(const Handle &) = default;

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

    friend auto qHash(const Handle<T> &handle) { return qHash(handle.uid()); }

private:
    Uid m_uid;
    T *m_target = nullptr;
};

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
