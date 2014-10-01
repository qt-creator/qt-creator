/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrycreatepackagestepconfigwidget.h"
#include "ui_blackberrycreatepackagestepconfigwidget.h"
#include "blackberrycreatepackagestep.h"
#include "qnxdeployqtlibrariesdialog.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryCreatePackageStepConfigWidget::BlackBerryCreatePackageStepConfigWidget(BlackBerryCreatePackageStep *step)
    : ProjectExplorer::BuildStepConfigWidget()
    , m_step(step)
{
    m_ui = new Ui::BlackBerryCreatePackageStepConfigWidget;
    m_ui->setupUi(this);

    m_ui->cskPassword->setText(m_step->cskPassword());
    m_ui->keystorePassword->setText(m_step->keystorePassword());
    m_ui->savePasswords->setChecked(m_step->savePasswords());
    m_ui->qtLibraryPath->setText(m_step->qtLibraryPath());

    m_qtLibraryExplanations[0] = tr("Use the Qt libraries shipped with the BlackBerry device.");
    m_qtLibraryExplanations[1] = tr("Include Qt libraries in the package. "
                                    "This will increase the package size.");
    m_qtLibraryExplanations[2] = tr("Use deployed Qt libraries on the device.");
    m_ui->qtLibrary->addItem(tr("Use Pre-installed Qt"), BlackBerryCreatePackageStep::PreInstalledQt);
    m_ui->qtLibrary->addItem(tr("Bundle Qt in Package"), BlackBerryCreatePackageStep::BundleQt);
    m_ui->qtLibrary->addItem(tr("Use Deployed Qt"), BlackBerryCreatePackageStep::DeployedQt);

    connect(m_ui->signPackages, SIGNAL(toggled(bool)), this, SLOT(setPackageMode(bool)));
    connect(m_ui->cskPassword, SIGNAL(textChanged(QString)), m_step, SLOT(setCskPassword(QString)));
    connect(m_ui->keystorePassword, SIGNAL(textChanged(QString)),
            m_step, SLOT(setKeystorePassword(QString)));
    connect(m_ui->showPasswords, SIGNAL(toggled(bool)), this, SLOT(showPasswords(bool)));
    connect(m_ui->savePasswords, SIGNAL(toggled(bool)), m_step, SLOT(setSavePasswords(bool)));
    connect(m_ui->qtLibrary, SIGNAL(currentIndexChanged(int)), this, SLOT(setBundleMode(int)));
    connect(m_ui->qtLibrary, SIGNAL(currentIndexChanged(int)), this, SLOT(updateDeployWidgetsState()));
    connect(m_ui->qtLibraryPath, SIGNAL(textChanged(QString)),
            m_step, SLOT(setQtLibraryPath(QString)));
    connect(m_ui->qtLibraryPath, SIGNAL(textChanged(QString)),
            this, SLOT(updateDeployWidgetsState()));
    connect(m_ui->deployNowButton, SIGNAL(clicked()), this, SLOT(deployLibraries()));
    connect(ProjectExplorer::KitManager::instance(), SIGNAL(kitsChanged()), this, SLOT(updateDeployWidgetsState()));

    connect(m_step, SIGNAL(cskPasswordChanged(QString)), m_ui->cskPassword, SLOT(setText(QString)));
    connect(m_step, SIGNAL(keystorePasswordChanged(QString)),
            m_ui->keystorePassword, SLOT(setText(QString)));

    m_ui->signPackages->setChecked(m_step->packageMode() ==
                                   BlackBerryCreatePackageStep::SigningPackageMode);
    m_ui->developmentMode->setChecked(m_step->packageMode() ==
                                      BlackBerryCreatePackageStep::DevelopmentMode);

    m_ui->qtLibrary->setCurrentIndex(m_ui->qtLibrary->findData(m_step->bundleMode()));
    setBundleMode(m_step->bundleMode());
    updateDeployWidgetsState();
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

void BlackBerryCreatePackageStepConfigWidget::setBundleMode(int qtLibraryIndex)
{
    QTC_ASSERT(m_qtLibraryExplanations.contains(qtLibraryIndex), return);

    BlackBerryCreatePackageStep::BundleMode bundleMode =
            static_cast<BlackBerryCreatePackageStep::BundleMode>(
                m_ui->qtLibrary->itemData(qtLibraryIndex).toInt());

    m_step->setBundleMode(bundleMode);
    m_ui->qtLibraryExplanationLabel->setText(m_qtLibraryExplanations[qtLibraryIndex]);
    m_ui->qtLibraryPath->setVisible(bundleMode == BlackBerryCreatePackageStep::DeployedQt);
    m_ui->qtLibraryLabel->setVisible(bundleMode == BlackBerryCreatePackageStep::DeployedQt);

    emit bundleModeChanged();
}

void BlackBerryCreatePackageStepConfigWidget::updateDeployWidgetsState()
{
    BlackBerryCreatePackageStep::BundleMode bundleMode =
            static_cast<BlackBerryCreatePackageStep::BundleMode>(
                m_ui->qtLibrary->itemData(m_ui->qtLibrary->currentIndex()).toInt());

    ProjectExplorer::Kit *kit = m_step->target()->kit();
    ProjectExplorer::IDevice::ConstPtr device = ProjectExplorer::DeviceKitInformation::device(kit);

    const bool enableButton = !m_ui->qtLibraryPath->text().isEmpty()
            && bundleMode == BlackBerryCreatePackageStep::DeployedQt
            && !device.isNull();
    const bool visibleButton = bundleMode == BlackBerryCreatePackageStep::DeployedQt;
    const bool visibleLabels = bundleMode == BlackBerryCreatePackageStep::DeployedQt
            && device.isNull();

    m_ui->deployNowButton->setEnabled(enableButton);
    m_ui->deployNowButton->setVisible(visibleButton);

    m_ui->deployErrorPixmap->setVisible(visibleLabels);
    m_ui->deployErrorLabel->setVisible(visibleLabels);
}

void BlackBerryCreatePackageStepConfigWidget::deployLibraries()
{
    ProjectExplorer::Kit *kit = m_step->target()->kit();
    QnxDeployQtLibrariesDialog dlg(ProjectExplorer::DeviceKitInformation::device(kit),
                                   QnxDeployQtLibrariesDialog::BB10,
                                   this);
    dlg.execAndDeploy(QtSupport::QtKitInformation::qtVersionId(kit), m_ui->qtLibraryPath->text());
}
