/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "itemlibraryinfo.h"
#include "import.h"

#include <utils/fancylineedit.h>
#include <utils/dropsupport.h>
#include <previewtooltip/previewtooltipbackend.h>
#include "itemlibraryassetsmodel.h"

#include <QFrame>
#include <QToolButton>
#include <QFileIconProvider>
#include <QQuickWidget>
#include <QQmlPropertyMap>
#include <QTimer>
#include <QPointF>

#include <memory>

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QShortcut;
QT_END_NAMESPACE

namespace Utils { class FileSystemWatcher; }

namespace QmlDesigner {

class MetaInfo;
class ItemLibraryEntry;
class Model;
class CustomFileSystemModel;

class ItemLibraryModel;
class ItemLibraryAssetsIconProvider;
class ItemLibraryAssetsModel;
class ItemLibraryAddImportModel;
class ItemLibraryResourceView;
class SynchronousImageCache;
class AsynchronousImageCache;
class ImageCacheCollector;

class ItemLibraryWidget : public QFrame
{
    Q_OBJECT

public:
    ItemLibraryWidget(AsynchronousImageCache &imageCache,
                      AsynchronousImageCache &asynchronousFontImageCache,
                      SynchronousImageCache &synchronousFontImageCache);
    ~ItemLibraryWidget();

    void setItemLibraryInfo(ItemLibraryInfo *itemLibraryInfo);
    QList<QToolButton *> createToolBarWidgets();

    static QString qmlSourcesPath();
    void clearSearchFilter();

    void delayedUpdateModel();
    void updateModel();
    void updatePossibleImports(const QList<Import> &possibleImports);
    void updateUsedImports(const QList<Import> &usedImports);

    void setResourcePath(const QString &resourcePath);
    void setModel(Model *model);
    void setFlowMode(bool b);
    static QPair<QString, QByteArray> getAssetTypeAndData(const QString &assetPath);

    inline static bool isHorizontalLayout = false;

    Q_INVOKABLE void startDragAndDrop(const QVariant &itemLibEntry, const QPointF &mousePos);
    Q_INVOKABLE void startDragAsset(const QStringList &assetPaths, const QPointF &mousePos);
    Q_INVOKABLE void removeImport(const QString &importUrl);
    Q_INVOKABLE void addImportForItem(const QString &importUrl);
    Q_INVOKABLE void handleTabChanged(int index);
    Q_INVOKABLE void handleAddModule();
    Q_INVOKABLE void handleAddAsset();
    Q_INVOKABLE void handleSearchfilterChanged(const QString &filterText);
    Q_INVOKABLE void handleAddImport(int index);
    Q_INVOKABLE bool isSearchActive() const;
    Q_INVOKABLE void handleFilesDrop(const QStringList &filesPaths);
    Q_INVOKABLE QSet<QString> supportedDropSuffixes();

signals:
    void itemActivated(const QString& itemName);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void reloadQmlSource();

    void addResources(const QStringList &files);
    void updateSearch();
    void handlePriorityImportsChanged();

    QTimer m_compressionTimer;
    QTimer m_assetCompressionTimer;
    QSize m_itemIconSize;

    SynchronousImageCache &m_fontImageCache;
    QPointer<ItemLibraryInfo> m_itemLibraryInfo;

    QPointer<ItemLibraryModel> m_itemLibraryModel;
    QPointer<ItemLibraryAddImportModel> m_itemLibraryAddImportModel;
    ItemLibraryAssetsIconProvider *m_assetsIconProvider = nullptr;
    Utils::FileSystemWatcher *m_fileSystemWatcher = nullptr;
    QPointer<ItemLibraryAssetsModel> m_assetsModel;

    QPointer<QStackedWidget> m_stackedWidget;

    QScopedPointer<QQuickWidget> m_headerWidget;
    QScopedPointer<QQuickWidget> m_addImportWidget;
    QScopedPointer<QQuickWidget> m_itemViewQuickWidget;
    QScopedPointer<QQuickWidget> m_assetsWidget;
    std::unique_ptr<PreviewTooltipBackend> m_previewTooltipBackend;
    std::unique_ptr<PreviewTooltipBackend> m_fontPreviewTooltipBackend;

    QShortcut *m_qmlSourceUpdateShortcut;
    AsynchronousImageCache &m_imageCache;
    QPointer<Model> m_model;
    QVariant m_itemToDrag;
    QStringList m_assetsToDrag;
    QPair<QString, QByteArray> m_assetToDragTypeAndData;
    bool m_updateRetry = false;
    QString m_filterText;
    QPoint m_dragStartPoint;

    inline static int HORIZONTAL_LAYOUT_WIDTH_LIMIT = 600;
};

} // namespace QmlDesigner
