// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QFrame>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QShortcut;
class QToolButton;
class QQuickWidget;
QT_END_NAMESPACE

namespace QmlDesigner {

class ContentLibraryTexture;
class ContentLibraryMaterial;
class ContentLibraryTexturesModel;
class ContentLibraryMaterialsModel;

class ContentLibraryWidget : public QFrame
{
    Q_OBJECT

public:
    ContentLibraryWidget();

    QList<QToolButton *> createToolBarWidgets();

    static QString qmlSourcesPath();
    void clearSearchFilter();

    Q_INVOKABLE void handleSearchFilterChanged(const QString &filterText);

    void setMaterialsModel(QPointer<ContentLibraryMaterialsModel> newMaterialsModel);

    QPointer<ContentLibraryMaterialsModel> materialsModel() const;

    Q_INVOKABLE void startDragMaterial(QmlDesigner::ContentLibraryMaterial *mat, const QPointF &mousePos);
    Q_INVOKABLE void startDragTexture(QmlDesigner::ContentLibraryTexture *tex, const QPointF &mousePos);
    Q_INVOKABLE void addImage(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void addTexture(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void addEnv(QmlDesigner::ContentLibraryTexture *tex);

    enum class AddTextureMode { Image, Texture, Environment };

signals:
    void bundleMaterialDragStarted(QmlDesigner::ContentLibraryMaterial *bundleMat);
    void draggedMaterialChanged();
    void addTextureRequested(const QString texPath, QmlDesigner::ContentLibraryWidget::AddTextureMode mode);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reloadQmlSource();
    void updateSearch();
    QString findTextureBundlePath();

    QScopedPointer<QQuickWidget> m_quickWidget;
    QPointer<ContentLibraryMaterialsModel> m_materialsModel;
    QPointer<ContentLibraryTexturesModel> m_texturesModel;
    QPointer<ContentLibraryTexturesModel> m_environmentsModel;

    QShortcut *m_qmlSourceUpdateShortcut = nullptr;

    QString m_filterText;

    ContentLibraryMaterial *m_materialToDrag = nullptr;
    ContentLibraryMaterial *m_draggedMaterial = nullptr;
    ContentLibraryTexture *m_textureToDrag = nullptr;
    QPoint m_dragStartPoint;
};

} // namespace QmlDesigner
