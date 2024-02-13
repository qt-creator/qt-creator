/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "ksyntaxhighlighting_version.h"

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/DefinitionDownloader>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>
#include <ansihighlighter.h>
#include <htmlhighlighter.h>

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
                             const QCommandLineOption &outputName,
                             const Ts &...highlightParams)
{
    if (parser.isSet(outputName)) {
        highlighter.setOutputFile(parser.value(outputName));
    } else {
        highlighter.setOutputFile(stdout);
    }

    if (fromFileName) {
        highlighter.highlightFile(inFileName, highlightParams...);
    } else {
        QFile inFile;
        inFile.open(stdin, QIODevice::ReadOnly);
        highlighter.highlightData(&inFile, highlightParams...);
    }
}

static Theme theme(const Repository &repo, const QString &themeName, Repository::DefaultTheme t)
{
    if (themeName.isEmpty()) {
        return repo.defaultTheme(t);
    }
    return repo.theme(themeName);
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("ksyntaxhighlighter"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setApplicationVersion(QStringLiteral(KSYNTAXHIGHLIGHTING_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(app.translate("SyntaxHighlightingCLI", "Command line syntax highlighter using KSyntaxHighlighting syntax definitions."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(
        app.translate("SyntaxHighlightingCLI", "source"),
        app.translate("SyntaxHighlightingCLI", "The source file to highlight. If absent, read the file from stdin and the --syntax option must be used."));

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
                                 app.translate("SyntaxHighlightingCLI", "theme"));
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
                                                 "values are format, region, context, stackSize and all."),
                                   app.translate("SyntaxHighlightingCLI", "type"));
    parser.addOption(traceOption);

    QCommandLineOption noAnsiEditorBg(QStringList() << QStringLiteral("b") << QStringLiteral("no-ansi-background"),
                                      app.translate("SyntaxHighlightingCLI", "Disable ANSI background for the default color."));
    parser.addOption(noAnsiEditorBg);

    QCommandLineOption unbufferedAnsi(QStringList() << QStringLiteral("U") << QStringLiteral("unbuffered"),
                                      app.translate("SyntaxHighlightingCLI", "For ansi and ansi256Colors formats, flush the output buffer on each line."));
    parser.addOption(unbufferedAnsi);

    QCommandLineOption titleOption(
        QStringList() << QStringLiteral("T") << QStringLiteral("title"),
        app.translate("SyntaxHighlightingCLI", "Set HTML page's title\n(default: the filename or \"KSyntaxHighlighter\" if reading from stdin)."),
        app.translate("SyntaxHighlightingCLI", "title"));
    parser.addOption(titleOption);

    parser.process(app);

    Repository repo;

    if (parser.isSet(listDefs)) {
        for (const auto &def : repo.definitions()) {
            std::cout << qPrintable(def.name()) << std::endl;
        }
        return 0;
    }

    if (parser.isSet(listThemes)) {
        for (const auto &theme : repo.themes()) {
            std::cout << qPrintable(theme.name()) << std::endl;
        }
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
                if (!def.isValid()) {
                    /* see if it's a filename instead */
                    def = repo.definitionForFileName(syntax);
                }
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

    const QString outputFormat = parser.value(outputFormatOption);
    if (0 == outputFormat.compare(QLatin1String("html"), Qt::CaseInsensitive)) {
        QString title;
        if (parser.isSet(titleOption)) {
            title = parser.value(titleOption);
        }

        HtmlHighlighter highlighter;
        highlighter.setDefinition(def);
        highlighter.setTheme(theme(repo, parser.value(themeName), Repository::LightTheme));
        applyHighlighter(highlighter, parser, fromFileName, inFileName, outputName, title);
    } else {
        auto AnsiFormat = AnsiHighlighter::AnsiFormat::TrueColor;
        if (0 == outputFormat.compare(QLatin1String("ansi256Colors"), Qt::CaseInsensitive)) {
            AnsiFormat = AnsiHighlighter::AnsiFormat::XTerm256Color;
        } else if (0 != outputFormat.compare(QLatin1String("ansi"), Qt::CaseInsensitive)) {
            std::cerr << "Unknown output format." << std::endl;
            return 2;
        }

        AnsiHighlighter::Options options{};
        options |= parser.isSet(noAnsiEditorBg) ? AnsiHighlighter::Option::NoOptions : AnsiHighlighter::Option::UseEditorBackground;
        options |= parser.isSet(unbufferedAnsi) ? AnsiHighlighter::Option::Unbuffered : AnsiHighlighter::Option::NoOptions;
        if (parser.isSet(traceOption)) {
            const auto traceOptions = parser.values(traceOption);
            for (auto const &option : traceOptions) {
                if (option == QStringLiteral("format")) {
                    options |= AnsiHighlighter::Option::TraceFormat;
                } else if (option == QStringLiteral("region")) {
                    options |= AnsiHighlighter::Option::TraceRegion;
                } else if (option == QStringLiteral("context")) {
                    options |= AnsiHighlighter::Option::TraceContext;
                } else if (option == QStringLiteral("stackSize")) {
                    options |= AnsiHighlighter::Option::TraceStackSize;
                } else if (option == QStringLiteral("all")) {
                    options |= AnsiHighlighter::Option::TraceAll;
                } else {
                    std::cerr << "Unknown trace name." << std::endl;
                    return 2;
                }
            }
        }

        AnsiHighlighter highlighter;
        highlighter.setDefinition(def);
        highlighter.setTheme(theme(repo, parser.value(themeName), Repository::DarkTheme));
        applyHighlighter(highlighter, parser, fromFileName, inFileName, outputName, AnsiFormat, options);
    }

    return 0;
}
