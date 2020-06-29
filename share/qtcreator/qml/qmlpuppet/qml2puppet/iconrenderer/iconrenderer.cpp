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
    DesignerSupport::activateDesignerWindowManager();

    m_quickView = new QQuickView;

    QSurfaceFormat surfaceFormat = m_quickView->requestedFormat();
    surfaceFormat.setVersion(4, 1);
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    m_quickView->setFormat(surfaceFormat);

    DesignerSupport::createOpenGLContext(m_quickView);

    QQmlComponent component(m_quickView->engine());
    component.loadUrl(QUrl::fromLocalFile(m_source));
    QObject *iconItem = component.create();

    if (iconItem) {
        QQuickItem *containerItem = nullptr;
        bool is3D = false;
#ifdef QUICK3D_MODULE
        if (auto scene = qobject_cast<QQuick3DNode *>(iconItem)) {
            qmlRegisterType<QmlDesigner::Internal::SelectionBoxGeometry>("SelectionBoxGeometry", 1, 0, "SelectionBoxGeometry");
            QQmlComponent component(m_quickView->engine());
            component.loadUrl(QUrl("qrc:/qtquickplugin/mockfiles/IconRenderer3D.qml"));
            containerItem = qobject_cast<QQuickItem *>(component.create());
            DesignerSupport::setRootItem(m_quickView, containerItem);

            auto helper = new QmlDesigner::Internal::GeneralHelper();
            m_quickView->engine()->rootContext()->setContextProperty("_generalHelper", helper);

            m_contentItem = QQmlProperty::read(containerItem, "view3D").value<QQuickItem *>();
            auto view3D = qobject_cast<QQuick3DViewport *>(m_contentItem);
            view3D->setImportScene(scene);
            is3D = true;
        } else
#endif
        if (auto scene = qobject_cast<QQuickItem *>(iconItem)) {
            m_contentItem = scene;
            containerItem = new QQuickItem();
            containerItem->setSize(QSizeF(1024, 1024));
            DesignerSupport::setRootItem(m_quickView, containerItem);
            m_contentItem->setParentItem(containerItem);
        }

        if (containerItem && m_contentItem) {
            m_contentItem->setSize(QSizeF(m_size, m_size));
            if (m_contentItem->width() > containerItem->width())
                containerItem->setWidth(m_contentItem->width());
            if (m_contentItem->height() > containerItem->height())
                containerItem->setHeight(m_contentItem->height());

            QTimer::singleShot(0, this, [this, is3D, containerItem]() {
                m_designerSupport.refFromEffectItem(m_quickView->rootObject(), false);
                QQuickDesignerSupportItems::disableNativeTextRendering(m_quickView->rootObject());

#ifdef QUICK3D_MODULE
                if (is3D) {
                    // Render once to make sure scene is up to date before we set up the selection box
                    render({});
                    QMetaObject::invokeMethod(containerItem, "setSceneToBox");
                    int tries = 0;
                    while (tries < 10) {
                        ++tries;
                        render({});
                        QMetaObject::invokeMethod(containerItem, "fitAndHideBox");
                    }
                }
#else
                Q_UNUSED(is3D)
                Q_UNUSED(containerItem)
#endif
                QFileInfo fi(m_filePath);

                // Render regular size image
                render(fi.absoluteFilePath());

                // Render @2x image
                m_contentItem->setSize(QSizeF(m_size * 2, m_size * 2));

                QString saveFile;
                saveFile = fi.absolutePath() + '/' + fi.completeBaseName() + "@2x";
                if (!fi.suffix().isEmpty())
                    saveFile += '.' + fi.suffix();

                fi.absoluteDir().mkpath(".");

                render(saveFile);

                // Allow little time for file operations to finish
                QTimer::singleShot(1000, qGuiApp, &QGuiApplication::quit);
            });
        } else {
            QTimer::singleShot(0, qGuiApp, &QGuiApplication::quit);
        }
    } else {
        QTimer::singleShot(0, qGuiApp, &QGuiApplication::quit);
    }
}

void IconRenderer::render(const QString &fileName)
{
    std::function<void (QQuickItem *)> updateNodesRecursive;
    updateNodesRecursive = [&updateNodesRecursive](QQuickItem *item) {
        const auto childItems = item->childItems();
        for (QQuickItem *childItem : childItems)
            updateNodesRecursive(childItem);
        DesignerSupport::updateDirtyNode(item);
    };
    updateNodesRecursive(m_quickView->rootObject());

    QRect rect(QPoint(), m_contentItem->size().toSize());
    QImage renderImage = m_designerSupport.renderImageForItem(m_quickView->rootObject(),
                                                              rect, rect.size());
    if (!fileName.isEmpty()) {
        QFileInfo fi(fileName);
        if (fi.suffix().isEmpty())
            renderImage.save(fileName, "PNG");
        else
            renderImage.save(fileName);
    }
}
