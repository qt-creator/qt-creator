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
#ifdef QT_SIMULATOR
#include <private/qsimulatorconnection_p.h>
#endif

#include <qt4nodeinstanceclientproxy.h>

#ifdef ENABLE_QT_BREAKPAD
#include <qtsystemexceptionhandler.h>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
#ifdef QT_SIMULATOR
    QtSimulatorPrivate::SimulatorConnection::createStubInstance();
#endif

    QApplication application(argc, argv);

    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setOrganizationDomain("qt-project.org");
    QCoreApplication::setApplicationName("QmlPuppet");
    QCoreApplication::setApplicationVersion("1.1.0");


    if (application.arguments().count() == 2 && application.arguments().at(1) == "--version") {
        qDebug() << QCoreApplication::applicationVersion();
        return 0;
    }

    if (application.arguments().count() != 4)
        return -1;

#ifdef ENABLE_QT_BREAKPAD
    QtSystemExceptionHandler systemExceptionHandler;
#endif

    new QmlDesigner::Qt4NodeInstanceClientProxy(&application);

#if defined(Q_OS_WIN) && defined(QT_NO_DEBUG) && !defined(ENABLE_QT_BREAKPAD)
    SetErrorMode(SEM_NOGPFAULTERRORBOX); //We do not want to see any message boxes
#endif

    return application.exec();
}
