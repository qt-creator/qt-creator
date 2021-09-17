/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "launcherlogging.h"
#include "launchersockethandler.h"
#include "singleton.h"

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

    auto cleanup = qScopeGuard([] { Utils::Singleton::deleteAll(); });

    Utils::Internal::LauncherSocketHandler launcher(app.arguments().constLast());
    QTimer::singleShot(0, &launcher, &Utils::Internal::LauncherSocketHandler::start);
    return app.exec();
}
