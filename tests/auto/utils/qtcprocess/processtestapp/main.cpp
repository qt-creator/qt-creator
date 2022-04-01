/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "processtestapp.h"

#include <app/app_version.h>

#include <utils/launcherinterface.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>

#include <QCoreApplication>
#include <QScopeGuard>

#ifdef Q_OS_WIN
#include <crtdbg.h>
#include <cstdlib>
#endif

using namespace Utils;

int main(int argc, char **argv)
{
#ifdef Q_OS_WIN
    // avoid crash reporter dialog
    _set_error_mode(_OUT_TO_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
    QCoreApplication app(argc, argv);

    auto cleanup = qScopeGuard([] { Singleton::deleteAll(); });

    TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath() + "/"
                                                    + Core::Constants::IDE_CASED_ID + "-XXXXXX");
    const QString libExecPath(qApp->applicationDirPath() + '/'
                              + QLatin1String(TEST_RELATIVE_LIBEXEC_PATH));
    LauncherInterface::setPathToLauncher(libExecPath);
    SubProcessConfig::setPathToProcessTestApp(QLatin1String(PROCESS_TESTAPP));

    QMetaObject::invokeMethod(&app, [] { ProcessTestApp::invokeSubProcess(); }, Qt::QueuedConnection);
    return app.exec();
}
