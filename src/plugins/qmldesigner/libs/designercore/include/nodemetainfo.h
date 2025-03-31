// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "propertymetainfo.h"
#include "qmldesignercorelib_global.h"

#include <projectstorage/projectstorageinterface.h>
#include <projectstorage/projectstoragetypes.h>
#include <projectstorageids.h>
#include <tracing/qmldesignertracingsourcelocation.h>

#include <QList>
#include <QString>
#include <QIcon>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

QT_BEGIN_NAMESPACE
class QDeclarativeContext;
QT_END_NAMESPACE

namespace QmlDesigner {

class MetaInfo;
class Model;
class AbstractProperty;
class NodeMetaInfoPrivate;

enum class MetaInfoType { None, Reference, Value, Sequence };

class QMLDESIGNERCORE_EXPORT NodeMetaInfo
{
    using SL = ModelTracing::SourceLocation;
    using NodeMetaInfos = std::vector<NodeMetaInfo>;

public:
    NodeMetaInfo();
#ifndef QDS_USE_PROJECTSTORAGE
    NodeMetaInfo(Model *model, const TypeName &typeName, int majorVersion, int minorVersion);
#else
    NodeMetaInfo(TypeId typeId, NotNullPointer<const ProjectStorageType> projectStorage)
        : m_typeId{typeId}
        , m_projectStorage{projectStorage}
    {}
    NodeMetaInfo(NotNullPointer<const ProjectStorageType> projectStorage)
        : m_projectStorage{projectStorage}
    {}
#endif

    NodeMetaInfo(const NodeMetaInfo &);
    NodeMetaInfo &operator=(const NodeMetaInfo &);
    NodeMetaInfo(NodeMetaInfo &&);
    NodeMetaInfo &operator=(NodeMetaInfo &&);
    ~NodeMetaInfo();

#ifdef QDS_USE_PROJECTSTORAGE
    static NodeMetaInfo create(NotNullPointer<const ProjectStorageType> projectStorage, TypeId typeId)
    {
        return {typeId, projectStorage};
    }

    static auto bind(NotNullPointer<const ProjectStorageType> projectStorage)
    {
        return std::bind_front(&NodeMetaInfo::create, projectStorage);
    }
#endif

    bool isValid() const;
    explicit operator bool() const { return isValid(); }

    TypeId id() const { return m_typeId; }

    MetaInfoType type(SL sl = {}) const;
    bool isFileComponent(SL sl = {}) const;
    bool isSingleton(SL sl = {}) const;
    bool isInsideProject(SL sl = {}) const;
    FlagIs canBeContainer(SL sl = {}) const;
    FlagIs forceClip(SL sl = {}) const;
    FlagIs doesLayoutChildren(SL sl = {}) const;
    FlagIs canBeDroppedInFormEditor(SL sl = {}) const;
    FlagIs canBeDroppedInNavigator(SL sl = {}) const;
    FlagIs canBeDroppedInView3D(SL sl = {}) const;
    FlagIs isMovable(SL sl = {}) const;
    FlagIs isResizable(SL sl = {}) const;
    FlagIs hasFormEditorItem(SL sl = {}) const;
    FlagIs isStackedContainer(SL sl = {}) const;
    FlagIs takesOverRenderingOfChildren(SL sl = {}) const;
    FlagIs visibleInNavigator(SL sl = {}) const;
    FlagIs hideInNavigator() const;
    FlagIs visibleInLibrary(SL sl = {}) const;

    bool hasProperty(::Utils::SmallStringView propertyName) const;
    PropertyMetaInfos properties(SL sl = {}) const;
    PropertyMetaInfos localProperties(SL sl = {}) const;
    PropertyMetaInfo property(PropertyNameView propertyName) const;
    PropertyNameList signalNames(SL sl = {}) const;
    PropertyNameList slotNames(SL sl = {}) const;
    PropertyName defaultPropertyName(SL sl = {}) const;
    PropertyMetaInfo defaultProperty(SL sl = {}) const;
    bool hasDefaultProperty(SL sl = {}) const;

    NodeMetaInfos selfAndPrototypes(SL sl = {}) const;
    NodeMetaInfos prototypes(SL sl = {}) const;
    NodeMetaInfos heirs() const;
    NodeMetaInfo commonBase(const NodeMetaInfo &metaInfo) const;

    bool defaultPropertyIsComponent() const;

    QString displayName() const;
#ifndef QDS_USE_PROJECTSTORAGE
    TypeName typeName() const;
    TypeName simplifiedTypeName() const;
    int majorVersion() const;
    int minorVersion() const;
#endif
    Storage::Info::ExportedTypeNames allExportedTypeNames(SL sl = {}) const;
    Storage::Info::ExportedTypeNames exportedTypeNamesForSourceId(SourceId sourceId) const;

    Storage::Info::TypeHints typeHints(SL sl = {}) const;
    Utils::PathString iconPath(SL sl = {}) const;
    Storage::Info::ItemLibraryEntries itemLibrariesEntries(SL sl = {}) const;

    SourceId sourceId(SL sl = {}) const;
#ifndef QDS_USE_PROJECTSTORAGE
    QString componentFileName() const;
#endif

    bool isBasedOn(const NodeMetaInfo &metaInfo, SL sl = {}) const;
    bool isBasedOn(const NodeMetaInfo &metaInfo1, const NodeMetaInfo &metaInfo2, SL sl = {}) const;
    bool isBasedOn(const NodeMetaInfo &metaInfo1,
                   const NodeMetaInfo &metaInfo2,
                   const NodeMetaInfo &metaInfo3,
                   SL sl = {}) const;
    bool isBasedOn(const NodeMetaInfo &metaInfo1,
                   const NodeMetaInfo &metaInfo2,
                   const NodeMetaInfo &metaInfo3,
                   const NodeMetaInfo &metaInfo4,
                   SL sl = {}) const;
    bool isBasedOn(const NodeMetaInfo &metaInfo1,
                   const NodeMetaInfo &metaInfo2,
                   const NodeMetaInfo &metaInfo3,
                   const NodeMetaInfo &metaInfo4,
                   const NodeMetaInfo &metaInfo5,
                   SL sl = {}) const;
    bool isBasedOn(const NodeMetaInfo &metaInfo1,
                   const NodeMetaInfo &metaInfo2,
                   const NodeMetaInfo &metaInfo3,
                   const NodeMetaInfo &metaInfo4,
                   const NodeMetaInfo &metaInfo5,
                   const NodeMetaInfo &metaInfo6,
                   SL sl = {}) const;
    bool isBasedOn(const NodeMetaInfo &metaInfo1,
                   const NodeMetaInfo &metaInfo2,
                   const NodeMetaInfo &metaInfo3,
                   const NodeMetaInfo &metaInfo4,
                   const NodeMetaInfo &metaInfo5,
                   const NodeMetaInfo &metaInfo6,
                   const NodeMetaInfo &metaInfo7,
                   SL sl = {}) const;

    bool isAlias(SL sl = {}) const;
    bool isBool(SL sl = {}) const;
    bool isColor(SL sl = {}) const;
    bool isFloat(SL sl = {}) const;
    bool isFlowViewFlowActionArea() const;
    bool isFlowViewFlowDecision(SL sl = {}) const;
    bool isFlowViewFlowItem(SL sl = {}) const;
    bool isFlowViewFlowTransition(SL sl = {}) const;
    bool isFlowViewFlowView(SL sl = {}) const;
    bool isFlowViewFlowWildcard(SL sl = {}) const;
    bool isFlowViewItem(SL sl = {}) const;
    bool isFont(SL sl = {}) const;
    bool isGraphicalItem(SL sl = {}) const;
    bool isInteger(SL sl = {}) const;
    bool isLayoutable(SL sl = {}) const;
    bool isListOrGridView(SL sl = {}) const;
    bool isNumber(SL sl = {}) const;
    bool isQmlComponent(SL sl = {}) const;
    bool isQtMultimediaSoundEffect(SL sl = {}) const;
    bool isQtObject(SL sl = {}) const;
    bool isQtQmlConnections(SL sl = {}) const;
    bool isQtQmlModelsListElement(SL sl = {}) const;
    bool isQtQuick3DBakedLightmap(SL sl = {}) const;
    bool isQtQuick3DBuffer(SL sl = {}) const;
    bool isQtQuick3DCamera(SL sl = {}) const;
    bool isQtQuick3DCommand(SL sl = {}) const;
    bool isQtQuick3DDefaultMaterial(SL sl = {}) const;
    bool isQtQuick3DEffect(SL sl = {}) const;
    bool isQtQuick3DInstanceList(SL sl = {}) const;
    bool isQtQuick3DInstanceListEntry(SL sl = {}) const;
    bool isQtQuick3DLight(SL sl = {}) const;
    bool isQtQuickListModel(SL sl = {}) const;
    bool isQtQuickListView(SL sl = {}) const;
    bool isQtQuickGridView(SL sl = {}) const;
    bool isQtQuick3DMaterial() const;
    bool isQtQuick3DModel(SL sl = {}) const;
    bool isQtQuick3DNode(SL sl = {}) const;
    bool isQtQuick3DParticlesAbstractShape(SL sl = {}) const;
    bool isQtQuick3DParticles3DAffector3D(SL sl = {}) const;
    bool isQtQuick3DParticles3DAttractor3D(SL sl = {}) const;
    bool isQtQuick3DParticles3DParticle3D(SL sl = {}) const;
    bool isQtQuick3DParticles3DParticleEmitter3D(SL sl = {}) const;
    bool isQtQuick3DParticles3DSpriteParticle3D(SL sl = {}) const;
    bool isQtQuick3DPass(SL sl = {}) const;
    bool isQtQuick3DPrincipledMaterial(SL sl = {}) const;
    bool isQtQuick3DSpecularGlossyMaterial(SL sl = {}) const;
    bool isQtQuick3DSceneEnvironment(SL sl = {}) const;
    bool isQtQuick3DShader(SL sl = {}) const;
    bool isQtQuick3DTexture(SL sl = {}) const;
    bool isQtQuick3DTextureInput(SL sl = {}) const;
    bool isQtQuick3DCubeMapTexture(SL sl = {}) const;
    bool isQtQuick3DView3D(SL sl = {}) const;
    bool isQtQuickBorderImage(SL sl = {}) const;
    bool isQtQuickControlsLabel(SL sl = {}) const;
    bool isQtQuickControlsSwipeView(SL sl = {}) const;
    bool isQtQuickControlsTabBar(SL sl = {}) const;
    bool isQtQuickExtrasPicture(SL sl = {}) const;
    bool isQtQuickGradient(SL sl = {}) const;
    bool isQtQuickImage(SL sl = {}) const;
    bool isQtQuickItem(SL sl = {}) const;
    bool isQtQuickLayoutsLayout(SL sl = {}) const;
    bool isQtQuickLoader(SL sl = {}) const;
    bool isQtQuickPath(SL sl = {}) const;
    bool isQtQuickPauseAnimation(SL sl = {}) const;
    bool isQtQuickPositioner(SL sl = {}) const;
    bool isQtQuickPropertyAnimation(SL sl = {}) const;
    bool isQtQuickPropertyChanges(SL sl = {}) const;
    bool isQtQuickRectangle(SL sl = {}) const;
    bool isQtQuickRepeater(SL sl = {}) const;
    bool isQtQuickShapesShape(SL sl = {}) const;
    bool isQtQuickState(SL sl = {}) const;
    bool isQtQuickStateGroup(SL sl = {}) const;
    bool isQtQuickStateOperation(SL sl = {}) const;
    bool isQtQuickStudioComponentsArcItem(SL sl = {}) const;
    bool isQtQuickStudioComponentsGroupItem(SL sl = {}) const;
    bool isQtQuickStudioComponentsSvgPathItem(SL sl = {}) const;
    bool isQtQuickStudioUtilsJsonListModel(SL sl = {}) const;
    bool isQtQuickText(SL sl = {}) const;
    bool isQtQuickTimelineKeyframe(SL sl = {}) const;
    bool isQtQuickTimelineKeyframeGroup(SL sl = {}) const;
    bool isQtQuickTimelineTimeline(SL sl = {}) const;
    bool isQtQuickTimelineTimelineAnimation(SL sl = {}) const;
    bool isQtQuickTransition(SL sl = {}) const;
    bool isQtQuickWindowWindow(SL sl = {}) const;
    bool isQtSafeRendererSafePicture(SL sl = {}) const;
    bool isQtSafeRendererSafeRendererPicture(SL sl = {}) const;
    bool isString(SL sl = {}) const;
    bool isSuitableForMouseAreaFill(SL sl = {}) const;
    bool isUrl(SL sl = {}) const;
    bool isVariant(SL sl = {}) const;
    bool isVector2D(SL sl = {}) const;
    bool isVector3D(SL sl = {}) const;
    bool isVector4D(SL sl = {}) const;
    bool isView(SL sl = {}) const;
    bool usesCustomParser(SL sl = {}) const;

    bool isEnumeration(SL sl = {}) const;
#ifndef QDS_USE_PROJECTSTORAGE
    QString importDirectoryPath() const;
    QString requiredImportString() const;
#endif
    friend bool operator==(const NodeMetaInfo &first, const NodeMetaInfo &second)
    {
        if constexpr (useProjectStorage())
            return first.m_typeId == second.m_typeId;
        else
            return first.m_privateData == second.m_privateData;
    }

    friend bool operator!=(const NodeMetaInfo &first, const NodeMetaInfo &second)
    {
        return !(first == second);
    }

    SourceId propertyEditorPathId(SL sl = {}) const;

    const ProjectStorageType &projectStorage() const { return *m_projectStorage; }

    void *key() const
    {
        if constexpr (!useProjectStorage())
            return m_privateData.get();

        return nullptr;
    }

private:
    const Storage::Info::Type &typeData() const;
    PropertyDeclarationId defaultPropertyDeclarationId() const;
    bool isSubclassOf(const TypeName &type, int majorVersion = -1, int minorVersion = -1) const;

private:
    TypeId m_typeId;
    NotNullPointer<const ProjectStorageType> m_projectStorage = {};
    mutable std::optional<Storage::Info::Type> m_typeData;
    mutable std::optional<PropertyDeclarationId> m_defaultPropertyId;
    std::shared_ptr<NodeMetaInfoPrivate> m_privateData;
};

using NodeMetaInfos = std::vector<NodeMetaInfo>;
template<qsizetype size>
using SmallNodeMetaInfos = QVarLengthArray<NodeMetaInfo, size>;

} //QmlDesigner

namespace std {
template<>
struct hash<QmlDesigner::NodeMetaInfo>
{
    auto operator()(const QmlDesigner::NodeMetaInfo &metaInfo) const
    {
        if constexpr (QmlDesigner::useProjectStorage())
            return std::hash<QmlDesigner::TypeId>{}(metaInfo.id());
        else
            return std::hash<void *>{}(metaInfo.key());
    }
};
} // namespace std
