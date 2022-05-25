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

#include "itemlibraryinfo.h"
#include "import.h"
#include "modelnode.h"

#include <utils/fancylineedit.h>
#include <utils/dropsupport.h>

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

namespace QmlDesigner {

class MaterialBrowserModel;
class PreviewImageProvider;

class MaterialBrowserWidget : public QFrame
{
    Q_OBJECT

public:
    MaterialBrowserWidget();
    ~MaterialBrowserWidget() = default;

    QList<QToolButton *> createToolBarWidgets();

    static QString qmlSourcesPath();
    void clearSearchFilter();

    QPointer<MaterialBrowserModel> materialBrowserModel() const;
    void updateMaterialPreview(const ModelNode &node, const QPixmap &pixmap);

    Q_INVOKABLE void handleSearchfilterChanged(const QString &filterText);

    QQuickWidget *quickWidget() const;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reloadQmlSource();
    void updateSearch();

    QPointer<MaterialBrowserModel> m_materialBrowserModel;
    QScopedPointer<QQuickWidget> m_quickWidget;

    QShortcut *m_qmlSourceUpdateShortcut = nullptr;
    PreviewImageProvider *m_previewImageProvider = nullptr;

    QString m_filterText;
};

} // namespace QmlDesigner
