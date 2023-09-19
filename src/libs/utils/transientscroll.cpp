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
static constexpr char focusedPropertyName[] = "focused";
static constexpr char skipChildPropertyName[] = "qds_transient_skipChildArea";

class Utils::ScrollAreaPrivate
{
public:
    ScrollAreaPrivate(QAbstractScrollArea *area)
        : area(area)
    {
        verticalScrollBar = dynamic_cast<ScrollBar *>(area->verticalScrollBar());
        if (!verticalScrollBar) {
            verticalScrollBar = new ScrollBar(area);
            area->setVerticalScrollBar(verticalScrollBar);
        }

        horizontalScrollBar = dynamic_cast<ScrollBar *>(area->horizontalScrollBar());
        if (!horizontalScrollBar) {
            horizontalScrollBar = new ScrollBar(area);
            area->setHorizontalScrollBar(horizontalScrollBar);
        }

        if (area->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)
            area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        if (area->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)
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
    inline QPointer<ScrollBar> adjacentScrollBar(QPointer<ScrollBar> scrollBar)
    {
        if (scrollBar == verticalScrollBar)
            return horizontalScrollBar;

        if (scrollBar == horizontalScrollBar)
            return verticalScrollBar;

        return {};
    }

    inline bool checkToFlashScroll(QPointer<ScrollBar> scrollBar, const QPoint &pos)
    {
        if (scrollBar.isNull())
            return false;

        if (!scrollBar->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, scrollBar))
            return false;

        Qt::ScrollBarPolicy policy = (scrollBar->orientation() == Qt::Horizontal)
                                         ? area->horizontalScrollBarPolicy()
                                         : area->verticalScrollBarPolicy();

        if (policy == Qt::ScrollBarAlwaysOff)
            return false;

        if (scrollBarRect(scrollBar).contains(pos)) {
            scrollBar->flash();
            return true;
        }

        return false;
    }

    inline bool setAdjacentHovered(QObject *w, bool setHovered)
    {
        if (!w)
            return false;

        QPointer<ScrollBar> scrollBar;
        if (w == verticalScrollBar)
            scrollBar = verticalScrollBar;
        else if (w == horizontalScrollBar)
            scrollBar = horizontalScrollBar;

        if (!scrollBar)
            return false;

        QPointer<ScrollBar> adjacent = adjacentScrollBar(scrollBar);
        if (!adjacent)
            return false;

        return adjacent->setAdjacentHovered(setHovered);
    }

    inline bool setAdjacentVisible(QObject *changedObject, bool setVisible)
    {
        if (!changedObject)
            return false;

        QPointer<ScrollBar> scrollBar;
        if (changedObject == verticalScrollBar) {
            scrollBar = verticalScrollBar;
        } else if (changedObject == horizontalScrollBar) {
            scrollBar = horizontalScrollBar;
        } else if (changedObject == area) {
            if (setVisible && verticalScrollBar && horizontalScrollBar) {
                bool anyChange = false;
                anyChange |= verticalScrollBar->setAdjacentVisible(horizontalScrollBar->isVisible());
                anyChange |= horizontalScrollBar->setAdjacentVisible(verticalScrollBar->isVisible());
                return anyChange;
            }
        }

        if (!scrollBar)
            return false;

        QPointer<ScrollBar> adjacent = adjacentScrollBar(scrollBar);
        if (!adjacent)
            return false;

        return adjacent->setAdjacentVisible(setVisible);
    }

    inline bool checkToFlashScroll(const QPoint &pos)
    {
        bool coversScroll = checkToFlashScroll(verticalScrollBar, pos);

        if (!coversScroll)
            coversScroll |= checkToFlashScroll(horizontalScrollBar, pos);

        return coversScroll;
    }

    inline bool canSetTransientProperty(QPointer<ScrollBar> scrollBar) const
    {
        if (scrollBar.isNull())
            return false;

        if (!scrollBar->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, scrollBar))
            return false;

        Qt::ScrollBarPolicy policy = (scrollBar->orientation() == Qt::Horizontal)
                                         ? area->horizontalScrollBarPolicy()
                                         : area->verticalScrollBarPolicy();

        if (policy == Qt::ScrollBarAlwaysOff)
            return false;

        return true;
    }

    inline bool setFocus(QPointer<ScrollBar> scrollBar, const bool &focus)
    {
        if (!canSetTransientProperty(scrollBar))
            return false;

        return scrollBar->setFocused(focus);
    }

    inline bool setFocus(const bool &focus)
    {
        bool flashChanged = false;
        flashChanged |= setFocus(verticalScrollBar, focus);
        flashChanged |= setFocus(horizontalScrollBar, focus);

        return flashChanged;
    }

    inline bool setViewPortIntraction(QPointer<ScrollBar> scrollBar, const bool &hovered)
    {
        if (!canSetTransientProperty(scrollBar))
            return false;

        return scrollBar->setViewPortInteraction(hovered);
    }

    inline bool setViewPortIntraction(const bool &hovered)
    {
        bool interactionChanged = false;
        interactionChanged |= setViewPortIntraction(verticalScrollBar, hovered);
        interactionChanged |= setViewPortIntraction(horizontalScrollBar, hovered);

        return interactionChanged;
    }

    inline void installViewPort(QObject *eventHandler)
    {
        QWidget *viewPort = area->viewport();
        if (viewPort
            && viewPort != this->viewPort
            && viewPort->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, viewPort)
            && (area->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff
                || area->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)) {
            viewPort->installEventFilter(eventHandler);

            if (verticalScrollBar)
                verticalScrollBar->installEventFilter(eventHandler);

            if (horizontalScrollBar)
                horizontalScrollBar->installEventFilter(eventHandler);

            this->viewPort = viewPort;
            setViewPortIntraction(true);
        }
    }

    inline void uninstallViewPort(QObject *eventHandler) {
        if (viewPort) {
            viewPort->removeEventFilter(eventHandler);

            if (verticalScrollBar)
                verticalScrollBar->removeEventFilter(eventHandler);

            if (horizontalScrollBar)
                horizontalScrollBar->removeEventFilter(eventHandler);

            this->viewPort = nullptr;
            setViewPortIntraction(false);
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

void TransientScrollAreaSupport::supportWidget(QWidget *widget)
{
    for (QAbstractScrollArea *area : widget->findChildren<QAbstractScrollArea *>()) {
        support(area);
    }
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
    } break;
    case QEvent::Leave: {
        if (watched == d->area)
            d->uninstallViewPort(this);
    } break;
    case QEvent::MouseMove: {
        if (watched == d->viewPort) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent) {
                if (d->checkToFlashScroll(mouseEvent->pos()))
                    return true;
            }
        }
    } break;
    case QEvent::HoverEnter:
    case QEvent::HoverMove: {
        QHoverEvent *hoverEvent = static_cast<QHoverEvent *>(event);
        if (watched == d->horizontalScrollBar || watched == d->verticalScrollBar) {
            if (hoverEvent)
                d->setAdjacentHovered(watched, true);
        }
    } break;
    case QEvent::HoverLeave: {
        QHoverEvent *hoverEvent = static_cast<QHoverEvent *>(event);
        if (watched == d->horizontalScrollBar || watched == d->verticalScrollBar) {
            if (hoverEvent)
                d->setAdjacentHovered(watched, false);
        }
    } break;
    case QEvent::DynamicPropertyChange: {
        if (watched == d->area) {
            auto *pEvent = static_cast<QDynamicPropertyChangeEvent *>(event);
            if (!pEvent || pEvent->propertyName() != focusedPropertyName)
                break;

            bool focused = d->area->property(focusedPropertyName).toBool();
            d->setFocus(focused);

            if (!d->area->property(skipChildPropertyName).toBool() && d->area->viewport()) {
                const QList<QAbstractScrollArea *> scrollChildren
                    = d->area->viewport()->findChildren<QAbstractScrollArea *>();
                for (QAbstractScrollArea *area : scrollChildren) {
                    area->setProperty(skipChildPropertyName, true);
                    area->setProperty(focusedPropertyName, focused);
                    area->setProperty(skipChildPropertyName, false);
                }
            }
        }
    } break;
    case QEvent::Hide:
        d->setAdjacentVisible(watched, false);
        break;
    case QEvent::Show:
        d->setAdjacentVisible(watched, true);
        break;
    case QEvent::Resize: {
        if (watched == d->area)
            d->setAdjacentVisible(watched, true);
    } break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

class Utils::ScrollBarPrivate {
public:
    bool flashed = false;
    int flashTimer = 0;
    bool focused = false;
    bool viewPortIntraction = false;
    bool adjacentHovered = false;
    bool adjacentVisible = false;
    bool isHandleUnderCursor = false;
    bool isGrooveUnderCursor = false;
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

void ScrollBar::flash()
{
    if (!style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, this))
        return;

    if (!isEnabled()) {
        d->flashed = false;
        hide();
        return;
    }

    if (!d->flashed) {
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
    if (style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, this)) {
        if (d->flashed || d->focused || d->viewPortIntraction)
            option->state |= QStyle::State_On;

        if (d->isGrooveUnderCursor || d->isHandleUnderCursor || d->adjacentHovered)
            option->subControls |= QStyle::SC_ScrollBarGroove;

        option->styleObject->setProperty("adjacentScroll", d->adjacentHovered);

        if (d->isHandleUnderCursor)
            option->activeSubControls |= QStyle::SC_ScrollBarSlider;

        if (d->adjacentVisible) {
            int scrollExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent, option, this);
            if (option->orientation == Qt::Horizontal)
                option->rect.adjust(0, 0, -scrollExtent, 0);
            else
                option->rect.adjust(0, 0, 0, -scrollExtent);
        }
    }
}

bool ScrollBar::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverMove: {
        QHoverEvent *hoverEvent = static_cast<QHoverEvent *>(event);
        if (hoverEvent) {
            QStyleOptionSlider option;
            option.initFrom(this);
            d->isHandleUnderCursor = style()
                                         ->subControlRect(QStyle::CC_ScrollBar,
                                                          &option,
                                                          QStyle::SC_ScrollBarSlider,
                                                          this)
                                         .contains(hoverEvent->pos());

            d->isGrooveUnderCursor = !d->isHandleUnderCursor
                                     && style()
                                            ->subControlRect(QStyle::CC_ScrollBar,
                                                             &option,
                                                             QStyle::SC_ScrollBarGroove,
                                                             this)
                                            .contains(hoverEvent->pos());
        }
    } break;
    case QEvent::HoverLeave:
        d->isHandleUnderCursor = false;
        d->isGrooveUnderCursor = false;
        break;
    case QEvent::Timer:
        if (static_cast<QTimerEvent *>(event)->timerId() == d->flashTimer) {
            if (d->flashed && style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, this)) {
                d->flashed = false;
                update();
            }
            killTimer(d->flashTimer);
            d->flashTimer = 0;
            return true;
        }
        break;
    default:
        break;
    }
    return QScrollBar::event(event);
}

bool ScrollBar::setFocused(const bool &focused)
{
    if (d->focused == focused)
        return false;

    d->focused = focused;

    if (d->focused)
        flash();
    else
        update();

    return true;
}

bool ScrollBar::setAdjacentVisible(const bool &visible)
{
    if (d->adjacentVisible == visible)
        return false;

    d->adjacentVisible = visible;
    update();
    return true;
}

bool ScrollBar::setAdjacentHovered(const bool &hovered)
{
    if (d->adjacentHovered == hovered)
        return false;

    d->adjacentHovered = hovered;
    update();
    return true;
}

bool ScrollBar::setViewPortInteraction(const bool &hovered)
{
    if (d->viewPortIntraction == hovered)
        return false;

    d->viewPortIntraction = hovered;

    if (d->viewPortIntraction)
        flash();
    else
        update();

    return true;
}

void GlobalTransientSupport::support(QWidget *widget)
{
    if (!widget)
        return;

    widget->installEventFilter(instance());
    QAbstractScrollArea *area = dynamic_cast<QAbstractScrollArea *>(widget);
    if (area)
        TransientScrollAreaSupport::support(area);

    for (QWidget *childWidget : widget->findChildren<QWidget *>(QString(), Qt::FindChildOption::FindDirectChildrenOnly))
        support(childWidget);
}

GlobalTransientSupport::GlobalTransientSupport()
    : QObject(nullptr)
{}

GlobalTransientSupport *GlobalTransientSupport::instance()
{
    static GlobalTransientSupport *gVal = nullptr;
    if (!gVal)
        gVal = new GlobalTransientSupport;

    return gVal;
}

bool GlobalTransientSupport::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::ChildAdded: {
        QChildEvent *childEvent = static_cast<QChildEvent *>(event);

        if (!childEvent || !childEvent->child() || !childEvent->child()->isWidgetType())
            break;

        QWidget *widget = dynamic_cast<QWidget *>(childEvent->child());
        if (widget)
            support(widget);
    }
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}
