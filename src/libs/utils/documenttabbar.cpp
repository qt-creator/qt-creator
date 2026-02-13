// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "documenttabbar.h"

#include <QMouseEvent>

/*!
    \class Utils::DocumentTabBar
    \inheaderfile utils/documenttabbar.h
    \inmodule QtCreator

    \brief The DocumentTabBar class adds handling of the middle mouse button
    for closing tabs to QTabBar.
*/

namespace Utils {

void DocumentTabBar::mousePressEvent(QMouseEvent *event)
{
    if (tabsClosable() && event->button() == Qt::MiddleButton)
        return; // do not switch tabs when closing tab is requested
    return QTabBar::mousePressEvent(event);
}

void DocumentTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (tabsClosable() && event->button() == Qt::MiddleButton) {
        int tabIndex = tabAt(event->pos());
        if (tabIndex != -1) {
            emit tabCloseRequested(tabIndex);
            return;
        }
    }
    QTabBar::mouseReleaseEvent(event);
}

} // namespace Utils
