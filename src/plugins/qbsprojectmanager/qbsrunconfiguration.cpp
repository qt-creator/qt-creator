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

#include <coreplugin/messagemanager.h>
#include <coreplugin/variablechooser.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/persistentsettings.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

const char QBS_RC_PREFIX[] = "Qbs.RunConfiguration:";

static QString rcNameSeparator() { return QLatin1String("---Qbs.RC.NameSeparator---"); }

static QString usingLibraryPathsKey() { return QString("Qbs.RunConfiguration.UsingLibraryPaths"); }

// --------------------------------------------------------------------
// QbsRunConfigurationWidget:
// --------------------------------------------------------------------

class QbsRunConfigurationWidget : public QWidget
{
public:
    explicit QbsRunConfigurationWidget(QbsRunConfiguration *rc);

private:
    void targetInformationHasChanged();

    QbsRunConfiguration *m_runConfiguration = nullptr;
    QLabel *m_executableLineLabel = nullptr;
};

QbsRunConfigurationWidget::QbsRunConfigurationWidget(QbsRunConfiguration *rc)
{
    m_runConfiguration = rc;

    auto toplayout = new QFormLayout(this);
    toplayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    toplayout->setMargin(0);

    m_executableLineLabel = new QLabel(this);
    m_executableLineLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    toplayout->addRow(QbsRunConfiguration::tr("Executable:"), m_executableLineLabel);

    m_runConfiguration->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, toplayout);
    m_runConfiguration->extraAspect<WorkingDirectoryAspect>()->addToMainConfigurationWidget(this, toplayout);
    m_runConfiguration->extraAspect<TerminalAspect>()->addToMainConfigurationWidget(this, toplayout);

    auto usingLibPathsCheckBox = new QCheckBox;
    usingLibPathsCheckBox->setText(QbsRunConfiguration::tr("Add library paths to run environment"));
    usingLibPathsCheckBox->setChecked(m_runConfiguration->usingLibraryPaths());
    connect(usingLibPathsCheckBox, &QCheckBox::toggled,
            m_runConfiguration, &QbsRunConfiguration::setUsingLibraryPaths);
    toplayout->addRow(QString(), usingLibPathsCheckBox);

    connect(m_runConfiguration, &QbsRunConfiguration::targetInformationChanged,
            this, &QbsRunConfigurationWidget::targetInformationHasChanged, Qt::QueuedConnection);

    connect(m_runConfiguration, &RunConfiguration::enabledChanged,
            this, &QbsRunConfigurationWidget::targetInformationHasChanged);

    Core::VariableChooser::addSupportForChildWidgets(this, rc->macroExpander());

    targetInformationHasChanged();
}

void QbsRunConfigurationWidget::targetInformationHasChanged()
{
    WorkingDirectoryAspect *aspect = m_runConfiguration->extraAspect<WorkingDirectoryAspect>();
    aspect->pathChooser()->setBaseFileName(m_runConfiguration->target()->project()->projectDirectory());

    QString text = m_runConfiguration->executable();
    m_executableLineLabel->setText(text.isEmpty() ? QbsRunConfiguration::tr("<unknown>") : text);
}

// --------------------------------------------------------------------
// QbsRunConfiguration:
// --------------------------------------------------------------------

QbsRunConfiguration::QbsRunConfiguration(Target *target)
    : RunConfiguration(target, QBS_RC_PREFIX)
{
    auto envAspect = new LocalEnvironmentAspect(this,
            [](RunConfiguration *rc, Environment &env) {
                static_cast<QbsRunConfiguration *>(rc)->addToBaseEnvironment(env);
            });
    addExtraAspect(envAspect);

    connect(project(), &Project::parsingFinished, this,
            [envAspect]() { envAspect->buildEnvironmentHasChanged(); });
    addExtraAspect(new ArgumentsAspect(this, "Qbs.RunConfiguration.CommandLineArguments"));
    addExtraAspect(new WorkingDirectoryAspect(this, "Qbs.RunConfiguration.WorkingDirectory"));

    addExtraAspect(new TerminalAspect(this, "Qbs.RunConfiguration.UseTerminal", isConsoleApplication()));

    connect(project(), &Project::parsingFinished, this, [this](bool success) {
        auto terminalAspect = extraAspect<TerminalAspect>();
        if (success && !terminalAspect->isUserSet())
            terminalAspect->setUseTerminal(isConsoleApplication());
    });
    connect(BuildManager::instance(), &BuildManager::buildStateChanged, this,
            [this](Project *p) {
                if (p == project() && !BuildManager::isBuilding(p)) {
                    const QString defaultWorkingDir = baseWorkingDirectory();
                    if (!defaultWorkingDir.isEmpty()) {
                        extraAspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(
                                    Utils::FileName::fromString(defaultWorkingDir));
                    }
                    emit enabledChanged();
                }
            }
    );

    connect(target, &Target::deploymentDataChanged,
            this, &QbsRunConfiguration::handleBuildSystemDataUpdated);
    connect(target, &Target::applicationTargetsChanged,
            this, &QbsRunConfiguration::handleBuildSystemDataUpdated);
    // Handles device changes, etc.
    connect(target, &Target::kitChanged,
            this, &QbsRunConfiguration::handleBuildSystemDataUpdated);
}

QbsRunConfiguration::~QbsRunConfiguration()
{
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

    m_buildKey = ProjectExplorer::idFromMap(map).suffixAfter(id());
    m_usingLibraryPaths = map.value(usingLibraryPathsKey(), true).toBool();

    setDefaultDisplayName(defaultDisplayName());

    return true;
}

QString QbsRunConfiguration::extraId() const
{
    return m_buildKey;
}

void QbsRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &rci)
{
    m_buildKey = rci.buildKey;
    setDefaultDisplayName(defaultDisplayName());
}

QWidget *QbsRunConfiguration::createConfigurationWidget()
{
    return wrapWidget(new QbsRunConfigurationWidget(this));
}

Runnable QbsRunConfiguration::runnable() const
{
    StandardRunnable r;
    r.executable = executable();
    r.workingDirectory = extraAspect<WorkingDirectoryAspect>()->workingDirectory().toString();
    r.commandLineArguments = extraAspect<ArgumentsAspect>()->arguments();
    r.runMode = extraAspect<TerminalAspect>()->runMode();
    r.environment = extraAspect<LocalEnvironmentAspect>()->environment();
    return r;
}

QString QbsRunConfiguration::executable() const
{
    BuildTargetInfo bti = target()->applicationTargets().buildTargetInfo(m_buildKey);
    return bti.targetFilePath.toString();
}

bool QbsRunConfiguration::isConsoleApplication() const
{
    BuildTargetInfo bti = target()->applicationTargets().buildTargetInfo(m_buildKey);
    return bti.usesTerminal;
}

void QbsRunConfiguration::setUsingLibraryPaths(bool useLibPaths)
{
    m_usingLibraryPaths = useLibPaths;
    extraAspect<LocalEnvironmentAspect>()->environmentChanged();
}

QString QbsRunConfiguration::baseWorkingDirectory() const
{
    const QString exe = executable();
    if (!exe.isEmpty())
        return QFileInfo(exe).absolutePath();
    return QString();
}

void QbsRunConfiguration::addToBaseEnvironment(Utils::Environment &env) const
{
    BuildTargetInfo bti = target()->applicationTargets().buildTargetInfo(m_buildKey);
    if (bti.runEnvModifier)
        bti.runEnvModifier(env, m_usingLibraryPaths);
}

QString QbsRunConfiguration::buildSystemTarget() const
{
    return m_buildKey;
}

QString QbsRunConfiguration::defaultDisplayName()
{
    const int sepPos = m_buildKey.indexOf(rcNameSeparator());
    if (sepPos == -1)
        return tr("Qbs Run Configuration");
    return m_buildKey.mid(sepPos + rcNameSeparator().size());
}

Utils::OutputFormatter *QbsRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

void QbsRunConfiguration::handleBuildSystemDataUpdated()
{
    emit targetInformationChanged();
    emit enabledChanged();
}

bool QbsRunConfiguration::canRunForNode(const Node *node) const
{
    if (auto pn = dynamic_cast<const QbsProductNode *>(node)) {
        const int sepPos = m_buildKey.indexOf(rcNameSeparator());
        const QString uniqueProductName = m_buildKey.left(sepPos);
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
