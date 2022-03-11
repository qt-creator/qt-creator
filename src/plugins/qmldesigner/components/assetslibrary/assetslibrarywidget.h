/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <previewtooltip/previewtooltipbackend.h>
#include "assetslibrarymodel.h"

#include <QFrame>
#include <QToolButton>
#include <QFileIconProvider>
#include <QQuickWidget>
#include <QQmlPropertyMap>
#include <QTimer>
#include <QPointF>

#include <memory>

QT_BEGIN_NAMESPACE
class QShortcut;
QT_END_NAMESPACE

namespace Utils { class FileSystemWatcher; }

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
    AssetsLibraryWidget(AsynchronousImageCache &imageCache,
                        AsynchronousImageCache &asynchronousFontImageCache,
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
    Q_INVOKABLE void handleSearchfilterChanged(const QString &filterText);
    Q_INVOKABLE void handleExtFilesDrop(const QStringList &filesPaths, const QString &targetDirPath);
    Q_INVOKABLE QSet<QString> supportedDropSuffixes();

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
    AsynchronousImageCache &m_imageCache;
    QPointer<Model> m_model;
    QStringList m_assetsToDrag;
    bool m_updateRetry = false;
    QString m_filterText;
    QPoint m_dragStartPoint;
};

} // namespace QmlDesigner
