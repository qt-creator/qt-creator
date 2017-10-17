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

#include "dynamicmatcherdiagnostics.h"
#include "sourcerangecontainerv2.h"

#include <utils/smallstringio.h>

namespace ClangBackEnd {

class DynamicASTMatcherDiagnosticContextContainer
{
public:
    DynamicASTMatcherDiagnosticContextContainer() = default;
    DynamicASTMatcherDiagnosticContextContainer(V2::SourceRangeContainer &&sourceRange,
                                                ClangQueryDiagnosticContextType contextType,
                                                Utils::SmallStringVector &&arguments)
        : m_sourceRange(sourceRange),
          m_contextType(contextType),
          m_arguments(std::move(arguments))
    {
    }

    const V2::SourceRangeContainer &sourceRange() const
    {
        return m_sourceRange;
    }

    ClangQueryDiagnosticContextType contextType() const
    {
        return m_contextType;
    }

    CLANGSUPPORT_EXPORT Utils::SmallString contextTypeText() const;

    const Utils::SmallStringVector &arguments() const
    {
        return m_arguments;
    }

    friend QDataStream &operator<<(QDataStream &out, const DynamicASTMatcherDiagnosticContextContainer &container)
    {
        out << container.m_sourceRange;
        out << quint32(container.m_contextType);
        out << container.m_arguments;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, DynamicASTMatcherDiagnosticContextContainer &container)
    {
        quint32 contextType;

        in >> container.m_sourceRange;
        in >> contextType;
        in >> container.m_arguments;

        container.m_contextType = static_cast<ClangQueryDiagnosticContextType>(contextType);

        return in;
    }

    friend bool operator==(const DynamicASTMatcherDiagnosticContextContainer &first,
                           const DynamicASTMatcherDiagnosticContextContainer &second)
    {
        return first.m_contextType == second.m_contextType
            && first.m_sourceRange == second.m_sourceRange
            && first.m_arguments == second.m_arguments;
    }

    DynamicASTMatcherDiagnosticContextContainer clone() const
    {
        return DynamicASTMatcherDiagnosticContextContainer(m_sourceRange.clone(),
                                                           m_contextType,
                                                           m_arguments.clone());
    }

private:
    V2::SourceRangeContainer m_sourceRange;
    ClangQueryDiagnosticContextType m_contextType = ClangQueryDiagnosticContextType::MatcherArg;
    Utils::SmallStringVector m_arguments;
};

using DynamicASTMatcherDiagnosticContextContainers = std::vector<DynamicASTMatcherDiagnosticContextContainer>;

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const DynamicASTMatcherDiagnosticContextContainer &container);
std::ostream &operator<<(std::ostream &os, const DynamicASTMatcherDiagnosticContextContainer &container);

} // namespace ClangBackEnd
