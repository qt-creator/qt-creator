/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <QtDebug>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <QtGui/QApplication>

#include <integrationcore.h>
#include <pluginmanager.h>
#include <model.h>

#include <exception.h>
#include "mainwindow.h"

enum {
    debug = false
};

static void doStyling(QApplication& app)
{
    QString ws;
#ifdef Q_WS_MAC
    ws = "macos";
#endif // Q_WS_MAC

#ifdef Q_WS_WIN
    ws = "windows";
#endif // Q_WS_WIN

#ifdef Q_WS_X11
    ws = "x11";
#endif // Q_WS_X11

    QFile platformCssFile(QString(":/bauhaus-%1.css").arg(ws));
    platformCssFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream platformCssStream(&platformCssFile);
    QString styleSheet(platformCssStream.readAll());
    platformCssFile.close();

    QFile genericCssFile(QLatin1String(":/bauhaus.css"));
    genericCssFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream genericCssStream(&genericCssFile);
    styleSheet.append(genericCssStream.readAll());
    genericCssFile.close();

    app.setStyleSheet(styleSheet);
}

static void parseArguments(const QStringList& argumentList, MainWindow& mainWindow)
{
    QStringList passArgumentList;
    passArgumentList.append("--no-resync");
    passArgumentList.append("-h");
    passArgumentList.append("-graphicssystem");

    for (int i = 1; i < argumentList.size(); ++i) {
        if (argumentList[i].at(0) == QLatin1Char('-')) {
            if (argumentList[i] == "--file" || argumentList[i] == "-file" || argumentList[i] == "-f") {
                ++i;
                if (i < argumentList.size()) {
                    mainWindow.openFile(argumentList[i]);
                } else {
                    mainWindow.doOpen();
                }
            } else if (argumentList[i] == "--help" || argumentList[i] == "-h") {
                qWarning() << "Usage: bauhaus [OPTION...]\n";
                qWarning() << "  -f, --file      open this file";
                qWarning() << "  --no-resync     disable rewriter";
                exit(0);
            } else if (passArgumentList.contains(argumentList[i].split('=').first())) {
            } else {
                qWarning() << "bauhaus: unrecognized option "<< argumentList[i];
                qWarning() << "Try `bauhaus --help'";
                exit(1);
            }
        } else {
            mainWindow.openFile(argumentList[i]);
        }
    }
}

static QStringList pluginPaths()
{
    QStringList result;
#ifdef Q_OS_MAC
    result += QCoreApplication::applicationDirPath() + "/../PlugIns/Bauhaus/ItemLibs";
#else // Q_OS_MAC
    result += QCoreApplication::applicationDirPath() + "/../lib/itemlibs";
#endif // Q_OS_MAC

    return result;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(bauhaus);

    doStyling(app);

#ifdef Q_WS_X11
    QIcon applicationIcon;
    applicationIcon.addFile(":/16xBauhaus_Log");
    applicationIcon.addFile(":/32xBauhaus_Logo.png");
    applicationIcon.addFile(":/64xBauhaus_Logo.png");
    applicationIcon.addFile(":/128xBauhaus_Logo.png");
    applicationIcon.addFile(":/256xBauhaus_Logo.png");
    Q_ASSERT(!applicationIcon.isNull());
    app.setWindowIcon(applicationIcon);
#endif

    QCoreApplication::setOrganizationName("Nokia");
    QCoreApplication::setOrganizationDomain("nokia.com");
    QCoreApplication::setApplicationName("Bauhaus");

    try {
        QmlDesigner::IntegrationCore core;
        core.pluginManager()->setPluginPaths(pluginPaths());

        MainWindow mainWindow;
        mainWindow.show();

        parseArguments(app.arguments(), mainWindow);

//        if (mainWindow.documentCount() == 0)
//            mainWindow.showWelcomeScreen();
//
        return app.exec();

    } catch (const QmlDesigner::Exception &exception) {
        qWarning() << exception;
        return -1;
    }
}
