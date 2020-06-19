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

#include "dockoverlay.h"

#include "dockareawidget.h"
#include "dockareatitlebar.h"

#include <utils/hostosinfo.h>
#include <utils/porting.h>

#include <QCursor>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QMap>
#include <QMoveEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>
#include <QWindow>
#include <QtGlobal>

#include <iostream>

namespace ADS {

    /**
     * Private data class of DockOverlay
     */
    class DockOverlayPrivate
    {
    public:
        DockOverlay *q;
        DockWidgetAreas m_allowedAreas = InvalidDockWidgetArea;
        DockOverlayCross *m_cross = nullptr;
        QPointer<QWidget> m_targetWidget;
        DockWidgetArea m_lastLocation = InvalidDockWidgetArea;
        bool m_dropPreviewEnabled = true;
        DockOverlay::eMode m_mode = DockOverlay::ModeDockAreaOverlay;
        QRect m_dropAreaRect;

        /**
          * Private data constructor
          */
        DockOverlayPrivate(DockOverlay *parent)
            : q(parent)
        {}
    };

    /**
     * Private data of DockOverlayCross class
     */
    class DockOverlayCrossPrivate
    {
    public:
        DockOverlayCross *q;
        DockOverlay::eMode m_mode = DockOverlay::ModeDockAreaOverlay;
        DockOverlay *m_dockOverlay = nullptr;
        QHash<DockWidgetArea, QWidget *> m_dropIndicatorWidgets;
        QGridLayout *m_gridLayout = nullptr;
        QColor m_iconColors[5];
        bool m_updateRequired = false;
        double m_lastDevicePixelRatio = 0.1;

        /**
         * Private data constructor
         */
        DockOverlayCrossPrivate(DockOverlayCross *parent)
            : q(parent)
        {}

        /**
         * @param area
         * @return
         */
        QPoint areaGridPosition(const DockWidgetArea area);

        /**
         * Palette based default icon colors
         */
        QColor defaultIconColor(DockOverlayCross::eIconColor colorIndex)
        {
            QPalette palette = q->palette();
            switch (colorIndex) {
            case DockOverlayCross::FrameColor:
                return palette.color(QPalette::Active, QPalette::Highlight);
            case DockOverlayCross::WindowBackgroundColor:
                return palette.color(QPalette::Active, QPalette::Base);
            case DockOverlayCross::OverlayColor: {
                QColor color = palette.color(QPalette::Active, QPalette::Highlight);
                color.setAlpha(64);
                return color;
            }
            case DockOverlayCross::ArrowColor:
                return palette.color(QPalette::Active, QPalette::Base);
            case DockOverlayCross::ShadowColor:
                return QColor(0, 0, 0, 64);
            }

            return QColor();
        }

        /**
         * Stylehseet based icon colors
         */
        QColor iconColor(DockOverlayCross::eIconColor colorIndex)
        {
            QColor color = m_iconColors[colorIndex];
            if (!color.isValid()) {
                color = defaultIconColor(colorIndex);
                m_iconColors[colorIndex] = color;
            }
            return color;
        }

        /**
         * Helper function that returns the drop indicator width depending on the
         * operating system
         */
        qreal dropIndicatiorWidth(QLabel *label) const
        {
            if (Utils::HostOsInfo::isLinuxHost())
                return 40;
            else
                return static_cast<qreal>(label->fontMetrics().height()) * 3.f;
        }

        QWidget *createDropIndicatorWidget(DockWidgetArea dockWidgetArea, DockOverlay::eMode mode)
        {
            QLabel *label = new QLabel();
            label->setObjectName("DockWidgetAreaLabel");

            const qreal metric = dropIndicatiorWidth(label);
            const QSizeF size(metric, metric);

            label->setPixmap(createHighDpiDropIndicatorPixmap(size, dockWidgetArea, mode));
            label->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
            label->setAttribute(Qt::WA_TranslucentBackground);
            label->setProperty("dockWidgetArea", dockWidgetArea);
            return label;
        }

        void updateDropIndicatorIcon(QWidget *dropIndicatorWidget)
        {
            QLabel *label = qobject_cast<QLabel *>(dropIndicatorWidget);
            const qreal metric = dropIndicatiorWidth(label);
            const QSizeF size(metric, metric);

            int area = label->property("dockWidgetArea").toInt();
            label->setPixmap(createHighDpiDropIndicatorPixmap(size,
                                                              static_cast<DockWidgetArea>(area),
                                                              m_mode)); // TODO
        }

        QPixmap createHighDpiDropIndicatorPixmap(const QSizeF &size,
                                                 DockWidgetArea dockWidgetArea,
                                                 DockOverlay::eMode mode)
        {
            const QColor borderColor = iconColor(DockOverlayCross::FrameColor);
            const QColor backgroundColor = iconColor(DockOverlayCross::WindowBackgroundColor);
            const double devicePixelRatio = q->window()->devicePixelRatioF();
            const QSizeF pixmapSize = size * devicePixelRatio;
            QPixmap pixmap(pixmapSize.toSize());
            pixmap.fill(QColor(0, 0, 0, 0));

            QPainter painter(&pixmap);
            QPen pen = painter.pen();
            QRectF shadowRect(pixmap.rect());
            QRectF baseRect;
            baseRect.setSize(shadowRect.size() * 0.7);
            baseRect.moveCenter(shadowRect.center());

            // Fill
            QColor shadowColor = iconColor(DockOverlayCross::ShadowColor);
            if (shadowColor.alpha() == 255) {
                shadowColor.setAlpha(64);
            }
            painter.fillRect(shadowRect, shadowColor);

            // Drop area rect.
            painter.save();
            QRectF areaRect;
            QLineF areaLine;
            QRectF nonAreaRect;
            switch (dockWidgetArea) {
            case TopDockWidgetArea:
                areaRect = QRectF(baseRect.x(), baseRect.y(), baseRect.width(), baseRect.height() * 0.5);
                nonAreaRect = QRectF(baseRect.x(),
                                     shadowRect.height() * 0.5,
                                     baseRect.width(),
                                     baseRect.height() * 0.5);
                areaLine = QLineF(areaRect.bottomLeft(), areaRect.bottomRight());
                break;
            case RightDockWidgetArea:
                areaRect = QRectF(shadowRect.width() * 0.5,
                                  baseRect.y(),
                                  baseRect.width() * 0.5,
                                  baseRect.height());
                nonAreaRect = QRectF(baseRect.x(),
                                     baseRect.y(),
                                     baseRect.width() * 0.5,
                                     baseRect.height());
                areaLine = QLineF(areaRect.topLeft(), areaRect.bottomLeft());
                break;
            case BottomDockWidgetArea:
                areaRect = QRectF(baseRect.x(),
                                  shadowRect.height() * 0.5,
                                  baseRect.width(),
                                  baseRect.height() * 0.5);
                nonAreaRect = QRectF(baseRect.x(),
                                     baseRect.y(),
                                     baseRect.width(),
                                     baseRect.height() * 0.5);
                areaLine = QLineF(areaRect.topLeft(), areaRect.topRight());
                break;
            case LeftDockWidgetArea:
                areaRect = QRectF(baseRect.x(), baseRect.y(), baseRect.width() * 0.5, baseRect.height());
                nonAreaRect = QRectF(shadowRect.width() * 0.5,
                                     baseRect.y(),
                                     baseRect.width() * 0.5,
                                     baseRect.height());
                areaLine = QLineF(areaRect.topRight(), areaRect.bottomRight());
                break;
            default:
                break;
            }

            const QSizeF baseSize = baseRect.size();
            if (DockOverlay::ModeContainerOverlay == mode && dockWidgetArea != CenterDockWidgetArea) {
                baseRect = areaRect;
            }

            painter.fillRect(baseRect, backgroundColor);
            if (areaRect.isValid()) {
                pen = painter.pen();
                pen.setColor(borderColor);
                QColor color = iconColor(DockOverlayCross::OverlayColor);
                if (color.alpha() == 255) {
                    color.setAlpha(64);
                }
                painter.setBrush(color);
                painter.setPen(Qt::NoPen);
                painter.drawRect(areaRect);

                pen = painter.pen();
                pen.setWidth(1);
                pen.setColor(borderColor);
                pen.setStyle(Qt::DashLine);
                painter.setPen(pen);
                painter.drawLine(areaLine);
            }
            painter.restore();

            painter.save();
            // Draw outer border
            pen = painter.pen();
            pen.setColor(borderColor);
            pen.setWidth(1);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(pen);
            painter.drawRect(baseRect);

            // draw window title bar
            painter.setBrush(borderColor);
            const QRectF frameRect(baseRect.topLeft(), QSizeF(baseRect.width(), baseSize.height() / 10));
            painter.drawRect(frameRect);
            painter.restore();

            // Draw arrow for outer container drop indicators
            if (DockOverlay::ModeContainerOverlay == mode && dockWidgetArea != CenterDockWidgetArea) {
                QRectF arrowRect;
                arrowRect.setSize(baseSize);
                arrowRect.setWidth(arrowRect.width() / 4.6);
                arrowRect.setHeight(arrowRect.height() / 2);
                arrowRect.moveCenter(QPointF(0, 0));
                QPolygonF arrow;
                arrow << arrowRect.topLeft() << QPointF(arrowRect.right(), arrowRect.center().y())
                      << arrowRect.bottomLeft();
                painter.setPen(Qt::NoPen);
                painter.setBrush(iconColor(DockOverlayCross::ArrowColor));
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.translate(nonAreaRect.center().x(), nonAreaRect.center().y());

                switch (dockWidgetArea) {
                case TopDockWidgetArea:
                    painter.rotate(-90);
                    break;
                case RightDockWidgetArea:
                    break;
                case BottomDockWidgetArea:
                    painter.rotate(90);
                    break;
                case LeftDockWidgetArea:
                    painter.rotate(180);
                    break;
                default:
                    break;
                }

                painter.drawPolygon(arrow);
            }

            pixmap.setDevicePixelRatio(devicePixelRatio);
            return pixmap;
        }
    }; // class DockOverlayCrossPrivate

    DockOverlay::DockOverlay(QWidget *parent, eMode mode)
        : QFrame(parent)
        , d(new DockOverlayPrivate(this))
    {
        d->m_mode = mode;
        d->m_cross = new DockOverlayCross(this);

        if (Utils::HostOsInfo::isLinuxHost())
            setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                           | Qt::X11BypassWindowManagerHint);
        else
            setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);

        setWindowOpacity(1);
        setWindowTitle("DockOverlay");
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);

        d->m_cross->setVisible(false);
        setVisible(false);
    }

    DockOverlay::~DockOverlay()
    {
        delete d;
    }

    void DockOverlay::setAllowedAreas(DockWidgetAreas areas)
    {
        if (areas == d->m_allowedAreas)
            return;

        d->m_allowedAreas = areas;
        d->m_cross->reset();
    }

    DockWidgetAreas DockOverlay::allowedAreas() const
    {
        return d->m_allowedAreas;
    }

    DockWidgetArea DockOverlay::dropAreaUnderCursor() const
    {
        DockWidgetArea result = d->m_cross->cursorLocation();
        if (result != InvalidDockWidgetArea)
            return result;

        DockAreaWidget *dockArea = qobject_cast<DockAreaWidget *>(d->m_targetWidget.data());
        if (!dockArea)
            return result;

        if (dockArea->allowedAreas().testFlag(CenterDockWidgetArea)
            && !dockArea->titleBar()->isHidden()
            && dockArea->titleBarGeometry().contains(dockArea->mapFromGlobal(QCursor::pos())))
            return CenterDockWidgetArea;

        return result;
    }

    DockWidgetArea DockOverlay::visibleDropAreaUnderCursor() const
    {
        if (isHidden() || !d->m_dropPreviewEnabled)
            return InvalidDockWidgetArea;
        else
            return dropAreaUnderCursor();
    }

    DockWidgetArea DockOverlay::showOverlay(QWidget *target)
    {
        if (d->m_targetWidget == target) {
            // Hint: We could update geometry of overlay here.
            DockWidgetArea dockWidgetArea = dropAreaUnderCursor();
            if (dockWidgetArea != d->m_lastLocation) {
                repaint();
                d->m_lastLocation = dockWidgetArea;
            }
            return dockWidgetArea;
        }

        d->m_targetWidget = target;
        d->m_lastLocation = InvalidDockWidgetArea;

        // Move it over the target.
        resize(target->size());
        QPoint topLeft = target->mapToGlobal(target->rect().topLeft());
        move(topLeft);
        show();
        d->m_cross->updatePosition();
        d->m_cross->updateOverlayIcons();
        return dropAreaUnderCursor();
    }

    void DockOverlay::hideOverlay()
    {
        hide();
        d->m_targetWidget.clear();
        d->m_lastLocation = InvalidDockWidgetArea;
        d->m_dropAreaRect = QRect();
    }

    void DockOverlay::enableDropPreview(bool enable)
    {
        d->m_dropPreviewEnabled = enable;
        update();
    }

    bool DockOverlay::dropPreviewEnabled() const
    {
        return d->m_dropPreviewEnabled;
    }

    void DockOverlay::paintEvent(QPaintEvent *event)
    {
        Q_UNUSED(event)
        // Draw rect based on location
        if (!d->m_dropPreviewEnabled) {
            d->m_dropAreaRect = QRect();
            return;
        }

        QRect rectangle = rect();
        const DockWidgetArea dockWidgetArea = dropAreaUnderCursor();
        double factor = (DockOverlay::ModeContainerOverlay == d->m_mode) ? 3 : 2;

        switch (dockWidgetArea) {
        case TopDockWidgetArea:
            rectangle.setHeight(static_cast<int>(rectangle.height() / factor));
            break;
        case RightDockWidgetArea:
            rectangle.setX(static_cast<int>(rectangle.width() * (1 - 1 / factor)));
            break;
        case BottomDockWidgetArea:
            rectangle.setY(static_cast<int>(rectangle.height() * (1 - 1 / factor)));
            break;
        case LeftDockWidgetArea:
            rectangle.setWidth(static_cast<int>(rectangle.width() / factor));
            break;
        case CenterDockWidgetArea:
            rectangle = rect();
            break;
        default:
            return;
        }
        QPainter painter(this);
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
        painter.drawRect(rectangle.adjusted(0, 0, -1, -1));
        d->m_dropAreaRect = rectangle;
    }

    QRect DockOverlay::dropOverlayRect() const
    {
        return d->m_dropAreaRect;
    }

    void DockOverlay::showEvent(QShowEvent *event)
    {
        d->m_cross->show();
        QFrame::showEvent(event);
    }

    void DockOverlay::hideEvent(QHideEvent *event)
    {
        d->m_cross->hide();
        QFrame::hideEvent(event);
    }

    bool DockOverlay::event(QEvent *event)
    {
        bool result = Super::event(event);
        if (event->type() == QEvent::Polish)
            d->m_cross->setupOverlayCross(d->m_mode);

        return result;
    }

    static int areaAlignment(const DockWidgetArea area)
    {
        switch (area) {
        case TopDockWidgetArea:
            return Qt::AlignHCenter | Qt::AlignBottom;
        case RightDockWidgetArea:
            return Qt::AlignLeft | Qt::AlignVCenter;
        case BottomDockWidgetArea:
            return Qt::AlignHCenter | Qt::AlignTop;
        case LeftDockWidgetArea:
            return Qt::AlignRight | Qt::AlignVCenter;
        case CenterDockWidgetArea:
            return Qt::AlignCenter;
        default:
            return Qt::AlignCenter;
        }
    }

    // DockOverlayCrossPrivate
    QPoint DockOverlayCrossPrivate::areaGridPosition(const DockWidgetArea area)
    {
        if (DockOverlay::ModeDockAreaOverlay == m_mode) {
            switch (area) {
            case TopDockWidgetArea:
                return QPoint(1, 2);
            case RightDockWidgetArea:
                return QPoint(2, 3);
            case BottomDockWidgetArea:
                return QPoint(3, 2);
            case LeftDockWidgetArea:
                return QPoint(2, 1);
            case CenterDockWidgetArea:
                return QPoint(2, 2);
            default:
                return QPoint();
            }
        } else {
            switch (area) {
            case TopDockWidgetArea:
                return QPoint(0, 2);
            case RightDockWidgetArea:
                return QPoint(2, 4);
            case BottomDockWidgetArea:
                return QPoint(4, 2);
            case LeftDockWidgetArea:
                return QPoint(2, 0);
            case CenterDockWidgetArea:
                return QPoint(2, 2);
            default:
                return QPoint();
            }
        }
    }

    DockOverlayCross::DockOverlayCross(DockOverlay *overlay)
        : QWidget(overlay->parentWidget())
        , d(new DockOverlayCrossPrivate(this))
    {
        d->m_dockOverlay = overlay;

        if (Utils::HostOsInfo::isLinuxHost())
            setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                           | Qt::X11BypassWindowManagerHint);
        else
            setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);

        setWindowTitle("DockOverlayCross");
        setAttribute(Qt::WA_TranslucentBackground);

        d->m_gridLayout = new QGridLayout();
        d->m_gridLayout->setSpacing(0);
        setLayout(d->m_gridLayout);
    }

    DockOverlayCross::~DockOverlayCross()
    {
        delete d;
    }

    void DockOverlayCross::setupOverlayCross(DockOverlay::eMode mode)
    {
        d->m_mode = mode;

        QHash<DockWidgetArea, QWidget *> areaWidgets;
        areaWidgets.insert(TopDockWidgetArea, d->createDropIndicatorWidget(TopDockWidgetArea, mode));
        areaWidgets.insert(RightDockWidgetArea, d->createDropIndicatorWidget(RightDockWidgetArea, mode));
        areaWidgets.insert(BottomDockWidgetArea,
                           d->createDropIndicatorWidget(BottomDockWidgetArea, mode));
        areaWidgets.insert(LeftDockWidgetArea, d->createDropIndicatorWidget(LeftDockWidgetArea, mode));
        areaWidgets.insert(CenterDockWidgetArea,
                           d->createDropIndicatorWidget(CenterDockWidgetArea, mode));
        d->m_lastDevicePixelRatio = devicePixelRatioF();
        setAreaWidgets(areaWidgets);
        d->m_updateRequired = false;
    }

    void DockOverlayCross::updateOverlayIcons()
    {
        if (windowHandle()->devicePixelRatio() == d->m_lastDevicePixelRatio) // TODO
            return;

        for (auto widget : d->m_dropIndicatorWidgets)
            d->updateDropIndicatorIcon(widget);

        d->m_lastDevicePixelRatio = devicePixelRatioF();
    }

    void DockOverlayCross::setIconColor(eIconColor colorIndex, const QColor &color)
    {
        d->m_iconColors[colorIndex] = color;
        d->m_updateRequired = true;
    }

    QColor DockOverlayCross::iconColor(eIconColor colorIndex) const
    {
        return d->m_iconColors[colorIndex];
    }

    void DockOverlayCross::setAreaWidgets(const QHash<DockWidgetArea, QWidget *> &widgets)
    {
        // Delete old widgets.
        const auto values = d->m_dropIndicatorWidgets.values();
        for (auto widget : values) {
            d->m_gridLayout->removeWidget(widget);
            delete widget;
        }
        d->m_dropIndicatorWidgets.clear();

        // Insert new widgets into grid.
        d->m_dropIndicatorWidgets = widgets;

        const QHash<DockWidgetArea, QWidget *> areas = d->m_dropIndicatorWidgets;
        QHash<DockWidgetArea, QWidget *>::const_iterator constIt;
        for (constIt = areas.begin(); constIt != areas.end(); ++constIt) {
            const DockWidgetArea area = constIt.key();
            QWidget *widget = constIt.value();
            const QPoint position = d->areaGridPosition(area);
            d->m_gridLayout->addWidget(widget,
                                       position.x(),
                                       position.y(),
                                       static_cast<Qt::Alignment>(areaAlignment(area)));
        }

        if (DockOverlay::ModeDockAreaOverlay == d->m_mode) {
            d->m_gridLayout->setContentsMargins(0, 0, 0, 0);
            d->m_gridLayout->setRowStretch(0, 1);
            d->m_gridLayout->setRowStretch(1, 0);
            d->m_gridLayout->setRowStretch(2, 0);
            d->m_gridLayout->setRowStretch(3, 0);
            d->m_gridLayout->setRowStretch(4, 1);

            d->m_gridLayout->setColumnStretch(0, 1);
            d->m_gridLayout->setColumnStretch(1, 0);
            d->m_gridLayout->setColumnStretch(2, 0);
            d->m_gridLayout->setColumnStretch(3, 0);
            d->m_gridLayout->setColumnStretch(4, 1);
        } else {
            d->m_gridLayout->setContentsMargins(4, 4, 4, 4);
            d->m_gridLayout->setRowStretch(0, 0);
            d->m_gridLayout->setRowStretch(1, 1);
            d->m_gridLayout->setRowStretch(2, 1);
            d->m_gridLayout->setRowStretch(3, 1);
            d->m_gridLayout->setRowStretch(4, 0);

            d->m_gridLayout->setColumnStretch(0, 0);
            d->m_gridLayout->setColumnStretch(1, 1);
            d->m_gridLayout->setColumnStretch(2, 1);
            d->m_gridLayout->setColumnStretch(3, 1);
            d->m_gridLayout->setColumnStretch(4, 0);
        }
        reset();
    }

    DockWidgetArea DockOverlayCross::cursorLocation() const
    {
        const QPoint position = mapFromGlobal(QCursor::pos());

        const QHash<DockWidgetArea, QWidget *> areas = d->m_dropIndicatorWidgets;
        QHash<DockWidgetArea, QWidget *>::const_iterator constIt;
        for (constIt = areas.begin(); constIt != areas.end(); ++constIt)
        {
            if (d->m_dockOverlay->allowedAreas().testFlag(constIt.key()) && constIt.value()
                && constIt.value()->isVisible() && constIt.value()->geometry().contains(position)) {
                return constIt.key();
            }
        }

        return InvalidDockWidgetArea;
    }

    void DockOverlayCross::showEvent(QShowEvent *)
    {
        if (d->m_updateRequired)
            setupOverlayCross(d->m_mode);

        this->updatePosition();
    }

    void DockOverlayCross::updatePosition()
    {
        resize(d->m_dockOverlay->size());
        QPoint topLeft = d->m_dockOverlay->pos();
        QPoint offest((this->width() - d->m_dockOverlay->width()) / 2,
                      (this->height() - d->m_dockOverlay->height()) / 2);
        QPoint crossTopLeft = topLeft - offest;
        move(crossTopLeft);
    }

    void DockOverlayCross::reset()
    {
        const QList<DockWidgetArea> allAreas{TopDockWidgetArea,
                                             RightDockWidgetArea,
                                             BottomDockWidgetArea,
                                             LeftDockWidgetArea,
                                             CenterDockWidgetArea};
        const DockWidgetAreas allowedAreas = d->m_dockOverlay->allowedAreas();

        // Update visibility of area widgets based on allowedAreas.
        for (auto area : allAreas) {
            const QPoint position = d->areaGridPosition(area);
            QLayoutItem *item = d->m_gridLayout->itemAtPosition(position.x(), position.y());
            QWidget *widget = nullptr;
            if (item && (widget = item->widget()) != nullptr)
                widget->setVisible(allowedAreas.testFlag(area));
        }
    }

    void DockOverlayCross::setIconColors(const QString &colors)
    {
        static const QMap<QString, int>
            colorCompenentStringMap{{"Frame", DockOverlayCross::FrameColor},
                                    {"Background", DockOverlayCross::WindowBackgroundColor},
                                    {"Overlay", DockOverlayCross::OverlayColor},
                                    {"Arrow", DockOverlayCross::ArrowColor},
                                    {"Shadow", DockOverlayCross::ShadowColor}};

        auto colorList = colors.split(' ', Utils::SkipEmptyParts);
        for (const auto &colorListEntry : colorList) {
            auto componentColor = colorListEntry.split('=', Utils::SkipEmptyParts);
            int component = colorCompenentStringMap.value(componentColor[0], -1);
            if (component < 0)
                continue;

            d->m_iconColors[component] = QColor(componentColor[1]);
        }

        d->m_updateRequired = true;
    }

    QString DockOverlayCross::iconColors() const
    {
        return QString();
    }

} // namespace ADS
