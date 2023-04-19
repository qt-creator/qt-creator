// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "transientscroll.h"

#include <QAbstractScrollArea>
#include <QMouseEvent>
#include <QPointer>
#include <QStyle>
#include <QStyleOption>

using namespace Utils;

static constexpr char transientScrollAreaSupportName[] = "transientScrollAreSupport";

class Utils::ScrollAreaPrivate
{
public:
    ScrollAreaPrivate(QAbstractScrollArea *area)
        : area(area)
    {
        verticalScrollBar = new ScrollBar(area);
        area->setVerticalScrollBar(verticalScrollBar);

        horizontalScrollBar = new ScrollBar(area);
        area->setHorizontalScrollBar(horizontalScrollBar);

        area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    inline QRect scrollBarRect(ScrollBar *scrollBar)
    {
        QRect rect = viewPort ? viewPort->rect() : area->rect();
        if (scrollBar->orientation() == Qt::Vertical) {
            int mDiff = rect.width() - scrollBar->sizeHint().width();
            return rect.adjusted(mDiff, 0, mDiff, 0);
        } else {
            int mDiff = rect.height() - scrollBar->sizeHint().height();
            return rect.adjusted(0, mDiff, 0, mDiff);
        }
    }

    inline bool checkToFlashScroll(QPointer<ScrollBar> scrollBar, const QPoint &pos)
    {
        if (scrollBar.isNull())
            return false;

        if (!scrollBar->style()->styleHint(
                QStyle::SH_ScrollBar_Transient,
                nullptr, scrollBar))
            return false;

        if (scrollBarRect(scrollBar).contains(pos)) {
            scrollBar->flash();
            return true;
        }

        return false;
    }

    inline bool checkToFlashScroll(const QPoint &pos)
    {
        bool coversScroll = checkToFlashScroll(verticalScrollBar, pos);
        if (!coversScroll)
            coversScroll |= checkToFlashScroll(horizontalScrollBar, pos);

        return coversScroll;
    }

    inline void installViewPort(QObject *eventHandler) {
        QWidget *viewPort = area->viewport();
        if (viewPort
            && viewPort != this->viewPort
            && viewPort->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, viewPort)
            && (area->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff
                || area->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)) {
            viewPort->installEventFilter(eventHandler);
            this->viewPort = viewPort;
        }
    }

    inline void uninstallViewPort(QObject *eventHandler) {
        if (viewPort) {
            viewPort->removeEventFilter(eventHandler);
            this->viewPort = nullptr;
        }
    }

    QAbstractScrollArea *area = nullptr;
    QPointer<QWidget> viewPort = nullptr;
    QPointer<ScrollBar> verticalScrollBar;
    QPointer<ScrollBar> horizontalScrollBar;
};

TransientScrollAreaSupport::TransientScrollAreaSupport(QAbstractScrollArea *scrollArea)
    : QObject(scrollArea)
    , d(new ScrollAreaPrivate(scrollArea))
{
    scrollArea->installEventFilter(this);
}

void TransientScrollAreaSupport::support(QAbstractScrollArea *scrollArea)
{
    QObject *prevSupport = scrollArea->property(transientScrollAreaSupportName)
                               .value<QObject *>();
    if (!prevSupport)
        scrollArea->setProperty(transientScrollAreaSupportName,
                                QVariant::fromValue(
                                    new TransientScrollAreaSupport(scrollArea))
                                );
}

TransientScrollAreaSupport::~TransientScrollAreaSupport()
{
    delete d;
}

bool TransientScrollAreaSupport::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Enter: {
        if (watched == d->area)
            d->installViewPort(this);
    }
    break;
    case QEvent::Leave: {
        if (watched == d->area)
            d->uninstallViewPort(this);
    }
    break;
    case QEvent::MouseMove: {
        if (watched == d->viewPort){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent) {
                if (d->checkToFlashScroll(mouseEvent->pos()))
                    return true;
            }
        }
    }
    break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

class Utils::ScrollBarPrivate {
public:
    bool flashed = false;
    int flashTimer = 0;
};

ScrollBar::ScrollBar(QWidget *parent)
    : QScrollBar(parent)
    , d(new ScrollBarPrivate)
{
}

ScrollBar::~ScrollBar()
{
    delete d;
}

QSize ScrollBar::sizeHint() const
{
    QSize sh = QScrollBar::sizeHint();
    if (style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, this)) {
        constexpr int thickness = 10;
        if (orientation() == Qt::Horizontal)
            sh.setHeight(thickness);
        else
            sh.setWidth(thickness);
    } else {
        constexpr int thickness = 12;
        if (orientation() == Qt::Horizontal)
            sh.setHeight(thickness);
        else
            sh.setWidth(thickness);
    }
    return sh;
}

void ScrollBar::flash()
{
    if (!d->flashed && style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, this)) {
        d->flashed = true;
        if (!isVisible())
            show();
        else
            update();
    }
    if (!d->flashTimer)
        d->flashTimer = startTimer(0);
}

void ScrollBar::initStyleOption(QStyleOptionSlider *option) const
{
    QScrollBar::initStyleOption(option);

    if (d->flashed && style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, this))
        option->state |= QStyle::State_On;
}

bool ScrollBar::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Timer:
        if (static_cast<QTimerEvent *>(event)->timerId() == d->flashTimer) {
            if (d->flashed && style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, this)) {
                d->flashed = false;
                update();
            }
            killTimer(d->flashTimer);
            d->flashTimer = 0;
        }
        break;
    default:
        break;
    }
    return QScrollBar::event(event);
}
