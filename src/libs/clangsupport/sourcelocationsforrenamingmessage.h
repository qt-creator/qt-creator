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
        : m_symbolName(std::move(symbolName)),
          m_sourceLocationContainer(std::move(sourceLocationContainer)),
          m_revision(textDocumentRevision)
    {}

    const Utils::SmallString &symbolName() const
    {
        return m_symbolName;
    }

    int textDocumentRevision() const
    {
        return m_revision;
    }

    const SourceLocationsContainer &sourceLocations() const
    {
        return m_sourceLocationContainer;
    }

    friend QDataStream &operator<<(QDataStream &out, const SourceLocationsForRenamingMessage &message)
    {
        out << message.m_symbolName;
        out << message.m_sourceLocationContainer;
        out << message.m_revision;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceLocationsForRenamingMessage &message)
    {
        in >> message.m_symbolName;
        in >> message.m_sourceLocationContainer;
        in >> message.m_revision;

        return in;
    }

    friend bool operator==(const SourceLocationsForRenamingMessage &first, const SourceLocationsForRenamingMessage &second)
    {
        return first.m_revision == second.m_revision
            && first.m_symbolName == second.m_symbolName
            && first.m_sourceLocationContainer == second.m_sourceLocationContainer;
    }

    SourceLocationsForRenamingMessage clone() const
    {
        return SourceLocationsForRenamingMessage(m_symbolName.clone(),
                                                 m_sourceLocationContainer.clone(),
                                                 m_revision);
    }

private:
    Utils::SmallString m_symbolName;
    SourceLocationsContainer m_sourceLocationContainer;
    int m_revision = 0;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const SourceLocationsForRenamingMessage &message);
std::ostream &operator<<(std::ostream &os, const SourceLocationsForRenamingMessage &message);

DECLARE_MESSAGE(SourceLocationsForRenamingMessage)
} // namespace ClangBackEnd
