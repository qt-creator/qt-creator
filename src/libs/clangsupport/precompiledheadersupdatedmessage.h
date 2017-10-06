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

#include "projectpartpch.h"

namespace ClangBackEnd {

class PrecompiledHeadersUpdatedMessage
{
public:
    PrecompiledHeadersUpdatedMessage() = default;
    PrecompiledHeadersUpdatedMessage(std::vector<ProjectPartPch> &&projectPartPchs)
        : m_projectPartPchs(std::move(projectPartPchs))
    {}

    const std::vector<ProjectPartPch> &projectPartPchs() const
    {
        return m_projectPartPchs;
    }

    friend QDataStream &operator<<(QDataStream &out, const PrecompiledHeadersUpdatedMessage &message)
    {
        out << message.m_projectPartPchs;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, PrecompiledHeadersUpdatedMessage &message)
    {
        in >> message.m_projectPartPchs;

        return in;
    }

    friend bool operator==(const PrecompiledHeadersUpdatedMessage &first,
                           const PrecompiledHeadersUpdatedMessage &second)
    {
        return first.m_projectPartPchs == second.m_projectPartPchs;
    }

    PrecompiledHeadersUpdatedMessage clone() const
    {
        return PrecompiledHeadersUpdatedMessage(Utils::clone(m_projectPartPchs));
    }

private:
    std::vector<ProjectPartPch> m_projectPartPchs;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const PrecompiledHeadersUpdatedMessage &message);
std::ostream &operator<<(std::ostream &out, const PrecompiledHeadersUpdatedMessage &message);

DECLARE_MESSAGE(PrecompiledHeadersUpdatedMessage)

} // namespace ClangBackEnd
