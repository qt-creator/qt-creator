// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iostool.h"

#include <QCoreApplication>
#include <QStringList>

int main(int argc, char *argv[])
{
    // Make sure that our runloop uses the CFRunLoop dispatcher.
    // Otherwise the MobileDevice.Framework notifications won't work.
    qputenv("QT_EVENT_DISPATCHER_CORE_FOUNDATION", "1");

    // We do not pass the real arguments to QCoreApplication because this wrapper needs to be able
    // to forward arguments like -qmljsdebugger=... that are filtered by QCoreApplication
    QStringList args;
    for (int iarg = 0; iarg < argc ; ++iarg)
        args << QString::fromLocal8Bit(argv[iarg]);
    char *qtArg = 0;
    int qtArgc = 0;
    if (argc > 0) {
        qtArg = argv[0];
        qtArgc = 1;
    }

    QCoreApplication a(qtArgc, &qtArg);
    QCoreApplication::setApplicationName("iostool");
    QCoreApplication::setApplicationVersion("1.0");

    Ios::IosTool tool;
    tool.run(args);
    int res = a.exec();
    exit(res);
}

