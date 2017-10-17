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
                  Utils::SmallStringVector &&commandLineArguments,
                  quint32 documentRevision = 0)
        : m_filePath(std::move(filePath)),
          m_unsavedFileContent(std::move(unsavedFileContent)),
          m_commandLineArguments(std::move(commandLineArguments)),
          m_documentRevision(documentRevision)
    {
    }

    const FilePath &filePath() const
    {
        return m_filePath;
    }

    const Utils::SmallStringVector &commandLineArguments() const
    {
        return m_commandLineArguments;
    }

    Utils::SmallStringVector takeCommandLineArguments()
    {
        return std::move(m_commandLineArguments);
    }

    const Utils::SmallString &unsavedFileContent() const
    {
        return m_unsavedFileContent;
    }

    Utils::SmallString takeUnsavedFileContent()
    {
        return std::move(m_unsavedFileContent);
    }

    quint32 documentRevision() const
    {
        return m_documentRevision;
    }


    friend QDataStream &operator<<(QDataStream &out, const FileContainer &container)
    {
        out << container.m_filePath;
        out << container.m_commandLineArguments;
        out << container.m_unsavedFileContent;
        out << container.m_documentRevision;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FileContainer &container)
    {
        in >> container.m_filePath;
        in >> container.m_commandLineArguments;
        in >> container.m_unsavedFileContent;
        in >> container.m_documentRevision;

        return in;
    }

    friend bool operator==(const FileContainer &first, const FileContainer &second)
    {
        return first.m_filePath == second.m_filePath
            && first.m_commandLineArguments == second.m_commandLineArguments;
    }

    friend bool operator<(const FileContainer &first, const FileContainer &second)
    {
        return std::tie(first.m_documentRevision, first.m_filePath, first.m_unsavedFileContent, first.m_commandLineArguments)
             < std::tie(second.m_documentRevision, second.m_filePath, second.m_unsavedFileContent, second.m_commandLineArguments);
    }

    FileContainer clone() const
    {
        return FileContainer(m_filePath.clone(),
                             m_unsavedFileContent.clone(),
                             m_commandLineArguments.clone(),
                             m_documentRevision);
    }

private:
    FilePath m_filePath;
    Utils::SmallString m_unsavedFileContent;
    Utils::SmallStringVector m_commandLineArguments;
    quint32 m_documentRevision = 0;
};

using FileContainers = std::vector<FileContainer>;

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const FileContainer &container);
std::ostream &operator<<(std::ostream &os, const FileContainer &container);

} // namespace V2
} // namespace ClangBackEnd
