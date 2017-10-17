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

class DynamicASTMatcherDiagnosticMessageContainer
{
public:
    DynamicASTMatcherDiagnosticMessageContainer() = default;
    DynamicASTMatcherDiagnosticMessageContainer(V2::SourceRangeContainer &&sourceRange,
                                                ClangQueryDiagnosticErrorType errorType,
                                                Utils::SmallStringVector &&arguments)
        : m_sourceRange(sourceRange),
          m_errorType(errorType),
          m_arguments(std::move(arguments))
    {
    }

    const V2::SourceRangeContainer &sourceRange() const
    {
        return m_sourceRange;
    }

    ClangQueryDiagnosticErrorType errorType() const
    {
        return m_errorType;
    }

    CLANGSUPPORT_EXPORT Utils::SmallString errorTypeText() const;

    const Utils::SmallStringVector &arguments() const
    {
        return m_arguments;
    }

    friend QDataStream &operator<<(QDataStream &out, const DynamicASTMatcherDiagnosticMessageContainer &container)
    {
        out << container.m_sourceRange;
        out << quint32(container.m_errorType);
        out << container.m_arguments;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, DynamicASTMatcherDiagnosticMessageContainer &container)
    {
        quint32 errorType;

        in >> container.m_sourceRange;
        in >> errorType;
        in >> container.m_arguments;

        container.m_errorType = static_cast<ClangQueryDiagnosticErrorType>(errorType);

        return in;
    }

    friend bool operator==(const DynamicASTMatcherDiagnosticMessageContainer &first,
                           const DynamicASTMatcherDiagnosticMessageContainer &second)
    {
        return first.m_errorType == second.m_errorType
            && first.m_sourceRange == second.m_sourceRange
            && first.m_arguments == second.m_arguments;
    }

    DynamicASTMatcherDiagnosticMessageContainer clone() const
    {
        return *this;
    }

private:
    V2::SourceRangeContainer m_sourceRange;
    ClangQueryDiagnosticErrorType m_errorType = ClangQueryDiagnosticErrorType::None;
    Utils::SmallStringVector m_arguments;
};

using DynamicASTMatcherDiagnosticMessageContainers = std::vector<DynamicASTMatcherDiagnosticMessageContainer>;

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const DynamicASTMatcherDiagnosticMessageContainer &container);
std::ostream &operator<<(std::ostream &os, const DynamicASTMatcherDiagnosticMessageContainer &container);

} // namespace ClangBackEnd
