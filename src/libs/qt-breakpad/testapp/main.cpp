// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtsystemexceptionhandler.h>

#include <QCoreApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    a.setApplicationName("testapp");
    a.setApplicationVersion("1.0.0");

    QtSystemExceptionHandler exceptionHandler;

    QTimer::singleShot(100, [] { QtSystemExceptionHandler::crash(); });

    return a.exec();
}
