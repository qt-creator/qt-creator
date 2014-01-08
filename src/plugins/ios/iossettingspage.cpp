/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "iossettingspage.h"

#include "iossettingswidget.h"
#include "iosconstants.h"

#include <QCoreApplication>

namespace Ios {
namespace Internal {

IosSettingsPage::IosSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
    setId(Constants::IOS_SETTINGS_ID);
    setDisplayName(tr("iOS Configurations"));
    setCategory(Constants::IOS_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("iOS",
        Constants::IOS_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::IOS_SETTINGS_CATEGORY_ICON));
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
