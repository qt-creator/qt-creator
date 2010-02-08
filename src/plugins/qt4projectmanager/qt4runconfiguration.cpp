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

#include "qt4runconfiguration.h"

#include "makestep.h"
#include "profilereader.h"
#include "qt4nodes.h"
#include "qt4project.h"
#include "qt4target.h"
#include "qt4buildconfiguration.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/variablemanager.h>
#include <coreplugin/ifile.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/environmenteditmodel.h>
#include <projectexplorer/persistentsettings.h>
#include <utils/qtcassert.h>

#include <QtGui/QFormLayout>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QToolButton>
#include <QtGui/QComboBox>

using namespace Qt4ProjectManager::Internal;
using namespace Qt4ProjectManager;
using ProjectExplorer::LocalApplicationRunConfiguration;
using ProjectExplorer::PersistentSettingsReader;
using ProjectExplorer::PersistentSettingsWriter;

namespace {
const char * const QT4_RC_ID("Qt4ProjectManager.Qt4RunConfiguration");
const char * const QT4_RC_PREFIX("Qt4ProjectManager.Qt4RunConfiguration.");

const char * const COMMAND_LINE_ARGUMENTS_KEY("Qt4ProjectManager.Qt4RunConfiguration.CommandLineArguments");
const char * const PRO_FILE_KEY("Qt4ProjectManager.Qt4RunConfiguration.ProFile");
const char * const USER_SET_NAME_KEY("Qt4ProjectManager.Qt4RunConfiguration.UserSetName");
const char * const USE_TERMINAL_KEY("Qt4ProjectManager.Qt4RunConfiguration.UseTerminal");
const char * const USE_DYLD_IMAGE_SUFFIX_KEY("Qt4ProjectManager.Qt4RunConfiguration.UseDyldImageSuffix");
const char * const USER_ENVIRONMENT_CHANGES_KEY("Qt4ProjectManager.Qt4RunConfiguration.UserEnvironmentChanges");
const char * const BASE_ENVIRONMENT_BASE_KEY("Qt4ProjectManager.Qt4RunConfiguration.BaseEnvironmentBase");
const char * const USER_SET_WORKING_DIRECTORY_KEY("Qt4ProjectManager.Qt4RunConfiguration.UserSetWorkingDirectory");
const char * const USER_WORKING_DIRECTORY_KEY("Qt4ProjectManager.Qt4RunConfiguration.UserWorkingDirectory");

QString pathFromId(const QString &id)
{
    if (!id.startsWith(QLatin1String(QT4_RC_PREFIX)))
        return QString();
    return id.mid(QString::fromLatin1(QT4_RC_PREFIX).size());
}

QString pathToId(const QString &path)
{
    return QString::fromLatin1(QT4_RC_PREFIX) + path;
}

} // namespace

//
// Qt4RunConfiguration
//

Qt4RunConfiguration::Qt4RunConfiguration(Qt4Target *parent, const QString &proFilePath) :
    LocalApplicationRunConfiguration(parent, QLatin1String(QT4_RC_ID)),
    m_proFilePath(proFilePath),
    m_runMode(Gui),
    m_userSetName(false),
    m_cachedTargetInformationValid(false),
    m_isUsingDyldImageSuffix(false),
    m_userSetWokingDirectory(false),
    m_baseEnvironmentBase(Qt4RunConfiguration::BuildEnvironmentBase)
{
    ctor();
}

Qt4RunConfiguration::Qt4RunConfiguration(Qt4Target *parent, Qt4RunConfiguration *source) :
    LocalApplicationRunConfiguration(parent, source),
    m_commandLineArguments(source->m_commandLineArguments),
    m_proFilePath(source->m_proFilePath),
    m_runMode(source->m_runMode),
    m_userSetName(source->m_userSetName),
    m_cachedTargetInformationValid(false),
    m_isUsingDyldImageSuffix(source->m_isUsingDyldImageSuffix),
    m_userSetWokingDirectory(source->m_userSetWokingDirectory),
    m_userWorkingDirectory(source->m_userWorkingDirectory),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges),
    m_baseEnvironmentBase(source->m_baseEnvironmentBase)
{
    ctor();
}

Qt4RunConfiguration::~Qt4RunConfiguration()
{
}

Qt4Target *Qt4RunConfiguration::qt4Target() const
{
    return static_cast<Qt4Target *>(target());
}

bool Qt4RunConfiguration::isEnabled(ProjectExplorer::BuildConfiguration *configuration) const
{
    Qt4BuildConfiguration *qt4bc = qobject_cast<Qt4BuildConfiguration *>(configuration);
    QTC_ASSERT(qt4bc, return false);

    using namespace ProjectExplorer;
    ToolChain::ToolChainType type = qt4bc->toolChainType();
    bool enabled;
    switch (type) {
    case ToolChain::MSVC:        case ToolChain::WINCE:
    case ToolChain::GCC:         case ToolChain::MinGW:
    case ToolChain::GCCE_GNUPOC: case ToolChain::RVCT_ARMV6_GNUPOC:
    case ToolChain::OTHER:       case ToolChain::UNKNOWN:
    case ToolChain::INVALID:
        enabled = true;
        break;
    case ToolChain::WINSCW:      case ToolChain::GCCE:
    case ToolChain::RVCT_ARMV5:  case ToolChain::RVCT_ARMV6:
    case ToolChain::GCC_MAEMO:
        enabled = false;
        break;
    }
    return enabled;
}

void Qt4RunConfiguration::proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro)
{
    if  (m_proFilePath == pro->path())
        invalidateCachedTargetInformation();
}

void Qt4RunConfiguration::ctor()
{
    setDefaultDisplayName();
    connect(qt4Target(), SIGNAL(targetInformationChanged()),
            this, SLOT(invalidateCachedTargetInformation()));

    connect(qt4Target(), SIGNAL(environmentChanged()),
            this, SIGNAL(baseEnvironmentChanged()));

    connect(qt4Target()->qt4Project(), SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*)),
            this, SLOT(proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode*)));
}

//////
/// Qt4RunConfigurationWidget
/////

Qt4RunConfigurationWidget::Qt4RunConfigurationWidget(Qt4RunConfiguration *qt4RunConfiguration, QWidget *parent)
    : QWidget(parent),
    m_qt4RunConfiguration(qt4RunConfiguration),
    m_ignoreChange(false),
    m_usingDyldImageSuffix(0),
    m_isShown(false)
{
    QVBoxLayout *vboxTopLayout = new QVBoxLayout(this);
    vboxTopLayout->setMargin(0);

    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vboxTopLayout->addWidget(m_detailsContainer);
    QWidget *detailsWidget = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(detailsWidget);
    QFormLayout *toplayout = new QFormLayout(detailsWidget);
    toplayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    toplayout->setMargin(0);

    QLabel *nameLabel = new QLabel(tr("Name:"));
    m_nameLineEdit = new QLineEdit(m_qt4RunConfiguration->displayName());
    nameLabel->setBuddy(m_nameLineEdit);
    toplayout->addRow(nameLabel, m_nameLineEdit);

    m_executableLabel = new QLabel(m_qt4RunConfiguration->executable());
    toplayout->addRow(tr("Executable:"), m_executableLabel);

    QLabel *argumentsLabel = new QLabel(tr("Arguments:"));
    m_argumentsLineEdit = new QLineEdit(ProjectExplorer::Environment::joinArgumentList(qt4RunConfiguration->commandLineArguments()));
    argumentsLabel->setBuddy(m_argumentsLineEdit);
    toplayout->addRow(argumentsLabel, m_argumentsLineEdit);

    m_workingDirectoryEdit = new Utils::PathChooser();
    m_workingDirectoryEdit->setPath(m_qt4RunConfiguration->workingDirectory());
    m_workingDirectoryEdit->setExpectedKind(Utils::PathChooser::Directory);
    m_workingDirectoryEdit->setPromptDialogTitle(tr("Select the working directory"));

    QToolButton *resetButton = new QToolButton();
    resetButton->setToolTip(tr("Reset to default"));
    resetButton->setIcon(QIcon(":/core/images/reset.png"));

    QHBoxLayout *boxlayout = new QHBoxLayout();
    boxlayout->setMargin(0);
    boxlayout->addWidget(m_workingDirectoryEdit);
    boxlayout->addWidget(resetButton);
    toplayout->addRow(tr("Working Directory:"), boxlayout);

    m_useTerminalCheck = new QCheckBox(tr("Run in Terminal"));
    m_useTerminalCheck->setChecked(m_qt4RunConfiguration->runMode() == ProjectExplorer::LocalApplicationRunConfiguration::Console);
    toplayout->addRow(QString(), m_useTerminalCheck);

#ifdef Q_OS_MAC
    m_usingDyldImageSuffix = new QCheckBox(tr("Use debug version of frameworks (DYLD_IMAGE_SUFFIX=_debug)"));
    m_usingDyldImageSuffix->setChecked(m_qt4RunConfiguration->isUsingDyldImageSuffix());
    toplayout->addRow(QString(), m_usingDyldImageSuffix);
    connect(m_usingDyldImageSuffix, SIGNAL(toggled(bool)),
            this, SLOT(usingDyldImageSuffixToggled(bool)));
#endif

    QLabel *environmentLabel = new QLabel(this);
    environmentLabel->setText(tr("Run Environment"));
    QFont f = environmentLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() *1.2);
    environmentLabel->setFont(f);
    vboxTopLayout->addWidget(environmentLabel);

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
    m_baseEnvironmentComboBox->setCurrentIndex(qt4RunConfiguration->baseEnvironmentBase());
    connect(m_baseEnvironmentComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(baseEnvironmentSelected(int)));
    baseEnvironmentLayout->addWidget(m_baseEnvironmentComboBox);
    baseEnvironmentLayout->addStretch(10);

    m_environmentWidget = new ProjectExplorer::EnvironmentWidget(this, baseEnvironmentWidget);
    m_environmentWidget->setBaseEnvironment(m_qt4RunConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_qt4RunConfiguration->baseEnvironmentText());
    m_environmentWidget->setUserChanges(m_qt4RunConfiguration->userEnvironmentChanges());
    m_environmentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    vboxTopLayout->addWidget(m_environmentWidget);

    connect(m_workingDirectoryEdit, SIGNAL(changed(QString)),
            this, SLOT(workDirectoryEdited()));

    connect(resetButton, SIGNAL(clicked()),
            this, SLOT(workingDirectoryReseted()));

    connect(m_argumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(argumentsEdited(QString)));
    connect(m_nameLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(displayNameEdited(QString)));
    connect(m_useTerminalCheck, SIGNAL(toggled(bool)),
            this, SLOT(termToggled(bool)));

    connect(m_environmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(userChangesEdited()));

    connect(qt4RunConfiguration, SIGNAL(workingDirectoryChanged(QString)),
            this, SLOT(workingDirectoryChanged(QString)));

    connect(qt4RunConfiguration, SIGNAL(commandLineArgumentsChanged(QString)),
            this, SLOT(commandLineArgumentsChanged(QString)));
    connect(qt4RunConfiguration, SIGNAL(displayNameChanged(QString)),
            this, SLOT(displayNameChanged(QString)));
    connect(qt4RunConfiguration, SIGNAL(runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode)),
            this, SLOT(runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode)));
    connect(qt4RunConfiguration, SIGNAL(usingDyldImageSuffixChanged(bool)),
            this, SLOT(usingDyldImageSuffixChanged(bool)));
    connect(qt4RunConfiguration, SIGNAL(effectiveTargetInformationChanged()),
            this, SLOT(effectiveTargetInformationChanged()), Qt::QueuedConnection);

    connect(qt4RunConfiguration, SIGNAL(userEnvironmentChangesChanged(QList<ProjectExplorer::EnvironmentItem>)),
            this, SLOT(userEnvironmentChangesChanged(QList<ProjectExplorer::EnvironmentItem>)));

    connect(qt4RunConfiguration, SIGNAL(baseEnvironmentChanged()),
            this, SLOT(baseEnvironmentChanged()));
}

void Qt4RunConfigurationWidget::baseEnvironmentSelected(int index)
{
    m_ignoreChange = true;
    m_qt4RunConfiguration->setBaseEnvironmentBase(Qt4RunConfiguration::BaseEnvironmentBase(index));

    m_environmentWidget->setBaseEnvironment(m_qt4RunConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_qt4RunConfiguration->baseEnvironmentText());
    m_ignoreChange = false;
}

void Qt4RunConfigurationWidget::baseEnvironmentChanged()
{
    if (m_ignoreChange)
        return;

    m_baseEnvironmentComboBox->setCurrentIndex(m_qt4RunConfiguration->baseEnvironmentBase());
    m_environmentWidget->setBaseEnvironment(m_qt4RunConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_qt4RunConfiguration->baseEnvironmentText());
}

void Qt4RunConfigurationWidget::userEnvironmentChangesChanged(const QList<ProjectExplorer::EnvironmentItem> &userChanges)
{
    if (m_ignoreChange)
        return;
    m_environmentWidget->setUserChanges(userChanges);
}

void Qt4RunConfigurationWidget::userChangesEdited()
{
    m_ignoreChange = true;
    m_qt4RunConfiguration->setUserEnvironmentChanges(m_environmentWidget->userChanges());
    m_ignoreChange = false;
}

void Qt4RunConfigurationWidget::workDirectoryEdited()
{
    if (m_ignoreChange)
        return;
    m_ignoreChange = true;
    m_qt4RunConfiguration->setWorkingDirectory(m_workingDirectoryEdit->path());
    m_ignoreChange = false;
}

void Qt4RunConfigurationWidget::workingDirectoryReseted()
{
    // This emits a signal connected to workingDirectoryChanged()
    // that sets the m_workingDirectoryEdit
    m_qt4RunConfiguration->setWorkingDirectory("");
}

void Qt4RunConfigurationWidget::argumentsEdited(const QString &args)
{
    m_ignoreChange = true;
    m_qt4RunConfiguration->setArguments(args);
    m_ignoreChange = false;
}

void Qt4RunConfigurationWidget::displayNameEdited(const QString &name)
{
    m_ignoreChange = true;
    m_qt4RunConfiguration->setUserName(name);
    m_ignoreChange = false;
}

void Qt4RunConfigurationWidget::termToggled(bool on)
{
    m_ignoreChange = true;
    m_qt4RunConfiguration->setRunMode(on ? LocalApplicationRunConfiguration::Console
                                         : LocalApplicationRunConfiguration::Gui);
    m_ignoreChange = false;
}

void Qt4RunConfigurationWidget::usingDyldImageSuffixToggled(bool state)
{
    m_ignoreChange = true;
    m_qt4RunConfiguration->setUsingDyldImageSuffix(state);
    m_ignoreChange = false;
}

void Qt4RunConfigurationWidget::workingDirectoryChanged(const QString &workingDirectory)
{
    if (!m_ignoreChange)
        m_workingDirectoryEdit->setPath(workingDirectory);
}

void Qt4RunConfigurationWidget::commandLineArgumentsChanged(const QString &args)
{
    if (m_ignoreChange)
        return;
    m_argumentsLineEdit->setText(args);
}

void Qt4RunConfigurationWidget::displayNameChanged(const QString &name)
{
    if (!m_ignoreChange)
        m_nameLineEdit->setText(name);
}

void Qt4RunConfigurationWidget::runModeChanged(LocalApplicationRunConfiguration::RunMode runMode)
{
    if (!m_ignoreChange)
        m_useTerminalCheck->setChecked(runMode == LocalApplicationRunConfiguration::Console);
}

void Qt4RunConfigurationWidget::usingDyldImageSuffixChanged(bool state)
{
    if (!m_ignoreChange && m_usingDyldImageSuffix)
        m_usingDyldImageSuffix->setChecked(state);
}

void Qt4RunConfigurationWidget::effectiveTargetInformationChanged()
{
    if (m_isShown) {
        m_executableLabel->setText(QDir::toNativeSeparators(m_qt4RunConfiguration->executable()));
        m_ignoreChange = true;
        m_workingDirectoryEdit->setPath(QDir::toNativeSeparators(m_qt4RunConfiguration->workingDirectory()));
        m_ignoreChange = false;
    }
}

void Qt4RunConfigurationWidget::showEvent(QShowEvent *event)
{
    m_isShown = true;
    effectiveTargetInformationChanged();
    QWidget::showEvent(event);
}

void Qt4RunConfigurationWidget::hideEvent(QHideEvent *event)
{
    m_isShown = false;
    QWidget::hideEvent(event);
}

////// TODO c&p above
QWidget *Qt4RunConfiguration::configurationWidget()
{
    return new Qt4RunConfigurationWidget(this, 0);
}

QVariantMap Qt4RunConfiguration::toMap() const
{
    const QDir projectDir = QFileInfo(target()->project()->file()->fileName()).absoluteDir();
    QVariantMap map(LocalApplicationRunConfiguration::toMap());
    map.insert(QLatin1String(COMMAND_LINE_ARGUMENTS_KEY), m_commandLineArguments);
    map.insert(QLatin1String(PRO_FILE_KEY), projectDir.relativeFilePath(m_proFilePath));
    map.insert(QLatin1String(USER_SET_NAME_KEY), m_userSetName);
    map.insert(QLatin1String(USE_TERMINAL_KEY), m_runMode == Console);
    map.insert(QLatin1String(USE_DYLD_IMAGE_SUFFIX_KEY), m_isUsingDyldImageSuffix);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), ProjectExplorer::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.insert(QLatin1String(BASE_ENVIRONMENT_BASE_KEY), m_baseEnvironmentBase);
    map.insert(QLatin1String(USER_SET_WORKING_DIRECTORY_KEY), m_userSetWokingDirectory);
    map.insert(QLatin1String(USER_WORKING_DIRECTORY_KEY), m_userWorkingDirectory);
    return map;
}

bool Qt4RunConfiguration::fromMap(const QVariantMap &map)
{
    const QDir projectDir = QFileInfo(target()->project()->file()->fileName()).absoluteDir();
    m_commandLineArguments = map.value(QLatin1String(COMMAND_LINE_ARGUMENTS_KEY)).toStringList();
    m_proFilePath = projectDir.filePath(map.value(QLatin1String(PRO_FILE_KEY)).toString());
    m_userSetName = map.value(QLatin1String(USER_SET_NAME_KEY), false).toBool();
    m_runMode = map.value(QLatin1String(USE_TERMINAL_KEY), false).toBool() ? Console : Gui;
    m_isUsingDyldImageSuffix = map.value(QLatin1String(USE_DYLD_IMAGE_SUFFIX_KEY), false).toBool();

    m_userSetWokingDirectory = map.value(QLatin1String(USER_SET_WORKING_DIRECTORY_KEY), false).toBool();
    m_userWorkingDirectory = map.value(QLatin1String(USER_WORKING_DIRECTORY_KEY)).toString();

    if (!m_proFilePath.isEmpty()) {
        m_cachedTargetInformationValid = false;
        if (!m_userSetName)
            setDefaultDisplayName();
    }
    m_userEnvironmentChanges = ProjectExplorer::EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());
    m_baseEnvironmentBase = static_cast<BaseEnvironmentBase>(map.value(QLatin1String(BASE_ENVIRONMENT_BASE_KEY), static_cast<int>(Qt4RunConfiguration::BuildEnvironmentBase)).toInt());

    return RunConfiguration::fromMap(map);
}

QString Qt4RunConfiguration::executable() const
{
    const_cast<Qt4RunConfiguration *>(this)->updateTarget();
    return m_executable;
}

LocalApplicationRunConfiguration::RunMode Qt4RunConfiguration::runMode() const
{
    return m_runMode;
}

bool Qt4RunConfiguration::isUsingDyldImageSuffix() const
{
    return m_isUsingDyldImageSuffix;
}

void Qt4RunConfiguration::setUsingDyldImageSuffix(bool state)
{
    m_isUsingDyldImageSuffix = state;
    emit usingDyldImageSuffixChanged(state);
}

QString Qt4RunConfiguration::workingDirectory() const
{
    // if the user overrode us, then return his working directory
    if (m_userSetWokingDirectory)
        return m_userWorkingDirectory;

    // else what the pro file reader tells us
    const_cast<Qt4RunConfiguration *>(this)->updateTarget();
    return m_workingDir;
}

QStringList Qt4RunConfiguration::commandLineArguments() const
{
    return m_commandLineArguments;
}

QString Qt4RunConfiguration::baseEnvironmentText() const
{
    if (m_baseEnvironmentBase == Qt4RunConfiguration::CleanEnvironmentBase)
        return tr("Clean Environment");
    else  if (m_baseEnvironmentBase == Qt4RunConfiguration::SystemEnvironmentBase)
        return tr("System Environment");
    else  if (m_baseEnvironmentBase == Qt4RunConfiguration::BuildEnvironmentBase)
        return tr("Build Environment");
    return QString();
}

ProjectExplorer::Environment Qt4RunConfiguration::baseEnvironment() const
{
    ProjectExplorer::Environment env;
    if (m_baseEnvironmentBase == Qt4RunConfiguration::CleanEnvironmentBase) {
        // Nothing
    } else  if (m_baseEnvironmentBase == Qt4RunConfiguration::SystemEnvironmentBase) {
        env = ProjectExplorer::Environment::systemEnvironment();
    } else  if (m_baseEnvironmentBase == Qt4RunConfiguration::BuildEnvironmentBase) {
        env = target()->activeBuildConfiguration()->environment();
    }
    if (m_isUsingDyldImageSuffix) {
        env.set("DYLD_IMAGE_SUFFIX", "_debug");
    }
    return env;
}

ProjectExplorer::Environment Qt4RunConfiguration::environment() const
{
    ProjectExplorer::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

QList<ProjectExplorer::EnvironmentItem> Qt4RunConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void Qt4RunConfiguration::setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges != diff) {
        m_userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

void Qt4RunConfiguration::setWorkingDirectory(const QString &wd)
{
    if (wd.isEmpty()) {
        m_userSetWokingDirectory = false;
        m_userWorkingDirectory.clear();
        emit workingDirectoryChanged(workingDirectory());
    } else {
        m_userSetWokingDirectory = true;
        m_userWorkingDirectory = wd;
        emit workingDirectoryChanged(m_userWorkingDirectory);
    }
}

void Qt4RunConfiguration::setArguments(const QString &argumentsString)
{
    m_commandLineArguments = ProjectExplorer::Environment::parseCombinedArgString(argumentsString);
    emit commandLineArgumentsChanged(argumentsString);
}

void Qt4RunConfiguration::setRunMode(RunMode runMode)
{
    m_runMode = runMode;
    emit runModeChanged(runMode);
}

void Qt4RunConfiguration::setUserName(const QString &name)
{
    if (name.isEmpty()) {
        m_userSetName = false;
        setDefaultDisplayName();
    } else {
        m_userSetName = true;
        setDisplayName(name);
    }
}

QString Qt4RunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

void Qt4RunConfiguration::updateTarget()
{
    if (m_cachedTargetInformationValid)
        return;
    Qt4TargetInformation info = qt4Target()->targetInformation(qt4Target()->activeBuildConfiguration(),
                                                               m_proFilePath);
    if (info.error != Qt4TargetInformation::NoError) {
        if (info.error == Qt4TargetInformation::ProParserError) {
            Core::ICore::instance()->messageManager()->printToOutputPane(
                    tr("Could not parse %1. The Qt4 run configuration %2 can not be started.")
                    .arg(m_proFilePath).arg(displayName()));
        }
        m_workingDir.clear();
        m_executable.clear();
        m_cachedTargetInformationValid = true;
        emit effectiveTargetInformationChanged();
        return;
    }
    m_workingDir = info.workingDir;
    m_executable = info.executable;

    m_cachedTargetInformationValid = true;

    emit effectiveTargetInformationChanged();
}

void Qt4RunConfiguration::invalidateCachedTargetInformation()
{
    m_cachedTargetInformationValid = false;
    emit effectiveTargetInformationChanged();
}

QString Qt4RunConfiguration::dumperLibrary() const
{
    QtVersion *version = qt4Target()->activeBuildConfiguration()->qtVersion();
    if (version)
        return version->debuggingHelperLibrary();
    return QString();
}

QStringList Qt4RunConfiguration::dumperLibraryLocations() const
{
    QtVersion *version = qt4Target()->activeBuildConfiguration()->qtVersion();
    if (version)
        return version->debuggingHelperLibraryLocations();
    return QStringList();
}

void Qt4RunConfiguration::setDefaultDisplayName()
{
    if (m_userSetName)
        return;
    if (!m_proFilePath.isEmpty())
        setDisplayName(QFileInfo(m_proFilePath).completeBaseName());
    else
        setDisplayName(tr("Qt4 RunConfiguration"));
}

void Qt4RunConfiguration::setBaseEnvironmentBase(BaseEnvironmentBase env)
{
    if (m_baseEnvironmentBase == env)
        return;
    m_baseEnvironmentBase = env;
    emit baseEnvironmentChanged();
}

Qt4RunConfiguration::BaseEnvironmentBase Qt4RunConfiguration::baseEnvironmentBase() const
{
    return m_baseEnvironmentBase;
}
ProjectExplorer::ToolChain::ToolChainType Qt4RunConfiguration::toolChainType() const
{
    Qt4BuildConfiguration *qt4bc = qt4Target()->activeBuildConfiguration();
    return qt4bc->toolChainType();
}

///
/// Qt4RunConfigurationFactory
/// This class is used to restore run settings (saved in .user files)
///

Qt4RunConfigurationFactory::Qt4RunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
}

Qt4RunConfigurationFactory::~Qt4RunConfigurationFactory()
{
}

bool Qt4RunConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const QString &id) const
{
    Qt4Target *t = qobject_cast<Qt4Target *>(parent);
    if (!t)
        return false;
    if (t->id() != QLatin1String(DESKTOP_TARGET_ID))
        return false;
    return t->qt4Project()->hasApplicationProFile(pathFromId(id));
}

ProjectExplorer::RunConfiguration *Qt4RunConfigurationFactory::create(ProjectExplorer::Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    Qt4Target *t(static_cast<Qt4Target *>(parent));
    return new Qt4RunConfiguration(t, pathFromId(id));
}

bool Qt4RunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    if (!qobject_cast<Qt4Target *>(parent))
        return false;
    if (parent->id() != QLatin1String(DESKTOP_TARGET_ID))
        return false;
    QString id(ProjectExplorer::idFromMap(map));
    return id.startsWith(QLatin1String(QT4_RC_ID));
}

ProjectExplorer::RunConfiguration *Qt4RunConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Qt4Target *t(static_cast<Qt4Target *>(parent));
    Qt4RunConfiguration *rc(new Qt4RunConfiguration(t, QString()));
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

bool Qt4RunConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration *Qt4RunConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    Qt4Target *t(static_cast<Qt4Target *>(parent));
    Qt4RunConfiguration *old(static_cast<Qt4RunConfiguration *>(source));
    return new Qt4RunConfiguration(t, old);
}

QStringList Qt4RunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    Qt4Target *t(qobject_cast<Qt4Target *>(parent));
    if (!t)
        return QStringList();
    if (t->id() != DESKTOP_TARGET_ID)
        return QStringList();
    return t->qt4Project()->applicationProFilePathes(QLatin1String(QT4_RC_PREFIX));
}

QString Qt4RunConfigurationFactory::displayNameForId(const QString &id) const
{
    return QFileInfo(pathFromId(id)).completeBaseName();
}
