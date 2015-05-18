/**************************************************************************
**
** Copyright (C) 2015 Dmitry Savchenko
** Copyright (C) 2015 Vasiliy Sorokin
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

#include "settings.h"
#include "constants.h"

#include <coreplugin/coreconstants.h>

#include <QSettings>

namespace Todo {
namespace Internal {

void Settings::save(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP));
    settings->setValue(QLatin1String(Constants::SCANNING_SCOPE), scanningScope);

    settings->beginWriteArray(QLatin1String(Constants::KEYWORDS_LIST));
    if (const int size = keywords.size()) {
        const QString nameKey = QLatin1String("name");
        const QString colorKey = QLatin1String("color");
        const QString iconResourceKey = QLatin1String("iconResource");
        for (int i = 0; i < size; ++i) {
            settings->setArrayIndex(i);
            settings->setValue(nameKey, keywords.at(i).name);
            settings->setValue(colorKey, keywords.at(i).color);
            settings->setValue(iconResourceKey, keywords.at(i).iconResource);
        }
    }
    settings->endArray();

    settings->endGroup();
    settings->sync();
}

void Settings::load(QSettings *settings)
{
    setDefault();

    settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP));

    scanningScope = static_cast<ScanningScope>(settings->value(QLatin1String(Constants::SCANNING_SCOPE),
        scanningScope).toInt());

    KeywordList newKeywords;
    const int keywordsSize = settings->beginReadArray(QLatin1String(Constants::KEYWORDS_LIST));
    if (keywordsSize > 0) {
        const QString nameKey = QLatin1String("name");
        const QString colorKey = QLatin1String("color");
        const QString iconResourceKey = QLatin1String("iconResource");
        for (int i = 0; i < keywordsSize; ++i) {
            settings->setArrayIndex(i);
            Keyword keyword;
            keyword.name = settings->value(nameKey).toString();
            keyword.color = settings->value(colorKey).value<QColor>();
            keyword.iconResource = settings->value(iconResourceKey).toString();
            newKeywords << keyword;
        }
        keywords = newKeywords;
    }
    settings->endArray();

    settings->endGroup();
}

void Settings::setDefault()
{
    scanningScope = ScanningScopeCurrentFile;

    keywords.clear();

    Keyword keyword;

    keyword.name = QLatin1String("TODO");
    keyword.iconResource = QLatin1String(Core::Constants::ICON_WARNING);
    keyword.color = QColor(QLatin1String(Constants::COLOR_TODO_BG));
    keywords.append(keyword);

    keyword.name = QLatin1String("NOTE");
    keyword.iconResource = QLatin1String(Core::Constants::ICON_INFO);
    keyword.color = QColor(QLatin1String(Constants::COLOR_NOTE_BG));
    keywords.append(keyword);

    keyword.name = QLatin1String("FIXME");
    keyword.iconResource = QLatin1String(Core::Constants::ICON_ERROR);
    keyword.color = QColor(QLatin1String(Constants::COLOR_FIXME_BG));
    keywords.append(keyword);

    keyword.name = QLatin1String("BUG");
    keyword.iconResource = QLatin1String(Core::Constants::ICON_ERROR);
    keyword.color = QColor(QLatin1String(Constants::COLOR_BUG_BG));
    keywords.append(keyword);

    keyword.name = QLatin1String("WARNING");
    keyword.iconResource = QLatin1String(Core::Constants::ICON_WARNING);
    keyword.color = QColor(QLatin1String(Constants::COLOR_WARNING_BG));
    keywords.append(keyword);
}

bool Settings::equals(const Settings &other) const
{
    return (keywords == other.keywords)
            && (scanningScope == other.scanningScope);
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

