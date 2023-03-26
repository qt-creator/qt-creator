// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Timeline {

template<class Base>
class SafeCastable
{
public:
    template<class Derived>
    bool is() const
    {
        return static_cast<const Base *>(this)->classId() == Derived::staticClassId;
    }

    template<class Derived>
    Derived &asRef()
    {
        Q_ASSERT(is<Derived>());
        return static_cast<Derived &>(*this);
    }

    template<class Derived>
    const Derived &asConstRef() const
    {
        Q_ASSERT(is<Derived>());
        return static_cast<const Derived &>(*this);
    }

    template<class Derived>
    Derived &&asRvalueRef()
    {
        Q_ASSERT(is<Derived>());
        return static_cast<Derived &&>(*this);
    }
};

} // namespace Timeline
