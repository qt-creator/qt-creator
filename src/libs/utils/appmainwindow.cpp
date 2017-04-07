/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "appmainwindow.h"
#include "theme/theme_p.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <QEvent>
#include <QCoreApplication>

namespace Utils {

/* The notification signal is delayed by using a custom event
 * as otherwise device removal is not detected properly
 * (devices are still present in the registry. */

class DeviceNotifyEvent : public QEvent {
public:
    explicit DeviceNotifyEvent(int id) : QEvent(static_cast<QEvent::Type>(id)) {}
};

AppMainWindow::AppMainWindow() :
        m_deviceEventId(QEvent::registerEventType(QEvent::User + 2))
{
}

void AppMainWindow::raiseWindow()
{
    setWindowState(windowState() & ~Qt::WindowMinimized);

    raise();

    activateWindow();
}

#ifdef Q_OS_WIN
bool AppMainWindow::event(QEvent *event)
{
    const QEvent::Type type = event->type();
    if (type == m_deviceEventId) {
        event->accept();
        emit deviceChange();
        return true;
    }
    if (type == QEvent::ThemeChange)
        setThemeApplicationPalette();
    return QMainWindow::event(event);
}

bool AppMainWindow::winEvent(MSG *msg, long *result)
{
    if (msg->message == WM_DEVICECHANGE) {
        if (msg->wParam & 0x7 /* DBT_DEVNODES_CHANGED */) {
            *result = TRUE;
            QCoreApplication::postEvent(this, new DeviceNotifyEvent(m_deviceEventId));
        }
    }
    return false;
}
#endif

} // namespace Utils

