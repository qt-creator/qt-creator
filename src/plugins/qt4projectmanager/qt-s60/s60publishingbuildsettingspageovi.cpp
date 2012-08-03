/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "s60publishingbuildsettingspageovi.h"
#include "s60publisherovi.h"
#include "ui_s60publishingbuildsettingspageovi.h"
#include "qt4buildconfiguration.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <QAbstractButton>

namespace Qt4ProjectManager {
namespace Internal {

S60PublishingBuildSettingsPageOvi::S60PublishingBuildSettingsPageOvi(S60PublisherOvi *publisher, const ProjectExplorer::Project *project, QWidget *parent) :
    QWizardPage(parent),
    m_bc(0),
    m_toolchain(0),
    m_ui(new Ui::S60PublishingBuildSettingsPageOvi),
    m_publisher(publisher)
{
    m_ui->setupUi(this);

#if 0 // FIXME: This needs serious work!
    QList<Qt4BuildConfiguration *> list;
    foreach (const ProjectExplorer::Target *const target, project->targets()) {
        if (target->id() != Qt4ProjectManager::Constants::S60_DEVICE_TARGET_ID)
            continue;
        foreach (ProjectExplorer::BuildConfiguration * const bc, target->buildConfigurations()) {
            Qt4BuildConfiguration * const qt4bc
                = qobject_cast<Qt4BuildConfiguration *>(bc);
            if (!qt4bc || !qt4bc->qtVersion())
                continue;
            if (qt4bc->qtVersion()->qtVersion() > QtSupport::QtVersionNumber(4, 6, 2))
                list << qt4bc;
        }
        break;
    }

    foreach (Qt4BuildConfiguration *qt4bc, list)
        m_ui->chooseBuildConfigDropDown->addItem(qt4bc->displayName(), QVariant::fromValue(static_cast<ProjectExplorer::BuildConfiguration *>(qt4bc)));

    // todo more intelligent selection? prefer newer versions?
    foreach (Qt4BuildConfiguration *qt4bc, list)
        if (!m_bc && !(qt4bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild))
            m_bc = qt4bc;

    if (!m_bc && !list.isEmpty())
        m_bc = list.first();

    m_ui->chooseBuildConfigDropDown->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    int focusedIndex = m_ui->chooseBuildConfigDropDown->findData(QVariant::fromValue(m_bc));
    m_ui->chooseBuildConfigDropDown->setCurrentIndex(focusedIndex);
    m_ui->chooseBuildConfigDropDown->setEnabled(!list.isEmpty());
    m_publisher->setBuildConfiguration(static_cast<Qt4BuildConfiguration *>(m_bc));
    m_ui->buildConfigInfoLabel->setVisible(list.isEmpty());

    m_ui->buildConfigInfoLabel->setToolTip(tr("No valid Qt version has been detected.<br>"
                                         "Define a correct Qt version in \"Options > Qt4\""));
    m_ui->toolchainInfoIconLabel->setToolTip(tr("No valid tool chain has been detected.<br>"
                                         "Define a correct tool chain in \"Options > Tool Chains\""));
    populateToolchainList(m_bc);
#else
    Q_UNUSED(project);
#endif

    //change the build configuration if the user changes it
    connect(m_ui->chooseBuildConfigDropDown, SIGNAL(currentIndexChanged(int)), this, SLOT(buildConfigChosen()));
    connect(this, SIGNAL(configurationChosen()), SIGNAL(completeChanged()));
    connect(this, SIGNAL(toolchainConfigurationChosen()), SIGNAL(completeChanged()));
}

bool S60PublishingBuildSettingsPageOvi::isComplete() const
{
    return m_bc && m_toolchain;
}

void S60PublishingBuildSettingsPageOvi::populateToolchainList(ProjectExplorer::BuildConfiguration *bc)
{
#if 0 // FIXME: Do the right thing here...
    if (!bc)
        return;

    disconnect(m_ui->chooseToolchainDropDown, SIGNAL(currentIndexChanged(int)), this, SLOT(toolchainChosen()));
    m_ui->chooseToolchainDropDown->clear();
    QList<ProjectExplorer::ToolChain *> toolchains = bc->target()->possibleToolChains(bc);

    int index = 0;
    bool toolchainChanged = true; // if the new build conf. doesn't contain previous toolchain
    foreach (ProjectExplorer::ToolChain *toolchain, toolchains) {
        m_ui->chooseToolchainDropDown->addItem(toolchain->displayName(),
                                               qVariantFromValue(static_cast<void *>(toolchain)));
        if (toolchainChanged && m_toolchain == toolchain) {
            toolchainChanged = false;
            m_ui->chooseToolchainDropDown->setCurrentIndex(index);
        }
        ++index;
    }

    connect(m_ui->chooseToolchainDropDown, SIGNAL(currentIndexChanged(int)), this, SLOT(toolchainChosen()));

    m_ui->toolchainInfoIconLabel->setVisible(!toolchains.size());
    m_ui->chooseToolchainDropDown->setEnabled(toolchains.size() > 1);

    if (toolchainChanged)
        toolchainChosen();
    else
        bc->setToolChain(m_toolchain);
#else
    Q_UNUSED(bc);
#endif
}

void S60PublishingBuildSettingsPageOvi::buildConfigChosen()
{
    int currentIndex = m_ui->chooseBuildConfigDropDown->currentIndex();
    if (currentIndex == -1)
        return;
    m_bc = m_ui->chooseBuildConfigDropDown->itemData(currentIndex).value<ProjectExplorer::BuildConfiguration *>();
    populateToolchainList(m_bc);
    m_publisher->setBuildConfiguration(static_cast<Qt4BuildConfiguration *>(m_bc));
    emit configurationChosen();
}

void S60PublishingBuildSettingsPageOvi::toolchainChosen()
{
#if 0 // FIXME: Do the right thing!
    const int currentIndex = m_ui->chooseToolchainDropDown->currentIndex();
    if (currentIndex == -1) {
        m_toolchain = 0;
        emit toolchainConfigurationChosen();
        return;
    }

    m_toolchain = static_cast<ProjectExplorer::ToolChain *>(m_ui->chooseToolchainDropDown->itemData(currentIndex, Qt::UserRole).value<void *>());

    if (m_bc)
        m_bc->setToolChain(m_toolchain);
#endif
    emit toolchainConfigurationChosen();
}

S60PublishingBuildSettingsPageOvi::~S60PublishingBuildSettingsPageOvi()
{
    delete m_ui;
}

} // namespace Internal
} // namespace Qt4ProjectManager
