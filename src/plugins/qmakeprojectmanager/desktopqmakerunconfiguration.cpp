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

#include "qmakebuildconfiguration.h"
#include "qmakenodes.h"
#include "qmakeproject.h"
#include "qmakeprojectmanagerconstants.h"

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

//
// QmakeRunConfiguration
//

DesktopQmakeRunConfiguration::DesktopQmakeRunConfiguration(Target *target)
    : RunConfiguration(target, QMAKE_RC_PREFIX)
{
    addExtraAspect(new LocalEnvironmentAspect(this, [](RunConfiguration *rc, Environment &env) {
                       static_cast<DesktopQmakeRunConfiguration *>(rc)->addToBaseEnvironment(env);
                   }));
    addExtraAspect(new ArgumentsAspect(this, "Qt4ProjectManager.Qt4RunConfiguration.CommandLineArguments"));
    addExtraAspect(new TerminalAspect(this, "Qt4ProjectManager.Qt4RunConfiguration.UseTerminal"));
    addExtraAspect(new WorkingDirectoryAspect(this, "Qt4ProjectManager.Qt4RunConfiguration.UserWorkingDirectory"));

    connect(target->project(), &Project::parsingFinished,
            this, &DesktopQmakeRunConfiguration::updateTargetInformation);
}

QString DesktopQmakeRunConfiguration::extraId() const
{
    return m_proFilePath.toString();
}

void DesktopQmakeRunConfiguration::updateTargetInformation()
{
    setDefaultDisplayName(defaultDisplayName());
    extraAspect<LocalEnvironmentAspect>()->buildEnvironmentHasChanged();

    auto wda = extraAspect<WorkingDirectoryAspect>();

    wda->setDefaultWorkingDirectory(FileName::fromString(baseWorkingDirectory()));
    if (wda->pathChooser())
        wda->pathChooser()->setBaseFileName(target()->project()->projectDirectory());
    auto terminalAspect = extraAspect<TerminalAspect>();
    if (!terminalAspect->isUserSet())
        terminalAspect->setUseTerminal(isConsoleApplication());

    emit effectiveTargetInformationChanged();
}

//////
/// DesktopQmakeRunConfigurationWidget
/////

DesktopQmakeRunConfigurationWidget::DesktopQmakeRunConfigurationWidget(DesktopQmakeRunConfiguration *qmakeRunConfiguration)
    :  m_qmakeRunConfiguration(qmakeRunConfiguration)
{
    auto vboxTopLayout = new QVBoxLayout(this);
    vboxTopLayout->setMargin(0);

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

    connect(qmakeRunConfiguration, &DesktopQmakeRunConfiguration::usingDyldImageSuffixChanged,
            this, &DesktopQmakeRunConfigurationWidget::usingDyldImageSuffixChanged);
    connect(qmakeRunConfiguration, &DesktopQmakeRunConfiguration::usingLibrarySearchPathChanged,
            this, &DesktopQmakeRunConfigurationWidget::usingLibrarySearchPathChanged);
    connect(qmakeRunConfiguration, &DesktopQmakeRunConfiguration::effectiveTargetInformationChanged,
            this, &DesktopQmakeRunConfigurationWidget::effectiveTargetInformationChanged, Qt::QueuedConnection);

    Core::VariableChooser::addSupportForChildWidgets(this, m_qmakeRunConfiguration->macroExpander());
    effectiveTargetInformationChanged();
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

    QString extraId = ProjectExplorer::idFromMap(map).suffixAfter(id());
    if (!extraId.isEmpty())
        m_proFilePath = FileName::fromString(extraId);

    const bool res = RunConfiguration::fromMap(map);
    updateTargetInformation();
    return res;
}

QString DesktopQmakeRunConfiguration::executable() const
{
    if (QmakeProFile *pro = proFile())
        return extractWorkingDirAndExecutable(pro).second;
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
    if (QmakeProFile *pro = proFile())
        return extractWorkingDirAndExecutable(pro).first;
    return QString();
}

bool DesktopQmakeRunConfiguration::isConsoleApplication() const
{
    if (QmakeProFile *pro = proFile()) {
        const QStringList config = pro->variableValue(Variable::Config);
        if (!config.contains("console") || config.contains("testcase"))
            return false;
        const QStringList qt = pro->variableValue(Variable::Qt);
        return !qt.contains("testlib") && !qt.contains("qmltest");
    }
    return false;
}

void DesktopQmakeRunConfiguration::addToBaseEnvironment(Environment &env) const
{
    if (m_isUsingDyldImageSuffix)
        env.set(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));

    QStringList libraryPaths;

    // The user could be linking to a library found via a -L/some/dir switch
    // to find those libraries while actually running we explicitly prepend those
    // dirs to the library search path
    const QmakeProFile *pro = proFile();
    if (m_isUsingLibrarySearchPath && pro) {
        const QStringList libDirectories = pro->variableValue(Variable::LibDirectories);
        if (!libDirectories.isEmpty()) {
            const QString proDirectory = pro->buildDir().toString();
            foreach (QString dir, libDirectories) {
                // Fix up relative entries like "LIBS+=-L.."
                const QFileInfo fi(dir);
                if (!fi.isAbsolute())
                    dir = QDir::cleanPath(proDirectory + QLatin1Char('/') + dir);
                libraryPaths << dir;
            } // foreach
        } // libDirectories
    } // pro

    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (qtVersion && m_isUsingLibrarySearchPath)
        libraryPaths << qtVersion->librarySearchPath().toString();
    env.prependOrSetLibrarySearchPaths(libraryPaths);
}

QString DesktopQmakeRunConfiguration::buildSystemTarget() const
{
    return m_proFilePath.toString();
}

Utils::FileName DesktopQmakeRunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

QmakeProject *DesktopQmakeRunConfiguration::qmakeProject() const
{
    return static_cast<QmakeProject *>(target()->project());
}

QmakeProFile *DesktopQmakeRunConfiguration::proFile() const
{
    QmakeProject *project = qmakeProject();
    QTC_ASSERT(project, return nullptr);
    QmakeProFile *rootProFile = project->rootProFile();
    return rootProFile ? rootProFile->findProFile(m_proFilePath) : nullptr;
}

QString DesktopQmakeRunConfiguration::defaultDisplayName()
{
    if (QmakeProFile *pro = proFile())
        return pro->displayName();

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

QPair<QString, QString> DesktopQmakeRunConfiguration::extractWorkingDirAndExecutable(const QmakeProFile *proFile) const
{
    if (!proFile)
        return {};

    TargetInformation ti = proFile->targetInformation();
    if (!ti.valid)
        return qMakePair(QString(), QString());

    const QStringList &config = proFile->variableValue(Variable::Config);

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
    setObjectName("DesktopQmakeRunConfigurationFactory");
    registerRunConfiguration<DesktopQmakeRunConfiguration>(QMAKE_RC_PREFIX);
    addSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    setSupportedTargetDeviceTypes({ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE});
}

bool DesktopQmakeRunConfigurationFactory::canCreateHelper(Target *parent, const QString &buildTarget) const
{
    QmakeProject *project = static_cast<QmakeProject *>(parent->project());
    return project->hasApplicationProFile(Utils::FileName::fromString(buildTarget));
}

QList<RunConfigurationCreationInfo>
DesktopQmakeRunConfigurationFactory::availableCreators(Target *parent) const
{
    QmakeProject *project = static_cast<QmakeProject *>(parent->project());
    return project->runConfigurationCreators(this);
}

bool DesktopQmakeRunConfigurationFactory::hasRunConfigForProFile(RunConfiguration *rc, const Utils::FileName &n) const
{
    auto qmakeRc = qobject_cast<DesktopQmakeRunConfiguration *>(rc);
    return qmakeRc && qmakeRc->proFilePath() == n;
}

} // namespace Internal
} // namespace QmakeProjectManager
