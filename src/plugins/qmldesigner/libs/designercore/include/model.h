// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <commondefines.h>
#include <customnotificationpackage.h>
#include <documentmessage.h>
#include <import.h>
#include <model/modelresourcemanagementinterface.h>
#include <module.h>
#include <projectstorage/projectstoragefwd.h>
#include <projectstorage/projectstorageinfotypes.h>
#include <projectstorage/projectstorageobserver.h>
#include <projectstorageids.h>
#include <utils/uniqueobjectptr.h>

#include <QMimeData>
#include <QObject>
#include <QPair>
#include <QVarLengthArray>

#include <variant>

QT_BEGIN_NAMESPACE
class QPixmap;
class QUrl;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {
class ModelPrivate;
class WriteLocker;
} //Internal

class AnchorLine;
class ModelNode;
class NodeState;
class AbstractView;
class NodeStateChangeSet;
class MetaInfo;
class NodeMetaInfo;
class NodeMetaInfoPrivate;
class ModelState;
class AbstractProperty;
class RewriterView;
class NodeInstanceView;
class TextModifier;
class ItemLibraryEntry;

using PropertyListType = QList<QPair<PropertyName, QVariant>>;
template<qsizetype size>
using SmallNodeMetaInfos = QVarLengthArray<NodeMetaInfo, size>;

enum class BypassModelResourceManagement { No, Yes };

class QMLDESIGNERCORE_EXPORT Model : public QObject
{
    friend ModelNode;
    friend NodeMetaInfo;
    friend NodeMetaInfoPrivate;
    friend AbstractProperty;
    friend AbstractView;
    friend Internal::ModelPrivate;
    friend Internal::WriteLocker;
    friend ModelDeleter;

    using SL = ModelTracing::SourceLocation;

    Q_OBJECT

public:
    enum ViewNotification { NotifyView, DoNotNotifyView };

    Model(ProjectStorageDependencies projectStorageDependencies,
          const TypeName &type,
          int major = 1,
          int minor = 1,
          Model *metaInfoProxyModel = nullptr,
          std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {},
          SL sl = {});
    Model(ProjectStorageDependencies projectStorageDependencies,
          Utils::SmallStringView typeName,
          Imports imports,
          const QUrl &fileUrl,
          std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {},
          SL sl = {});
#ifndef QDS_USE_PROJECTSTORAGE
    Model(const TypeName &typeName,
          int major = 1,
          int minor = 1,
          Model *metaInfoProxyModel = nullptr,
          std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {});
#endif
    ~Model();

#ifndef QDS_USE_PROJECTSTORAGE
    static ModelPointer create(const TypeName &typeName,
                               int major = 1,
                               int minor = 1,
                               Model *metaInfoProxyModel = nullptr,
                               std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {})
    {
        return ModelPointer(
            new Model(typeName, major, minor, metaInfoProxyModel, std::move(resourceManagement)));
    }
#endif

    static ModelPointer create(ProjectStorageDependencies projectStorageDependencies,
                               Utils::SmallStringView typeName,
                               Imports imports,
                               const QUrl &fileUrl,
                               std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {},
                               SL sl = {})
    {
        NanotraceHR::Tracer tracer{"model create",
                                   ModelTracing::category(),
                                   keyValue("caller location", sl)};

        return ModelPointer(new Model(projectStorageDependencies,
                                      typeName,
                                      imports,
                                      fileUrl,
                                      std::move(resourceManagement)));
    }

#ifndef QDS_USE_PROJECTSTORAGE
    static ModelPointer create(ProjectStorageDependencies projectStorageDependencies,
                               const TypeName &typeName,
                               int major = 1,
                               int minor = 1,
                               std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {})
    {
        return ModelPointer(new Model(projectStorageDependencies,
                                      typeName,
                                      major,
                                      minor,
                                      nullptr,
                                      std::move(resourceManagement)));
    }
#endif

    ModelPointer createModel(const TypeName &typeName,
                             std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {},
                             SL sl = {});

    const QUrl &fileUrl() const;
    SourceId fileUrlSourceId() const;
    void setFileUrl(const QUrl &url, SL sl = {});

#ifndef QDS_USE_PROJECTSTORAGE
    const MetaInfo metaInfo() const;
    MetaInfo metaInfo();
    void setMetaInfo(const MetaInfo &metaInfo);
#endif

    Module module(Utils::SmallStringView moduleName, Storage::ModuleKind moduleKind, SL sl = {}) const;
    SmallModuleIds<128> moduleIdsStartsWith(Utils::SmallStringView startsWith,
                                            Storage::ModuleKind kind,
                                            SL sl = {}) const;
    NodeMetaInfo metaInfo(Utils::SmallStringView typeName,
                          int majorVersion = -1,
                          int minorVersion = -1,
                          SL sl = {}) const;
    NodeMetaInfo metaInfo(Module module,
                          Utils::SmallStringView typeName,
                          Storage::Version version = Storage::Version{},
                          SL sl = {}) const;
    bool hasNodeMetaInfo(const TypeName &typeName,
                         int majorVersion = -1,
                         int minorVersion = -1,
                         SL sl = {}) const;

    NodeMetaInfo boolMetaInfo() const;
    NodeMetaInfo doubleMetaInfo() const;
    NodeMetaInfo floatMetaInfo() const;
    NodeMetaInfo fontMetaInfo() const;
    NodeMetaInfo qmlQtObjectMetaInfo() const;
    NodeMetaInfo qtQmlConnectionsMetaInfo() const;
    NodeMetaInfo qtQmlModelsListModelMetaInfo() const;
    NodeMetaInfo qtQmlModelsListElementMetaInfo() const;
    NodeMetaInfo qtQmlXmlListModelXmlListModelRoleMetaInfo() const;
    NodeMetaInfo qtQuick3DBakedLightmapMetaInfo() const;
    NodeMetaInfo qtQuick3DDefaultMaterialMetaInfo() const;
    NodeMetaInfo qtQuick3DDirectionalLightMetaInfo() const;
    NodeMetaInfo qtQuick3DLightMetaInfo() const;
    NodeMetaInfo qtQuick3DMaterialMetaInfo() const;
    NodeMetaInfo qtQuick3DModelMetaInfo() const;
    NodeMetaInfo qtQuick3DNodeMetaInfo() const;
    NodeMetaInfo qtQuick3DObject3DMetaInfo() const;
    NodeMetaInfo qtQuick3DCameraMetaInfo() const;
    NodeMetaInfo qtQuick3DOrthographicCameraMetaInfo() const;
    NodeMetaInfo qtQuick3DParticles3DSpriteParticle3DMetaInfo() const;
    NodeMetaInfo qtQuick3DPerspectiveCameraMetaInfo() const;
    NodeMetaInfo qtQuick3DPointLightMetaInfo() const;
    NodeMetaInfo qtQuick3DPrincipledMaterialMetaInfo() const;
    NodeMetaInfo qtQuick3DSceneEnvironmentMetaInfo() const;
    NodeMetaInfo qtQuick3DSpotLightMetaInfo() const;
    NodeMetaInfo qtQuick3DTextureMetaInfo() const;
    NodeMetaInfo qtQuick3DTextureInputMetaInfo() const;
    NodeMetaInfo qtQuick3DView3DMetaInfo() const;
    NodeMetaInfo qtQuickBorderImageMetaInfo() const;
    NodeMetaInfo qtQuickTemplatesLabelMetaInfo() const;
    NodeMetaInfo qtQuickTemplatesTextAreaMetaInfo() const;
    NodeMetaInfo qtQuickGradientMetaInfo() const;
    NodeMetaInfo qtQuickImageMetaInfo() const;
    NodeMetaInfo qtQuickItemMetaInfo() const;
    NodeMetaInfo qtQuickPropertyAnimationMetaInfo() const;
    NodeMetaInfo qtQuickPropertyChangesMetaInfo() const;
    NodeMetaInfo qtQuickRectangleMetaInfo() const;
    NodeMetaInfo qtQuickShapesShapeMetaInfo() const;
    NodeMetaInfo qtQuickStateGroupMetaInfo() const;
    NodeMetaInfo qtQuickTextEditMetaInfo() const;
    NodeMetaInfo qtQuickTextMetaInfo() const;
    NodeMetaInfo qtQuickTimelineKeyframeGroupMetaInfo() const;
    NodeMetaInfo qtQuickTimelineTimelineMetaInfo() const;
    NodeMetaInfo qtQuickTransistionMetaInfo() const;
    NodeMetaInfo qtQuickWindowMetaInfo() const;
    NodeMetaInfo qtQuickDialogsAbstractDialogMetaInfo() const;
    NodeMetaInfo qtQuickTemplatesPopupMetaInfo() const;
    NodeMetaInfo vector2dMetaInfo() const;
    NodeMetaInfo vector3dMetaInfo() const;
    NodeMetaInfo vector4dMetaInfo() const;

#ifdef QDS_USE_PROJECTSTORAGE
    SmallNodeMetaInfos<256> metaInfosForModule(Module module, SL sl = {}) const;
    SmallNodeMetaInfos<256> singletonMetaInfos(SL sl = {}) const;
#endif

    QList<ItemLibraryEntry> itemLibraryEntries(SL sl = {}) const;
    QList<ItemLibraryEntry> directoryImportsItemLibraryEntries(SL sl = {}) const;
    QList<ItemLibraryEntry> allItemLibraryEntries(SL sl = {}) const;

    void attachView(AbstractView *view, SL sl = {});
    void detachView(AbstractView *view, ViewNotification emitDetachNotify = NotifyView, SL sl = {});

    QList<ModelNode> allModelNodesUnordered(SL sl = {});
    ModelNode rootModelNode(SL sl = {});

    ModelNode modelNodeForId(const QString &id, SL sl = {});
    QHash<QStringView, ModelNode> idModelNodeDict(SL sl = {});

    ModelNode createModelNode(const TypeName &typeName, SL sl = {});
    void changeRootNodeType(const TypeName &type, SL sl = {});

    void removeModelNodes(ModelNodes nodes,
                          BypassModelResourceManagement = BypassModelResourceManagement::No,
                          SL sl = {});
    void removeProperties(AbstractProperties properties,
                          BypassModelResourceManagement = BypassModelResourceManagement::No,
                          SL sl = {});

    // Editing sub-components:

    // Imports:
    const Imports &imports() const;
    Imports possibleImports(SL sl = {}) const;
    Imports usedImports() const;
    void changeImports(Imports importsToBeAdded, Imports importsToBeRemoved, SL sl = {});
    void setImports(Imports imports, SL sl = {});

#ifndef QDS_USE_PROJECTSTORAGE
    void setPossibleImports(Imports possibleImports);
    void setUsedImports(Imports usedImports);
#endif
    bool hasImport(const Import &import,
                   bool ignoreAlias = true,
                   bool allowHigherVersion = false,
                   SL sl = {}) const;
    bool isImportPossible(const Import &import,
                          bool ignoreAlias = true,
                          bool allowHigherVersion = false,
                          SL sl = {}) const;
    QStringList importPaths(SL sl = {}) const;
    Import highestPossibleImport(const QString &importPath, SL sl = {});

    ModuleIds moduleIds(SL sl = {}) const;

    Storage::Info::ExportedTypeName exportedTypeNameForMetaInfo(const NodeMetaInfo &metaInfo,
                                                                SL sl = {}) const;

    RewriterView *rewriterView() const;
    void setRewriterView(RewriterView *rewriterView, SL sl = {});

    const AbstractView *nodeInstanceView() const;
    void setNodeInstanceView(AbstractView *nodeInstanceView, SL sl = {});

    Model *metaInfoProxyModel() const;

    void setDocumentMessages(const QList<DocumentMessage> &errors,
                             const QList<DocumentMessage> &warnings,
                             SL sl = {});

    QList<ModelNode> selectedNodes(AbstractView *view, SL sl = {}) const;
    void setSelectedModelNodes(Utils::span<const ModelNode> selectedNodes, SL sl = {});

    void clearMetaInfoCache(SL sl = {});

    bool hasId(const QString &id) const;
    bool hasImport(const QString &importUrl, SL sl = {}) const;

    QString generateNewId(const QString &prefixName,
                          const QString &fallbackPrefix = "element",
                          SL sl = {}) const;

    void startDrag(std::unique_ptr<QMimeData> mimeData,
                   const QPixmap &icon,
                   QWidget *sourceWidget,
                   SL sl = {});
    void endDrag(SL sl = {});

    void setCurrentStateNode(const ModelNode &node, SL sl = {});
    ModelNode currentStateNode(AbstractView *view = nullptr, SL sl = {});

    void setCurrentTimelineNode(const ModelNode &timeline, SL sl = {});

    NotNullPointer<const ProjectStorageType> projectStorage() const;
    const PathCacheType &pathCache() const;
    PathCacheType &pathCache();
    ProjectStorageTriggerUpdateInterface &projectStorageTriggerUpdate() const;

    ProjectStorageDependencies projectStorageDependencies() const;

    void emitInstancePropertyChange(AbstractView *view,
                                    Utils::span<const QPair<ModelNode, PropertyName>> properties,
                                    SL sl = {});
    void emitInstanceErrorChange(AbstractView *view, Utils::span<const qint32> instanceIds, SL sl = {});
    void emitInstancesCompleted(AbstractView *view, Utils::span<const ModelNode> nodes, SL sl = {});
    void emitInstanceInformationsChange(AbstractView *view,
                                        const QMultiHash<ModelNode, InformationName> &informationChangeHash,
                                        SL sl = {});
    void emitInstancesRenderImageChanged(AbstractView *view,
                                         Utils::span<const ModelNode> nodes,
                                         SL sl = {});
    void emitInstancesPreviewImageChanged(AbstractView *view,
                                          Utils::span<const ModelNode> nodes,
                                          SL sl = {});
    void emitInstancesChildrenChanged(AbstractView *view,
                                      Utils::span<const ModelNode> nodes,
                                      SL sl = {});
    void emitInstanceToken(AbstractView *view,
                           const QString &token,
                           int number,
                           const QVector<ModelNode> &nodeVector,
                           SL sl = {});
    void emitRenderImage3DChanged(AbstractView *view, const QImage &image, SL sl = {});
    void emitUpdateActiveScene3D(AbstractView *view, const QVariantMap &sceneState, SL sl = {});
    void emitModelNodelPreviewPixmapChanged(AbstractView *view,
                                            const ModelNode &node,
                                            const QPixmap &pixmap,
                                            const QByteArray &requestId,
                                            SL sl = {});
    void emitImport3DSupportChanged(AbstractView *view, const QVariantMap &supportMap, SL sl = {});
    void emitNodeAtPosResult(AbstractView *view,
                             const ModelNode &modelNode,
                             const QVector3D &pos3d,
                             SL sl = {});
    void emitView3DAction(View3DActionType type, const QVariant &value, SL sl = {});

    void emitDocumentMessage(const QList<DocumentMessage> &errors,
                             const QList<DocumentMessage> &warnings = {},
                             SL sl = {});
    void emitDocumentMessage(const QString &error, SL sl = {});
    void emitCustomNotification(AbstractView *view,
                                const QString &identifier,
                                Utils::span<const ModelNode> nodes = {},
                                const QList<QVariant> &data = {},
                                SL sl = {});

    void sendCustomNotificationTo(AbstractView *to,
                                  const CustomNotificationPackage &package,
                                  SL sl = {});
    void sendCustomNotificationToNodeInstanceView(const CustomNotificationPackage &package,
                                                  SL sl = {});

    void emitRewriterBeginTransaction(SL sl = {});
    void emitRewriterEndTransaction(SL sl = {});

private:
    template<Storage::Info::StaticString moduleName,
             Storage::Info::StaticString typeName,
             Storage::ModuleKind moduleKind = Storage::ModuleKind::QmlLibrary>
    NodeMetaInfo createNodeMetaInfo() const;
    void detachAllViews();

private:
    std::unique_ptr<Internal::ModelPrivate> d;
};

} // namespace QmlDesigner
