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
    std::cout << "Usage: " << qPrintable(QFileInfo(qApp->arguments().at(0)).fileName())
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
    foreach (const QString &fileName, files) {
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

        Document::Ptr doc = Document::create(fileName);
        doc->control()->setDiagnosticClient(0);
        doc->setUtf8Source(source);
        doc->parse();
    }

    return EXIT_SUCCESS;
}
