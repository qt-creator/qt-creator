/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "androiddeploystepwidget.h"
#include "ui_androiddeploystepwidget.h"

#include "androiddeploystep.h"
#include "androidrunconfiguration.h"

#include <coreplugin/icore.h>

#include <QFileDialog>

namespace Android {
namespace Internal {

AndroidDeployStepWidget::AndroidDeployStepWidget(AndroidDeployStep *step) :
    ProjectExplorer::BuildStepConfigWidget(),
    ui(new Ui::AndroidDeployStepWidget),
    m_step(step)
{
    ui->setupUi(this);

    ui->useLocalQtLibs->setChecked(m_step->useLocalQtLibs());
    switch (m_step->deployAction()) {
    case AndroidDeployStep::DeployLocal:
        ui->deployQtLibs->setChecked(true);
        break;
    default:
        ui->devicesQtLibs->setChecked(true);
        break;
    }

    connect(m_step, SIGNAL(resetDelopyAction()), SLOT(resetAction()));
    connect(ui->devicesQtLibs, SIGNAL(clicked()), SLOT(resetAction()));
    connect(ui->deployQtLibs, SIGNAL(clicked()), SLOT(setDeployLocalQtLibs()));
    connect(ui->chooseButton, SIGNAL(clicked()), SLOT(setQASIPackagePath()));
    connect(ui->useLocalQtLibs, SIGNAL(stateChanged(int)), SLOT(useLocalQtLibsStateChanged(int)));
    connect(ui->editRulesFilePushButton, SIGNAL(clicked()), SLOT(editRulesFile()));
}

AndroidDeployStepWidget::~AndroidDeployStepWidget()
{
    delete ui;
}

QString AndroidDeployStepWidget::displayName() const
{
    return tr("<b>Deploy configurations</b>");
}

QString AndroidDeployStepWidget::summaryText() const
{
    return displayName();
}

void AndroidDeployStepWidget::resetAction()
{
    ui->devicesQtLibs->setChecked(true);
    m_step->setDeployAction(AndroidDeployStep::NoDeploy);
}

void AndroidDeployStepWidget::setDeployLocalQtLibs()
{
    m_step->setDeployAction(AndroidDeployStep::DeployLocal);
}

void AndroidDeployStepWidget::setQASIPackagePath()
{
    QString packagePath =
        QFileDialog::getOpenFileName(this, tr("Qt Android Smart Installer"),
                                     QDir::homePath(), tr("Android package (*.apk)"));
    if (packagePath.length())
        m_step->setDeployQASIPackagePath(packagePath);
}

void AndroidDeployStepWidget::useLocalQtLibsStateChanged(int state)
{
    m_step->setUseLocalQtLibs(state == Qt::Checked);
}

void AndroidDeployStepWidget::editRulesFile()
{
    Core::ICore::instance()->openFiles(QStringList() << m_step->localLibsRulesFilePath().toString(), Core::ICore::SwitchMode);
}

} // namespace Internal
} // namespace Qt4ProjectManager
