// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "windowmanager.h"

#include <coreplugin/icore.h>

#include <QCursor>
#include <QGuiApplication>
#include <QMainWindow>
#include <QScreen>
#include <QWindow>

namespace QmlDesignerBase {

QPointer<WindowManager> WindowManager::m_instance = nullptr;

WindowManager::WindowManager()
{
    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &WindowManager::focusWindowChanged);
    connect(qGuiApp, &QGuiApplication::aboutToQuit, this, &WindowManager::aboutToQuit);
    connect(Core::ICore::instance()->mainWindow()->windowHandle(),
            &QWindow::visibleChanged,
            this,
            &WindowManager::mainWindowVisibleChanged);
}

void WindowManager::registerDeclarativeType()
{
    [[maybe_unused]] static const int typeIndex
        = qmlRegisterSingletonType<WindowManager>("StudioWindowManager",
                                                  1,
                                                  0,
                                                  "WindowManager",
                                                  [](QQmlEngine *, QJSEngine *) {
                                                      return new WindowManager();
                                                  });
}

WindowManager::~WindowManager() {}

QPoint WindowManager::globalCursorPosition()
{
    return QCursor::pos();
}

QRect WindowManager::getScreenGeometry(QPoint point)
{
    QScreen *screen = QGuiApplication::screenAt(point);

    if (!screen)
        return {};

    return screen->geometry();
}

} // namespace QmlDesignerBase
