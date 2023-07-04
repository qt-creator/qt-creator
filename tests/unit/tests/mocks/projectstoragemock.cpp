// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectstoragemock.h"

#include <projectstorage/projectstorageinfotypes.h>

using QmlDesigner::ModuleId;
using QmlDesigner::PropertyDeclarationId;
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
    static ModuleId moduleId;
    incrementBasicId(moduleId);

    ON_CALL(*this, moduleId(Eq(moduleName))).WillByDefault(Return(moduleId));

    return moduleId;
}

PropertyDeclarationId ProjectStorageMock::createProperty(TypeId typeId,
                                                         Utils::SmallString name,
                                                         PropertyDeclarationTraits traits,
                                                         TypeId propertyTypeId)
{
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

TypeId ProjectStorageMock::createType(ModuleId moduleId,
                                      Utils::SmallStringView typeName,
                                      Utils::SmallStringView defaultPropertyName,
                                      PropertyDeclarationTraits defaultPropertyTraits,
                                      TypeId defaultPropertyTypeId,
                                      Storage::TypeTraits typeTraits,
                                      TypeIds baseTypeIds)
{
    static TypeId typeId;
    incrementBasicId(typeId);

    ON_CALL(*this, typeId(Eq(moduleId), Eq(typeName), _)).WillByDefault(Return(typeId));
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
        .WillByDefault(Return(Storage::Info::Type{defaultPropertyDeclarationId, typeTraits}));

    ON_CALL(*this, isBasedOn(Eq(typeId), Eq(typeId))).WillByDefault(Return(true));

    for (TypeId baseTypeId : baseTypeIds)
        ON_CALL(*this, isBasedOn(Eq(typeId), Eq(baseTypeId))).WillByDefault(Return(true));

    return typeId;
}

QmlDesigner::TypeId ProjectStorageMock::createType(QmlDesigner::ModuleId moduleId,
                                                   Utils::SmallStringView typeName,
                                                   QmlDesigner::Storage::TypeTraits typeTraits,
                                                   QmlDesigner::TypeIds baseTypeIds)
{
    return createType(moduleId, typeName, {}, {}, TypeId{}, typeTraits, baseTypeIds);
}

TypeId ProjectStorageMock::createObject(ModuleId moduleId,
                                        Utils::SmallStringView typeName,
                                        Utils::SmallStringView defaultPropertyName,
                                        PropertyDeclarationTraits defaultPropertyTraits,
                                        QmlDesigner::TypeId defaultPropertyTypeId,
                                        TypeIds baseTypeIds)
{
    return createType(moduleId,
                      typeName,
                      defaultPropertyName,
                      defaultPropertyTraits,
                      defaultPropertyTypeId,
                      Storage::TypeTraits::Reference,
                      baseTypeIds);
}

TypeId ProjectStorageMock::createObject(ModuleId moduleId,
                                        Utils::SmallStringView typeName,
                                        TypeIds baseTypeIds)
{
    return createType(moduleId, typeName, Storage::TypeTraits::Reference, baseTypeIds);
}

void ProjectStorageMock::setupQtQtuick()
{
    setupIsBasedOn(*this);

    auto qmlModuleId = createModule("QML");
    auto qtQmlModelsModuleId = createModule("QtQml.Models");
    auto qtQuickModuleId = createModule("QtQuick");
    auto qtQuickNativeModuleId = createModule("QtQuick-cppnative");

    createType(qmlModuleId, "int", Storage::TypeTraits::Value);

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

    auto itemId = createObject(qtQuickModuleId,
                               "Item",
                               "data",
                               PropertyDeclarationTraits::IsList,
                               qtObjectId,
                               {qtObjectId});
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
                 {qtObjectId, itemId});
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

void ProjectStorageMock::setupCommonTypeCache()
{
    ON_CALL(*this, commonTypeCache()).WillByDefault(ReturnRef(typeCache));
}
