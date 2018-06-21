/****************************************************************************
**
** Copyright (C) 2016 Dmitry Savchenko
** Copyright (C) 2016 Vasiliy Sorokin
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

#include "settings.h"
#include "constants.h"

#include <coreplugin/coreconstants.h>
#include <utils/theme/theme.h>

#include <QSettings>

namespace Todo {
namespace Internal {

void Settings::save(QSettings *settings) const
{
    if (!keywordsEdited)
        return;

    settings->beginGroup(Constants::SETTINGS_GROUP);
    settings->setValue(Constants::SCANNING_SCOPE, scanningScope);

    settings->beginWriteArray(Constants::KEYWORDS_LIST);
    if (const int size = keywords.size()) {
        const QString nameKey = "name";
        const QString colorKey = "color";
        const QString iconTypeKey = "iconType";
        for (int i = 0; i < size; ++i) {
            settings->setArrayIndex(i);
            settings->setValue(nameKey, keywords.at(i).name);
            settings->setValue(colorKey, keywords.at(i).color);
            settings->setValue(iconTypeKey, static_cast<int>(keywords.at(i).iconType));
        }
    }
    settings->endArray();

    settings->endGroup();
    settings->sync();
}

void Settings::load(QSettings *settings)
{
    setDefault();

    settings->beginGroup(Constants::SETTINGS_GROUP);

    scanningScope = static_cast<ScanningScope>(settings->value(Constants::SCANNING_SCOPE,
        ScanningScopeCurrentFile).toInt());
    if (scanningScope >= ScanningScopeMax)
        scanningScope = ScanningScopeCurrentFile;

    KeywordList newKeywords;
    const int keywordsSize = settings->beginReadArray(Constants::KEYWORDS_LIST);
    if (keywordsSize > 0) {
        const QString nameKey = "name";
        const QString colorKey = "color";
        const QString iconTypeKey = "iconType";
        for (int i = 0; i < keywordsSize; ++i) {
            settings->setArrayIndex(i);
            Keyword keyword;
            keyword.name = settings->value(nameKey).toString();
            keyword.color = settings->value(colorKey).value<QColor>();
            keyword.iconType = static_cast<IconType>(settings->value(iconTypeKey).toInt());
            newKeywords << keyword;
        }
        keywords = newKeywords;
        keywordsEdited = true; // Otherwise they wouldn't have been saved
    }
    settings->endArray();

    settings->endGroup();
}

void Settings::setDefault()
{
    scanningScope = ScanningScopeCurrentFile;
    Utils::Theme *theme = Utils::creatorTheme();

    keywords.clear();

    Keyword keyword;

    keyword.name = "TODO";
    keyword.iconType = IconType::Todo;
    keyword.color = theme->color(Utils::Theme::OutputPanes_NormalMessageTextColor);
    keywords.append(keyword);

    keyword.name = "NOTE";
    keyword.iconType = IconType::Info;
    keyword.color = theme->color(Utils::Theme::OutputPanes_NormalMessageTextColor);
    keywords.append(keyword);

    keyword.name = "FIXME";
    keyword.iconType = IconType::Error;
    keyword.color = theme->color(Utils::Theme::OutputPanes_ErrorMessageTextColor);
    keywords.append(keyword);

    keyword.name = "BUG";
    keyword.iconType = IconType::Bug;
    keyword.color = theme->color(Utils::Theme::OutputPanes_ErrorMessageTextColor);
    keywords.append(keyword);

    keyword.name = "WARNING";
    keyword.iconType = IconType::Warning;
    keyword.color = theme->color(Utils::Theme::OutputPanes_WarningMessageTextColor);
    keywords.append(keyword);

    keywordsEdited = false;
}

bool Settings::equals(const Settings &other) const
{
    return (keywords == other.keywords)
            && (scanningScope == other.scanningScope)
            && (keywordsEdited == other.keywordsEdited);
}

bool operator ==(Settings &s1, Settings &s2)
{
    return s1.equals(s2);
}

bool operator !=(Settings &s1, Settings &s2)
{
    return !s1.equals(s2);
}

} // namespace Internal
} // namespace Todo

