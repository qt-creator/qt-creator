/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
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

#include "baremetalrunconfiguration.h"

#include "baremetalconstants.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <QFormLayout>
#include <QLabel>
#include <QDir>

using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

// BareMetalRunConfigurationWidget

class BareMetalRunConfigurationWidget : public QWidget
{
public:
    explicit BareMetalRunConfigurationWidget(BareMetalRunConfiguration *runConfiguration);

private:
    void updateTargetInformation();

    BareMetalRunConfiguration * const m_runConfiguration;
    QLabel m_localExecutableLabel;
};

BareMetalRunConfigurationWidget::BareMetalRunConfigurationWidget(BareMetalRunConfiguration *runConfiguration)
    : m_runConfiguration(runConfiguration)
{
    auto formLayout = new QFormLayout(this);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_localExecutableLabel.setText(m_runConfiguration->localExecutableFilePath());
    formLayout->addRow(BareMetalRunConfiguration::tr("Executable:"), &m_localExecutableLabel);

    //d->genericWidgetsLayout.addRow(tr("Debugger host:"),d->runConfiguration);
    //d->genericWidgetsLayout.addRow(tr("Debugger port:"),d->runConfiguration);
    runConfiguration->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, formLayout);
    runConfiguration->extraAspect<WorkingDirectoryAspect>()->addToMainConfigurationWidget(this, formLayout);

    connect(m_runConfiguration, &BareMetalRunConfiguration::targetInformationChanged,
            this, &BareMetalRunConfigurationWidget::updateTargetInformation);
}

void BareMetalRunConfigurationWidget::updateTargetInformation()
{
    const QString regularText = QDir::toNativeSeparators(m_runConfiguration->localExecutableFilePath());
    const QString errorMessage = "<font color=\"red\">" + tr("Unknown") + "</font>";
    m_localExecutableLabel.setText(regularText.isEmpty() ? errorMessage : regularText);
}


// BareMetalRunConfiguration

BareMetalRunConfiguration::BareMetalRunConfiguration(Target *target)
    : RunConfiguration(target, IdPrefix)
{
    addExtraAspect(new ArgumentsAspect(this, "Qt4ProjectManager.MaemoRunConfiguration.Arguments"));
    addExtraAspect(new WorkingDirectoryAspect(this, "BareMetal.RunConfig.WorkingDirectory"));

    connect(target, &Target::deploymentDataChanged,
            this, &BareMetalRunConfiguration::handleBuildSystemDataUpdated);
    connect(target, &Target::applicationTargetsChanged,
            this, &BareMetalRunConfiguration::handleBuildSystemDataUpdated);
    connect(target, &Target::kitChanged,
            this, &BareMetalRunConfiguration::handleBuildSystemDataUpdated); // Handles device changes, etc.
}

QWidget *BareMetalRunConfiguration::createConfigurationWidget()
{
    return wrapWidget(new BareMetalRunConfigurationWidget(this));
}

QVariantMap BareMetalRunConfiguration::toMap() const
{
    return RunConfiguration::toMap();
}

bool BareMetalRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    return true;
}

QString BareMetalRunConfiguration::localExecutableFilePath() const
{
    const BuildTargetInfo bti = target()->applicationTargets().buildTargetInfo(buildKey());
    return bti.targetFilePath.toString();
}

void BareMetalRunConfiguration::handleBuildSystemDataUpdated()
{
    emit targetInformationChanged();
    emit enabledChanged();
}

const char *BareMetalRunConfiguration::IdPrefix = "BareMetal";


// BareMetalRunConfigurationFactory

BareMetalRunConfigurationFactory::BareMetalRunConfigurationFactory()
{
    registerRunConfiguration<BareMetalRunConfiguration>(BareMetalRunConfiguration::IdPrefix);
    setDecorateDisplayNames(true);
    addSupportedTargetDeviceType(BareMetal::Constants::BareMetalOsType);
}

} // namespace Internal
} // namespace BareMetal

