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

#include "qt4runconfiguration.h"

#include "../qt4nodes.h"
#include "../qt4project.h"
#include "../qt4buildconfiguration.h"

#include <coreplugin/coreconstants.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/target.h>
#include <utils/qtcprocess.h>
#include <utils/pathchooser.h>
#include <utils/detailswidget.h>
#include <utils/stringutils.h>
#include <utils/persistentsettings.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/hostosinfo.h>

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QToolButton>
#include <QComboBox>
#include <QFileInfo>
#include <QDir>

using namespace Qt4ProjectManager::Internal;
using namespace Qt4ProjectManager;
using ProjectExplorer::LocalApplicationRunConfiguration;
using Utils::PersistentSettingsReader;
using Utils::PersistentSettingsWriter;

namespace {

const char QT4_RC_PREFIX[] = "Qt4ProjectManager.Qt4RunConfiguration:";
const char COMMAND_LINE_ARGUMENTS_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.CommandLineArguments";
const char PRO_FILE_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.ProFile";
const char USE_TERMINAL_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.UseTerminal";
const char USE_DYLD_IMAGE_SUFFIX_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.UseDyldImageSuffix";
const char USER_WORKING_DIRECTORY_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.UserWorkingDirectory";

QString pathFromId(Core::Id id)
{
    return id.suffixAfter(QT4_RC_PREFIX);
}

} // namespace

//
// Qt4RunConfiguration
//

Qt4RunConfiguration::Qt4RunConfiguration(ProjectExplorer::Target *parent, Core::Id id) :
    LocalApplicationRunConfiguration(parent, id),
    m_proFilePath(pathFromId(id)),
    m_runMode(Gui),
    m_isUsingDyldImageSuffix(false)
{
    Qt4Project *project = static_cast<Qt4Project *>(parent->project());
    m_parseSuccess = project->validParse(m_proFilePath);
    m_parseInProgress = project->parseInProgress(m_proFilePath);

    ctor();
}

Qt4RunConfiguration::Qt4RunConfiguration(ProjectExplorer::Target *parent, Qt4RunConfiguration *source) :
    LocalApplicationRunConfiguration(parent, source),
    m_commandLineArguments(source->m_commandLineArguments),
    m_proFilePath(source->m_proFilePath),
    m_runMode(source->m_runMode),
    m_isUsingDyldImageSuffix(source->m_isUsingDyldImageSuffix),
    m_userWorkingDirectory(source->m_userWorkingDirectory),
    m_parseSuccess(source->m_parseSuccess),
    m_parseInProgress(source->m_parseInProgress)
{
    ctor();
}

Qt4RunConfiguration::~Qt4RunConfiguration()
{
}

bool Qt4RunConfiguration::isEnabled() const
{
    return m_parseSuccess && !m_parseInProgress;
}

QString Qt4RunConfiguration::disabledReason() const
{
    if (m_parseInProgress)
        return tr("The .pro file '%1' is currently being parsed.")
                .arg(QFileInfo(m_proFilePath).fileName());

    if (!m_parseSuccess)
        return static_cast<Qt4Project *>(target()->project())->disabledReasonForRunConfiguration(m_proFilePath);
    return QString();
}

void Qt4RunConfiguration::proFileUpdated(Qt4ProjectManager::Qt4ProFileNode *pro, bool success, bool parseInProgress)
{
    ProjectExplorer::LocalEnvironmentAspect *aspect
            = extraAspect<ProjectExplorer::LocalEnvironmentAspect>();
    QTC_ASSERT(aspect, return);

    if (m_proFilePath != pro->path()) {
        if (!parseInProgress) {
            // We depend on all .pro files for the LD_LIBRARY_PATH so we emit a signal for all .pro files
            // This can be optimized by checking whether LD_LIBRARY_PATH changed
            aspect->buildEnvironmentHasChanged();
        }
        return;
    }

    bool enabled = isEnabled();
    QString reason = disabledReason();
    m_parseSuccess = success;
    m_parseInProgress = parseInProgress;
    if (enabled != isEnabled() || reason != disabledReason())
        emit enabledChanged();

    if (!parseInProgress) {
        emit effectiveTargetInformationChanged();
        aspect->buildEnvironmentHasChanged();
    }
}

void Qt4RunConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    m_forcedGuiMode = (version && version->type() == QLatin1String(QtSupport::Constants::SIMULATORQT));

    connect(target()->project(), SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)),
            this, SLOT(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)));
    connect(target(), SIGNAL(kitChanged()),
            this, SLOT(kitChanged()));
}

void Qt4RunConfiguration::kitChanged()
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    m_forcedGuiMode = (version && version->type() == QLatin1String(QtSupport::Constants::SIMULATORQT));
    emit runModeChanged(runMode()); // Always emit
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

    QHBoxLayout *hl = new QHBoxLayout();
    hl->addStretch();
    m_disabledIcon = new QLabel(this);
    m_disabledIcon->setPixmap(QPixmap(QLatin1String(":/projectexplorer/images/compile_warning.png")));
    hl->addWidget(m_disabledIcon);
    m_disabledReason = new QLabel(this);
    m_disabledReason->setVisible(false);
    hl->addWidget(m_disabledReason);
    hl->addStretch();
    vboxTopLayout->addLayout(hl);

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
    m_workingDirectoryEdit->setExpectedKind(Utils::PathChooser::Directory);
    m_workingDirectoryEdit->setPath(m_qt4RunConfiguration->baseWorkingDirectory());
    m_workingDirectoryEdit->setBaseDirectory(m_qt4RunConfiguration->target()->project()->projectDirectory());
    ProjectExplorer::EnvironmentAspect *aspect
            = qt4RunConfiguration->extraAspect<ProjectExplorer::EnvironmentAspect>();
    if (aspect) {
        connect(aspect, SIGNAL(environmentChanged()), this, SLOT(environmentWasChanged()));
        environmentWasChanged();
    }
    m_workingDirectoryEdit->setPromptDialogTitle(tr("Select Working Directory"));

    QToolButton *resetButton = new QToolButton(this);
    resetButton->setToolTip(tr("Reset to default"));
    resetButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_RESET)));

    QHBoxLayout *boxlayout = new QHBoxLayout();
    boxlayout->setMargin(0);
    boxlayout->addWidget(m_workingDirectoryEdit);
    boxlayout->addWidget(resetButton);
    toplayout->addRow(tr("Working directory:"), boxlayout);

    QHBoxLayout *innerBox = new QHBoxLayout();
    m_useTerminalCheck = new QCheckBox(tr("Run in terminal"), this);
    m_useTerminalCheck->setChecked(m_qt4RunConfiguration->runMode() == ProjectExplorer::LocalApplicationRunConfiguration::Console);
    m_useTerminalCheck->setVisible(!m_qt4RunConfiguration->forcedGuiMode());
    innerBox->addWidget(m_useTerminalCheck);

    m_useQvfbCheck = new QCheckBox(tr("Run on QVFb"), this);
    m_useQvfbCheck->setToolTip(tr("Check this option to run the application on a Qt Virtual Framebuffer."));
    m_useQvfbCheck->setChecked(m_qt4RunConfiguration->runMode() == ProjectExplorer::LocalApplicationRunConfiguration::Console);
    m_useQvfbCheck->setVisible(false);
    innerBox->addWidget(m_useQvfbCheck);
    innerBox->addStretch();
    toplayout->addRow(QString(), innerBox);

    if (Utils::HostOsInfo::isMacHost()) {
        m_usingDyldImageSuffix = new QCheckBox(tr("Use debug version of frameworks (DYLD_IMAGE_SUFFIX=_debug)"), this);
        m_usingDyldImageSuffix->setChecked(m_qt4RunConfiguration->isUsingDyldImageSuffix());
        toplayout->addRow(QString(), m_usingDyldImageSuffix);
        connect(m_usingDyldImageSuffix, SIGNAL(toggled(bool)),
                this, SLOT(usingDyldImageSuffixToggled(bool)));
    }

    runConfigurationEnabledChange();

    connect(m_workingDirectoryEdit, SIGNAL(changed(QString)),
            this, SLOT(workDirectoryEdited()));

    connect(resetButton, SIGNAL(clicked()),
            this, SLOT(workingDirectoryReseted()));

    connect(m_argumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(argumentsEdited(QString)));
    connect(m_useTerminalCheck, SIGNAL(toggled(bool)),
            this, SLOT(termToggled(bool)));
    connect(m_useQvfbCheck, SIGNAL(toggled(bool)),
            this, SLOT(qvfbToggled(bool)));

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

    connect(qt4RunConfiguration, SIGNAL(enabledChanged()),
            this, SLOT(runConfigurationEnabledChange()));
}

Qt4RunConfigurationWidget::~Qt4RunConfigurationWidget()
{
}

void Qt4RunConfigurationWidget::environmentWasChanged()
{
    ProjectExplorer::EnvironmentAspect *aspect
            = m_qt4RunConfiguration->extraAspect<ProjectExplorer::EnvironmentAspect>();
    QTC_ASSERT(aspect, return);
    m_workingDirectoryEdit->setEnvironment(aspect->environment());
}

void Qt4RunConfigurationWidget::runConfigurationEnabledChange()
{
    bool enabled = m_qt4RunConfiguration->isEnabled();
    m_disabledIcon->setVisible(!enabled);
    m_disabledReason->setVisible(!enabled);
    m_disabledReason->setText(m_qt4RunConfiguration->disabledReason());
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
    m_qt4RunConfiguration->setBaseWorkingDirectory(QString());
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

void Qt4RunConfigurationWidget::qvfbToggled(bool on)
{
    Q_UNUSED(on);
    m_ignoreChange = true;
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
    if (!m_ignoreChange) {
        m_useTerminalCheck->setVisible(!m_qt4RunConfiguration->forcedGuiMode());
        m_useTerminalCheck->setChecked(runMode == LocalApplicationRunConfiguration::Console);
    }
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
    map.insert(QLatin1String(USER_WORKING_DIRECTORY_KEY), m_userWorkingDirectory);
    return map;
}

bool Qt4RunConfiguration::fromMap(const QVariantMap &map)
{
    const QDir projectDir = QDir(target()->project()->projectDirectory());
    m_commandLineArguments = map.value(QLatin1String(COMMAND_LINE_ARGUMENTS_KEY)).toString();
    m_proFilePath = QDir::cleanPath(projectDir.filePath(map.value(QLatin1String(PRO_FILE_KEY)).toString()));
    m_runMode = map.value(QLatin1String(USE_TERMINAL_KEY), false).toBool() ? Console : Gui;
    m_isUsingDyldImageSuffix = map.value(QLatin1String(USE_DYLD_IMAGE_SUFFIX_KEY), false).toBool();

    m_userWorkingDirectory = map.value(QLatin1String(USER_WORKING_DIRECTORY_KEY)).toString();

    m_parseSuccess = static_cast<Qt4Project *>(target()->project())->validParse(m_proFilePath);
    m_parseInProgress = static_cast<Qt4Project *>(target()->project())->parseInProgress(m_proFilePath);

    return RunConfiguration::fromMap(map);
}

QString Qt4RunConfiguration::executable() const
{
    Qt4Project *pro = static_cast<Qt4Project *>(target()->project());
    const Qt4ProFileNode *node = pro->rootQt4ProjectNode()->findProFileFor(m_proFilePath);
    return extractWorkingDirAndExecutable(node).second;
}

LocalApplicationRunConfiguration::RunMode Qt4RunConfiguration::runMode() const
{
    if (m_forcedGuiMode)
        return LocalApplicationRunConfiguration::Gui;
    return m_runMode;
}

bool Qt4RunConfiguration::forcedGuiMode() const
{
    return m_forcedGuiMode;
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
    ProjectExplorer::EnvironmentAspect *aspect
            = extraAspect<ProjectExplorer::EnvironmentAspect>();
    QTC_ASSERT(aspect, baseWorkingDirectory());
    return QDir::cleanPath(aspect->environment().expandVariables(
                Utils::expandMacros(baseWorkingDirectory(), macroExpander())));
}

QString Qt4RunConfiguration::baseWorkingDirectory() const
{
    // if the user overrode us, then return his working directory
    if (!m_userWorkingDirectory.isEmpty())
        return m_userWorkingDirectory;

    // else what the pro file reader tells us
    Qt4Project *pro = static_cast<Qt4Project *>(target()->project());
    const Qt4ProFileNode *node = pro->rootQt4ProjectNode()->findProFileFor(m_proFilePath);
    return extractWorkingDirAndExecutable(node).first;
}

QString Qt4RunConfiguration::commandLineArguments() const
{
    return Utils::QtcProcess::expandMacros(m_commandLineArguments, macroExpander());
}

QString Qt4RunConfiguration::rawCommandLineArguments() const
{
    return m_commandLineArguments;
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

void Qt4RunConfiguration::addToBaseEnvironment(Utils::Environment &env) const
{
    if (m_isUsingDyldImageSuffix)
        env.set(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));

    // The user could be linking to a library found via a -L/some/dir switch
    // to find those libraries while actually running we explicitly prepend those
    // dirs to the library search path
    const Qt4ProFileNode *node = static_cast<Qt4Project *>(target()->project())->rootQt4ProjectNode()->findProFileFor(m_proFilePath);
    if (node) {
        const QStringList libDirectories = node->variableValue(LibDirectoriesVar);
        if (!libDirectories.isEmpty()) {
            const QString proDirectory = node->buildDir();
            foreach (QString dir, libDirectories) {
                // Fix up relative entries like "LIBS+=-L.."
                const QFileInfo fi(dir);
                if (!fi.isAbsolute())
                    dir = QDir::cleanPath(proDirectory + QLatin1Char('/') + dir);
                env.prependOrSetLibrarySearchPath(dir);
            } // foreach
        } // libDirectories
    } // node

    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (qtVersion)
        env.prependOrSetLibrarySearchPath(qtVersion->qmakeProperty("QT_INSTALL_LIBS"));
}

QString Qt4RunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

QString Qt4RunConfiguration::dumperLibrary() const
{
    return QtSupport::QtKitInformation::dumperLibrary(target()->kit());
}

QStringList Qt4RunConfiguration::dumperLibraryLocations() const
{
    return QtSupport::QtKitInformation::dumperLibraryLocations(target()->kit());
}

QString Qt4RunConfiguration::defaultDisplayName()
{
    QString defaultName;
    if (!m_proFilePath.isEmpty())
        defaultName = QFileInfo(m_proFilePath).completeBaseName();
    else
        defaultName = tr("Qt Run Configuration");
    return defaultName;
}

Utils::OutputFormatter *Qt4RunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

QPair<QString, QString> Qt4RunConfiguration::extractWorkingDirAndExecutable(const Qt4ProFileNode *node) const
{
    if (!node)
        return qMakePair(QString(), QString());
    TargetInformation ti = node->targetInformation();
    if (!ti.valid)
        return qMakePair(QString(), QString());

    const QStringList &config = node->variableValue(ConfigVar);

    QString destDir = ti.destDir;
    QString workingDir;
    if (!destDir.isEmpty()) {
        bool workingDirIsBaseDir = false;
        if (destDir == ti.buildTarget) {
            workingDirIsBaseDir = true;
        }
        if (QDir::isRelativePath(destDir))
            destDir = QDir::cleanPath(ti.buildDir + QLatin1Char('/') + destDir);

        if (workingDirIsBaseDir)
            workingDir = ti.buildDir;
        else
            workingDir = destDir;
    } else {
        destDir = ti.buildDir;
        workingDir = ti.buildDir;
    }

    if (Utils::HostOsInfo::isMacHost()
            && config.contains(QLatin1String("app_bundle"))) {
        const QString infix = QLatin1Char('/') + ti.target
                + QLatin1String(".app/Contents/MacOS");
        workingDir += infix;
        destDir += infix;
    }

    QString executable = QDir::cleanPath(destDir + QLatin1Char('/') + ti.target);
    executable = Utils::HostOsInfo::withExecutableSuffix(executable);
    //qDebug() << "##### Qt4RunConfiguration::extractWorkingDirAndExecutable:" workingDir << executable;
    return qMakePair(workingDir, executable);
}

///
/// Qt4RunConfigurationFactory
/// This class is used to restore run settings (saved in .user files)
///

Qt4RunConfigurationFactory::Qt4RunConfigurationFactory(QObject *parent) :
    QmakeRunConfigurationFactory(parent)
{ setObjectName(QLatin1String("Qt4RunConfigurationFactory")); }

Qt4RunConfigurationFactory::~Qt4RunConfigurationFactory()
{ }

bool Qt4RunConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    Qt4Project *project = static_cast<Qt4Project *>(parent->project());
    return project->hasApplicationProFile(pathFromId(id));
}

ProjectExplorer::RunConfiguration *Qt4RunConfigurationFactory::doCreate(ProjectExplorer::Target *parent, const Core::Id id)
{
    Qt4RunConfiguration *rc = new Qt4RunConfiguration(parent, id);
    const Qt4ProFileNode *node = static_cast<Qt4Project *>(parent->project())->rootQt4ProjectNode()->findProFileFor(rc->proFilePath());
    if (node) // should always be found
        rc->setRunMode(node->variableValue(ConfigVar).contains(QLatin1String("console"))
                       && !node->variableValue(QtVar).contains(QLatin1String("testlib"))
                       ? ProjectExplorer::LocalApplicationRunConfiguration::Console
                       : ProjectExplorer::LocalApplicationRunConfiguration::Gui);
    return rc;
}

bool Qt4RunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map).toString().startsWith(QLatin1String(QT4_RC_PREFIX));
}

ProjectExplorer::RunConfiguration *Qt4RunConfigurationFactory::doRestore(ProjectExplorer::Target *parent,
                                                                         const QVariantMap &map)
{
    return new Qt4RunConfiguration(parent, ProjectExplorer::idFromMap(map));
}

bool Qt4RunConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration *Qt4RunConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    Qt4RunConfiguration *old = static_cast<Qt4RunConfiguration *>(source);
    return new Qt4RunConfiguration(parent, old);
}

QList<Core::Id> Qt4RunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> result;
    if (!canHandle(parent))
        return result;

    Qt4Project *project = static_cast<Qt4Project *>(parent->project());
    QStringList proFiles = project->applicationProFilePathes(QLatin1String(QT4_RC_PREFIX));
    foreach (const QString &pf, proFiles)
        result << Core::Id::fromString(pf);
    return result;
}

QString Qt4RunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    return QFileInfo(pathFromId(id)).completeBaseName();
}

bool Qt4RunConfigurationFactory::canHandle(ProjectExplorer::Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    if (!qobject_cast<Qt4Project *>(t->project()))
        return false;
    Core::Id devType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(t->kit());
    return devType == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

QList<ProjectExplorer::RunConfiguration *> Qt4RunConfigurationFactory::runConfigurationsForNode(ProjectExplorer::Target *t, ProjectExplorer::Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, t->runConfigurations())
        if (Qt4RunConfiguration *qt4c = qobject_cast<Qt4RunConfiguration *>(rc))
            if (qt4c->proFilePath() == n->path())
                result << rc;
    return result;

}
