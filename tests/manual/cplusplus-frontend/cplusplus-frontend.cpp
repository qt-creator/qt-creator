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

#include <AST.h>
#include <ASTVisitor.h>
#include <ASTPatternBuilder.h>
#include <ASTMatcher.h>
#include <Control.h>
#include <Scope.h>
#include <TranslationUnit.h>
#include <Literals.h>
#include <Symbols.h>
#include <Names.h>
#include <CoreTypes.h>
#include <CppDocument.h>

#include "cplusplus-tools-utils.h"

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
