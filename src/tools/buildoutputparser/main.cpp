/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "outputprocessor.h"

#include <QCoreApplication>
#include <QFile>
#include <QMetaObject>
#include <QFileInfo>
#include <QStringList>

#include <stdio.h>
#include <stdlib.h>

static void printUsage()
{
    fprintf(stderr, "Usage: %s [--type <compiler type>] <file>\n",
            qPrintable(QFileInfo(QCoreApplication::applicationFilePath()).fileName()));
    fprintf(stderr, "Possible compiler types: gcc, clang%s. Default is gcc.\n",
#ifdef HAS_MSVC_PARSER
            ", msvc"
#else
            ""
#endif
    );
}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList args = app.arguments().mid(1);
    QString filePath;
    CompilerType compilerType = CompilerTypeGcc;
    while (filePath.isEmpty() && !args.isEmpty()) {
        const QString currentArg = args.takeFirst();
        if (currentArg == QLatin1String("--type")) {
            if (args.isEmpty()) {
                fprintf(stderr, "Error: Option --type needs argument.\n");
                printUsage();
                return EXIT_FAILURE;
            }
            const QString typeString = args.takeFirst();
            if (typeString == QLatin1String("gcc")) {
                compilerType = CompilerTypeGcc;
            } else if (typeString == QLatin1String("clang")) {
                compilerType = CompilerTypeClang;
#ifdef HAS_MSVC_PARSER
            } else if (typeString == QLatin1String("msvc")) {
                compilerType = CompilerTypeMsvc;
#endif
            } else {
                fprintf(stderr, "Invalid compiler type '%s'.\n", qPrintable(typeString));
                printUsage();
                return EXIT_FAILURE;
            }
        } else {
            filePath = currentArg;
        }
    }

    if (filePath.isEmpty()) {
        fprintf(stderr, "Error: No file supplied.\n");
        printUsage();
        return EXIT_FAILURE;
    }

    if (!args.isEmpty())
        qDebug("Ignoring extraneous parameters '%s'.\n", qPrintable(args.join(QLatin1Char(' '))));

    QFile compilerOutputFile(filePath);
    if (!compilerOutputFile.open(QIODevice::ReadOnly)) {
        fprintf(stderr, "Error opening file '%s': %s\n", qPrintable(compilerOutputFile.fileName()),
                qPrintable(compilerOutputFile.errorString()));
        return EXIT_FAILURE;
    }
    CompilerOutputProcessor cop(compilerType, compilerOutputFile);
    QMetaObject::invokeMethod(&cop, "start", Qt::QueuedConnection);
    return app.exec();
}
