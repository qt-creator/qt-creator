// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectstoragemock.h"

#include <matchers/import-matcher.h>

#include <projectstorage/projectstorageinfotypes.h>

using QmlDesigner::ImportedTypeNameId;
using QmlDesigner::ImportId;
using QmlDesigner::ModuleId;
using QmlDesigner::PropertyDeclarationId;
using QmlDesigner::SourceId;
using QmlDesigner::TypeId;
using QmlDesigner::TypeIds;
using QmlDesigner::Storage::PropertyDeclarationTraits;

namespace Storage = QmlDesigner::Storage;

namespace {

template<typename BasicId>
void incrementBasicId(BasicId &id)
{
    id = BasicId::create(id.internalId() + 1);
}

void setupIsBasedOn(ProjectStorageMock &mock)
{
    auto call = [&](TypeId typeId, auto... ids) -> bool {
        return (mock.isBasedOn(typeId, ids) || ...);
    };
    ON_CALL(mock, isBasedOn(_, _, _)).WillByDefault(call);
    ON_CALL(mock, isBasedOn(_, _, _, _)).WillByDefault(call);
    ON_CALL(mock, isBasedOn(_, _, _, _, _)).WillByDefault(call);
    ON_CALL(mock, isBasedOn(_, _, _, _, _, _)).WillByDefault(call);
    ON_CALL(mock, isBasedOn(_, _, _, _, _, _, _)).WillByDefault(call);
    ON_CALL(mock, isBasedOn(_, _, _, _, _, _, _, _)).WillByDefault(call);
}

} // namespace

ModuleId ProjectStorageMock::createModule(Utils::SmallStringView moduleName)
{
    if (auto id = moduleId(moduleName)) {
        return id;
    }

    static ModuleId moduleId;
    incrementBasicId(moduleId);

    ON_CALL(*this, moduleId(Eq(moduleName))).WillByDefault(Return(moduleId));
    ON_CALL(*this, fetchModuleIdUnguarded(Eq(moduleName))).WillByDefault(Return(moduleId));

    return moduleId;
}

QmlDesigner::ImportedTypeNameId ProjectStorageMock::createImportedTypeNameId(
    SourceId sourceId, Utils::SmallStringView typeName, TypeId typeId)
{
    if (auto id = importedTypeNameId(sourceId, typeName)) {
        return id;
    }

    static ImportedTypeNameId importedTypeNameId;
    incrementBasicId(importedTypeNameId);

    ON_CALL(*this, importedTypeNameId(sourceId, Eq(typeName)))
        .WillByDefault(Return(importedTypeNameId));

    ON_CALL(*this, typeId(importedTypeNameId)).WillByDefault(Return(typeId));

    return importedTypeNameId;
}

QmlDesigner::ImportedTypeNameId ProjectStorageMock::createImportedTypeNameId(
    QmlDesigner::SourceId sourceId, Utils::SmallStringView typeName, QmlDesigner::ModuleId moduleId)
{
    return createImportedTypeNameId(sourceId,
                                    typeName,
                                    typeId(moduleId, typeName, QmlDesigner::Storage::Version{}));
}

QmlDesigner::ImportedTypeNameId ProjectStorageMock::createImportedTypeNameId(
    QmlDesigner::ImportId importId, Utils::SmallStringView typeName, QmlDesigner::TypeId typeId)
{
    if (auto id = importedTypeNameId(importId, typeName)) {
        return id;
    }

    static ImportedTypeNameId importedTypeNameId;
    incrementBasicId(importedTypeNameId);

    ON_CALL(*this, importedTypeNameId(importId, Eq(typeName)))
        .WillByDefault(Return(importedTypeNameId));

    ON_CALL(*this, typeId(importedTypeNameId)).WillByDefault(Return(typeId));

    return importedTypeNameId;
}

QmlDesigner::ImportId ProjectStorageMock::createImportId(QmlDesigner::ModuleId moduleId,
                                                         QmlDesigner::SourceId sourceId,
                                                         QmlDesigner::Storage::Version version)
{
    static ImportId importId;
    incrementBasicId(importId);

    ON_CALL(*this, importId(IsImport(moduleId, sourceId, version))).WillByDefault(Return(importId));

    return importId;
}

void ProjectStorageMock::addExportedTypeName(QmlDesigner::TypeId typeId,
                                             QmlDesigner::ModuleId moduleId,
                                             Utils::SmallStringView typeName)
{
    ON_CALL(*this, typeId(Eq(moduleId), Eq(typeName), _)).WillByDefault(Return(typeId));
    ON_CALL(*this, fetchTypeIdByModuleIdAndExportedName(Eq(moduleId), Eq(typeName)))
        .WillByDefault(Return(typeId));
    exportedTypeName[typeId].emplace_back(moduleId, typeName);
}

void ProjectStorageMock::removeExportedTypeName(QmlDesigner::TypeId typeId,
                                                QmlDesigner::ModuleId moduleId,
                                                Utils::SmallStringView typeName)
{
    ON_CALL(*this, typeId(Eq(moduleId), Eq(typeName), _)).WillByDefault(Return(TypeId{}));
    ON_CALL(*this, fetchTypeIdByModuleIdAndExportedName(Eq(moduleId), Eq(typeName)))
        .WillByDefault(Return(TypeId{}));
    exportedTypeName.erase(typeId);
}

PropertyDeclarationId ProjectStorageMock::createProperty(TypeId typeId,
                                                         Utils::SmallString name,
                                                         PropertyDeclarationTraits traits,
                                                         TypeId propertyTypeId)
{
    if (auto id = propertyDeclarationId(typeId, name)) {
        return id;
    }

    static PropertyDeclarationId propertyId;
    incrementBasicId(propertyId);

    ON_CALL(*this, propertyDeclarationId(Eq(typeId), Eq(name))).WillByDefault(Return(propertyId));
    ON_CALL(*this, propertyName(Eq(propertyId))).WillByDefault(Return(name));

    ON_CALL(*this, propertyDeclaration(Eq(propertyId)))
        .WillByDefault(Return(
            QmlDesigner::Storage::Info::PropertyDeclaration{typeId, name, traits, propertyTypeId}));

    auto ids = localPropertyDeclarationIds(typeId);
    ids.push_back(propertyId);
    ON_CALL(*this, propertyDeclarationIds(Eq(typeId))).WillByDefault(Return(ids));
    ON_CALL(*this, localPropertyDeclarationIds(Eq(typeId))).WillByDefault(Return(ids));

    return propertyId;
}

void ProjectStorageMock::removeProperty(QmlDesigner::TypeId typeId, Utils::SmallString name)
{
    auto propertyId = propertyDeclarationId(typeId, name);

    ON_CALL(*this, propertyDeclarationId(Eq(typeId), Eq(name)))
        .WillByDefault(Return(PropertyDeclarationId{}));
    ON_CALL(*this, propertyName(Eq(propertyId)))
        .WillByDefault(Return(std::optional<Utils::SmallString>{}));

    ON_CALL(*this, propertyDeclaration(Eq(propertyId)))
        .WillByDefault(Return(std::optional<QmlDesigner::Storage::Info::PropertyDeclaration>{}));

    ON_CALL(*this, propertyDeclarationIds(Eq(typeId)))
        .WillByDefault(Return(QVarLengthArray<PropertyDeclarationId, 128>{}));
    ON_CALL(*this, localPropertyDeclarationIds(Eq(typeId)))
        .WillByDefault(Return(QVarLengthArray<PropertyDeclarationId, 128>{}));
}

QmlDesigner::PropertyDeclarationId ProjectStorageMock::createProperty(
    QmlDesigner::TypeId typeId, Utils::SmallString name, QmlDesigner::TypeId propertyTypeId)
{
    return createProperty(typeId, name, {}, propertyTypeId);
}

void ProjectStorageMock::createSignal(QmlDesigner::TypeId typeId, Utils::SmallString name)
{
    auto signalNames = signalDeclarationNames(typeId);
    signalNames.push_back(name);
    ON_CALL(*this, signalDeclarationNames(Eq(typeId))).WillByDefault(Return(signalNames));
}

void ProjectStorageMock::createFunction(QmlDesigner::TypeId typeId, Utils::SmallString name)
{
    auto functionNames = functionDeclarationNames(typeId);
    functionNames.push_back(name);
    ON_CALL(*this, functionDeclarationNames(Eq(typeId))).WillByDefault(Return(functionNames));
}

void ProjectStorageMock::setPropertyEditorPathId(QmlDesigner::TypeId typeId,
                                                 QmlDesigner::SourceId sourceId)
{
    ON_CALL(*this, propertyEditorPathId(Eq(typeId))).WillByDefault(Return(sourceId));
}

namespace {
void addBaseProperties(TypeId typeId, TypeIds baseTypeIds, ProjectStorageMock &storage)
{
    for (TypeId baseTypeId : baseTypeIds) {
        for (const auto &propertyId : storage.localPropertyDeclarationIds(baseTypeId)) {
            auto data = storage.propertyDeclaration(propertyId);
            if (data) {
                storage.createProperty(typeId, data->name, data->traits, data->propertyTypeId);
            }
        }
    }
}

void setType(TypeId typeId, ModuleId moduleId, Utils::SmallStringView typeName, ProjectStorageMock &storage)
{
    ON_CALL(storage, typeId(Eq(moduleId), Eq(typeName), _)).WillByDefault(Return(typeId));
    ON_CALL(storage, fetchTypeIdByModuleIdAndExportedName(Eq(moduleId), Eq(typeName)))
        .WillByDefault(Return(typeId));
}

} // namespace

TypeId ProjectStorageMock::createType(ModuleId moduleId,
                                      Utils::SmallStringView typeName,
                                      Utils::SmallStringView defaultPropertyName,
                                      PropertyDeclarationTraits defaultPropertyTraits,
                                      TypeId defaultPropertyTypeId,
                                      Storage::TypeTraits typeTraits,
                                      TypeIds baseTypeIds,
                                      SourceId sourceId)
{
    if (auto id = typeId(moduleId, typeName)) {
        return id;
    }

    static TypeId typeId;
    incrementBasicId(typeId);

    setType(typeId, moduleId, typeName, *this);

    addBaseProperties(typeId, baseTypeIds, *this);

    addExportedTypeName(typeId, moduleId, typeName);

    PropertyDeclarationId defaultPropertyDeclarationId;
    if (defaultPropertyName.size()) {
        if (!defaultPropertyTypeId) {
            defaultPropertyTypeId = typeId;
        }

        defaultPropertyDeclarationId = createProperty(typeId,
                                                      defaultPropertyName,
                                                      defaultPropertyTraits,
                                                      defaultPropertyTypeId);
    }

    ON_CALL(*this, type(Eq(typeId)))
        .WillByDefault(Return(Storage::Info::Type{defaultPropertyDeclarationId, sourceId, typeTraits}));

    ON_CALL(*this, isBasedOn(Eq(typeId), Eq(typeId))).WillByDefault(Return(true));

    for (TypeId baseTypeId : baseTypeIds)
        ON_CALL(*this, isBasedOn(Eq(typeId), Eq(baseTypeId))).WillByDefault(Return(true));

    TypeIds selfAndPrototypes;
    selfAndPrototypes.reserve(baseTypeIds.size() + 1);
    selfAndPrototypes.push_back(typeId);
    selfAndPrototypes.insert(selfAndPrototypes.end(), baseTypeIds.begin(), baseTypeIds.end());
    ON_CALL(*this, prototypeAndSelfIds(Eq(typeId))).WillByDefault(Return(selfAndPrototypes));
    ON_CALL(*this, prototypeIds(Eq(typeId))).WillByDefault(Return(baseTypeIds));

    return typeId;
}

void ProjectStorageMock::removeType(QmlDesigner::ModuleId moduleId, Utils::SmallStringView typeName)
{
    auto oldTypeId = typeId(moduleId, typeName);

    setType(TypeId{}, moduleId, typeName, *this);

    removeExportedTypeName(oldTypeId, moduleId, typeName);

    ON_CALL(*this, type(Eq(oldTypeId))).WillByDefault(Return(std::optional<Storage::Info::Type>{}));
}

QmlDesigner::TypeId ProjectStorageMock::createType(QmlDesigner::ModuleId moduleId,
                                                   Utils::SmallStringView typeName,
                                                   QmlDesigner::Storage::TypeTraits typeTraits,
                                                   QmlDesigner::TypeIds baseTypeIds,
                                                   SourceId sourceId)
{
    return createType(moduleId, typeName, {}, {}, TypeId{}, typeTraits, baseTypeIds, sourceId);
}

TypeId ProjectStorageMock::createObject(ModuleId moduleId,
                                        Utils::SmallStringView typeName,
                                        Utils::SmallStringView defaultPropertyName,
                                        PropertyDeclarationTraits defaultPropertyTraits,
                                        QmlDesigner::TypeId defaultPropertyTypeId,
                                        TypeIds baseTypeIds,
                                        QmlDesigner::SourceId sourceId)
{
    return createType(moduleId,
                      typeName,
                      defaultPropertyName,
                      defaultPropertyTraits,
                      defaultPropertyTypeId,
                      Storage::TypeTraits::Reference,
                      baseTypeIds,
                      sourceId);
}

TypeId ProjectStorageMock::createObject(ModuleId moduleId,
                                        Utils::SmallStringView typeName,
                                        TypeIds baseTypeIds)
{
    return createType(moduleId, typeName, Storage::TypeTraits::Reference, baseTypeIds);
}

QmlDesigner::TypeId ProjectStorageMock::createValue(QmlDesigner::ModuleId moduleId,
                                                    Utils::SmallStringView typeName,
                                                    QmlDesigner::TypeIds baseTypeIds)
{
    return createType(moduleId, typeName, Storage::TypeTraits::Value, baseTypeIds);
}

ProjectStorageMock::ProjectStorageMock()
{
    ON_CALL(*this, exportedTypeNames(_)).WillByDefault([&](TypeId id) {
        return exportedTypeName[id];
    });
}

void ProjectStorageMock::setupQtQuick()
{
    setupIsBasedOn(*this);

    auto qmlModuleId = createModule("QML");
    auto qmlNativeModuleId = createModule("QML-cppnative");
    auto qtQmlModelsModuleId = createModule("QtQml.Models");
    auto qtQuickModuleId = createModule("QtQuick");
    auto qtQuickNativeModuleId = createModule("QtQuick-cppnative");

    auto boolId = createValue(qmlModuleId, "bool");
    auto intId = createValue(qmlModuleId, "int");
    createValue(qmlNativeModuleId, "uint");
    auto doubleId = createValue(qmlModuleId, "double");
    addExportedTypeName(doubleId, qmlModuleId, "real");
    createValue(qmlNativeModuleId, "float");
    createValue(qmlModuleId, "date");
    auto stringId = createValue(qmlModuleId, "string");
    createValue(qmlModuleId, "url");

    auto qtObjectId = createObject(qmlModuleId,
                                   "QtObject",
                                   "children",
                                   PropertyDeclarationTraits::IsList,
                                   TypeId{});

    auto listElementId = createObject(qtQmlModelsModuleId, "ListElement", {qtObjectId});

    createObject(qtQmlModelsModuleId,
                 "ListModel",
                 "children",
                 PropertyDeclarationTraits::IsList,
                 listElementId,
                 {qtObjectId});

    auto fontValueTypeId = createValue(qtQuickModuleId, "QQuickFontValueType");
    createProperty(fontValueTypeId, "family", stringId);
    createProperty(fontValueTypeId, "styleName", stringId);
    createProperty(fontValueTypeId, "underline", boolId);
    createProperty(fontValueTypeId, "overline", boolId);
    createProperty(fontValueTypeId, "pointSize", doubleId);
    createProperty(fontValueTypeId, "pixelSize", intId);
    auto fontId = createValue(qtQuickModuleId, "font", {fontValueTypeId});

    auto itemId = createObject(qtQuickModuleId,
                               "Item",
                               "data",
                               PropertyDeclarationTraits::IsList,
                               qtObjectId,
                               {qtObjectId});
    createProperty(itemId, "x", doubleId);
    createProperty(itemId, "font", fontId);

    auto inputDeviceId = createObject(qtQuickModuleId, "InputDevice", {qtObjectId});
    createProperty(inputDeviceId, "seatName", stringId);

    createObject(qtQuickModuleId,
                 "ListView",
                 "data",
                 PropertyDeclarationTraits::IsList,
                 qtObjectId,
                 {qtObjectId, itemId});
    createObject(qtQuickModuleId, "StateGroup", {qtObjectId});
    createObject(qtQuickModuleId,
                 "State",
                 "changes",
                 PropertyDeclarationTraits::IsList,
                 qtObjectId,
                 {qtObjectId});
    auto animationId = createObject(qtQuickModuleId, "Animation", {qtObjectId});
    createObject(qtQuickModuleId,
                 "Transition",
                 "animations",
                 PropertyDeclarationTraits::IsList,
                 animationId,
                 {qtObjectId});
    createObject(qtQuickModuleId, "PropertyAnimation", {qtObjectId});
    auto stateOperationsId = createObject(qtQuickNativeModuleId,
                                          " QQuickStateOperation",
                                          {qtObjectId});
    createObject(qtQuickModuleId, "PropertyChanges", {qtObjectId, stateOperationsId});

    auto qtQuickTimelineModuleId = createModule("QtQuick.Timeline");
    createObject(qtQuickTimelineModuleId, "KeyframeGroup", {qtObjectId});
    createObject(qtQuickTimelineModuleId, "Keyframe", {qtObjectId});

    auto flowViewModuleId = createModule("FlowView");
    createObject(flowViewModuleId,
                 "FlowActionArea",
                 "data",
                 PropertyDeclarationTraits::IsList,
                 qtObjectId,
                 {itemId, qtObjectId});
    createObject(flowViewModuleId,
                 "FlowWildcard",
                 "data",
                 PropertyDeclarationTraits::IsList,
                 qtObjectId,
                 {qtObjectId});
    createObject(flowViewModuleId, "FlowDecision", {qtObjectId});
    createObject(flowViewModuleId, "FlowTransition", {qtObjectId});
    createObject(flowViewModuleId,
                 "FlowItem",
                 "data",
                 PropertyDeclarationTraits::IsList,
                 qtObjectId,
                 {qtObjectId, itemId});
}

void ProjectStorageMock::setupQtQuickImportedTypeNameIds(QmlDesigner::SourceId sourceId)
{
    auto qmlModuleId = moduleId("QML");
    auto qtQmlModelsModuleId = moduleId("QtQml.Models");
    auto qtQuickModuleId = moduleId("QtQuick");
    auto qtQuickNativeModuleId = moduleId("QtQuick-cppnative");
    auto qtQuickTimelineModuleId = moduleId("QtQuick.Timeline");
    auto flowViewModuleId = moduleId("FlowView");

    createImportedTypeNameId(sourceId, "int", qmlModuleId);
    createImportedTypeNameId(sourceId, "QtObject", qmlModuleId);
    createImportedTypeNameId(sourceId, "ListElement", qtQmlModelsModuleId);
    createImportedTypeNameId(sourceId, "ListModel", qtQmlModelsModuleId);
    createImportedTypeNameId(sourceId, "Item", qtQuickModuleId);
    createImportedTypeNameId(sourceId, "ListView", qtQuickModuleId);
    createImportedTypeNameId(sourceId, "StateGroup", qtQuickModuleId);
    createImportedTypeNameId(sourceId, "State", qtQuickModuleId);
    createImportedTypeNameId(sourceId, "Animation", qtQuickModuleId);
    createImportedTypeNameId(sourceId, "Transition", qtQuickModuleId);
    createImportedTypeNameId(sourceId, "PropertyAnimation", qtQuickModuleId);
    createImportedTypeNameId(sourceId, "QQuickStateOperation", qtQuickNativeModuleId);
    createImportedTypeNameId(sourceId, "PropertyChanges", qtQuickModuleId);
    createImportedTypeNameId(sourceId, "KeyframeGroup", qtQuickTimelineModuleId);
    createImportedTypeNameId(sourceId, "Keyframe", qtQuickTimelineModuleId);
    createImportedTypeNameId(sourceId, "FlowActionArea", flowViewModuleId);
    createImportedTypeNameId(sourceId, "FlowWildcard", flowViewModuleId);
    createImportedTypeNameId(sourceId, "FlowDecision", flowViewModuleId);
    createImportedTypeNameId(sourceId, "FlowTransition", flowViewModuleId);
    createImportedTypeNameId(sourceId, "FlowItem", flowViewModuleId);
}

void ProjectStorageMock::setupCommonTypeCache()
{
    ON_CALL(*this, commonTypeCache()).WillByDefault(ReturnRef(typeCache));
}
