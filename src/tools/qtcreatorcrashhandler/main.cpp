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

// Called by signal handler of qtcreator.
// Usage: $0 <name of signal causing the crash>
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QLatin1String(APPLICATION_NAME));
    app.setWindowIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical));

    // Check usage.
    Q_PID parentPid = getppid();
    QString parentExecutable = QFile::symLinkTarget(QString::fromLatin1("/proc/%1/exe")
        .arg(QString::number(parentPid)));
    if (argc > 2 || !parentExecutable.contains(QLatin1String("qtcreator"))) {
        QTextStream err(stderr);
        err << QString::fromLatin1("This crash handler will be called by Qt Creator itself. "
                                   "Do not call this manually.\n");
        return EXIT_FAILURE;
    }

    // Run.
    CrashHandler *crashHandler = new CrashHandler(parentPid, app.arguments().at(1));
    crashHandler->run();

    return app.exec();
}
