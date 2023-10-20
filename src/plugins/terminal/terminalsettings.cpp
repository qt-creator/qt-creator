// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalsettings.h"

#include "terminaltr.h"

#include <coreplugin/icore.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/dropsupport.h>
#include <utils/environment.h>
#include <utils/expected.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QFontComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QXmlStreamReader>

Q_LOGGING_CATEGORY(schemeLog, "qtc.terminal.scheme", QtWarningMsg)

using namespace Utils;

namespace Terminal {

static QString defaultFontFamily()
{
    if (HostOsInfo::isMacHost())
        return QLatin1String("Menlo");

    if (Utils::HostOsInfo::isAnyUnixHost())
        return QLatin1String("Monospace");

    return QLatin1String("Consolas");
}

static int defaultFontSize()
{
    if (Utils::HostOsInfo::isMacHost())
        return 12;
    if (Utils::HostOsInfo::isAnyUnixHost())
        return 9;
    return 10;
}

static QString defaultShell()
{
    if (HostOsInfo::isWindowsHost())
        return qtcEnvironmentVariable("COMSPEC");

    FilePath defaultShell = FilePath::fromUserInput(qtcEnvironmentVariable("SHELL"));
    if (defaultShell.isExecutableFile())
        return defaultShell.toUserOutput();

    return Environment::systemEnvironment().searchInPath("sh").toUserOutput();
}

void setupColor(TerminalSettings *settings,
                ColorAspect &color,
                const QString &label,
                const QColor &defaultColor,
                const QString &humanReadableName = {})
{
    color.setSettingsKey(keyFromString(label));
    color.setDefaultValue(defaultColor);
    color.setToolTip(Tr::tr("The color used for %1.")
                         .arg(humanReadableName.isEmpty() ? label : humanReadableName));
    settings->registerAspect(&color);
}

static expected_str<void> loadXdefaults(const FilePath &path)
{
    const expected_str<QByteArray> readResult = path.fileContents();
    if (!readResult)
        return make_unexpected(readResult.error());

    QRegularExpression re(R"(.*\*(color[0-9]{1,2}|foreground|background):\s*(#[0-9a-f]{6}))");

    for (const QByteArray &line : readResult->split('\n')) {
        if (line.trimmed().startsWith('!'))
            continue;

        const auto match = re.match(QString::fromUtf8(line));
        if (match.hasMatch()) {
            const QString colorName = match.captured(1);
            const QColor color(match.captured(2));
            if (colorName == "foreground") {
                settings().foregroundColor.setVolatileValue(color);
            } else if (colorName == "background") {
                settings().backgroundColor.setVolatileValue(color);
            } else {
                const int colorIndex = colorName.mid(5).toInt();
                if (colorIndex >= 0 && colorIndex < 16)
                    settings().colors[colorIndex].setVolatileValue(color);
            }
        }
    }

    return {};
}

static expected_str<void> loadItermColors(const FilePath &path)
{
    QFile f(path.toFSPathString());
    const bool opened = f.open(QIODevice::ReadOnly);
    if (!opened)
        return make_unexpected(Tr::tr("Failed to open file."));

    QXmlStreamReader reader(&f);
    while (!reader.atEnd() && reader.readNextStartElement()) {
        if (reader.name() == u"plist") {
            while (!reader.atEnd() && reader.readNextStartElement()) {
                if (reader.name() == u"dict") {
                    QString colorName;
                    while (!reader.atEnd() && reader.readNextStartElement()) {
                        if (reader.name() == u"key") {
                            colorName = reader.readElementText();
                        } else if (reader.name() == u"dict") {
                            QColor color;
                            int component = 0;
                            while (!reader.atEnd() && reader.readNextStartElement()) {
                                if (reader.name() == u"key") {
                                    const auto &text = reader.readElementText();
                                    if (text == u"Red Component")
                                        component = 0;
                                    else if (text == u"Green Component")
                                        component = 1;
                                    else if (text == u"Blue Component")
                                        component = 2;
                                    else if (text == u"Alpha Component")
                                        component = 3;
                                } else if (reader.name() == u"real") {
                                    // clang-format off
                                    switch (component) {
                                        case 0: color.setRedF(reader.readElementText().toDouble()); break;
                                        case 1: color.setGreenF(reader.readElementText().toDouble()); break;
                                        case 2: color.setBlueF(reader.readElementText().toDouble()); break;
                                        case 3: color.setAlphaF(reader.readElementText().toDouble()); break;
                                    }
                                    // clang-format on
                                } else {
                                    reader.skipCurrentElement();
                                }
                            }

                            if (colorName.startsWith("Ansi")) {
                                const auto c = colorName.mid(5, 2);
                                const int colorIndex = c.toInt();
                                if (colorIndex >= 0 && colorIndex < 16)
                                    settings().colors[colorIndex].setVolatileValue(
                                        color);
                            } else if (colorName == "Foreground Color") {
                                settings().foregroundColor.setVolatileValue(color);
                            } else if (colorName == "Background Color") {
                                settings().backgroundColor.setVolatileValue(color);
                            } else if (colorName == "Selection Color") {
                                settings().selectionColor.setVolatileValue(color);
                            }
                        }
                    }
                }
            }
            break;
        }
    }
    if (reader.hasError())
        return make_unexpected(reader.errorString());

    return {};
}

static expected_str<void> loadWindowsTerminalColors(const FilePath &path)
{
    const expected_str<QByteArray> readResult = path.fileContents();
    if (!readResult)
        return make_unexpected(readResult.error());

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(*readResult, &error);
    if (error.error != QJsonParseError::NoError)
        return make_unexpected(Tr::tr("JSON parsing error: \"%1\", at offset: %2")
                                   .arg(error.errorString())
                                   .arg(error.offset));

    const QJsonObject colors = doc.object();

    // clang-format off
    const QList<QPair<QStringView, ColorAspect *>> colorKeys = {
        qMakePair(u"background", &settings().backgroundColor),
        qMakePair(u"foreground", &settings().foregroundColor),
        qMakePair(u"selectionBackground", &settings().selectionColor),

        qMakePair(u"black", &settings().colors[0]),
        qMakePair(u"brightBlack", &settings().colors[8]),

        qMakePair(u"red", &settings().colors[1]),
        qMakePair(u"brightRed", &settings().colors[9]),

        qMakePair(u"green", &settings().colors[2]),
        qMakePair(u"brightGreen", &settings().colors[10]),

        qMakePair(u"yellow", &settings().colors[3]),
        qMakePair(u"brightYellow", &settings().colors[11]),

        qMakePair(u"blue", &settings().colors[4]),
        qMakePair(u"brightBlue", &settings().colors[12]),

        qMakePair(u"magenta", &settings().colors[5]),
        qMakePair(u"brightMagenta", &settings().colors[13]),

        qMakePair(u"cyan", &settings().colors[6]),
        qMakePair(u"brightCyan", &settings().colors[14]),

        qMakePair(u"white", &settings().colors[7]),
        qMakePair(u"brightWhite", &settings().colors[15])
    };
    // clang-format on

    for (const auto &pair : colorKeys) {
        const auto it = colors.find(pair.first);
        if (it != colors.end()) {
            const QString colorString = it->toString();
            if (colorString.startsWith("#")) {
                QColor color(colorString.mid(0, 7));
                if (colorString.size() > 7) {
                    int alpha = colorString.mid(7).toInt(nullptr, 16);
                    color.setAlpha(alpha);
                }
                if (color.isValid())
                    pair.second->setVolatileValue(color);
            }
        }
    }

    return {};
}

static expected_str<void> loadVsCodeColors(const FilePath &path)
{
    const expected_str<QByteArray> readResult = path.fileContents();
    if (!readResult)
        return make_unexpected(readResult.error());

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(*readResult, &error);
    if (error.error != QJsonParseError::NoError)
        return make_unexpected(Tr::tr("JSON parsing error: \"%1\", at offset: %2")
                                   .arg(error.errorString())
                                   .arg(error.offset));

    const QJsonObject root = doc.object();
    const auto itColors = root.find("colors");
    if (itColors == root.end())
        return make_unexpected(Tr::tr("No colors found."));

    const QJsonObject colors = itColors->toObject();

    // clang-format off
    const QList<QPair<QStringView, ColorAspect *>> colorKeys = {
        qMakePair(u"editor.background", &settings().backgroundColor),
        qMakePair(u"terminal.foreground", &settings().foregroundColor),
        qMakePair(u"terminal.selectionBackground", &settings().selectionColor),

        qMakePair(u"terminal.ansiBlack", &settings().colors[0]),
        qMakePair(u"terminal.ansiBrightBlack", &settings().colors[8]),

        qMakePair(u"terminal.ansiRed", &settings().colors[1]),
        qMakePair(u"terminal.ansiBrightRed", &settings().colors[9]),

        qMakePair(u"terminal.ansiGreen", &settings().colors[2]),
        qMakePair(u"terminal.ansiBrightGreen", &settings().colors[10]),

        qMakePair(u"terminal.ansiYellow", &settings().colors[3]),
        qMakePair(u"terminal.ansiBrightYellow", &settings().colors[11]),

        qMakePair(u"terminal.ansiBlue", &settings().colors[4]),
        qMakePair(u"terminal.ansiBrightBlue", &settings().colors[12]),

        qMakePair(u"terminal.ansiMagenta", &settings().colors[5]),
        qMakePair(u"terminal.ansiBrightMagenta", &settings().colors[13]),

        qMakePair(u"terminal.ansiCyan", &settings().colors[6]),
        qMakePair(u"terminal.ansiBrightCyan", &settings().colors[14]),

        qMakePair(u"terminal.ansiWhite", &settings().colors[7]),
        qMakePair(u"terminal.ansiBrightWhite", &settings().colors[15])
    };
    // clang-format on

    for (const auto &pair : colorKeys) {
        const auto it = colors.find(pair.first);
        if (it != colors.end()) {
            const QString colorString = it->toString();
            if (colorString.startsWith("#")) {
                QColor color(colorString.mid(0, 7));
                if (colorString.size() > 7) {
                    int alpha = colorString.mid(7).toInt(nullptr, 16);
                    color.setAlpha(alpha);
                }
                if (color.isValid())
                    pair.second->setVolatileValue(color);
            }
        }
    }

    return {};
}

static expected_str<void> loadKonsoleColorScheme(const FilePath &path)
{
    auto parseColor = [](const QStringList &parts) -> expected_str<QColor> {
        if (parts.size() != 3 && parts.size() != 4)
            return make_unexpected(Tr::tr("Invalid color format."));
        int alpha = parts.size() == 4 ? parts[3].toInt() : 255;
        return QColor(parts[0].toInt(), parts[1].toInt(), parts[2].toInt(), alpha);
    };

    // clang-format off
    TerminalSettings &s = settings();
    const QPair<QString, ColorAspect *> colorKeys[] = {
        { "Background/Color", &s.backgroundColor },
        { "Foreground/Color", &s.foregroundColor},

        { "Color0/Color", &s.colors[0] },
        { "Color0Intense/Color", &s.colors[8] },

        { "Color1/Color", &s.colors[1] },
        { "Color1Intense/Color", &s.colors[9] },

        { "Color2/Color", &s.colors[2] },
        { "Color2Intense/Color", &s.colors[10] },

        { "Color3/Color", &s.colors[3] },
        { "Color3Intense/Color", &s.colors[11] },

        { "Color4/Color", &s.colors[4] },
        { "Color4Intense/Color", &s.colors[12] },

        { "Color5/Color", &s.colors[5] },
        { "Color5Intense/Color", &s.colors[13] },

        { "Color6/Color", &s.colors[6] },
        { "Color6Intense/Color", &s.colors[14] },

        { "Color7/Color", &s.colors[7] },
        { "Color7Intense/Color", &s.colors[15] }
    };
    // clang-format on

    QSettings ini(path.toFSPathString(), QSettings::IniFormat);
    for (const auto &colorKey : colorKeys) {
        if (ini.contains(colorKey.first)) {
            const auto color = parseColor(ini.value(colorKey.first).toStringList());
            if (!color)
                return make_unexpected(color.error());

            colorKey.second->setVolatileValue(*color);
        }
    }

    return {};
}

static expected_str<void> loadXFCE4ColorScheme(const FilePath &path)
{
    expected_str<QByteArray> arr = path.fileContents();
    if (!arr)
        return make_unexpected(arr.error());

    arr->replace(';', ',');

    QTemporaryFile f;
    f.open();
    f.write(*arr);
    f.close();

    QSettings ini(f.fileName(), QSettings::IniFormat);
    TerminalSettings &s = settings();

    // clang-format off
    const QPair<QString, ColorAspect *> colorKeys[] = {
        { "Scheme/ColorBackground", &s.backgroundColor },
        { "Scheme/ColorForeground", &s.foregroundColor }
    };
    // clang-format on

    for (const auto &colorKey : colorKeys) {
        if (ini.contains(colorKey.first))
            colorKey.second->setVolatileValue(QColor(ini.value(colorKey.first).toString()));
    }

    QStringList colors = ini.value(QLatin1String("Scheme/ColorPalette")).toStringList();
    int i = 0;
    for (const QString &color : colors)
        s.colors[i++].setVolatileValue(QColor(color));

    return {};
}

static expected_str<void> loadVsCodeOrWindows(const FilePath &path)
{
    return loadVsCodeColors(path).or_else(
        [path](const auto &) { return loadWindowsTerminalColors(path); });
}

static expected_str<void> loadColorScheme(const FilePath &path)
{
    if (path.endsWith("Xdefaults"))
        return loadXdefaults(path);
    else if (path.suffix() == "itermcolors")
        return loadItermColors(path);
    else if (path.suffix() == "json")
        return loadVsCodeOrWindows(path);
    else if (path.suffix() == "colorscheme")
        return loadKonsoleColorScheme(path);
    else if (path.suffix() == "theme" || path.completeSuffix() == "theme.txt")
        return loadXFCE4ColorScheme(path);

    return make_unexpected(Tr::tr("Unknown color scheme format."));
}

TerminalSettings &settings()
{
    static TerminalSettings theSettings;
    return theSettings;
}

TerminalSettings::TerminalSettings()
{
    setSettingsGroup("Terminal");
    setAutoApply(false);

    enableTerminal.setSettingsKey("EnableTerminal");
    enableTerminal.setLabelText(Tr::tr("Use internal terminal"));
    enableTerminal.setToolTip(
        Tr::tr("Uses the internal terminal when \"Run In Terminal\" is "
               "enabled and for \"Open Terminal here\"."));
    enableTerminal.setDefaultValue(true);

    font.setSettingsKey("FontFamily");
    font.setLabelText(Tr::tr("Family:"));
    font.setHistoryCompleter("Terminal.Fonts.History");
    font.setToolTip(Tr::tr("The font family used in the terminal."));
    font.setDefaultValue(defaultFontFamily());

    fontSize.setSettingsKey("FontSize");
    fontSize.setLabelText(Tr::tr("Size:"));
    fontSize.setToolTip(Tr::tr("The font size used in the terminal (in points)."));
    fontSize.setDefaultValue(defaultFontSize());
    fontSize.setRange(1, 100);

    allowBlinkingCursor.setSettingsKey("AllowBlinkingCursor");
    allowBlinkingCursor.setLabelText(Tr::tr("Allow blinking cursor"));
    allowBlinkingCursor.setToolTip(Tr::tr("Allow the cursor to blink."));
    allowBlinkingCursor.setDefaultValue(false);

    shell.setSettingsKey("ShellPath");
    shell.setLabelText(Tr::tr("Shell path:"));
    shell.setExpectedKind(PathChooser::ExistingCommand);
    shell.setHistoryCompleter("Terminal.Shell.History");
    shell.setToolTip(Tr::tr("The shell executable to be started."));
    shell.setDefaultValue(defaultShell());

    shellArguments.setSettingsKey("ShellArguments");
    shellArguments.setLabelText(Tr::tr("Shell arguments:"));
    shellArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    shellArguments.setHistoryCompleter("Terminal.Shell.History");
    shellArguments.setToolTip(Tr::tr("The arguments to be passed to the shell."));
    if (!HostOsInfo::isWindowsHost())
        shellArguments.setDefaultValue(QString("-l"));

    sendEscapeToTerminal.setSettingsKey("SendEscapeToTerminal");
    sendEscapeToTerminal.setLabelText(Tr::tr("Send escape key to terminal"));
    sendEscapeToTerminal.setToolTip(Tr::tr("Sends the escape key to the terminal when pressed "
                                           "instead of closing the terminal."));
    sendEscapeToTerminal.setDefaultValue(false);

    lockKeyboard.setSettingsKey("LockKeyboard");
    lockKeyboard.setLabelText(Tr::tr("Block shortcuts in terminal"));
    lockKeyboard.setToolTip(
        Tr::tr("Keeps Qt Creator shortcuts from interfering with the terminal."));
    lockKeyboard.setDefaultValue(true);

    audibleBell.setSettingsKey("AudibleBell");
    audibleBell.setLabelText(Tr::tr("Audible bell"));
    audibleBell.setToolTip(Tr::tr("Makes the terminal beep when a bell "
                                  "character is received."));
    audibleBell.setDefaultValue(true);

    enableMouseTracking.setSettingsKey("EnableMouseTracking");
    enableMouseTracking.setLabelText(Tr::tr("Enable mouse tracking"));
    enableMouseTracking.setToolTip(Tr::tr("Enables mouse tracking in the terminal."));
    enableMouseTracking.setDefaultValue(true);

    Theme *theme = Utils::creatorTheme();

    setupColor(this, foregroundColor, "Foreground", theme->color(Theme::TerminalForeground));
    setupColor(this, backgroundColor, "Background", theme->color(Theme::TerminalBackground));
    setupColor(this, selectionColor, "Selection", theme->color(Theme::TerminalSelection));

    setupColor(this, findMatchColor, "Find matches", theme->color(Theme::TerminalFindMatch));

    setupColor(this, colors[0], "0", theme->color(Theme::TerminalAnsi0), "black");
    setupColor(this, colors[8], "8", theme->color(Theme::TerminalAnsi8), "bright black");

    setupColor(this, colors[1], "1", theme->color(Theme::TerminalAnsi1), "red");
    setupColor(this, colors[9], "9", theme->color(Theme::TerminalAnsi9), "bright red");

    setupColor(this, colors[2], "2", theme->color(Theme::TerminalAnsi2), "green");
    setupColor(this, colors[10], "10", theme->color(Theme::TerminalAnsi10), "bright green");

    setupColor(this, colors[3], "3", theme->color(Theme::TerminalAnsi3), "yellow");
    setupColor(this, colors[11], "11", theme->color(Theme::TerminalAnsi11), "bright yellow");

    setupColor(this, colors[4], "4", theme->color(Theme::TerminalAnsi4), "blue");
    setupColor(this, colors[12], "12", theme->color(Theme::TerminalAnsi12), "bright blue");

    setupColor(this, colors[5], "5", theme->color(Theme::TerminalAnsi5), "magenta");
    setupColor(this, colors[13], "13", theme->color(Theme::TerminalAnsi13), "bright magenta");

    setupColor(this, colors[6], "6", theme->color(Theme::TerminalAnsi6), "cyan");
    setupColor(this, colors[14], "14", theme->color(Theme::TerminalAnsi14), "bright cyan");

    setupColor(this, colors[7], "7", theme->color(Theme::TerminalAnsi7), "white");
    setupColor(this, colors[15], "15", theme->color(Theme::TerminalAnsi15), "bright white");

    setLayouter([this] {
        using namespace Layouting;

        QFontComboBox *fontComboBox = new QFontComboBox;
        fontComboBox->setFontFilters(QFontComboBox::MonospacedFonts);
        fontComboBox->setCurrentFont(font());

        connect(fontComboBox, &QFontComboBox::currentFontChanged, this, [this](const QFont &f) {
            font.setVolatileValue(f.family());
        });

        auto loadThemeButton = new QPushButton(Tr::tr("Load Theme..."));
        auto resetTheme = new QPushButton(Tr::tr("Reset Theme"));
        auto copyTheme = schemeLog().isDebugEnabled() ? new QPushButton(Tr::tr("Copy Theme"))
                                                      : nullptr;

        connect(loadThemeButton, &QPushButton::clicked, this, [] {
            const FilePath path = FileUtils::getOpenFilePath(
                Core::ICore::dialogParent(),
                "Open Theme",
                {},
                "All Scheme formats (*.itermcolors *.json *.colorscheme *.theme *.theme.txt);;"
                "Xdefaults (.Xdefaults Xdefaults);;"
                "iTerm Color Schemes(*.itermcolors);;"
                "VS Code Color Schemes(*.json);;"
                "Windows Terminal Schemes(*.json);;"
                "Konsole Color Schemes(*.colorscheme);;"
                "XFCE4 Terminal Color Schemes(*.theme *.theme.txt);;"
                "All files (*)",
                nullptr,
                {},
                true,
                false);

            if (path.isEmpty())
                return;

            const expected_str<void> result = loadColorScheme(path);
            if (!result)
                QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("Error"), result.error());
        });

        connect(resetTheme, &QPushButton::clicked, this, [this] {
            foregroundColor.setVolatileValue(foregroundColor.defaultValue());
            backgroundColor.setVolatileValue(backgroundColor.defaultValue());
            selectionColor.setVolatileValue(selectionColor.defaultValue());

            for (ColorAspect &color : colors)
                color.setVolatileValue(color.defaultValue());
        });

        if (schemeLog().isDebugEnabled()) {
            connect(copyTheme, &QPushButton::clicked, this, [this] {
                auto toThemeColor = [](const ColorAspect &color) -> QString {
                    QColor c = color.value();
                    QString a = c.alpha() != 255 ? QString("%1").arg(c.alpha(), 2, 16, QChar('0'))
                                                 : QString();
                    return QString("%1%2%3%4")
                        .arg(a)
                        .arg(c.red(), 2, 16, QChar('0'))
                        .arg(c.green(), 2, 16, QChar('0'))
                        .arg(c.blue(), 2, 16, QChar('0'));
                };

                QString theme;
                QTextStream stream(&theme);
                stream << "TerminalForeground=" << toThemeColor(foregroundColor) << '\n';
                stream << "TerminalBackground=" << toThemeColor(backgroundColor) << '\n';
                stream << "TerminalSelection=" << toThemeColor(selectionColor) << '\n';
                stream << "TerminalFindMatch=" << toThemeColor(findMatchColor) << '\n';
                for (int i = 0; i < 16; ++i)
                    stream << "TerminalAnsi" << i << '=' << toThemeColor(colors[i]) << '\n';

                setClipboardAndSelection(theme);
            });
        }

        // clang-format off
        return Column {
            Group {
                title(Tr::tr("General")),
                Column {
                    enableTerminal, st,
                    sendEscapeToTerminal, st,
                    lockKeyboard, st,
                    audibleBell, st,
                    allowBlinkingCursor, st,
                    enableMouseTracking, st,
                },
            },
            Group {
                title(Tr::tr("Font")),
                Row {
                    font.labelText(), fontComboBox, Space(20),
                    fontSize, st,
                },
            },
            Group {
                title(Tr::tr("Colors")),
                Column {
                    Row {
                        Tr::tr("Foreground"), foregroundColor, st,
                        Tr::tr("Background"), backgroundColor, st,
                        Tr::tr("Selection"), selectionColor, st,
                        Tr::tr("Find match"), findMatchColor, st,
                    },
                    Row {
                        colors[0], colors[1],
                        colors[2], colors[3],
                        colors[4], colors[5],
                        colors[6], colors[7]
                    },
                    Row {
                        colors[8], colors[9],
                        colors[10], colors[11],
                        colors[12], colors[13],
                        colors[14], colors[15]
                    },
                    Row {
                        loadThemeButton, resetTheme, copyTheme, st,
                    }
                },
            },
            Group {
                title(Tr::tr("Default Shell")),
                Column {
                    shell,
                    shellArguments,
                },
            },
            st,
        };
        // clang-format on
    });

    readSettings();
}

class TerminalSettingsPage final : public Core::IOptionsPage
{
public:
    TerminalSettingsPage()
    {
        setId("Terminal.General");
        setDisplayName("Terminal");
        setCategory("ZY.Terminal");
        setDisplayCategory("Terminal");
        setCategoryIconPath(":/terminal/images/settingscategory_terminal.png");
        setSettingsProvider([] { return &settings(); });
    }
};

const TerminalSettingsPage settingsPage;

} // Terminal
