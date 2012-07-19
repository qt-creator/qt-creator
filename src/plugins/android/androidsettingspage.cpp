/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "androidsettingspage.h"

#include "androidsettingswidget.h"
#include "androidconstants.h"

#include <QCoreApplication>

namespace Android {
namespace Internal {

AndroidSettingsPage::AndroidSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
    setId(QLatin1String(Constants::ANDROID_SETTINGS_ID));
    setDisplayName(tr("Android Configurations"));
    setCategory(QLatin1String(Constants::ANDROID_SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("Android",
        Constants::ANDROID_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::ANDROID_SETTINGS_CATEGORY_ICON));
}

bool AndroidSettingsPage::matches(const QString &searchKeyWord) const
{
    return m_keywords.contains(searchKeyWord, Qt::CaseInsensitive);
}

QWidget *AndroidSettingsPage::createPage(QWidget *parent)
{
    m_widget = new AndroidSettingsWidget(parent);
    if (m_keywords.isEmpty())
        m_keywords = m_widget->searchKeywords();
    return m_widget;
}

void AndroidSettingsPage::apply()
{
    m_widget->saveSettings();
}

void AndroidSettingsPage::finish()
{
}

} // namespace Internal
} // namespace Android
