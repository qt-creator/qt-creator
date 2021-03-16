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

#include "iostool.h"

#include <QGuiApplication>
#include <QStringList>

int main(int argc, char *argv[])
{
    //This keeps iostool from stealing focus
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "true");
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

    QGuiApplication a(qtArgc, &qtArg);
    Ios::IosTool tool;
    tool.run(args);
    int res = a.exec();
    exit(res);
}

