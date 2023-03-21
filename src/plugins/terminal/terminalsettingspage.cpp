// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalsettingspage.h"

#include "terminalsettings.h"
#include "terminaltr.h"

#include <coreplugin/icore.h>

#include <utils/aspects.h>
#include <utils/dropsupport.h>
#include <utils/expected.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QFontComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QXmlStreamReader>

using namespace Utils;

namespace Terminal {

TerminalSettingsPage::TerminalSettingsPage()
{
    setId("Terminal.General");
    setDisplayName("Terminal");
    setCategory("ZY.Terminal");
    setDisplayCategory("Terminal");
    setSettings(&TerminalSettings::instance());
    setCategoryIconPath(":/terminal/images/settingscategory_terminal.png");
}

void TerminalSettingsPage::init() {}

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
                TerminalSettings::instance().foregroundColor.setVolatileValue(color);
            } else if (colorName == "background") {
                TerminalSettings::instance().backgroundColor.setVolatileValue(color);
            } else {
                const int colorIndex = colorName.mid(5).toInt();
                if (colorIndex >= 0 && colorIndex < 16)
                    TerminalSettings::instance().colors[colorIndex].setVolatileValue(color);
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
        return make_unexpected(Tr::tr("Failed to open file"));

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
                            int component;
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
                                    TerminalSettings::instance().colors[colorIndex].setVolatileValue(
                                        color);
                            } else if (colorName == "Foreground Color") {
                                TerminalSettings::instance().foregroundColor.setVolatileValue(color);
                            } else if (colorName == "Background Color") {
                                TerminalSettings::instance().backgroundColor.setVolatileValue(color);
                            } else if (colorName == "Selection Color") {
                                TerminalSettings::instance().selectionColor.setVolatileValue(color);
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
        return make_unexpected(Tr::tr("No colors found"));

    const QJsonObject colors = itColors->toObject();

    // clang-format off
    const QList<QPair<QStringView, ColorAspect *>> colorKeys = {
        qMakePair(u"editor.background", &TerminalSettings::instance().backgroundColor),
        qMakePair(u"terminal.foreground", &TerminalSettings::instance().foregroundColor),
        qMakePair(u"terminal.selectionBackground", &TerminalSettings::instance().selectionColor),

        qMakePair(u"terminal.ansiBlack", &TerminalSettings::instance().colors[0]),
        qMakePair(u"terminal.ansiBrightBlack", &TerminalSettings::instance().colors[8]),

        qMakePair(u"terminal.ansiRed", &TerminalSettings::instance().colors[1]),
        qMakePair(u"terminal.ansiBrightRed", &TerminalSettings::instance().colors[9]),

        qMakePair(u"terminal.ansiGreen", &TerminalSettings::instance().colors[2]),
        qMakePair(u"terminal.ansiBrightGreen", &TerminalSettings::instance().colors[10]),

        qMakePair(u"terminal.ansiYellow", &TerminalSettings::instance().colors[3]),
        qMakePair(u"terminal.ansiBrightYellow", &TerminalSettings::instance().colors[11]),

        qMakePair(u"terminal.ansiBlue", &TerminalSettings::instance().colors[4]),
        qMakePair(u"terminal.ansiBrightBlue", &TerminalSettings::instance().colors[12]),

        qMakePair(u"terminal.ansiMagenta", &TerminalSettings::instance().colors[5]),
        qMakePair(u"terminal.ansiBrightMagenta", &TerminalSettings::instance().colors[13]),

        qMakePair(u"terminal.ansiCyan", &TerminalSettings::instance().colors[6]),
        qMakePair(u"terminal.ansiBrightCyan", &TerminalSettings::instance().colors[14]),

        qMakePair(u"terminal.ansiWhite", &TerminalSettings::instance().colors[7]),
        qMakePair(u"terminal.ansiBrightWhite", &TerminalSettings::instance().colors[15])
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
    QSettings settings(path.toFSPathString(), QSettings::IniFormat);

    auto parseColor = [](const QStringList &parts) -> expected_str<QColor> {
        if (parts.size() != 3 && parts.size() != 4)
            return make_unexpected(Tr::tr("Invalid color format"));
        int alpha = parts.size() == 4 ? parts[3].toInt() : 255;
        return QColor(parts[0].toInt(), parts[1].toInt(), parts[2].toInt(), alpha);
    };

    // clang-format off
    const QList<QPair<QString, ColorAspect *>> colorKeys = {
        qMakePair(QLatin1String("Background/Color"), &TerminalSettings::instance().backgroundColor),
        qMakePair(QLatin1String("Foreground/Color"), &TerminalSettings::instance().foregroundColor),

        qMakePair(QLatin1String("Color0/Color"), &TerminalSettings::instance().colors[0]),
        qMakePair(QLatin1String("Color0Intense/Color"), &TerminalSettings::instance().colors[8]),

        qMakePair(QLatin1String("Color1/Color"), &TerminalSettings::instance().colors[1]),
        qMakePair(QLatin1String("Color1Intense/Color"), &TerminalSettings::instance().colors[9]),

        qMakePair(QLatin1String("Color2/Color"), &TerminalSettings::instance().colors[2]),
        qMakePair(QLatin1String("Color2Intense/Color"), &TerminalSettings::instance().colors[10]),

        qMakePair(QLatin1String("Color3/Color"), &TerminalSettings::instance().colors[3]),
        qMakePair(QLatin1String("Color3Intense/Color"), &TerminalSettings::instance().colors[11]),

        qMakePair(QLatin1String("Color4/Color"), &TerminalSettings::instance().colors[4]),
        qMakePair(QLatin1String("Color4Intense/Color"), &TerminalSettings::instance().colors[12]),

        qMakePair(QLatin1String("Color5/Color"), &TerminalSettings::instance().colors[5]),
        qMakePair(QLatin1String("Color5Intense/Color"), &TerminalSettings::instance().colors[13]),

        qMakePair(QLatin1String("Color6/Color"), &TerminalSettings::instance().colors[6]),
        qMakePair(QLatin1String("Color6Intense/Color"), &TerminalSettings::instance().colors[14]),

        qMakePair(QLatin1String("Color7/Color"), &TerminalSettings::instance().colors[7]),
        qMakePair(QLatin1String("Color7Intense/Color"), &TerminalSettings::instance().colors[15])
    };
    // clang-format on

    for (const auto &colorKey : colorKeys) {
        if (settings.contains(colorKey.first)) {
            const auto color = parseColor(settings.value(colorKey.first).toStringList());
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

    QSettings settings(f.fileName(), QSettings::IniFormat);

    // clang-format off
    const QList<QPair<QString, ColorAspect *>> colorKeys = {
        qMakePair(QLatin1String("Scheme/ColorBackground"), &TerminalSettings::instance().backgroundColor),
        qMakePair(QLatin1String("Scheme/ColorForeground"), &TerminalSettings::instance().foregroundColor),
    };
    // clang-format on

    for (const auto &colorKey : colorKeys) {
        if (settings.contains(colorKey.first)) {
            colorKey.second->setVolatileValue(QColor(settings.value(colorKey.first).toString()));
        }
    }

    QStringList colors = settings.value(QLatin1String("Scheme/ColorPalette")).toStringList();
    int i = 0;
    for (const auto &color : colors) {
        TerminalSettings::instance().colors[i++].setVolatileValue(QColor(color));
    }

    return {};
}

static expected_str<void> loadColorScheme(const FilePath &path)
{
    if (path.endsWith("Xdefaults"))
        return loadXdefaults(path);
    else if (path.suffix() == "itermcolors")
        return loadItermColors(path);
    else if (path.suffix() == "json")
        return loadVsCodeColors(path);
    else if (path.suffix() == "colorscheme")
        return loadKonsoleColorScheme(path);
    else if (path.suffix() == "theme" || path.completeSuffix() == "theme.txt")
        return loadXFCE4ColorScheme(path);

    return make_unexpected(Tr::tr("Unknown color scheme format"));
}

QWidget *TerminalSettingsPage::widget()
{
    QWidget *widget = new QWidget;

    using namespace Layouting;

    QFontComboBox *fontComboBox = new QFontComboBox(widget);
    fontComboBox->setFontFilters(QFontComboBox::MonospacedFonts);
    fontComboBox->setCurrentFont(TerminalSettings::instance().font.value());

    connect(fontComboBox, &QFontComboBox::currentFontChanged, this, [](const QFont &f) {
        TerminalSettings::instance().font.setValue(f.family());
    });

    TerminalSettings &settings = TerminalSettings::instance();

    QPushButton *loadThemeButton = new QPushButton(Tr::tr("Load Theme..."));

    connect(loadThemeButton, &QPushButton::clicked, this, [widget] {
        const FilePath path = FileUtils::getOpenFilePath(
            widget,
            "Open Theme",
            {},
            "All Scheme formats (*.itermcolors *.json *.colorscheme *.theme *.theme.txt);;"
            "Xdefaults (.Xdefaults Xdefaults);;"
            "iTerm Color Schemes(*.itermcolors);;"
            "VS Code Color Schemes(*.json);;"
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
            QMessageBox::warning(widget, Tr::tr("Error"), result.error());
    });

    // clang-format off
    Column {
        Row {
            settings.enableTerminal, st,
        },
        Group {
            title(Tr::tr("Font")),
            Row {
                settings.font.labelText(), fontComboBox, Space(20),
                settings.fontSize, st,
            },
        },
        Group {
            title(Tr::tr("Cursor")),
            Row {
                settings.allowBlinkingCursor, st,
            },
        },
        Group {
            title(Tr::tr("Colors")),
            Column {
                Row {
                    Tr::tr("Foreground"), settings.foregroundColor, st,
                    Tr::tr("Background"), settings.backgroundColor, st,
                    Tr::tr("Selection"), settings.selectionColor, st,
                },
                Row {
                    settings.colors[0], settings.colors[1],
                    settings.colors[2], settings.colors[3],
                    settings.colors[4], settings.colors[5],
                    settings.colors[6], settings.colors[7]
                },
                Row {
                    settings.colors[8], settings.colors[9],
                    settings.colors[10], settings.colors[11],
                    settings.colors[12], settings.colors[13],
                    settings.colors[14], settings.colors[15]
                },
                Row {
                    loadThemeButton, st,
                }
            },
        },
        Row {
            settings.shell,
        },
        st,
    }.attachTo(widget);
    // clang-format on

    DropSupport *dropSupport = new DropSupport(widget);
    connect(dropSupport,
            &DropSupport::filesDropped,
            this,
            [widget](const QList<DropSupport::FileSpec> &files) {
                if (files.size() != 1)
                    return;

                const expected_str<void> result = loadColorScheme(files.at(0).filePath);
                if (!result)
                    QMessageBox::warning(widget, Tr::tr("Error"), result.error());
            });

    return widget;
}

TerminalSettingsPage &TerminalSettingsPage::instance()
{
    static TerminalSettingsPage settingsPage;
    return settingsPage;
}

} // namespace Terminal
