// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <projectstorage/projectstoragetypes.h>

MATCHER_P2(IsTypeHint,
           name,
           expression,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(QmlDesigner::Storage::Info::TypeHint{name, expression}))
{
    const QmlDesigner::Storage::Info::TypeHint &typeHint = arg;

    return typeHint.name == name && typeHint.expression == expression;
}

template<typename PropertiesMatcher, typename ExtraFilePathsMatcher>
auto IsItemLibraryEntry(QmlDesigner::TypeId typeId,
                        Utils::SmallStringView typeName,
                        Utils::SmallStringView name,
                        Utils::SmallStringView iconPath,
                        Utils::SmallStringView category,
                        Utils::SmallStringView import,
                        QmlDesigner::Storage::ModuleKind moduleKind,
                        Utils::SmallStringView toolTip,
                        Utils::SmallStringView templatePath,
                        PropertiesMatcher propertiesMatcher,
                        ExtraFilePathsMatcher extraFilePathsMatcher)
{
    using QmlDesigner::Storage::Info::ItemLibraryEntry;
    return AllOf(Field("typeId", &ItemLibraryEntry::typeId, typeId),
                 Field("typeName", &ItemLibraryEntry::typeName, typeName),
                 Field("name", &ItemLibraryEntry::name, name),
                 Field("iconPath", &ItemLibraryEntry::iconPath, iconPath),
                 Field("category", &ItemLibraryEntry::category, category),
                 Field("import", &ItemLibraryEntry::import, import),
                 Field("moduleKind", &ItemLibraryEntry::moduleKind, moduleKind),
                 Field("toolTip", &ItemLibraryEntry::toolTip, toolTip),
                 Field("templatePath", &ItemLibraryEntry::templatePath, templatePath),
                 Field("properties", &ItemLibraryEntry::properties, propertiesMatcher),
                 Field("extraFilePaths", &ItemLibraryEntry::extraFilePaths, extraFilePathsMatcher));
}

inline auto IsItemLibraryEntry(QmlDesigner::TypeId typeId,
                               Utils::SmallStringView typeName,
                               Utils::SmallStringView name,
                               Utils::SmallStringView category,
                               Utils::SmallStringView import,
                               QmlDesigner::Storage::ModuleKind moduleKind,
                               QmlDesigner::SourceId componentSourceId)
{
    using QmlDesigner::Storage::Info::ItemLibraryEntry;
    return AllOf(Field("typeId", &ItemLibraryEntry::typeId, typeId),
                 Field("typeName", &ItemLibraryEntry::typeName, typeName),
                 Field("name", &ItemLibraryEntry::name, name),
                 Field("category", &ItemLibraryEntry::category, category),
                 Field("import", &ItemLibraryEntry::import, import),
                 Field("moduleKind", &ItemLibraryEntry::moduleKind, moduleKind),
                 Field("componentSourceId", &ItemLibraryEntry::componentSourceId, componentSourceId));
}

MATCHER_P3(IsItemLibraryProperty,
           name,
           type,
           value,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(QmlDesigner::Storage::Info::ItemLibraryProperty(
                   name, type, Sqlite::ValueView::create(value))))
{
    const QmlDesigner::Storage::Info::ItemLibraryProperty &property = arg;

    return property.name == name && property.type == type && property.value == value;
}

template<typename IconPathMatcher, typename TypeTraitsMatcher, typename HintsJsonMatcher, typename ItemLibraryJsonMatcher>
auto IsTypeAnnotation(QmlDesigner::SourceId sourceId,
                      QmlDesigner::SourceContextId directoryId,
                      Utils::SmallStringView typeName,
                      QmlDesigner::ModuleId moduleId,
                      IconPathMatcher iconPath,
                      TypeTraitsMatcher traits,
                      HintsJsonMatcher hintsJsonMatcher,
                      ItemLibraryJsonMatcher itemLibraryJsonMatcher)
{
    using QmlDesigner::Storage::Synchronization::TypeAnnotation;
    return AllOf(Field("sourceId", &TypeAnnotation::sourceId, sourceId),
                 Field("directory sourceId", &TypeAnnotation::directoryId, directoryId),
                 Field("typeName", &TypeAnnotation::typeName, typeName),
                 Field("moduleId", &TypeAnnotation::moduleId, moduleId),
                 Field("iconPath", &TypeAnnotation::iconPath, iconPath),
                 Field("traits", &TypeAnnotation::traits, traits),
                 Field("hintsJson", &TypeAnnotation::hintsJson, hintsJsonMatcher),
                 Field("itemLibraryJson", &TypeAnnotation::itemLibraryJson, itemLibraryJsonMatcher));
}
