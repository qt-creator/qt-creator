/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberrycheckdevicestatusstepconfigwidget.h"
#include "ui_blackberrycheckdevicestatusstepconfigwidget.h"
#include "blackberrycheckdevicestatusstep.h"

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryCheckDeviceStatusStepConfigWidget::BlackBerryCheckDeviceStatusStepConfigWidget(
        BlackBerryCheckDeviceStatusStep *checkDeviceStatuStep)
   : ProjectExplorer::BuildStepConfigWidget()
   , m_checkDeviceStatusStep(checkDeviceStatuStep)
   , m_ui(new Ui::BlackBerryCheckDeviceStatusStepConfigWidget)
{
    m_ui->setupUi(this);
    m_ui->checkRuntime->setChecked(m_checkDeviceStatusStep->runtimeCheckEnabled());
    m_ui->checkDebugToken->setChecked(m_checkDeviceStatusStep->debugTokenCheckEnabled());

    connect(m_ui->checkRuntime, SIGNAL(clicked(bool)),
            m_checkDeviceStatusStep, SLOT(enableRuntimeCheck(bool)));
    connect(m_ui->checkDebugToken, SIGNAL(clicked(bool)),
            m_checkDeviceStatusStep, SLOT(enableDebugTokenCheck(bool)));
}

BlackBerryCheckDeviceStatusStepConfigWidget::~BlackBerryCheckDeviceStatusStepConfigWidget()
{
    delete m_ui;
}

QString BlackBerryCheckDeviceStatusStepConfigWidget::displayName() const
{
    return tr("<b>Check device status</b>");
}

QString BlackBerryCheckDeviceStatusStepConfigWidget::summaryText() const
{
    return displayName();
}

bool BlackBerryCheckDeviceStatusStepConfigWidget::showWidget() const
{
    return true;
}
