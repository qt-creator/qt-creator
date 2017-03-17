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

#include "desktopqmakerunconfiguration.h"

#include "qmakenodes.h"
#include "qmakeproject.h"
#include "qmakebuildconfiguration.h"

#include <coreplugin/variablechooser.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/detailswidget.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

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
const char PRO_FILE_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.ProFile";
const char USE_DYLD_IMAGE_SUFFIX_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.UseDyldImageSuffix";
const char USE_LIBRARY_SEARCH_PATH[] = "QmakeProjectManager.QmakeRunConfiguration.UseLibrarySearchPath";

static Utils::FileName pathFromId(Core::Id id)
{
    return Utils::FileName::fromString(id.suffixAfter(QMAKE_RC_PREFIX));
}

//
// QmakeRunConfiguration
//

DesktopQmakeRunConfiguration::DesktopQmakeRunConfiguration(Target *parent, Core::Id id) :
    RunConfiguration(parent, id),
    m_proFilePath(pathFromId(id))
{
    addExtraAspect(new LocalEnvironmentAspect(this, [](RunConfiguration *rc, Environment &env) {
                       static_cast<DesktopQmakeRunConfiguration *>(rc)->addToBaseEnvironment(env);
                   }));
    addExtraAspect(new ArgumentsAspect(this, QStringLiteral("Qt4ProjectManager.Qt4RunConfiguration.CommandLineArguments")));
    addExtraAspect(new TerminalAspect(this, QStringLiteral("Qt4ProjectManager.Qt4RunConfiguration.UseTerminal")));
    addExtraAspect(new WorkingDirectoryAspect(this,
                       QStringLiteral("Qt4ProjectManager.Qt4RunConfiguration.UserWorkingDirectory")));
    QmakeProject *project = qmakeProject();
    m_parseSuccess = project->validParse(m_proFilePath);
    m_parseInProgress = project->parseInProgress(m_proFilePath);

    ctor();
}

DesktopQmakeRunConfiguration::DesktopQmakeRunConfiguration(Target *parent, DesktopQmakeRunConfiguration *source) :
    RunConfiguration(parent, source),
    m_proFilePath(source->m_proFilePath),
    m_isUsingDyldImageSuffix(source->m_isUsingDyldImageSuffix),
    m_isUsingLibrarySearchPath(source->m_isUsingLibrarySearchPath),
    m_parseSuccess(source->m_parseSuccess),
    m_parseInProgress(source->m_parseInProgress)
{
    ctor();
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
        return qmakeProject()->disabledReasonForRunConfiguration(m_proFilePath);
    return QString();
}

void DesktopQmakeRunConfiguration::proFileUpdated(QmakeProFile *pro, bool success, bool parseInProgress)
{
    if (m_proFilePath != pro->filePath())
        return;
    const bool enabled = isEnabled();
    const QString reason = disabledReason();
    m_parseSuccess = success;
    m_parseInProgress = parseInProgress;
    if (enabled != isEnabled() || reason != disabledReason())
        emit enabledChanged();

    if (!parseInProgress) {
        emit effectiveTargetInformationChanged();
        setDefaultDisplayName(defaultDisplayName());
        extraAspect<LocalEnvironmentAspect>()->buildEnvironmentHasChanged();

        extraAspect<WorkingDirectoryAspect>()
                ->setDefaultWorkingDirectory(FileName::fromString(baseWorkingDirectory()));
        auto terminalAspect = extraAspect<TerminalAspect>();
        if (!terminalAspect->isUserSet())
            terminalAspect->setUseTerminal(isConsoleApplication());
    }
}

void DesktopQmakeRunConfiguration::proFileEvaluated()
{
    // We depend on all .pro files for the LD_LIBRARY_PATH so we emit a signal for all .pro files
    // This can be optimized by checking whether LD_LIBRARY_PATH changed
    return extraAspect<LocalEnvironmentAspect>()->buildEnvironmentHasChanged();
}

void DesktopQmakeRunConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());

    QmakeProject *project = qmakeProject();
    connect(project, &QmakeProject::proFileUpdated,
            this, &DesktopQmakeRunConfiguration::proFileUpdated);
    connect(project, &QmakeProject::proFilesEvaluated,
            this, &DesktopQmakeRunConfiguration::proFileEvaluated);
}

//////
/// DesktopQmakeRunConfigurationWidget
/////

DesktopQmakeRunConfigurationWidget::DesktopQmakeRunConfigurationWidget(DesktopQmakeRunConfiguration *qmakeRunConfiguration)
    :  m_qmakeRunConfiguration(qmakeRunConfiguration)
{
    auto vboxTopLayout = new QVBoxLayout(this);
    vboxTopLayout->setMargin(0);

    auto hl = new QHBoxLayout();
    hl->addStretch();
    m_disabledIcon = new QLabel(this);
    m_disabledIcon->setPixmap(Utils::Icons::WARNING.pixmap());
    hl->addWidget(m_disabledIcon);
    m_disabledReason = new QLabel(this);
    m_disabledReason->setVisible(false);
    hl->addWidget(m_disabledReason);
    hl->addStretch();
    vboxTopLayout->addLayout(hl);

    auto detailsContainer = new DetailsWidget(this);
    detailsContainer->setState(DetailsWidget::NoSummary);
    vboxTopLayout->addWidget(detailsContainer);
    auto detailsWidget = new QWidget(detailsContainer);
    detailsContainer->setWidget(detailsWidget);
    auto toplayout = new QFormLayout(detailsWidget);
    toplayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    toplayout->setMargin(0);

    m_executableLineLabel = new QLabel(this);
    m_executableLineLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    toplayout->addRow(tr("Executable:"), m_executableLineLabel);

    m_qmakeRunConfiguration->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, toplayout);
    m_qmakeRunConfiguration->extraAspect<WorkingDirectoryAspect>()->addToMainConfigurationWidget(this, toplayout);
    m_qmakeRunConfiguration->extraAspect<TerminalAspect>()->addToMainConfigurationWidget(this, toplayout);

    m_useQvfbCheck = new QCheckBox(tr("Run on QVFb"), this);
    m_useQvfbCheck->setToolTip(tr("Check this option to run the application on a Qt Virtual Framebuffer."));
    m_useQvfbCheck->setChecked(m_qmakeRunConfiguration->runnable().as<StandardRunnable>().runMode
                               == ApplicationLauncher::Console);
    m_useQvfbCheck->setVisible(false);
    auto innerBox = new QHBoxLayout();
    innerBox->addWidget(m_useQvfbCheck);
    innerBox->addStretch();
    toplayout->addRow(QString(), innerBox);

    if (HostOsInfo::isMacHost()) {
        m_usingDyldImageSuffix = new QCheckBox(tr("Use debug version of frameworks (DYLD_IMAGE_SUFFIX=_debug)"), this);
        m_usingDyldImageSuffix->setChecked(m_qmakeRunConfiguration->isUsingDyldImageSuffix());
        toplayout->addRow(QString(), m_usingDyldImageSuffix);
        connect(m_usingDyldImageSuffix, &QAbstractButton::toggled,
                this, &DesktopQmakeRunConfigurationWidget::usingDyldImageSuffixToggled);
    }

    QString librarySeachPathLabel;
    if (HostOsInfo::isMacHost()) {
        librarySeachPathLabel
                = tr("Add build library search path to DYLD_LIBRARY_PATH and DYLD_FRAMEWORK_PATH");
    } else if (HostOsInfo::isWindowsHost()) {
        librarySeachPathLabel
                = tr("Add build library search path to PATH");
    } else if (HostOsInfo::isLinuxHost() || HostOsInfo::isAnyUnixHost()) {
        librarySeachPathLabel
                = tr("Add build library search path to LD_LIBRARY_PATH");
    }

    if (!librarySeachPathLabel.isEmpty()) {
        m_usingLibrarySearchPath = new QCheckBox(librarySeachPathLabel);
        m_usingLibrarySearchPath->setChecked(m_qmakeRunConfiguration->isUsingLibrarySearchPath());
        toplayout->addRow(QString(), m_usingLibrarySearchPath);
        connect(m_usingLibrarySearchPath, &QCheckBox::toggled,
                this, &DesktopQmakeRunConfigurationWidget::usingLibrarySearchPathToggled);
    }

    runConfigurationEnabledChange();
    effectiveTargetInformationChanged();

    connect(qmakeRunConfiguration, &DesktopQmakeRunConfiguration::usingDyldImageSuffixChanged,
            this, &DesktopQmakeRunConfigurationWidget::usingDyldImageSuffixChanged);
    connect(qmakeRunConfiguration, &DesktopQmakeRunConfiguration::usingLibrarySearchPathChanged,
            this, &DesktopQmakeRunConfigurationWidget::usingLibrarySearchPathChanged);
    connect(qmakeRunConfiguration, &DesktopQmakeRunConfiguration::effectiveTargetInformationChanged,
            this, &DesktopQmakeRunConfigurationWidget::effectiveTargetInformationChanged, Qt::QueuedConnection);

    connect(qmakeRunConfiguration, &RunConfiguration::enabledChanged,
            this, &DesktopQmakeRunConfigurationWidget::runConfigurationEnabledChange);

    Core::VariableChooser::addSupportForChildWidgets(this, m_qmakeRunConfiguration->macroExpander());
}

void DesktopQmakeRunConfigurationWidget::runConfigurationEnabledChange()
{
    bool enabled = m_qmakeRunConfiguration->isEnabled();
    m_disabledIcon->setVisible(!enabled);
    m_disabledReason->setVisible(!enabled);
    m_disabledReason->setText(m_qmakeRunConfiguration->disabledReason());
}

void DesktopQmakeRunConfigurationWidget::usingDyldImageSuffixToggled(bool state)
{
    m_ignoreChange = true;
    m_qmakeRunConfiguration->setUsingDyldImageSuffix(state);
    m_ignoreChange = false;
}

void DesktopQmakeRunConfigurationWidget::usingLibrarySearchPathToggled(bool state)
{
    m_ignoreChange = true;
    m_qmakeRunConfiguration->setUsingLibrarySearchPath(state);
    m_ignoreChange = false;
}

void DesktopQmakeRunConfigurationWidget::usingDyldImageSuffixChanged(bool state)
{
    if (!m_ignoreChange && m_usingDyldImageSuffix)
        m_usingDyldImageSuffix->setChecked(state);
}

void DesktopQmakeRunConfigurationWidget::usingLibrarySearchPathChanged(bool state)
{
    if (!m_ignoreChange && m_usingLibrarySearchPath)
        m_usingLibrarySearchPath->setChecked(state);
}

void DesktopQmakeRunConfigurationWidget::effectiveTargetInformationChanged()
{
    m_executableLineLabel->setText(QDir::toNativeSeparators(m_qmakeRunConfiguration->executable()));

    m_ignoreChange = true;
    auto aspect = m_qmakeRunConfiguration->extraAspect<WorkingDirectoryAspect>();
    aspect->setDefaultWorkingDirectory(FileName::fromString(m_qmakeRunConfiguration->baseWorkingDirectory()));
    aspect->pathChooser()->setBaseFileName(m_qmakeRunConfiguration->target()->project()->projectDirectory());
    auto terminalAspect = m_qmakeRunConfiguration->extraAspect<TerminalAspect>();
    if (!terminalAspect->isUserSet())
        terminalAspect->setUseTerminal(m_qmakeRunConfiguration->isConsoleApplication());
    m_ignoreChange = false;
}

QWidget *DesktopQmakeRunConfiguration::createConfigurationWidget()
{
    return new DesktopQmakeRunConfigurationWidget(this);
}

Runnable DesktopQmakeRunConfiguration::runnable() const
{
    StandardRunnable r;
    r.executable = executable();
    r.commandLineArguments = extraAspect<ArgumentsAspect>()->arguments();
    r.workingDirectory = extraAspect<WorkingDirectoryAspect>()->workingDirectory().toString();
    r.environment = extraAspect<LocalEnvironmentAspect>()->environment();
    r.runMode = extraAspect<TerminalAspect>()->runMode();
    return r;
}

QVariantMap DesktopQmakeRunConfiguration::toMap() const
{
    const QDir projectDir = QDir(target()->project()->projectDirectory().toString());
    QVariantMap map(RunConfiguration::toMap());
    map.insert(QLatin1String(PRO_FILE_KEY), projectDir.relativeFilePath(m_proFilePath.toString()));
    map.insert(QLatin1String(USE_DYLD_IMAGE_SUFFIX_KEY), m_isUsingDyldImageSuffix);
    map.insert(QLatin1String(USE_LIBRARY_SEARCH_PATH), m_isUsingLibrarySearchPath);
    return map;
}

bool DesktopQmakeRunConfiguration::fromMap(const QVariantMap &map)
{
    const QDir projectDir = QDir(target()->project()->projectDirectory().toString());
    m_proFilePath = Utils::FileName::fromUserInput(projectDir.filePath(map.value(QLatin1String(PRO_FILE_KEY)).toString()));
    m_isUsingDyldImageSuffix = map.value(QLatin1String(USE_DYLD_IMAGE_SUFFIX_KEY), false).toBool();
    m_isUsingLibrarySearchPath = map.value(QLatin1String(USE_LIBRARY_SEARCH_PATH), true).toBool();

    m_parseSuccess = qmakeProject()->validParse(m_proFilePath);
    m_parseInProgress = qmakeProject()->parseInProgress(m_proFilePath);

    return RunConfiguration::fromMap(map);
}

QString DesktopQmakeRunConfiguration::executable() const
{
    if (QmakeProFileNode *node = projectNode())
        return extractWorkingDirAndExecutable(node).second;
    return QString();
}

bool DesktopQmakeRunConfiguration::isUsingDyldImageSuffix() const
{
    return m_isUsingDyldImageSuffix;
}

void DesktopQmakeRunConfiguration::setUsingDyldImageSuffix(bool state)
{
    m_isUsingDyldImageSuffix = state;
    emit usingDyldImageSuffixChanged(state);

    return extraAspect<LocalEnvironmentAspect>()->environmentChanged();
}

bool DesktopQmakeRunConfiguration::isUsingLibrarySearchPath() const
{
    return m_isUsingLibrarySearchPath;
}

void DesktopQmakeRunConfiguration::setUsingLibrarySearchPath(bool state)
{
    m_isUsingLibrarySearchPath = state;
    emit usingLibrarySearchPathChanged(state);

    return extraAspect<LocalEnvironmentAspect>()->environmentChanged();
}

QString DesktopQmakeRunConfiguration::baseWorkingDirectory() const
{
    if (QmakeProFileNode *node = projectNode())
        return extractWorkingDirAndExecutable(node).first;
    return QString();
}

bool DesktopQmakeRunConfiguration::isConsoleApplication() const
{
    if (QmakeProFileNode *node = projectNode()) {
        const QStringList config = node->variableValue(Variable::Config);
        if (!config.contains("console") || config.contains("testcase"))
            return false;
        const QStringList qt = node->variableValue(Variable::Qt);
        return !qt.contains("testlib") && !qt.contains("qmltest");
    }
    return false;
}

void DesktopQmakeRunConfiguration::addToBaseEnvironment(Environment &env) const
{
    if (m_isUsingDyldImageSuffix)
        env.set(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));

    // The user could be linking to a library found via a -L/some/dir switch
    // to find those libraries while actually running we explicitly prepend those
    // dirs to the library search path
    const QmakeProFileNode *node = projectNode();
    if (m_isUsingLibrarySearchPath && node) {
        const QStringList libDirectories = node->variableValue(Variable::LibDirectories);
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
    if (qtVersion && m_isUsingLibrarySearchPath)
        env.prependOrSetLibrarySearchPath(qtVersion->qmakeProperty("QT_INSTALL_LIBS"));
}

QString DesktopQmakeRunConfiguration::buildSystemTarget() const
{
    return qmakeProject()->mapProFilePathToTarget(m_proFilePath);
}

Utils::FileName DesktopQmakeRunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

QmakeProject *DesktopQmakeRunConfiguration::qmakeProject() const
{
    return static_cast<QmakeProject *>(target()->project());
}

QmakeProFileNode *DesktopQmakeRunConfiguration::projectNode() const
{
    QmakeProject *project = qmakeProject();
    QTC_ASSERT(project, return nullptr);
    QmakeProFileNode *rootNode = project->rootProjectNode();
    if (!rootNode)
        return nullptr;
    return rootNode->findProFileFor(m_proFilePath);
}

QString DesktopQmakeRunConfiguration::defaultDisplayName()
{
    if (QmakeProFileNode *node = projectNode())
        return node->displayName();

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
        return { };

    QmakeProFile *pro = node->proFile();
    QTC_ASSERT(pro, return { });

    TargetInformation ti = pro->targetInformation();
    if (!ti.valid)
        return qMakePair(QString(), QString());

    const QStringList &config = pro->variableValue(Variable::Config);

    QString destDir = ti.destDir.toString();
    QString workingDir;
    if (!destDir.isEmpty()) {
        bool workingDirIsBaseDir = false;
        if (destDir == ti.buildTarget)
            workingDirIsBaseDir = true;
        if (QDir::isRelativePath(destDir))
            destDir = QDir::cleanPath(ti.buildDir.toString() + '/' + destDir);

        if (workingDirIsBaseDir)
            workingDir = ti.buildDir.toString();
        else
            workingDir = destDir;
    } else {
        destDir = ti.buildDir.toString();
        workingDir = ti.buildDir.toString();
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
{
    setObjectName(QLatin1String("DesktopQmakeRunConfigurationFactory"));
}

bool DesktopQmakeRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    QmakeProject *project = static_cast<QmakeProject *>(parent->project());
    return project->hasApplicationProFile(pathFromId(id));
}

RunConfiguration *DesktopQmakeRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    return new DesktopQmakeRunConfiguration(parent, id);
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
    return project->creationIds(QMAKE_RC_PREFIX, mode);
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
        if (DesktopQmakeRunConfiguration *qmakeRc = qobject_cast<DesktopQmakeRunConfiguration *>(rc))
            if (qmakeRc->proFilePath() == n->filePath())
                result << rc;
    return result;
}

} // namespace Internal
} // namespace QmakeProjectManager
