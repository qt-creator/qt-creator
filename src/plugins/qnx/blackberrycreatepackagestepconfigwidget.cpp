/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
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

#include "blackberrycreatepackagestepconfigwidget.h"
#include "ui_blackberrycreatepackagestepconfigwidget.h"
#include "blackberrycreatepackagestep.h"

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryCreatePackageStepConfigWidget::BlackBerryCreatePackageStepConfigWidget(BlackBerryCreatePackageStep *step)
    : ProjectExplorer::BuildStepConfigWidget()
    , m_step(step)
{
    m_ui = new Ui::BlackBerryCreatePackageStepConfigWidget;
    m_ui->setupUi(this);

    m_ui->signPackages->setChecked(m_step->packageMode() == BlackBerryCreatePackageStep::SigningPackageMode);
    m_ui->developmentMode->setChecked(m_step->packageMode() == BlackBerryCreatePackageStep::DevelopmentMode);

    m_ui->cskPassword->setText(m_step->cskPassword());
    m_ui->keystorePassword->setText(m_step->keystorePassword());
    m_ui->savePasswords->setChecked(m_step->savePasswords());

    connect(m_ui->signPackages, SIGNAL(toggled(bool)), this, SLOT(setPackageMode(bool)));
    connect(m_ui->cskPassword, SIGNAL(textChanged(QString)), m_step, SLOT(setCskPassword(QString)));
    connect(m_ui->keystorePassword, SIGNAL(textChanged(QString)), m_step, SLOT(setKeystorePassword(QString)));
    connect(m_ui->showPasswords, SIGNAL(toggled(bool)), this, SLOT(showPasswords(bool)));
    connect(m_ui->savePasswords, SIGNAL(toggled(bool)), m_step, SLOT(setSavePasswords(bool)));
    connect(m_step, SIGNAL(cskPasswordChanged(QString)), m_ui->cskPassword, SLOT(setText(QString)));
    connect(m_step, SIGNAL(keystorePasswordChanged(QString)), m_ui->keystorePassword, SLOT(setText(QString)));

    m_ui->signPackagesWidget->setEnabled(m_ui->signPackages->isChecked());
}

BlackBerryCreatePackageStepConfigWidget::~BlackBerryCreatePackageStepConfigWidget()
{
    delete m_ui;
    m_ui = 0;
}

QString BlackBerryCreatePackageStepConfigWidget::displayName() const
{
    return tr("<b>Create packages</b>");
}

QString BlackBerryCreatePackageStepConfigWidget::summaryText() const
{
    return displayName();
}

bool BlackBerryCreatePackageStepConfigWidget::showWidget() const
{
    return true;
}

void BlackBerryCreatePackageStepConfigWidget::setPackageMode(bool signPackagesChecked)
{
    m_step->setPackageMode(signPackagesChecked ? BlackBerryCreatePackageStep::SigningPackageMode : BlackBerryCreatePackageStep::DevelopmentMode);
}

void BlackBerryCreatePackageStepConfigWidget::showPasswords(bool show)
{
    m_ui->cskPassword->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    m_ui->keystorePassword->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
}
