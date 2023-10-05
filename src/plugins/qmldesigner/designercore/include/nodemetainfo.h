// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "propertymetainfo.h"
#include "qmldesignercorelib_global.h"

#include <projectstorage/projectstorageinterface.h>
#include <projectstorage/projectstoragetypes.h>
#include <projectstorageids.h>

#include <QList>
#include <QString>
#include <QIcon>

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
public:
    NodeMetaInfo();
    NodeMetaInfo(Model *model, const TypeName &typeName, int majorVersion, int minorVersion);
    NodeMetaInfo(TypeId typeId, NotNullPointer<const ProjectStorageType> projectStorage)
        : m_typeId{typeId}
        , m_projectStorage{projectStorage}
    {}
    NodeMetaInfo(NotNullPointer<const ProjectStorageType> projectStorage)
        : m_projectStorage{projectStorage}
    {}

    NodeMetaInfo(const NodeMetaInfo &);
    NodeMetaInfo &operator=(const NodeMetaInfo &);
    NodeMetaInfo(NodeMetaInfo &&);
    NodeMetaInfo &operator=(NodeMetaInfo &&);
    ~NodeMetaInfo();

    bool isValid() const;
    explicit operator bool() const { return isValid(); }

    TypeId id() const { return m_typeId; }

    MetaInfoType type() const;
    bool isFileComponent() const;
    bool isProjectComponent() const;
    bool isInProjectModule() const;
    bool hasProperty(::Utils::SmallStringView propertyName) const;
    PropertyMetaInfos properties() const;
    PropertyMetaInfos localProperties() const;
    PropertyMetaInfo property(const PropertyName &propertyName) const;
    PropertyNameList signalNames() const;
    PropertyNameList slotNames() const;
    PropertyName defaultPropertyName() const;
    PropertyMetaInfo defaultProperty() const;
    bool hasDefaultProperty() const;

    std::vector<NodeMetaInfo> selfAndPrototypes() const;
    std::vector<NodeMetaInfo> prototypes() const;
    NodeMetaInfo commonBase(const NodeMetaInfo &metaInfo) const;

    bool defaultPropertyIsComponent() const;

    TypeName typeName() const;
    TypeName simplifiedTypeName() const;
    int majorVersion() const;
    int minorVersion() const;

    Storage::Info::ExportedTypeNames allExportedTypeNames() const;
    Storage::Info::ExportedTypeNames exportedTypeNamesForSourceId(SourceId sourceId) const;

    SourceId sourceId() const;
    QString componentFileName() const;

    bool isBasedOn(const NodeMetaInfo &metaInfo) const;
    bool isBasedOn(const NodeMetaInfo &metaInfo1, const NodeMetaInfo &metaInfo2) const;
    bool isBasedOn(const NodeMetaInfo &metaInfo1,
                   const NodeMetaInfo &metaInfo2,
                   const NodeMetaInfo &metaInfo3) const;
    bool isBasedOn(const NodeMetaInfo &metaInfo1,
                   const NodeMetaInfo &metaInfo2,
                   const NodeMetaInfo &metaInfo3,
                   const NodeMetaInfo &metaInfo4) const;
    bool isBasedOn(const NodeMetaInfo &metaInfo1,
                   const NodeMetaInfo &metaInfo2,
                   const NodeMetaInfo &metaInfo3,
                   const NodeMetaInfo &metaInfo4,
                   const NodeMetaInfo &metaInfo5) const;
    bool isBasedOn(const NodeMetaInfo &metaInfo1,
                   const NodeMetaInfo &metaInfo2,
                   const NodeMetaInfo &metaInfo3,
                   const NodeMetaInfo &metaInfo4,
                   const NodeMetaInfo &metaInfo5,
                   const NodeMetaInfo &metaInfo6) const;
    bool isBasedOn(const NodeMetaInfo &metaInfo1,
                   const NodeMetaInfo &metaInfo2,
                   const NodeMetaInfo &metaInfo3,
                   const NodeMetaInfo &metaInfo4,
                   const NodeMetaInfo &metaInfo5,
                   const NodeMetaInfo &metaInfo6,
                   const NodeMetaInfo &metaInfo7) const;

    bool isAlias() const;
    bool isBool() const;
    bool isColor() const;
    bool isFloat() const;
    bool isFlowViewFlowActionArea() const;
    bool isFlowViewFlowDecision() const;
    bool isFlowViewFlowItem() const;
    bool isFlowViewFlowTransition() const;
    bool isFlowViewFlowView() const;
    bool isFlowViewFlowWildcard() const;
    bool isFlowViewItem() const;
    bool isFont() const;
    bool isGraphicalItem() const;
    bool isInteger() const;
    bool isLayoutable() const;
    bool isListOrGridView() const;
    bool isNumber() const;
    bool isQmlComponent() const;
    bool isQtMultimediaSoundEffect() const;
    bool isQtObject() const;
    bool isQtQuick3DBakedLightmap() const;
    bool isQtQuick3DBuffer() const;
    bool isQtQuick3DCamera() const;
    bool isQtQuick3DCommand() const;
    bool isQtQuick3DDefaultMaterial() const;
    bool isQtQuick3DEffect() const;
    bool isQtQuick3DInstanceList() const;
    bool isQtQuick3DInstanceListEntry() const;
    bool isQtQuick3DLight() const;
    bool isQtQuickListElement() const;
    bool isQtQuickListModel() const;
    bool isQtQuick3DMaterial() const;
    bool isQtQuick3DModel() const;
    bool isQtQuick3DNode() const;
    bool isQtQuick3DParticlesAbstractShape() const;
    bool isQtQuick3DParticles3DAffector3D() const;
    bool isQtQuick3DParticles3DAttractor3D() const;
    bool isQtQuick3DParticles3DParticle3D() const;
    bool isQtQuick3DParticles3DParticleEmitter3D() const;
    bool isQtQuick3DParticles3DSpriteParticle3D() const;
    bool isQtQuick3DPass() const;
    bool isQtQuick3DPrincipledMaterial() const;
    bool isQtQuick3DSpecularGlossyMaterial() const;
    bool isQtQuick3DSceneEnvironment() const;
    bool isQtQuick3DShader() const;
    bool isQtQuick3DTexture() const;
    bool isQtQuick3DTextureInput() const;
    bool isQtQuick3DCubeMapTexture() const;
    bool isQtQuick3DView3D() const;
    bool isQtQuickBorderImage() const;
    bool isQtQuickControlsSwipeView() const;
    bool isQtQuickControlsTabBar() const;
    bool isQtQuickExtrasPicture() const;
    bool isQtQuickImage() const;
    bool isQtQuickItem() const;
    bool isQtQuickLayoutsLayout() const;
    bool isQtQuickLoader() const;
    bool isQtQuickPath() const;
    bool isQtQuickPauseAnimation() const;
    bool isQtQuickPositioner() const;
    bool isQtQuickPropertyAnimation() const;
    bool isQtQuickPropertyChanges() const;
    bool isQtQuickRepeater() const;
    bool isQtQuickState() const;
    bool isQtQuickStateOperation() const;
    bool isQtQuickStudioComponentsGroupItem() const;
    bool isQtQuickText() const;
    bool isQtQuickTimelineKeyframe() const;
    bool isQtQuickTimelineKeyframeGroup() const;
    bool isQtQuickTimelineTimeline() const;
    bool isQtQuickTimelineTimelineAnimation() const;
    bool isQtQuickTransition() const;
    bool isQtQuickWindowWindow() const;
    bool isQtSafeRendererSafePicture() const;
    bool isQtSafeRendererSafeRendererPicture() const;
    bool isString() const;
    bool isSuitableForMouseAreaFill() const;
    bool isUrl() const;
    bool isVariant() const;
    bool isVector2D() const;
    bool isVector3D() const;
    bool isVector4D() const;
    bool isView() const;
    bool usesCustomParser() const;

    bool isEnumeration() const;
    QString importDirectoryPath() const;
    QString requiredImportString() const;

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

    SourceId propertyEditorPathId() const;

    const ProjectStorageType &projectStorage() const { return *m_projectStorage; }

    void *key() const
    {
        if constexpr (!useProjectStorage())
            return m_privateData.get();

        return nullptr;
    }

private:
    const Storage::Info::Type &typeData() const;
    bool isSubclassOf(const TypeName &type, int majorVersion = -1, int minorVersion = -1) const;

private:
    TypeId m_typeId;
    NotNullPointer<const ProjectStorageType> m_projectStorage = {};
    mutable std::optional<Storage::Info::Type> m_typeData;
    std::shared_ptr<NodeMetaInfoPrivate> m_privateData;
};

using NodeMetaInfos = std::vector<NodeMetaInfo>;

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
