// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace qark {
namespace impl {

class ObjectId
{
public:
    explicit ObjectId(int id = -1) : m_id(id) { }

    int get() const { return m_id; }
    void set(int id) { m_id = id; }
    bool isValid() const { return m_id >= 0; }

    ObjectId operator++(int) { ObjectId id(*this); ++m_id; return id; }

private:
    int m_id = 1;
};

inline bool operator<(const ObjectId &lhs, const ObjectId &rhs)
{
    return lhs.get() < rhs.get();
}

} // namespace impl
} // namespace qark
