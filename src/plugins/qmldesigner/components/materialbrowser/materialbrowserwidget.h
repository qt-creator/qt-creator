// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelnode.h"

#include <coreplugin/icontext.h>

#include <QFrame>

QT_BEGIN_NAMESPACE
class QPointF;
class QShortcut;
class QToolButton;
QT_END_NAMESPACE

class StudioQuickWidget;

namespace QmlDesigner {

class AssetImageProvider;
class MaterialBrowserView;
class MaterialBrowserModel;
class MaterialBrowserTexturesModel;
class PreviewImageProvider;

class MaterialBrowserWidget : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(bool materialSectionFocused MEMBER m_materialSectionFocused NOTIFY materialSectionFocusedChanged)

    // Needed for a workaround for a bug where after drag-n-dropping an item, the ScrollView scrolls to a random position
    Q_PROPERTY(bool isDragging MEMBER m_isDragging NOTIFY isDraggingChanged)

public:
    MaterialBrowserWidget(class AsynchronousImageCache &imageCache, MaterialBrowserView *view);
    ~MaterialBrowserWidget() = default;

    QList<QToolButton *> createToolBarWidgets();
    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    static QString qmlSourcesPath();
    void clearSearchFilter();

    QPointer<MaterialBrowserModel> materialBrowserModel() const;
    QPointer<MaterialBrowserTexturesModel> materialBrowserTexturesModel() const;
    void updateMaterialPreview(const ModelNode &node, const QPixmap &pixmap);
    void deleteSelectedItem();

    Q_INVOKABLE void handleSearchFilterChanged(const QString &filterText);
    Q_INVOKABLE void startDragMaterial(int index, const QPointF &mousePos);
    Q_INVOKABLE void startDragTexture(int index, const QPointF &mousePos);
    Q_INVOKABLE void acceptBundleMaterialDrop();
    Q_INVOKABLE bool hasAcceptableAssets(const QList<QUrl> &urls);
    Q_INVOKABLE void acceptBundleTextureDrop();
    Q_INVOKABLE void acceptBundleTextureDropOnMaterial(int matIndex, const QUrl &bundleTexPath);
    Q_INVOKABLE void acceptAssetsDrop(const QList<QUrl> &urls);
    Q_INVOKABLE void acceptAssetsDropOnMaterial(int matIndex, const QList<QUrl> &urls);
    Q_INVOKABLE void acceptTextureDropOnMaterial(int matIndex, const QString &texId);
    Q_INVOKABLE void focusMaterialSection(bool focusMatSec);

    StudioQuickWidget *quickWidget() const;

    void clearPreviewCache();

    QSize sizeHint() const override;

signals:
    void materialSectionFocusedChanged();
    void isDraggingChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reloadQmlSource();
    void updateSearch();

    void setIsDragging(bool val);

    QPointer<MaterialBrowserView>  m_materialBrowserView;
    QPointer<MaterialBrowserModel> m_materialBrowserModel;
    QPointer<MaterialBrowserTexturesModel> m_materialBrowserTexturesModel;
    QScopedPointer<StudioQuickWidget> m_quickWidget;

    QShortcut *m_qmlSourceUpdateShortcut = nullptr;
    PreviewImageProvider *m_previewImageProvider = nullptr;
    AssetImageProvider *m_textureImageProvider = nullptr;
    Core::IContext *m_context = nullptr;

    QString m_filterText;

    ModelNode m_materialToDrag;
    ModelNode m_textureToDrag;
    QPoint m_dragStartPoint;

    bool m_materialSectionFocused = true;
    bool m_isDragging = false;
};

} // namespace QmlDesigner
