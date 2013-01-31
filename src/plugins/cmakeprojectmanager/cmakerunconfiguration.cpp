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

#include "cmakerunconfiguration.h"

#include "cmakebuildconfiguration.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/helpmanager.h>
#include <qtsupport/debugginghelper.h>
#include <projectexplorer/environmentwidget.h>
#include <projectexplorer/target.h>

#include <utils/pathchooser.h>
#include <utils/detailswidget.h>
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

namespace {
const char CMAKE_RC_PREFIX[] = "CMakeProjectManager.CMakeRunConfiguration.";

const char USER_WORKING_DIRECTORY_KEY[] = "CMakeProjectManager.CMakeRunConfiguration.UserWorkingDirectory";
const char USE_TERMINAL_KEY[] = "CMakeProjectManager.CMakeRunConfiguration.UseTerminal";
const char TITLE_KEY[] = "CMakeProjectManager.CMakeRunConfiguation.Title";
const char ARGUMENTS_KEY[] = "CMakeProjectManager.CMakeRunConfiguration.Arguments";
const char USER_ENVIRONMENT_CHANGES_KEY[] = "CMakeProjectManager.CMakeRunConfiguration.UserEnvironmentChanges";
const char BASE_ENVIRONMENT_BASE_KEY[] = "CMakeProjectManager.BaseEnvironmentBase";

} // namespace

CMakeRunConfiguration::CMakeRunConfiguration(ProjectExplorer::Target *parent, Core::Id id, const QString &target,
                                             const QString &workingDirectory, const QString &title) :
    ProjectExplorer::LocalApplicationRunConfiguration(parent, id),
    m_runMode(Gui),
    m_buildTarget(target),
    m_workingDirectory(workingDirectory),
    m_title(title),
    m_baseEnvironmentBase(CMakeRunConfiguration::BuildEnvironmentBase),
    m_enabled(true)
{
    ctor();
}

CMakeRunConfiguration::CMakeRunConfiguration(ProjectExplorer::Target *parent, CMakeRunConfiguration *source) :
    ProjectExplorer::LocalApplicationRunConfiguration(parent, source),
    m_runMode(source->m_runMode),
    m_buildTarget(source->m_buildTarget),
    m_workingDirectory(source->m_workingDirectory),
    m_userWorkingDirectory(source->m_userWorkingDirectory),
    m_title(source->m_title),
    m_arguments(source->m_arguments),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges),
    m_baseEnvironmentBase(source->m_baseEnvironmentBase),
    m_enabled(source->m_enabled)
{
    ctor();
}

CMakeRunConfiguration::~CMakeRunConfiguration()
{
}

void CMakeRunConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());
    connect(target(), SIGNAL(environmentChanged()),
            this, SIGNAL(baseEnvironmentChanged()));
}

QString CMakeRunConfiguration::executable() const
{
    return m_buildTarget;
}

ProjectExplorer::LocalApplicationRunConfiguration::RunMode CMakeRunConfiguration::runMode() const
{
    return m_runMode;
}

void CMakeRunConfiguration::setRunMode(RunMode runMode)
{
    m_runMode = runMode;
}

QString CMakeRunConfiguration::workingDirectory() const
{
    return QDir::cleanPath(environment().expandVariables(
                Utils::expandMacros(baseWorkingDirectory(), macroExpander())));
}

QString CMakeRunConfiguration::baseWorkingDirectory() const
{
    if (!m_userWorkingDirectory.isEmpty())
        return m_userWorkingDirectory;
    return m_workingDirectory;
}

QString CMakeRunConfiguration::commandLineArguments() const
{
    return Utils::QtcProcess::expandMacros(m_arguments, macroExpander());
}

QString CMakeRunConfiguration::title() const
{
    return m_title;
}

void CMakeRunConfiguration::setExecutable(const QString &executable)
{
    m_buildTarget = executable;
}

void CMakeRunConfiguration::setBaseWorkingDirectory(const QString &wd)
{
    const QString &oldWorkingDirectory = workingDirectory();

    m_workingDirectory = wd;

    const QString &newWorkingDirectory = workingDirectory();
    if (oldWorkingDirectory != newWorkingDirectory)
        emit baseWorkingDirectoryChanged(newWorkingDirectory);
}

void CMakeRunConfiguration::setUserWorkingDirectory(const QString &wd)
{
    const QString & oldWorkingDirectory = workingDirectory();

    m_userWorkingDirectory = wd;

    const QString &newWorkingDirectory = workingDirectory();
    if (oldWorkingDirectory != newWorkingDirectory)
        emit baseWorkingDirectoryChanged(newWorkingDirectory);
}

QVariantMap CMakeRunConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::LocalApplicationRunConfiguration::toMap());

    map.insert(QLatin1String(USER_WORKING_DIRECTORY_KEY), m_userWorkingDirectory);
    map.insert(QLatin1String(USE_TERMINAL_KEY), m_runMode == Console);
    map.insert(QLatin1String(TITLE_KEY), m_title);
    map.insert(QLatin1String(ARGUMENTS_KEY), m_arguments);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), Utils::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.insert(QLatin1String(BASE_ENVIRONMENT_BASE_KEY), m_baseEnvironmentBase);

    return map;
}

bool CMakeRunConfiguration::fromMap(const QVariantMap &map)
{
    m_userWorkingDirectory = map.value(QLatin1String(USER_WORKING_DIRECTORY_KEY)).toString();
    m_runMode = map.value(QLatin1String(USE_TERMINAL_KEY)).toBool() ? Console : Gui;
    m_title = map.value(QLatin1String(TITLE_KEY)).toString();
    m_arguments = map.value(QLatin1String(ARGUMENTS_KEY)).toString();
    m_userEnvironmentChanges = Utils::EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());
    m_baseEnvironmentBase = static_cast<BaseEnvironmentBase>(map.value(QLatin1String(BASE_ENVIRONMENT_BASE_KEY), static_cast<int>(CMakeRunConfiguration::BuildEnvironmentBase)).toInt());

    return RunConfiguration::fromMap(map);
}

QString CMakeRunConfiguration::defaultDisplayName() const
{
    if (m_title.isEmpty())
        return tr("Run CMake kit");
    return m_title + (m_enabled ? QString() : tr(" (disabled)"));
}

QWidget *CMakeRunConfiguration::createConfigurationWidget()
{
    return new CMakeRunConfigurationWidget(this);
}

void CMakeRunConfiguration::setCommandLineArguments(const QString &newText)
{
    m_arguments = newText;
}

QString CMakeRunConfiguration::dumperLibrary() const
{
    Utils::FileName qmakePath = QtSupport::DebuggingHelperLibrary::findSystemQt(environment());
    QString qtInstallData = QtSupport::DebuggingHelperLibrary::qtInstallDataDir(qmakePath);
    QString dhl = QtSupport::DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData);
    return dhl;
}

QStringList CMakeRunConfiguration::dumperLibraryLocations() const
{
    Utils::FileName qmakePath = QtSupport::DebuggingHelperLibrary::findSystemQt(environment());
    QString qtInstallData = QtSupport::DebuggingHelperLibrary::qtInstallDataDir(qmakePath);
    return QtSupport::DebuggingHelperLibrary::debuggingHelperLibraryDirectories(qtInstallData);
}

Utils::Environment CMakeRunConfiguration::baseEnvironment() const
{
    Utils::Environment env;
    if (m_baseEnvironmentBase == CMakeRunConfiguration::CleanEnvironmentBase) {
        // Nothing
    } else  if (m_baseEnvironmentBase == CMakeRunConfiguration::SystemEnvironmentBase) {
        env = Utils::Environment::systemEnvironment();
    } else  if (m_baseEnvironmentBase == CMakeRunConfiguration::BuildEnvironmentBase) {
        env = activeBuildConfiguration()->environment();
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

Utils::Environment CMakeRunConfiguration::environment() const
{
    Utils::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

QList<Utils::EnvironmentItem> CMakeRunConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void CMakeRunConfiguration::setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges != diff) {
        m_userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

void CMakeRunConfiguration::setEnabled(bool b)
{
    if (m_enabled == b)
        return;
    m_enabled = b;
    emit enabledChanged();
    setDefaultDisplayName(defaultDisplayName());
}

bool CMakeRunConfiguration::isEnabled() const
{
    return m_enabled;
}

QString CMakeRunConfiguration::disabledReason() const
{
    if (!m_enabled)
        return tr("The executable is not built by the current build configuration");
    return QString();
}

// Configuration widget
CMakeRunConfigurationWidget::CMakeRunConfigurationWidget(CMakeRunConfiguration *cmakeRunConfiguration, QWidget *parent)
    : QWidget(parent), m_ignoreChange(false), m_cmakeRunConfiguration(cmakeRunConfiguration)
{
    QFormLayout *fl = new QFormLayout();
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    QLineEdit *argumentsLineEdit = new QLineEdit();
    argumentsLineEdit->setText(cmakeRunConfiguration->commandLineArguments());
    connect(argumentsLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(setArguments(QString)));
    fl->addRow(tr("Arguments:"), argumentsLineEdit);

    m_workingDirectoryEdit = new Utils::PathChooser();
    m_workingDirectoryEdit->setExpectedKind(Utils::PathChooser::Directory);
    m_workingDirectoryEdit->setBaseDirectory(m_cmakeRunConfiguration->target()->project()->projectDirectory());
    m_workingDirectoryEdit->setPath(m_cmakeRunConfiguration->baseWorkingDirectory());
    m_workingDirectoryEdit->setPromptDialogTitle(tr("Select Working Directory"));

    QToolButton *resetButton = new QToolButton();
    resetButton->setToolTip(tr("Reset to default"));
    resetButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_RESET)));

    QHBoxLayout *boxlayout = new QHBoxLayout();
    boxlayout->addWidget(m_workingDirectoryEdit);
    boxlayout->addWidget(resetButton);

    fl->addRow(tr("Working directory:"), boxlayout);

    QCheckBox *runInTerminal = new QCheckBox;
    fl->addRow(tr("Run in Terminal"), runInTerminal);

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

    connect(runInTerminal, SIGNAL(toggled(bool)),
            this, SLOT(runInTerminalToggled(bool)));

    connect(m_environmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(userChangesChanged()));

    connect(m_cmakeRunConfiguration, SIGNAL(baseWorkingDirectoryChanged(QString)),
            this, SLOT(workingDirectoryChanged(QString)));
    connect(m_cmakeRunConfiguration, SIGNAL(baseEnvironmentChanged()),
            this, SLOT(baseEnvironmentChanged()));
    connect(m_cmakeRunConfiguration, SIGNAL(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)),
            this, SLOT(userEnvironmentChangesChanged()));

    setEnabled(m_cmakeRunConfiguration->isEnabled());
}

void CMakeRunConfigurationWidget::setWorkingDirectory()
{
    if (m_ignoreChange)
        return;
    m_ignoreChange = true;
    m_cmakeRunConfiguration->setUserWorkingDirectory(m_workingDirectoryEdit->rawPath());
    m_ignoreChange = false;
}

void CMakeRunConfigurationWidget::workingDirectoryChanged(const QString &workingDirectory)
{
    if (!m_ignoreChange) {
        m_ignoreChange = true;
        m_workingDirectoryEdit->setPath(workingDirectory);
        m_ignoreChange = false;
    }
}

void CMakeRunConfigurationWidget::resetWorkingDirectory()
{
    // This emits a signal connected to workingDirectoryChanged()
    // that sets the m_workingDirectoryEdit
    m_cmakeRunConfiguration->setUserWorkingDirectory(QString());
}

void CMakeRunConfigurationWidget::runInTerminalToggled(bool toggled)
{
    m_cmakeRunConfiguration->setRunMode(toggled ? ProjectExplorer::LocalApplicationRunConfiguration::Console
                                                : ProjectExplorer::LocalApplicationRunConfiguration::Gui);
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
    m_cmakeRunConfiguration->setCommandLineArguments(args);
}

// Factory
CMakeRunConfigurationFactory::CMakeRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{ setObjectName(QLatin1String("CMakeRunConfigurationFactory")); }

CMakeRunConfigurationFactory::~CMakeRunConfigurationFactory()
{
}

// used to show the list of possible additons to a project, returns a list of ids
QList<Core::Id> CMakeRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());
    QList<Core::Id> allIds;
    foreach (const QString &buildTarget, project->buildTargetTitles())
        allIds << idFromBuildTarget(buildTarget);
    return allIds;
}

// used to translate the ids to names to display to the user
QString CMakeRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    return buildTargetFromId(id);
}

bool CMakeRunConfigurationFactory::canHandle(ProjectExplorer::Target *parent) const
{
    if (!parent->project()->supportsKit(parent->kit()))
        return false;
    return qobject_cast<CMakeProject *>(parent->project());
}

bool CMakeRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());
    return project->hasBuildTarget(buildTargetFromId(id));
}

ProjectExplorer::RunConfiguration *CMakeRunConfigurationFactory::create(ProjectExplorer::Target *parent,
                                                                        const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());
    const QString title(buildTargetFromId(id));
    const CMakeBuildTarget &ct = project->buildTargetForTitle(title);
    return new CMakeRunConfiguration(parent, id, ct.executable, ct.workingDirectory, ct.title);
}

bool CMakeRunConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const
{
    if (!canHandle(parent))
        return false;
    return source->id().toString().startsWith(QLatin1String(CMAKE_RC_PREFIX));
}

ProjectExplorer::RunConfiguration *CMakeRunConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration * source)
{
    if (!canClone(parent, source))
        return 0;
    CMakeRunConfiguration *crc(static_cast<CMakeRunConfiguration *>(source));
    return new CMakeRunConfiguration(parent, crc);
}

bool CMakeRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    if (!qobject_cast<CMakeProject *>(parent->project()))
        return false;
    QString id = QString::fromUtf8(ProjectExplorer::idFromMap(map).name());
    return id.startsWith(QLatin1String(CMAKE_RC_PREFIX));
}

ProjectExplorer::RunConfiguration *CMakeRunConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    CMakeRunConfiguration *rc(new CMakeRunConfiguration(parent, ProjectExplorer::idFromMap(map),
                                                        QString(), QString(), QString()));
    if (rc->fromMap(map))
        return rc;
    delete rc;
    return 0;
}

QString CMakeRunConfigurationFactory::buildTargetFromId(Core::Id id)
{
    QString idstr = QString::fromUtf8(id.name());
    if (!idstr.startsWith(QLatin1String(CMAKE_RC_PREFIX)))
        return QString();
    return idstr.mid(QString::fromLatin1(CMAKE_RC_PREFIX).length());
}

Core::Id CMakeRunConfigurationFactory::idFromBuildTarget(const QString &target)
{
    QString id = QString::fromLatin1(CMAKE_RC_PREFIX) + target;
    return Core::Id(id.toUtf8());
}
