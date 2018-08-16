/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <filepathid.h>

#include <ctime>

namespace ClangBackEnd {

class SourcesManager
{
    struct FilePathIdTime
    {
        FilePathIdTime(FilePathId filePathId, std::time_t modifiedTime)
            : filePathId(filePathId),
              modifiedTime(modifiedTime)
        {}
        FilePathId filePathId;
        std::time_t modifiedTime = 0;
    };

public:
    bool alreadyParsed(FilePathId filePathId, std::time_t modifiedTime)
    {
        auto found = std::lower_bound(m_modifiedTimeStamps.cbegin(),
                                      m_modifiedTimeStamps.cend(),
                                      filePathId,
                                      [] (FilePathIdTime entry, FilePathId filePathId) {
                return entry.filePathId < filePathId;
        });

        bool upToDate = found != m_modifiedTimeStamps.end() && found->filePathId == filePathId
                && modifiedTime <= found->modifiedTime;

        if (!upToDate)
           addOrUpdateNewEntry(filePathId, modifiedTime);

        m_dependentFilesModified = m_dependentFilesModified || !upToDate;

        return upToDate ;
    }

    void updateModifiedTimeStamps()
    {
        std::vector<FilePathIdTime> mergedModifiedTimeStamps;
        mergedModifiedTimeStamps.reserve(m_newModifiedTimeStamps.size() + m_modifiedTimeStamps.size());

        auto compare = [](FilePathIdTime first, FilePathIdTime second) {
            return first.filePathId < second.filePathId;
        };

        std::set_union(m_newModifiedTimeStamps.begin(),
                       m_newModifiedTimeStamps.end(),
                       m_modifiedTimeStamps.begin(),
                       m_modifiedTimeStamps.end(),
                       std::back_inserter(mergedModifiedTimeStamps),
                       compare);

        m_modifiedTimeStamps = std::move(mergedModifiedTimeStamps);
        m_newModifiedTimeStamps.clear();
        m_dependentFilesModified = false;
    }

    bool dependentFilesModified() const
    {
        return m_dependentFilesModified;
    }

    bool alreadyParsedAllDependentFiles(FilePathId filePathId, std::time_t modifiedTime)
    {
        return alreadyParsed(filePathId, modifiedTime) && !dependentFilesModified();
    }

private:
    void addOrUpdateNewEntry(FilePathId filePathId, std::time_t modifiedTime)
    {
        auto found = std::lower_bound(m_newModifiedTimeStamps.begin(),
                                      m_newModifiedTimeStamps.end(),
                                      filePathId,
                                      [] (FilePathIdTime entry, FilePathId filePathId) {
                return entry.filePathId < filePathId;
        });

        if (found != m_newModifiedTimeStamps.end() && found->filePathId == filePathId)
            found->modifiedTime = modifiedTime;
        else
            m_newModifiedTimeStamps.emplace(found, filePathId, modifiedTime);
    }

private:
    std::vector<FilePathIdTime> m_modifiedTimeStamps;
    std::vector<FilePathIdTime> m_newModifiedTimeStamps;
    bool m_dependentFilesModified = false;
};

}
