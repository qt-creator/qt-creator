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

#include "qmlprojectrunconfiguration.h"
#include "qmlproject.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlmainfileaspect.h"
#include "qmlmultilanguageaspect.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/projectconfigurationaspects.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcprocess.h>
#include <utils/winutils.h>

#include <qmljstools/qmljstoolsconstants.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace QmlProjectManager {
class QmlMultiLanguageAspect;
namespace Internal {

// QmlProjectRunConfiguration

class QmlProjectRunConfiguration final : public RunConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(QmlProjectManager::QmlProjectRunConfiguration)

public:
    QmlProjectRunConfiguration(Target *target, Utils::Id id);

private:
    Runnable runnable() const final;
    QString disabledReason() const final;
    bool isEnabled() const final;

    QString mainScript() const;
    Utils::FilePath qmlScenePath() const;
    QString commandLineArguments() const;

    BaseStringAspect *m_qmlViewerAspect = nullptr;
    QmlMainFileAspect *m_qmlMainFileAspect = nullptr;
    QmlMultiLanguageAspect *m_multiLanguageAspect = nullptr;
};

QmlProjectRunConfiguration::QmlProjectRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
{
    m_qmlViewerAspect = addAspect<BaseStringAspect>();
    m_qmlViewerAspect->setLabelText(tr("QML Viewer:"));
    m_qmlViewerAspect->setPlaceHolderText(commandLine().executable().toString());
    m_qmlViewerAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    m_qmlViewerAspect->setHistoryCompleter("QmlProjectManager.viewer.history");

    auto argumentAspect = addAspect<ArgumentsAspect>();
    argumentAspect->setSettingsKey(Constants::QML_VIEWER_ARGUMENTS_KEY);

    setCommandLineGetter([this] {
        return CommandLine(qmlScenePath(), commandLineArguments(), CommandLine::Raw);
    });

    m_qmlMainFileAspect = addAspect<QmlMainFileAspect>(target);
    connect(m_qmlMainFileAspect, &QmlMainFileAspect::changed, this, &RunConfiguration::update);

    connect(target, &Target::kitChanged, this, &RunConfiguration::update);

    m_multiLanguageAspect = addAspect<QmlMultiLanguageAspect>(target);

    auto envAspect = addAspect<EnvironmentAspect>();
    connect(m_multiLanguageAspect, &QmlMultiLanguageAspect::changed, envAspect, &EnvironmentAspect::environmentChanged);

    auto envModifier = [this](Environment env) {
        if (auto bs = dynamic_cast<const QmlBuildSystem *>(activeBuildSystem()))
            env.modify(bs->environment());

        if (m_multiLanguageAspect && m_multiLanguageAspect->value() && !m_multiLanguageAspect->databaseFilePath().isEmpty()) {
            env.set("QT_MULTILANGUAGE_DATABASE", m_multiLanguageAspect->databaseFilePath().toString());
            env.set("QT_MULTILANGUAGE_LANGUAGE", m_multiLanguageAspect->lastUsedLanguage());
        }
        return env;
    };

    const Id deviceTypeId = DeviceTypeKitAspect::deviceTypeId(target->kit());
    if (deviceTypeId == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        envAspect->addPreferredBaseEnvironment(tr("System Environment"), [envModifier] {
            return envModifier(Environment::systemEnvironment());
        });
    }

    envAspect->addSupportedBaseEnvironment(tr("Clean Environment"), [envModifier] {
        Environment environment;
        return envModifier(environment);
    });

    setDisplayName(tr("QML Scene", "QMLRunConfiguration display name."));
    update();
}

Runnable QmlProjectRunConfiguration::runnable() const
{
    Runnable r;
    r.setCommandLine(commandLine());
    r.environment = aspect<EnvironmentAspect>()->environment();
    const QmlBuildSystem *bs = static_cast<QmlBuildSystem *>(activeBuildSystem());
    r.workingDirectory = bs->targetDirectory().toString();
    return r;
}

QString QmlProjectRunConfiguration::disabledReason() const
{
    if (mainScript().isEmpty())
        return tr("No script file to execute.");

    const FilePath viewer = qmlScenePath();
    if (DeviceTypeKitAspect::deviceTypeId(target()->kit())
            == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE
            && !viewer.exists()) {
        return tr("No qmlscene found.");
    }
    if (viewer.isEmpty())
        return tr("No qmlscene binary specified for target device.");
    return RunConfiguration::disabledReason();
}

FilePath QmlProjectRunConfiguration::qmlScenePath() const
{
    const QString qmlViewer = m_qmlViewerAspect->value();
    if (!qmlViewer.isEmpty())
        return FilePath::fromString(qmlViewer);

    Kit *kit = target()->kit();
    BaseQtVersion *version = QtKitAspect::qtVersion(kit);
    if (!version) // No Qt version in Kit. Don't try to run qmlscene.
        return {};

    const Id deviceType = DeviceTypeKitAspect::deviceTypeId(kit);
    if (deviceType == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        // If not given explicitly by Qt Version, try to pick it from $PATH.
        const bool isDesktop = version->type() == QtSupport::Constants::DESKTOPQT;
        return FilePath::fromString(isDesktop ? version->qmlsceneCommand() : QString("qmlscene"));
    }

    IDevice::ConstPtr dev = DeviceKitAspect::device(kit);
    if (dev.isNull()) // No device set. We don't know where to run qmlscene.
        return {};

    const QString qmlscene = dev->qmlsceneCommand();
    // If not given explicitly by device, try to pick it from $PATH.
    return FilePath::fromString(qmlscene.isEmpty() ? QString("qmlscene") : qmlscene);
}

QString QmlProjectRunConfiguration::commandLineArguments() const
{
    // arguments in .user file
    QString args = aspect<ArgumentsAspect>()->arguments(macroExpander());
    const IDevice::ConstPtr device = DeviceKitAspect::device(target()->kit());
    const OsType osType = device ? device->osType() : HostOsInfo::hostOs();

    // arguments from .qmlproject file
    const QmlBuildSystem *bs = qobject_cast<QmlBuildSystem *>(target()->buildSystem());
    foreach (const QString &importPath,
             QmlBuildSystem::makeAbsolute(bs->targetDirectory(), bs->customImportPaths())) {
        QtcProcess::addArg(&args, "-I", osType);
        QtcProcess::addArg(&args, importPath, osType);
    }

    for (const QString &fileSelector : bs->customFileSelectors()) {
        QtcProcess::addArg(&args, "-S", osType);
        QtcProcess::addArg(&args, fileSelector, osType);
    }

    if (Utils::HostOsInfo::isWindowsHost() && bs->forceFreeType()) {
        Utils::QtcProcess::addArg(&args, "-platform", osType);
        Utils::QtcProcess::addArg(&args, "windows:fontengine=freetype", osType);
    }

    const QString main = bs->targetFile(FilePath::fromString(mainScript())).toString();
    if (!main.isEmpty())
        QtcProcess::addArg(&args, main, osType);

    if (m_multiLanguageAspect && m_multiLanguageAspect->value())
        QtcProcess::addArg(&args, "-qmljsdebugger=file:unused_if_debugger_arguments_added,services:DebugTranslation", osType);

    return args;
}

bool QmlProjectRunConfiguration::isEnabled() const
{
    if (m_qmlMainFileAspect->isQmlFilePresent() && !commandLine().executable().isEmpty()) {
        BuildSystem *bs = activeBuildSystem();
        return !bs->isParsing() && bs->hasParsingData();
    }
    return false;
}

QString QmlProjectRunConfiguration::mainScript() const
{
    return m_qmlMainFileAspect->mainScript();
}

// QmlProjectRunConfigurationFactory

QmlProjectRunConfigurationFactory::QmlProjectRunConfigurationFactory()
    : FixedRunConfigurationFactory(QmlProjectRunConfiguration::tr("QML Scene"), false)
{
    registerRunConfiguration<QmlProjectRunConfiguration>
            ("QmlProjectManager.QmlRunConfiguration.QmlScene");
    addSupportedProjectType(QmlProjectManager::Constants::QML_PROJECT_ID);
}

} // namespace Internal
} // namespace QmlProjectManager
