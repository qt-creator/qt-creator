// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mainwidget.h"
#include "dumpsender.h"

#include <QApplication>
#include <QFileInfo>
#include <QHostInfo>
#include <QNetworkProxyFactory>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    if (application.arguments().count() <= 1)
        return 0;

    const QString dumpPath = QApplication::arguments().at(1);
    if (!QFileInfo(dumpPath).exists())
        return 0;

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    QHostInfo hostInfo = QHostInfo::fromName("crashes.qt.io");

//    if (hostInfo.error() != QHostInfo::NoError)
//        return 0;

    DumpSender dumpSender;

    MainWidget mainWindow;

    mainWindow.setProgressbarMaximum(dumpSender.dumperSize());

    QObject::connect(&mainWindow, &MainWidget::restartCrashedApplication,
                     &dumpSender, &DumpSender::restartCrashedApplication);
    QObject::connect(&mainWindow, &MainWidget::restartCrashedApplicationAndSendDump,
                     &dumpSender, &DumpSender::restartCrashedApplicationAndSendDump);
    QObject::connect(&mainWindow, &MainWidget::sendDump,
                     &dumpSender, &DumpSender::sendDumpAndQuit);
    QObject::connect(&mainWindow, &MainWidget::commentChanged,
                     &dumpSender, &DumpSender::setCommentText);
    QObject::connect(&mainWindow, &MainWidget::emailAdressChanged,
                     &dumpSender, &DumpSender::setEmailAddress);
    QObject::connect(&dumpSender, &DumpSender::uploadProgress,
                     &mainWindow, &MainWidget::updateProgressBar);

    mainWindow.show();
    return application.exec();
}
