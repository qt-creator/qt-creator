/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androiddeployqtwidget.h"
#include "ui_androiddeployqtwidget.h"

#include "androiddeployqtstep.h"
#include "androidmanager.h"

#include <QFileDialog>

using namespace Android;
using namespace Internal;

AndroidDeployQtWidget::AndroidDeployQtWidget(AndroidDeployQtStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_ui(new Ui::AndroidDeployQtWidget),
      m_step(step)
{
    m_ui->setupUi(this);

    m_ui->uninstallPreviousPackage->setChecked(m_step->uninstallPreviousPackage() > AndroidDeployQtStep::Keep);
    m_ui->uninstallPreviousPackage->setEnabled(m_step->uninstallPreviousPackage() != AndroidDeployQtStep::ForceUnintall);
    connect(m_ui->installMinistroButton, &QAbstractButton::clicked,
            this, &AndroidDeployQtWidget::installMinistro);
    connect(m_ui->cleanLibsPushButton, &QAbstractButton::clicked,
            this, &AndroidDeployQtWidget::cleanLibsOnDevice);
    connect(m_ui->resetDefaultDevices, &QAbstractButton::clicked,
            this, &AndroidDeployQtWidget::resetDefaultDevices);
    connect(m_ui->uninstallPreviousPackage, &QAbstractButton::toggled,
            m_step, &AndroidDeployQtStep::setUninstallPreviousPackage);

}

AndroidDeployQtWidget::~AndroidDeployQtWidget()
{
    delete m_ui;
}

QString AndroidDeployQtWidget::displayName() const
{
    return tr("<b>Deploy configurations</b>");
}

QString AndroidDeployQtWidget::summaryText() const
{
    return displayName();
}

void AndroidDeployQtWidget::installMinistro()
{
    QString packagePath =
        QFileDialog::getOpenFileName(this, tr("Qt Android Smart Installer"),
                                     QDir::homePath(), tr("Android package (*.apk)"));
    if (!packagePath.isEmpty())
        AndroidManager::installQASIPackage(m_step->target(), packagePath);
}

void AndroidDeployQtWidget::cleanLibsOnDevice()
{
    AndroidManager::cleanLibsOnDevice(m_step->target());
}

void AndroidDeployQtWidget::resetDefaultDevices()
{
    AndroidConfigurations::clearDefaultDevices(m_step->project());
}

