// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <sqlite/sqliteids.h>

#include <utils/span.h>

#include <QVarLengthArray>

namespace QmlDesigner {

enum class SourcePathIdType {
    FileName,
    DirectoryPath,

};

using DirectoryPathId = Sqlite::BasicId<SourcePathIdType::DirectoryPath, int>;
using DirectoryPathIds = std::vector<DirectoryPathId>;
template<std::size_t size>
using SmallDirectoryPathIds = QVarLengthArray<DirectoryPathId, size>;

using FileNameId = Sqlite::BasicId<SourcePathIdType::FileName, int>;
using FileNameIds = std::vector<FileNameId>;
template<std::size_t size>
using SmallFileNameIds = QVarLengthArray<FileNameId, size>;

using SourceId = Sqlite::CompoundBasicId<SourcePathIdType::FileName, SourcePathIdType::DirectoryPath>;
using SourceIds = std::vector<SourceId>;
template<std::size_t size>
using SmallSourceIds = QVarLengthArray<SourceId, size>;

} // namespace QmlDesigner
