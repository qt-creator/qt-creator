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

#include "filepath.h"

#include <unordered_map>

namespace ClangBackEnd {

using FilePathDict = std::unordered_map<uint, FilePath>;

class SourceFilePathContainerBase
{
public:
    SourceFilePathContainerBase() = default;
    SourceFilePathContainerBase(std::unordered_map<uint, FilePath> &&filePathHash)
        : m_filePathHash(std::move(filePathHash))
    {
    }

    void insertFilePath(uint fileId, Utils::PathString &&filePath)
    {
        if (m_filePathHash.find(fileId) == m_filePathHash.end())
            m_filePathHash.emplace(fileId, FilePath(std::move(filePath)));
    }

    void reserve(std::size_t size)
    {
        m_filePathHash.reserve(size / 3);
    }

    const FilePathDict &filePaths() const
    {
        return m_filePathHash;
    }

protected:
    FilePathDict m_filePathHash;
};

} // namespace ClangBackEnd
