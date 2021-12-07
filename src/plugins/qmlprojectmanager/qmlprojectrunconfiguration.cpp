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

#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/aspects.h>
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

static bool isQtDesignStudio()
{
    QSettings *settings = Core::ICore::settings();
    const QString qdsStandaloneEntry = "QML/Designer/StandAloneMode"; //entry from qml settings

    return settings->value(qdsStandaloneEntry, false).toBool();
}

class QmlProjectRunConfiguration final : public RunConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(QmlProjectManager::QmlProjectRunConfiguration)

public:
    QmlProjectRunConfiguration(Target *target, Id id);

private:
    QString disabledReason() const final;
    bool isEnabled() const final;

    QString mainScript() const;
    FilePath qmlRuntimeFilePath() const;
    QString commandLineArguments() const;
    void createQtVersionAspect();

    StringAspect *m_qmlViewerAspect = nullptr;
    QmlMainFileAspect *m_qmlMainFileAspect = nullptr;
    QmlMultiLanguageAspect *m_multiLanguageAspect = nullptr;
    SelectionAspect *m_qtversionAspect = nullptr;
};

QmlProjectRunConfiguration::QmlProjectRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
{
    m_qmlViewerAspect = addAspect<StringAspect>();
    m_qmlViewerAspect->setLabelText(tr("QML Viewer:"));
    m_qmlViewerAspect->setPlaceHolderText(commandLine().executable().toString());
    m_qmlViewerAspect->setDisplayStyle(StringAspect::LineEditDisplay);
    m_qmlViewerAspect->setHistoryCompleter("QmlProjectManager.viewer.history");

    auto argumentAspect = addAspect<ArgumentsAspect>();
    argumentAspect->setSettingsKey(Constants::QML_VIEWER_ARGUMENTS_KEY);

    setCommandLineGetter([this] {
        return CommandLine(qmlRuntimeFilePath(), commandLineArguments(), CommandLine::Raw);
    });

    m_qmlMainFileAspect = addAspect<QmlMainFileAspect>(target);
    connect(m_qmlMainFileAspect, &QmlMainFileAspect::changed, this, &RunConfiguration::update);

    createQtVersionAspect();

    connect(target, &Target::kitChanged, this, &RunConfiguration::update);

    m_multiLanguageAspect = addAspect<QmlMultiLanguageAspect>(target);

    auto envAspect = addAspect<EnvironmentAspect>();
    connect(m_multiLanguageAspect, &QmlMultiLanguageAspect::changed, envAspect, &EnvironmentAspect::environmentChanged);

    auto envModifier = [this](Environment env) {
        if (auto bs = dynamic_cast<const QmlBuildSystem *>(activeBuildSystem()))
            env.modify(bs->environment());

        if (m_multiLanguageAspect && m_multiLanguageAspect->value() && !m_multiLanguageAspect->databaseFilePath().isEmpty()) {
            env.set("QT_MULTILANGUAGE_DATABASE", m_multiLanguageAspect->databaseFilePath().toString());
            env.set("QT_MULTILANGUAGE_LANGUAGE", m_multiLanguageAspect->currentLocale());
        } else {
            env.unset("QT_MULTILANGUAGE_DATABASE");
            env.unset("QT_MULTILANGUAGE_LANGUAGE");
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

    setRunnableModifier([this](Runnable &r) {
        const QmlBuildSystem *bs = static_cast<QmlBuildSystem *>(activeBuildSystem());
        r.workingDirectory = bs->targetDirectory();
    });

    setDisplayName(tr("QML Utility", "QMLRunConfiguration display name."));
    update();
}

QString QmlProjectRunConfiguration::disabledReason() const
{
    if (mainScript().isEmpty())
        return tr("No script file to execute.");

    const FilePath viewer = qmlRuntimeFilePath();
    if (DeviceTypeKitAspect::deviceTypeId(kit())
            == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE
            && !viewer.exists()) {
        return tr("No QML utility found.");
    }
    if (viewer.isEmpty())
        return tr("No QML utility specified for target device.");
    return RunConfiguration::disabledReason();
}

FilePath QmlProjectRunConfiguration::qmlRuntimeFilePath() const
{
    const QString qmlViewer = m_qmlViewerAspect->value();
    if (!qmlViewer.isEmpty())
        return FilePath::fromString(qmlViewer);

    Kit *kit = target()->kit();
    BaseQtVersion *version = QtKitAspect::qtVersion(kit);
    if (!version) // No Qt version in Kit. Don't try to run QML runtime.
        return {};

    const Id deviceType = DeviceTypeKitAspect::deviceTypeId(kit);
    if (deviceType == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        // If not given explicitly by Qt Version, try to pick it from $PATH.
        const bool isDesktop = version->type() == QtSupport::Constants::DESKTOPQT;
        return isDesktop ? version->qmlRuntimeFilePath() : "qmlscene";
    }

    IDevice::ConstPtr dev = DeviceKitAspect::device(kit);
    if (dev.isNull()) // No device set. We don't know where a QML utility is.
        return {};

    const FilePath qmlRuntime = dev->qmlRunCommand();
    // If not given explicitly by device, try to pick it from $PATH.
    return qmlRuntime.isEmpty() ? "qmlscene" : qmlRuntime;
}

QString QmlProjectRunConfiguration::commandLineArguments() const
{
    // arguments in .user file
    QString args = aspect<ArgumentsAspect>()->arguments(macroExpander());
    const IDevice::ConstPtr device = DeviceKitAspect::device(kit());
    const OsType osType = device ? device->osType() : HostOsInfo::hostOs();

    // arguments from .qmlproject file
    const QmlBuildSystem *bs = qobject_cast<QmlBuildSystem *>(target()->buildSystem());
    foreach (const QString &importPath,
             QmlBuildSystem::makeAbsolute(bs->targetDirectory(), bs->customImportPaths())) {
        ProcessArgs::addArg(&args, "-I", osType);
        ProcessArgs::addArg(&args, importPath, osType);
    }

    for (const QString &fileSelector : bs->customFileSelectors()) {
        ProcessArgs::addArg(&args, "-S", osType);
        ProcessArgs::addArg(&args, fileSelector, osType);
    }

    if (HostOsInfo::isWindowsHost() && bs->forceFreeType()) {
        ProcessArgs::addArg(&args, "-platform", osType);
        ProcessArgs::addArg(&args, "windows:fontengine=freetype", osType);
    }

    if (bs->qt6Project() && bs->widgetApp()) {
        ProcessArgs::addArg(&args, "--apptype", osType);
        ProcessArgs::addArg(&args, "widget", osType);
    }

    const QString main = bs->targetFile(FilePath::fromString(mainScript())).toString();
    if (!main.isEmpty())
        ProcessArgs::addArg(&args, main, osType);

    if (m_multiLanguageAspect && m_multiLanguageAspect->value())
        ProcessArgs::addArg(&args, "-qmljsdebugger=file:unused_if_debugger_arguments_added,services:DebugTranslation", osType);

    return args;
}

void QmlProjectRunConfiguration::createQtVersionAspect()
{
    if (!isQtDesignStudio())
        return;

    m_qtversionAspect = addAspect<SelectionAspect>();
    m_qtversionAspect->setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    m_qtversionAspect->setLabelText(tr("Qt Version:"));
    m_qtversionAspect->setSettingsKey("QmlProjectManager.kit");

    Kit *kit = target()->kit();
    BaseQtVersion *version = QtKitAspect::qtVersion(kit);

    if (version) {
        const QmlBuildSystem *buildSystem = qobject_cast<QmlBuildSystem *>(target()->buildSystem());
        const bool isQt6Project = buildSystem && buildSystem->qt6Project();

        if (isQt6Project) {
            m_qtversionAspect->addOption(tr("Qt 6"));
            m_qtversionAspect->setReadOnly(true);
        } else { /* Only if this is not a Qt 6 project changing kits makes sense */
            m_qtversionAspect->addOption(tr("Qt 5"));
            m_qtversionAspect->addOption(tr("Qt 6"));

            const int valueForVersion = version->qtVersion().majorVersion == 6 ? 1 : 0;

            m_qtversionAspect->setValue(valueForVersion);

            connect(m_qtversionAspect, &SelectionAspect::changed, this, [&]() {
                QTC_ASSERT(target(), return );
                auto project = target()->project();
                QTC_ASSERT(project, return );

                int oldValue = !m_qtversionAspect->value();
                const int preferedQtVersion = m_qtversionAspect->value() > 0 ? 6 : 5;
                Kit *currentKit = target()->kit();

                const QList<Kit *> kits = Utils::filtered(KitManager::kits(), [&](const Kit *k) {
                    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
                    return (version && version->qtVersion().majorVersion == preferedQtVersion)
                           && DeviceTypeKitAspect::deviceTypeId(k)
                                  == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
                });

                if (kits.contains(currentKit))
                    return;

                if (!kits.isEmpty()) {
                    auto newTarget = target()->project()->target(kits.first());
                    if (!newTarget)
                        newTarget = project->addTargetForKit(kits.first());

                    SessionManager::setActiveTarget(project, newTarget, SetActive::Cascade);

                    /* Reset the aspect. We changed the target and this aspect should not change. */
                    m_qtversionAspect->blockSignals(true);
                    m_qtversionAspect->setValue(oldValue);
                    m_qtversionAspect->blockSignals(false);
                }
            });
        }
    }
}

bool QmlProjectRunConfiguration::isEnabled() const
{
    return m_qmlMainFileAspect->isQmlFilePresent() && !commandLine().executable().isEmpty()
            && activeBuildSystem()->hasParsingData();
}

QString QmlProjectRunConfiguration::mainScript() const
{
    return m_qmlMainFileAspect->mainScript();
}

// QmlProjectRunConfigurationFactory

QmlProjectRunConfigurationFactory::QmlProjectRunConfigurationFactory()
    : FixedRunConfigurationFactory(QmlProjectRunConfiguration::tr("QML Runtime"), false)
{
    registerRunConfiguration<QmlProjectRunConfiguration>
            ("QmlProjectManager.QmlRunConfiguration.Qml");
    addSupportedProjectType(QmlProjectManager::Constants::QML_PROJECT_ID);
}

} // namespace Internal
} // namespace QmlProjectManager
