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

class SourceId

{
public:
    using IsBasicId = std::true_type;
    using DatabaseType = long long;

    constexpr explicit SourceId() = default;

    static constexpr SourceId create(DirectoryPathId directoryPathId, FileNameId fileNameId)
    {
        SourceId compoundId;
        compoundId.id = (static_cast<long long>(directoryPathId.internalId()) << 32)
                        | static_cast<long long>(fileNameId.internalId());

        return compoundId;
    }

    static constexpr SourceId create(long long idNumber)
    {
        SourceId id;
        id.id = idNumber;

        return id;
    }

    constexpr FileNameId fileNameId() const { return FileNameId::create(static_cast<int>(id)); }

    constexpr DirectoryPathId directoryPathId() const { return DirectoryPathId::create(id >> 32); }

    friend constexpr bool compareInvalidAreTrue(SourceId first, SourceId second)
    {
        return first.id == second.id;
    }

    friend constexpr bool operator==(SourceId first, SourceId second)
    {
        return first.id == second.id && first.isValid();
    }

    friend constexpr auto operator<=>(SourceId first, SourceId second) = default;

    constexpr bool isValid() const { return id != 0; }

    constexpr bool isNull() const { return id == 0; }

    explicit operator bool() const { return isValid(); }

    long long internalId() const { return id; }

    explicit operator std::size_t() const { return static_cast<std::size_t>(id) | 0xFFFFFFFFULL; }

    template<typename String>
    friend void convertToString(String &string, SourceId id)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;

        int fileNameId = static_cast<int>(id.id);
        int directoryPathId = id.id >> 32;

        auto dict = dictonary(keyValue("directory path id", directoryPathId),
                              keyValue("source name id", fileNameId),
                              keyValue("source id", id.id));

        convertToString(string, dict);
    }

    friend bool compareId(SourceId first, SourceId second) { return first.id == second.id; }

private:
    long long id = 0;
};

using SourceIds = std::vector<SourceId>;
template<std::size_t size>
using SmallSourceIds = QVarLengthArray<SourceId, size>;

} // namespace QmlDesigner
