// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sourcepathids.h"

namespace QmlDesigner {

enum class ProjectStorageIdType {
    Type,
    PropertyType,
    PropertyDeclaration,
    SourceName,
    SourceContext,
    StorageCacheIndex,
    FunctionDeclaration,
    SignalDeclaration,
    EnumerationDeclaration,
    Module,
    ProjectPartId,
    Import,
    ImportedTypeName,
    ExportedTypeName,
    ModuleExportedImport
};

using TypeId = Sqlite::BasicId<ProjectStorageIdType::Type>;
using TypeIds = std::vector<TypeId>;
template<std::size_t size>
using SmallTypeIds = QVarLengthArray<TypeId, size>;

using PropertyDeclarationId = Sqlite::BasicId<ProjectStorageIdType::PropertyDeclaration>;
using PropertyDeclarationIds = std::vector<PropertyDeclarationId>;

using FunctionDeclarationId = Sqlite::BasicId<ProjectStorageIdType::FunctionDeclaration>;
using FunctionDeclarationIds = std::vector<FunctionDeclarationId>;

using SignalDeclarationId = Sqlite::BasicId<ProjectStorageIdType::SignalDeclaration>;
using SignalDeclarationIds = std::vector<SignalDeclarationId>;

using EnumerationDeclarationId = Sqlite::BasicId<ProjectStorageIdType::EnumerationDeclaration>;
using EnumerationDeclarationIds = std::vector<EnumerationDeclarationId>;

using ModuleId = Sqlite::BasicId<ProjectStorageIdType::Module, int>;
using ModuleIds = std::vector<ModuleId>;
using ModuleIdSpan = Utils::span<ModuleId>;

using ProjectPartId = Sqlite::BasicId<ProjectStorageIdType::ProjectPartId>;
using ProjectPartIds = std::vector<ProjectPartId>;

using ImportId = Sqlite::BasicId<ProjectStorageIdType::Import>;
using ImportIds = std::vector<ImportId>;

using ImportedTypeNameId = Sqlite::BasicId<ProjectStorageIdType::ImportedTypeName>;
using ImportedTypeNameIds = std::vector<ImportedTypeNameId>;

using ExportedTypeNameId = Sqlite::BasicId<ProjectStorageIdType::ExportedTypeName>;
using ExportedTypeNameIds = std::vector<ExportedTypeNameId>;

using ModuleExportedImportId = Sqlite::BasicId<ProjectStorageIdType::ModuleExportedImport>;
using ModuleExportedImportIds = std::vector<ModuleExportedImportId>;

} // namespace QmlDesigner
