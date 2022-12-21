// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "seekerslider.h"

#include <utils/icon.h>

#include <QStyleOption>
#include <QSlider>
#include <QDebug>
#include <QPainter>

namespace QmlDesigner {

SeekerSlider::SeekerSlider(QWidget *parentWidget)
    : QWidget(parentWidget),
      m_bgIcon(QLatin1String(":/icon/layout/scrubbg.png"))
{
    m_handleIcon.addFile(QLatin1String(":/icon/layout/scrubhandle-24.png"), QSize(24, 24));
    m_handleIcon.addFile(QLatin1String(":/icon/layout/scrubhandle-48.png"), QSize(48, 48));
    m_handleIcon.addFile(QLatin1String(":/icon/layout/scrubhandle-disabled-24.png"), QSize(24, 24), QIcon::Disabled);
    m_handleIcon.addFile(QLatin1String(":/icon/layout/scrubhandle-disabled-48.png"), QSize(48, 48), QIcon::Disabled);
    const Utils::Icon bg({{":/icon/layout/scrubbg.png", Utils::Theme::IconsBaseColor}});
    m_bgWidth = bg.pixmap().width();
    m_bgHeight = bg.pixmap().height();
    m_handleWidth = m_bgHeight;
    m_handleHeight = m_bgHeight;
    int width = m_bgWidth + m_handleWidth * 2;
    m_sliderHalfWidth = m_bgWidth / 2;
    setMinimumWidth(width);
    setMaximumWidth(width);
    setProperty("panelwidget", true);
    setProperty("panelwidget_singlerow", true);
}

void SeekerSlider::paintEvent([[maybe_unused]] QPaintEvent *event)
{
    QPainter painter(this);
    {
        QStyleOptionToolBar option;
        option.rect = rect();
        option.state = QStyle::State_Horizontal;
        style()->drawControl(QStyle::CE_ToolBar, &option, &painter, this);
    }

    int x = rect().width() / 2;
    int y = rect().height() / 2;

    const QPixmap bg = m_bgIcon.pixmap(QSize(m_bgWidth, m_bgHeight), isEnabled() ? QIcon::Normal : QIcon::Disabled, QIcon::On);
    painter.drawPixmap(x - m_bgWidth / 2, y - m_bgHeight / 2, bg);

    if (m_moving) {
        const QPixmap handle = m_handleIcon.pixmap(QSize(m_handleWidth, m_handleHeight), QIcon::Active, QIcon::On);
        painter.drawPixmap(x - m_handleWidth / 2 + m_sliderPos, y - m_handleHeight / 2, handle);
    } else {
        const QPixmap handle = m_handleIcon.pixmap(QSize(m_handleWidth, m_handleHeight), isEnabled() ? QIcon::Normal : QIcon::Disabled, QIcon::On);
        painter.drawPixmap(x - m_handleWidth / 2, y - m_handleHeight / 2, handle);
    }
}

void SeekerSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    int x = rect().width() / 2;
    int y = rect().height() / 2;
    auto pos = event->localPos();
    if (pos.x() >= x - m_handleWidth / 2 && pos.x() <= x + m_handleWidth / 2
            && pos.y() >= y - m_handleHeight / 2 && pos.y() <= y + m_handleHeight / 2) {
        m_moving = true;
        m_startPos = pos.x();
    }
}

void SeekerSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_moving) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    auto pos = event->localPos();
    int delta = pos.x() - m_startPos;
    m_sliderPos = qBound(-m_sliderHalfWidth, delta, m_sliderHalfWidth);
    delta = m_maxPosition * m_sliderPos / m_sliderHalfWidth;
    if (delta != m_position) {
        m_position = delta;
        Q_EMIT positionChanged();
        update();
    }
}

void SeekerSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_moving) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    m_moving = false;
    m_position = 0;
    m_startPos = 0;
    m_sliderPos = 0;
    Q_EMIT positionChanged();
    update();
}

int SeekerSlider::position() const
{
    return m_position;
}

} // namespace QmlDesigner
