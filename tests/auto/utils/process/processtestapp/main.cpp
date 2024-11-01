// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processtestapp.h"

#include <app/app_version.h>

#include <utils/processreaper.h>
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

    const QScopeGuard cleanup([] { ProcessReaper::deleteAll(); });

    TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath() + "/"
                                                    + Core::Constants::IDE_CASED_ID + "-XXXXXX");
    SubProcessConfig::setPathToProcessTestApp(QLatin1String(PROCESS_TESTAPP));

    QMetaObject::invokeMethod(&app, [] { ProcessTestApp::invokeSubProcess(); }, Qt::QueuedConnection);
    return app.exec();
}
