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

#include "clangbackendipc_global.h"
#include "filepath.h"

#include <utils/smallstringvector.h>

namespace ClangBackEnd {

class CMBIPC_EXPORT RequestSourceLocationsForRenamingMessage
{
public:
    RequestSourceLocationsForRenamingMessage() = default;
    RequestSourceLocationsForRenamingMessage(FilePath &&filePath,
                                             uint line,
                                             uint column,
                                             Utils::SmallString &&unsavedContent,
                                             Utils::SmallStringVector &&commandLine,
                                             int textDocumentRevision)
        : filePath_(std::move(filePath)),
          unsavedContent_(std::move(unsavedContent)),
          commandLine_(std::move(commandLine)),
          line_(line),
          column_(column),
          revision(textDocumentRevision)
    {}

    const FilePath &filePath() const
    {
        return filePath_;
    }

    uint line() const
    {
        return line_;
    }

    uint column() const
    {
        return column_;
    }

    const Utils::SmallString &unsavedContent() const
    {
        return unsavedContent_;
    }

    const Utils::SmallStringVector &commandLine() const
    {
        return commandLine_;
    }

    int textDocumentRevision() const
    {
        return revision;
    }

    friend QDataStream &operator<<(QDataStream &out, const RequestSourceLocationsForRenamingMessage &message)
    {
        out << message.filePath_;
        out << message.unsavedContent_;
        out << message.commandLine_;
        out << message.line_;
        out << message.column_;
        out << message.revision;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RequestSourceLocationsForRenamingMessage &message)
    {
        in >> message.filePath_;
        in >> message.unsavedContent_;
        in >> message.commandLine_;
        in >> message.line_;
        in >> message.column_;
        in >> message.revision;

        return in;
    }

    friend bool operator==(const RequestSourceLocationsForRenamingMessage &first, const RequestSourceLocationsForRenamingMessage &second)
    {
        return first.filePath_ == second.filePath_
                && first.line_ == second.line_
                && first.column_ == second.column_
                && first.revision == second.revision
                && first.unsavedContent_ == second.unsavedContent_
                && first.commandLine_ == second.commandLine_;
    }

    RequestSourceLocationsForRenamingMessage clone() const
    {
        return RequestSourceLocationsForRenamingMessage(filePath_.clone(),
                                                        line_, column_,
                                                        unsavedContent_.clone(),
                                                        commandLine_.clone(),
                                                        revision);
    }

private:
    FilePath filePath_;
    Utils::SmallString unsavedContent_;
    Utils::SmallStringVector commandLine_;
    uint line_ = 1;
    uint column_ = 1;
    int revision = 1;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const RequestSourceLocationsForRenamingMessage &message);
void PrintTo(const RequestSourceLocationsForRenamingMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(RequestSourceLocationsForRenamingMessage)
} // namespace ClangBackEnd
