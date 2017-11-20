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

#include "cmakerunconfiguration.h"

#include "cmakebuildconfiguration.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"

#include <coreplugin/coreicons.h>
#include <coreplugin/helpmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtoutputformatter.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <utils/detailswidget.h>
#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QFormLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QToolButton>
#include <QCheckBox>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace ProjectExplorer;

namespace {
const char CMAKE_RC_PREFIX[] = "CMakeProjectManager.CMakeRunConfiguration.";
const char TITLE_KEY[] = "CMakeProjectManager.CMakeRunConfiguation.Title";
} // namespace

CMakeRunConfiguration::CMakeRunConfiguration(Target *target)
    : RunConfiguration(target)
{
    // Workaround for QTCREATORBUG-19354:
    auto cmakeRunEnvironmentModifier = [](RunConfiguration *rc, Utils::Environment &env) {
        if (!Utils::HostOsInfo::isWindowsHost() || !rc)
            return;

        const Kit *k = rc->target()->kit();
        const QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(k);
        if (qt)
            env.prependOrSetPath(qt->qmakeProperty("QT_INSTALL_BINS"));
    };
    addExtraAspect(new LocalEnvironmentAspect(this, cmakeRunEnvironmentModifier));
    addExtraAspect(new ArgumentsAspect(this, "CMakeProjectManager.CMakeRunConfiguration.Arguments"));
    addExtraAspect(new TerminalAspect(this, "CMakeProjectManager.CMakeRunConfiguration.UseTerminal"));
    addExtraAspect(new WorkingDirectoryAspect(this, "CMakeProjectManager.CMakeRunConfiguration.UserWorkingDirectory"));
}

void CMakeRunConfiguration::initialize(Core::Id id)
{
    RunConfiguration::initialize(id);
    QString executable = id.suffixAfter(CMAKE_RC_PREFIX);

    CMakeProject *project = static_cast<CMakeProject *>(target()->project());

    m_buildSystemTarget = executable;
    m_executable = executable;

    if (!executable.isEmpty()) {
        const CMakeBuildTarget ct = project->buildTargetForTitle(executable);
        m_title = ct.title;
        extraAspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(ct.workingDirectory);
    }

    setDefaultDisplayName(defaultDisplayName());
}

Runnable CMakeRunConfiguration::runnable() const
{
    StandardRunnable r;
    r.executable = m_executable;
    r.commandLineArguments = extraAspect<ArgumentsAspect>()->arguments();
    r.workingDirectory = extraAspect<WorkingDirectoryAspect>()->workingDirectory().toString();
    r.environment = extraAspect<LocalEnvironmentAspect>()->environment();
    r.runMode = extraAspect<TerminalAspect>()->runMode();
    return r;
}

QString CMakeRunConfiguration::baseWorkingDirectory() const
{
    const QString exe = m_executable;
    if (!exe.isEmpty())
        return QFileInfo(m_executable).absolutePath();
    return QString();
}

QString CMakeRunConfiguration::title() const
{
    return m_title;
}

void CMakeRunConfiguration::setExecutable(const QString &executable)
{
    m_executable = executable;
}

void CMakeRunConfiguration::setBaseWorkingDirectory(const Utils::FileName &wd)
{
    extraAspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(wd);
}

QVariantMap CMakeRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(QLatin1String(TITLE_KEY), m_title);
    return map;
}

bool CMakeRunConfiguration::fromMap(const QVariantMap &map)
{
    m_title = map.value(QLatin1String(TITLE_KEY)).toString();
    return RunConfiguration::fromMap(map);
}

QString CMakeRunConfiguration::defaultDisplayName() const
{
    if (m_title.isEmpty())
        return tr("Run CMake kit");
    return m_title;
}

void CMakeRunConfiguration::updateEnabledState()
{
    auto cp = qobject_cast<CMakeProject *>(target()->project());
    if (!cp->hasBuildTarget(m_buildSystemTarget))
        setEnabled(false);
    else
        RunConfiguration::updateEnabledState();
}

QWidget *CMakeRunConfiguration::createConfigurationWidget()
{
    return new CMakeRunConfigurationWidget(this);
}

QString CMakeRunConfiguration::disabledReason() const
{
    auto cp = qobject_cast<CMakeProject *>(target()->project());
    QTC_ASSERT(cp, return QString());

    if (!cp->hasBuildTarget(m_buildSystemTarget))
        return tr("The project no longer builds the target associated with this run configuration.");
    return RunConfiguration::disabledReason();
}

Utils::OutputFormatter *CMakeRunConfiguration::createOutputFormatter() const
{
    if (QtSupport::QtKitInformation::qtVersion(target()->kit()))
        return new QtSupport::QtOutputFormatter(target()->project());
    return RunConfiguration::createOutputFormatter();
}

static void updateExecutable(CMakeRunConfiguration *rc, Utils::FancyLineEdit *fle)
{
    const Runnable runnable = rc->runnable();
    fle->setText(runnable.is<StandardRunnable>()
                 ? Utils::FileName::fromString(runnable.as<StandardRunnable>().executable).toUserOutput()
                 : QString());
}

// Configuration widget
CMakeRunConfigurationWidget::CMakeRunConfigurationWidget(CMakeRunConfiguration *cmakeRunConfiguration, QWidget *parent)
    : QWidget(parent)
{
    auto fl = new QFormLayout();
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto executableLabel = new QLabel(tr("Executable:"));
    auto executable = new Utils::FancyLineEdit;
    executable->setReadOnly(true);
    executable->setPlaceholderText(tr("<unknown>"));
    connect(cmakeRunConfiguration, &CMakeRunConfiguration::enabledChanged,
            this, std::bind(updateExecutable, cmakeRunConfiguration, executable));
    updateExecutable(cmakeRunConfiguration, executable);

    fl->addRow(executableLabel, executable);

    cmakeRunConfiguration->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, fl);
    cmakeRunConfiguration->extraAspect<WorkingDirectoryAspect>()->addToMainConfigurationWidget(this, fl);
    cmakeRunConfiguration->extraAspect<TerminalAspect>()->addToMainConfigurationWidget(this, fl);

    auto detailsContainer = new Utils::DetailsWidget(this);
    detailsContainer->setState(Utils::DetailsWidget::NoSummary);

    auto detailsWidget = new QWidget(detailsContainer);
    detailsContainer->setWidget(detailsWidget);
    detailsWidget->setLayout(fl);

    auto vbx = new QVBoxLayout(this);
    vbx->setMargin(0);
    vbx->addWidget(detailsContainer);
}

// Factory
CMakeRunConfigurationFactory::CMakeRunConfigurationFactory(QObject *parent) :
    IRunConfigurationFactory(parent)
{
    setObjectName("CMakeRunConfigurationFactory");
    registerRunConfiguration<CMakeRunConfiguration>(CMAKE_RC_PREFIX);
    setSupportedProjectType<CMakeProject>();
}

QList<QString> CMakeRunConfigurationFactory::availableBuildTargets(Target *parent, CreationMode) const
{
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());
    return project->buildTargetTitles(true);
}

bool CMakeRunConfigurationFactory::canCreateHelper(Target *parent, const QString &buildTarget) const
{
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());
    return project->hasBuildTarget(buildTarget);
}
