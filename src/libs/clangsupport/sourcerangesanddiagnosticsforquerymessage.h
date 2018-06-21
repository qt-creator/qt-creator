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

#include "sourcerangescontainer.h"
#include "dynamicastmatcherdiagnosticcontainer.h"

#include <utils/smallstring.h>

namespace ClangBackEnd {

class SourceRangesAndDiagnosticsForQueryMessage
{
public:
    SourceRangesAndDiagnosticsForQueryMessage() = default;
    SourceRangesAndDiagnosticsForQueryMessage(SourceRangesContainer &&sourceRangesContainer,
                                              std::vector<DynamicASTMatcherDiagnosticContainer> &&diagnosticContainers)
        : sourceRanges(std::move(sourceRangesContainer)),
          diagnostics(std::move(diagnosticContainers))
    {}

    SourceRangesContainer takeSourceRanges()
    {
        return std::move(sourceRanges);
    }

    friend QDataStream &operator<<(QDataStream &out, const SourceRangesAndDiagnosticsForQueryMessage &message)
    {
        out << message.sourceRanges;
        out << message.diagnostics;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceRangesAndDiagnosticsForQueryMessage &message)
    {
        in >> message.sourceRanges;
        in >> message.diagnostics;

        return in;
    }

    friend bool operator==(const SourceRangesAndDiagnosticsForQueryMessage &first, const SourceRangesAndDiagnosticsForQueryMessage &second)
    {
        return first.sourceRanges == second.sourceRanges
            && first.diagnostics == second.diagnostics;
    }

    SourceRangesAndDiagnosticsForQueryMessage clone() const
    {
        return SourceRangesAndDiagnosticsForQueryMessage(sourceRanges.clone(),
                                                         Utils::clone(diagnostics));
    }

public:
    SourceRangesContainer sourceRanges;
    DynamicASTMatcherDiagnosticContainers diagnostics;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const SourceRangesAndDiagnosticsForQueryMessage &message);

DECLARE_MESSAGE(SourceRangesAndDiagnosticsForQueryMessage)

} // namespace ClangBackEnd
