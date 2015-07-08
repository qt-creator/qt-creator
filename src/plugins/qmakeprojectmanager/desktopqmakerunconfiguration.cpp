/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "desktopqmakerunconfiguration.h"

#include "qmakenodes.h"
#include "qmakeproject.h"
#include "qmakebuildconfiguration.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/variablechooser.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/detailswidget.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/persistentsettings.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

const char QMAKE_RC_PREFIX[] = "Qt4ProjectManager.Qt4RunConfiguration:";
const char COMMAND_LINE_ARGUMENTS_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.CommandLineArguments";
const char PRO_FILE_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.ProFile";
const char USE_TERMINAL_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.UseTerminal";
const char USE_DYLD_IMAGE_SUFFIX_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.UseDyldImageSuffix";
const char USER_WORKING_DIRECTORY_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.UserWorkingDirectory";

static Utils::FileName pathFromId(Core::Id id)
{
    return Utils::FileName::fromString(id.suffixAfter(QMAKE_RC_PREFIX));
}

//
// QmakeRunConfiguration
//

DesktopQmakeRunConfiguration::DesktopQmakeRunConfiguration(Target *parent, Core::Id id) :
    LocalApplicationRunConfiguration(parent, id),
    m_proFilePath(pathFromId(id))
{
    addExtraAspect(new ProjectExplorer::LocalEnvironmentAspect(this));

    QmakeProject *project = static_cast<QmakeProject *>(parent->project());
    m_parseSuccess = project->validParse(m_proFilePath);
    m_parseInProgress = project->parseInProgress(m_proFilePath);

    ctor();
}

DesktopQmakeRunConfiguration::DesktopQmakeRunConfiguration(Target *parent, DesktopQmakeRunConfiguration *source) :
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

DesktopQmakeRunConfiguration::~DesktopQmakeRunConfiguration()
{
}

bool DesktopQmakeRunConfiguration::isEnabled() const
{
    return m_parseSuccess && !m_parseInProgress;
}

QString DesktopQmakeRunConfiguration::disabledReason() const
{
    if (m_parseInProgress)
        return tr("The .pro file \"%1\" is currently being parsed.")
                .arg(m_proFilePath.fileName());

    if (!m_parseSuccess)
        return static_cast<QmakeProject *>(target()->project())->disabledReasonForRunConfiguration(m_proFilePath);
    return QString();
}

void DesktopQmakeRunConfiguration::proFileUpdated(QmakeProFileNode *pro, bool success, bool parseInProgress)
{
    if (m_proFilePath != pro->path())
        return;
    bool enabled = isEnabled();
    QString reason = disabledReason();
    m_parseSuccess = success;
    m_parseInProgress = parseInProgress;
    if (enabled != isEnabled() || reason != disabledReason())
        emit enabledChanged();

    if (!parseInProgress) {
        emit effectiveTargetInformationChanged();
        LocalEnvironmentAspect *aspect = extraAspect<LocalEnvironmentAspect>();
        QTC_ASSERT(aspect, return);
        aspect->buildEnvironmentHasChanged();
    }
}

void DesktopQmakeRunConfiguration::proFileEvaluated()
{
    // We depend on all .pro files for the LD_LIBRARY_PATH so we emit a signal for all .pro files
    // This can be optimized by checking whether LD_LIBRARY_PATH changed
    LocalEnvironmentAspect *aspect = extraAspect<LocalEnvironmentAspect>();
    QTC_ASSERT(aspect, return);
    aspect->buildEnvironmentHasChanged();
}

void DesktopQmakeRunConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());

    QmakeProject *project = static_cast<QmakeProject *>(target()->project());
    connect(project, &QmakeProject::proFileUpdated,
            this, &DesktopQmakeRunConfiguration::proFileUpdated);
    connect(project, &QmakeProject::proFilesEvaluated,
            this, &DesktopQmakeRunConfiguration::proFileEvaluated);
}

//////
/// DesktopQmakeRunConfigurationWidget
/////

DesktopQmakeRunConfigurationWidget::DesktopQmakeRunConfigurationWidget(DesktopQmakeRunConfiguration *qmakeRunConfiguration, QWidget *parent)
    : QWidget(parent),
    m_qmakeRunConfiguration(qmakeRunConfiguration)
{
    QVBoxLayout *vboxTopLayout = new QVBoxLayout(this);
    vboxTopLayout->setMargin(0);

    QHBoxLayout *hl = new QHBoxLayout();
    hl->addStretch();
    m_disabledIcon = new QLabel(this);
    m_disabledIcon->setPixmap(QPixmap(QLatin1String(Core::Constants::ICON_WARNING)));
    hl->addWidget(m_disabledIcon);
    m_disabledReason = new QLabel(this);
    m_disabledReason->setVisible(false);
    hl->addWidget(m_disabledReason);
    hl->addStretch();
    vboxTopLayout->addLayout(hl);

    m_detailsContainer = new DetailsWidget(this);
    m_detailsContainer->setState(DetailsWidget::NoSummary);
    vboxTopLayout->addWidget(m_detailsContainer);
    QWidget *detailsWidget = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(detailsWidget);
    QFormLayout *toplayout = new QFormLayout(detailsWidget);
    toplayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    toplayout->setMargin(0);

    m_executableLineLabel = new QLabel(m_qmakeRunConfiguration->executable(), this);
    m_executableLineLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    toplayout->addRow(tr("Executable:"), m_executableLineLabel);

    QLabel *argumentsLabel = new QLabel(tr("Arguments:"), this);
    m_argumentsLineEdit = new QLineEdit(qmakeRunConfiguration->rawCommandLineArguments(), this);
    argumentsLabel->setBuddy(m_argumentsLineEdit);
    toplayout->addRow(argumentsLabel, m_argumentsLineEdit);

    m_workingDirectoryEdit = new PathChooser(this);
    m_workingDirectoryEdit->setExpectedKind(PathChooser::Directory);
    m_workingDirectoryEdit->setHistoryCompleter(QLatin1String("Qmake.WorkingDir.History"));
    m_workingDirectoryEdit->setPath(m_qmakeRunConfiguration->baseWorkingDirectory());
    m_workingDirectoryEdit->setBaseFileName(m_qmakeRunConfiguration->target()->project()->projectDirectory());
    EnvironmentAspect *aspect = qmakeRunConfiguration->extraAspect<EnvironmentAspect>();
    if (aspect) {
        connect(aspect, SIGNAL(environmentChanged()), this, SLOT(environmentWasChanged()));
        environmentWasChanged();
    }
    m_workingDirectoryEdit->setPromptDialogTitle(tr("Select Working Directory"));

    QToolButton *resetButton = new QToolButton(this);
    resetButton->setToolTip(tr("Reset to Default"));
    resetButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_RESET)));

    QHBoxLayout *boxlayout = new QHBoxLayout();
    boxlayout->setMargin(0);
    boxlayout->addWidget(m_workingDirectoryEdit);
    boxlayout->addWidget(resetButton);
    toplayout->addRow(tr("Working directory:"), boxlayout);

    QHBoxLayout *innerBox = new QHBoxLayout();
    m_useTerminalCheck = new QCheckBox(tr("Run in terminal"), this);
    m_useTerminalCheck->setChecked(m_qmakeRunConfiguration->runMode() == ApplicationLauncher::Console);
    innerBox->addWidget(m_useTerminalCheck);

    m_useQvfbCheck = new QCheckBox(tr("Run on QVFb"), this);
    m_useQvfbCheck->setToolTip(tr("Check this option to run the application on a Qt Virtual Framebuffer."));
    m_useQvfbCheck->setChecked(m_qmakeRunConfiguration->runMode() == ApplicationLauncher::Console);
    m_useQvfbCheck->setVisible(false);
    innerBox->addWidget(m_useQvfbCheck);
    innerBox->addStretch();
    toplayout->addRow(QString(), innerBox);

    if (HostOsInfo::isMacHost()) {
        m_usingDyldImageSuffix = new QCheckBox(tr("Use debug version of frameworks (DYLD_IMAGE_SUFFIX=_debug)"), this);
        m_usingDyldImageSuffix->setChecked(m_qmakeRunConfiguration->isUsingDyldImageSuffix());
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

    connect(qmakeRunConfiguration, SIGNAL(baseWorkingDirectoryChanged(QString)),
            this, SLOT(workingDirectoryChanged(QString)));

    connect(qmakeRunConfiguration, SIGNAL(commandLineArgumentsChanged(QString)),
            this, SLOT(commandLineArgumentsChanged(QString)));
    connect(qmakeRunConfiguration, SIGNAL(runModeChanged(ProjectExplorer::ApplicationLauncher::Mode)),
            this, SLOT(runModeChanged(ProjectExplorer::ApplicationLauncher::Mode)));
    connect(qmakeRunConfiguration, SIGNAL(usingDyldImageSuffixChanged(bool)),
            this, SLOT(usingDyldImageSuffixChanged(bool)));
    connect(qmakeRunConfiguration, SIGNAL(effectiveTargetInformationChanged()),
            this, SLOT(effectiveTargetInformationChanged()), Qt::QueuedConnection);

    connect(qmakeRunConfiguration, SIGNAL(enabledChanged()),
            this, SLOT(runConfigurationEnabledChange()));

    Core::VariableChooser::addSupportForChildWidgets(this, m_qmakeRunConfiguration->macroExpander());
}

DesktopQmakeRunConfigurationWidget::~DesktopQmakeRunConfigurationWidget()
{
}

void DesktopQmakeRunConfigurationWidget::environmentWasChanged()
{
    EnvironmentAspect *aspect = m_qmakeRunConfiguration->extraAspect<EnvironmentAspect>();
    QTC_ASSERT(aspect, return);
    m_workingDirectoryEdit->setEnvironment(aspect->environment());
}

void DesktopQmakeRunConfigurationWidget::runConfigurationEnabledChange()
{
    bool enabled = m_qmakeRunConfiguration->isEnabled();
    m_disabledIcon->setVisible(!enabled);
    m_disabledReason->setVisible(!enabled);
    m_disabledReason->setText(m_qmakeRunConfiguration->disabledReason());
}

void DesktopQmakeRunConfigurationWidget::workDirectoryEdited()
{
    if (m_ignoreChange)
        return;
    m_ignoreChange = true;
    m_qmakeRunConfiguration->setBaseWorkingDirectory(m_workingDirectoryEdit->rawPath());
    m_ignoreChange = false;
}

void DesktopQmakeRunConfigurationWidget::workingDirectoryReseted()
{
    // This emits a signal connected to workingDirectoryChanged()
    // that sets the m_workingDirectoryEdit
    m_qmakeRunConfiguration->setBaseWorkingDirectory(QString());
}

void DesktopQmakeRunConfigurationWidget::argumentsEdited(const QString &args)
{
    m_ignoreChange = true;
    m_qmakeRunConfiguration->setCommandLineArguments(args);
    m_ignoreChange = false;
}

void DesktopQmakeRunConfigurationWidget::termToggled(bool on)
{
    m_ignoreChange = true;
    m_qmakeRunConfiguration->setRunMode(on ? ApplicationLauncher::Console
                                           : ApplicationLauncher::Gui);
    m_ignoreChange = false;
}

void DesktopQmakeRunConfigurationWidget::qvfbToggled(bool on)
{
    Q_UNUSED(on);
    m_ignoreChange = true;
    m_ignoreChange = false;
}

void DesktopQmakeRunConfigurationWidget::usingDyldImageSuffixToggled(bool state)
{
    m_ignoreChange = true;
    m_qmakeRunConfiguration->setUsingDyldImageSuffix(state);
    m_ignoreChange = false;
}

void DesktopQmakeRunConfigurationWidget::workingDirectoryChanged(const QString &workingDirectory)
{
    if (!m_ignoreChange)
        m_workingDirectoryEdit->setPath(workingDirectory);
}

void DesktopQmakeRunConfigurationWidget::commandLineArgumentsChanged(const QString &args)
{
    if (m_ignoreChange)
        return;
    m_argumentsLineEdit->setText(args);
}

void DesktopQmakeRunConfigurationWidget::runModeChanged(ApplicationLauncher::Mode runMode)
{
    if (!m_ignoreChange)
        m_useTerminalCheck->setChecked(runMode == ApplicationLauncher::Console);
}

void DesktopQmakeRunConfigurationWidget::usingDyldImageSuffixChanged(bool state)
{
    if (!m_ignoreChange && m_usingDyldImageSuffix)
        m_usingDyldImageSuffix->setChecked(state);
}

void DesktopQmakeRunConfigurationWidget::effectiveTargetInformationChanged()
{
    if (m_isShown) {
        m_executableLineLabel->setText(QDir::toNativeSeparators(m_qmakeRunConfiguration->executable()));
        m_ignoreChange = true;
        m_workingDirectoryEdit->setPath(QDir::toNativeSeparators(m_qmakeRunConfiguration->baseWorkingDirectory()));
        m_ignoreChange = false;
    }
}

void DesktopQmakeRunConfigurationWidget::showEvent(QShowEvent *event)
{
    m_isShown = true;
    effectiveTargetInformationChanged();
    QWidget::showEvent(event);
}

void DesktopQmakeRunConfigurationWidget::hideEvent(QHideEvent *event)
{
    m_isShown = false;
    QWidget::hideEvent(event);
}

QWidget *DesktopQmakeRunConfiguration::createConfigurationWidget()
{
    return new DesktopQmakeRunConfigurationWidget(this, 0);
}

QVariantMap DesktopQmakeRunConfiguration::toMap() const
{
    const QDir projectDir = QDir(target()->project()->projectDirectory().toString());
    QVariantMap map(LocalApplicationRunConfiguration::toMap());
    map.insert(QLatin1String(COMMAND_LINE_ARGUMENTS_KEY), m_commandLineArguments);
    map.insert(QLatin1String(PRO_FILE_KEY), projectDir.relativeFilePath(m_proFilePath.toString()));
    map.insert(QLatin1String(USE_TERMINAL_KEY), m_runMode == ApplicationLauncher::Console);
    map.insert(QLatin1String(USE_DYLD_IMAGE_SUFFIX_KEY), m_isUsingDyldImageSuffix);
    map.insert(QLatin1String(USER_WORKING_DIRECTORY_KEY), m_userWorkingDirectory);
    return map;
}

bool DesktopQmakeRunConfiguration::fromMap(const QVariantMap &map)
{
    const QDir projectDir = QDir(target()->project()->projectDirectory().toString());
    m_commandLineArguments = map.value(QLatin1String(COMMAND_LINE_ARGUMENTS_KEY)).toString();
    m_proFilePath = Utils::FileName::fromUserInput(projectDir.filePath(map.value(QLatin1String(PRO_FILE_KEY)).toString()));
    m_runMode = map.value(QLatin1String(USE_TERMINAL_KEY), false).toBool()
            ? ApplicationLauncher::Console : ApplicationLauncher::Gui;
    m_isUsingDyldImageSuffix = map.value(QLatin1String(USE_DYLD_IMAGE_SUFFIX_KEY), false).toBool();

    m_userWorkingDirectory = map.value(QLatin1String(USER_WORKING_DIRECTORY_KEY)).toString();

    m_parseSuccess = static_cast<QmakeProject *>(target()->project())->validParse(m_proFilePath);
    m_parseInProgress = static_cast<QmakeProject *>(target()->project())->parseInProgress(m_proFilePath);

    return LocalApplicationRunConfiguration::fromMap(map);
}

QString DesktopQmakeRunConfiguration::executable() const
{
    QmakeProject *pro = static_cast<QmakeProject *>(target()->project());
    const QmakeProFileNode *node = pro->rootQmakeProjectNode()->findProFileFor(m_proFilePath);
    return extractWorkingDirAndExecutable(node).second;
}

ApplicationLauncher::Mode DesktopQmakeRunConfiguration::runMode() const
{
    return m_runMode;
}

bool DesktopQmakeRunConfiguration::isUsingDyldImageSuffix() const
{
    return m_isUsingDyldImageSuffix;
}

void DesktopQmakeRunConfiguration::setUsingDyldImageSuffix(bool state)
{
    m_isUsingDyldImageSuffix = state;
    emit usingDyldImageSuffixChanged(state);
}

QString DesktopQmakeRunConfiguration::workingDirectory() const
{
    EnvironmentAspect *aspect = extraAspect<EnvironmentAspect>();
    QTC_ASSERT(aspect, return baseWorkingDirectory());
    return QDir::cleanPath(aspect->environment().expandVariables(
                macroExpander()->expand(baseWorkingDirectory())));
}

QString DesktopQmakeRunConfiguration::baseWorkingDirectory() const
{
    // if the user overrode us, then return his working directory
    if (!m_userWorkingDirectory.isEmpty())
        return m_userWorkingDirectory;

    // else what the pro file reader tells us
    QmakeProject *pro = static_cast<QmakeProject *>(target()->project());
    const QmakeProFileNode *node = pro->rootQmakeProjectNode()->findProFileFor(m_proFilePath);
    return extractWorkingDirAndExecutable(node).first;
}

QString DesktopQmakeRunConfiguration::commandLineArguments() const
{
    return macroExpander()->expandProcessArgs(m_commandLineArguments);
}

QString DesktopQmakeRunConfiguration::rawCommandLineArguments() const
{
    return m_commandLineArguments;
}

void DesktopQmakeRunConfiguration::setBaseWorkingDirectory(const QString &wd)
{
    const QString &oldWorkingDirectory = workingDirectory();

    m_userWorkingDirectory = wd;

    const QString &newWorkingDirectory = workingDirectory();
    if (oldWorkingDirectory != newWorkingDirectory)
        emit baseWorkingDirectoryChanged(newWorkingDirectory);
}

void DesktopQmakeRunConfiguration::setCommandLineArguments(const QString &argumentsString)
{
    m_commandLineArguments = argumentsString;
    emit commandLineArgumentsChanged(argumentsString);
}

void DesktopQmakeRunConfiguration::setRunMode(ApplicationLauncher::Mode runMode)
{
    m_runMode = runMode;
    emit runModeChanged(runMode);
}

void DesktopQmakeRunConfiguration::addToBaseEnvironment(Environment &env) const
{
    if (m_isUsingDyldImageSuffix)
        env.set(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));

    // The user could be linking to a library found via a -L/some/dir switch
    // to find those libraries while actually running we explicitly prepend those
    // dirs to the library search path
    const QmakeProFileNode *node = static_cast<QmakeProject *>(target()->project())->rootQmakeProjectNode()->findProFileFor(m_proFilePath);
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

Utils::FileName DesktopQmakeRunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

QString DesktopQmakeRunConfiguration::defaultDisplayName()
{
    QString defaultName;
    if (!m_proFilePath.isEmpty())
        defaultName = m_proFilePath.toFileInfo().completeBaseName();
    else
        defaultName = tr("Qt Run Configuration");
    return defaultName;
}

OutputFormatter *DesktopQmakeRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

QPair<QString, QString> DesktopQmakeRunConfiguration::extractWorkingDirAndExecutable(const QmakeProFileNode *node) const
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
        if (destDir == ti.buildTarget)
            workingDirIsBaseDir = true;
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

    if (HostOsInfo::isMacHost() && config.contains(QLatin1String("app_bundle"))) {
        const QString infix = QLatin1Char('/') + ti.target
                + QLatin1String(".app/Contents/MacOS");
        workingDir += infix;
        destDir += infix;
    }

    QString executable = QDir::cleanPath(destDir + QLatin1Char('/') + ti.target);
    executable = HostOsInfo::withExecutableSuffix(executable);
    //qDebug() << "##### QmakeRunConfiguration::extractWorkingDirAndExecutable:" workingDir << executable;
    return qMakePair(workingDir, executable);
}

///
/// DesktopQmakeRunConfigurationFactory
/// This class is used to restore run settings (saved in .user files)
///

DesktopQmakeRunConfigurationFactory::DesktopQmakeRunConfigurationFactory(QObject *parent) :
    QmakeRunConfigurationFactory(parent)
{ setObjectName(QLatin1String("DesktopQmakeRunConfigurationFactory")); }

DesktopQmakeRunConfigurationFactory::~DesktopQmakeRunConfigurationFactory()
{ }

bool DesktopQmakeRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    QmakeProject *project = static_cast<QmakeProject *>(parent->project());
    return project->hasApplicationProFile(pathFromId(id));
}

RunConfiguration *DesktopQmakeRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    DesktopQmakeRunConfiguration *rc = new DesktopQmakeRunConfiguration(parent, id);
    const QmakeProFileNode *node = static_cast<QmakeProject *>(parent->project())->rootQmakeProjectNode()->findProFileFor(rc->proFilePath());
    if (node) // should always be found
        rc->setRunMode(node->variableValue(ConfigVar).contains(QLatin1String("console"))
                       && !node->variableValue(QtVar).contains(QLatin1String("testlib"))
                       ? ApplicationLauncher::Console : ApplicationLauncher::Gui);
    return rc;
}

bool DesktopQmakeRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return idFromMap(map).toString().startsWith(QLatin1String(QMAKE_RC_PREFIX));
}

RunConfiguration *DesktopQmakeRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    return new DesktopQmakeRunConfiguration(parent, idFromMap(map));
}

bool DesktopQmakeRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

RunConfiguration *DesktopQmakeRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    DesktopQmakeRunConfiguration *old = static_cast<DesktopQmakeRunConfiguration *>(source);
    return new DesktopQmakeRunConfiguration(parent, old);
}

QList<Core::Id> DesktopQmakeRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();

    QmakeProject *project = static_cast<QmakeProject *>(parent->project());
    QList<QmakeProFileNode *> nodes = project->applicationProFiles();
    if (mode == AutoCreate)
        nodes = QmakeProject::nodesWithQtcRunnable(nodes);
    return QmakeProject::idsForNodes(Core::Id(QMAKE_RC_PREFIX), nodes);
}

QString DesktopQmakeRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    return pathFromId(id).toFileInfo().completeBaseName();
}

bool DesktopQmakeRunConfigurationFactory::canHandle(Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    if (!qobject_cast<QmakeProject *>(t->project()))
        return false;
    Core::Id devType = DeviceTypeKitInformation::deviceTypeId(t->kit());
    return devType == Constants::DESKTOP_DEVICE_TYPE;
}

QList<RunConfiguration *> DesktopQmakeRunConfigurationFactory::runConfigurationsForNode(Target *t, const Node *n)
{
    QList<RunConfiguration *> result;
    foreach (RunConfiguration *rc, t->runConfigurations())
        if (DesktopQmakeRunConfiguration *qt4c = qobject_cast<DesktopQmakeRunConfiguration *>(rc))
            if (qt4c->proFilePath() == n->path())
                result << rc;
    return result;
}

} // namespace Internal
} // namespace QmakeProjectManager
