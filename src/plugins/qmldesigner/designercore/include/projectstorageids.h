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

#include <sqliteids.h>

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
