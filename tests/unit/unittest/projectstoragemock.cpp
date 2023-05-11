// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectstoragemock.h"

namespace QmlDesigner {
namespace {

template<typename BasicId>
void incrementBasicId(BasicId &id)
{
    id = BasicId::create(id.internalId() + 1);
}

ModuleId createModule(ProjectStorageMock &mock, Utils::SmallStringView moduleName)
{
    static ModuleId moduleId;
    incrementBasicId(moduleId);

    ON_CALL(mock, moduleId(Eq(moduleName))).WillByDefault(Return(moduleId));

    return moduleId;
}

PropertyDeclarationId createProperty(ProjectStorageMock &mock, TypeId typeId, Utils::SmallString name)
{
    static PropertyDeclarationId propertyId;
    incrementBasicId(propertyId);

    ON_CALL(mock, propertyDeclarationId(Eq(typeId), Eq(name))).WillByDefault(Return(propertyId));
    ON_CALL(mock, propertyName(Eq(propertyId))).WillByDefault(Return(name));

    return propertyId;
}

TypeId createType(ProjectStorageMock &mock,
                  ModuleId moduleId,
                  Utils::SmallStringView typeName,
                  Utils::SmallString defaultPropertyName,
                  Storage::TypeTraits typeTraits,
                  TypeIds baseTypeIds = {})
{
    static TypeId typeId;
    incrementBasicId(typeId);

    ON_CALL(mock, typeId(Eq(moduleId), Eq(typeName), _)).WillByDefault(Return(typeId));
    PropertyDeclarationId defaultPropertyDeclarationId;
    if (defaultPropertyName.size())
        defaultPropertyDeclarationId = createProperty(mock, typeId, defaultPropertyName);
    ON_CALL(mock, type(Eq(typeId)))
        .WillByDefault(Return(Storage::Info::Type{defaultPropertyDeclarationId, typeTraits}));

    ON_CALL(mock, isBasedOn(Eq(typeId), Eq(typeId))).WillByDefault(Return(true));

    for (TypeId baseTypeId : baseTypeIds)
        ON_CALL(mock, isBasedOn(Eq(typeId), Eq(baseTypeId))).WillByDefault(Return(true));

    return typeId;
}

TypeId createObject(ProjectStorageMock &mock,
                    ModuleId moduleId,
                    Utils::SmallStringView typeName,
                    Utils::SmallString defaultPropertyName,
                    TypeIds baseTypeIds = {})
{
    return createType(
        mock, moduleId, typeName, defaultPropertyName, Storage::TypeTraits::Reference, baseTypeIds);
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
} // namespace QmlDesigner

void ProjectStorageMock::setupQtQtuick()
{
    QmlDesigner::setupIsBasedOn(*this);

    auto qmlModuleId = QmlDesigner::createModule(*this, "QML");
    auto qtQmlModelsModuleId = QmlDesigner::createModule(*this, "QtQml.Models");
    auto qtQuickModuleId = QmlDesigner::createModule(*this, "QtQuick");
    auto qtQuickNativeModuleId = QmlDesigner::createModule(*this, "QtQuick-cppnative");

    auto qtObjectId = QmlDesigner::createObject(*this, qmlModuleId, "QtObject", "children");

    QmlDesigner::createObject(*this, qtQmlModelsModuleId, "ListModel", "children", {qtObjectId});
    QmlDesigner::createObject(*this, qtQmlModelsModuleId, "ListElement", "children", {qtObjectId});

    auto itemId = QmlDesigner::createObject(*this, qtQuickModuleId, "Item", "data", {qtObjectId});
    QmlDesigner::createObject(*this, qtQuickModuleId, "ListView", "data", {qtObjectId, itemId});
    QmlDesigner::createObject(*this, qtQuickModuleId, "StateGroup", "states", {qtObjectId});
    QmlDesigner::createObject(*this, qtQuickModuleId, "State", "changes", {qtObjectId});
    QmlDesigner::createObject(*this, qtQuickModuleId, "Transition", "animations", {qtObjectId});
    QmlDesigner::createObject(*this, qtQuickModuleId, "PropertyAnimation", "", {qtObjectId});
    auto stateOperationsId = QmlDesigner::createObject(*this,
                                                       qtQuickNativeModuleId,
                                                       " QQuickStateOperation",
                                                       "",
                                                       {qtObjectId});
    QmlDesigner::createObject(*this,
                              qtQuickModuleId,
                              "PropertyChanges",
                              "",
                              {qtObjectId, stateOperationsId});

    auto qtQuickTimelineModuleId = QmlDesigner::createModule(*this, "QtQuick.Timeline");
    QmlDesigner::createObject(*this, qtQuickTimelineModuleId, "KeyframeGroup", "keyframes", {qtObjectId});
    QmlDesigner::createObject(*this, qtQuickTimelineModuleId, "Keyframe", "", {qtObjectId});

    auto flowViewModuleId = QmlDesigner::createModule(*this, "FlowView");
    QmlDesigner::createObject(*this, flowViewModuleId, "FlowActionArea", "data", {qtObjectId, itemId});
    QmlDesigner::createObject(*this, flowViewModuleId, "FlowWildcard", "data", {qtObjectId});
    QmlDesigner::createObject(*this, flowViewModuleId, "FlowDecision", "", {qtObjectId});
    QmlDesigner::createObject(*this, flowViewModuleId, "FlowTransition", "", {qtObjectId});
}

void ProjectStorageMock::setupCommonTypeCache()
{
    ON_CALL(*this, commonTypeCache()).WillByDefault(ReturnRef(typeCache));
}
