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

TypeId createType(ProjectStorageMock &mock,
                  ModuleId moduleId,
                  Utils::SmallStringView typeName,
                  Utils::SmallString defaultPropertyName,
                  Storage::TypeTraits typeTraits,
                  TypeId baseTypeId = TypeId{})
{
    static TypeId typeId;
    incrementBasicId(typeId);

    static PropertyDeclarationId defaultPropertyId;
    incrementBasicId(defaultPropertyId);

    ON_CALL(mock, typeId(Eq(moduleId), Eq(typeName), _)).WillByDefault(Return(typeId));
    ON_CALL(mock, type(Eq(typeId)))
        .WillByDefault(Return(Storage::Info::Type{defaultPropertyId, typeTraits}));
    ON_CALL(mock, propertyName(Eq(defaultPropertyId))).WillByDefault(Return(defaultPropertyName));

    if (baseTypeId)
        ON_CALL(mock, isBasedOn(Eq(typeId), Eq(baseTypeId))).WillByDefault(Return(true));

    return typeId;
}

TypeId createObject(ProjectStorageMock &mock,
                    ModuleId moduleId,
                    Utils::SmallStringView typeName,
                    Utils::SmallString defaultPropertyName,
                    TypeId baseTypeId = TypeId{})
{
    return createType(
        mock, moduleId, typeName, defaultPropertyName, Storage::TypeTraits::Reference, baseTypeId);
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

    auto qtObjectId = QmlDesigner::createObject(*this, qmlModuleId, "QtObject", "children");

    QmlDesigner::createObject(*this, qtQmlModelsModuleId, "ListModel", "children", qtObjectId);
    QmlDesigner::createObject(*this, qtQmlModelsModuleId, "ListElement", "children", qtObjectId);

    auto itemId = QmlDesigner::createObject(*this, qtQuickModuleId, "Item", "data", qtObjectId);
    QmlDesigner::createObject(*this, qtQuickModuleId, "ListView", "data", itemId);
}
