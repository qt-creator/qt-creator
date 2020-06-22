/****************************************************************************
**
** Copyright (C) 2020 Uwe Kindler
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "ads_globals.h"

#include "dockmanager.h"
#include "docksplitter.h"
#include "iconprovider.h"

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
