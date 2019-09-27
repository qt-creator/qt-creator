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

#include <cpptools/cppprojectfile.h>
#include <cpptools/projectpart.h>

#include <utils/fileutils.h>

namespace ClangTools {
namespace Internal {

class FileInfo
{
public:
    FileInfo() = default;
    FileInfo(Utils::FilePath file,
             CppTools::ProjectFile::Kind kind,
             CppTools::ProjectPart::Ptr projectPart)
        : file(std::move(file))
        , kind(kind)
        , projectPart(projectPart)
    {}
    Utils::FilePath file;
    CppTools::ProjectFile::Kind kind;
    CppTools::ProjectPart::Ptr projectPart;
};
using FileInfos = std::vector<FileInfo>;

inline bool operator==(const FileInfo &lhs, const FileInfo &rhs) {
    return lhs.file == rhs.file;
}

class FileInfoSelection {
public:
    QSet<Utils::FilePath> dirs;
    QSet<Utils::FilePath> files;
};

class FileInfoProvider {
public:
    QString displayName;
    FileInfos fileInfos;
    FileInfoSelection selection;

    enum ExpandPolicy {
        All,
        Limited,
    } expandPolicy = All;

    using OnSelectionAccepted = std::function<void(const FileInfoSelection &selection)>;
    OnSelectionAccepted onSelectionAccepted;
};
using FileInfoProviders = std::vector<FileInfoProvider>;

} // namespace Internal
} // namespace ClangTools
