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

#include "winrtpackagedeploymentstepwidget.h"
#include <ui_winrtpackagedeploymentstepwidget.h>
#include <utils/utilsicons.h>
#include <QIcon>

namespace WinRt {
namespace Internal {

WinRtPackageDeploymentStepWidget::WinRtPackageDeploymentStepWidget(WinRtPackageDeploymentStep *step)
    : m_ui(new Ui::WinRtPackageDeploymentStepWidget)
    , m_step(step)
{
    m_ui->setupUi(this);
    m_ui->leArguments->setText(m_step->winDeployQtArguments());
    m_ui->btnRestoreDefaultArgs->setIcon(Utils::Icons::RESET.icon());
    connect(m_ui->btnRestoreDefaultArgs, &QToolButton::pressed,
            this, &WinRtPackageDeploymentStepWidget::restoreDefaultArguments);
    connect(m_ui->leArguments, &QLineEdit::textChanged,
            m_step, &WinRtPackageDeploymentStep::setWinDeployQtArguments);
}

WinRtPackageDeploymentStepWidget::~WinRtPackageDeploymentStepWidget()
{
    delete m_ui;
}

QString WinRtPackageDeploymentStepWidget::summaryText() const
{
    return displayName();
}

QString WinRtPackageDeploymentStepWidget::displayName() const
{
    return m_step->displayName();
}

void WinRtPackageDeploymentStepWidget::restoreDefaultArguments()
{
    m_ui->leArguments->setText(m_step->defaultWinDeployQtArguments());
}

} // namespace Internal
} // namespace WinRt
