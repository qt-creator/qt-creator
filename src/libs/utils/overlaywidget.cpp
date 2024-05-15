// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "overlaywidget.h"

#include "qtcassert.h"

#include <QEvent>
#include <QPainter>

namespace Utils::Internal {
class OverlayWidgetPrivate
{
public:
    OverlayWidget::PaintFunction m_paint;
    OverlayWidget::ResizeFunction m_resize;
};
} // namespace Utils::Internal

Utils::OverlayWidget::OverlayWidget(QWidget *parent)
    : d(new Internal::OverlayWidgetPrivate)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    if (parent)
        attachToWidget(parent);
    d->m_resize = [](QWidget *w, const QSize &size) { w->setGeometry(QRect(QPoint(0, 0), size)); };
}

Utils::OverlayWidget::~OverlayWidget() = default;

void Utils::OverlayWidget::setPaintFunction(const Utils::OverlayWidget::PaintFunction &paint)
{
    d->m_paint = paint;
}

void Utils::OverlayWidget::setResizeFunction(const ResizeFunction &resize)
{
    d->m_resize = resize;
}

bool Utils::OverlayWidget::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == parent() && ev->type() == QEvent::Resize)
        resizeToParent();
    return QWidget::eventFilter(obj, ev);
}

void Utils::OverlayWidget::paintEvent(QPaintEvent *ev)
{
    if (d->m_paint) {
        QPainter p(this);
        d->m_paint(this, p, ev);
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
    if (d->m_resize)
        d->m_resize(this, parentWidget()->size());
}
