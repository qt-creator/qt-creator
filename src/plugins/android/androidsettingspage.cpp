/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidsettingspage.h"

#include "androidsettingswidget.h"
#include "androidconstants.h"
#include "androidtoolchain.h"
#include <projectexplorer/toolchainmanager.h>

#include <QCoreApplication>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidSettingsPage::AndroidSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
    setId(Constants::ANDROID_SETTINGS_ID);
    setDisplayName(tr("Android Configurations"));
    setCategory(Constants::ANDROID_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Android",
        Constants::ANDROID_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::ANDROID_SETTINGS_CATEGORY_ICON));
}

QWidget *AndroidSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new AndroidSettingsWidget;
    return m_widget;
}

void AndroidSettingsPage::apply()
{
    m_widget->saveSettings();
}

void AndroidSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace Android
