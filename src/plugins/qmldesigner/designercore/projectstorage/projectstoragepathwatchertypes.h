/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "projectstorageids.h"

#include <vector>

namespace QmlDesigner {

enum class SourceType : int { Qml, QmlUi, QmlTypes, QmlDir };

class ProjectChunkId
{
public:
    ProjectPartId id;
    SourceType sourceType;

    friend bool operator==(ProjectChunkId first, ProjectChunkId second)
    {
        return first.id == second.id && first.sourceType == second.sourceType;
    }

    friend bool operator==(ProjectChunkId first, ProjectPartId second)
    {
        return first.id == second;
    }

    friend bool operator==(ProjectPartId first, ProjectChunkId second)
    {
        return first == second.id;
    }

    friend bool operator!=(ProjectChunkId first, ProjectChunkId second)
    {
        return !(first == second);
    }

    friend bool operator<(ProjectChunkId first, ProjectChunkId second)
    {
        return std::tie(first.id, first.sourceType) < std::tie(second.id, second.sourceType);
    }

    friend bool operator<(ProjectChunkId first, ProjectPartId second) { return first.id < second; }

    friend bool operator<(ProjectPartId first, ProjectChunkId second) { return first < second.id; }
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

public:
    ProjectChunkId id;
    SourceIds sourceIds;
};

class WatcherEntry
{
public:
    ProjectChunkId id;
    SourceContextId sourceContextId;
    SourceId sourceId;
    long long lastModified = -1;

    friend bool operator==(WatcherEntry first, WatcherEntry second)
    {
        return first.id == second.id && first.sourceContextId == second.sourceContextId
               && first.sourceId == second.sourceId;
    }

    friend bool operator<(WatcherEntry first, WatcherEntry second)
    {
        return std::tie(first.sourceContextId, first.sourceId, first.id)
               < std::tie(second.sourceContextId, second.sourceId, second.id);
    }

    friend bool operator<(SourceContextId sourceContextId, WatcherEntry entry)
    {
        return sourceContextId < entry.sourceContextId;
    }

    friend bool operator<(WatcherEntry entry, SourceContextId sourceContextId)
    {
        return entry.sourceContextId < sourceContextId;
    }

    operator SourceId() const { return sourceId; }

    operator SourceContextId() const { return sourceContextId; }
};

using WatcherEntries = std::vector<WatcherEntry>;

} // namespace QmlDesigner
