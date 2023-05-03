// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "createtexture.h"
#include "previewtooltipbackend.h"

#include <coreplugin/icontext.h>

#include <QFrame>
#include <QQmlPropertyMap>
#include <QQuickWidget>

#include <memory>

QT_BEGIN_NAMESPACE
class QPointF;
class QShortcut;
class QToolButton;
QT_END_NAMESPACE

class StudioQuickWidget;

namespace Utils {
    class Process;
}

namespace QmlDesigner {

class MetaInfo;
class Model;

class AssetsLibraryIconProvider;
class AssetsLibraryModel;
class AssetsLibraryView;
class SynchronousImageCache;
class AsynchronousImageCache;
class ImageCacheCollector;

class AssetsLibraryWidget : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(bool hasMaterialLibrary MEMBER m_hasMaterialLibrary NOTIFY hasMaterialLibraryChanged)
    Q_PROPERTY(bool hasSceneEnv MEMBER m_hasSceneEnv NOTIFY hasSceneEnvChanged)

    // Needed for a workaround for a bug where after drag-n-dropping an item, the ScrollView scrolls to a random position
    Q_PROPERTY(bool isDragging MEMBER m_isDragging NOTIFY isDraggingChanged)

public:
    AssetsLibraryWidget(AsynchronousImageCache &asynchronousFontImageCache,
                        SynchronousImageCache &synchronousFontImageCache, AssetsLibraryView *view);
    ~AssetsLibraryWidget() = default;

    QList<QToolButton *> createToolBarWidgets();
    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    static QString qmlSourcesPath();
    void clearSearchFilter();

    void delayedUpdateModel();
    void updateModel();

    void setResourcePath(const QString &resourcePath);
    static QPair<QString, QByteArray> getAssetTypeAndData(const QString &assetPath);

    void deleteSelectedAssets();

    Q_INVOKABLE void startDragAsset(const QStringList &assetPaths, const QPointF &mousePos);
    Q_INVOKABLE void handleAddAsset();
    Q_INVOKABLE void handleSearchFilterChanged(const QString &filterText);

    Q_INVOKABLE void handleExtFilesDrop(const QList<QUrl> &simpleFilePaths,
                                        const QList<QUrl> &complexFilePaths,
                                        const QString &targetDirPath);

    Q_INVOKABLE void emitExtFilesDrop(const QList<QUrl> &simpleFilePaths,
                                      const QList<QUrl> &complexFilePaths,
                                      const QString &targetDirPath = {});

    Q_INVOKABLE QSet<QString> supportedAssetSuffixes(bool complex);
    Q_INVOKABLE void openEffectMaker(const QString &filePath);
    Q_INVOKABLE int qtVersion() const;
    Q_INVOKABLE void invalidateThumbnail(const QString &id);
    Q_INVOKABLE QSize imageSize(const QString &id);
    Q_INVOKABLE QString assetFileSize(const QString &id);
    Q_INVOKABLE bool assetIsImageOrTexture(const QString &id);

    Q_INVOKABLE void addTextures(const QStringList &filePaths);
    Q_INVOKABLE void addLightProbe(const QString &filePaths);
    Q_INVOKABLE void updateContextMenuActionsEnableState();

    Q_INVOKABLE QString getUniqueEffectPath(const QString &parentFolder, const QString &effectName);
    Q_INVOKABLE bool createNewEffect(const QString &effectPath, bool openEffectMaker = true);

    Q_INVOKABLE bool canCreateEffects() const;

    Q_INVOKABLE void showInGraphicalShell(const QString &path);
    Q_INVOKABLE QString showInGraphicalShellMsg() const;

signals:
    void itemActivated(const QString &itemName);
    void extFilesDrop(const QList<QUrl> &simpleFilePaths,
                      const QList<QUrl> &complexFilePaths,
                      const QString &targetDirPath);
    void directoryCreated(const QString &path);
    void hasMaterialLibraryChanged();
    void hasSceneEnvChanged();
    void isDraggingChanged();
    void endDrag();
    void deleteSelectedAssetsRequested();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reloadQmlSource();

    void addResources(const QStringList &files, bool showDialog = true);
    void updateSearch();
    void setIsDragging(bool val);

    void setHasMaterialLibrary(bool enable);
    void setHasSceneEnv(bool b);

    QSize m_itemIconSize;

    SynchronousImageCache &m_fontImageCache;

    AssetsLibraryIconProvider *m_assetsIconProvider = nullptr;
    AssetsLibraryModel *m_assetsModel = nullptr;
    AssetsLibraryView *m_assetsView = nullptr;
    CreateTextures m_createTextures = nullptr;

    QScopedPointer<StudioQuickWidget> m_assetsWidget;
    std::unique_ptr<PreviewTooltipBackend> m_fontPreviewTooltipBackend;

    QShortcut *m_qmlSourceUpdateShortcut = nullptr;
    QStringList m_assetsToDrag;
    bool m_updateRetry = false;
    QString m_filterText;
    QPoint m_dragStartPoint;
    bool m_hasMaterialLibrary = false;
    bool m_hasSceneEnv = false;
    bool m_isDragging = false;
};

} // namespace QmlDesigner
