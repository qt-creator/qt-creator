// Copyright (C) 2016 Alexander Drozdov.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "projectnodes.h"

#include <utils/filepath.h>
#include <utils/mimeutils.h>

#include <QPromise>

#include <functional>
#include <vector>

namespace Core { class IVersionControl; }

namespace ProjectExplorer::TreeScanner {

class PROJECTEXPLORER_EXPORT Result final
{
public:
    std::vector<std::unique_ptr<FileNode>> allFiles;
    std::vector<std::unique_ptr<Node>> firstLevelNodes;

    Result() = default;
    ~Result() = default;

    Result(Result &&) = default;
    Result &operator=(Result &&) = default;

    Result(const Result &) = delete;
    Result &operator=(const Result &) = delete;
};

using Filter = std::function<bool(const Utils::MimeType &, const Utils::FilePath &)>;
using FileTypeFactory = std::function<ProjectExplorer::FileType(const Utils::MimeType &)>;

// Standard filters helpers
PROJECTEXPLORER_EXPORT bool isWellKnownBinary(const Utils::FilePath &fn);
PROJECTEXPLORER_EXPORT bool isMimeBinary(const Utils::MimeType &mimeType);
PROJECTEXPLORER_EXPORT bool isMimeTypeIgnored(const Utils::MimeType &mimeType); // employs internal cache

// Standard file factory
PROJECTEXPLORER_EXPORT ProjectExplorer::FileType genericFileType(const Utils::MimeType &mdb);

PROJECTEXPLORER_EXPORT void scanForFiles(QPromise<Result> &promise,
                                         const Utils::FilePath &directory,
                                         const Filter &filter,
                                         QDir::Filters dirFilter,
                                         const FileTypeFactory &factory);

} // namespace ProjectExplorer::TreeScanner
