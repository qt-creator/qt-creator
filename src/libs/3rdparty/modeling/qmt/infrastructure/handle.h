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

#ifndef QMT_HANDLE_H
#define QMT_HANDLE_H

#include "uid.h"
#include "qmtassert.h"

namespace qmt {

template<class T>
class Handle
{
public:
    Handle() : m_target(0) { }
    explicit Handle(const Uid &uid) : m_uid(uid), m_target(0) { }
    explicit Handle(T *t) : m_uid(t ? t->uid() : Uid()), m_target(t) { }
    Handle(const Handle &rhs) : m_uid(rhs.m_uid), m_target(rhs.m_target) { }
    template<class U>
    Handle(const Handle<U> &rhs) : m_uid(rhs.uid()), m_target(rhs.target()) { }

    bool isNull() const { return !m_uid.isValid(); }
    bool hasTarget() const { return m_target != 0; }
    Uid uid() const { return m_uid; }
    T *target() const { return m_target; }

    void setUid(const Uid &uid)
    {
        QMT_CHECK(m_target != 0 ? (m_target->uid() == uid) : true);
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
        m_target = 0;
    }

    void clearTarget() { m_target = 0; }

private:
    Uid m_uid;
    T *m_target;
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

#endif // QMT_HANDLE_H
