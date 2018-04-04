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

#include "dynamicastmatcherdiagnosticmessagecontainer.h"
#include "dynamicastmatcherdiagnosticcontextcontainer.h"

namespace ClangBackEnd {

class DynamicASTMatcherDiagnosticContainer
{
public:
    DynamicASTMatcherDiagnosticContainer() = default;
    DynamicASTMatcherDiagnosticContainer(DynamicASTMatcherDiagnosticMessageContainers &&messages,
                                         DynamicASTMatcherDiagnosticContextContainers &&contexts)
        : messages(std::move(messages)),
          contexts(std::move(contexts))
    {
    }

    void insertMessage(V2::SourceRangeContainer &&sourceRange,
                       ClangQueryDiagnosticErrorType errorType,
                       Utils::SmallStringVector &&arguments) {
        messages.emplace_back(std::move(sourceRange), errorType, std::move(arguments));
    }

    void insertContext(V2::SourceRangeContainer &&sourceRange,
                       ClangQueryDiagnosticContextType contextType,
                       Utils::SmallStringVector &&arguments) {
        contexts.emplace_back(std::move(sourceRange), contextType, std::move(arguments));
    }

    friend QDataStream &operator<<(QDataStream &out, const DynamicASTMatcherDiagnosticContainer &container)
    {
        out << container.messages;
        out << container.contexts;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, DynamicASTMatcherDiagnosticContainer &container)
    {
        in >> container.messages;
        in >> container.contexts;

        return in;
    }

    friend bool operator==(const DynamicASTMatcherDiagnosticContainer &first,
                           const DynamicASTMatcherDiagnosticContainer &second)
    {
        return first.messages == second.messages
            && first.contexts == second.contexts;
    }

    DynamicASTMatcherDiagnosticContainer clone() const
    {
        return *this;
    }

public:
    DynamicASTMatcherDiagnosticMessageContainers messages;
    DynamicASTMatcherDiagnosticContextContainers contexts;
};

using DynamicASTMatcherDiagnosticContainers = std::vector<DynamicASTMatcherDiagnosticContainer>;

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const DynamicASTMatcherDiagnosticContainer &container);

} // namespace ClangBackEnd
