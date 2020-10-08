/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "iconrenderer.h"
#include "../editor3d/selectionboxgeometry.h"
#include "../editor3d/generalhelper.h"

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/qimage.h>
#include <QtGui/qguiapplication.h>
#include <QtCore/qtimer.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>

#ifdef QUICK3D_MODULE
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#endif

#include <private/qquickdesignersupportitems_p.h>

IconRenderer::IconRenderer(int size, const QString &filePath, const QString &source)
    : QObject(nullptr)
    , m_size(size)
    , m_filePath(filePath)
    , m_source(source)
{
}

void IconRenderer::setupRender()
{
    DesignerSupport::activateDesignerMode();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    DesignerSupport::activateDesignerWindowManager();
#endif

    m_quickView = new QQuickView;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QSurfaceFormat surfaceFormat = m_quickView->requestedFormat();
    surfaceFormat.setVersion(4, 1);
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    m_quickView->setFormat(surfaceFormat);

    DesignerSupport::createOpenGLContext(m_quickView);
#else
    m_quickView->setDefaultAlphaBuffer(true);
    m_quickView->setColor(Qt::transparent);
    m_ratio = m_quickView->devicePixelRatio();
    m_quickView->installEventFilter(this);
#endif

    QQmlComponent component(m_quickView->engine());
    component.loadUrl(QUrl::fromLocalFile(m_source));
    QObject *iconItem = component.create();

    if (iconItem) {
#ifdef QUICK3D_MODULE
        if (auto scene = qobject_cast<QQuick3DNode *>(iconItem)) {
            qmlRegisterType<QmlDesigner::Internal::SelectionBoxGeometry>("SelectionBoxGeometry", 1, 0, "SelectionBoxGeometry");
            QQmlComponent component(m_quickView->engine());
            component.loadUrl(QUrl("qrc:/qtquickplugin/mockfiles/IconRenderer3D.qml"));
            m_containerItem = qobject_cast<QQuickItem *>(component.create());
            DesignerSupport::setRootItem(m_quickView, m_containerItem);

            auto helper = new QmlDesigner::Internal::GeneralHelper();
            m_quickView->engine()->rootContext()->setContextProperty("_generalHelper", helper);

            m_contentItem = QQmlProperty::read(m_containerItem, "view3D").value<QQuickItem *>();
            auto view3D = qobject_cast<QQuick3DViewport *>(m_contentItem);
            view3D->setImportScene(scene);
            m_is3D = true;
        } else
#endif
        if (auto scene = qobject_cast<QQuickItem *>(iconItem)) {
            m_contentItem = scene;
            m_containerItem = new QQuickItem();
            m_containerItem->setSize(QSizeF(1024, 1024));
            DesignerSupport::setRootItem(m_quickView, m_containerItem);
            m_contentItem->setParentItem(m_containerItem);
        }

        if (m_containerItem && m_contentItem) {
            resizeContent(m_size);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QTimer::singleShot(0, this, &IconRenderer::createIcon);
#else
            m_quickView->show();
            m_quickView->lower();

            // Failsafe to exit eventually if window fails to expose
            QTimer::singleShot(10000, qGuiApp, &QGuiApplication::quit);
#endif
        } else {
            QTimer::singleShot(0, qGuiApp, &QGuiApplication::quit);
        }
    } else {
        QTimer::singleShot(0, qGuiApp, &QGuiApplication::quit);
    }
}

bool IconRenderer::eventFilter(QObject *watched, QEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (watched == m_quickView && event->type() == QEvent::Expose)
        QTimer::singleShot(0, this, &IconRenderer::createIcon);
#endif
    return false;
}

void IconRenderer::createIcon()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_designerSupport.refFromEffectItem(m_quickView->rootObject(), false);
#endif
    QQuickDesignerSupportItems::disableNativeTextRendering(m_quickView->rootObject());

#ifdef QUICK3D_MODULE
    if (m_is3D) {
        // Render once to make sure scene is up to date before we set up the selection box
        render({});
        QMetaObject::invokeMethod(m_containerItem, "setSceneToBox");
        int tries = 0;
        while (tries < 10) {
            ++tries;
            render({});
            QMetaObject::invokeMethod(m_containerItem, "fitAndHideBox");
        }
    }
#endif
    QFileInfo fi(m_filePath);

    // Render regular size image
    render(fi.absoluteFilePath());

    // Render @2x image
    resizeContent(m_size * 2);

    QString saveFile;
    saveFile = fi.absolutePath() + '/' + fi.completeBaseName() + "@2x";
    if (!fi.suffix().isEmpty())
        saveFile += '.' + fi.suffix();

    fi.absoluteDir().mkpath(".");

    render(saveFile);

    // Allow little time for file operations to finish
    QTimer::singleShot(1000, qGuiApp, &QGuiApplication::quit);
}

void IconRenderer::render(const QString &fileName)
{
    std::function<void (QQuickItem *)> updateNodesRecursive;
    updateNodesRecursive = [&updateNodesRecursive](QQuickItem *item) {
        const auto childItems = item->childItems();
        for (QQuickItem *childItem : childItems)
            updateNodesRecursive(childItem);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        DesignerSupport::updateDirtyNode(item);
#else
        if (item->flags() & QQuickItem::ItemHasContents)
            item->update();
#endif
    };
    updateNodesRecursive(m_quickView->rootObject());

    QRect rect(QPoint(), m_contentItem->size().toSize());
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QImage renderImage = m_designerSupport.renderImageForItem(m_quickView->rootObject(),
                                                              rect, rect.size());
#else
    QImage renderImage = m_quickView->grabWindow();
#endif
    if (!fileName.isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (m_ratio != 1.) {
            rect.setWidth(qRound(rect.size().width() * m_ratio));
            rect.setHeight(qRound(rect.size().height() * m_ratio));
        }
        renderImage = renderImage.copy(rect);
#endif
        QFileInfo fi(fileName);
        if (fi.suffix().isEmpty())
            renderImage.save(fileName, "PNG");
        else
            renderImage.save(fileName);
    }
}

void IconRenderer::resizeContent(int size)
{
    int theSize = size;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (m_ratio != 1.)
        theSize = qRound(qreal(size) / m_quickView->devicePixelRatio());
#endif
    m_contentItem->setSize(QSizeF(theSize, theSize));
    if (m_contentItem->width() > m_containerItem->width())
        m_containerItem->setWidth(m_contentItem->width());
    if (m_contentItem->height() > m_containerItem->height())
        m_containerItem->setHeight(m_contentItem->height());
}
