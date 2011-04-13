/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "s60publishingbuildsettingspageovi.h"
#include "s60publisherovi.h"
#include "ui_s60publishingbuildsettingspageovi.h"
#include "qt4buildconfiguration.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <QtGui/QAbstractButton>

namespace Qt4ProjectManager {
namespace Internal {

S60PublishingBuildSettingsPageOvi::S60PublishingBuildSettingsPageOvi(S60PublisherOvi *publisher, const ProjectExplorer::Project *project, QWidget *parent) :
    QWizardPage(parent),
    m_ui(new Ui::S60PublishingBuildSettingsPageOvi),
    m_publisher(publisher)
{
    m_ui->setupUi(this);

    QList<Qt4BuildConfiguration *> list;
    foreach (const ProjectExplorer::Target *const target, project->targets()) {
        if (target->id() != QLatin1String(Qt4ProjectManager::Constants::S60_DEVICE_TARGET_ID))
            continue;
        foreach (ProjectExplorer::BuildConfiguration * const bc, target->buildConfigurations()) {
            Qt4BuildConfiguration * const qt4bc
                = qobject_cast<Qt4BuildConfiguration *>(bc);
            if (!qt4bc)
                continue;
            if (qt4bc->qtVersion()->qtVersion() > QtVersionNumber(4, 6, 2))
                list << qt4bc;
        }
        break;
    }

    foreach (Qt4BuildConfiguration *qt4bc, list)
        m_ui->chooseBuildConfigDropDown->addItem(qt4bc->displayName(), QVariant::fromValue(static_cast<ProjectExplorer::BuildConfiguration *>(qt4bc)));


    m_bc = 0;

    // todo more intelligent selection? prefer newer versions?
    foreach (Qt4BuildConfiguration *qt4bc, list)
        if (!m_bc && !(qt4bc->qmakeBuildConfiguration() & QtVersion::DebugBuild))
            m_bc = qt4bc;

    if (!m_bc && !list.isEmpty())
        m_bc = list.first();

    m_ui->chooseBuildConfigDropDown->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    int focusedIndex = m_ui->chooseBuildConfigDropDown->findData(QVariant::fromValue(m_bc));
    m_ui->chooseBuildConfigDropDown->setCurrentIndex(focusedIndex);
    m_publisher->setBuildConfiguration(static_cast<Qt4BuildConfiguration *>(m_bc));
    //change the build configuration if the user changes it
    connect(m_ui->chooseBuildConfigDropDown, SIGNAL(currentIndexChanged(int)), this, SLOT(buildConfigChosen()));
    connect(this, SIGNAL(buildChosen()), SIGNAL(completeChanged()));
}

bool S60PublishingBuildSettingsPageOvi::isComplete() const
{
    return (m_bc != 0);
}

void S60PublishingBuildSettingsPageOvi::buildConfigChosen()
{
    int currentIndex = m_ui->chooseBuildConfigDropDown->currentIndex();
    if (currentIndex == -1)
        return;
    m_bc = m_ui->chooseBuildConfigDropDown->itemData(currentIndex).value<ProjectExplorer::BuildConfiguration *>();
    m_publisher->setBuildConfiguration(static_cast<Qt4BuildConfiguration *>(m_bc));
    emit buildChosen();
}

S60PublishingBuildSettingsPageOvi::~S60PublishingBuildSettingsPageOvi()
{
    delete m_ui;
}

} // namespace Internal
} // namespace Qt4ProjectManager
