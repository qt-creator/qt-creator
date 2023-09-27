// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "ads_globals.h"

#include "dockmanager.h"
#include "docksplitter.h"
#include "iconprovider.h"

#include <utils/hostosinfo.h>

#include <QAbstractButton>
#include <QPainter>
#include <QStyle>
#include <QVariant>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include <QApplication>
#include <QFile>
#endif

namespace ADS {

namespace internal {

const int g_floatingWidgetDragStartEvent = QEvent::registerEventType();
const int g_dockedWidgetDragStartEvent = QEvent::registerEventType();

void replaceSplitterWidget(QSplitter *splitter, QWidget *from, QWidget *to)
{
    int index = splitter->indexOf(from);
    from->setParent(nullptr);
    splitter->insertWidget(index, to);
}

void hideEmptyParentSplitters(DockSplitter *splitter)
{
    while (splitter && splitter->isVisible()) {
        if (!splitter->hasVisibleContent())
            splitter->hide();

        splitter = internal::findParent<DockSplitter *>(splitter);
    }
}

DockInsertParam dockAreaInsertParameters(DockWidgetArea area)
{
    switch (area) {
    case TopDockWidgetArea:
        return DockInsertParam(Qt::Vertical, false);
    case RightDockWidgetArea:
        return DockInsertParam(Qt::Horizontal, true);
    case CenterDockWidgetArea:
    case BottomDockWidgetArea:
        return DockInsertParam(Qt::Vertical, true);
    case LeftDockWidgetArea:
        return DockInsertParam(Qt::Horizontal, false);
    default:
        DockInsertParam(Qt::Vertical, false);
    }

    return DockInsertParam(Qt::Vertical, false);
}

SideBarLocation toSideBarLocation(DockWidgetArea area)
{
    switch (area) {
    case LeftAutoHideArea:
        return SideBarLeft;
    case RightAutoHideArea:
        return SideBarRight;
    case TopAutoHideArea:
        return SideBarTop;
    case BottomAutoHideArea:
        return SideBarBottom;
    default:
        return SideBarNone;
    }

    return SideBarNone;
}

bool isHorizontalSideBarLocation(SideBarLocation location)
{
    switch (location) {
    case SideBarTop:
    case SideBarBottom:
        return true;
    case SideBarLeft:
    case SideBarRight:
        return false;
    default:
        return false;
    }

    return false;
}

bool isSideBarArea(DockWidgetArea area)
{
    return toSideBarLocation(area) != SideBarNone;
}

QPixmap createTransparentPixmap(const QPixmap &source, qreal opacity)
{
    QPixmap transparentPixmap(source.size());
    transparentPixmap.fill(Qt::transparent);
    QPainter painter(&transparentPixmap);
    painter.setOpacity(opacity);
    painter.drawPixmap(0, 0, source);
    return transparentPixmap;
}

void setButtonIcon(QAbstractButton *button,
                   QStyle::StandardPixmap standardPixmap,
                   ADS::eIcon customIconId)
{
    // First we try to use custom icons if available
    QIcon icon = DockManager::iconProvider().customIcon(customIconId);
    if (!icon.isNull()) {
        button->setIcon(icon);
        return;
    }

    if (Utils::HostOsInfo::isAnyUnixHost() && !Utils::HostOsInfo::isMacHost()) {
        button->setIcon(button->style()->standardIcon(standardPixmap));
    } else {
        // The standard icons does not look good on high DPI screens so we create
        // our own "standard" icon here.
        QPixmap normalPixmap = button->style()->standardPixmap(standardPixmap, nullptr, button);
        icon.addPixmap(internal::createTransparentPixmap(normalPixmap, 0.25), QIcon::Disabled);
        icon.addPixmap(normalPixmap, QIcon::Normal);
        button->setIcon(icon);
    }
}

void repolishStyle(QWidget *widget, eRepolishChildOptions options)
{
    if (!widget)
        return;

    widget->style()->unpolish(widget);
    widget->style()->polish(widget);

    if (RepolishIgnoreChildren == options)
        return;

    QList<QWidget *> children = widget->findChildren<QWidget *>(QString(),
                                                                (RepolishDirectChildren == options)
                                                                    ? Qt::FindDirectChildrenOnly
                                                                    : Qt::FindChildrenRecursively);
    for (auto w : children) {
        w->style()->unpolish(w);
        w->style()->polish(w);
    }
}

QRect globalGeometry(QWidget *w)
{
    QRect g = w->geometry();
    g.moveTopLeft(w->mapToGlobal(QPoint(0, 0)));
    return g;
}

} // namespace internal
} // namespace ADS
