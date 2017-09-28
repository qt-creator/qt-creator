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
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <utils/detailswidget.h>
#include <utils/fancylineedit.h>
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
    addExtraAspect(new LocalEnvironmentAspect(this, LocalEnvironmentAspect::BaseEnvironmentModifier()));
    addExtraAspect(new ArgumentsAspect(this, "CMakeProjectManager.CMakeRunConfiguration.Arguments"));
    addExtraAspect(new TerminalAspect(this, "CMakeProjectManager.CMakeRunConfiguration.UseTerminal"));
    addExtraAspect(new WorkingDirectoryAspect(this, "CMakeProjectManager.CMakeRunConfiguration.UserWorkingDirectory"));
}

void CMakeRunConfiguration::initialize(Core::Id id, const QString &target,
                                  const Utils::FileName &workingDirectory, const QString &title)
{
    RunConfiguration::initialize(id);
    m_buildSystemTarget = target;
    m_executable = target;
    m_title = title;

    extraAspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(workingDirectory);

    setDefaultDisplayName(defaultDisplayName());
}

void CMakeRunConfiguration::copyFrom(const CMakeRunConfiguration *source)
{
    RunConfiguration::copyFrom(source);

    m_buildSystemTarget = source->m_buildSystemTarget;
    m_executable = source->m_executable;
    m_title = source->m_title;

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
{ setObjectName(QLatin1String("CMakeRunConfigurationFactory")); }

// used to show the list of possible additons to a project, returns a list of ids
QList<Core::Id> CMakeRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode)
    if (!canHandle(parent))
        return QList<Core::Id>();
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());
    QList<Core::Id> allIds;
    foreach (const QString &buildTarget, project->buildTargetTitles(true))
        allIds << idFromBuildTarget(buildTarget);
    return allIds;
}

// used to translate the ids to names to display to the user
QString CMakeRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    return buildTargetFromId(id);
}

bool CMakeRunConfigurationFactory::canHandle(Target *parent) const
{
    if (!parent->project()->supportsKit(parent->kit()))
        return false;
    return qobject_cast<CMakeProject *>(parent->project());
}

bool CMakeRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());
    return project->hasBuildTarget(buildTargetFromId(id));
}

RunConfiguration *CMakeRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());
    const QString title(buildTargetFromId(id));
    const CMakeBuildTarget &ct = project->buildTargetForTitle(title);
    return createHelper<CMakeRunConfiguration>(parent, id, title, ct.workingDirectory, ct.title);
}

bool CMakeRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    if (!canHandle(parent))
        return false;
    return source->id().name().startsWith(CMAKE_RC_PREFIX);
}

RunConfiguration *CMakeRunConfigurationFactory::clone(Target *parent, RunConfiguration * source)
{
    if (!canClone(parent, source))
        return 0;
    return cloneHelper<CMakeRunConfiguration>(parent, source);
}

bool CMakeRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!qobject_cast<CMakeProject *>(parent->project()))
        return false;
    return idFromMap(map).name().startsWith(CMAKE_RC_PREFIX);
}

RunConfiguration *CMakeRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    const Core::Id id = idFromMap(map);
    return createHelper<CMakeRunConfiguration>(parent, id, buildTargetFromId(id), Utils::FileName(), QString());
}

QString CMakeRunConfigurationFactory::buildTargetFromId(Core::Id id)
{
    return id.suffixAfter(CMAKE_RC_PREFIX);
}

Core::Id CMakeRunConfigurationFactory::idFromBuildTarget(const QString &target)
{
    return Core::Id(CMAKE_RC_PREFIX).withSuffix(target);
}
