// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "createtexture.h"
#include "previewtooltipbackend.h"

#include <QFrame>
#include <QQmlPropertyMap>
#include <QQuickWidget>

#include <memory>

QT_BEGIN_NAMESPACE
class QPointF;
class QShortcut;
class QToolButton;
QT_END_NAMESPACE

namespace Utils {
    class QtcProcess;
}

namespace QmlDesigner {

class MetaInfo;
class Model;

class AssetsLibraryIconProvider;
class AssetsLibraryModel;
class SynchronousImageCache;
class AsynchronousImageCache;
class ImageCacheCollector;

class AssetsLibraryWidget : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(bool hasMaterialLibrary READ hasMaterialLibrary NOTIFY hasMaterialLibraryChanged)

    // Needed for a workaround for a bug where after drag-n-dropping an item, the ScrollView scrolls to a random position
    Q_PROPERTY(bool isDragging MEMBER m_isDragging NOTIFY isDraggingChanged)

public:
    AssetsLibraryWidget(AsynchronousImageCache &asynchronousFontImageCache,
                        SynchronousImageCache &synchronousFontImageCache);
    ~AssetsLibraryWidget() = default;

    QList<QToolButton *> createToolBarWidgets();

    static QString qmlSourcesPath();
    void clearSearchFilter();

    void delayedUpdateModel();
    void updateModel();

    void setResourcePath(const QString &resourcePath);
    void setModel(Model *model);
    static QPair<QString, QByteArray> getAssetTypeAndData(const QString &assetPath);

    bool hasMaterialLibrary() const;
    void setHasMaterialLibrary(bool enable);

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
    Q_INVOKABLE bool qtVersionIs6_4() const;
    Q_INVOKABLE void invalidateThumbnail(const QString &id);
    Q_INVOKABLE QSize imageSize(const QString &id);
    Q_INVOKABLE QString assetFileSize(const QString &id);
    Q_INVOKABLE bool assetIsImage(const QString &id);

    Q_INVOKABLE void addTextures(const QStringList &filePaths);
    Q_INVOKABLE void addLightProbe(const QString &filePaths);
    Q_INVOKABLE void updateHasMaterialLibrary();

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
    void addTexturesRequested(const QStringList &filePaths, QmlDesigner::AddTextureMode mode);
    void hasMaterialLibraryUpdateRequested();
    void hasMaterialLibraryChanged();
    void isDraggingChanged();
    void endDrag();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reloadQmlSource();

    void addResources(const QStringList &files);
    void updateSearch();
    void setIsDragging(bool val);

    QSize m_itemIconSize;

    SynchronousImageCache &m_fontImageCache;

    AssetsLibraryIconProvider *m_assetsIconProvider = nullptr;
    AssetsLibraryModel *m_assetsModel = nullptr;

    QScopedPointer<QQuickWidget> m_assetsWidget;
    std::unique_ptr<PreviewTooltipBackend> m_fontPreviewTooltipBackend;

    QShortcut *m_qmlSourceUpdateShortcut = nullptr;
    QPointer<Model> m_model;
    QStringList m_assetsToDrag;
    bool m_updateRetry = false;
    QString m_filterText;
    QPoint m_dragStartPoint;
    bool m_hasMaterialLibrary = false;
    bool m_isDragging = false;
};

} // namespace QmlDesigner
