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

#include "outputprocessor.h"

#include <utils/theme/theme_p.h>

#include <QGuiApplication>
#include <QFile>
#include <QMetaObject>
#include <QFileInfo>
#include <QStringList>
#include <QTimer>

#include <stdio.h>
#include <stdlib.h>

class DummyTheme : public Utils::Theme
{
public:
    DummyTheme() : Utils::Theme(QLatin1String("dummy"))
    {
        const QPair<QColor, QString> colorEntry(QColor(Qt::red), QLatin1String("red"));
        for (int i = 0; i < d->colors.count(); ++i)
            d->colors[i] = colorEntry;
    }
};

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
    QGuiApplication app(argc, argv);
    DummyTheme dummyTheme;
    Utils::setCreatorTheme(&dummyTheme);

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
    QTimer::singleShot(0, &cop, &CompilerOutputProcessor::start);
    return app.exec();
}
