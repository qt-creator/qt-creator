/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "ksyntaxhighlighting_version.h"

#include <ansihighlighter.h>
#include <definition.h>
#include <definitiondownloader.h>
#include <htmlhighlighter.h>
#include <repository.h>
#include <theme.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>

#include <iostream>

using namespace KSyntaxHighlighting;

template<class Highlighter, class... Ts>
static void applyHighlighter(Highlighter &highlighter,
                             QCommandLineParser &parser,
                             bool fromFileName,
                             const QString &inFileName,
                             const QCommandLineOption &stdinOption,
                             const QCommandLineOption &outputName,
                             const Ts &...highlightParams)
{
    if (parser.isSet(outputName))
        highlighter.setOutputFile(parser.value(outputName));
    else
        highlighter.setOutputFile(stdout);

    if (fromFileName) {
        highlighter.highlightFile(inFileName, highlightParams...);
    } else if (parser.isSet(stdinOption)) {
        QFile inFile;
        inFile.open(stdin, QIODevice::ReadOnly);
        highlighter.highlightData(&inFile, highlightParams...);
    } else {
        parser.showHelp(1);
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("kate-syntax-highlighter"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setApplicationVersion(QStringLiteral(SyntaxHighlighting_VERSION_STRING));

    Repository repo;

    QCommandLineParser parser;
    parser.setApplicationDescription(app.translate("SyntaxHighlightingCLI", "Command line syntax highlighter using Kate syntax definitions."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(app.translate("SyntaxHighlightingCLI", "source"), app.translate("SyntaxHighlightingCLI", "The source file to highlight."));

    QCommandLineOption listDefs(QStringList() << QStringLiteral("l") << QStringLiteral("list"),
                                app.translate("SyntaxHighlightingCLI", "List all available syntax definitions."));
    parser.addOption(listDefs);
    QCommandLineOption listThemes(QStringList() << QStringLiteral("list-themes"), app.translate("SyntaxHighlightingCLI", "List all available themes."));
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
                                 app.translate("SyntaxHighlightingCLI", "theme"),
                                 repo.defaultTheme(Repository::LightTheme).name());
    parser.addOption(themeName);

    QCommandLineOption outputFormatOption(
        QStringList() << QStringLiteral("f") << QStringLiteral("output-format"),
        app.translate("SyntaxHighlightingCLI", "Use the specified format instead of html. Must be html, ansi or ansi256Colors."),
        app.translate("SyntaxHighlightingCLI", "format"),
        QStringLiteral("html"));
    parser.addOption(outputFormatOption);

    QCommandLineOption traceOption(QStringList() << QStringLiteral("syntax-trace"),
                                   app.translate("SyntaxHighlightingCLI",
                                                 "Add information to debug a syntax file. Only works with --output-format=ansi or ansi256Colors. Possible "
                                                 "values are format, region, context and stackSize."),
                                   app.translate("SyntaxHighlightingCLI", "type"));
    parser.addOption(traceOption);

    QCommandLineOption noAnsiEditorBg(QStringList() << QStringLiteral("b") << QStringLiteral("no-ansi-background"),
                                      app.translate("SyntaxHighlightingCLI", "Disable ANSI background for the default color."));
    parser.addOption(noAnsiEditorBg);

    QCommandLineOption titleOption(
        QStringList() << QStringLiteral("T") << QStringLiteral("title"),
        app.translate("SyntaxHighlightingCLI", "Set HTML page's title\n(default: the filename or \"Kate Syntax Highlighter\" if reading from stdin)."),
        app.translate("SyntaxHighlightingCLI", "title"));
    parser.addOption(titleOption);

    QCommandLineOption stdinOption(QStringList() << QStringLiteral("stdin"),
                                   app.translate("SyntaxHighlightingCLI", "Read file from stdin. The -s option must also be used."));
    parser.addOption(stdinOption);

    parser.process(app);

    if (parser.isSet(listDefs)) {
        for (const auto &def : repo.definitions()) {
            std::cout << qPrintable(def.name()) << std::endl;
        }
        return 0;
    }

    if (parser.isSet(listThemes)) {
        for (const auto &theme : repo.themes())
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
        const QString syntax = parser.value(syntaxName);
        def = repo.definitionForName(syntax);
        if (!def.isValid()) {
            /* see if it's a mimetype instead */
            def = repo.definitionForMimeType(syntax);
            if (!def.isValid()) {
                /* see if it's a extension instead */
                def = repo.definitionForFileName(QLatin1String("f.") + syntax);
                if (!def.isValid())
                    /* see if it's a filename instead */
                    def = repo.definitionForFileName(syntax);
            }
        }
    } else if (fromFileName) {
        def = repo.definitionForFileName(inFileName);
    } else {
        parser.showHelp(1);
    }

    if (!def.isValid()) {
        std::cerr << "Unknown syntax." << std::endl;
        return 1;
    }

    QString outputFormat = parser.value(outputFormatOption);
    if (0 == outputFormat.compare(QLatin1String("html"), Qt::CaseInsensitive)) {
        QString title;
        if (parser.isSet(titleOption))
            title = parser.value(titleOption);

        HtmlHighlighter highlighter;
        highlighter.setDefinition(def);
        highlighter.setTheme(repo.theme(parser.value(themeName)));
        applyHighlighter(highlighter, parser, fromFileName, inFileName, stdinOption, outputName, title);
    } else {
        auto AnsiFormat = AnsiHighlighter::AnsiFormat::TrueColor;
        if (0 == outputFormat.compare(QLatin1String("ansi256Colors"), Qt::CaseInsensitive)) {
            AnsiFormat = AnsiHighlighter::AnsiFormat::XTerm256Color;
        } else if (0 != outputFormat.compare(QLatin1String("ansi"), Qt::CaseInsensitive)) {
            std::cerr << "Unknown output format." << std::endl;
            return 2;
        }

        auto debugOptions = AnsiHighlighter::TraceOptions();
        if (parser.isSet(traceOption)) {
            const auto options = parser.values(traceOption);
            for (auto const &option : options) {
                if (option == QStringLiteral("format")) {
                    debugOptions |= AnsiHighlighter::TraceOption::Format;
                } else if (option == QStringLiteral("region")) {
                    debugOptions |= AnsiHighlighter::TraceOption::Region;
                } else if (option == QStringLiteral("context")) {
                    debugOptions |= AnsiHighlighter::TraceOption::Context;
                } else if (option == QStringLiteral("stackSize")) {
                    debugOptions |= AnsiHighlighter::TraceOption::StackSize;
                } else {
                    std::cerr << "Unknown trace name." << std::endl;
                    return 2;
                }
            }
        }

        AnsiHighlighter highlighter;
        highlighter.setDefinition(def);
        highlighter.setTheme(repo.theme(parser.value(themeName)));
        applyHighlighter(highlighter, parser, fromFileName, inFileName, stdinOption, outputName, AnsiFormat, !parser.isSet(noAnsiEditorBg), debugOptions);
    }

    return 0;
}
