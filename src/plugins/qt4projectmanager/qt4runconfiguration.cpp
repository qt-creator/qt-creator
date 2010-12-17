/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qt4runconfiguration.h"

#include "makestep.h"
#include "profilereader.h"
#include "qt4nodes.h"
#include "qt4project.h"
#include "qt4target.h"
#include "qt4buildconfiguration.h"
#include "qt4projectmanagerconstants.h"
#include "qtoutputformatter.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/variablemanager.h>
#include <coreplugin/ifile.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/environmenteditmodel.h>
#include <projectexplorer/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/pathchooser.h>
#include <utils/detailswidget.h>
#include <utils/debuggerlanguagechooser.h>

#include <QtGui/QFormLayout>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
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
const char * const USE_TERMINAL_KEY("Qt4ProjectManager.Qt4RunConfiguration.UseTerminal");
const char * const USE_DYLD_IMAGE_SUFFIX_KEY("Qt4ProjectManager.Qt4RunConfiguration.UseDyldImageSuffix");
const char * const USER_ENVIRONMENT_CHANGES_KEY("Qt4ProjectManager.Qt4RunConfiguration.UserEnvironmentChanges");
const char * const BASE_ENVIRONMENT_BASE_KEY("Qt4ProjectManager.Qt4RunConfiguration.BaseEnvironmentBase");
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
    m_isUsingDyldImageSuffix(false),
    m_baseEnvironmentBase(Qt4RunConfiguration::BuildEnvironmentBase),
    m_parseSuccess(parent->qt4Project()->validParse(m_proFilePath))
{
    ctor();
}

Qt4RunConfiguration::Qt4RunConfiguration(Qt4Target *parent, Qt4RunConfiguration *source) :
    LocalApplicationRunConfiguration(parent, source),
    m_commandLineArguments(source->m_commandLineArguments),
    m_proFilePath(source->m_proFilePath),
    m_runMode(source->m_runMode),
    m_isUsingDyldImageSuffix(source->m_isUsingDyldImageSuffix),
    m_userWorkingDirectory(source->m_userWorkingDirectory),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges),
    m_baseEnvironmentBase(source->m_baseEnvironmentBase),
    m_parseSuccess(source->m_parseSuccess)
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

bool Qt4RunConfiguration::isEnabled(ProjectExplorer::BuildConfiguration * /* configuration */) const
{
    if (!m_parseSuccess)
        return false;
    return true;
}

void Qt4RunConfiguration::handleParseState(bool success)
{
    bool enabled = isEnabled();
    m_parseSuccess = success;
    if (enabled != isEnabled())
        emit isEnabledChanged(!enabled);
}

void Qt4RunConfiguration::proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *pro, bool success)
{
    if (m_proFilePath != pro->path())
        return;
    handleParseState(success);
    emit effectiveTargetInformationChanged();
}

void Qt4RunConfiguration::proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *pro)
{
    if (pro->path() != m_proFilePath)
        return;
    handleParseState(false);
}

void Qt4RunConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());

    connect(qt4Target(), SIGNAL(environmentChanged()),
            this, SIGNAL(baseEnvironmentChanged()));
    connect(qt4Target()->qt4Project(), SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)),
            this, SLOT(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)));

    connect(qt4Target()->qt4Project(), SIGNAL(proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode*)),
            this, SLOT(proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode*)));
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

    m_executableLineEdit = new QLineEdit(m_qt4RunConfiguration->executable(), this);
    m_executableLineEdit->setEnabled(false);
    toplayout->addRow(tr("Executable:"), m_executableLineEdit);

    QLabel *argumentsLabel = new QLabel(tr("Arguments:"), this);
    m_argumentsLineEdit = new QLineEdit(qt4RunConfiguration->rawCommandLineArguments(), this);
    argumentsLabel->setBuddy(m_argumentsLineEdit);
    toplayout->addRow(argumentsLabel, m_argumentsLineEdit);

    m_workingDirectoryEdit = new Utils::PathChooser(this);
    m_workingDirectoryEdit->setPath(m_qt4RunConfiguration->baseWorkingDirectory());
    m_workingDirectoryEdit->setBaseDirectory(m_qt4RunConfiguration->target()->project()->projectDirectory());
    m_workingDirectoryEdit->setExpectedKind(Utils::PathChooser::Directory);
    m_workingDirectoryEdit->setEnvironment(m_qt4RunConfiguration->environment());
    m_workingDirectoryEdit->setPromptDialogTitle(tr("Select Working Directory"));

    QToolButton *resetButton = new QToolButton(this);
    resetButton->setToolTip(tr("Reset to default"));
    resetButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_RESET)));

    QHBoxLayout *boxlayout = new QHBoxLayout();
    boxlayout->setMargin(0);
    boxlayout->addWidget(m_workingDirectoryEdit);
    boxlayout->addWidget(resetButton);
    toplayout->addRow(tr("Working directory:"), boxlayout);

    m_useTerminalCheck = new QCheckBox(tr("Run in terminal"), this);
    m_useTerminalCheck->setChecked(m_qt4RunConfiguration->runMode() == ProjectExplorer::LocalApplicationRunConfiguration::Console);
    toplayout->addRow(QString(), m_useTerminalCheck);

    QWidget *debuggerLabelWidget = new QWidget(this);
    QVBoxLayout *debuggerLabelLayout = new QVBoxLayout(debuggerLabelWidget);
    debuggerLabelLayout->setMargin(0);
    debuggerLabelLayout->setSpacing(0);
    debuggerLabelWidget->setLayout(debuggerLabelLayout);
    QLabel *debuggerLabel = new QLabel(tr("Debugger:"), this);
    debuggerLabelLayout->addWidget(debuggerLabel);
    debuggerLabelLayout->addStretch(10);

    m_debuggerLanguageChooser = new Utils::DebuggerLanguageChooser(this);
    toplayout->addRow(debuggerLabelWidget, m_debuggerLanguageChooser);

    m_debuggerLanguageChooser->setCppChecked(m_qt4RunConfiguration->useCppDebugger());
    m_debuggerLanguageChooser->setQmlChecked(m_qt4RunConfiguration->useQmlDebugger());
    m_debuggerLanguageChooser->setQmlDebugServerPort(m_qt4RunConfiguration->qmlDebugServerPort());

#ifdef Q_OS_MAC
    m_usingDyldImageSuffix = new QCheckBox(tr("Use debug version of frameworks (DYLD_IMAGE_SUFFIX=_debug)"), this);
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

    QWidget *baseEnvironmentWidget = new QWidget(this);
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

    setEnabled(m_qt4RunConfiguration->isEnabled());

    connect(m_workingDirectoryEdit, SIGNAL(changed(QString)),
            this, SLOT(workDirectoryEdited()));

    connect(resetButton, SIGNAL(clicked()),
            this, SLOT(workingDirectoryReseted()));

    connect(m_argumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(argumentsEdited(QString)));
    connect(m_useTerminalCheck, SIGNAL(toggled(bool)),
            this, SLOT(termToggled(bool)));

    connect(m_debuggerLanguageChooser, SIGNAL(cppLanguageToggled(bool)),
            this, SLOT(useCppDebuggerToggled(bool)));
    connect(m_debuggerLanguageChooser, SIGNAL(qmlLanguageToggled(bool)),
            this, SLOT(useQmlDebuggerToggled(bool)));
    connect(m_debuggerLanguageChooser, SIGNAL(qmlDebugServerPortChanged(uint)),
            this, SLOT(qmlDebugServerPortChanged(uint)));

    connect(m_environmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(userChangesEdited()));

    connect(qt4RunConfiguration, SIGNAL(baseWorkingDirectoryChanged(QString)),
            this, SLOT(workingDirectoryChanged(QString)));

    connect(qt4RunConfiguration, SIGNAL(commandLineArgumentsChanged(QString)),
            this, SLOT(commandLineArgumentsChanged(QString)));
    connect(qt4RunConfiguration, SIGNAL(runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode)),
            this, SLOT(runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode)));
    connect(qt4RunConfiguration, SIGNAL(usingDyldImageSuffixChanged(bool)),
            this, SLOT(usingDyldImageSuffixChanged(bool)));
    connect(qt4RunConfiguration, SIGNAL(effectiveTargetInformationChanged()),
            this, SLOT(effectiveTargetInformationChanged()), Qt::QueuedConnection);

    connect(qt4RunConfiguration, SIGNAL(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)),
            this, SLOT(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)));

    connect(qt4RunConfiguration, SIGNAL(baseEnvironmentChanged()),
            this, SLOT(baseEnvironmentChanged()));

    connect(qt4RunConfiguration, SIGNAL(isEnabledChanged(bool)),
            this, SLOT(runConfigurationEnabledChange(bool)));

    setEnabled(qt4RunConfiguration->isEnabled());
}

Qt4RunConfigurationWidget::~Qt4RunConfigurationWidget()
{
}

void Qt4RunConfigurationWidget::useCppDebuggerToggled(bool toggled)
{
    m_qt4RunConfiguration->setUseCppDebugger(toggled);
}

void Qt4RunConfigurationWidget::useQmlDebuggerToggled(bool toggled)
{
    m_qt4RunConfiguration->setUseQmlDebugger(toggled);
}

void Qt4RunConfigurationWidget::qmlDebugServerPortChanged(uint port)
{
    m_qt4RunConfiguration->setQmlDebugServerPort(port);
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

void Qt4RunConfigurationWidget::userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &userChanges)
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

void Qt4RunConfigurationWidget::runConfigurationEnabledChange(bool enabled)
{
    setEnabled(enabled);
}

void Qt4RunConfigurationWidget::workDirectoryEdited()
{
    if (m_ignoreChange)
        return;
    m_ignoreChange = true;
    m_qt4RunConfiguration->setBaseWorkingDirectory(m_workingDirectoryEdit->rawPath());
    m_ignoreChange = false;
}

void Qt4RunConfigurationWidget::workingDirectoryReseted()
{
    // This emits a signal connected to workingDirectoryChanged()
    // that sets the m_workingDirectoryEdit
    m_qt4RunConfiguration->setBaseWorkingDirectory("");
}

void Qt4RunConfigurationWidget::argumentsEdited(const QString &args)
{
    m_ignoreChange = true;
    m_qt4RunConfiguration->setCommandLineArguments(args);
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
        m_executableLineEdit->setText(QDir::toNativeSeparators(m_qt4RunConfiguration->executable()));
        m_ignoreChange = true;
        m_workingDirectoryEdit->setPath(QDir::toNativeSeparators(m_qt4RunConfiguration->baseWorkingDirectory()));
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
QWidget *Qt4RunConfiguration::createConfigurationWidget()
{
    return new Qt4RunConfigurationWidget(this, 0);
}

QVariantMap Qt4RunConfiguration::toMap() const
{
    const QDir projectDir = QDir(target()->project()->projectDirectory());
    QVariantMap map(LocalApplicationRunConfiguration::toMap());
    map.insert(QLatin1String(COMMAND_LINE_ARGUMENTS_KEY), m_commandLineArguments);
    map.insert(QLatin1String(PRO_FILE_KEY), projectDir.relativeFilePath(m_proFilePath));
    map.insert(QLatin1String(USE_TERMINAL_KEY), m_runMode == Console);
    map.insert(QLatin1String(USE_DYLD_IMAGE_SUFFIX_KEY), m_isUsingDyldImageSuffix);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), Utils::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.insert(QLatin1String(BASE_ENVIRONMENT_BASE_KEY), m_baseEnvironmentBase);
    map.insert(QLatin1String(USER_WORKING_DIRECTORY_KEY), m_userWorkingDirectory);
    return map;
}

bool Qt4RunConfiguration::fromMap(const QVariantMap &map)
{
    const QDir projectDir = QDir(target()->project()->projectDirectory());
    m_commandLineArguments = map.value(QLatin1String(COMMAND_LINE_ARGUMENTS_KEY)).toString();
    m_proFilePath = projectDir.filePath(map.value(QLatin1String(PRO_FILE_KEY)).toString());
    m_runMode = map.value(QLatin1String(USE_TERMINAL_KEY), false).toBool() ? Console : Gui;
    m_isUsingDyldImageSuffix = map.value(QLatin1String(USE_DYLD_IMAGE_SUFFIX_KEY), false).toBool();

    m_userWorkingDirectory = map.value(QLatin1String(USER_WORKING_DIRECTORY_KEY)).toString();

    m_userEnvironmentChanges = Utils::EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());
    m_baseEnvironmentBase = static_cast<BaseEnvironmentBase>(map.value(QLatin1String(BASE_ENVIRONMENT_BASE_KEY), static_cast<int>(Qt4RunConfiguration::BuildEnvironmentBase)).toInt());

    m_parseSuccess = qt4Target()->qt4Project()->validParse(m_proFilePath);

    return RunConfiguration::fromMap(map);
}

QString Qt4RunConfiguration::executable() const
{
    Qt4Project *pro = qt4Target()->qt4Project();
    TargetInformation ti = pro->rootProjectNode()->targetInformation(m_proFilePath);
    if (!ti.valid)
        return QString();
    return ti.executable;
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
    return QDir::cleanPath(environment().expandVariables(
                Utils::expandMacros(baseWorkingDirectory(), macroExpander())));
}

QString Qt4RunConfiguration::baseWorkingDirectory() const
{
    // if the user overrode us, then return his working directory
    if (!m_userWorkingDirectory.isEmpty())
        return m_userWorkingDirectory;

    // else what the pro file reader tells us
    Qt4Project *pro = qt4Target()->qt4Project();
    TargetInformation ti = pro->rootProjectNode()->targetInformation(m_proFilePath);
    if(!ti.valid)
        return QString();
    return ti.workingDir;
}

QString Qt4RunConfiguration::commandLineArguments() const
{
    return Utils::QtcProcess::expandMacros(m_commandLineArguments, macroExpander());
}

QString Qt4RunConfiguration::rawCommandLineArguments() const
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

Utils::Environment Qt4RunConfiguration::baseEnvironment() const
{
    Utils::Environment env;
    if (m_baseEnvironmentBase == Qt4RunConfiguration::CleanEnvironmentBase) {
        // Nothing
    } else  if (m_baseEnvironmentBase == Qt4RunConfiguration::SystemEnvironmentBase) {
        env = Utils::Environment::systemEnvironment();
    } else  if (m_baseEnvironmentBase == Qt4RunConfiguration::BuildEnvironmentBase) {
        env = target()->activeBuildConfiguration()->environment();
    }
    if (m_isUsingDyldImageSuffix) {
        env.set("DYLD_IMAGE_SUFFIX", "_debug");
    }

#ifdef Q_OS_WIN
    // On windows the user could be linking to a library found via a -L/some/dir switch
    // to find those libraries while actually running we explicitly prepend those
    // dirs to the path
    const Qt4ProFileNode *node = qt4Target()->qt4Project()->rootProjectNode()->findProFileFor(m_proFilePath);
    if (node)
        foreach(const QString dir, node->variableValue(LibDirectoriesVar))
            env.prependOrSetPath(dir);
#endif
    return env;
}

Utils::Environment Qt4RunConfiguration::environment() const
{
    Utils::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

QList<Utils::EnvironmentItem> Qt4RunConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void Qt4RunConfiguration::setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges != diff) {
        m_userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

void Qt4RunConfiguration::setBaseWorkingDirectory(const QString &wd)
{
    const QString &oldWorkingDirectory = workingDirectory();

    m_userWorkingDirectory = wd;

    const QString &newWorkingDirectory = workingDirectory();
    if (oldWorkingDirectory != newWorkingDirectory)
        emit baseWorkingDirectoryChanged(newWorkingDirectory);
}

void Qt4RunConfiguration::setCommandLineArguments(const QString &argumentsString)
{
    m_commandLineArguments = argumentsString;
    emit commandLineArgumentsChanged(argumentsString);
}

void Qt4RunConfiguration::setRunMode(RunMode runMode)
{
    m_runMode = runMode;
    emit runModeChanged(runMode);
}

QString Qt4RunConfiguration::proFilePath() const
{
    return m_proFilePath;
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

QString Qt4RunConfiguration::defaultDisplayName()
{
    QString defaultName;
    if (!m_proFilePath.isEmpty())
        defaultName = QFileInfo(m_proFilePath).completeBaseName();
    else
        defaultName = tr("Qt4 RunConfiguration");
    return defaultName;
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
ProjectExplorer::ToolChainType Qt4RunConfiguration::toolChainType() const
{
    Qt4BuildConfiguration *qt4bc = qt4Target()->activeBuildConfiguration();
    return qt4bc->toolChainType();
}

ProjectExplorer::OutputFormatter *Qt4RunConfiguration::createOutputFormatter() const
{
    return new QtOutputFormatter(qt4Target()->qt4Project());
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
    if (t->id() != QLatin1String(Constants::DESKTOP_TARGET_ID))
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
    if (parent->id() != QLatin1String(Constants::DESKTOP_TARGET_ID))
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
    if (t->id() != Constants::DESKTOP_TARGET_ID)
        return QStringList();
    return t->qt4Project()->applicationProFilePathes(QLatin1String(QT4_RC_PREFIX));
}

QString Qt4RunConfigurationFactory::displayNameForId(const QString &id) const
{
    return QFileInfo(pathFromId(id)).completeBaseName();
}
