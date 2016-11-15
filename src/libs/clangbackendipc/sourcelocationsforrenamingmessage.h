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
        : symbolName_(std::move(symbolName)),
          sourceLocationContainer(std::move(sourceLocationContainer)),
          revision(textDocumentRevision)
    {}

    const Utils::SmallString &symbolName() const
    {
        return symbolName_;
    }

    int textDocumentRevision() const
    {
        return revision;
    }

    const SourceLocationsContainer &sourceLocations() const
    {
        return sourceLocationContainer;
    }

    friend QDataStream &operator<<(QDataStream &out, const SourceLocationsForRenamingMessage &message)
    {
        out << message.symbolName_;
        out << message.sourceLocationContainer;
        out << message.revision;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceLocationsForRenamingMessage &message)
    {
        in >> message.symbolName_;
        in >> message.sourceLocationContainer;
        in >> message.revision;

        return in;
    }

    friend bool operator==(const SourceLocationsForRenamingMessage &first, const SourceLocationsForRenamingMessage &second)
    {
        return first.revision == second.revision
            && first.symbolName_ == second.symbolName_
            && first.sourceLocationContainer == second.sourceLocationContainer;
    }

    SourceLocationsForRenamingMessage clone() const
    {
        return SourceLocationsForRenamingMessage(symbolName_.clone(),
                                                 sourceLocationContainer.clone(),
                                                 revision);
    }

private:
    Utils::SmallString symbolName_;
    SourceLocationsContainer sourceLocationContainer;
    int revision = 0;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const SourceLocationsForRenamingMessage &message);
void PrintTo(const SourceLocationsForRenamingMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(SourceLocationsForRenamingMessage)
} // namespace ClangBackEnd
