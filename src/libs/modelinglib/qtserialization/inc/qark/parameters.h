// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "flag.h"

namespace qark {

class Parameters
{
public:
    Parameters() = default;

    explicit Parameters(const Flag &flag)
        : m_flags(flag.mask())
    {
    }

    void setFlag(const Flag &flag) { m_flags |= flag.mask(); }
    void clearFlag(const Flag &flag) { m_flags &= ~flag.mask(); }
    bool hasFlag(const Flag &flag) const { return (m_flags & flag.mask()) != 0; }

    bool takeFlag(const Flag &flag)
    {
        bool f = (m_flags & flag.mask()) != 0;
        m_flags &= ~flag.mask();
        return f;
    }

private:
    Flag::MaskType m_flags = 0;
};

} // namespace qark
