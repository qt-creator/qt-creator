// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "version-matcher.h"

#include <projectstorage/projectstoragetypes.h>

template<typename ModuleIdMatcher,
         typename SourceIdMatcher,
         typename MajorVersionMatcher,
         typename MinorVersionMatcher>
auto IsImport(const ModuleIdMatcher &moduleIdMatcher,
              const SourceIdMatcher &sourceIdMatcher,
              const MajorVersionMatcher &majorVersionMatcher,
              const MinorVersionMatcher &minorVersionMatcher)
{
    return AllOf(Field(&QmlDesigner::Storage::Import::moduleId, moduleIdMatcher),
                 Field(&QmlDesigner::Storage::Import::sourceId, sourceIdMatcher),
                 Field(&QmlDesigner::Storage::Import::version,
                       IsVersion(majorVersionMatcher, minorVersionMatcher)));
}

template<typename ModuleIdMatcher, typename SourceIdMatcher, typename VersionMatcher>
auto IsImport(const ModuleIdMatcher &moduleIdMatcher,
              const SourceIdMatcher &sourceIdMatcher,
              const VersionMatcher &versionMatcher)
{
    return AllOf(Field(&QmlDesigner::Storage::Import::moduleId, moduleIdMatcher),
                 Field(&QmlDesigner::Storage::Import::sourceId, sourceIdMatcher),
                 Field(&QmlDesigner::Storage::Import::version, versionMatcher));
}
