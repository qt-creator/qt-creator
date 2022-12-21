// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <previewtooltip/previewtooltipbackend.h>

#include "assetslibrarymodel.h"
#include "createtexture.h"

#include <QFileIconProvider>
#include <QFrame>
#include <QPointF>
#include <QQmlPropertyMap>
#include <QQuickWidget>
#include <QTimer>
#include <QToolButton>

#include <memory>

QT_BEGIN_NAMESPACE
class QShortcut;
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
    Q_INVOKABLE bool qtVersionIsAtLeast6_4() const;
    Q_INVOKABLE void invalidateThumbnail(const QString &id);
    Q_INVOKABLE void addTextures(const QStringList &filePaths);
    Q_INVOKABLE void addLightProbe(const QString &filePaths);

signals:
    void itemActivated(const QString &itemName);
    void extFilesDrop(const QList<QUrl> &simpleFilePaths,
                      const QList<QUrl> &complexFilePaths,
                      const QString &targetDirPath);
    void directoryCreated(const QString &path);
    void addTexturesRequested(const QStringList &filePaths, QmlDesigner::AddTextureMode mode);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reloadQmlSource();

    void addResources(const QStringList &files);
    void updateSearch();

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
};

} // namespace QmlDesigner
