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
