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

#ifdef QDS_USE_PROJECTSTORAGE
#  define DEPRECATED_OLD_CREATE_MODELNODE \
      [[deprecated("Use unqualified type names and no versions!")]]
#else
#  define DEPRECATED_OLD_CREATE_MODELNODE
#endif

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

    Q_OBJECT

public:
    enum ViewNotification { NotifyView, DoNotNotifyView };

    Model(ProjectStorageDependencies projectStorageDependencies,
          const TypeName &type,
          int major = 1,
          int minor = 1,
          Model *metaInfoProxyModel = nullptr,
          std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {});
    Model(ProjectStorageDependencies projectStorageDependencies,
          Utils::SmallStringView typeName,
          Imports imports,
          const QUrl &fileUrl,
          std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {});
    Model(const TypeName &typeName,
          int major = 1,
          int minor = 1,
          Model *metaInfoProxyModel = nullptr,
          std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {});

    ~Model();

    DEPRECATED_OLD_CREATE_MODELNODE static ModelPointer create(
        const TypeName &typeName,
        int major = 1,
        int minor = 1,
        Model *metaInfoProxyModel = nullptr,
        std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {})
    {
        return ModelPointer(
            new Model(typeName, major, minor, metaInfoProxyModel, std::move(resourceManagement)));
    }

    static ModelPointer create(
        ProjectStorageDependencies projectStorageDependencies,
        Utils::SmallStringView typeName,
        Imports imports,
        const QUrl &fileUrl,
        std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {})
    {
        return ModelPointer(new Model(projectStorageDependencies,
                                      typeName,
                                      imports,
                                      fileUrl,
                                      std::move(resourceManagement)));
    }

    DEPRECATED_OLD_CREATE_MODELNODE static ModelPointer create(
        ProjectStorageDependencies projectStorageDependencies,
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

    ModelPointer createModel(const TypeName &typeName,
                             std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {});

    const QUrl &fileUrl() const;
    SourceId fileUrlSourceId() const;
    void setFileUrl(const QUrl &url);

#ifndef QDS_USE_PROJECTSTORAGE
    const MetaInfo metaInfo() const;
    MetaInfo metaInfo();
    void setMetaInfo(const MetaInfo &metaInfo);
#endif

    Module module(Utils::SmallStringView moduleName, Storage::ModuleKind moduleKind);
    NodeMetaInfo metaInfo(const TypeName &typeName, int majorVersion = -1, int minorVersion = -1) const;
    NodeMetaInfo metaInfo(Module module,
                          Utils::SmallStringView typeName,
                          Storage::Version version = Storage::Version{}) const;
    bool hasNodeMetaInfo(const TypeName &typeName, int majorVersion = -1, int minorVersion = -1) const;

    NodeMetaInfo boolMetaInfo() const;
    NodeMetaInfo doubleMetaInfo() const;
    NodeMetaInfo flowViewFlowActionAreaMetaInfo() const;
    NodeMetaInfo flowViewFlowDecisionMetaInfo() const;
    NodeMetaInfo flowViewFlowItemMetaInfo() const;
    NodeMetaInfo flowViewFlowTransitionMetaInfo() const;
    NodeMetaInfo flowViewFlowWildcardMetaInfo() const;
    NodeMetaInfo fontMetaInfo() const;
    NodeMetaInfo qmlQtObjectMetaInfo() const;
    NodeMetaInfo qtQmlConnectionsMetaInfo() const;
    NodeMetaInfo qtQmlModelsListModelMetaInfo() const;
    NodeMetaInfo qtQmlModelsListElementMetaInfo() const;
    NodeMetaInfo qtQmlXmlListModelXmlListModelRoleMetaInfo() const;
    NodeMetaInfo qtQuick3DBakedLightmapMetaInfo() const;
    NodeMetaInfo qtQuick3DDefaultMaterialMetaInfo() const;
    NodeMetaInfo qtQuick3DDirectionalLightMetaInfo() const;
    NodeMetaInfo qtQuick3DMaterialMetaInfo() const;
    NodeMetaInfo qtQuick3DModelMetaInfo() const;
    NodeMetaInfo qtQuick3DNodeMetaInfo() const;
    NodeMetaInfo qtQuick3DOrthographicCameraMetaInfo() const;
    NodeMetaInfo qtQuick3DPerspectiveCameraMetaInfo() const;
    NodeMetaInfo qtQuick3DPointLightMetaInfo() const;
    NodeMetaInfo qtQuick3DPrincipledMaterialMetaInfo() const;
    NodeMetaInfo qtQuick3DSpotLightMetaInfo() const;
    NodeMetaInfo qtQuick3DTextureMetaInfo() const;
    NodeMetaInfo qtQuickControlsTextAreaMetaInfo() const;
    NodeMetaInfo qtQuickImageMetaInfo() const;
    NodeMetaInfo qtQuickItemMetaInfo() const;
    NodeMetaInfo qtQuickPropertyAnimationMetaInfo() const;
    NodeMetaInfo qtQuickPropertyChangesMetaInfo() const;
    NodeMetaInfo qtQuickRectangleMetaInfo() const;
    NodeMetaInfo qtQuickStateGroupMetaInfo() const;
    NodeMetaInfo qtQuickTextEditMetaInfo() const;
    NodeMetaInfo qtQuickTextMetaInfo() const;
    NodeMetaInfo qtQuickTimelineKeyframeGroupMetaInfo() const;
    NodeMetaInfo qtQuickTimelineTimelineMetaInfo() const;
    NodeMetaInfo qtQuickTransistionMetaInfo() const;
    NodeMetaInfo vector2dMetaInfo() const;
    NodeMetaInfo vector3dMetaInfo() const;
    NodeMetaInfo vector4dMetaInfo() const;
    QVarLengthArray<NodeMetaInfo, 256> metaInfosForModule(Module module) const;

    QList<ItemLibraryEntry> itemLibraryEntries() const;

    void attachView(AbstractView *view);
    void detachView(AbstractView *view, ViewNotification emitDetachNotify = NotifyView);

    QList<ModelNode> allModelNodesUnordered();
    ModelNode rootModelNode();

    ModelNode modelNodeForId(const QString &id);
    QHash<QStringView, ModelNode> idModelNodeDict();

    ModelNode createModelNode(const TypeName &typeName);
    void changeRootNodeType(const TypeName &type);

    void removeModelNodes(ModelNodes nodes,
                          BypassModelResourceManagement = BypassModelResourceManagement::No);
    void removeProperties(AbstractProperties properties,
                          BypassModelResourceManagement = BypassModelResourceManagement::No);

    // Editing sub-components:

    // Imports:
    const Imports &imports() const;
    Imports possibleImports() const;
    Imports usedImports() const;
    void changeImports(Imports importsToBeAdded, Imports importsToBeRemoved);
#ifndef QDS_USE_PROJECTSTORAGE
    void setPossibleImports(Imports possibleImports);
#endif
#ifndef QDS_USE_PROJECTSTORAGE
    void setUsedImports(Imports usedImports);
#endif
    bool hasImport(const Import &import, bool ignoreAlias = true, bool allowHigherVersion = false) const;
    bool isImportPossible(const Import &import, bool ignoreAlias = true, bool allowHigherVersion = false) const;
    QStringList importPaths() const;
    Import highestPossibleImport(const QString &importPath);

    ModuleIds moduleIds() const;

    Storage::Info::ExportedTypeName exportedTypeNameForMetaInfo(const NodeMetaInfo &metaInfo) const;

    RewriterView *rewriterView() const;
    void setRewriterView(RewriterView *rewriterView);

    const AbstractView *nodeInstanceView() const;
    void setNodeInstanceView(AbstractView *nodeInstanceView);

    Model *metaInfoProxyModel() const;

    void setDocumentMessages(const QList<DocumentMessage> &errors,
                             const QList<DocumentMessage> &warnings);

    QList<ModelNode> selectedNodes(AbstractView *view) const;
    void setSelectedModelNodes(const QList<ModelNode> &selectedNodeList);

    void clearMetaInfoCache();

    bool hasId(const QString &id) const;
    bool hasImport(const QString &importUrl) const;

    QString generateNewId(const QString &prefixName, const QString &fallbackPrefix = "element") const;

    void startDrag(std::unique_ptr<QMimeData> mimeData, const QPixmap &icon, QWidget *sourceWidget);
    void endDrag();

    void setCurrentStateNode(const ModelNode &node);
    ModelNode currentStateNode(AbstractView *view = nullptr);

    void setCurrentTimelineNode(const ModelNode &timeline);

    NotNullPointer<const ProjectStorageType> projectStorage() const;
    const PathCacheType &pathCache() const;
    PathCacheType &pathCache();

    void emitInstancePropertyChange(AbstractView *view,
                                    const QList<QPair<ModelNode, PropertyName>> &propertyList);
    void emitInstanceErrorChange(AbstractView *view, const QVector<qint32> &instanceIds);
    void emitInstancesCompleted(AbstractView *view, const QVector<ModelNode> &nodeList);
    void emitInstanceInformationsChange(
        AbstractView *view, const QMultiHash<ModelNode, InformationName> &informationChangeHash);
    void emitInstancesRenderImageChanged(AbstractView *view, const QVector<ModelNode> &nodeList);
    void emitInstancesPreviewImageChanged(AbstractView *view, const QVector<ModelNode> &nodeList);
    void emitInstancesChildrenChanged(AbstractView *view, const QVector<ModelNode> &nodeList);
    void emitInstanceToken(AbstractView *view,
                           const QString &token,
                           int number,
                           const QVector<ModelNode> &nodeVector);
    void emitRenderImage3DChanged(AbstractView *view, const QImage &image);
    void emitUpdateActiveScene3D(AbstractView *view, const QVariantMap &sceneState);
    void emitModelNodelPreviewPixmapChanged(AbstractView *view,
                                            const ModelNode &node,
                                            const QPixmap &pixmap,
                                            const QByteArray &requestId);
    void emitImport3DSupportChanged(AbstractView *view, const QVariantMap &supportMap);
    void emitNodeAtPosResult(AbstractView *view, const ModelNode &modelNode, const QVector3D &pos3d);
    void emitView3DAction(View3DActionType type, const QVariant &value);

    void emitDocumentMessage(const QList<DocumentMessage> &errors,
                             const QList<DocumentMessage> &warnings = {});
    void emitDocumentMessage(const QString &error);
    void emitCustomNotification(AbstractView *view,
                                const QString &identifier,
                                const QList<ModelNode> &nodeList = {},
                                const QList<QVariant> &data = {});

    void sendCustomNotificationTo(AbstractView *to, const CustomNotificationPackage &package);
    void sendCustomNotificationToNodeInstanceView(const CustomNotificationPackage &package);

    void emitRewriterBeginTransaction();
    void emitRewriterEndTransaction();

private:
    template<const auto &moduleName, const auto &typeName>
    NodeMetaInfo createNodeMetaInfo() const;
    void detachAllViews();

private:
    std::unique_ptr<Internal::ModelPrivate> d;
};

} // namespace QmlDesigner
