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
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <utils/qtcprocess.h>
#include <utils/pathchooser.h>
#include <utils/detailswidget.h>
#include <utils/stringutils.h>
#include <utils/persistentsettings.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/hostosinfo.h>

#include "api/runenvironment.h"

#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QToolButton>
#include <QComboBox>
#include <QDir>

using namespace ProjectExplorer;

namespace QbsProjectManager {
namespace Internal {

const char QBS_RC_PREFIX[] = "Qbs.RunConfiguration:";

static QString rcNameSeparator() { return QLatin1String("---Qbs.RC.NameSeparator---"); }

static Core::Id idFromProduct(const QbsProject *project, const qbs::ProductData &product)
{
    QString id = QLatin1String(QBS_RC_PREFIX);
    id.append(QbsProject::uniqueProductName(product)).append(rcNameSeparator())
            .append(QbsProject::productDisplayName(project->qbsProject(), product));
    return Core::Id::fromString(id);
}

static QString uniqueProductNameFromId(Core::Id id)
{
    const QString suffix = id.suffixAfter(QBS_RC_PREFIX);
    return suffix.left(suffix.indexOf(rcNameSeparator()));
}

static QString productDisplayNameFromId(Core::Id id)
{
    const QString suffix = id.suffixAfter(QBS_RC_PREFIX);
    const int sepPos = suffix.indexOf(rcNameSeparator());
    if (sepPos == -1)
        return suffix;
    return suffix.mid(sepPos + rcNameSeparator().count());
}

const qbs::ProductData findProduct(const qbs::ProjectData &pro, const QString &uniqeName)
{
    foreach (const qbs::ProductData &product, pro.allProducts()) {
        if (QbsProject::uniqueProductName(product) == uniqeName)
            return product;
    }
    return qbs::ProductData();
}

// --------------------------------------------------------------------
// QbsRunConfiguration:
// --------------------------------------------------------------------

QbsRunConfiguration::QbsRunConfiguration(Target *parent, Core::Id id) :
    LocalApplicationRunConfiguration(parent, id),
    m_uniqueProductName(uniqueProductNameFromId(id)),
    m_currentInstallStep(0),
    m_currentBuildStepList(0)
{
    addExtraAspect(new LocalEnvironmentAspect(this));
    addExtraAspect(new ArgumentsAspect(this, QStringLiteral("Qbs.RunConfiguration.CommandLineArguments")));
    addExtraAspect(new WorkingDirectoryAspect(this, QStringLiteral("Qbs.RunConfiguration.WorkingDirectory")));

    addExtraAspect(new TerminalAspect(this,
                                      QStringLiteral("Qbs.RunConfiguration.UseTerminal"),
                                      isConsoleApplication()));

    ctor();
}

QbsRunConfiguration::QbsRunConfiguration(Target *parent, QbsRunConfiguration *source) :
    LocalApplicationRunConfiguration(parent, source),
    m_uniqueProductName(source->m_uniqueProductName),
    m_currentInstallStep(0), // no need to copy this, we will get if from the DC anyway.
    m_currentBuildStepList(0) // ditto
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
    connect(project, &QbsProject::projectParsingStarted, this, &RunConfiguration::enabledChanged);
    connect(project, &QbsProject::projectParsingDone, this, [this](bool success) {
        auto terminalAspect = extraAspect<TerminalAspect>();
        if (success && !terminalAspect->isUserSet())
            terminalAspect->setUseTerminal(isConsoleApplication());
        emit enabledChanged();
    });

    connect(target(), &Target::activeDeployConfigurationChanged,
            this, &QbsRunConfiguration::installStepChanged);
    installStepChanged();
}

QWidget *QbsRunConfiguration::createConfigurationWidget()
{
    return new QbsRunConfigurationWidget(this, 0);
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
        connect(m_currentBuildStepList, SIGNAL(aboutToRemoveStep(int)), this,
                SLOT(installStepToBeRemoved(int)));
        connect(m_currentBuildStepList, SIGNAL(stepRemoved(int)), this, SLOT(installStepChanged()));
        connect(m_currentBuildStepList, SIGNAL(stepMoved(int,int)), this, SLOT(installStepChanged()));
    }

    emit targetInformationChanged();
}

void QbsRunConfiguration::installStepToBeRemoved(int pos)
{
    QTC_ASSERT(m_currentBuildStepList, return);
    // TODO: Our logic is rather broken. Users can create as many qbs install steps as they want,
    // but we ignore all but the first one.
    if (m_currentBuildStepList->steps().at(pos) != m_currentInstallStep)
        return;
    disconnect(m_currentInstallStep, SIGNAL(changed()), this, SIGNAL(targetInformationChanged()));
    m_currentInstallStep = 0;
}

QString QbsRunConfiguration::executable() const
{
    QbsProject *pro = static_cast<QbsProject *>(target()->project());
    const qbs::ProductData product = findProduct(pro->qbsProjectData(), m_uniqueProductName);

    if (!product.isValid() || !pro->qbsProject().isValid())
        return QString();

    return pro->qbsProject().targetExecutable(product, installOptions());
}

ApplicationLauncher::Mode QbsRunConfiguration::runMode() const
{
    return extraAspect<TerminalAspect>()->runMode();
}

bool QbsRunConfiguration::isConsoleApplication() const
{
    QbsProject *pro = static_cast<QbsProject *>(target()->project());
    const qbs::ProductData product = findProduct(pro->qbsProjectData(), m_uniqueProductName);
    return product.properties().value(QLatin1String("consoleApplication"), false).toBool();
}

QString QbsRunConfiguration::workingDirectory() const
{
    EnvironmentAspect *aspect = extraAspect<EnvironmentAspect>();
    QTC_ASSERT(aspect, return baseWorkingDirectory());
    return QDir::cleanPath(aspect->environment().expandVariables(
                macroExpander()->expand(baseWorkingDirectory())));
}

QString QbsRunConfiguration::baseWorkingDirectory() const
{
    WorkingDirectoryAspect *aspect = extraAspect<WorkingDirectoryAspect>();
    // if the user overrode us, then return his working directory
    QString wd = aspect->unexpandedWorkingDirectory();
    if (!wd.isEmpty())
        return wd;

    // else what the pro file reader tells us
    const QString exe = executable();
    if (!exe.isEmpty())
        return QFileInfo(executable()).absolutePath();
    return QString();
}

QString QbsRunConfiguration::commandLineArguments() const
{
    return extraAspect<ArgumentsAspect>()->arguments();
}

void QbsRunConfiguration::setBaseWorkingDirectory(const QString &wd)
{
    WorkingDirectoryAspect *aspect = extraAspect<WorkingDirectoryAspect>();
    const QString &oldWorkingDirectory = workingDirectory();

    aspect->setWorkingDirectory(wd);

    const QString &newWorkingDirectory = workingDirectory();
    if (oldWorkingDirectory != newWorkingDirectory)
        emit baseWorkingDirectoryChanged(newWorkingDirectory);
}

void QbsRunConfiguration::setRunMode(ApplicationLauncher::Mode runMode)
{
    extraAspect<TerminalAspect>()->setRunMode(runMode);
}

void QbsRunConfiguration::addToBaseEnvironment(Utils::Environment &env) const
{
    QbsProject *project = static_cast<QbsProject *>(target()->project());
    if (project) {
        const qbs::ProductData product = findProduct(project->qbsProjectData(), m_uniqueProductName);
        if (product.isValid()) {
            QProcessEnvironment procEnv = env.toProcessEnvironment();
            procEnv.insert(QLatin1String("QBS_RUN_FILE_PATH"), executable());
            qbs::RunEnvironment qbsRunEnv = project->qbsProject().getRunEnvironment(product, installOptions(),
                    procEnv, QbsManager::settings());
            procEnv = qbsRunEnv.runEnvironment();
            if (!procEnv.isEmpty()) {
                env = Utils::Environment();
                foreach (const QString &key, procEnv.keys())
                    env.set(key, procEnv.value(key));
            }
        }
    }

    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (qtVersion)
        env.prependOrSetLibrarySearchPath(qtVersion->qmakeProperty("QT_INSTALL_LIBS"));
}

QString QbsRunConfiguration::uniqueProductName() const
{
    return m_uniqueProductName;
}

QString QbsRunConfiguration::defaultDisplayName()
{
    QString defaultName = productDisplayNameFromId(id());
    if (defaultName.isEmpty())
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
    m_disabledIcon->setPixmap(QPixmap(QLatin1String(Core::Constants::ICON_WARNING)));
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

    m_executableLineLabel = new QLabel(this);
    m_executableLineLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    setExecutableLineText();
    toplayout->addRow(tr("Executable:"), m_executableLineLabel);

    m_rc->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, toplayout);
    m_rc->extraAspect<WorkingDirectoryAspect>()->addToMainConfigurationWidget(this, toplayout);

    m_rc->extraAspect<TerminalAspect>()->addToMainConfigurationWidget(this, toplayout);

    runConfigurationEnabledChange();

    connect(m_rc, SIGNAL(targetInformationChanged()),
            this, SLOT(targetInformationHasChanged()), Qt::QueuedConnection);

    connect(m_rc, SIGNAL(enabledChanged()),
            this, SLOT(runConfigurationEnabledChange()));
}

void QbsRunConfigurationWidget::runConfigurationEnabledChange()
{
    bool enabled = m_rc->isEnabled();
    m_disabledIcon->setVisible(!enabled);
    m_disabledReason->setVisible(!enabled);
    m_disabledReason->setText(m_rc->disabledReason());

    targetInformationHasChanged();
}

void QbsRunConfigurationWidget::targetInformationHasChanged()
{
    m_ignoreChange = true;
    setExecutableLineText(m_rc->executable());

    WorkingDirectoryAspect *aspect = m_rc->extraAspect<WorkingDirectoryAspect>();
    aspect->pathChooser()->setPath(m_rc->baseWorkingDirectory());
    aspect->pathChooser()->setBaseFileName(m_rc->target()->project()->projectDirectory());
    m_ignoreChange = false;
}

void QbsRunConfigurationWidget::setExecutableLineText(const QString &text)
{
    const QString newText = text.isEmpty() ? tr("<unknown>") : text;
    m_executableLineLabel->setText(newText);
}

// --------------------------------------------------------------------
// QbsRunConfigurationFactory:
// --------------------------------------------------------------------

QbsRunConfigurationFactory::QbsRunConfigurationFactory(QObject *parent) :
    IRunConfigurationFactory(parent)
{
    setObjectName(QLatin1String("QbsRunConfigurationFactory"));
}

QbsRunConfigurationFactory::~QbsRunConfigurationFactory()
{ }

bool QbsRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;

    QbsProject *project = static_cast<QbsProject *>(parent->project());
    return findProduct(project->qbsProjectData(), uniqueProductNameFromId(id)).isValid();
}

RunConfiguration *QbsRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    return new QbsRunConfiguration(parent, id);
}

bool QbsRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return idFromMap(map).toString().startsWith(QLatin1String(QBS_RC_PREFIX));
}

RunConfiguration *QbsRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    return new QbsRunConfiguration(parent, idFromMap(map));
}

bool QbsRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

RunConfiguration *QbsRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    QbsRunConfiguration *old = static_cast<QbsRunConfiguration *>(source);
    return new QbsRunConfiguration(parent, old);
}

QList<Core::Id> QbsRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode)
    QList<Core::Id> result;
    if (!canHandle(parent))
        return result;

    QbsProject *project = static_cast<QbsProject *>(parent->project());
    if (!project || !project->qbsProject().isValid())
        return result;

    foreach (const qbs::ProductData &product, project->qbsProjectData().allProducts()) {
        if (product.isRunnable() && product.isEnabled())
            result << idFromProduct(project, product);
    }

    return result;
}

QString QbsRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    return productDisplayNameFromId(id);
}

bool QbsRunConfigurationFactory::canHandle(Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    if (!qobject_cast<QbsProject *>(t->project()))
        return false;
    Core::Id devType = DeviceTypeKitInformation::deviceTypeId(t->kit());
    return devType == Constants::DESKTOP_DEVICE_TYPE;
}

} // namespace Internal
} // namespace QbsProjectManager
