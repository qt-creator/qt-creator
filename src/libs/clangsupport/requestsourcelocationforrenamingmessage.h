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
        : filePath(std::move(filePath)),
          unsavedContent(std::move(unsavedContent)),
          commandLine(std::move(commandLine)),
          line(line),
          column(column),
          textDocumentRevision(textDocumentRevision)
    {}

    friend QDataStream &operator<<(QDataStream &out, const RequestSourceLocationsForRenamingMessage &message)
    {
        out << message.filePath;
        out << message.unsavedContent;
        out << message.commandLine;
        out << message.line;
        out << message.column;
        out << message.textDocumentRevision;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RequestSourceLocationsForRenamingMessage &message)
    {
        in >> message.filePath;
        in >> message.unsavedContent;
        in >> message.commandLine;
        in >> message.line;
        in >> message.column;
        in >> message.textDocumentRevision;

        return in;
    }

    friend bool operator==(const RequestSourceLocationsForRenamingMessage &first, const RequestSourceLocationsForRenamingMessage &second)
    {
        return first.filePath == second.filePath
                && first.line == second.line
                && first.column == second.column
                && first.textDocumentRevision == second.textDocumentRevision
                && first.unsavedContent == second.unsavedContent
                && first.commandLine == second.commandLine;
    }

    RequestSourceLocationsForRenamingMessage clone() const
    {
        return RequestSourceLocationsForRenamingMessage(filePath.clone(),
                                                        line, column,
                                                        unsavedContent.clone(),
                                                        commandLine.clone(),
                                                        textDocumentRevision);
    }

public:
    FilePath filePath;
    Utils::SmallString unsavedContent;
    Utils::SmallStringVector commandLine;
    uint line = 1;
    uint column = 1;
    int textDocumentRevision = 1;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const RequestSourceLocationsForRenamingMessage &message);

DECLARE_MESSAGE(RequestSourceLocationsForRenamingMessage)
} // namespace ClangBackEnd
