// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace qark {

class Flag
{
public:
    typedef unsigned int MaskType;

    explicit Flag() : m_mask(m_nextMask) { m_nextMask *= 2; }

    MaskType mask() const { return m_mask; }

private:
    static MaskType m_nextMask;
    MaskType m_mask = 0;
};

} // namespace qark
