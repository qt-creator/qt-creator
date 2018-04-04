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

#include "sourcelocationscontainer.h"

#include <utils/smallstring.h>

namespace ClangBackEnd {

class SourceLocationsForRenamingMessage
{
public:
    SourceLocationsForRenamingMessage() = default;
    SourceLocationsForRenamingMessage(Utils::SmallString &&symbolName,
                                      SourceLocationsContainer &&sourceLocationContainer,
                                      int textDocumentRevision)
        : symbolName(std::move(symbolName)),
          sourceLocations(std::move(sourceLocationContainer)),
          textDocumentRevision(textDocumentRevision)
    {}

    friend QDataStream &operator<<(QDataStream &out, const SourceLocationsForRenamingMessage &message)
    {
        out << message.symbolName;
        out << message.sourceLocations;
        out << message.textDocumentRevision;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceLocationsForRenamingMessage &message)
    {
        in >> message.symbolName;
        in >> message.sourceLocations;
        in >> message.textDocumentRevision;

        return in;
    }

    friend bool operator==(const SourceLocationsForRenamingMessage &first, const SourceLocationsForRenamingMessage &second)
    {
        return first.textDocumentRevision == second.textDocumentRevision
            && first.symbolName == second.symbolName
            && first.sourceLocations == second.sourceLocations;
    }

    SourceLocationsForRenamingMessage clone() const
    {
        return SourceLocationsForRenamingMessage(symbolName.clone(),
                                                 sourceLocations.clone(),
                                                 textDocumentRevision);
    }

public:
    Utils::SmallString symbolName;
    SourceLocationsContainer sourceLocations;
    int textDocumentRevision = 0;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const SourceLocationsForRenamingMessage &message);

DECLARE_MESSAGE(SourceLocationsForRenamingMessage)
} // namespace ClangBackEnd
