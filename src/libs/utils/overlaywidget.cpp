// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "overlaywidget.h"

#include "qtcassert.h"

#include <QEvent>
#include <QPainter>

Utils::OverlayWidget::OverlayWidget(QWidget *parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    if (parent)
        attachToWidget(parent);
}

void Utils::OverlayWidget::setPaintFunction(const Utils::OverlayWidget::PaintFunction &paint)
{
    m_paint = paint;
}

bool Utils::OverlayWidget::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == parent() && ev->type() == QEvent::Resize)
        resizeToParent();
    return QWidget::eventFilter(obj, ev);
}

void Utils::OverlayWidget::paintEvent(QPaintEvent *ev)
{
    if (m_paint) {
        QPainter p(this);
        m_paint(this, p, ev);
    }
}

void Utils::OverlayWidget::attachToWidget(QWidget *parent)
{
    if (parentWidget())
        parentWidget()->removeEventFilter(this);
    setParent(parent);
    if (parent) {
        parent->installEventFilter(this);
        resizeToParent();
        raise();
    }
}

void Utils::OverlayWidget::resizeToParent()
{
    QTC_ASSERT(parentWidget(), return );
    setGeometry(QRect(QPoint(0, 0), parentWidget()->size()));
}
