// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "createtexture.h"

#include <QFrame>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QDir;
class QShortcut;
class QToolButton;
QT_END_NAMESPACE

class StudioQuickWidget;

namespace QmlDesigner {

class ContentLibraryEffect;
class ContentLibraryEffectsModel;
class ContentLibraryMaterial;
class ContentLibraryMaterialsModel;
class ContentLibraryTexture;
class ContentLibraryTexturesModel;

class ContentLibraryWidget : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(bool hasQuick3DImport READ hasQuick3DImport NOTIFY hasQuick3DImportChanged)
    Q_PROPERTY(bool hasMaterialLibrary READ hasMaterialLibrary NOTIFY hasMaterialLibraryChanged)
    Q_PROPERTY(bool hasActive3DScene READ hasActive3DScene WRITE setHasActive3DScene NOTIFY hasActive3DSceneChanged)
    Q_PROPERTY(bool isQt6Project READ isQt6Project NOTIFY isQt6ProjectChanged)

    // Needed for a workaround for a bug where after drag-n-dropping an item, the ScrollView scrolls to a random position
    Q_PROPERTY(bool isDragging MEMBER m_isDragging NOTIFY isDraggingChanged)

public:
    ContentLibraryWidget();

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

    Q_INVOKABLE void handleSearchFilterChanged(const QString &filterText);

    void setMaterialsModel(QPointer<ContentLibraryMaterialsModel> newMaterialsModel);

    QPointer<ContentLibraryMaterialsModel> materialsModel() const;
    QPointer<ContentLibraryTexturesModel> texturesModel() const;
    QPointer<ContentLibraryTexturesModel> environmentsModel() const;
    QPointer<ContentLibraryEffectsModel> effectsModel() const;

    Q_INVOKABLE void startDragEffect(QmlDesigner::ContentLibraryEffect *eff, const QPointF &mousePos);
    Q_INVOKABLE void startDragMaterial(QmlDesigner::ContentLibraryMaterial *mat, const QPointF &mousePos);
    Q_INVOKABLE void startDragTexture(QmlDesigner::ContentLibraryTexture *tex, const QPointF &mousePos);
    Q_INVOKABLE void addImage(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void addTexture(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void addLightProbe(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void updateSceneEnvState();
    Q_INVOKABLE void markTextureUpdated(const QString &textureKey);

    QSize sizeHint() const override;

signals:
    void bundleEffectDragStarted(QmlDesigner::ContentLibraryEffect *bundleEff);
    void bundleMaterialDragStarted(QmlDesigner::ContentLibraryMaterial *bundleMat);
    void bundleTextureDragStarted(QmlDesigner::ContentLibraryTexture *bundleTex);
    void addTextureRequested(const QString texPath, QmlDesigner::AddTextureMode mode);
    void updateSceneEnvStateRequested();
    void hasQuick3DImportChanged();
    void hasMaterialLibraryChanged();
    void hasActive3DSceneChanged();
    void isDraggingChanged();
    void isQt6ProjectChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reloadQmlSource();
    void updateSearch();
    void setIsDragging(bool val);
    QString findTextureBundlePath();
    void loadTextureBundle();
    QVariantMap readBundleMetadata();
    bool fetchTextureBundleMetadata(const QDir &bundleDir);
    bool fetchTextureBundleIcons(const QDir &bundleDir);
    void fetchNewTextureIcons(const QVariantMap &existingFiles, const QVariantMap &newFiles,
                              const QString &existingMetaFilePath, const QDir &bundleDir);
    std::tuple<QVariantMap, QVariantMap, QVariantMap> compareTextureMetaFiles(
        const QString &existingMetaFile, const QString downloadedMetaFile);
    QStringList saveNewTextures(const QDir &bundleDir, const QStringList &newFiles);

    QScopedPointer<StudioQuickWidget> m_quickWidget;
    QPointer<ContentLibraryMaterialsModel> m_materialsModel;
    QPointer<ContentLibraryTexturesModel> m_texturesModel;
    QPointer<ContentLibraryTexturesModel> m_environmentsModel;
    QPointer<ContentLibraryEffectsModel> m_effectsModel;

    QShortcut *m_qmlSourceUpdateShortcut = nullptr;

    QString m_filterText;

    ContentLibraryEffect *m_effectToDrag = nullptr;
    ContentLibraryMaterial *m_materialToDrag = nullptr;
    ContentLibraryTexture *m_textureToDrag = nullptr;
    QPoint m_dragStartPoint;

    bool m_hasMaterialLibrary = false;
    bool m_hasActive3DScene = false;
    bool m_hasQuick3DImport = false;
    bool m_isDragging = false;
    bool m_isQt6Project = false;
    QString m_baseUrl;
    QString m_texturesUrl;
    QString m_textureIconsUrl;
    QString m_environmentIconsUrl;
    QString m_environmentsUrl;
    QString m_downloadPath;
};

} // namespace QmlDesigner
