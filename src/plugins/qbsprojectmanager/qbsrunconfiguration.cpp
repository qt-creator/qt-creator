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

#include "qbsrunconfiguration.h"

#include "qbsdeployconfigurationfactory.h"
#include "qbsinstallstep.h"
#include "qbsproject.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/variablechooser.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <utils/algorithm.h>
#include <utils/qtcprocess.h>
#include <utils/pathchooser.h>
#include <utils/detailswidget.h>
#include <utils/stringutils.h>
#include <utils/persistentsettings.h>
#include <utils/utilsicons.h>
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
using namespace Utils;

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

QbsRunConfiguration::QbsRunConfiguration(Target *target)
    : RunConfiguration(target)
{
    auto envAspect = new LocalEnvironmentAspect(this,
            [](RunConfiguration *rc, Environment &env) {
                static_cast<QbsRunConfiguration *>(rc)->addToBaseEnvironment(env);
            });
    addExtraAspect(envAspect);
    connect(static_cast<QbsProject *>(target->project()), &Project::parsingFinished, this,
            [envAspect]() { envAspect->buildEnvironmentHasChanged(); });
    addExtraAspect(new ArgumentsAspect(this, "Qbs.RunConfiguration.CommandLineArguments"));
    addExtraAspect(new WorkingDirectoryAspect(this, "Qbs.RunConfiguration.WorkingDirectory"));

    addExtraAspect(new TerminalAspect(this, "Qbs.RunConfiguration.UseTerminal", isConsoleApplication()));

    QbsProject *project = static_cast<QbsProject *>(target->project());
    connect(project, &Project::parsingFinished, this, [this](bool success) {
        auto terminalAspect = extraAspect<TerminalAspect>();
        if (success && !terminalAspect->isUserSet())
            terminalAspect->setUseTerminal(isConsoleApplication());
    });
    connect(BuildManager::instance(), &BuildManager::buildStateChanged, this,
            [this, project](Project *p) {
                if (p == project && !BuildManager::isBuilding(p))
                    emit enabledChanged();
            }
    );

    connect(target, &Target::activeDeployConfigurationChanged,
            this, &QbsRunConfiguration::installStepChanged);
}

void QbsRunConfiguration::initialize(Core::Id id)
{
    RunConfiguration::initialize(id);
    m_uniqueProductName = uniqueProductNameFromId(id);
    ctor();
}

void QbsRunConfiguration::copyFrom(const QbsRunConfiguration *source)
{
    RunConfiguration::copyFrom(source);
    m_uniqueProductName = source->m_uniqueProductName;
    m_currentInstallStep = nullptr; // no need to copy this, we will get if from the DC anyway.
    m_currentBuildStepList = nullptr;  // ditto

    ctor();
}

void QbsRunConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());
    installStepChanged();
}

QWidget *QbsRunConfiguration::createConfigurationWidget()
{
    return new QbsRunConfigurationWidget(this);
}

void QbsRunConfiguration::installStepChanged()
{
    if (m_currentInstallStep) {
        disconnect(m_currentInstallStep, &QbsInstallStep::changed,
                   this, &QbsRunConfiguration::targetInformationChanged);
    }
    if (m_currentBuildStepList) {
        disconnect(m_currentBuildStepList, &BuildStepList::stepInserted,
                   this, &QbsRunConfiguration::installStepChanged);
        disconnect(m_currentBuildStepList, &BuildStepList::stepRemoved,
                   this, &QbsRunConfiguration::installStepChanged);
        disconnect(m_currentBuildStepList, &BuildStepList::stepMoved,
                   this, &QbsRunConfiguration::installStepChanged);
    }

    QbsDeployConfiguration *activeDc = qobject_cast<QbsDeployConfiguration *>(target()->activeDeployConfiguration());
    m_currentBuildStepList = activeDc ? activeDc->stepList() : 0;
    if (m_currentInstallStep) {
        connect(m_currentInstallStep, &QbsInstallStep::changed,
                this, &QbsRunConfiguration::targetInformationChanged);
    }
    if (m_currentBuildStepList) {
        connect(m_currentBuildStepList, &BuildStepList::stepInserted,
                this, &QbsRunConfiguration::installStepChanged);
        connect(m_currentBuildStepList, &BuildStepList::aboutToRemoveStep, this,
                &QbsRunConfiguration::installStepToBeRemoved);
        connect(m_currentBuildStepList, &BuildStepList::stepRemoved,
                this, &QbsRunConfiguration::installStepChanged);
        connect(m_currentBuildStepList, &BuildStepList::stepMoved,
                this, &QbsRunConfiguration::installStepChanged);
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
    disconnect(m_currentInstallStep, &QbsInstallStep::changed,
               this, &QbsRunConfiguration::targetInformationChanged);
    m_currentInstallStep = 0;
}

Runnable QbsRunConfiguration::runnable() const
{
    StandardRunnable r;
    r.executable = executable();
    r.workingDirectory = extraAspect<WorkingDirectoryAspect>()->workingDirectory().toString();
    r.commandLineArguments = extraAspect<ArgumentsAspect>()->arguments();
    r.runMode = extraAspect<TerminalAspect>()->runMode();
    r.environment = extraAspect<LocalEnvironmentAspect>()->environment();
    return r;
}

QString QbsRunConfiguration::executable() const
{
    QbsProject *pro = static_cast<QbsProject *>(target()->project());
    const qbs::ProductData product = findProduct(pro->qbsProjectData(), m_uniqueProductName);

    if (!product.isValid() || !pro->qbsProject().isValid())
        return QString();

    return product.targetExecutable();
}

bool QbsRunConfiguration::isConsoleApplication() const
{
    QbsProject *pro = static_cast<QbsProject *>(target()->project());
    const qbs::ProductData product = findProduct(pro->qbsProjectData(), m_uniqueProductName);
    return product.properties().value(QLatin1String("consoleApplication"), false).toBool();
}

QString QbsRunConfiguration::baseWorkingDirectory() const
{
    const QString exe = executable();
    if (!exe.isEmpty())
        return QFileInfo(exe).absolutePath();
    return QString();
}

void QbsRunConfiguration::addToBaseEnvironment(Utils::Environment &env) const
{
    QbsProject *project = static_cast<QbsProject *>(target()->project());
    if (project && project->qbsProject().isValid()) {
        const qbs::ProductData product = findProduct(project->qbsProjectData(), m_uniqueProductName);
        if (product.isValid()) {
            QProcessEnvironment procEnv = env.toProcessEnvironment();
            procEnv.insert(QLatin1String("QBS_RUN_FILE_PATH"), executable());
            qbs::RunEnvironment qbsRunEnv = project->qbsProject().getRunEnvironment(product,
                    qbs::InstallOptions(), procEnv, QbsManager::settings());
            qbs::ErrorInfo error;
            procEnv = qbsRunEnv.runEnvironment(&error);
            if (error.hasError()) {
                Core::MessageManager::write(tr("Error retrieving run environment: %1")
                                            .arg(error.toString()));
            }
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

QString QbsRunConfiguration::buildSystemTarget() const
{
    return productDisplayNameFromId(id());
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

Utils::OutputFormatter *QbsRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

// --------------------------------------------------------------------
// QbsRunConfigurationWidget:
// --------------------------------------------------------------------

QbsRunConfigurationWidget::QbsRunConfigurationWidget(QbsRunConfiguration *rc)
    : m_rc(rc)
{
    auto vboxTopLayout = new QVBoxLayout(this);
    vboxTopLayout->setMargin(0);

    auto detailsContainer = new Utils::DetailsWidget(this);
    detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vboxTopLayout->addWidget(detailsContainer);
    auto detailsWidget = new QWidget(detailsContainer);
    detailsContainer->setWidget(detailsWidget);
    auto toplayout = new QFormLayout(detailsWidget);
    toplayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    toplayout->setMargin(0);

    m_executableLineLabel = new QLabel(this);
    m_executableLineLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    setExecutableLineText();
    toplayout->addRow(tr("Executable:"), m_executableLineLabel);

    m_rc->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, toplayout);
    m_rc->extraAspect<WorkingDirectoryAspect>()->addToMainConfigurationWidget(this, toplayout);

    m_rc->extraAspect<TerminalAspect>()->addToMainConfigurationWidget(this, toplayout);

    connect(m_rc, &QbsRunConfiguration::targetInformationChanged,
            this, &QbsRunConfigurationWidget::targetInformationHasChanged, Qt::QueuedConnection);

    connect(m_rc, &RunConfiguration::enabledChanged,
            this, &QbsRunConfigurationWidget::targetInformationHasChanged);
    targetInformationHasChanged();

    Core::VariableChooser::addSupportForChildWidgets(this, rc->macroExpander());
}

void QbsRunConfigurationWidget::targetInformationHasChanged()
{
    m_ignoreChange = true;
    setExecutableLineText(m_rc->executable());

    WorkingDirectoryAspect *aspect = m_rc->extraAspect<WorkingDirectoryAspect>();
    aspect->setDefaultWorkingDirectory(Utils::FileName::fromString(m_rc->baseWorkingDirectory()));
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

bool QbsRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;

    QbsProject *project = static_cast<QbsProject *>(parent->project());
    return findProduct(project->qbsProjectData(), uniqueProductNameFromId(id)).isValid();
}

RunConfiguration *QbsRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    return createHelper<QbsRunConfiguration>(parent, id);
}

bool QbsRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return idFromMap(map).toString().startsWith(QLatin1String(QBS_RC_PREFIX));
}

RunConfiguration *QbsRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    return createHelper<QbsRunConfiguration>(parent, idFromMap(map));
}

bool QbsRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

RunConfiguration *QbsRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return cloneHelper<QbsRunConfiguration>(parent, source);
}

QList<Core::Id> QbsRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    QList<qbs::ProductData> products;

    if (!canHandle(parent))
        return QList<Core::Id>();

    QbsProject *project = static_cast<QbsProject *>(parent->project());
    if (!project || !project->qbsProject().isValid())
        return QList<Core::Id>();

    foreach (const qbs::ProductData &product, project->qbsProjectData().allProducts()) {
        if (product.isRunnable() && product.isEnabled())
            products << product;
    }

    if (mode == AutoCreate) {
        std::function<bool (const qbs::ProductData &)> hasQtcRunnable = [](const qbs::ProductData &product) {
            return product.properties().value("qtcRunnable").toBool();
        };

        if (Utils::anyOf(products, hasQtcRunnable))
            Utils::erase(products, std::not1(hasQtcRunnable));
    }

    return Utils::transform(products, [project](const qbs::ProductData &product) {
        return idFromProduct(project, product);
    });
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
