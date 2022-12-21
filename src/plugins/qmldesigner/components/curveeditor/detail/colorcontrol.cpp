// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "colorcontrol.h"

#include <QColorDialog>
#include <QEvent>
#include <QHelpEvent>
#include <QPainter>
#include <QToolTip>

namespace QmlDesigner {

namespace StyleEditor {

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

    if (color.isValid() && color != m_color) {
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

} // End namespace StyleEditor.

} // End namespace QmlDesigner.
