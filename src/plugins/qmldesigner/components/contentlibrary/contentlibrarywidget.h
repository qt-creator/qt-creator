// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "createtexture.h"

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

    Q_PROPERTY(bool hasQuick3DImport READ hasQuick3DImport  NOTIFY hasQuick3DImportChanged)
    Q_PROPERTY(bool hasMaterialLibrary READ hasMaterialLibrary NOTIFY hasMaterialLibraryChanged)

public:
    ContentLibraryWidget();

    QList<QToolButton *> createToolBarWidgets();

    static QString qmlSourcesPath();
    void clearSearchFilter();

    bool hasQuick3DImport() const;
    void setHasQuick3DImport(bool b);

    bool hasMaterialLibrary() const;
    void setHasMaterialLibrary(bool b);

    Q_INVOKABLE void handleSearchFilterChanged(const QString &filterText);

    void setMaterialsModel(QPointer<ContentLibraryMaterialsModel> newMaterialsModel);

    QPointer<ContentLibraryMaterialsModel> materialsModel() const;
    QPointer<ContentLibraryTexturesModel> texturesModel() const;
    QPointer<ContentLibraryTexturesModel> environmentsModel() const;

    Q_INVOKABLE void startDragMaterial(QmlDesigner::ContentLibraryMaterial *mat, const QPointF &mousePos);
    Q_INVOKABLE void startDragTexture(QmlDesigner::ContentLibraryTexture *tex, const QPointF &mousePos);
    Q_INVOKABLE void addImage(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void addTexture(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void addLightProbe(QmlDesigner::ContentLibraryTexture *tex);
    Q_INVOKABLE void updateSceneEnvState();

signals:
    void bundleMaterialDragStarted(QmlDesigner::ContentLibraryMaterial *bundleMat);
    void bundleTextureDragStarted(QmlDesigner::ContentLibraryTexture *bundleTex);
    void addTextureRequested(const QString texPath, QmlDesigner::AddTextureMode mode);
    void updateSceneEnvStateRequested();
    void hasQuick3DImportChanged();
    void hasMaterialLibraryChanged();

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
    ContentLibraryTexture *m_textureToDrag = nullptr;
    QPoint m_dragStartPoint;

    bool m_hasMaterialLibrary = false;
    bool m_hasQuick3DImport = false;
};

} // namespace QmlDesigner
