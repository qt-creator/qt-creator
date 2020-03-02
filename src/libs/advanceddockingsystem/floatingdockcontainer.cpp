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

#include "floatingdockcontainer.h"

#include "dockareawidget.h"
#include "dockcontainerwidget.h"
#include "dockmanager.h"
#include "dockoverlay.h"
#include "dockwidget.h"
#include "linux/floatingwidgettitlebar.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QDebug>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QPointer>

static Q_LOGGING_CATEGORY(adsLog, "qtc.qmldesigner.advanceddockingsystem", QtWarningMsg)

namespace ADS
{
    AbstractFloatingWidget::~AbstractFloatingWidget() = default;

    static unsigned int zOrderCounter = 0;
    /**
     * Private data class of FloatingDockContainer class (pimpl)
     */
    class FloatingDockContainerPrivate
    {
    public:
        FloatingDockContainer *q;
        DockContainerWidget *m_dockContainer = nullptr;
        unsigned int m_zOrderIndex = ++zOrderCounter;
        QPointer<DockManager> m_dockManager;
        eDragState m_draggingState = DraggingInactive;
        QPoint m_dragStartMousePosition;
        DockContainerWidget *m_dropContainer = nullptr;
        DockAreaWidget *m_singleDockArea = nullptr;
        QWidget *m_mouseEventHandler = nullptr;
        FloatingWidgetTitleBar *m_titleBar = nullptr;

        /**
         * Private data constructor
         */
        FloatingDockContainerPrivate(FloatingDockContainer *parent);

        void titleMouseReleaseEvent();
        void updateDropOverlays(const QPoint &globalPosition);

        /**
         * Returns true if the given config flag is set
         */
        static bool testConfigFlag(DockManager::eConfigFlag flag)
        {
            return DockManager::configFlags().testFlag(flag);
        }

        /**
         * Tests is a certain state is active
         */
        bool isState(eDragState stateId) const { return stateId == m_draggingState; }

        void setState(eDragState stateId) { m_draggingState = stateId; }

        void setWindowTitle(const QString &text)
        {
            if (Utils::HostOsInfo::isLinuxHost())
                m_titleBar->setTitle(text);
            else
                q->setWindowTitle(text);
        }

        void reflectCurrentWidget(DockWidget *currentWidget)
        {
            // reflect CurrentWidget's title if configured to do so, otherwise display application name as window title
            if (testConfigFlag(DockManager::FloatingContainerHasWidgetTitle)) {
                setWindowTitle(currentWidget->windowTitle());
            } else {
                setWindowTitle(QApplication::applicationDisplayName());
            }

            // reflect CurrentWidget's icon if configured to do so, otherwise display application icon as window icon
            QIcon CurrentWidgetIcon = currentWidget->icon();
            if (testConfigFlag(DockManager::FloatingContainerHasWidgetIcon)
                    && !CurrentWidgetIcon.isNull())
            {
                q->setWindowIcon(currentWidget->icon());
            } else {
                q->setWindowIcon(QApplication::windowIcon());
            }
        }
    }; // class FloatingDockContainerPrivate

    FloatingDockContainerPrivate::FloatingDockContainerPrivate(FloatingDockContainer *parent)
        : q(parent)
    {}

    void FloatingDockContainerPrivate::titleMouseReleaseEvent()
    {
        setState(DraggingInactive);
        if (!m_dropContainer) {
            return;
        }

        if (m_dockManager->dockAreaOverlay()->dropAreaUnderCursor() != InvalidDockWidgetArea
            || m_dockManager->containerOverlay()->dropAreaUnderCursor() != InvalidDockWidgetArea) {
            // Resize the floating widget to the size of the highlighted drop area rectangle
            DockOverlay *overlay = m_dockManager->containerOverlay();
            if (!overlay->dropOverlayRect().isValid()) {
                overlay = m_dockManager->dockAreaOverlay();
            }

            QRect rect = overlay->dropOverlayRect();
            int frameWidth = (q->frameSize().width() - q->rect().width()) / 2;
            int titleBarHeight = q->frameSize().height() - q->rect().height() - frameWidth;
            if (rect.isValid()) {
                QPoint topLeft = overlay->mapToGlobal(rect.topLeft());
                topLeft.ry() += titleBarHeight;
                q->setGeometry(QRect(topLeft, QSize(rect.width(), rect.height() - titleBarHeight)));
                QApplication::processEvents();
            }
            m_dropContainer->dropFloatingWidget(q, QCursor::pos());
        }

        m_dockManager->containerOverlay()->hideOverlay();
        m_dockManager->dockAreaOverlay()->hideOverlay();
    }

    void FloatingDockContainerPrivate::updateDropOverlays(const QPoint &globalPosition)
    {
        if (!q->isVisible() || !m_dockManager) {
            return;
        }

        auto containers = m_dockManager->dockContainers();
        DockContainerWidget *topContainer = nullptr;
        for (auto containerWidget : containers) {
            if (!containerWidget->isVisible()) {
                continue;
            }

            if (m_dockContainer == containerWidget) {
                continue;
            }

            QPoint mappedPos = containerWidget->mapFromGlobal(globalPosition);
            if (containerWidget->rect().contains(mappedPos)) {
                if (!topContainer || containerWidget->isInFrontOf(topContainer)) {
                    topContainer = containerWidget;
                }
            }
        }

        m_dropContainer = topContainer;
        auto containerOverlay = m_dockManager->containerOverlay();
        auto dockAreaOverlay = m_dockManager->dockAreaOverlay();

        if (!topContainer) {
            containerOverlay->hideOverlay();
            dockAreaOverlay->hideOverlay();
            return;
        }

        int visibleDockAreas = topContainer->visibleDockAreaCount();
        containerOverlay->setAllowedAreas(visibleDockAreas > 1 ? OuterDockAreas : AllDockAreas);
        DockWidgetArea containerArea = containerOverlay->showOverlay(topContainer);
        containerOverlay->enableDropPreview(containerArea != InvalidDockWidgetArea);
        auto dockArea = topContainer->dockAreaAt(globalPosition);
        if (dockArea && dockArea->isVisible() && visibleDockAreas > 0) {
            dockAreaOverlay->enableDropPreview(true);
            dockAreaOverlay->setAllowedAreas((visibleDockAreas == 1) ? NoDockWidgetArea
                                                                     : dockArea->allowedAreas());
            DockWidgetArea area = dockAreaOverlay->showOverlay(dockArea);

            // A CenterDockWidgetArea for the dockAreaOverlay() indicates that the mouse is in
            // the title bar. If the ContainerArea is valid then we ignore the dock area of the
            // dockAreaOverlay() and disable the drop preview
            if ((area == CenterDockWidgetArea) && (containerArea != InvalidDockWidgetArea)) {
                dockAreaOverlay->enableDropPreview(false);
                containerOverlay->enableDropPreview(true);
            } else {
                containerOverlay->enableDropPreview(InvalidDockWidgetArea == area);
            }
        } else {
            dockAreaOverlay->hideOverlay();
        }
    }

    FloatingDockContainer::FloatingDockContainer(DockManager *dockManager)
        : FloatingWidgetBaseType(dockManager)
        , d(new FloatingDockContainerPrivate(this))
    {
        d->m_dockManager = dockManager;
        d->m_dockContainer = new DockContainerWidget(dockManager, this);
        connect(d->m_dockContainer,
                &DockContainerWidget::dockAreasAdded,
                this,
                &FloatingDockContainer::onDockAreasAddedOrRemoved);
        connect(d->m_dockContainer,
                &DockContainerWidget::dockAreasRemoved,
                this,
                &FloatingDockContainer::onDockAreasAddedOrRemoved);

#ifdef Q_OS_LINUX
        d->m_titleBar = new FloatingWidgetTitleBar(this);
        setWindowFlags(windowFlags() | Qt::Tool);
        QDockWidget::setWidget(d->m_dockContainer);
        QDockWidget::setFloating(true);
        QDockWidget::setFeatures(QDockWidget::AllDockWidgetFeatures);
        setTitleBarWidget(d->m_titleBar);
        connect(d->m_titleBar,
                &FloatingWidgetTitleBar::closeRequested,
                this,
                &FloatingDockContainer::close);
#else
        setWindowFlags(Qt::Window | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
        QBoxLayout *boxLayout = new QBoxLayout(QBoxLayout::TopToBottom);
        boxLayout->setContentsMargins(0, 0, 0, 0);
        boxLayout->setSpacing(0);
        setLayout(boxLayout);
        boxLayout->addWidget(d->m_dockContainer);
#endif
        dockManager->registerFloatingWidget(this);
    }

    FloatingDockContainer::FloatingDockContainer(DockAreaWidget *dockArea)
        : FloatingDockContainer(dockArea->dockManager())
    {
        d->m_dockContainer->addDockArea(dockArea);
        if (Utils::HostOsInfo::isLinuxHost())
            d->m_titleBar->enableCloseButton(isClosable());

        auto dw = topLevelDockWidget();
        if (dw) {
            dw->emitTopLevelChanged(true);
        }
    }

    FloatingDockContainer::FloatingDockContainer(DockWidget *dockWidget)
        : FloatingDockContainer(dockWidget->dockManager())
    {
        d->m_dockContainer->addDockWidget(CenterDockWidgetArea, dockWidget);
        if (Utils::HostOsInfo::isLinuxHost())
            d->m_titleBar->enableCloseButton(isClosable());

        auto dw = topLevelDockWidget();
        if (dw) {
            dw->emitTopLevelChanged(true);
        }
    }

    FloatingDockContainer::~FloatingDockContainer()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        if (d->m_dockManager) {
            d->m_dockManager->removeFloatingWidget(this);
        }
        delete d;
    }

    DockContainerWidget *FloatingDockContainer::dockContainer() const { return d->m_dockContainer; }

    void FloatingDockContainer::changeEvent(QEvent *event)
    {
        QWidget::changeEvent(event);
        if ((event->type() == QEvent::ActivationChange) && isActiveWindow()) {
            qCInfo(adsLog) << Q_FUNC_INFO << "QEvent::ActivationChange";
            d->m_zOrderIndex = ++zOrderCounter;
            return;
        }
    }

    void FloatingDockContainer::moveEvent(QMoveEvent *event)
    {
        QWidget::moveEvent(event);
        switch (d->m_draggingState) {
        case DraggingMousePressed:
            d->setState(DraggingFloatingWidget);
            d->updateDropOverlays(QCursor::pos());
            break;

        case DraggingFloatingWidget:
            d->updateDropOverlays(QCursor::pos());
            if (Utils::HostOsInfo::isMacHost()) {
                // In macOS when hiding the DockAreaOverlay the application would set
                // the main window as the active window for some reason. This fixes
                // that by resetting the active window to the floating widget after
                // updating the overlays.
                QApplication::setActiveWindow(this);
            }
            break;
        default:
            break;
        }
    }

    void FloatingDockContainer::closeEvent(QCloseEvent *event)
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        d->setState(DraggingInactive);
        event->ignore();

        if (isClosable()) {
            auto dw = topLevelDockWidget();
            if (dw && dw->features().testFlag(DockWidget::DockWidgetDeleteOnClose)) {
                if (!dw->closeDockWidgetInternal()) {
                    return;
                }
            }

            this->hide();
        }
    }

    void FloatingDockContainer::hideEvent(QHideEvent *event)
    {
        Super::hideEvent(event);
        if (event->spontaneous()) {
            return;
        }

        // Prevent toogleView() events during restore state
        if (d->m_dockManager->isRestoringState()) {
            return;
        }

        for (auto dockArea : d->m_dockContainer->openedDockAreas()) {
            for (auto dockWidget : dockArea->openedDockWidgets()) {
                dockWidget->toggleView(false);
            }
        }
    }

    void FloatingDockContainer::showEvent(QShowEvent *event) { Super::showEvent(event); }

    bool FloatingDockContainer::event(QEvent *event)
    {
        switch (d->m_draggingState) {
        case DraggingInactive: {
            // Normally we would check here, if the left mouse button is pressed.
            // But from QT version 5.12.2 on the mouse events from
            // QEvent::NonClientAreaMouseButtonPress return the wrong mouse button
            // The event always returns Qt::RightButton even if the left button is clicked.
            // It is really great to work around the whole NonClientMouseArea bugs
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 2))
            if (event->type()
                == QEvent::
                    NonClientAreaMouseButtonPress /*&& QGuiApplication::mouseButtons().testFlag(Qt::LeftButton)*/) {
                qCInfo(adsLog) << Q_FUNC_INFO << "QEvent::NonClientAreaMouseButtonPress"
                               << event->type();
                d->setState(DraggingMousePressed);
            }
#else
            if (event->type() == QEvent::NonClientAreaMouseButtonPress
                && QGuiApplication::mouseButtons().testFlag(Qt::LeftButton)) {
                qCInfo(adsLog) << Q_FUNC_INFO << "QEvent::NonClientAreaMouseButtonPress"
                               << event->type();
                d->setState(DraggingMousePressed);
            }
#endif
        } break;

        case DraggingMousePressed:
            switch (event->type()) {
            case QEvent::NonClientAreaMouseButtonDblClick:
                qCInfo(adsLog) << Q_FUNC_INFO << "QEvent::NonClientAreaMouseButtonDblClick";
                d->setState(DraggingInactive);
                break;

            case QEvent::Resize:
                // If the first event after the mouse press is a resize event, then
                // the user resizes the window instead of dragging it around.
                // But there is one exception. If the window is maximized,
                // then dragging the window via title bar will cause the widget to
                // leave the maximized state. This in turn will trigger a resize event.
                // To know, if the resize event was triggered by user via moving a
                // corner of the window frame or if it was caused by a windows state
                // change, we check, if we are not in maximized state.
                if (!isMaximized()) {
                    d->setState(DraggingInactive);
                }
                break;

            default:
                break;
            }
            break;

        case DraggingFloatingWidget:
            if (event->type() == QEvent::NonClientAreaMouseButtonRelease) {
                qCInfo(adsLog) << Q_FUNC_INFO << "QEvent::NonClientAreaMouseButtonRelease";
                d->titleMouseReleaseEvent();
            }
            break;

        default:
            break;
        }

#if (ADS_DEBUG_LEVEL > 0)
        qDebug() << "FloatingDockContainer::event " << event->type();
#endif
        return QWidget::event(event);
    }

    void FloatingDockContainer::startFloating(const QPoint &dragStartMousePos,
                                              const QSize &size,
                                              eDragState dragState,
                                              QWidget *mouseEventHandler)
    {
        resize(size);
        d->setState(dragState);
        d->m_dragStartMousePosition = dragStartMousePos;

        if (Utils::HostOsInfo::isLinuxHost()) {
            if (DraggingFloatingWidget == dragState) {
                setAttribute(Qt::WA_X11NetWmWindowTypeDock, true);
                d->m_mouseEventHandler = mouseEventHandler;
                if (d->m_mouseEventHandler) {
                    d->m_mouseEventHandler->grabMouse();
                }
            }
        }
        moveFloating();
        show();
    }

    void FloatingDockContainer::moveFloating()
    {
        int borderSize = (frameSize().width() - size().width()) / 2;
        const QPoint moveToPos = QCursor::pos() - d->m_dragStartMousePosition
                                 - QPoint(borderSize, 0);
        move(moveToPos);
    }

    bool FloatingDockContainer::isClosable() const
    {
        return d->m_dockContainer->features().testFlag(DockWidget::DockWidgetClosable);
    }

    void FloatingDockContainer::onDockAreasAddedOrRemoved()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        auto topLevelDockArea = d->m_dockContainer->topLevelDockArea();
        if (topLevelDockArea) {
            d->m_singleDockArea = topLevelDockArea;
            DockWidget *currentWidget = d->m_singleDockArea->currentDockWidget();
            d->reflectCurrentWidget(currentWidget);
            connect(d->m_singleDockArea,
                    &DockAreaWidget::currentChanged,
                    this,
                    &FloatingDockContainer::onDockAreaCurrentChanged);
        } else {
            if (d->m_singleDockArea) {
                disconnect(d->m_singleDockArea,
                           &DockAreaWidget::currentChanged,
                           this,
                           &FloatingDockContainer::onDockAreaCurrentChanged);
                d->m_singleDockArea = nullptr;
            }
            d->setWindowTitle(QApplication::applicationDisplayName());
            setWindowIcon(QApplication::windowIcon());
        }
    }

    void FloatingDockContainer::updateWindowTitle()
    {
        auto topLevelDockArea = d->m_dockContainer->topLevelDockArea();
        if (topLevelDockArea) {
            DockWidget *currentWidget = topLevelDockArea->currentDockWidget();
            d->reflectCurrentWidget(currentWidget);
        } else {
            d->setWindowTitle(QApplication::applicationDisplayName());
            setWindowIcon(QApplication::windowIcon());
        }
    }

    void FloatingDockContainer::onDockAreaCurrentChanged(int index)
    {
        Q_UNUSED(index)
        DockWidget *currentWidget = d->m_singleDockArea->currentDockWidget();
        d->reflectCurrentWidget(currentWidget);
    }

    bool FloatingDockContainer::restoreState(DockingStateReader &stream, bool testing)
    {
        if (!d->m_dockContainer->restoreState(stream, testing)) {
            return false;
        }

        onDockAreasAddedOrRemoved();
        return true;
    }

    bool FloatingDockContainer::hasTopLevelDockWidget() const
    {
        return d->m_dockContainer->hasTopLevelDockWidget();
    }

    DockWidget *FloatingDockContainer::topLevelDockWidget() const
    {
        return d->m_dockContainer->topLevelDockWidget();
    }

    QList<DockWidget *> FloatingDockContainer::dockWidgets() const
    {
        return d->m_dockContainer->dockWidgets();
    }

    void FloatingDockContainer::finishDragging()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;

        if (Utils::HostOsInfo::isLinuxHost()) {
            setAttribute(Qt::WA_X11NetWmWindowTypeDock, false);
            setWindowOpacity(1);
            activateWindow();
            if (d->m_mouseEventHandler) {
                d->m_mouseEventHandler->releaseMouse();
                d->m_mouseEventHandler = nullptr;
            }
        }
        d->titleMouseReleaseEvent();
    }

} // namespace ADS
