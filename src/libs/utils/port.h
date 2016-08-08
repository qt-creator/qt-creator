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
#include "qtcassert.h"
#include <QMetaType>

namespace Utils {

class QTCREATOR_UTILS_EXPORT Port
{
public:
    Port() : m_port(-1) {}
    explicit Port(quint16 port) : m_port(port) {}
    explicit Port(int port) :
        m_port((port < 0 || port > std::numeric_limits<quint16>::max()) ? -1 : port)
    {
    }

    explicit Port(uint port) :
        m_port(port > std::numeric_limits<quint16>::max() ? -1 : port)
    {
    }

    quint16 number() const { QTC_ASSERT(isValid(), return 0); return quint16(m_port); }
    bool isValid() const { return m_port != -1; }

private:
    int m_port;
};

inline bool operator<(const Port &p1, const Port &p2) { return p1.number() < p2.number(); }
inline bool operator<=(const Port &p1, const Port &p2) { return p1.number() <= p2.number(); }
inline bool operator>(const Port &p1, const Port &p2) { return p1.number() > p2.number(); }
inline bool operator>=(const Port &p1, const Port &p2) { return p1.number() >= p2.number(); }

inline bool operator==(const Port &p1, const Port &p2)
{
    return p1.isValid() ? (p2.isValid() && p1.number() == p2.number()) : !p2.isValid();
}

inline bool operator!=(const Port &p1, const Port &p2)
{
    return p1.isValid() ? (!p2.isValid() || p1.number() != p2.number()) : p2.isValid();
}

} // Utils

Q_DECLARE_METATYPE(Utils::Port)
