// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <cplusplus/AST.h>
#include <cplusplus/ASTMatcher.h>
#include <cplusplus/ASTPatternBuilder.h>
#include <cplusplus/ASTVisitor.h>
#include <cplusplus/Control.h>
#include <cplusplus/CoreTypes.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Names.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TranslationUnit.h>

#include <utils/filepath.h>

#include "utils.h"

#include <QFile>
#include <QList>
#include <QCoreApplication>
#include <QStringList>
#include <QFileInfo>
#include <QTime>
#include <QDebug>

#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace CPlusPlus;

void printUsage()
{
    std::cout << "Usage: " << qPrintable(QFileInfo(QCoreApplication::arguments().at(0)).fileName())
              << " [-v] <file1> <file2> ...\n\n"
              << "Run the parser with the given files.\n";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeFirst();

    bool optionVerbose = false;

    // Process options & arguments
    if (args.contains(QLatin1String("-v"))) {
        optionVerbose = true;
        args.removeOne(QLatin1String("-v"));
    }
    const bool helpRequested = args.contains(QLatin1String("-h")) || args.contains(QLatin1String("-help"));
    if (args.isEmpty() || helpRequested) {
        printUsage();
        return helpRequested ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    // Process files
    const QStringList files = args;
    for (const QString &fileName : files) {
        // Run preprocessor
        const QString fileNamePreprocessed = fileName + QLatin1String(".preprocessed");
        CplusplusToolsUtils::SystemPreprocessor preprocessor(optionVerbose);
        preprocessor.preprocessFile(fileName, fileNamePreprocessed);

        // Run parser
        QFile file(fileNamePreprocessed);
        if (! file.open(QFile::ReadOnly)) {
            std::cerr << "Error: Could not open file \"" << qPrintable(file.fileName()) << "\"."
                      << std::endl;
            return EXIT_FAILURE;
        }
        const QByteArray source = file.readAll();
        file.close();

        Document::Ptr doc = Document::create(Utils::FilePath::fromString(fileName));
        doc->control()->setDiagnosticClient(0);
        doc->setUtf8Source(source);
        doc->parse();
    }

    return EXIT_SUCCESS;
}
