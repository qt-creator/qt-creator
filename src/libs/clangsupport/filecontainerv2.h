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

#include <vector>

namespace ClangBackEnd {
namespace V2 {

class FileContainer
{
public:
    FileContainer() = default;

    FileContainer(FilePath &&filePath,
                  Utils::SmallString &&unsavedFileContent,
                  Utils::SmallStringVector &&commandLineArguments = {},
                  quint32 documentRevision = 0)
        : filePath(std::move(filePath)),
          unsavedFileContent(std::move(unsavedFileContent)),
          commandLineArguments(std::move(commandLineArguments)),
          documentRevision(documentRevision)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const FileContainer &container)
    {
        out << container.filePath;
        out << container.commandLineArguments;
        out << container.unsavedFileContent;
        out << container.documentRevision;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FileContainer &container)
    {
        in >> container.filePath;
        in >> container.commandLineArguments;
        in >> container.unsavedFileContent;
        in >> container.documentRevision;

        return in;
    }

    friend bool operator==(const FileContainer &first, const FileContainer &second)
    {
        return first.filePath == second.filePath
            && first.commandLineArguments == second.commandLineArguments;
    }

    friend bool operator<(const FileContainer &first, const FileContainer &second)
    {
        return std::tie(first.filePath, first.documentRevision, first.unsavedFileContent, first.commandLineArguments)
             < std::tie(second.filePath, second.documentRevision, second.unsavedFileContent, second.commandLineArguments);
    }

    FileContainer clone() const
    {
        return FileContainer(filePath.clone(),
                             unsavedFileContent.clone(),
                             commandLineArguments.clone(),
                             documentRevision);
    }

public:
    FilePath filePath;
    Utils::SmallString unsavedFileContent;
    Utils::SmallStringVector commandLineArguments;
    quint32 documentRevision = 0;
};

using FileContainers = std::vector<FileContainer>;

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const FileContainer &container);

} // namespace V2
} // namespace ClangBackEnd
