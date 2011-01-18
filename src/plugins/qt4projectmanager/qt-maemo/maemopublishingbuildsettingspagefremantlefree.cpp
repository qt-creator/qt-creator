/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "maemopublishingbuildsettingspagefremantlefree.h"
#include "ui_maemopublishingbuildsettingspagefremantlefree.h"

#include "maemoglobal.h"
#include "maemopublisherfremantlefree.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

MaemoPublishingBuildSettingsPageFremantleFree::MaemoPublishingBuildSettingsPageFremantleFree(const Project *project,
    MaemoPublisherFremantleFree *publisher, QWidget *parent) :
    QWizardPage(parent),
    m_publisher(publisher),
    ui(new Ui::MaemoPublishingWizardPageFremantleFree)
{
    ui->setupUi(this);
    collectBuildConfigurations(project);
    QTC_ASSERT(!m_buildConfigs.isEmpty(), return);
    foreach (const Qt4BuildConfiguration * const bc, m_buildConfigs) {
        ui->buildConfigComboBox->addItem(bc->displayName());
    }
    ui->buildConfigComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    ui->buildConfigComboBox->setCurrentIndex(0);
    connect(ui->skipUploadCheckBox, SIGNAL(toggled(bool)),
        SLOT(handleNoUploadSettingChanged()));
}

MaemoPublishingBuildSettingsPageFremantleFree::~MaemoPublishingBuildSettingsPageFremantleFree()
{
    delete ui;
}

void MaemoPublishingBuildSettingsPageFremantleFree::collectBuildConfigurations(const Project *project)
{
    foreach (const Target *const target, project->targets()) {
        if (target->id() != QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID))
            continue;
        foreach (BuildConfiguration * const bc, target->buildConfigurations()) {
            Qt4BuildConfiguration * const qt4Bc
                = qobject_cast<Qt4BuildConfiguration *>(bc);
            if (!qt4Bc)
                continue;
            if (MaemoGlobal::version(qt4Bc->qtVersion()) == MaemoGlobal::Maemo5)
                m_buildConfigs << qt4Bc;
        }
        break;
    }
}

bool MaemoPublishingBuildSettingsPageFremantleFree::validatePage()
{
    m_publisher->setBuildConfiguration(m_buildConfigs.at(ui->buildConfigComboBox->currentIndex()));
    m_publisher->setDoUpload(!skipUpload());
    return true;
}

void MaemoPublishingBuildSettingsPageFremantleFree::handleNoUploadSettingChanged()
{
    setCommitPage(skipUpload());
}

bool MaemoPublishingBuildSettingsPageFremantleFree::skipUpload() const
{
    return ui->skipUploadCheckBox->isChecked();
}

} // namespace Internal
} // namespace Qt4ProjectManager
