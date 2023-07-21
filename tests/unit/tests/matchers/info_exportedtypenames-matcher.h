// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "version-matcher.h"

#include <projectstorage/projectstorageinfotypes.h>

template<typename ModuleIdMatcher,
         typename NameMatcher,
         typename MajorVersionMatcher,
         typename MinorVersionMatcher>
auto IsInfoExportTypeNames(const ModuleIdMatcher &moduleIdMatcher,
                           const NameMatcher &nameMatcher,
                           const MajorVersionMatcher &majorVersionMatcher,
                           const MinorVersionMatcher &minorVersionMatcher)
{
    return AllOf(Field(&QmlDesigner::Storage::Info::ExportedTypeName::moduleId, moduleIdMatcher),
                 Field(&QmlDesigner::Storage::Info::ExportedTypeName::name, nameMatcher),
                 Field(&QmlDesigner::Storage::Info::ExportedTypeName::version,
                       IsVersion(majorVersionMatcher, minorVersionMatcher)));
}

template<typename ModuleIdMatcher, typename NameMatcher, typename VersionMatcher>
auto IsInfoExportTypeNames(const ModuleIdMatcher &moduleIdMatcher,
                           const NameMatcher &nameMatcher,
                           const VersionMatcher &versionMatcher)
{
    return AllOf(Field(&QmlDesigner::Storage::Info::ExportedTypeName::moduleId, moduleIdMatcher),
                 Field(&QmlDesigner::Storage::Info::ExportedTypeName::name, nameMatcher),
                 Field(&QmlDesigner::Storage::Info::ExportedTypeName::version, versionMatcher));
}
