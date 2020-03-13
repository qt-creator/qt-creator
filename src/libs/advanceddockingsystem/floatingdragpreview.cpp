/****************************************************************************
**
** Copyright (C) 2020 Uwe Kindler
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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "floatingdragpreview.h"

#include "dockareawidget.h"
#include "dockcontainerwidget.h"
#include "dockmanager.h"
#include "dockoverlay.h"
#include "dockwidget.h"

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QPainter>

#include <iostream>

static Q_LOGGING_CATEGORY(adsLog, "qtc.qmldesigner.advanceddockingsystem", QtWarningMsg)

namespace ADS
{
    /**
     * Private data class (pimpl)
     */
    class FloatingDragPreviewPrivate
    {
    public:
        FloatingDragPreview *q;
        QWidget *m_content = nullptr;
        DockAreaWidget *m_contentSourceArea = nullptr;
        DockContainerWidget *m_contenSourceContainer = nullptr;
        QPoint m_dragStartMousePosition;
        DockManager *m_dockManager = nullptr;
        DockContainerWidget *m_dropContainer = nullptr;
        bool m_hidden = false;
        QPixmap m_contentPreviewPixmap;
        bool m_canceled = false;

        /**
         * Private data constructor
         */
        FloatingDragPreviewPrivate(FloatingDragPreview *parent);
        void updateDropOverlays(const QPoint &globalPosition);

        void setHidden(bool value)
        {
            m_hidden = value;
            q->update();
        }

        /**
         * Cancel dragging and emit the draggingCanceled event
         */
        void cancelDragging()
        {
            m_canceled = true;
            emit q->draggingCanceled();
            m_dockManager->containerOverlay()->hideOverlay();
            m_dockManager->dockAreaOverlay()->hideOverlay();
            q->close();
        }
    }; // class FloatingDragPreviewPrivate

    void FloatingDragPreviewPrivate::updateDropOverlays(const QPoint &globalPosition)
    {
        if (!q->isVisible() || !m_dockManager)
            return;

        auto containers = m_dockManager->dockContainers();
        DockContainerWidget *topContainer = nullptr;
        for (auto containerWidget : containers) {
            if (!containerWidget->isVisible())
                continue;

            QPoint mappedPosition = containerWidget->mapFromGlobal(globalPosition);
            if (containerWidget->rect().contains(mappedPosition)) {
                if (!topContainer || containerWidget->isInFrontOf(topContainer))
                    topContainer = containerWidget;
            }
        }

        m_dropContainer = topContainer;
        auto containerOverlay = m_dockManager->containerOverlay();
        auto dockAreaOverlay = m_dockManager->dockAreaOverlay();
        auto dockDropArea = dockAreaOverlay->dropAreaUnderCursor();
        auto containerDropArea = containerOverlay->dropAreaUnderCursor();

        if (!topContainer) {
            containerOverlay->hideOverlay();
            dockAreaOverlay->hideOverlay();
            if (DockManager::configFlags().testFlag(DockManager::DragPreviewIsDynamic))
                setHidden(false);

            return;
        }

        int visibleDockAreas = topContainer->visibleDockAreaCount();
        containerOverlay->setAllowedAreas(visibleDockAreas > 1 ? OuterDockAreas : AllDockAreas);
        DockWidgetArea containerArea = containerOverlay->showOverlay(topContainer);
        containerOverlay->enableDropPreview(containerArea != InvalidDockWidgetArea);
        auto dockArea = topContainer->dockAreaAt(globalPosition);
        if (dockArea && dockArea->isVisible() && visibleDockAreas > 0
            && dockArea != m_contentSourceArea) {
            dockAreaOverlay->enableDropPreview(true);
            dockAreaOverlay->setAllowedAreas((visibleDockAreas == 1) ? NoDockWidgetArea
                                                                     : dockArea->allowedAreas());
            DockWidgetArea area = dockAreaOverlay->showOverlay(dockArea);

            // A CenterDockWidgetArea for the dockAreaOverlay() indicates that the mouse is in the
            // title bar. If the ContainerArea is valid then we ignore the dock area of the
            // dockAreaOverlay() and disable the drop preview
            if ((area == CenterDockWidgetArea) && (containerArea != InvalidDockWidgetArea)) {
                dockAreaOverlay->enableDropPreview(false);
                containerOverlay->enableDropPreview(true);
            } else {
                containerOverlay->enableDropPreview(InvalidDockWidgetArea == area);
            }
        } else {
            dockAreaOverlay->hideOverlay();
            // If there is only one single visible dock area in a container, then
            // it does not make sense to show a dock overlay because the dock area
            // would be removed and inserted at the same position
            if (visibleDockAreas <= 1)
                containerOverlay->hide();

            if (dockArea == m_contentSourceArea && InvalidDockWidgetArea == containerDropArea)
                m_dropContainer = nullptr;
        }

        if (DockManager::configFlags().testFlag(DockManager::DragPreviewIsDynamic)) {
            setHidden(dockDropArea != InvalidDockWidgetArea
                      || containerDropArea != InvalidDockWidgetArea);
        }
    }

    FloatingDragPreviewPrivate::FloatingDragPreviewPrivate(FloatingDragPreview *parent)
        : q(parent)
    {}

    FloatingDragPreview::FloatingDragPreview(QWidget *content, QWidget *parent)
        : QWidget(parent)
        , d(new FloatingDragPreviewPrivate(this))
    {
        d->m_content = content;
        setAttribute(Qt::WA_DeleteOnClose);
        if (DockManager::configFlags().testFlag(DockManager::DragPreviewHasWindowFrame)) {
            setWindowFlags(Qt::Window | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
        } else {
            setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
            setAttribute(Qt::WA_NoSystemBackground);
            setAttribute(Qt::WA_TranslucentBackground);
        }

        if (Utils::HostOsInfo::isLinuxHost()) {
            auto flags = windowFlags();
            flags |= Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint;
            setWindowFlags(flags);
        }

        setWindowOpacity(0.6);

        // Create a static image of the widget that should get undocked
        // This is like some kind preview image like it is uses in drag and drop operations
        if (DockManager::configFlags().testFlag(DockManager::DragPreviewShowsContentPixmap)) {
            d->m_contentPreviewPixmap = QPixmap(content->size());
            content->render(&d->m_contentPreviewPixmap);
        }
        connect(qApp,
                &QApplication::applicationStateChanged,
                this,
                &FloatingDragPreview::onApplicationStateChanged);
        // The only safe way to receive escape key presses is to install an event
        // filter for the application object
        QApplication::instance()->installEventFilter(this);
    }

    FloatingDragPreview::FloatingDragPreview(DockWidget *content)
        : FloatingDragPreview(static_cast<QWidget *>(content),
                              content->dockManager()) // TODO static_cast?
    {
        d->m_dockManager = content->dockManager();
        if (content->dockAreaWidget()->openDockWidgetsCount() == 1) {
            d->m_contentSourceArea = content->dockAreaWidget();
            d->m_contenSourceContainer = content->dockContainer();
        }
        setWindowTitle(content->windowTitle());
    }

    FloatingDragPreview::FloatingDragPreview(DockAreaWidget *content)
        : FloatingDragPreview(static_cast<QWidget *>(content),
                              content->dockManager()) // TODO static_cast?
    {
        d->m_dockManager = content->dockManager();
        d->m_contentSourceArea = content;
        d->m_contenSourceContainer = content->dockContainer();
        setWindowTitle(content->currentDockWidget()->windowTitle());
    }

    FloatingDragPreview::~FloatingDragPreview() { delete d; }

    void FloatingDragPreview::moveFloating()
    {
        int borderSize = (frameSize().width() - size().width()) / 2;
        const QPoint moveToPos = QCursor::pos() - d->m_dragStartMousePosition
                                 - QPoint(borderSize, 0);
        move(moveToPos);
    }

    void FloatingDragPreview::startFloating(const QPoint &dragStartMousePos,
                                            const QSize &size,
                                            eDragState dragState,
                                            QWidget *mouseEventHandler)
    {
        Q_UNUSED(mouseEventHandler)
        Q_UNUSED(dragState)
        resize(size);
        d->m_dragStartMousePosition = dragStartMousePos;
        moveFloating();
        show();
    }

    void FloatingDragPreview::moveEvent(QMoveEvent *event)
    {
        QWidget::moveEvent(event);
        d->updateDropOverlays(QCursor::pos());
    }

    void FloatingDragPreview::finishDragging()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        auto dockDropArea = d->m_dockManager->dockAreaOverlay()->visibleDropAreaUnderCursor();
        auto containerDropArea = d->m_dockManager->containerOverlay()->visibleDropAreaUnderCursor();
        if (d->m_dropContainer && (dockDropArea != InvalidDockWidgetArea)) {
            d->m_dropContainer->dropWidget(d->m_content,
                                           dockDropArea,
                                           d->m_dropContainer->dockAreaAt(QCursor::pos()));
        } else if (d->m_dropContainer && (containerDropArea != InvalidDockWidgetArea)) {
            d->m_dropContainer->dropWidget(d->m_content, containerDropArea, nullptr);
        } else {
            DockWidget *dockWidget = qobject_cast<DockWidget *>(d->m_content);
            FloatingDockContainer *floatingWidget = nullptr;

            if (dockWidget && dockWidget->features().testFlag(DockWidget::DockWidgetFloatable)) {
                floatingWidget = new FloatingDockContainer(dockWidget);
            } else {
                DockAreaWidget *dockArea = qobject_cast<DockAreaWidget *>(d->m_content);
                if (dockArea->features().testFlag(DockWidget::DockWidgetFloatable))
                    floatingWidget = new FloatingDockContainer(dockArea);
            }

            if (floatingWidget) {
                floatingWidget->setGeometry(this->geometry());
                floatingWidget->show();
                if (!DockManager::configFlags().testFlag(DockManager::DragPreviewHasWindowFrame)) {
                    QApplication::processEvents();
                    int frameHeight = floatingWidget->frameGeometry().height() - floatingWidget->geometry().height();
                    QRect fixedGeometry = this->geometry();
                    fixedGeometry.adjust(0, frameHeight, 0, 0);
                    floatingWidget->setGeometry(fixedGeometry);
                }
            }
        }

        this->close();
        d->m_dockManager->containerOverlay()->hideOverlay();
        d->m_dockManager->dockAreaOverlay()->hideOverlay();
    }

    void FloatingDragPreview::paintEvent(QPaintEvent *event)
    {
        Q_UNUSED(event)
        if (d->m_hidden)
            return;

        QPainter painter(this);
        if (DockManager::configFlags().testFlag(DockManager::DragPreviewShowsContentPixmap))
            painter.drawPixmap(QPoint(0, 0), d->m_contentPreviewPixmap);

        // If we do not have a window frame then we paint a QRubberBand like frameless window
        if (!DockManager::configFlags().testFlag(DockManager::DragPreviewHasWindowFrame)) {
            QColor color = palette().color(QPalette::Active, QPalette::Highlight);
            QPen pen = painter.pen();
            pen.setColor(color.darker(120));
            pen.setStyle(Qt::SolidLine);
            pen.setWidth(1);
            pen.setCosmetic(true);
            painter.setPen(pen);
            color = color.lighter(130);
            color.setAlpha(64);
            painter.setBrush(color);
            painter.drawRect(rect().adjusted(0, 0, -1, -1));
        }
    }

    void FloatingDragPreview::onApplicationStateChanged(Qt::ApplicationState state)
    {
        if (state != Qt::ApplicationActive) {
            disconnect(qApp,
                       &QApplication::applicationStateChanged,
                       this,
                       &FloatingDragPreview::onApplicationStateChanged);
            d->cancelDragging();
        }
    }

    bool FloatingDragPreview::eventFilter(QObject *watched, QEvent *event)
    {
        if (!d->m_canceled && event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                watched->removeEventFilter(this);
                d->cancelDragging();
            }
        }

        return false;
    }

} // namespace ADS
