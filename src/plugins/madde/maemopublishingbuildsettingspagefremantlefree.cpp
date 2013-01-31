/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "maemopublishingbuildsettingspagefremantlefree.h"
#include "ui_maemopublishingbuildsettingspagefremantlefree.h"

#include "maemoconstants.h"
#include "maemoglobal.h"
#include "maemopublisherfremantlefree.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace Madde {
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

    foreach (const Qt4BuildConfiguration *const bc, m_buildConfigs)
        ui->buildConfigComboBox->addItem(bc->displayName());

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
    m_buildConfigs.clear();

    foreach (const Target *const target, project->targets()) {
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
        if (!version || version->platformName() != QLatin1String(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM))
            continue;
        foreach (BuildConfiguration *const bc, target->buildConfigurations()) {
            Qt4BuildConfiguration *const qt4Bc = qobject_cast<Qt4BuildConfiguration *>(bc);
            if (qt4Bc)
                m_buildConfigs << qt4Bc;
        }
    }
}

void MaemoPublishingBuildSettingsPageFremantleFree::initializePage()
{
    ui->skipUploadCheckBox->setChecked(true);
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
} // namespace Madde
