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

#include "modelnode.h"

#include <coreplugin/icontext.h>
#include <utils/dropsupport.h>
#include <utils/fancylineedit.h>

#include <QFileIconProvider>
#include <QFrame>
#include <QJsonObject>
#include <QPointF>
#include <QQmlPropertyMap>
#include <QQuickWidget>
#include <QTimer>
#include <QToolButton>

#include <memory>

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QShortcut;
QT_END_NAMESPACE

namespace QmlDesigner {

class BundleMaterial;
class MaterialBrowserView;
class MaterialBrowserModel;
class MaterialBrowserBundleModel;
class PreviewImageProvider;

class MaterialBrowserWidget : public QFrame
{
    Q_OBJECT

public:
    MaterialBrowserWidget(MaterialBrowserView *view);
    ~MaterialBrowserWidget() = default;

    QList<QToolButton *> createToolBarWidgets();
    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    static QString qmlSourcesPath();
    void clearSearchFilter();

    QPointer<MaterialBrowserModel> materialBrowserModel() const;
    QPointer<MaterialBrowserBundleModel> materialBrowserBundleModel() const;
    void updateMaterialPreview(const ModelNode &node, const QPixmap &pixmap);

    Q_INVOKABLE void handleSearchFilterChanged(const QString &filterText);
    Q_INVOKABLE void startDragMaterial(int index, const QPointF &mousePos);
    Q_INVOKABLE void startDragBundleMaterial(QmlDesigner::BundleMaterial *bundleMat, const QPointF &mousePos);

    QQuickWidget *quickWidget() const;

signals:
    void bundleMaterialDragStarted(QmlDesigner::BundleMaterial *bundleMat);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reloadQmlSource();
    void updateSearch();

    QPointer<MaterialBrowserView>  m_materialBrowserView;
    QPointer<MaterialBrowserModel> m_materialBrowserModel;
    QPointer<MaterialBrowserBundleModel> m_materialBrowserBundleModel;
    QScopedPointer<QQuickWidget> m_quickWidget;

    QShortcut *m_qmlSourceUpdateShortcut = nullptr;
    PreviewImageProvider *m_previewImageProvider = nullptr;
    Core::IContext *m_context = nullptr;

    QString m_filterText;

    ModelNode m_materialToDrag;
    BundleMaterial *m_bundleMaterialToDrag = nullptr;
    QPoint m_dragStartPoint;
};

} // namespace QmlDesigner
