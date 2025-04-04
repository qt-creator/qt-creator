// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "version-matcher.h"

#include <projectstorage/projectstorageinfotypes.h>

template<typename ModuleIdMatcher, typename TypeIdMatcher, typename NameMatcher, typename MajorVersionMatcher, typename MinorVersionMatcher>
auto IsInfoExportTypeName(const ModuleIdMatcher &moduleIdMatcher,
                          const TypeIdMatcher &typeIdMatcher,
                          const NameMatcher &nameMatcher,
                          const MajorVersionMatcher &majorVersionMatcher,
                          const MinorVersionMatcher &minorVersionMatcher)
{
    return AllOf(Field("QmlDesigner::Storage::Info::ExportedTypeName::moduleId",
                       &QmlDesigner::Storage::Info::ExportedTypeName::moduleId,
                       moduleIdMatcher),
                 Field("QmlDesigner::Storage::Info::ExportedTypeName::typeId",
                       &QmlDesigner::Storage::Info::ExportedTypeName::typeId,
                       typeIdMatcher),
                 Field("QmlDesigner::Storage::Info::ExportedTypeName::name",
                       &QmlDesigner::Storage::Info::ExportedTypeName::name,
                       nameMatcher),
                 Field("QmlDesigner::Storage::Info::ExportedTypeName::version",
                       &QmlDesigner::Storage::Info::ExportedTypeName::version,
                       IsVersion(majorVersionMatcher, minorVersionMatcher)));
}

template<typename ModuleIdMatcher, typename TypeIdMatcher, typename NameMatcher, typename VersionMatcher>
auto IsInfoExportTypeName(const ModuleIdMatcher &moduleIdMatcher,
                          const TypeIdMatcher &typeIdMatcher,
                          const NameMatcher &nameMatcher,
                          const VersionMatcher &versionMatcher)
{
    return AllOf(Field("QmlDesigner::Storage::Info::ExportedTypeName::moduleId",
                       &QmlDesigner::Storage::Info::ExportedTypeName::moduleId,
                       moduleIdMatcher),
                 Field("QmlDesigner::Storage::Info::ExportedTypeName::typeId",
                       &QmlDesigner::Storage::Info::ExportedTypeName::typeId,
                       typeIdMatcher),
                 Field("QmlDesigner::Storage::Info::ExportedTypeName::name",
                       &QmlDesigner::Storage::Info::ExportedTypeName::name,
                       nameMatcher),
                 Field("QmlDesigner::Storage::Info::ExportedTypeName::version",
                       &QmlDesigner::Storage::Info::ExportedTypeName::version,
                       versionMatcher));
}
