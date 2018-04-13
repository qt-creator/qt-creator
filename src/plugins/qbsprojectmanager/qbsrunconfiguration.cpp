/****************************************************************************
**
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

#include "qbsrunconfiguration.h"

#include "qbsnodes.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsproject.h"

#include <coreplugin/variablechooser.h>

#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtoutputformatter.h>

#include <QCheckBox>
#include <QFileInfo>
#include <QFormLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

const char QBS_RC_PREFIX[] = "Qbs.RunConfiguration:";

static QString usingLibraryPathsKey() { return QString("Qbs.RunConfiguration.UsingLibraryPaths"); }

// --------------------------------------------------------------------
// QbsRunConfigurationWidget:
// --------------------------------------------------------------------

class QbsRunConfigurationWidget : public QWidget
{
public:
    explicit QbsRunConfigurationWidget(QbsRunConfiguration *rc)
    {
        auto toplayout = new QFormLayout(this);

        rc->extraAspect<ExecutableAspect>()->addToMainConfigurationWidget(this, toplayout);
        rc->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, toplayout);
        rc->extraAspect<WorkingDirectoryAspect>()->addToMainConfigurationWidget(this, toplayout);
        rc->extraAspect<TerminalAspect>()->addToMainConfigurationWidget(this, toplayout);

        auto usingLibPathsCheckBox = new QCheckBox;
        usingLibPathsCheckBox->setText(QbsRunConfiguration::tr("Add library paths to run environment"));
        usingLibPathsCheckBox->setChecked(rc->usingLibraryPaths());
        connect(usingLibPathsCheckBox, &QCheckBox::toggled,
                rc, &QbsRunConfiguration::setUsingLibraryPaths);
        toplayout->addRow(QString(), usingLibPathsCheckBox);

        Core::VariableChooser::addSupportForChildWidgets(this, rc->macroExpander());
    }
};

// --------------------------------------------------------------------
// QbsRunConfiguration:
// --------------------------------------------------------------------

QbsRunConfiguration::QbsRunConfiguration(Target *target)
    : RunConfiguration(target, QBS_RC_PREFIX)
{
    m_usingLibraryPaths = ProjectExplorerPlugin::projectExplorerSettings().addLibraryPathsToRunEnv;

    auto envAspect = new LocalEnvironmentAspect(this,
            [](RunConfiguration *rc, Environment &env) {
                static_cast<QbsRunConfiguration *>(rc)->addToBaseEnvironment(env);
            });
    addExtraAspect(envAspect);

    addExtraAspect(new ExecutableAspect(this));
    addExtraAspect(new ArgumentsAspect(this, "Qbs.RunConfiguration.CommandLineArguments"));
    addExtraAspect(new WorkingDirectoryAspect(this, "Qbs.RunConfiguration.WorkingDirectory"));
    addExtraAspect(new TerminalAspect(this, "Qbs.RunConfiguration.UseTerminal"));

    connect(project(), &Project::parsingFinished, this,
            [envAspect]() { envAspect->buildEnvironmentHasChanged(); });

    connect(target, &Target::deploymentDataChanged,
            this, &QbsRunConfiguration::updateTargetInformation);
    connect(target, &Target::applicationTargetsChanged,
            this, &QbsRunConfiguration::updateTargetInformation);
    // Handles device changes, etc.
    connect(target, &Target::kitChanged,
            this, &QbsRunConfiguration::updateTargetInformation);

    QbsProject *qbsProject = static_cast<QbsProject *>(project());
    connect(qbsProject, &QbsProject::dataChanged, this, [this] { m_envCache.clear(); });
}

QVariantMap QbsRunConfiguration::toMap() const
{
    QVariantMap map = RunConfiguration::toMap();
    map.insert(usingLibraryPathsKey(), usingLibraryPaths());
    return map;
}

bool QbsRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    m_usingLibraryPaths = map.value(usingLibraryPathsKey(), true).toBool();

    updateTargetInformation();
    return true;
}

void QbsRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &info)
{
    setDefaultDisplayName(info.displayName);
    updateTargetInformation();
}

QWidget *QbsRunConfiguration::createConfigurationWidget()
{
    return wrapWidget(new QbsRunConfigurationWidget(this));
}

Runnable QbsRunConfiguration::runnable() const
{
    StandardRunnable r;
    r.executable = extraAspect<ExecutableAspect>()->executable().toString();
    r.workingDirectory = extraAspect<WorkingDirectoryAspect>()->workingDirectory().toString();
    r.commandLineArguments = extraAspect<ArgumentsAspect>()->arguments();
    r.runMode = extraAspect<TerminalAspect>()->runMode();
    r.environment = extraAspect<LocalEnvironmentAspect>()->environment();
    return r;
}

void QbsRunConfiguration::setUsingLibraryPaths(bool useLibPaths)
{
    m_usingLibraryPaths = useLibPaths;
    extraAspect<LocalEnvironmentAspect>()->environmentChanged();
}

void QbsRunConfiguration::addToBaseEnvironment(Utils::Environment &env) const
{
    const auto key = qMakePair(env.toStringList(), m_usingLibraryPaths);
    const auto it = m_envCache.constFind(key);
    if (it != m_envCache.constEnd()) {
        env = it.value();
        return;
    }
    BuildTargetInfo bti = target()->applicationTargets().buildTargetInfo(buildKey());
    if (bti.runEnvModifier)
        bti.runEnvModifier(env, m_usingLibraryPaths);
    m_envCache.insert(key, env);
}

Utils::OutputFormatter *QbsRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

void QbsRunConfiguration::updateTargetInformation()
{
    BuildTargetInfo bti = target()->applicationTargets().buildTargetInfo(buildKey());
    FileName executable = bti.targetFilePath;

    auto terminalAspect = extraAspect<TerminalAspect>();
    if (!terminalAspect->isUserSet())
        terminalAspect->setUseTerminal(bti.usesTerminal);

    extraAspect<ExecutableAspect>()->setExecutable(executable);

    if (!executable.isEmpty()) {
        QString defaultWorkingDir = QFileInfo(executable.toString()).absolutePath();
        if (!defaultWorkingDir.isEmpty()) {
            auto wdAspect = extraAspect<WorkingDirectoryAspect>();
            wdAspect->setDefaultWorkingDirectory(FileName::fromString(defaultWorkingDir));
        }
    }

    emit enabledChanged();
}

bool QbsRunConfiguration::canRunForNode(const Node *node) const
{
    if (auto pn = dynamic_cast<const QbsProductNode *>(node)) {
        const QString uniqueProductName = buildKey();
        return uniqueProductName == QbsProject::uniqueProductName(pn->qbsProductData());
    }

    return false;
}

// --------------------------------------------------------------------
// QbsRunConfigurationFactory:
// --------------------------------------------------------------------

QbsRunConfigurationFactory::QbsRunConfigurationFactory()
{
    registerRunConfiguration<QbsRunConfiguration>(QBS_RC_PREFIX);
    addSupportedProjectType(Constants::PROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

} // namespace Internal
} // namespace QbsProjectManager
