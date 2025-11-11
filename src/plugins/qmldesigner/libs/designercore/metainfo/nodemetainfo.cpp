// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nodemetainfo.h"
#include "model.h"
#include "model/model_p.h"

#include <enumeration.h>
#include <projectstorage/projectstorage.h>
#include <propertyparser.h>
#include <rewriterview.h>

#include <QDebug>
#include <QDir>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include <languageutils/fakemetaobject.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsvalueowner.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

// remove that if the old code model is removed
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
QT_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
QT_WARNING_DISABLE_MSVC(4996)

namespace QmlDesigner {

using NanotraceHR::keyValue;
using Storage::ModuleKind;

using ModelTracing::category;

bool NodeMetaInfo::isValid() const
{
    return bool(m_typeId);
}

MetaInfoType NodeMetaInfo::type(SL sl) const
{
    if (isValid()) {
        NanotraceHR::Tracer tracer{"node meta info get type",
                                   category(),
                                   keyValue("type id", m_typeId),
                                   keyValue("caller location", sl)};
        auto kind = typeData().traits.kind;
        tracer.end(keyValue("type kind", kind));

        switch (kind) {
        case Storage::TypeTraitsKind::Reference:
            return MetaInfoType::Reference;
        case Storage::TypeTraitsKind::Value:
            return MetaInfoType::Value;
        case Storage::TypeTraitsKind::Sequence:
            return MetaInfoType::Sequence;
        case Storage::TypeTraitsKind::None:
            return MetaInfoType::None;
        }
    }

    return MetaInfoType::None;
}

bool NodeMetaInfo::isFileComponentInProject(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info is file component",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    const auto &traits = typeData().traits;
    auto isFileComponent = traits.isFileComponent and traits.isInsideProject;

    tracer.end(keyValue("is file component", isFileComponent));

    return isFileComponent;
}

bool NodeMetaInfo::isSingleton(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info is singleton",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto isSingleton = typeData().traits.isSingleton;

    tracer.end(keyValue("is singleton", isSingleton));

    return isSingleton;
}

bool NodeMetaInfo::isInsideProject(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info is inside project",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto isInsideProject = typeData().traits.isInsideProject;

    tracer.end(keyValue("is inside project", isInsideProject));

    return isInsideProject;
}

FlagIs NodeMetaInfo::canBeContainer(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info can be container",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto canBeContainer = typeData().traits.canBeContainer;

    tracer.end(keyValue("can be container", canBeContainer));

    return canBeContainer;
}

FlagIs NodeMetaInfo::forceClip(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info force clip",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto forceClip = typeData().traits.forceClip;

    tracer.end(keyValue("force clip", forceClip));

    return forceClip;
}

FlagIs NodeMetaInfo::doesLayoutChildren(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info does layout children",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto doesLayoutChildren = typeData().traits.doesLayoutChildren;

    tracer.end(keyValue("does layout children", doesLayoutChildren));

    return doesLayoutChildren;
}

FlagIs NodeMetaInfo::canBeDroppedInFormEditor(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info can be dropped in form editor",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto canBeDroppedInFormEditor = typeData().traits.canBeDroppedInFormEditor;

    tracer.end(keyValue("can be dropped in form editor", canBeDroppedInFormEditor));

    return canBeDroppedInFormEditor;
}

FlagIs NodeMetaInfo::canBeDroppedInNavigator(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info can be dropped in navigator",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto canBeDroppedInNavigator = typeData().traits.canBeDroppedInNavigator;

    tracer.end(keyValue("can be dropped in navigator", canBeDroppedInNavigator));

    return canBeDroppedInNavigator;
}

FlagIs NodeMetaInfo::canBeDroppedInView3D(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info can be dropped in view3d",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto canBeDroppedInView3D = typeData().traits.canBeDroppedInView3D;

    tracer.end(keyValue("can be dropped in view3d", canBeDroppedInView3D));

    return canBeDroppedInView3D;
}

FlagIs NodeMetaInfo::isMovable(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info is movable",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto isMovable = typeData().traits.isMovable;

    tracer.end(keyValue("is movable", isMovable));

    return isMovable;
}

FlagIs NodeMetaInfo::isResizable(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info is resizable",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto isResizable = typeData().traits.isResizable;

    tracer.end(keyValue("is resizable", isResizable));

    return isResizable;
}

FlagIs NodeMetaInfo::hasFormEditorItem(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info has form editor item",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto hasFormEditorItem = typeData().traits.hasFormEditorItem;

    tracer.end(keyValue("has form editor item", hasFormEditorItem));

    return hasFormEditorItem;
}

FlagIs NodeMetaInfo::isStackedContainer(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info is stacked container",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto isStackedContainer = typeData().traits.isStackedContainer;

    tracer.end(keyValue("is stacked container", isStackedContainer));

    return isStackedContainer;
}

FlagIs NodeMetaInfo::takesOverRenderingOfChildren(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info takes over rendering of children",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto takesOverRenderingOfChildren = typeData().traits.takesOverRenderingOfChildren;

    tracer.end(keyValue("takes over rendering of children", takesOverRenderingOfChildren));

    return takesOverRenderingOfChildren;
}

FlagIs NodeMetaInfo::visibleInNavigator(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info visible in navigator",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto visibleInNavigator = typeData().traits.visibleInNavigator;

    tracer.end(keyValue("visible in navigator", visibleInNavigator));

    return visibleInNavigator;
}

FlagIs NodeMetaInfo::hideInNavigator() const
{
    if (isValid())
        return typeData().traits.hideInNavigator;

    return FlagIs::False;
}

FlagIs NodeMetaInfo::visibleInLibrary(SL sl) const
{
    if (!isValid())
        return FlagIs::False;

    NanotraceHR::Tracer tracer{"node meta info visible in library",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto visibleInLibrary = typeData().traits.visibleInLibrary;

    tracer.end(keyValue("visible in library", visibleInLibrary));

    return visibleInLibrary;
}

namespace {

[[maybe_unused]] auto propertyId(const ProjectStorageType &projectStorage,
                                 TypeId typeId,
                                 Utils::SmallStringView propertyName)
{
    auto begin = propertyName.begin();
    const auto end = propertyName.end();

    auto found = std::find(begin, end, '.');
    auto propertyId = projectStorage.propertyDeclarationId(typeId, {begin, found});

    if (propertyId && found != end) {
        auto propertyData = projectStorage.propertyDeclaration(propertyId);
        if (auto propertyTypeId = propertyData->propertyTypeId) {
            begin = std::next(found);
            found = std::find(begin, end, '.');
            propertyId = projectStorage.propertyDeclarationId(propertyTypeId, {begin, found});

            if (propertyId && found != end) {
                begin = std::next(found);
                return projectStorage.propertyDeclarationId(propertyTypeId, {begin, end});
            }
        }
    }

    return propertyId;
}

} // namespace

bool NodeMetaInfo::hasProperty(Utils::SmallStringView propertyName, SL sl) const
{
    NanotraceHR::Tracer tracer{"node meta info has property",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("property name", propertyName),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    auto hasPropertyId = bool(propertyId(*m_projectStorage, m_typeId, propertyName));

    tracer.end(keyValue("has property", hasPropertyId));

    return hasPropertyId;
}

PropertyMetaInfos NodeMetaInfo::properties(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get properties",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return Utils::transform<PropertyMetaInfos>(m_projectStorage->propertyDeclarationIds(m_typeId),
                                               PropertyMetaInfo::bind(m_projectStorage));
}

PropertyMetaInfos NodeMetaInfo::localProperties(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get local properties",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return Utils::transform<PropertyMetaInfos>(m_projectStorage->localPropertyDeclarationIds(m_typeId),
                                               PropertyMetaInfo::bind(m_projectStorage));
}

PropertyMetaInfo NodeMetaInfo::property(PropertyNameView propertyName, SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get property",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("property name", propertyName),
                               keyValue("caller location", sl)};

    return {propertyId(*m_projectStorage, m_typeId, propertyName), m_projectStorage};
}

PropertyNameList NodeMetaInfo::signalNames(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get signal names",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return Utils::transform<PropertyNameList>(m_projectStorage->signalDeclarationNames(m_typeId),
                                              &Utils::SmallString::toQByteArray);
}

PropertyNameList NodeMetaInfo::slotNames(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get slot names",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return Utils::transform<PropertyNameList>(m_projectStorage->functionDeclarationNames(m_typeId),
                                              &Utils::SmallString::toQByteArray);
}

namespace {
template<Storage::Info::StaticString moduleName,
         Storage::Info::StaticString typeName,
         ModuleKind moduleKind = ModuleKind::QmlLibrary>
bool isBasedOnCommonType(NotNullPointer<const ProjectStorageType> projectStorage, TypeId typeId)
{
    if (!typeId)
        return false;

    auto base = projectStorage->commonTypeId<moduleName, typeName, moduleKind>();

    return bool(projectStorage->basedOn(typeId, base));
}
} // namespace

PropertyName NodeMetaInfo::defaultPropertyName(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get default property name",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;

    if (auto name = m_projectStorage->propertyName(defaultPropertyDeclarationId())) {
        tracer.end(keyValue("default property name", name));
        return name->toQByteArray();
    }

    if (typeData().traits.usesCustomParser
        || isBasedOnCommonType<QML, Component>(m_projectStorage, m_typeId)) {
        return "data";
    }

    return {};
}

PropertyMetaInfo NodeMetaInfo::defaultProperty(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get default property",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto id = defaultPropertyDeclarationId();

    tracer.end(keyValue("default property id", id));

    return PropertyMetaInfo(id, m_projectStorage);
}

bool NodeMetaInfo::hasDefaultProperty(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info has default property",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};
    auto hasDefaultProperty = bool(defaultPropertyDeclarationId());
    tracer.end(keyValue("has default property", hasDefaultProperty));

    return hasDefaultProperty;
}

std::vector<NodeMetaInfo> NodeMetaInfo::selfAndPrototypes([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get self and prototypes",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return Utils::transform<NodeMetaInfos>(m_projectStorage->prototypeAndSelfIds(m_typeId),
                                           NodeMetaInfo::bind(m_projectStorage));
}

NodeMetaInfos NodeMetaInfo::prototypes([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get prototypes",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};
    return Utils::transform<NodeMetaInfos>(m_projectStorage->prototypeIds(m_typeId),
                                           NodeMetaInfo::bind(m_projectStorage));
}

bool NodeMetaInfo::defaultPropertyIsComponent() const
{
    if (!isValid())
        return false;

    auto id = defaultPropertyDeclarationId();
    auto propertyDeclaration = m_projectStorage->propertyDeclaration(id);

    using namespace Storage::Info;
    return isBasedOnCommonType<QML, Component>(m_projectStorage, propertyDeclaration->typeId);
}

Storage::Info::ExportedTypeNames NodeMetaInfo::allExportedTypeNames(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get all exported type names",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return m_projectStorage->exportedTypeNames(m_typeId);
}

Storage::Info::ExportedTypeNames NodeMetaInfo::exportedTypeNamesForSourceId(SourceId sourceId) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get exported type names for source id",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("source id", sourceId)};

    return m_projectStorage->exportedTypeNames(m_typeId, sourceId);
}

Storage::Info::TypeHints NodeMetaInfo::typeHints(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get type hints",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto hints = m_projectStorage->typeHints(m_typeId);

    tracer.end(keyValue("type hints", hints));

    return hints;
}

Utils::PathString NodeMetaInfo::iconPath(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get icon path",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto iconPath = m_projectStorage->typeIconPath(m_typeId);

    tracer.end(keyValue("icon path", iconPath));

    return iconPath;
}

Storage::Info::ItemLibraryEntries NodeMetaInfo::itemLibrariesEntries(SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get item library entries",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto entries = m_projectStorage->itemLibraryEntries(m_typeId);

    tracer.end(keyValue("item library entries", entries));

    return entries;
}

SourceId NodeMetaInfo::sourceId(SL sl) const
{
    if (!isValid())
        return SourceId{};

    NanotraceHR::Tracer tracer{"node meta info get source id",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto id = typeData().sourceId;

    tracer.end(keyValue("source id", id));

    return id;
}

SourceId NodeMetaInfo::propertyEditorPathId(SL sl) const
{
    if (!isValid())
        return SourceId{};

    NanotraceHR::Tracer tracer{"node meta info get property editor path id",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    auto id = m_projectStorage->propertyEditorPathId(m_typeId);

    tracer.end(keyValue("property editor path id", id));

    return id;
}

const Storage::Info::Type &NodeMetaInfo::typeData() const
{
    if (!m_typeData)
        m_typeData = m_projectStorage->type(m_typeId);

    return *m_typeData;
}

PropertyDeclarationId NodeMetaInfo::defaultPropertyDeclarationId() const
{
    if (!m_defaultPropertyId)
        m_defaultPropertyId.emplace(m_projectStorage->defaultPropertyDeclarationId(m_typeId));

    return *m_defaultPropertyId;
}

bool NodeMetaInfo::isSuitableForMouseAreaFill(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is suitable for mouse area fill",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    auto itemId = m_projectStorage->commonTypeId<QtQuick, Item>();
    auto mouseAreaId = m_projectStorage->commonTypeId<QtQuick, MouseArea>();
    auto templatesControlId = m_projectStorage->commonTypeId<QtQuick_Templates, Control>();

    auto isSuitableForMouseAreaFill = bool(
        m_projectStorage->basedOn(m_typeId, itemId, mouseAreaId, templatesControlId));

    tracer.end(keyValue("is suitable for mouse area fill", isSuitableForMouseAreaFill));

    return isSuitableForMouseAreaFill;
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo, [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is based on 1 node meta info",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("meta info type id", metaInfo.m_typeId),
                               keyValue("caller location", sl)};

    return bool(m_projectStorage->basedOn(m_typeId, metaInfo.m_typeId));
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo1,
                             const NodeMetaInfo &metaInfo2,
                             [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is based on 2 node meta infos",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return bool(m_projectStorage->basedOn(m_typeId, metaInfo1.m_typeId, metaInfo2.m_typeId));
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo1,
                             const NodeMetaInfo &metaInfo2,
                             const NodeMetaInfo &metaInfo3,
                             [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is based on 3 node meta infos",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return bool(
        m_projectStorage->basedOn(m_typeId, metaInfo1.m_typeId, metaInfo2.m_typeId, metaInfo3.m_typeId));
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo1,
                             const NodeMetaInfo &metaInfo2,
                             const NodeMetaInfo &metaInfo3,
                             const NodeMetaInfo &metaInfo4,
                             [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is based on 4 node meta infos",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return bool(m_projectStorage->basedOn(
        m_typeId, metaInfo1.m_typeId, metaInfo2.m_typeId, metaInfo3.m_typeId, metaInfo4.m_typeId));
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo1,
                             const NodeMetaInfo &metaInfo2,
                             const NodeMetaInfo &metaInfo3,
                             const NodeMetaInfo &metaInfo4,
                             const NodeMetaInfo &metaInfo5,
                             [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is based on 5 node meta infos",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return bool(m_projectStorage->basedOn(m_typeId,
                                          metaInfo1.m_typeId,
                                          metaInfo2.m_typeId,
                                          metaInfo3.m_typeId,
                                          metaInfo4.m_typeId,
                                          metaInfo5.m_typeId));
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo1,
                             const NodeMetaInfo &metaInfo2,
                             const NodeMetaInfo &metaInfo3,
                             const NodeMetaInfo &metaInfo4,
                             const NodeMetaInfo &metaInfo5,
                             const NodeMetaInfo &metaInfo6,
                             [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is based on 6 node meta infos",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return bool(m_projectStorage->basedOn(m_typeId,
                                          metaInfo1.m_typeId,
                                          metaInfo2.m_typeId,
                                          metaInfo3.m_typeId,
                                          metaInfo4.m_typeId,
                                          metaInfo5.m_typeId,
                                          metaInfo6.m_typeId));
}

bool NodeMetaInfo::isBasedOn(const NodeMetaInfo &metaInfo1,
                             const NodeMetaInfo &metaInfo2,
                             const NodeMetaInfo &metaInfo3,
                             const NodeMetaInfo &metaInfo4,
                             const NodeMetaInfo &metaInfo5,
                             const NodeMetaInfo &metaInfo6,
                             const NodeMetaInfo &metaInfo7,
                             [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is based on 7 node meta infos",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return bool(m_projectStorage->basedOn(m_typeId,
                                          metaInfo1.m_typeId,
                                          metaInfo2.m_typeId,
                                          metaInfo3.m_typeId,
                                          metaInfo4.m_typeId,
                                          metaInfo5.m_typeId,
                                          metaInfo6.m_typeId,
                                          metaInfo7.m_typeId));
}

NodeMetaInfo NodeMetaInfo::basedOn([[maybe_unused]] const NodeMetaInfo &metaInfo,
                                   [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"based on 1 node meta info",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("meta info type id", metaInfo.m_typeId),
                               keyValue("caller location", sl)};

    return {m_projectStorage->basedOn(m_typeId, metaInfo.m_typeId), m_projectStorage};
}

NodeMetaInfo NodeMetaInfo::basedOn([[maybe_unused]] const NodeMetaInfo &metaInfo1,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo2,
                                   [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"based on 2 node meta infos",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return {m_projectStorage->basedOn(m_typeId, metaInfo1.m_typeId, metaInfo2.m_typeId),
            m_projectStorage};
}

NodeMetaInfo NodeMetaInfo::basedOn([[maybe_unused]] const NodeMetaInfo &metaInfo1,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo2,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo3,
                                   [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"based on 3 node meta infos",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return {m_projectStorage->basedOn(m_typeId, metaInfo1.m_typeId, metaInfo2.m_typeId, metaInfo3.m_typeId),
            m_projectStorage};
}

NodeMetaInfo NodeMetaInfo::basedOn([[maybe_unused]] const NodeMetaInfo &metaInfo1,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo2,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo3,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo4,
                                   [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"based on 4 node meta infos",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return {m_projectStorage->basedOn(m_typeId,
                                      metaInfo1.m_typeId,
                                      metaInfo2.m_typeId,
                                      metaInfo3.m_typeId,
                                      metaInfo4.m_typeId),
            m_projectStorage};
}

NodeMetaInfo NodeMetaInfo::basedOn([[maybe_unused]] const NodeMetaInfo &metaInfo1,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo2,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo3,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo4,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo5,
                                   [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"based on 5 node meta infos",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return {m_projectStorage->basedOn(m_typeId,
                                      metaInfo1.m_typeId,
                                      metaInfo2.m_typeId,
                                      metaInfo3.m_typeId,
                                      metaInfo4.m_typeId,
                                      metaInfo5.m_typeId),
            m_projectStorage};
}

NodeMetaInfo NodeMetaInfo::basedOn([[maybe_unused]] const NodeMetaInfo &metaInfo1,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo2,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo3,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo4,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo5,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo6,
                                   [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"based on 6 node meta infos",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return {m_projectStorage->basedOn(m_typeId,
                                      metaInfo1.m_typeId,
                                      metaInfo2.m_typeId,
                                      metaInfo3.m_typeId,
                                      metaInfo4.m_typeId,
                                      metaInfo5.m_typeId,
                                      metaInfo6.m_typeId),
            m_projectStorage};
}

NodeMetaInfo NodeMetaInfo::basedOn([[maybe_unused]] const NodeMetaInfo &metaInfo1,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo2,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo3,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo4,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo5,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo6,
                                   [[maybe_unused]] const NodeMetaInfo &metaInfo7,
                                   [[maybe_unused]] SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"based on 7 node meta infos",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return {m_projectStorage->basedOn(m_typeId,
                                      metaInfo1.m_typeId,
                                      metaInfo2.m_typeId,
                                      metaInfo3.m_typeId,
                                      metaInfo4.m_typeId,
                                      metaInfo5.m_typeId,
                                      metaInfo6.m_typeId,
                                      metaInfo7.m_typeId),
            m_projectStorage};
}

bool NodeMetaInfo::inheritsAll(Utils::span<const TypeId> typeIds, SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info inherits all",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return m_projectStorage->inheritsAll(typeIds, m_typeId);
}

bool NodeMetaInfo::isGraphicalItem(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is graphical item",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    auto itemId = m_projectStorage->commonTypeId<QtQuick, Item>();
    auto windowId = m_projectStorage->commonTypeId<QtQuick, Window>();
    auto dialogId = m_projectStorage
                        ->commonTypeId<QtQuick_Dialogs, QQuickAbstractDialog, ModuleKind::CppLibrary>();
    auto popupId = m_projectStorage->commonTypeId<QtQuick_Templates, Popup>();

    return bool(m_projectStorage->basedOn(m_typeId, itemId, windowId, dialogId, popupId));
}

bool NodeMetaInfo::isQtObject(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is Qt object",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QML, QtObject>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQmlConnections([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is Qt Qml connections",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQml, Connections>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isLayoutable(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is layoutable",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    auto positionerId = m_projectStorage->commonTypeId<QtQuick, Positioner>();
    auto flowId = m_projectStorage->commonTypeId<QtQuick, Flow>();
    auto rowId = m_projectStorage->commonTypeId<QtQuick, Row>();
    auto columnId = m_projectStorage->commonTypeId<QtQuick, Column>();
    auto gridId = m_projectStorage->commonTypeId<QtQuick, Grid>();
    auto repeaterId = m_projectStorage->commonTypeId<QtQuick, Repeater>();
    auto layoutId = m_projectStorage->commonTypeId<QtQuick_Layouts, Layout>();
    auto columnLayoutId = m_projectStorage->commonTypeId<QtQuick_Layouts, ColumnLayout>();
    auto gridLayoutId = m_projectStorage->commonTypeId<QtQuick_Layouts, GridLayout>();
    auto rowLayoutId = m_projectStorage->commonTypeId<QtQuick_Layouts, RowLayout>();
    auto splitViewId = m_projectStorage->commonTypeId<QtQuick_Templates, SplitView>();
    auto swipeViewId = m_projectStorage->commonTypeId<QtQuick_Templates, SwipeView>();

    return bool(m_projectStorage->basedOn(m_typeId,
                                          positionerId,
                                          flowId,
                                          rowId,
                                          columnId,
                                          gridId,
                                          repeaterId,
                                          layoutId,
                                          columnLayoutId,
                                          gridLayoutId,
                                          rowLayoutId,
                                          splitViewId,
                                          swipeViewId));
}

bool NodeMetaInfo::isQtQuickLayoutsLayout(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Layouts.Layout",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Layouts, Layout>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isView(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is view",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    auto listViewId = m_projectStorage->commonTypeId<QtQuick, ListView>();
    auto gridViewId = m_projectStorage->commonTypeId<QtQuick, GridView>();
    auto pathViewId = m_projectStorage->commonTypeId<QtQuick, PathView>();
    return bool(m_projectStorage->basedOn(m_typeId, listViewId, gridViewId, pathViewId));
}

bool NodeMetaInfo::usesCustomParser([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info uses custom parser",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return typeData().traits.usesCustomParser;
}

namespace {

template<typename... TypeIds>
bool isTypeId(TypeId typeId, TypeIds... otherTypeIds)
{
    static_assert(((std::is_same_v<TypeId, TypeIds>) && ...), "Parameter must be a TypeId!");

    return ((typeId == otherTypeIds) || ...);
}

} // namespace

bool NodeMetaInfo::isVector2D(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is vector2d",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isTypeId(m_typeId, m_projectStorage->commonTypeId<QtQuick, vector2d>());
}

bool NodeMetaInfo::isVector3D(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is vector3d",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isTypeId(m_typeId, m_projectStorage->commonTypeId<QtQuick, vector3d>());
}

bool NodeMetaInfo::isVector4D(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is vector4d",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isTypeId(m_typeId, m_projectStorage->commonTypeId<QtQuick, vector4d>());
}

bool NodeMetaInfo::isQtQuickPropertyChanges(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.PropertyChanges",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, Storage::Info::PropertyChanges>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtSafeRendererSafeRendererPicture(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is Qt.SafeRenderer.SafeRendererPicture",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<Qt_SafeRenderer, SafeRendererPicture>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtSafeRendererSafePicture(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is Qt.SafeRenderer.SafePicture",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<Qt_SafeRenderer, SafePicture>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickTimelineKeyframe(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Timeline.Keyframe",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Timeline, Keyframe>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickTimelineTimelineAnimation(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Timeline.TimelineAnimation",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Timeline, TimelineAnimation>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickTimelineTimeline(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Timeline.Timeline",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Timeline, Timeline>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickTimelineKeyframeGroup(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Timeline.KeyframeGroup",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Timeline, KeyframeGroup>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isListOrGridView(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is list or grid view",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    auto listViewId = m_projectStorage->commonTypeId<QtQuick, ListView>();
    auto gridViewId = m_projectStorage->commonTypeId<QtQuick, GridView>();
    return bool(m_projectStorage->basedOn(m_typeId, listViewId, gridViewId));
}

bool NodeMetaInfo::isNumber(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is number",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    auto intId = m_projectStorage->commonTypeId<QML, IntType>();
    auto uintId = m_projectStorage->commonTypeId<QML, UIntType, ModuleKind::CppLibrary>();
    auto floatId = m_projectStorage->commonTypeId<QML, FloatType, ModuleKind::CppLibrary>();
    auto doubleId = m_projectStorage->commonTypeId<QML, DoubleType>();

    return isTypeId(m_typeId, intId, uintId, floatId, doubleId);
}

bool NodeMetaInfo::isQtQuickGradient([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Gradient",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, Gradient>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickImage([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Image",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;

    return isBasedOnCommonType<QtQuick, Image>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickBorderImage([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.BorderImage",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;

    return isBasedOnCommonType<QtQuick, BorderImage>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickPositioner(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Positioner",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;

    return isBasedOnCommonType<QtQuick, Positioner>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickPropertyAnimation(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.PropertyAnimation",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, PropertyAnimation>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickRectangle([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Rectange",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, Rectangle>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickRepeater(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Repeater",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, Repeater>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickShapesShape(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Shapes.Shape",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Shapes, Shape>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickTemplatesTabBar(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Controls.TabBar",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Templates, TabBar>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickTemplatesLabel([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Templates.SwipeView",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Templates, Label>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickTemplatesSwipeView(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Templates.SwipeView",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Templates, SwipeView>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DCamera(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Camera",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Camera>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DBakedLightmap(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.BakedLightmap",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, BakedLightmap>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DBuffer(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Buffer",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Buffer>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DInstanceListEntry(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.InstanceListEntry",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, InstanceListEntry>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DLight(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Light",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Light>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DLightmapper(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Lightmapper",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Lightmapper>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQmlModelsListElement(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQml.Models.ListElement",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQml_Models, ListElement>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickListModel(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.ListModel",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQml_Models, ListModel>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickListView(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.ListView",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, ListView>(m_projectStorage, m_typeId);
}

bool QmlDesigner::NodeMetaInfo::isQtQuickGridView(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.GridView",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, GridView>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DInstanceList(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.InstanceList",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, InstanceList>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DParticles3DParticle3D(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Particles3D.Particle3D",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D_Particles3D, Particle3D>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DParticles3DParticleEmitter3D(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Particles3D.ParticleEmitter3D",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D_Particles3D, ParticleEmitter3D>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DParticles3DAttractor3D(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Particles3D.Attractor3D",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D_Particles3D, Attractor3D>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DParticlesAbstractShape(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Particles3D.AbstractShape",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D_Particles3D, QQuick3DParticleAbstractShape, ModuleKind::CppLibrary>(
        m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickItem(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Item",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, Item>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickPath(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Path",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, Path>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickPauseAnimation(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.PauseAnimation",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, PauseAnimation>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickTransition(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Transition",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, Transition>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickWindow(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Window.Window",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, Window>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickLoader(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Loader",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, Loader>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickState(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.State",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, State>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickStateGroup(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.StateGroup",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, StateGroup>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickStateOperation(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.StateOperation",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, QQuickStateOperation, ModuleKind::CppLibrary>(m_projectStorage,
                                                                                      m_typeId);
}

bool NodeMetaInfo::isQtQuickStudioComponentsArcItem(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Studio.Components.ArcItem",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Studio_Components, ArcItem>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickText(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Text",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick, Text>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtMultimediaSoundEffect(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtMultimedia.SoundEffect",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtMultimedia, SoundEffect>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickStudioComponentsGroupItem(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Studio.Components.GroupItem",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Studio_Components, GroupItem>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickStudioComponentsSvgPathItem(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Studio.Components.SvgPathItem",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Studio_Components, SvgPathItem>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuickStudioUtilsJsonListModel(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick.Studio.Utils.JsonListModel",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick_Studio_Components, JsonListModel>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQmlComponent([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QML.Component",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QML, Component>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isFont([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is font",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isValid() && isTypeId(m_typeId, m_projectStorage->commonTypeId<QtQuick, font>());
}

bool NodeMetaInfo::isColor([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is color",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isValid() && isTypeId(m_typeId, m_projectStorage->commonTypeId<QtQuick, color>());
}

bool NodeMetaInfo::isBool([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is bool",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isValid() && isTypeId(m_typeId, m_projectStorage->commonTypeId<QML, BoolType>());
}

bool NodeMetaInfo::isInteger([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is integer",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isValid() && isTypeId(m_typeId, m_projectStorage->commonTypeId<QML, IntType>());
}

bool NodeMetaInfo::isFloat([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is float",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    auto floatId = m_projectStorage->commonTypeId<QML, FloatType, ModuleKind::CppLibrary>();
    auto doubleId = m_projectStorage->commonTypeId<QML, DoubleType>();

    return isTypeId(m_typeId, floatId, doubleId);
}

bool NodeMetaInfo::isVariant([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is variant",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isValid() && isTypeId(m_typeId, m_projectStorage->commonTypeId<QML, var>());
}

bool NodeMetaInfo::isString([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is string",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isValid() && isTypeId(m_typeId, m_projectStorage->commonTypeId<QML, string>());
}

bool NodeMetaInfo::isUrl([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is url",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isValid() && isTypeId(m_typeId, m_projectStorage->commonTypeId<QML, url>());
}

bool NodeMetaInfo::isQtQuick3DTexture(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Texture",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Texture>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DShader(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Shader",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Shader>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DPass(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Pass",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Pass>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DCommand(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Command",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Command>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DDefaultMaterial(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.DefaultMaterial",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, DefaultMaterial>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DMaterial() const
{
    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Material>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DModel(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Model",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Storage::Info::Model>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DNode(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Node",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Node>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DObject3D(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"is QtQuick3D.Object3D",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Object3D>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DParticles3DAffector3D(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Particles3D.Affector3D",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D_Particles3D, Affector3D>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DView3D(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.View3D",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, View3D>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DPrincipledMaterial(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.PrincipledMaterial",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, PrincipledMaterial>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DSpecularGlossyMaterial(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.SpecularGlossyMaterial",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, SpecularGlossyMaterial>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DParticles3DSpriteParticle3D(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Particles3D.SpriteParticle3D",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D_Particles3D, SpriteParticle3D>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DTextureInput(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.TextureInput",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, TextureInput>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DCubeMapTexture(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.CubeMapTexture",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, CubeMapTexture>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DSceneEnvironment(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.SceneEnvironment",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, SceneEnvironment>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isQtQuick3DEffect(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is QtQuick3D.Effect",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return isBasedOnCommonType<QtQuick3D, Effect>(m_projectStorage, m_typeId);
}

bool NodeMetaInfo::isEnumeration(SL sl) const
{
    if (!isValid())
        return false;

    NanotraceHR::Tracer tracer{"node meta info is enumeration",
                               category(),
                               keyValue("type id", m_typeId),
                               keyValue("caller location", sl)};

    return typeData().traits.isEnum;
}

NodeMetaInfo PropertyMetaInfo::propertyType() const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get property type",
                               category(),
                               keyValue("property declaration id", m_id)};

    return {propertyData().propertyTypeId, m_projectStorage};
}

NodeMetaInfo PropertyMetaInfo::type() const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get property owner type ",
                               category(),
                               keyValue("property declaration id", m_id)};

    return NodeMetaInfo(propertyData().typeId, m_projectStorage);
}

PropertyName PropertyMetaInfo::name() const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info get property name",
                               category(),
                               keyValue("property declaration id", m_id)};

    return propertyData().name.toQByteArray();
}

bool PropertyMetaInfo::isWritable() const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info is property writable",
                               category(),
                               keyValue("property declaration id", m_id)};

    return !(propertyData().traits & Storage::PropertyDeclarationTraits::IsReadOnly);
}

bool PropertyMetaInfo::isReadOnly() const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info is property read only",
                               category(),
                               keyValue("property declaration id", m_id)};

    return propertyData().traits & Storage::PropertyDeclarationTraits::IsReadOnly;
}

bool PropertyMetaInfo::isListProperty() const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info is list property",
                               category(),
                               keyValue("property declaration id", m_id)};

    return propertyData().traits & Storage::PropertyDeclarationTraits::IsList;
}

bool PropertyMetaInfo::isEnumType() const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info is enum type",
                               category(),
                               keyValue("property has enumeration type", m_id)};

    return propertyType().isEnumeration();
}

bool PropertyMetaInfo::isPrivate() const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info is private property",
                               category(),
                               keyValue("property declaration id", m_id)};

    return isValid() && propertyData().name.startsWith("__");
}

bool PropertyMetaInfo::isPointer() const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"node meta info is pointer property",
                               category(),
                               keyValue("property declaration id", m_id)};

    return isValid() && (propertyData().traits & Storage::PropertyDeclarationTraits::IsPointer);
}

namespace {
template<typename... QMetaTypes>
bool isType(const QMetaType &type, const QMetaTypes &...types)
{
    return ((type == types) || ...);
}
} // namespace

QVariant PropertyMetaInfo::castedValue(const QVariant &value) const
{
    if (!isValid())
        return {};

    const TypeId &typeId = propertyData().propertyTypeId;

    using namespace Storage::Info;

    if ((isEnumType() || typeId == m_projectStorage->commonTypeId<QML, IntType>())
        && value.canConvert<Enumeration>()) {
        return value;
    }

    static constexpr auto boolType = QMetaType::fromType<bool>();
    static constexpr auto intType = QMetaType::fromType<int>();
    static constexpr auto longType = QMetaType::fromType<long>();
    static constexpr auto longLongType = QMetaType::fromType<long long>();
    static constexpr auto floatType = QMetaType::fromType<float>();
    static constexpr auto doubleType = QMetaType::fromType<double>();
    static constexpr auto qStringType = QMetaType::fromType<QString>();
    static constexpr auto qUrlType = QMetaType::fromType<QUrl>();
    static constexpr auto qColorType = QMetaType::fromType<QColor>();

    if (value.typeId() == QMetaType::User && value.typeId() == ModelNode::variantTypeId()) {
        return value;
    } else if (typeId == m_projectStorage->commonTypeId<QML, var>()) {
        return value;
    } else if (typeId == m_projectStorage->commonTypeId<QML, DoubleType>()) {
        return value.toDouble();
    } else if (typeId == m_projectStorage->commonTypeId<QML, FloatType, ModuleKind::CppLibrary>()) {
        return value.toFloat();
    } else if (typeId == m_projectStorage->commonTypeId<QML, IntType>()) {
        return value.toInt();
    } else if (typeId == m_projectStorage->commonTypeId<QML, BoolType>()) {
        return isType(value.metaType(), boolType, intType, longType, longLongType, floatType, doubleType)
               && value.toBool();
    } else if (typeId == m_projectStorage->commonTypeId<QML, string>()) {
        if (isType(value.metaType(), qStringType))
            return value;
        else
            return QString{};
    } else if (typeId == m_projectStorage->commonTypeId<QML, date>()) {
        return value.toDateTime();
    } else if (typeId == m_projectStorage->commonTypeId<QML, url>()) {
        if (isType(value.metaType(), qUrlType))
            return value;
        else if (isType(value.metaType(), qStringType))
            return value.toUrl();
        else
            return QUrl{};
    } else if (typeId == m_projectStorage->commonTypeId<QtQuick, color>()) {
        if (isType(value.metaType(), qColorType))
            return value;
        else
            return QColor{};
    } else if (typeId == m_projectStorage->commonTypeId<QtQuick, vector2d>()) {
        return value.value<QVector2D>();
    } else if (typeId == m_projectStorage->commonTypeId<QtQuick, vector3d>()) {
        return value.value<QVector3D>();
    } else if (typeId == m_projectStorage->commonTypeId<QtQuick, vector4d>()) {
        return value.value<QVector4D>();
    }

    return {};
}

const Storage::Info::PropertyDeclaration &PropertyMetaInfo::propertyData() const
{
    if (!m_propertyData)
        m_propertyData = m_projectStorage->propertyDeclaration(m_id);

    return *m_propertyData;
}

NodeMetaInfo NodeMetaInfo::commonPrototype(const NodeMetaInfo &metaInfo) const
{
    if (isValid() && metaInfo) {
        const auto firstTypeIds = m_projectStorage->prototypeAndSelfIds(m_typeId);
        const auto secondTypeIds = m_projectStorage->prototypeAndSelfIds(metaInfo.m_typeId);
        auto found = std::ranges::mismatch(firstTypeIds.rbegin(),
                                           firstTypeIds.rend(),
                                           secondTypeIds.rbegin(),
                                           secondTypeIds.rend())
                         .in1.base();

        if (found != firstTypeIds.end())
            return NodeMetaInfo{*found, m_projectStorage};
    }

    return {};
}

NodeMetaInfo::NodeMetaInfos NodeMetaInfo::heirs() const
{
    if (isValid()) {
        return Utils::transform<NodeMetaInfos>(m_projectStorage->heirIds(m_typeId),
                                               NodeMetaInfo::bind(m_projectStorage));
    }

    return {};
}

namespace {

void addCompoundProperties(CompoundPropertyMetaInfos &inflatedProperties,
                           const PropertyMetaInfo &parentProperty,
                           PropertyMetaInfos properties)
{
    for (PropertyMetaInfo &property : properties)
        inflatedProperties.emplace_back(std::move(property), parentProperty);
}

bool maybeCanHaveProperties(const NodeMetaInfo &type)
{
    if (!type)
        return false;

    using namespace Storage::Info;
    const auto &cache = type.projectStorage().commonTypeCache();
    auto typeId = type.id();
    const auto &typeIdsWithoutProperties = cache.typeIdsWithoutProperties();
    const auto begin = typeIdsWithoutProperties.begin();
    const auto end = typeIdsWithoutProperties.end();

    return std::find(begin, end, typeId) == end;
}

void addSubProperties(CompoundPropertyMetaInfos &inflatedProperties,
                      PropertyMetaInfo &propertyMetaInfo,
                      const NodeMetaInfo &propertyType)
{
    if (maybeCanHaveProperties(propertyType)) {
        auto subProperties = propertyType.properties();
        if (!subProperties.empty()) {
            addCompoundProperties(inflatedProperties, propertyMetaInfo, subProperties);
            return;
        }
    }

    inflatedProperties.emplace_back(std::move(propertyMetaInfo));
}

bool isValueOrNonListReference(const NodeMetaInfo &propertyType, const PropertyMetaInfo &property)
{
    return (propertyType.type() == MetaInfoType::Value
            || propertyType.type() == MetaInfoType::Reference)
           && !property.isListProperty();
}

} // namespace

CompoundPropertyMetaInfos MetaInfoUtils::inflateValueProperties(PropertyMetaInfos properties)
{
    CompoundPropertyMetaInfos inflatedProperties;
    inflatedProperties.reserve(properties.size() * 2);

    for (auto &property : properties) {
        if (auto propertyType = property.propertyType(); propertyType.type() == MetaInfoType::Value)
            addSubProperties(inflatedProperties, property, propertyType);
        else
            inflatedProperties.emplace_back(std::move(property));
    }

    return inflatedProperties;
}

CompoundPropertyMetaInfos MetaInfoUtils::inflateValueAndReferenceProperties(PropertyMetaInfos properties)
{
    CompoundPropertyMetaInfos inflatedProperties;
    inflatedProperties.reserve(properties.size() * 2);

    for (auto &property : properties) {
        if (auto propertyType = property.propertyType();
            propertyType && isValueOrNonListReference(propertyType, property)) {
            addSubProperties(inflatedProperties, property, propertyType);
        } else {
            inflatedProperties.emplace_back(std::move(property));
        }
    }

    return inflatedProperties;
}

CompoundPropertyMetaInfos MetaInfoUtils::addInflatedValueAndReferenceProperties(PropertyMetaInfos properties)
{
    CompoundPropertyMetaInfos inflatedProperties;
    inflatedProperties.reserve(properties.size() * 2);

    for (auto &property : properties) {
        auto copy = property;
        inflatedProperties.emplace_back(std::move(copy));
        if (auto propertyType = property.propertyType();
            propertyType && isValueOrNonListReference(propertyType, property)) {
            addSubProperties(inflatedProperties, property, propertyType);
        }
    }

    return inflatedProperties;
}

} // namespace QmlDesigner

QT_WARNING_POP
