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

namespace ADS {

namespace internal {

void replaceSplitterWidget(QSplitter *splitter, QWidget *from, QWidget *to)
{
    int index = splitter->indexOf(from);
    from->setParent(nullptr);
    splitter->insertWidget(index, to);
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

QPixmap createTransparentPixmap(const QPixmap &source, qreal opacity)
{
    QPixmap transparentPixmap(source.size());
    transparentPixmap.fill(Qt::transparent);
    QPainter painter(&transparentPixmap);
    painter.setOpacity(opacity);
    painter.drawPixmap(0, 0, source);
    return transparentPixmap;
}

void hideEmptyParentSplitters(DockSplitter *splitter)
{
    while (splitter && splitter->isVisible()) {
        if (!splitter->hasVisibleContent()) {
            splitter->hide();
        }
        splitter = internal::findParent<DockSplitter *>(splitter);
    }
}

void setButtonIcon(QAbstractButton *button,
                   QStyle::StandardPixmap standarPixmap,
                   ADS::eIcon customIconId)
{
    // First we try to use custom icons if available
    QIcon icon = DockManager::iconProvider().customIcon(customIconId);
    if (!icon.isNull()) {
        button->setIcon(icon);
        return;
    }

    if (Utils::HostOsInfo::isLinuxHost()) {
        button->setIcon(button->style()->standardIcon(standarPixmap));
    } else {
        // The standard icons does not look good on high DPI screens so we create
        // our own "standard" icon here.
        QPixmap normalPixmap = button->style()->standardPixmap(standarPixmap, nullptr, button);
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

    QList<QWidget*> children = widget->findChildren<QWidget *>(QString(),
        (RepolishDirectChildren == options) ? Qt::FindDirectChildrenOnly : Qt::FindChildrenRecursively);
    for (auto w : children)
    {
        w->style()->unpolish(w);
        w->style()->polish(w);
    }
}

} // namespace internal
} // namespace ADS
