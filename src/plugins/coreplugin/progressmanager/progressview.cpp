// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "progressview.h"

#include "../coreplugintr.h"

#include <utils/icon.h>
#include <utils/overlaywidget.h>

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

using namespace Utils;

const int PIN_SIZE = 12;

namespace Core::Internal {

ProgressView::ProgressView(QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout;
    setLayout(m_layout);
    m_layout->setContentsMargins(0, 0, 0, 1);
    m_layout->setSpacing(0);
    m_layout->setSizeConstraint(QLayout::SetFixedSize);
    setWindowTitle(Tr::tr("Processes"));

    auto pinButton = new OverlayWidget(this);
    pinButton->attachToWidget(this);
    pinButton->setAttribute(Qt::WA_TransparentForMouseEvents, false); // override OverlayWidget
    pinButton->setPaintFunction([](QWidget *that, QPainter &p, QPaintEvent *) {
        static const QIcon icon = Icon({{":/utils/images/pinned_small.png", Theme::IconsBaseColor}},
                                        Icon::Tint)
                                       .icon();
        QRect iconRect(0, 0, PIN_SIZE, PIN_SIZE);
        iconRect.moveTopRight(that->rect().topRight());
        icon.paint(&p, iconRect);
    });
    pinButton->setVisible(false);
    pinButton->installEventFilter(this);
    m_pinButton = pinButton;
}

ProgressView::~ProgressView() = default;

void ProgressView::addProgressWidget(QWidget *widget)
{
    m_layout->insertWidget(0, widget);
    m_pinButton->raise();
}

void ProgressView::removeProgressWidget(QWidget *widget)
{
    m_layout->removeWidget(widget);
}

bool ProgressView::isHovered() const
{
    return m_hovered;
}

void ProgressView::setReferenceWidget(QWidget *widget)
{
    if (m_referenceWidget)
        removeEventFilter(this);
    m_referenceWidget = widget;
    if (m_referenceWidget)
        installEventFilter(this);
    m_anchorBottomRight = {};
    reposition();
}

bool ProgressView::event(QEvent *event)
{
    if (event->type() == QEvent::ParentAboutToChange && parentWidget()) {
        parentWidget()->removeEventFilter(this);
    } else if (event->type() == QEvent::ParentChange && parentWidget()) {
        parentWidget()->installEventFilter(this);
    } else if (event->type() == QEvent::Resize) {
        reposition();
    } else if (event->type() == QEvent::Enter) {
        m_hovered = true;
        if (m_anchorBottomRight != QPoint())
            m_pinButton->setVisible(true);
        emit hoveredChanged(m_hovered);
    } else if (event->type() == QEvent::Leave) {
        m_hovered = false;
        m_pinButton->setVisible(false);
        emit hoveredChanged(m_hovered);
    } else if (event->type() == QEvent::Show) {
        m_anchorBottomRight = {}; // reset temporarily user-moved progress details
        reposition();
    }
    return QWidget::event(event);
}

bool ProgressView::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == parentWidget() || obj == m_referenceWidget) && event->type() == QEvent::Resize)
        reposition();
    if (obj == m_pinButton && event->type() == QEvent::MouseButtonRelease) {
        auto me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton
            && QRectF(m_pinButton->width() - PIN_SIZE, 0, PIN_SIZE, PIN_SIZE)
                   .contains(me->position())) {
            me->accept();
            m_anchorBottomRight = {};
            reposition();
        }
    }
    return false;
}

void ProgressView::mousePressEvent(QMouseEvent *ev)
{
    if ((ev->buttons() & Qt::LeftButton) && parentWidget() && m_referenceWidget) {
        m_clickPosition = ev->globalPosition();
        m_clickPositionInWidget = ev->position();
    } else {
        m_clickPosition.reset();
    }
    QWidget::mousePressEvent(ev);
}

static QPoint boundedInParent(QWidget *widget, const QPoint &pos, QWidget *parent)
{
    QPoint bounded = pos;
    bounded.setX(std::max(widget->rect().width(), std::min(bounded.x(), parent->width())));
    bounded.setY(std::max(widget->rect().height(), std::min(bounded.y(), parent->height())));
    return bounded;
}

void ProgressView::mouseMoveEvent(QMouseEvent *ev)
{
    if (m_clickPosition) {
        const QPointF current = ev->globalPosition();
        if (m_isDragging
            || (current - *m_clickPosition).manhattanLength() > QApplication::startDragDistance()) {
            m_isDragging = true;
            const QPointF newGlobal = current - m_clickPositionInWidget;
            const QPoint bottomRightInParent = parentWidget()->mapFromGlobal(newGlobal).toPoint()
                                               + rect().bottomRight();
            m_anchorBottomRight = boundedInParent(this, bottomRightInParent, parentWidget())
                                  - topRightReferenceInParent();
            if (m_anchorBottomRight.manhattanLength() <= QApplication::startDragDistance())
                m_anchorBottomRight = {};
            QMetaObject::invokeMethod(this, [this] { reposition(); });
        }
    }
    QWidget::mouseMoveEvent(ev);
}

void ProgressView::mouseReleaseEvent(QMouseEvent *ev)
{
    if ((ev->buttons() & Qt::LeftButton)) {
        m_clickPosition.reset();
        m_isDragging = false;
    }
    QWidget::mouseReleaseEvent(ev);
}

void ProgressView::reposition()
{
    if (!parentWidget() || !m_referenceWidget)
        return;

    m_pinButton->setVisible(m_anchorBottomRight != QPoint() && m_hovered);

    move(boundedInParent(this, topRightReferenceInParent() + m_anchorBottomRight, parentWidget())
         - rect().bottomRight());
}

QPoint ProgressView::topRightReferenceInParent() const
{
    if (!parentWidget() || !m_referenceWidget)
        return {};
    return m_referenceWidget->mapTo(parentWidget(), m_referenceWidget->rect().topRight());
}

} // Core::Internal
