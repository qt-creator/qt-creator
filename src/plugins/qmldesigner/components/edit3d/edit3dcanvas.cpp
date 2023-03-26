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

void Edit3DCanvas::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton && e->modifiers() == Qt::NoModifier)
        m_parent->view()->startContextMenu(e->pos());

    m_parent->view()->sendInputEvent(e);
    QWidget::mousePressEvent(e);
}

void Edit3DCanvas::mouseReleaseEvent(QMouseEvent *e)
{
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
    m_parent->view()->sendInputEvent(e);
    QWidget::mouseMoveEvent(e);
}

void Edit3DCanvas::wheelEvent(QWheelEvent *e)
{
    m_parent->view()->sendInputEvent(e);
    QWidget::wheelEvent(e);
}

void Edit3DCanvas::keyPressEvent(QKeyEvent *e)
{
    m_parent->view()->sendInputEvent(e);
    QWidget::keyPressEvent(e);
}

void Edit3DCanvas::keyReleaseEvent(QKeyEvent *e)
{
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
    QWidget::focusOutEvent(focusEvent);
}

void Edit3DCanvas::focusInEvent(QFocusEvent *focusEvent)
{
    m_usageTimer.restart();
    QWidget::focusInEvent(focusEvent);
}

void Edit3DCanvas::positionBusyInidicator()
{
    m_busyIndicator->move(width() / 2 - 32, height() / 2 - 32);
}

}
