// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "createtexture.h"

#include <modelfwd.h>

#include <utils/uniqueobjectptr.h>

#include <QFrame>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QDir;
class QShortcut;
class QToolButton;
QT_END_NAMESPACE

class StudioQuickWidget;

namespace QmlDesigner {

class ContentLibraryBundleImporter;
class ContentLibraryEffectsModel;
class ContentLibraryIconProvider;
class ContentLibraryItem;
class ContentLibraryMaterial;
class ContentLibraryMaterialsModel;
class ContentLibraryTexture;
class ContentLibraryTexturesModel;
class ContentLibraryUserModel;

class ContentLibraryWidget : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(bool hasQuick3DImport READ hasQuick3DImport NOTIFY hasQuick3DImportChanged)
    Q_PROPERTY(bool hasMaterialLibrary READ hasMaterialLibrary NOTIFY hasMaterialLibraryChanged)
    Q_PROPERTY(bool hasActive3DScene READ hasActive3DScene WRITE setHasActive3DScene NOTIFY hasActive3DSceneChanged)
    Q_PROPERTY(bool isQt6Project READ isQt6Project NOTIFY isQt6ProjectChanged)
    Q_PROPERTY(bool importerRunning READ importerRunning WRITE setImporterRunning NOTIFY importerRunningChanged)
    Q_PROPERTY(bool hasModelSelection READ hasModelSelection NOTIFY hasModelSelectionChanged)

    // Needed for a workaround for a bug where after drag-n-dropping an item, the ScrollView scrolls to a random position
    Q_PROPERTY(bool isDragging MEMBER m_isDragging NOTIFY isDraggingChanged)

public:
    ContentLibraryWidget();
    ~ContentLibraryWidget();

    QList<QToolButton *> createToolBarWidgets();

    static QString qmlSourcesPath();
    void clearSearchFilter();

    bool hasQuick3DImport() const;
    void setHasQuick3DImport(bool b);

    bool hasMaterialLibrary() const;
    void setHasMaterialLibrary(bool b);

    bool hasActive3DScene() const;
    void setHasActive3DScene(bool b);

    bool isQt6Project() const;
    void setIsQt6Project(bool b);

    bool importerRunning() const;
    void setImporterRunning(bool b);

    bool hasModelSelection() const;
    void setHasModelSelection(bool b);

    void setMaterialsModel(QPointer<ContentLibraryMaterialsModel> newMaterialsModel);
    void updateImportedState(const QString &bundleId);

    QPointer<ContentLibraryMaterialsModel> materialsModel() const;
    QPointer<ContentLibraryTexturesModel> texturesModel() const;
    QPointer<ContentLibraryTexturesModel> environmentsModel() const;
    QPointer<ContentLibraryEffectsModel> effectsModel() const;
    QPointer<ContentLibraryUserModel> userModel() const;

    Q_INVOKABLE void handleSearchFilterChanged(const QString &filterText);
    Q_INVOKABLE void startDragItem(QmlDesigner::ContentLibraryItem *item, const QPointF &mousePos);
    Q_INVOKABLE void startDragMaterial(QmlDesigner::ContentLibraryMaterial *mat, const QPointF &mousePos);
    Q_INVOKABLE void startDragTexture(QmlDesigner::ContentLibraryTexture *tex, const QPointF &mousePos);
    Q_INVOKABLE void addImage(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void addTexture(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void addLightProbe(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void updateSceneEnvState();
    Q_INVOKABLE void markTextureUpdated(const QString &textureKey);

    QSize sizeHint() const override;

    ContentLibraryBundleImporter *importer() const;
    ContentLibraryIconProvider *iconProvider() const;

signals:
    void bundleItemDragStarted(QmlDesigner::ContentLibraryItem *item);
    void bundleMaterialDragStarted(QmlDesigner::ContentLibraryMaterial *bundleMat);
    void bundleTextureDragStarted(QmlDesigner::ContentLibraryTexture *bundleTex);
    void addTextureRequested(const QString texPath, QmlDesigner::AddTextureMode mode);
    void updateSceneEnvStateRequested();
    void hasQuick3DImportChanged();
    void hasMaterialLibraryChanged();
    void hasActive3DSceneChanged();
    void isDraggingChanged();
    void isQt6ProjectChanged();
    void importerRunningChanged();
    void hasModelSelectionChanged();
    void importBundle();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reloadQmlSource();
    void updateSearch();
    void setIsDragging(bool val);
    void loadTextureBundles();
    QVariantMap readTextureBundleJson();
    bool fetchTextureBundleJson(const QDir &bundleDir);
    bool fetchTextureBundleIcons(const QDir &bundleDir);
    void fetchNewTextureIcons(const QVariantMap &existingFiles, const QVariantMap &newFiles,
                              const QString &existingMetaFilePath, const QDir &bundleDir);
    std::tuple<QVariantMap, QVariantMap, QVariantMap> compareTextureMetaFiles(
        const QString &existingMetaFile, const QString downloadedMetaFile);
    QStringList saveNewTextures(const QDir &bundleDir, const QStringList &newFiles);
    void populateTextureBundleModels();
    void createImporter();

    Utils::UniqueObjectPtr<ContentLibraryIconProvider> m_iconProvider;
    Utils::UniqueObjectPtr<StudioQuickWidget> m_quickWidget;
    QPointer<ContentLibraryMaterialsModel> m_materialsModel;
    QPointer<ContentLibraryTexturesModel> m_texturesModel;
    QPointer<ContentLibraryTexturesModel> m_environmentsModel;
    QPointer<ContentLibraryEffectsModel> m_effectsModel;
    QPointer<ContentLibraryUserModel> m_userModel;

    ContentLibraryBundleImporter *m_importer = nullptr;
    QShortcut *m_qmlSourceUpdateShortcut = nullptr;

    QString m_filterText;

    ContentLibraryItem *m_itemToDrag = nullptr;
    ContentLibraryMaterial *m_materialToDrag = nullptr;
    ContentLibraryTexture *m_textureToDrag = nullptr;
    QPoint m_dragStartPoint;

    bool m_hasMaterialLibrary = false;
    bool m_hasActive3DScene = false;
    bool m_hasQuick3DImport = false;
    bool m_isDragging = false;
    bool m_isQt6Project = false;
    bool m_importerRunning = false;
    bool m_hasModelSelection = false;
    QString m_textureBundleUrl;
    QString m_bundlePath;
};

} // namespace QmlDesigner
