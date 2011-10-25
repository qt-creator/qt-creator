/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Dmitry Savchenko.
** Copyright (c) 2010 Vasiliy Sorokin.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "settings.h"
#include "constants.h"

namespace Todo {
namespace Internal {

void Settings::save(QSettings *settings) const
{
    settings->beginGroup(Constants::SETTINGS_GROUP);
    settings->setValue(Constants::SCANNING_SCOPE, scanningScope);

    settings->beginWriteArray(Constants::KEYWORDS_LIST);
    for (int i = 0; i < keywords.size(); ++i) {
        settings->setArrayIndex(i);
        settings->setValue("name", keywords.at(i).name);
        settings->setValue("color", keywords.at(i).color);
        settings->setValue("iconResource", keywords.at(i).iconResource);
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
        scanningScope).toInt());

    KeywordList newKeywords;
    int size = settings->beginReadArray(Constants::KEYWORDS_LIST);
    if (size > 0) {
        for (int i = 0; i < size; ++i) {
            settings->setArrayIndex(i);
            Keyword keyword;
            keyword.name = settings->value("name").toString();
            keyword.color = settings->value("color").value<QColor>();
            keyword.iconResource = settings->value("iconResource").toString();
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
    keyword.iconResource = QLatin1String(Constants::ICON_WARNING);
    keyword.color = QColor(QLatin1String(Constants::COLOR_TODO_BG));
    keywords.append(keyword);

    keyword.name = QLatin1String("NOTE");
    keyword.iconResource = QLatin1String(Constants::ICON_INFO);
    keyword.color = QColor(QLatin1String(Constants::COLOR_NOTE_BG));
    keywords.append(keyword);

    keyword.name = QLatin1String("FIXME");
    keyword.iconResource = QLatin1String(Constants::ICON_ERROR);
    keyword.color = QColor(QLatin1String(Constants::COLOR_FIXME_BG));
    keywords.append(keyword);

    keyword.name = QLatin1String("BUG");
    keyword.iconResource = QLatin1String(Constants::ICON_ERROR);
    keyword.color = QColor(QLatin1String(Constants::COLOR_BUG_BG));
    keywords.append(keyword);

    keyword.name = QLatin1String("WARNING");
    keyword.iconResource = QLatin1String(Constants::ICON_WARNING);
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

