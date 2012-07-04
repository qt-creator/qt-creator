/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "crashhandler.h"
#include "utils.h"

#include <QApplication>
#include <QFile>
#include <QProcess>
#include <QString>
#include <QStyle>
#include <QTextStream>

#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QLatin1String(APPLICATION_NAME));
    app.setWindowIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical));

    // Check usage.
    Q_PID parentPid = getppid();
    QString parentExecutable = QFile::symLinkTarget(QString::fromLatin1("/proc/%1/exe")
        .arg(QString::number(parentPid)));
    if (argc > 1 || !parentExecutable.contains("qtcreator")) {
        QTextStream err(stderr);
        err << QString::fromLatin1("This crash handler will be called by Qt Creator itself. "
                                   "Don't call this manually.\n");
        return EXIT_FAILURE;
    }

    // Run.
    CrashHandler *crashHandler = new CrashHandler(parentPid);
    crashHandler->run();

    return app.exec();
}
