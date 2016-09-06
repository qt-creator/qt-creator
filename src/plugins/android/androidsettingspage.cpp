/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidsettingspage.h"

#include "androidsettingswidget.h"
#include "androidconstants.h"
#include "androidtoolchain.h"
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>

#include <QCoreApplication>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidSettingsPage::AndroidSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
    setId(Constants::ANDROID_SETTINGS_ID);
    setDisplayName(tr("Android"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
                                       ProjectExplorer::Constants::DEVICE_SETTINGS_TR_CATEGORY));
}

QWidget *AndroidSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new AndroidSettingsWidget;
    return m_widget;
}

void AndroidSettingsPage::apply()
{
    if (m_widget)
        m_widget->saveSettings();
}

void AndroidSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace Android
