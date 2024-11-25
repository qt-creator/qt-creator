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

#include <cstdio>

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

    QCommandLineOption outputFormatOption(QStringList() << QStringLiteral("f") << QStringLiteral("output-format"),
                                          app.translate("SyntaxHighlightingCLI", "Use the specified format instead of html. Must be html, ansi or ansi256."),
                                          app.translate("SyntaxHighlightingCLI", "format"),
                                          QStringLiteral("html"));
    parser.addOption(outputFormatOption);

    QCommandLineOption traceOption(QStringList() << QStringLiteral("syntax-trace"),
                                   app.translate("SyntaxHighlightingCLI",
                                                 "Add information to debug a syntax file. Only works with --output-format=ansi or ansi256. Possible "
                                                 "values are format, region, context, stackSize and all."),
                                   app.translate("SyntaxHighlightingCLI", "type"));
    parser.addOption(traceOption);

    QCommandLineOption noAnsiEditorBg(QStringList() << QStringLiteral("b") << QStringLiteral("no-ansi-background"),
                                      app.translate("SyntaxHighlightingCLI", "Disable ANSI background for the default color."));
    parser.addOption(noAnsiEditorBg);

    QCommandLineOption bgRole(QStringList() << QStringLiteral("B") << QStringLiteral("background-role"),
                              app.translate("SyntaxHighlightingCLI", "Select background color role from theme."),
                              app.translate("SyntaxHighlightingCLI", "role"));
    parser.addOption(bgRole);

    QCommandLineOption unbufferedAnsi(QStringList() << QStringLiteral("U") << QStringLiteral("unbuffered"),
                                      app.translate("SyntaxHighlightingCLI", "For ansi and ansi256 formats, flush the output buffer on each line."));
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
            fprintf(stdout, "%s\n", qPrintable(def.name()));
        }
        return 0;
    }

    if (parser.isSet(listThemes)) {
        for (const auto &theme : repo.themes()) {
            fprintf(stdout, "%s\n", qPrintable(theme.name()));
        }
        return 0;
    }

    Theme::EditorColorRole bgColorRole = Theme::BackgroundColor;

    if (parser.isSet(bgRole)) {
        /*
         * Theme::EditorColorRole contains border, foreground and background colors.
         * To ensure that only the background colors used in text editing are used,
         * QMetaEnum is avoided and values are listed in hard.
         */

        struct BgRole {
            QStringView name;
            Theme::EditorColorRole role;
            // name for display
            const char *asciiName;
        };

#define BG_ROLE(role)                                                                                                                                          \
    BgRole                                                                                                                                                     \
    {                                                                                                                                                          \
        QStringView(u"" #role, sizeof(#role) - 1), Theme::role, #role                                                                                          \
    }
        constexpr BgRole bgRoles[] = {
            BG_ROLE(BackgroundColor),
            BG_ROLE(TextSelection),
            BG_ROLE(CurrentLine),
            BG_ROLE(SearchHighlight),
            BG_ROLE(ReplaceHighlight),
            BG_ROLE(BracketMatching),
            BG_ROLE(CodeFolding),
            BG_ROLE(MarkBookmark),
            BG_ROLE(MarkBreakpointActive),
            BG_ROLE(MarkBreakpointReached),
            BG_ROLE(MarkBreakpointDisabled),
            BG_ROLE(MarkExecution),
            BG_ROLE(MarkWarning),
            BG_ROLE(MarkError),
            BG_ROLE(TemplateBackground),
            BG_ROLE(TemplatePlaceholder),
            BG_ROLE(TemplateFocusedPlaceholder),
            BG_ROLE(TemplateReadOnlyPlaceholder),
        };
#undef BG_ROLE

        const auto role = parser.value(bgRole);
        bool ok = false;
        for (const auto &def : bgRoles) {
            if (def.name == role) {
                bgColorRole = def.role;
                ok = true;
                break;
            }
        }

        if (!ok) {
            fprintf(stderr, "Unknown background role. Expected:\n");
            for (const auto &def : bgRoles) {
                fprintf(stderr, "  - %s\n", def.asciiName);
            }
            return 1;
        }
    }

    if (parser.isSet(updateDefs)) {
        DefinitionDownloader downloader(&repo);
        QObject::connect(&downloader, &DefinitionDownloader::informationMessage, &app, [](const QString &msg) {
            fprintf(stdout, "%s\n", qPrintable(msg));
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
        fprintf(stderr, "Unknown syntax.\n");
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
        highlighter.setBackgroundRole(bgColorRole);
        highlighter.setTheme(theme(repo, parser.value(themeName), Repository::LightTheme));
        applyHighlighter(highlighter, parser, fromFileName, inFileName, outputName, title);
    } else {
        auto AnsiFormat = AnsiHighlighter::AnsiFormat::TrueColor;
        // compatible with the old ansi256Colors value
        if (outputFormat.startsWith(QLatin1String("ansi256"), Qt::CaseInsensitive)) {
            AnsiFormat = AnsiHighlighter::AnsiFormat::XTerm256Color;
        } else if (0 != outputFormat.compare(QLatin1String("ansi"), Qt::CaseInsensitive)) {
            fprintf(stderr, "Unknown output format.\n");
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
                    fprintf(stderr, "Unknown trace name.\n");
                    return 2;
                }
            }
        }

        AnsiHighlighter highlighter;
        highlighter.setDefinition(def);
        highlighter.setBackgroundRole(bgColorRole);
        highlighter.setTheme(theme(repo, parser.value(themeName), Repository::DarkTheme));
        applyHighlighter(highlighter, parser, fromFileName, inFileName, outputName, AnsiFormat, options);
    }

    return 0;
}
