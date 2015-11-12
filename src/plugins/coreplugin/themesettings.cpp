/****************************************************************************
**
** Copyright (C) 2015 Thorben Kroeger <thorbenkroeger@gmail.com>.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "themesettings.h"
#include "themesettingswidget.h"
#include "coreconstants.h"
#include "icore.h"

#include <utils/algorithm.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QSettings>

static const char themeNameKey[] = "ThemeName";

namespace Core {
namespace Internal {

ThemeEntry::ThemeEntry(Id id, const QString &filePath, bool readOnly)
    : m_id(id),
      m_filePath(filePath),
      m_readOnly(readOnly)
{
}

Id ThemeEntry::id() const
{
    return m_id;
}

QString ThemeEntry::displayName() const
{
    if (m_displayName.isEmpty() && !m_filePath.isEmpty()) {
        QSettings settings(m_filePath, QSettings::IniFormat);
        m_displayName = settings.value(QLatin1String(themeNameKey),
                                       QCoreApplication::tr("unnamed")).toString();
        if (false) // TODO: Revert to m_readOnly
            m_displayName = QCoreApplication::tr("%1 (built-in)").arg(m_displayName);
    }
    return m_displayName;
}

QString ThemeEntry::filePath() const
{
    return m_filePath;
}

bool ThemeEntry::readOnly() const
{
    return m_readOnly;
}

ThemeSettings::ThemeSettings()
{
    setId(Constants::SETTINGS_ID_INTERFACE);
    setDisplayName(tr("Theme"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setDisplayCategory(QCoreApplication::translate("Core", Constants::SETTINGS_TR_CATEGORY_CORE));
    setCategoryIcon(QLatin1String(Constants::SETTINGS_CATEGORY_CORE_ICON));
}

ThemeSettings::~ThemeSettings()
{
    delete m_widget;
}

QWidget *ThemeSettings::widget()
{
    if (!m_widget)
        m_widget = new ThemeSettingsWidget;
    return m_widget;
}

void ThemeSettings::apply()
{
    if (m_widget)
        m_widget->apply();
}

void ThemeSettings::finish()
{
    delete m_widget;
    m_widget = 0;
}

static void addThemesFromPath(const QString &path, bool readOnly, QList<ThemeEntry> *themes)
{
    static const QLatin1String extension(".creatortheme");
    QDir themeDir(path);
    themeDir.setNameFilters(QStringList() << QLatin1String("*.creatortheme"));
    themeDir.setFilter(QDir::Files);
    const QStringList themeList = themeDir.entryList();
    foreach (const QString &fileName, themeList) {
        QString id = QFileInfo(fileName).completeBaseName();
        themes->append(ThemeEntry(Id::fromString(id), themeDir.absoluteFilePath(fileName), readOnly));
    }
}

QList<ThemeEntry> ThemeSettings::availableThemes()
{
    QList<ThemeEntry> themes;

    static const QString installThemeDir = ICore::resourcePath() + QLatin1String("/themes");
    static const QString userThemeDir = ICore::userResourcePath() + QLatin1String("/themes");
    addThemesFromPath(installThemeDir, /*readOnly=*/true, &themes);
    if (themes.isEmpty())
        qWarning() << "Warning: No themes found in installation: "
                   << QDir::toNativeSeparators(installThemeDir);
    // move default theme to front
    int defaultIndex = Utils::indexOf(themes, Utils::equal(&ThemeEntry::id, Id("default")));
    if (defaultIndex > 0) { // == exists and not at front
        ThemeEntry defaultEntry = themes.takeAt(defaultIndex);
        themes.prepend(defaultEntry);
    }
    addThemesFromPath(userThemeDir, /*readOnly=*/false, &themes);
    return themes;
}

} // namespace Internal
} // namespace Core
