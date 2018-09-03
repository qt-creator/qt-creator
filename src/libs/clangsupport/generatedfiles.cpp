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

#include "generatedfiles.h"

namespace ClangBackEnd {

void GeneratedFiles::update(V2::FileContainers &&fileContainers)
{
    V2::FileContainers unionFileContainers;
    unionFileContainers.reserve(m_fileContainers.size() + fileContainers.size());

    auto compare = [] (const V2::FileContainer &first, const V2::FileContainer &second) {
        return first.filePath < second.filePath;
    };

    std::set_union(std::make_move_iterator(fileContainers.begin()),
                   std::make_move_iterator(fileContainers.end()),
                   std::make_move_iterator(m_fileContainers.begin()),
                   std::make_move_iterator(m_fileContainers.end()),
                   std::back_inserter(unionFileContainers),
                   compare);

    m_fileContainers = std::move(unionFileContainers);
}

void GeneratedFiles::update(const V2::FileContainers &fileContainers)
{
    V2::FileContainers unionFileContainers;
    unionFileContainers.reserve(m_fileContainers.size() + fileContainers.size());

    auto compare = [] (const V2::FileContainer &first, const V2::FileContainer &second) {
        return first.filePath < second.filePath;
    };

    std::set_union(fileContainers.begin(),
                   fileContainers.end(),
                   std::make_move_iterator(m_fileContainers.begin()),
                   std::make_move_iterator(m_fileContainers.end()),
                   std::back_inserter(unionFileContainers),
                   compare);

    m_fileContainers = std::move(unionFileContainers);
}

class Compare {
public:
    bool operator()(const FilePath &first, const FilePath &second)
    {
        return first < second;
    }
    bool operator()(const V2::FileContainer &first, const V2::FileContainer &second)
    {
        return first.filePath < second.filePath;
    }
    bool operator()(const V2::FileContainer &first, const FilePath &second)
    {
        return first.filePath < second;
    }
    bool operator()(const FilePath &first, const V2::FileContainer &second)
    {
        return first < second.filePath;
    }
};

void GeneratedFiles::remove(const FilePaths &filePaths)
{
    V2::FileContainers differenceFileContainers;
    differenceFileContainers.reserve(m_fileContainers.size());

    std::set_difference(std::make_move_iterator(m_fileContainers.begin()),
                        std::make_move_iterator(m_fileContainers.end()),
                        filePaths.begin(),
                        filePaths.end(),
                        std::back_inserter(differenceFileContainers),
                        Compare{});

    m_fileContainers = std::move(differenceFileContainers);
}

const V2::FileContainers &GeneratedFiles::fileContainers() const
{
    return m_fileContainers;
}

} // namespace ClangBackEnd
