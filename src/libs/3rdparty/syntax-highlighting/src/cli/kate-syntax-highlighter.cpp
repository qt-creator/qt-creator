/*
    Copyright (C) 2016 Volker Krause <vkrause@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ksyntaxhighlighting_version.h"

#include <definition.h>
#include <definitiondownloader.h>
#include <htmlhighlighter.h>
#include <repository.h>
#include <theme.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QVector>

#include <iostream>

using namespace KSyntaxHighlighting;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("kate-syntax-highlighter"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setApplicationVersion(QStringLiteral(SyntaxHighlighting_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(app.translate("SyntaxHighlightingCLI", "Command line syntax highlighter using Kate syntax definitions."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(app.translate("SyntaxHighlightingCLI", "source"),
                                 app.translate("SyntaxHighlightingCLI", "The source file to highlight."));

    QCommandLineOption listDefs(QStringList() << QStringLiteral("l") << QStringLiteral("list"),
                                app.translate("SyntaxHighlightingCLI", "List all available syntax definitions."));
    parser.addOption(listDefs);
    QCommandLineOption listThemes(QStringList() << QStringLiteral("list-themes"),
                                  app.translate("SyntaxHighlightingCLI", "List all available themes."));
    parser.addOption(listThemes);

    QCommandLineOption updateDefs(QStringList() << QStringLiteral("u") << QStringLiteral("update"),
                                  app.translate("SyntaxHighlightingCLI", "Download new/updated syntax definitions."));
    parser.addOption(updateDefs);

    QCommandLineOption outputName(QStringList() << QStringLiteral("o") << QStringLiteral("output"),
                                  app.translate("SyntaxHighlightingCLI", "File to write HTML output to (default: stdout)."),
                                  app.translate("SyntaxHighlightingCLI", "output"));
    parser.addOption(outputName);

    QCommandLineOption syntaxName(QStringList() << QStringLiteral("s") << QStringLiteral("syntax"),
                                  app.translate("SyntaxHighlightingCLI", "Highlight using this syntax definition (default: auto-detect based on input file)."),
                                  app.translate("SyntaxHighlightingCLI", "syntax"));
    parser.addOption(syntaxName);

    QCommandLineOption themeName(QStringList() << QStringLiteral("t") << QStringLiteral("theme"),
                                 app.translate("SyntaxHighlightingCLI", "Color theme to use for highlighting."),
                                 app.translate("SyntaxHighlightingCLI", "theme"), QStringLiteral("Default"));
    parser.addOption(themeName);

    QCommandLineOption titleOption(QStringList() << QStringLiteral("T") << QStringLiteral("title"),
                                   app.translate("SyntaxHighlightingCLI", "Set HTML page's title\n(default: the filename or \"Kate Syntax Highlighter\" if reading from stdin)."),
                                   app.translate("SyntaxHighlightingCLI", "title"));
    parser.addOption(titleOption);

    QCommandLineOption stdinOption(QStringList() << QStringLiteral("stdin"),
                                   app.translate("SyntaxHighlightingCLI", "Read file from stdin. The -s option must also be used."));
    parser.addOption(stdinOption);

    parser.process(app);

    Repository repo;
    if (parser.isSet(listDefs)) {
        foreach (const auto &def, repo.definitions()) {
            std::cout << qPrintable(def.name()) << std::endl;
        }
        return 0;
    }

    if (parser.isSet(listThemes)) {
        foreach (const auto &theme, repo.themes())
            std::cout << qPrintable(theme.name()) << std::endl;
        return 0;
    }

    if (parser.isSet(updateDefs)) {
        DefinitionDownloader downloader(&repo);
        QObject::connect(&downloader, &DefinitionDownloader::informationMessage, [](const QString &msg) {
            std::cout << qPrintable(msg) << std::endl;
        });
        QObject::connect(&downloader, &DefinitionDownloader::done, &app, &QCoreApplication::quit);
        downloader.start();
        return app.exec();
    }

    bool fromFileName = false;
    QString inFileName;
    if (parser.positionalArguments().size() == 1) {
        fromFileName = true;
        inFileName = parser.positionalArguments().at(0);
    }

    Definition def;
    if (parser.isSet(syntaxName)) {
        def = repo.definitionForName(parser.value(syntaxName));
        if (!def.isValid())
            /* see if it's a mimetype instead */
            def = repo.definitionForMimeType(parser.value(syntaxName));
    } else if (fromFileName) {
        def = repo.definitionForFileName(inFileName);
    } else {
        parser.showHelp(1);
    }

    QString title;
    if (parser.isSet(titleOption))
        title = parser.value(titleOption);

    if (!def.isValid()) {
        std::cerr << "Unknown syntax." << std::endl;
        return 1;
    }

    HtmlHighlighter highlighter;
    highlighter.setDefinition(def);
    if (parser.isSet(outputName))
        highlighter.setOutputFile(parser.value(outputName));
    else
        highlighter.setOutputFile(stdout);
    highlighter.setTheme(repo.theme(parser.value(themeName)));

    if (fromFileName) {
        highlighter.highlightFile(inFileName, title);
    } else if (parser.isSet(stdinOption)) {
        QFile inFile;
        inFile.open(stdin, QIODevice::ReadOnly);
        highlighter.highlightData(&inFile, title);
    } else {
        parser.showHelp(1);
    }

    return 0;
}
