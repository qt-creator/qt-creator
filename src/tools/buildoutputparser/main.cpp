// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    fprintf(stderr, "Possible compiler types: gcc, clang, msvc. Default is gcc.\n");
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
            } else if (typeString == QLatin1String("msvc")) {
                compilerType = CompilerTypeMsvc;
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
