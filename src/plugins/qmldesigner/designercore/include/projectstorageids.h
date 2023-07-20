// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <sqlite/sqliteids.h>

#include <utils/span.h>

namespace QmlDesigner {

enum class BasicIdType {
    Type,
    PropertyType,
    PropertyDeclaration,
    Source,
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

using TypeId = Sqlite::BasicId<BasicIdType::Type>;
using TypeIds = std::vector<TypeId>;

using PropertyDeclarationId = Sqlite::BasicId<BasicIdType::PropertyDeclaration>;
using PropertyDeclarationIds = std::vector<PropertyDeclarationId>;

using FunctionDeclarationId = Sqlite::BasicId<BasicIdType::FunctionDeclaration>;
using FunctionDeclarationIds = std::vector<FunctionDeclarationId>;

using SignalDeclarationId = Sqlite::BasicId<BasicIdType::SignalDeclaration>;
using SignalDeclarationIds = std::vector<SignalDeclarationId>;

using EnumerationDeclarationId = Sqlite::BasicId<BasicIdType::EnumerationDeclaration>;
using EnumerationDeclarationIds = std::vector<EnumerationDeclarationId>;

using SourceContextId = Sqlite::BasicId<BasicIdType::SourceContext, int>;
using SourceContextIds = std::vector<SourceContextId>;

using SourceId = Sqlite::BasicId<BasicIdType::Source, int>;
using SourceIds = std::vector<SourceId>;

using ModuleId = Sqlite::BasicId<BasicIdType::Module, int>;
using ModuleIds = std::vector<ModuleId>;
using ModuleIdSpan = Utils::span<ModuleId>;

using ProjectPartId = Sqlite::BasicId<BasicIdType::ProjectPartId>;
using ProjectPartIds = std::vector<ProjectPartId>;

using ImportId = Sqlite::BasicId<BasicIdType::Import>;
using ImportIds = std::vector<ImportId>;

using ImportedTypeNameId = Sqlite::BasicId<BasicIdType::ImportedTypeName>;
using ImportedTypeNameIds = std::vector<ImportedTypeNameId>;

using ExportedTypeNameId = Sqlite::BasicId<BasicIdType::ExportedTypeName>;
using ExportedTypeNameIds = std::vector<ExportedTypeNameId>;

using ModuleExportedImportId = Sqlite::BasicId<BasicIdType::ModuleExportedImport>;
using ModuleExportedImportIds = std::vector<ModuleExportedImportId>;

} // namespace QmlDesigner
