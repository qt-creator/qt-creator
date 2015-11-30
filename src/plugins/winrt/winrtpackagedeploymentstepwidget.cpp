/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "winrtpackagedeploymentstepwidget.h"
#include <ui_winrtpackagedeploymentstepwidget.h>
#include <coreplugin/coreicons.h>
#include <QIcon>

namespace WinRt {
namespace Internal {

WinRtPackageDeploymentStepWidget::WinRtPackageDeploymentStepWidget(WinRtPackageDeploymentStep *step)
    : m_ui(new Ui::WinRtPackageDeploymentStepWidget)
    , m_step(step)
{
    m_ui->setupUi(this);
    m_ui->leArguments->setText(m_step->winDeployQtArguments());
    m_ui->btnRestoreDefaultArgs->setIcon(Core::Icons::RESET.icon());
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

void WinRtPackageDeploymentStepWidget::on_btnRestoreDefaultArgs_clicked()
{
    m_ui->leArguments->setText(m_step->defaultWinDeployQtArguments());
}

void WinRtPackageDeploymentStepWidget::on_leArguments_textChanged(QString str)
{
    m_step->setWinDeployQtArguments(str);
}

} // namespace Internal
} // namespace WinRt
