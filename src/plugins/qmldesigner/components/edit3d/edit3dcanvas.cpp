// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "edit3dcanvas.h"
#include "edit3dview.h"
#include "edit3dwidget.h"

#include <bindingproperty.h>
#include <nodemetainfo.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

#include <utils/qtcassert.h>

#include <coreplugin/icore.h>

#include <qmldesignerplugin.h>
#include <qmldesignerconstants.h>

#include <QApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QPainter>
#include <QQuickWidget>
#include <QtCore/qmimedata.h>

namespace QmlDesigner {

static QQuickWidget *createBusyIndicator(QWidget *p)
{
    auto widget = new QQuickWidget(p);

    const QString source = Core::ICore::resourcePath("qmldesigner/misc/BusyIndicator.qml").toString();
    QTC_ASSERT(QFileInfo::exists(source), return widget);
    widget->setSource(QUrl::fromLocalFile(source));
    widget->setFixedSize(64, 64);
    widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    widget->setClearColor(Qt::transparent);
    widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    widget->setObjectName(Constants::OBJECT_NAME_BUSY_INDICATOR);

    return widget;
}

Edit3DCanvas::Edit3DCanvas(Edit3DWidget *parent)
    : QWidget(parent)
    , m_parent(parent)
    , m_busyIndicator(createBusyIndicator(this))
{
    setMouseTracking(true);
    setAcceptDrops(true);
    setFocusPolicy(Qt::ClickFocus);
    m_busyIndicator->show();

    installEventFilter(this);
}

void Edit3DCanvas::updateRenderImage(const QImage &img)
{
    m_image = img;
    update();
}

void Edit3DCanvas::updateActiveScene(qint32 activeScene)
{
    m_activeScene = activeScene;
}

QImage QmlDesigner::Edit3DCanvas::renderImage() const
{
    return m_image;
}

void Edit3DCanvas::setOpacity(qreal opacity)
{
    m_opacity = opacity;
}

QWidget *Edit3DCanvas::busyIndicator() const
{
    return m_busyIndicator;
}

void Edit3DCanvas::setFlyMode(bool enabled, const QPoint &pos)
{
    if (m_flyMode == enabled)
        return;

    m_flyMode = enabled;
    m_isQDSTrusted = Edit3DView::isQDSTrusted();

    if (enabled) {
        m_flyModeStartTime = QDateTime::currentMSecsSinceEpoch();

        // Mouse cursor will be hidden in the flight mode
        QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

        m_flyModeStartCursorPos = pos;
        m_flyModeFirstUpdate = true;

        // Hide cursor on the middle of the active split to make the wheel work during flight mode.
        // We can't rely on current activeSplit value, as mouse press to enter flight mode can change the
        // active split, so hide the cursor based on its current location.
        QPoint center = mapToGlobal(QPoint(width() / 2, height() / 2));
        if (m_parent->view()->isSplitView()) {
            if (pos.x() <= center.x()) {
                if (pos.y() <= center.y())
                    m_hiddenCursorPos = mapToGlobal(QPoint(width() / 4, height() / 4));
                else
                    m_hiddenCursorPos = mapToGlobal(QPoint(width() / 4, (height() / 4) * 3));
            } else {
                if (pos.y() <= center.y())
                    m_hiddenCursorPos = mapToGlobal(QPoint((width() / 4) * 3, height() / 4));
                else
                    m_hiddenCursorPos = mapToGlobal(QPoint((width() / 4) * 3, (height() / 4) * 3));
            }
        } else {
            m_hiddenCursorPos = center;
        }

        QCursor::setPos(m_hiddenCursorPos);
    } else {
        QCursor::setPos(m_flyModeStartCursorPos);

        if (QApplication::overrideCursor())
            QApplication::restoreOverrideCursor();

        if (m_contextMenuPending && (QDateTime::currentMSecsSinceEpoch() - m_flyModeStartTime) < 500)
            m_parent->view()->showContextMenu();

        m_contextMenuPending = false;
        m_flyModeStartTime = 0;
    }

    m_parent->view()->setFlyMode(enabled);
}

bool Edit3DCanvas::eventFilter(QObject *obj, QEvent *event)
{
    if (m_flyMode && event->type() == QEvent::ShortcutOverride) {
        // Suppress shortcuts that conflict with fly mode keys
        const QList<int> controlKeys = { Qt::Key_W, Qt::Key_A, Qt::Key_S,
                                        Qt::Key_D, Qt::Key_Q, Qt::Key_E,
                                        Qt::Key_Up, Qt::Key_Down, Qt::Key_Left,
                                        Qt::Key_Right, Qt::Key_PageDown, Qt::Key_PageUp,
                                        Qt::Key_Alt, Qt::Key_Shift };
        auto ke = static_cast<QKeyEvent *>(event);
        if (controlKeys.contains(ke->key()))
            event->accept();
   }

    return QObject::eventFilter(obj, event);
}

void Edit3DCanvas::mousePressEvent(QMouseEvent *e)
{
    m_contextMenuPending = false;
    if (!m_flyMode && e->modifiers() == Qt::NoModifier && e->buttons() == Qt::RightButton) {
        setFlyMode(true, e->globalPos());
        m_parent->view()->startContextMenu(e->pos());
        m_contextMenuPending = true;
    }

    m_parent->view()->sendInputEvent(e);
    QWidget::mousePressEvent(e);
}

void Edit3DCanvas::mouseReleaseEvent(QMouseEvent *e)
{
    if ((e->buttons() & Qt::RightButton) == Qt::NoButton)
        setFlyMode(false);
    m_parent->view()->sendInputEvent(e);
    QWidget::mouseReleaseEvent(e);
}

void Edit3DCanvas::mouseDoubleClickEvent(QMouseEvent *e)
{
    m_parent->view()->sendInputEvent(e);
    QWidget::mouseDoubleClickEvent(e);
}

void Edit3DCanvas::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_flyMode)
        m_parent->view()->sendInputEvent(e);

    QWidget::mouseMoveEvent(e);

    if (m_flyMode && e->globalPos() != m_hiddenCursorPos) {
        if (!m_flyModeFirstUpdate) {
            // We notify explicit camera rotation need for puppet rather than rely in mouse events,
            // as mouse isn't grabbed on puppet side and can't handle fast movements that go out of
            // edit camera mouse area. This also simplifies split view handling.
            QPointF diff = m_isQDSTrusted ? (m_hiddenCursorPos - e->globalPos()) : (m_lastCursorPos - e->globalPos());

            if (auto model = m_parent->view()->model()) {
                if (e->buttons() == (Qt::LeftButton | Qt::RightButton)) {
                    model->emitView3DAction(View3DActionType::EditCameraMove,
                                            QVector3D{float(-diff.x()), float(-diff.y()), 0.f});
                } else {
                    model->emitView3DAction(View3DActionType::EditCameraRotation, diff / 6.);
                }
            }
        } else {
            // Skip first move to avoid undesirable jump occasionally when initiating flight mode
            m_flyModeFirstUpdate = false;
        }

        if (m_isQDSTrusted)
            QCursor::setPos(m_hiddenCursorPos);
        else
            m_lastCursorPos = e->globalPos();
    }
}

void Edit3DCanvas::wheelEvent(QWheelEvent *e)
{
    if (m_flyMode) {
        // In fly mode, wheel controls the camera speed slider (value range 1-100)
        double speed;
        double mult;
        m_parent->view()->getCameraSpeedAuxData(speed, mult);
        speed = qMin(100., qMax(1., speed + double(e->angleDelta().y()) / 40.));
        m_parent->view()->setCameraSpeedAuxData(speed, mult);
    } else {
        m_parent->view()->sendInputEvent(e);
    }
    QWidget::wheelEvent(e);
}

void Edit3DCanvas::keyPressEvent(QKeyEvent *e)
{
    if (!e->isAutoRepeat())
        m_parent->view()->sendInputEvent(e);
    QWidget::keyPressEvent(e);
}

void Edit3DCanvas::keyReleaseEvent(QKeyEvent *e)
{
    if (!e->isAutoRepeat())
        m_parent->view()->sendInputEvent(e);
    QWidget::keyReleaseEvent(e);
}

void Edit3DCanvas::paintEvent([[maybe_unused]] QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter painter(this);

    if (m_opacity < 1.0) {
        painter.fillRect(rect(), Qt::black);
        painter.setOpacity(m_opacity);
    }

    painter.drawImage(rect(), m_image, QRect(0, 0, m_image.width(), m_image.height()));
}

void Edit3DCanvas::resizeEvent(QResizeEvent *e)
{
    positionBusyInidicator();
    m_parent->view()->edit3DViewResized(e->size());
}

void Edit3DCanvas::focusOutEvent(QFocusEvent *focusEvent)
{
    QmlDesignerPlugin::emitUsageStatisticsTime(Constants::EVENT_3DEDITOR_TIME,
                                               m_usageTimer.elapsed());

    setFlyMode(false);
    if (auto model = m_parent->view()->model())
        model->emitView3DAction(View3DActionType::EditCameraStopAllMoves, {});

    QWidget::focusOutEvent(focusEvent);
}

void Edit3DCanvas::focusInEvent(QFocusEvent *focusEvent)
{
    m_usageTimer.restart();
    QWidget::focusInEvent(focusEvent);
}

void Edit3DCanvas::enterEvent(QEnterEvent *e)
{
    m_parent->view()->sendInputEvent(e);
    QWidget::enterEvent(e);
}

void Edit3DCanvas::leaveEvent(QEvent *e)
{
    m_parent->view()->sendInputEvent(e);
    QWidget::leaveEvent(e);
}

void Edit3DCanvas::positionBusyInidicator()
{
    m_busyIndicator->move(width() / 2 - 32, height() / 2 - 32);
}

}
