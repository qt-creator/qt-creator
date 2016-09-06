/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com)
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

#include "qnxsettingspage.h"
#include "qnxconfiguration.h"
#include "qnxconfigurationmanager.h"
#include "qnxconstants.h"
#include "qnxsettingswidget.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QCoreApplication>

#include <QMessageBox>

namespace Qnx {
namespace Internal {

QnxSettingsPage::QnxSettingsPage(QObject* parent) :
    Core::IOptionsPage(parent)
{
    setId(Core::Id(Constants::QNX_SETTINGS_ID));
    setDisplayName(tr("QNX"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
                                       ProjectExplorer::Constants::DEVICE_SETTINGS_TR_CATEGORY));
}

QWidget* QnxSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new QnxSettingsWidget;
    return m_widget;
}

void QnxSettingsPage::apply()
{
    m_widget->applyChanges();
}

void QnxSettingsPage::finish()
{
    delete m_widget;
}

}
}
