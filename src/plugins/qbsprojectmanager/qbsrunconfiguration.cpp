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

#include "qbsrunconfiguration.h"

#include "qbsdeployconfigurationfactory.h"
#include "qbsinstallstep.h"
#include "qbsproject.h"

#include <coreplugin/coreconstants.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/localapplicationruncontrol.h>
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

#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QToolButton>
#include <QComboBox>
#include <QDir>

namespace {

const char QBS_RC_PREFIX[] = "Qbs.RunConfiguration:";
const char COMMAND_LINE_ARGUMENTS_KEY[] = "Qbs.RunConfiguration.CommandLineArguments";
const char USE_TERMINAL_KEY[] = "Qbs.RunConfiguration.UseTerminal";
const char USER_WORKING_DIRECTORY_KEY[] = "Qbs.RunConfiguration.UserWorkingDirectory";

QString productFromId(Core::Id id)
{
    return id.suffixAfter(QBS_RC_PREFIX);
}

const qbs::ProductData findProduct(const qbs::ProjectData &pro, const QString &name)
{
    foreach (const qbs::ProductData &product, pro.allProducts()) {
        if (product.name() == name)
            return product;
    }
    return qbs::ProductData();
}

} // namespace

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsRunConfiguration:
// --------------------------------------------------------------------

QbsRunConfiguration::QbsRunConfiguration(ProjectExplorer::Target *parent, Core::Id id) :
    LocalApplicationRunConfiguration(parent, id),
    m_qbsProduct(productFromId(id)),
    m_runMode(Gui),
    m_currentInstallStep(0),
    m_currentBuildStepList(0)
{
    ctor();
}

QbsRunConfiguration::QbsRunConfiguration(ProjectExplorer::Target *parent, QbsRunConfiguration *source) :
    LocalApplicationRunConfiguration(parent, source),
    m_qbsProduct(source->m_qbsProduct),
    m_commandLineArguments(source->m_commandLineArguments),
    m_runMode(source->m_runMode),
    m_userWorkingDirectory(source->m_userWorkingDirectory),
    m_currentInstallStep(source->m_currentInstallStep),
    m_currentBuildStepList(source->m_currentBuildStepList)
{
    ctor();
}

bool QbsRunConfiguration::isEnabled() const
{
    QbsProject *project = static_cast<QbsProject *>(target()->project());
    return !project->isParsing() && project->hasParseResult();
}

QString QbsRunConfiguration::disabledReason() const
{
    QbsProject *project = static_cast<QbsProject *>(target()->project());
    if (project->isParsing())
        return tr("The .qbs files are currently being parsed.");

    if (!project->hasParseResult())
        return tr("Parsing of .qbs files has failed.");
    return QString();
}

void QbsRunConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());

    QbsProject *project = static_cast<QbsProject *>(target()->project());
    connect(project, SIGNAL(projectParsingStarted()), this, SIGNAL(enabledChanged()));
    connect(project, SIGNAL(projectParsingDone(bool)), this, SIGNAL(enabledChanged()));

    connect(target(), SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),
            this, SLOT(installStepChanged()));
    installStepChanged();
}

QWidget *QbsRunConfiguration::createConfigurationWidget()
{
    return new QbsRunConfigurationWidget(this, 0);
}

QVariantMap QbsRunConfiguration::toMap() const
{
    QVariantMap map(LocalApplicationRunConfiguration::toMap());
    map.insert(QLatin1String(COMMAND_LINE_ARGUMENTS_KEY), m_commandLineArguments);
    map.insert(QLatin1String(USE_TERMINAL_KEY), m_runMode == Console);
    map.insert(QLatin1String(USER_WORKING_DIRECTORY_KEY), m_userWorkingDirectory);
    return map;
}

bool QbsRunConfiguration::fromMap(const QVariantMap &map)
{
    m_commandLineArguments = map.value(QLatin1String(COMMAND_LINE_ARGUMENTS_KEY)).toString();
    m_runMode = map.value(QLatin1String(USE_TERMINAL_KEY), false).toBool() ? Console : Gui;

    m_userWorkingDirectory = map.value(QLatin1String(USER_WORKING_DIRECTORY_KEY)).toString();

    return RunConfiguration::fromMap(map);
}

void QbsRunConfiguration::installStepChanged()
{
    if (m_currentInstallStep)
        disconnect(m_currentInstallStep, SIGNAL(changed()), this, SIGNAL(targetInformationChanged()));
    if (m_currentBuildStepList) {
        disconnect(m_currentBuildStepList, SIGNAL(stepInserted(int)), this, SLOT(installStepChanged()));
        disconnect(m_currentBuildStepList, SIGNAL(stepRemoved(int)), this, SLOT(installStepChanged()));
        disconnect(m_currentBuildStepList, SIGNAL(stepMoved(int,int)), this, SLOT(installStepChanged()));
    }

    QbsDeployConfiguration *activeDc = qobject_cast<QbsDeployConfiguration *>(target()->activeDeployConfiguration());
    m_currentBuildStepList = activeDc ? activeDc->stepList() : 0;
    m_currentInstallStep = activeDc ? activeDc->qbsInstallStep() : 0;

    if (m_currentInstallStep)
        connect(m_currentInstallStep, SIGNAL(changed()), this, SIGNAL(targetInformationChanged()));

    if (m_currentBuildStepList) {
        connect(m_currentBuildStepList, SIGNAL(stepInserted(int)), this, SLOT(installStepChanged()));
        connect(m_currentBuildStepList, SIGNAL(stepRemoved(int)), this, SLOT(installStepChanged()));
        connect(m_currentBuildStepList, SIGNAL(stepMoved(int,int)), this, SLOT(installStepChanged()));
    }

    emit targetInformationChanged();
}

QString QbsRunConfiguration::executable() const
{
    QbsProject *pro = static_cast<QbsProject *>(target()->project());
    const qbs::ProductData product = findProduct(pro->qbsProjectData(), m_qbsProduct);

    if (!product.isValid() || !pro->qbsProject())
        return QString();

    return pro->qbsProject()->targetExecutable(product, installOptions());
}

ProjectExplorer::LocalApplicationRunConfiguration::RunMode QbsRunConfiguration::runMode() const
{
    if (m_forcedGuiMode)
        return LocalApplicationRunConfiguration::Gui;
    return m_runMode;
}

bool QbsRunConfiguration::forcedGuiMode() const
{
    return m_forcedGuiMode;
}

QString QbsRunConfiguration::workingDirectory() const
{
    ProjectExplorer::EnvironmentAspect *aspect
            = extraAspect<ProjectExplorer::EnvironmentAspect>();
    QTC_ASSERT(aspect, baseWorkingDirectory());
    return QDir::cleanPath(aspect->environment().expandVariables(
                Utils::expandMacros(baseWorkingDirectory(), macroExpander())));
}

QString QbsRunConfiguration::baseWorkingDirectory() const
{
    // if the user overrode us, then return his working directory
    if (!m_userWorkingDirectory.isEmpty())
        return m_userWorkingDirectory;

    // else what the pro file reader tells us
    const QString exe = executable();
    if (!exe.isEmpty())
        return QFileInfo(executable()).absolutePath();
    return QString();
}

QString QbsRunConfiguration::commandLineArguments() const
{
    return Utils::QtcProcess::expandMacros(m_commandLineArguments, macroExpander());
}

QString QbsRunConfiguration::rawCommandLineArguments() const
{
    return m_commandLineArguments;
}

void QbsRunConfiguration::setBaseWorkingDirectory(const QString &wd)
{
    const QString &oldWorkingDirectory = workingDirectory();

    m_userWorkingDirectory = wd;

    const QString &newWorkingDirectory = workingDirectory();
    if (oldWorkingDirectory != newWorkingDirectory)
        emit baseWorkingDirectoryChanged(newWorkingDirectory);
}

void QbsRunConfiguration::setCommandLineArguments(const QString &argumentsString)
{
    m_commandLineArguments = argumentsString;
    emit commandLineArgumentsChanged(argumentsString);
}

void QbsRunConfiguration::setRunMode(RunMode runMode)
{
    m_runMode = runMode;
    emit runModeChanged(runMode);
}

void QbsRunConfiguration::addToBaseEnvironment(Utils::Environment &env) const
{
    // TODO: Use environment from Qbs!

    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (qtVersion)
        env.prependOrSetLibrarySearchPath(qtVersion->qmakeProperty("QT_INSTALL_LIBS"));
}

QString QbsRunConfiguration::qbsProduct() const
{
    return m_qbsProduct;
}

QString QbsRunConfiguration::dumperLibrary() const
{
    return QtSupport::QtKitInformation::dumperLibrary(target()->kit());
}

QStringList QbsRunConfiguration::dumperLibraryLocations() const
{
    return QtSupport::QtKitInformation::dumperLibraryLocations(target()->kit());
}

QString QbsRunConfiguration::defaultDisplayName()
{
    QString defaultName;
    if (!m_qbsProduct.isEmpty())
        defaultName = m_qbsProduct;
    else
        defaultName = tr("Qbs Run Configuration");
    return defaultName;
}

qbs::InstallOptions QbsRunConfiguration::installOptions() const
{
    if (m_currentInstallStep)
        return m_currentInstallStep->installOptions();
    return qbs::InstallOptions();
}

QString QbsRunConfiguration::installRoot() const
{
    if (m_currentInstallStep)
        return m_currentInstallStep->absoluteInstallRoot();
    return QString();
}

Utils::OutputFormatter *QbsRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

// --------------------------------------------------------------------
// QbsRunConfigurationWidget:
// --------------------------------------------------------------------

QbsRunConfigurationWidget::QbsRunConfigurationWidget(QbsRunConfiguration *rc, QWidget *parent)
    : QWidget(parent),
    m_rc(rc),
    m_ignoreChange(false),
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

    m_executableLineEdit = new QLineEdit(this);
    m_executableLineEdit->setEnabled(false);
    toplayout->addRow(tr("Executable:"), m_executableLineEdit);

    QLabel *argumentsLabel = new QLabel(tr("Arguments:"), this);
    m_argumentsLineEdit = new QLineEdit(m_rc->rawCommandLineArguments(), this);
    argumentsLabel->setBuddy(m_argumentsLineEdit);
    toplayout->addRow(argumentsLabel, m_argumentsLineEdit);

    m_workingDirectoryEdit = new Utils::PathChooser(this);
    m_workingDirectoryEdit->setExpectedKind(Utils::PathChooser::Directory);
    ProjectExplorer::EnvironmentAspect *aspect
            = m_rc->extraAspect<ProjectExplorer::EnvironmentAspect>();
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
    m_useTerminalCheck->setChecked(m_rc->runMode() == ProjectExplorer::LocalApplicationRunConfiguration::Console);
    m_useTerminalCheck->setVisible(!m_rc->forcedGuiMode());
    innerBox->addWidget(m_useTerminalCheck);

    innerBox->addStretch();
    toplayout->addRow(QString(), innerBox);

    runConfigurationEnabledChange();

    connect(m_workingDirectoryEdit, SIGNAL(changed(QString)),
            this, SLOT(workDirectoryEdited()));

    connect(resetButton, SIGNAL(clicked()),
            this, SLOT(workingDirectoryWasReset()));

    connect(m_argumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(argumentsEdited(QString)));
    connect(m_useTerminalCheck, SIGNAL(toggled(bool)),
            this, SLOT(termToggled(bool)));

    connect(m_rc, SIGNAL(baseWorkingDirectoryChanged(QString)),
            this, SLOT(workingDirectoryChanged(QString)));

    connect(m_rc, SIGNAL(commandLineArgumentsChanged(QString)),
            this, SLOT(commandLineArgumentsChanged(QString)));
    connect(m_rc, SIGNAL(runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode)),
            this, SLOT(runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode)));
    connect(m_rc, SIGNAL(targetInformationChanged()),
            this, SLOT(targetInformationHasChanged()), Qt::QueuedConnection);

    connect(m_rc, SIGNAL(enabledChanged()),
            this, SLOT(runConfigurationEnabledChange()));
}

void QbsRunConfigurationWidget::environmentWasChanged()
{
    ProjectExplorer::EnvironmentAspect *aspect
            = m_rc->extraAspect<ProjectExplorer::EnvironmentAspect>();
    QTC_ASSERT(aspect, return);
    m_workingDirectoryEdit->setEnvironment(aspect->environment());
}

void QbsRunConfigurationWidget::runConfigurationEnabledChange()
{
    bool enabled = m_rc->isEnabled();
    m_disabledIcon->setVisible(!enabled);
    m_disabledReason->setVisible(!enabled);
    m_disabledReason->setText(m_rc->disabledReason());
    targetInformationHasChanged();
}

void QbsRunConfigurationWidget::workDirectoryEdited()
{
    if (m_ignoreChange)
        return;
    m_ignoreChange = true;
    m_rc->setBaseWorkingDirectory(m_workingDirectoryEdit->rawPath());
    m_ignoreChange = false;
}

void QbsRunConfigurationWidget::workingDirectoryWasReset()
{
    // This emits a signal connected to workingDirectoryChanged()
    // that sets the m_workingDirectoryEdit
    m_rc->setBaseWorkingDirectory(QString());
}

void QbsRunConfigurationWidget::argumentsEdited(const QString &args)
{
    m_ignoreChange = true;
    m_rc->setCommandLineArguments(args);
    m_ignoreChange = false;
}

void QbsRunConfigurationWidget::termToggled(bool on)
{
    m_ignoreChange = true;
    m_rc->setRunMode(on ? ProjectExplorer::LocalApplicationRunConfiguration::Console
                        : ProjectExplorer::LocalApplicationRunConfiguration::Gui);
    m_ignoreChange = false;
}

void QbsRunConfigurationWidget::targetInformationHasChanged()
{
    m_ignoreChange = true;
    m_executableLineEdit->setText(m_rc->executable());

    m_workingDirectoryEdit->setPath(m_rc->baseWorkingDirectory());
    m_workingDirectoryEdit->setBaseDirectory(m_rc->target()->project()->projectDirectory());
    m_ignoreChange = false;
}

void QbsRunConfigurationWidget::workingDirectoryChanged(const QString &workingDirectory)
{
    if (!m_ignoreChange)
        m_workingDirectoryEdit->setPath(workingDirectory);
}

void QbsRunConfigurationWidget::commandLineArgumentsChanged(const QString &args)
{
    if (m_ignoreChange)
        return;
    m_argumentsLineEdit->setText(args);
}

void QbsRunConfigurationWidget::runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode runMode)
{
    if (!m_ignoreChange) {
        m_useTerminalCheck->setVisible(!m_rc->forcedGuiMode());
        m_useTerminalCheck->setChecked(runMode == ProjectExplorer::LocalApplicationRunConfiguration::Console);
    }
}

// --------------------------------------------------------------------
// QbsRunConfigurationFactory:
// --------------------------------------------------------------------

QbsRunConfigurationFactory::QbsRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
    setObjectName(QLatin1String("QbsRunConfigurationFactory"));
}

QbsRunConfigurationFactory::~QbsRunConfigurationFactory()
{ }

bool QbsRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;

    QbsProject *project = static_cast<QbsProject *>(parent->project());
    return findProduct(project->qbsProjectData(), productFromId(id)).isValid();
}

ProjectExplorer::RunConfiguration *QbsRunConfigurationFactory::doCreate(ProjectExplorer::Target *parent, const Core::Id id)
{
    return new QbsRunConfiguration(parent, id);
}

bool QbsRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map).toString().startsWith(QLatin1String(QBS_RC_PREFIX));
}

ProjectExplorer::RunConfiguration *QbsRunConfigurationFactory::doRestore(ProjectExplorer::Target *parent,
                                                                         const QVariantMap &map)
{
    return new QbsRunConfiguration(parent, ProjectExplorer::idFromMap(map));
}

bool QbsRunConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration *QbsRunConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    QbsRunConfiguration *old = static_cast<QbsRunConfiguration *>(source);
    return new QbsRunConfiguration(parent, old);
}

QList<Core::Id> QbsRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> result;
    if (!canHandle(parent))
        return result;

    QbsProject *project = static_cast<QbsProject *>(parent->project());
    if (!project || !project->qbsProject())
        return result;

    foreach (const qbs::ProductData &product, project->qbsProjectData().allProducts()) {
        if (!project->qbsProject()->targetExecutable(product, qbs::InstallOptions()).isEmpty())
            result << Core::Id::fromString(QString::fromLatin1(QBS_RC_PREFIX) + product.name());
    }
    return result;
}

QString QbsRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    return productFromId(id);
}

bool QbsRunConfigurationFactory::canHandle(ProjectExplorer::Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    if (!qobject_cast<QbsProject *>(t->project()))
        return false;
    Core::Id devType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(t->kit());
    return devType == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

} // namespace Internal
} // namespace QbsProjectManager
