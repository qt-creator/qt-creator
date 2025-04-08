// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"

#include <tuple>
#include <vector>

namespace QmlDesigner {

enum class SourceType : int { Qml, QmlUi, QmlTypes, QmlDir, Directory };

template<typename String>
void convertToString(String &string, SourceType sourceType)
{
    switch (sourceType) {
    case SourceType::Qml:
        convertToString(string, "Qml");
        break;
    case SourceType::QmlUi:
        convertToString(string, "QmlUi");
        break;
    case SourceType::QmlTypes:
        convertToString(string, "QmlTypes");
        break;
    case SourceType::QmlDir:
        convertToString(string, "QmlDir");
        break;
    case SourceType::Directory:
        convertToString(string, "Directory");
        break;
    }
}

class ProjectChunkId
{
public:
    ProjectPartId id;
    SourceType sourceType;

    auto operator==(const ProjectChunkId &other) const
    {
        return std::tie(id, sourceType) == std::tie(other.id, other.sourceType);
    }

    auto operator<=>(const ProjectChunkId &other) const
    {
        return std::tie(id, sourceType) <=> std::tie(other.id, other.sourceType);
    }

    template<typename String>
    friend void convertToString(String &string, const ProjectChunkId &id)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("project part id", id.id),
                              keyValue("source type", id.sourceType));

        convertToString(string, dict);
    }
};

using ProjectChunkIds = std::vector<ProjectChunkId>;

class IdPaths
{
public:
    IdPaths(ProjectPartId projectPartId, SourceType sourceType, SourceIds &&sourceIds)
        : id{projectPartId, sourceType}
        , sourceIds(std::move(sourceIds))
    {}
    IdPaths(ProjectChunkId projectChunkId, SourceIds &&sourceIds)
        : id(projectChunkId)
        , sourceIds(std::move(sourceIds))
    {}

    friend bool operator==(IdPaths first, IdPaths second)
    {
        return first.id == second.id && first.sourceIds == second.sourceIds;
    }

    template<typename String>
    friend void convertToString(String &string, const IdPaths &idPaths)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("id", idPaths.id), keyValue("source ids", idPaths.sourceIds));

        convertToString(string, dict);
    }

public:
    ProjectChunkId id;
    SourceIds sourceIds;
};

class WatcherEntry
{
public:
    ProjectChunkId id;
    DirectoryPathId directoryPathId;
    SourceId sourceId;
    long long lastModified = -1;
    long long size = -1;

    friend bool operator==(WatcherEntry first, WatcherEntry second)
    {
        return first.id == second.id && first.directoryPathId == second.directoryPathId
               && first.sourceId == second.sourceId;
    }

    friend std::weak_ordering operator<=>(const WatcherEntry &first, const WatcherEntry &second)
    {
        return std::tie(first.directoryPathId, first.sourceId, first.id)
               <=> std::tie(second.directoryPathId, second.sourceId, second.id);
    }

    operator SourceId() const { return sourceId; }

    operator DirectoryPathId() const { return directoryPathId; }
};

using WatcherEntries = std::vector<WatcherEntry>;

} // namespace QmlDesigner
