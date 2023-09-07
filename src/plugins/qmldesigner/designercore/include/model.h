// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <documentmessage.h>
#include <model/modelresourcemanagementinterface.h>
#include <module.h>
#include <projectstorage/projectstoragefwd.h>
#include <projectstorage/projectstorageinfotypes.h>
#include <projectstorageids.h>

#include <QMimeData>
#include <QObject>
#include <QPair>

#include <import.h>

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
class NodeAnchors;
class AbstractProperty;
class RewriterView;
class NodeInstanceView;
class TextModifier;

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

    static ModelPointer create(const TypeName &typeName,
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
    static ModelPointer create(ProjectStorageDependencies m_projectStorageDependencies,
                               const TypeName &typeName,
                               int major = 1,
                               int minor = 1,
                               std::unique_ptr<ModelResourceManagementInterface> resourceManagement = {})
    {
        return ModelPointer(new Model(m_projectStorageDependencies,
                                      typeName,
                                      major,
                                      minor,
                                      nullptr,
                                      std::move(resourceManagement)));
    }

    QUrl fileUrl() const;
    SourceId fileUrlSourceId() const;
    void setFileUrl(const QUrl &url);

    const MetaInfo metaInfo() const;
    MetaInfo metaInfo();
    Module module(Utils::SmallStringView moduleName);
    NodeMetaInfo metaInfo(const TypeName &typeName, int majorVersion = -1, int minorVersion = -1) const;
    NodeMetaInfo metaInfo(Module module,
                          Utils::SmallStringView typeName,
                          Storage::Version version = Storage::Version{}) const;
    bool hasNodeMetaInfo(const TypeName &typeName, int majorVersion = -1, int minorVersion = -1) const;
    void setMetaInfo(const MetaInfo &metaInfo);

    NodeMetaInfo boolMetaInfo() const;
    NodeMetaInfo doubleMetaInfo() const;
    NodeMetaInfo flowViewFlowActionAreaMetaInfo() const;
    NodeMetaInfo flowViewFlowDecisionMetaInfo() const;
    NodeMetaInfo flowViewFlowItemMetaInfo() const;
    NodeMetaInfo flowViewFlowTransitionMetaInfo() const;
    NodeMetaInfo flowViewFlowWildcardMetaInfo() const;
    NodeMetaInfo fontMetaInfo() const;
    NodeMetaInfo qmlQtObjectMetaInfo() const;
    NodeMetaInfo qtQmlModelsListModelMetaInfo() const;
    NodeMetaInfo qtQmlModelsListElementMetaInfo() const;
    NodeMetaInfo qtQuick3DBakedLightmapMetaInfo() const;
    NodeMetaInfo qtQuick3DDefaultMaterialMetaInfo() const;
    NodeMetaInfo qtQuick3DMaterialMetaInfo() const;
    NodeMetaInfo qtQuick3DModelMetaInfo() const;
    NodeMetaInfo qtQuick3DNodeMetaInfo() const;
    NodeMetaInfo qtQuick3DPrincipledMaterialMetaInfo() const;
    NodeMetaInfo qtQuick3DTextureMetaInfo() const;
    NodeMetaInfo qtQuickConnectionsMetaInfo() const;
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
    const Imports &possibleImports() const;
    const Imports &usedImports() const;
    void changeImports(Imports importsToBeAdded, Imports importsToBeRemoved);
    void setPossibleImports(Imports possibleImports);
    void setUsedImports(Imports usedImports);
    bool hasImport(const Import &import, bool ignoreAlias = true, bool allowHigherVersion = false) const;
    bool isImportPossible(const Import &import, bool ignoreAlias = true, bool allowHigherVersion = false) const;
    QString pathForImport(const Import &import);
    QStringList importPaths() const;
    Import highestPossibleImport(const QString &importPath);

    RewriterView *rewriterView() const;
    void setRewriterView(RewriterView *rewriterView);

    const NodeInstanceView *nodeInstanceView() const;
    void setNodeInstanceView(NodeInstanceView *nodeInstanceView);

    Model *metaInfoProxyModel() const;

    void setDocumentMessages(const QList<DocumentMessage> &errors,
                             const QList<DocumentMessage> &warnings);

    QList<ModelNode> selectedNodes(AbstractView *view) const;

    void clearMetaInfoCache();

    bool hasId(const QString &id) const;
    bool hasImport(const QString &importUrl) const;

    QString generateNewId(const QString &prefixName,
                          const QString &fallbackPrefix = "element",
                          std::optional<std::function<bool(const QString &)>> isDuplicate = {}) const;
    QString generateIdFromName(const QString &name, const QString &fallbackId = "element") const;

    void setActive3DSceneId(qint32 sceneId);
    qint32 active3DSceneId() const;

    void startDrag(QMimeData *mimeData, const QPixmap &icon);
    void endDrag();

    NotNullPointer<const ProjectStorageType> projectStorage() const;
    const PathCacheType &pathCache() const;
    PathCacheType &pathCache();

private:
    template<const auto &moduleName, const auto &typeName>
    NodeMetaInfo createNodeMetaInfo() const;
    void detachAllViews();

private:
    std::unique_ptr<Internal::ModelPrivate> d;
};

} // namespace QmlDesigner
