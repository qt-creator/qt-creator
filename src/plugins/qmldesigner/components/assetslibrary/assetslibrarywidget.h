// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <previewtooltip/previewtooltipbackend.h>
#include "assetslibrarymodel.h"

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
    class FileSystemWatcher;
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
    ~AssetsLibraryWidget();

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
                                        const QString &targetDirPath = {});
    Q_INVOKABLE QSet<QString> supportedAssetSuffixes(bool complex);
    Q_INVOKABLE void openEffectMaker(const QString &filePath);

signals:
    void itemActivated(const QString &itemName);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reloadQmlSource();

    void addResources(const QStringList &files);
    void updateSearch();

    QTimer m_assetCompressionTimer;
    QSize m_itemIconSize;

    SynchronousImageCache &m_fontImageCache;

    AssetsLibraryIconProvider *m_assetsIconProvider = nullptr;
    Utils::FileSystemWatcher *m_fileSystemWatcher = nullptr;
    QPointer<AssetsLibraryModel> m_assetsModel;

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
