/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "iossettingspage.h"

#include "iossettingswidget.h"
#include "iosconstants.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <QCoreApplication>

namespace Ios {
namespace Internal {

IosSettingsPage::IosSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
    setId(Constants::IOS_SETTINGS_ID);
    setDisplayName(tr("iOS"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
                                       ProjectExplorer::Constants::DEVICE_SETTINGS_TR_CATEGORY));
}

QWidget *IosSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new IosSettingsWidget;
    return m_widget;
}

void IosSettingsPage::apply()
{
    m_widget->saveSettings();
    IosConfigurations::updateAutomaticKitList();
}

void IosSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace Ios
