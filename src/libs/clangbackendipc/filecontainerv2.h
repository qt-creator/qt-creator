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

#include <vector>

namespace ClangBackEnd {
namespace V2 {

class FileContainer
{
public:
    FileContainer() = default;

    FileContainer(FilePath &&filePath,
                  Utils::SmallString &&unsavedFileContent,
                  Utils::SmallStringVector &&commandLineArguments,
                  quint32 documentRevision = 0)
        : filePath_(std::move(filePath)),
          unsavedFileContent_(std::move(unsavedFileContent)),
          commandLineArguments_(std::move(commandLineArguments)),
          documentRevision_(documentRevision)
    {
    }

    const FilePath &filePath() const
    {
        return filePath_;
    }

    const Utils::SmallStringVector &commandLineArguments() const
    {
        return commandLineArguments_;
    }

    Utils::SmallStringVector takeCommandLineArguments()
    {
        return std::move(commandLineArguments_);
    }

    const Utils::SmallString &unsavedFileContent() const
    {
        return unsavedFileContent_;
    }

    Utils::SmallString takeUnsavedFileContent()
    {
        return std::move(unsavedFileContent_);
    }

    quint32 documentRevision() const
    {
        return documentRevision_;
    }


    friend QDataStream &operator<<(QDataStream &out, const FileContainer &container)
    {
        out << container.filePath_;
        out << container.commandLineArguments_;
        out << container.unsavedFileContent_;
        out << container.documentRevision_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FileContainer &container)
    {
        in >> container.filePath_;
        in >> container.commandLineArguments_;
        in >> container.unsavedFileContent_;
        in >> container.documentRevision_;

        return in;
    }

    friend bool operator==(const FileContainer &first, const FileContainer &second)
    {
        return first.filePath_ == second.filePath_
            && first.commandLineArguments_ == second.commandLineArguments_;
    }

    friend bool operator<(const FileContainer &first, const FileContainer &second)
    {
        return std::tie(first.documentRevision_, first.filePath_, first.unsavedFileContent_, first.commandLineArguments_)
             < std::tie(second.documentRevision_, second.filePath_, second.unsavedFileContent_, second.commandLineArguments_);
    }

    FileContainer clone() const
    {
        return FileContainer(filePath_.clone(),
                             unsavedFileContent_.clone(),
                             commandLineArguments_.clone(),
                             documentRevision_);
    }

private:
    FilePath filePath_;
    Utils::SmallString unsavedFileContent_;
    Utils::SmallStringVector commandLineArguments_;
    quint32 documentRevision_ = 0;
};

using FileContainers = std::vector<FileContainer>;

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const FileContainer &container);
void PrintTo(const FileContainer &container, ::std::ostream* os);

} // namespace V2
} // namespace ClangBackEnd
