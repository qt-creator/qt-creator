// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "themeselector.h"

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <coreplugin/manhattanstyle.h>

#include <QApplication>
#include <QDir>
#include <QSettings>

namespace ManualTest {

static const char themeNameKey[] = "ThemeName";

void ThemeSelector::setTheme(const QString &themeFile)
{
    using namespace Utils;

    static Theme theme("");
    QSettings settings(themeFile, QSettings::IniFormat);
    theme.readSettings(settings);
    setCreatorTheme(&theme);
    StyleHelper::setBaseColor(QColor(StyleHelper::DEFAULT_BASE_COLOR));
    QApplication::setStyle(new ManhattanStyle(creatorTheme()->preferredStyles().value(0)));
    QApplication::setPalette(theme.palette());
}

ThemeSelector::ThemeSelector(QWidget *parent)
    : QComboBox(parent)
{
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationName("tst_manual");

    for (const QFileInfo &themeFile : QDir(":/themes/", "*.creatortheme").entryInfoList()) {
        QSettings settings(themeFile.absoluteFilePath(), QSettings::IniFormat);
        addItem(settings.value("ThemeName").toString(), themeFile.absoluteFilePath());
    }

    QSettings appSettings;
    setCurrentText(appSettings.value(themeNameKey, "Flat Dark").toString());
    setTheme(currentData().toString());

    connect(this, &QComboBox::currentTextChanged, this, [this] {
        setTheme(currentData().toString());
        QSettings appSettings;
        appSettings.setValue(themeNameKey, currentText());
    });
}

} // namespace ManualTest
