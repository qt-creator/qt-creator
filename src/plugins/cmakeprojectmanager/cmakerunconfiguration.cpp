/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cmakerunconfiguration.h"

#include "cmakeproject.h"
#include "cmakebuildconfiguration.h"
#include "cmakeprojectconstants.h"

#include <projectexplorer/environment.h>
#include <projectexplorer/debugginghelper.h>
#include <utils/qtcassert.h>
#include <QtGui/QFormLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QToolButton>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

namespace {
const char * const CMAKE_RC_ID("CMakeProjectManager.CMakeRunConfiguration");
const char * const CMAKE_RC_PREFIX("CMakeProjectManager.CMakeRunConfiguration.");

const char * const TARGET_KEY("CMakeProjectManager.CMakeRunConfiguration.Target");
const char * const WORKING_DIRECTORY_KEY("CMakeProjectManager.CMakeRunConfiguration.WorkingDirectory");
const char * const USER_WORKING_DIRECTORY_KEY("CMakeProjectManager.CMakeRunConfiguration.UserWorkingDirectory");
const char * const USE_TERMINAL_KEY("CMakeProjectManager.CMakeRunConfiguration.UseTerminal");
const char * const TITLE_KEY("CMakeProjectManager.CMakeRunConfiguation.Title");
const char * const ARGUMENTS_KEY("CMakeProjectManager.CMakeRunConfiguration.Arguments");
const char * const USER_ENVIRONMENT_CHANGES_KEY("CMakeProjectManager.CMakeRunConfiguration.UserEnvironmentChanges");
const char * const BASE_ENVIRONMENT_BASE_KEY("CMakeProjectManager.BaseEnvironmentBase");

QString targetFromId(const QString &id)
{
    if (!id.startsWith(QLatin1String(CMAKE_RC_PREFIX)))
        return QString();
    return id.mid(QString::fromLatin1(CMAKE_RC_PREFIX).length());
}

QString idFromTarget(const QString &target)
{
    return QString::fromLatin1(CMAKE_RC_PREFIX) + target;
}

} // namespace

CMakeRunConfiguration::CMakeRunConfiguration(CMakeProject *pro, const QString &target, const QString &workingDirectory, const QString &title) :
    ProjectExplorer::LocalApplicationRunConfiguration(pro, QString::fromLatin1(CMAKE_RC_PREFIX)),
    m_runMode(Gui),
    m_target(target),
    m_workingDirectory(workingDirectory),
    m_title(title),
    m_baseEnvironmentBase(CMakeRunConfiguration::BuildEnvironmentBase)
{
    ctor();
}

CMakeRunConfiguration::CMakeRunConfiguration(CMakeProject *pro, CMakeRunConfiguration *source) :
    ProjectExplorer::LocalApplicationRunConfiguration(pro, source),
    m_runMode(source->m_runMode),
    m_target(source->m_target),
    m_workingDirectory(source->m_workingDirectory),
    m_userWorkingDirectory(source->m_userWorkingDirectory),
    m_title(source->m_title),
    m_arguments(source->m_arguments),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges),
    m_baseEnvironmentBase(source->m_baseEnvironmentBase)
{
    ctor();
}

CMakeRunConfiguration::~CMakeRunConfiguration()
{
}

void CMakeRunConfiguration::ctor()
{
    setDisplayName(m_title);

    connect(project(), SIGNAL(environmentChanged()),
            this, SIGNAL(baseEnvironmentChanged()));
}

CMakeProject *CMakeRunConfiguration::cmakeProject() const
{
    return static_cast<CMakeProject *>(project());
}

QString CMakeRunConfiguration::executable() const
{
    return m_target;
}

ProjectExplorer::LocalApplicationRunConfiguration::RunMode CMakeRunConfiguration::runMode() const
{
    return m_runMode;
}

QString CMakeRunConfiguration::workingDirectory() const
{
    if (!m_userWorkingDirectory.isEmpty())
        return m_userWorkingDirectory;
    return m_workingDirectory;
}

QStringList CMakeRunConfiguration::commandLineArguments() const
{
    return ProjectExplorer::Environment::parseCombinedArgString(m_arguments);
}

QString CMakeRunConfiguration::title() const
{
    return m_title;
}

void CMakeRunConfiguration::setExecutable(const QString &executable)
{
    m_target = executable;
}

void CMakeRunConfiguration::setWorkingDirectory(const QString &wd)
{
    const QString & oldWorkingDirectory = workingDirectory();

    m_workingDirectory = wd;

    const QString &newWorkingDirectory = workingDirectory();
    if (oldWorkingDirectory != newWorkingDirectory)
        emit workingDirectoryChanged(newWorkingDirectory);
}

void CMakeRunConfiguration::setUserWorkingDirectory(const QString &wd)
{
    const QString & oldWorkingDirectory = workingDirectory();

    m_userWorkingDirectory = wd;

    const QString &newWorkingDirectory = workingDirectory();
    if (oldWorkingDirectory != newWorkingDirectory)
        emit workingDirectoryChanged(newWorkingDirectory);
}

QVariantMap CMakeRunConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::LocalApplicationRunConfiguration::toMap());

    map.insert(QLatin1String(TARGET_KEY), m_target);
    map.insert(QLatin1String(WORKING_DIRECTORY_KEY), m_workingDirectory);
    map.insert(QLatin1String(USER_WORKING_DIRECTORY_KEY), m_userWorkingDirectory);
    map.insert(QLatin1String(USE_TERMINAL_KEY), m_runMode == Console);
    map.insert(QLatin1String(TITLE_KEY), m_title);
    map.insert(QLatin1String(ARGUMENTS_KEY), m_arguments);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), ProjectExplorer::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.insert(QLatin1String(BASE_ENVIRONMENT_BASE_KEY), m_baseEnvironmentBase);

    return map;
}

bool CMakeRunConfiguration::fromMap(const QVariantMap &map)
{
    m_target = map.value(QLatin1String(TARGET_KEY)).toString();
    m_workingDirectory = map.value(QLatin1String(WORKING_DIRECTORY_KEY)).toString();
    m_userWorkingDirectory = map.value(QLatin1String(USER_WORKING_DIRECTORY_KEY)).toString();
    m_runMode = map.value(QLatin1String(USE_TERMINAL_KEY)).toBool() ? Console : Gui;
    m_title = map.value(QLatin1String(TITLE_KEY)).toString();
    m_arguments = map.value(QLatin1String(ARGUMENTS_KEY)).toString();
    m_userEnvironmentChanges = ProjectExplorer::EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());
    m_baseEnvironmentBase = static_cast<BaseEnvironmentBase>(map.value(QLatin1String(BASE_ENVIRONMENT_BASE_KEY), static_cast<int>(CMakeRunConfiguration::BuildEnvironmentBase)).toInt());

    return RunConfiguration::fromMap(map);
}

QWidget *CMakeRunConfiguration::configurationWidget()
{
    return new CMakeRunConfigurationWidget(this);
}

void CMakeRunConfiguration::setArguments(const QString &newText)
{
    m_arguments = newText;
}

QString CMakeRunConfiguration::dumperLibrary() const
{
    QString qmakePath = ProjectExplorer::DebuggingHelperLibrary::findSystemQt(environment());
    QString qtInstallData = ProjectExplorer::DebuggingHelperLibrary::qtInstallDataDir(qmakePath);
    QString dhl = ProjectExplorer::DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData);
    return dhl;
}

QStringList CMakeRunConfiguration::dumperLibraryLocations() const
{
    QString qmakePath = ProjectExplorer::DebuggingHelperLibrary::findSystemQt(environment());
    QString qtInstallData = ProjectExplorer::DebuggingHelperLibrary::qtInstallDataDir(qmakePath);
    return ProjectExplorer::DebuggingHelperLibrary::debuggingHelperLibraryLocationsByInstallData(qtInstallData);
}

ProjectExplorer::Environment CMakeRunConfiguration::baseEnvironment() const
{
    ProjectExplorer::Environment env;
    if (m_baseEnvironmentBase == CMakeRunConfiguration::CleanEnvironmentBase) {
        // Nothing
    } else  if (m_baseEnvironmentBase == CMakeRunConfiguration::SystemEnvironmentBase) {
        env = ProjectExplorer::Environment::systemEnvironment();
    } else  if (m_baseEnvironmentBase == CMakeRunConfiguration::BuildEnvironmentBase) {
        env = project()->activeBuildConfiguration()->environment();
    }
    return env;
}

QString CMakeRunConfiguration::baseEnvironmentText() const
{
    if (m_baseEnvironmentBase == CMakeRunConfiguration::CleanEnvironmentBase) {
        return tr("Clean Environment");
    } else  if (m_baseEnvironmentBase == CMakeRunConfiguration::SystemEnvironmentBase) {
        return tr("System Environment");
    } else  if (m_baseEnvironmentBase == CMakeRunConfiguration::BuildEnvironmentBase) {
        return tr("Build Environment");
    }
    return QString();
}

void CMakeRunConfiguration::setBaseEnvironmentBase(BaseEnvironmentBase env)
{
    if (m_baseEnvironmentBase == env)
        return;
    m_baseEnvironmentBase = env;
    emit baseEnvironmentChanged();
}

CMakeRunConfiguration::BaseEnvironmentBase CMakeRunConfiguration::baseEnvironmentBase() const
{
    return m_baseEnvironmentBase;
}

ProjectExplorer::Environment CMakeRunConfiguration::environment() const
{
    ProjectExplorer::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

QList<ProjectExplorer::EnvironmentItem> CMakeRunConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void CMakeRunConfiguration::setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges != diff) {
        m_userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

ProjectExplorer::ToolChain::ToolChainType CMakeRunConfiguration::toolChainType() const
{
    CMakeBuildConfiguration *bc = cmakeProject()->activeCMakeBuildConfiguration();
    return bc->toolChainType();
}

// Configuration widget


CMakeRunConfigurationWidget::CMakeRunConfigurationWidget(CMakeRunConfiguration *cmakeRunConfiguration, QWidget *parent)
    : QWidget(parent), m_ignoreChange(false), m_cmakeRunConfiguration(cmakeRunConfiguration)
{

    QFormLayout *fl = new QFormLayout();
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    QLineEdit *argumentsLineEdit = new QLineEdit();
    argumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(cmakeRunConfiguration->commandLineArguments()));
    connect(argumentsLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(setArguments(QString)));
    fl->addRow(tr("Arguments:"), argumentsLineEdit);

    m_workingDirectoryEdit = new Utils::PathChooser();
    m_workingDirectoryEdit->setPath(m_cmakeRunConfiguration->workingDirectory());
    m_workingDirectoryEdit->setExpectedKind(Utils::PathChooser::Directory);
    m_workingDirectoryEdit->setPromptDialogTitle(tr("Select the working directory"));

    QToolButton *resetButton = new QToolButton();
    resetButton->setToolTip(tr("Reset to default"));
    resetButton->setIcon(QIcon(":/core/images/reset.png"));

    QHBoxLayout *boxlayout = new QHBoxLayout();
    boxlayout->addWidget(m_workingDirectoryEdit);
    boxlayout->addWidget(resetButton);

    fl->addRow(tr("Working Directory:"), boxlayout);

    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);

    QWidget *m_details = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(m_details);
    m_details->setLayout(fl);

    QVBoxLayout *vbx = new QVBoxLayout(this);
    vbx->setMargin(0);;
    vbx->addWidget(m_detailsContainer);

    QLabel *environmentLabel = new QLabel(this);
    environmentLabel->setText(tr("Run Environment"));
    QFont f = environmentLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() *1.2);
    environmentLabel->setFont(f);
    vbx->addWidget(environmentLabel);

    QWidget *baseEnvironmentWidget = new QWidget;
    QHBoxLayout *baseEnvironmentLayout = new QHBoxLayout(baseEnvironmentWidget);
    baseEnvironmentLayout->setMargin(0);
    QLabel *label = new QLabel(tr("Base environment for this runconfiguration:"), this);
    baseEnvironmentLayout->addWidget(label);
    m_baseEnvironmentComboBox = new QComboBox(this);
    m_baseEnvironmentComboBox->addItems(QStringList()
                                        << tr("Clean Environment")
                                        << tr("System Environment")
                                        << tr("Build Environment"));
    m_baseEnvironmentComboBox->setCurrentIndex(m_cmakeRunConfiguration->baseEnvironmentBase());
    connect(m_baseEnvironmentComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(baseEnvironmentComboBoxChanged(int)));
    baseEnvironmentLayout->addWidget(m_baseEnvironmentComboBox);
    baseEnvironmentLayout->addStretch(10);

    m_environmentWidget = new ProjectExplorer::EnvironmentWidget(this, baseEnvironmentWidget);
    m_environmentWidget->setBaseEnvironment(m_cmakeRunConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_cmakeRunConfiguration->baseEnvironmentText());
    m_environmentWidget->setUserChanges(m_cmakeRunConfiguration->userEnvironmentChanges());

    vbx->addWidget(m_environmentWidget);

    connect(m_workingDirectoryEdit, SIGNAL(changed(QString)),
            this, SLOT(setWorkingDirectory()));

    connect(resetButton, SIGNAL(clicked()),
            this, SLOT(resetWorkingDirectory()));

    connect(m_environmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(userChangesChanged()));

    connect(m_cmakeRunConfiguration, SIGNAL(workingDirectoryChanged(QString)),
            this, SLOT(workingDirectoryChanged(QString)));
    connect(m_cmakeRunConfiguration, SIGNAL(baseEnvironmentChanged()),
            this, SLOT(baseEnvironmentChanged()));
    connect(m_cmakeRunConfiguration, SIGNAL(userEnvironmentChangesChanged(QList<ProjectExplorer::EnvironmentItem>)),
            this, SLOT(userEnvironmentChangesChanged()));

}

void CMakeRunConfigurationWidget::setWorkingDirectory()
{
    if (m_ignoreChange)
        return;
    m_ignoreChange = true;
    m_cmakeRunConfiguration->setUserWorkingDirectory(m_workingDirectoryEdit->path());
    m_ignoreChange = false;
}

void CMakeRunConfigurationWidget::workingDirectoryChanged(const QString &workingDirectory)
{
    if (!m_ignoreChange)
        m_workingDirectoryEdit->setPath(workingDirectory);
}

void CMakeRunConfigurationWidget::resetWorkingDirectory()
{
    // This emits a signal connected to workingDirectoryChanged()
    // that sets the m_workingDirectoryEdit
    m_cmakeRunConfiguration->setUserWorkingDirectory("");
}

void CMakeRunConfigurationWidget::userChangesChanged()
{
    m_cmakeRunConfiguration->setUserEnvironmentChanges(m_environmentWidget->userChanges());
}

void CMakeRunConfigurationWidget::baseEnvironmentComboBoxChanged(int index)
{
    m_ignoreChange = true;
    m_cmakeRunConfiguration->setBaseEnvironmentBase(CMakeRunConfiguration::BaseEnvironmentBase(index));

    m_environmentWidget->setBaseEnvironment(m_cmakeRunConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_cmakeRunConfiguration->baseEnvironmentText());
    m_ignoreChange = false;
}

void CMakeRunConfigurationWidget::baseEnvironmentChanged()
{
    if (m_ignoreChange)
        return;

    m_baseEnvironmentComboBox->setCurrentIndex(m_cmakeRunConfiguration->baseEnvironmentBase());
    m_environmentWidget->setBaseEnvironment(m_cmakeRunConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_cmakeRunConfiguration->baseEnvironmentText());
}

void CMakeRunConfigurationWidget::userEnvironmentChangesChanged()
{
    m_environmentWidget->setUserChanges(m_cmakeRunConfiguration->userEnvironmentChanges());
}

void CMakeRunConfigurationWidget::setArguments(const QString &args)
{
    m_cmakeRunConfiguration->setArguments(args);
}

// Factory
CMakeRunConfigurationFactory::CMakeRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
}

CMakeRunConfigurationFactory::~CMakeRunConfigurationFactory()
{
}

// used to show the list of possible additons to a project, returns a list of ids
QStringList CMakeRunConfigurationFactory::availableCreationIds(ProjectExplorer::Project *parent) const
{
    CMakeProject *project(qobject_cast<CMakeProject *>(parent));
    if (!project)
        return QStringList();
    QStringList allIds;
    foreach (const QString &target, project->targets())
        allIds << idFromTarget(target);
    return allIds;
}

// used to translate the ids to names to display to the user
QString CMakeRunConfigurationFactory::displayNameForId(const QString &id) const
{
    return targetFromId(id);
}

bool CMakeRunConfigurationFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    CMakeProject *project(qobject_cast<CMakeProject *>(parent));
    if (!project)
        return false;
    return project->hasTarget(targetFromId(id));
}

ProjectExplorer::RunConfiguration *CMakeRunConfigurationFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    CMakeProject *project(static_cast<CMakeProject *>(parent));

    const QString title(targetFromId(id));
    const CMakeTarget &ct = project->targetForTitle(title);
    return new CMakeRunConfiguration(project, ct.executable, ct.workingDirectory, ct.title);
}

bool CMakeRunConfigurationFactory::canClone(ProjectExplorer::Project *parent, ProjectExplorer::RunConfiguration *source) const
{
    if (!qobject_cast<CMakeProject *>(parent))
        return false;
    return source->id() == QLatin1String(CMAKE_RC_ID);
}

ProjectExplorer::RunConfiguration *CMakeRunConfigurationFactory::clone(ProjectExplorer::Project *parent, ProjectExplorer::RunConfiguration * source)
{
    if (!canClone(parent, source))
        return 0;
    CMakeProject *project(static_cast<CMakeProject *>(parent));
    CMakeRunConfiguration *crc(static_cast<CMakeRunConfiguration *>(source));
    return new CMakeRunConfiguration(project, crc);
}

bool CMakeRunConfigurationFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    if (!qobject_cast<CMakeProject *>(parent))
        return false;
    QString id(ProjectExplorer::idFromMap(map));
    return id.startsWith(QLatin1String(CMAKE_RC_ID));
}

ProjectExplorer::RunConfiguration *CMakeRunConfigurationFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    CMakeProject *project(static_cast<CMakeProject *>(parent));
    CMakeRunConfiguration *rc(new CMakeRunConfiguration(project, QString(), QString(), QString()));
    if (rc->fromMap(map))
        return rc;
    delete rc;
    return 0;
}
