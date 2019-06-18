/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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
#include "colorcontrol.h"

#include <QColorDialog>
#include <QEvent>
#include <QHelpEvent>
#include <QPainter>
#include <QToolTip>

namespace DesignTools {

ColorControl::ColorControl()
    : QWidget(nullptr)
    , m_color(Qt::black)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(20);
}

ColorControl::ColorControl(const QColor &color, QWidget *parent)
    : QWidget(parent)
    , m_color(color)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(20);
}

ColorControl::~ColorControl() = default;

QColor ColorControl::value() const
{
    return m_color;
}

void ColorControl::setValue(const QColor &val)
{
    m_color = val;
}

bool ColorControl::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        if (auto helpEvent = static_cast<const QHelpEvent *>(event)) {
            QToolTip::showText(helpEvent->globalPos(), m_color.name());
            return true;
        }
    }
    return QWidget::event(event);
}

void ColorControl::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), m_color);
}

void ColorControl::mouseReleaseEvent(QMouseEvent *event)
{
    QColor color = QColorDialog::getColor(m_color, this);

    event->accept();

    if (color != m_color) {
        m_color = color;
        update();
        emit valueChanged();
    }
}

void ColorControl::mousePressEvent(QMouseEvent *event)
{
    // Required if embedded in a QGraphicsProxywidget
    // in order to call mouseRelease properly.
    QWidget::mousePressEvent(event);
    event->accept();
}

} // End namespace DesignTools.
