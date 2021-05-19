/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

    connect(this, &QComboBox::currentTextChanged, [this]{
        setTheme(currentData().toString());
        QSettings appSettings;
        appSettings.setValue(themeNameKey, currentText());
    });
}

} // namespace ManualTest
