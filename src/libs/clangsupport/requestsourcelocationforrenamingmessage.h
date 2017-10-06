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

#include "clangsupport_global.h"
#include "filepath.h"

#include <utils/smallstringvector.h>

namespace ClangBackEnd {

class CLANGSUPPORT_EXPORT RequestSourceLocationsForRenamingMessage
{
public:
    RequestSourceLocationsForRenamingMessage() = default;
    RequestSourceLocationsForRenamingMessage(FilePath &&filePath,
                                             uint line,
                                             uint column,
                                             Utils::SmallString &&unsavedContent,
                                             Utils::SmallStringVector &&commandLine,
                                             int textDocumentRevision)
        : m_filePath(std::move(filePath)),
          m_unsavedContent(std::move(unsavedContent)),
          m_commandLine(std::move(commandLine)),
          m_line(line),
          m_column(column),
          m_revision(textDocumentRevision)
    {}

    const FilePath &filePath() const
    {
        return m_filePath;
    }

    uint line() const
    {
        return m_line;
    }

    uint column() const
    {
        return m_column;
    }

    const Utils::SmallString &unsavedContent() const
    {
        return m_unsavedContent;
    }

    const Utils::SmallStringVector &commandLine() const
    {
        return m_commandLine;
    }

    int textDocumentRevision() const
    {
        return m_revision;
    }

    friend QDataStream &operator<<(QDataStream &out, const RequestSourceLocationsForRenamingMessage &message)
    {
        out << message.m_filePath;
        out << message.m_unsavedContent;
        out << message.m_commandLine;
        out << message.m_line;
        out << message.m_column;
        out << message.m_revision;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RequestSourceLocationsForRenamingMessage &message)
    {
        in >> message.m_filePath;
        in >> message.m_unsavedContent;
        in >> message.m_commandLine;
        in >> message.m_line;
        in >> message.m_column;
        in >> message.m_revision;

        return in;
    }

    friend bool operator==(const RequestSourceLocationsForRenamingMessage &first, const RequestSourceLocationsForRenamingMessage &second)
    {
        return first.m_filePath == second.m_filePath
                && first.m_line == second.m_line
                && first.m_column == second.m_column
                && first.m_revision == second.m_revision
                && first.m_unsavedContent == second.m_unsavedContent
                && first.m_commandLine == second.m_commandLine;
    }

    RequestSourceLocationsForRenamingMessage clone() const
    {
        return RequestSourceLocationsForRenamingMessage(m_filePath.clone(),
                                                        m_line, m_column,
                                                        m_unsavedContent.clone(),
                                                        m_commandLine.clone(),
                                                        m_revision);
    }

private:
    FilePath m_filePath;
    Utils::SmallString m_unsavedContent;
    Utils::SmallStringVector m_commandLine;
    uint m_line = 1;
    uint m_column = 1;
    int m_revision = 1;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const RequestSourceLocationsForRenamingMessage &message);
std::ostream &operator<<(std::ostream &os, const RequestSourceLocationsForRenamingMessage &message);

DECLARE_MESSAGE(RequestSourceLocationsForRenamingMessage)
} // namespace ClangBackEnd
