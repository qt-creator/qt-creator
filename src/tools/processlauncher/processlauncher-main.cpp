// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "launcherlogging.h"
#include "launchersockethandler.h"

#include <utils/singleton.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qtimer.h>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>

BOOL WINAPI consoleCtrlHandler(DWORD)
{
    // Ignore Ctrl-C / Ctrl-Break. QtCreator will tell us to exit gracefully.
    return TRUE;
}
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#endif

    QCoreApplication app(argc, argv);
    if (app.arguments().size() != 2) {
        Utils::Internal::logError("Need exactly one argument (path to socket)");
        return 1;
    }

    const QScopeGuard cleanup([] { Utils::Singleton::deleteAll(); });

    Utils::Internal::LauncherSocketHandler launcher(app.arguments().constLast());
    QTimer::singleShot(0, &launcher, &Utils::Internal::LauncherSocketHandler::start);
    return app.exec();
}
