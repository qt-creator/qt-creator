/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QDebug>

#include <QApplication>
#include <QStringList>

#include <qt5nodeinstanceclientproxy.h>

#include <QQmlComponent>
#include <QQmlEngine>

#ifdef ENABLE_QT_BREAKPAD
#include <qtsystemexceptionhandler.h>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
    // Since we always render text into an FBO, we need to globally disable
    // subpixel antialiasing and instead use gray.
    qputenv("QSG_DISTANCEFIELD_ANTIALIASING", "gray");
#ifdef Q_OS_MAC //This keeps qml2puppet from stealing focus
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "true");
#endif

    QApplication application(argc, argv);

    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setOrganizationDomain("qt-project.org");
    QCoreApplication::setApplicationName("Qml2Puppet");
    QCoreApplication::setApplicationVersion("1.0.0");

    if (application.arguments().count() == 2 && application.arguments().at(1) == "--test") {
        qDebug() << QCoreApplication::applicationVersion();
        QQmlEngine engine;

        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nItem {\n}\n", QUrl::fromLocalFile("test.qml"));

        QObject *object = component.create();

        if (object) {
            qDebug() << "Basic QtQuick 2.0 working...";
        } else {
            qDebug() << "Basic QtQuick 2.0 not working...";
            qDebug() << component.errorString();
        }
        delete object;
        return 0;
    }

    if (application.arguments().count() == 2 && application.arguments().at(1) == "--version") {
        qDebug() << QCoreApplication::applicationVersion();
        return 0;
    }

    if (application.arguments().count() < 4) {
        qDebug() << "Wrong argument count: " << application.arguments().count();
        return -1;
    }



#ifdef ENABLE_QT_BREAKPAD
    QtSystemExceptionHandler systemExceptionHandler;
#endif

    new QmlDesigner::Qt5NodeInstanceClientProxy(&application);

#if defined(Q_OS_WIN) && defined(QT_NO_DEBUG)
    SetErrorMode(SEM_NOGPFAULTERRORBOX); //We do not want to see any message boxes
#endif

    return application.exec();
}
