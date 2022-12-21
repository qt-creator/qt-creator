// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprojectrunconfiguration.h"
#include "qmlproject.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlmainfileaspect.h"
#include "qmlmultilanguageaspect.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/winutils.h>

#include <qmljstools/qmljstoolsconstants.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace QmlProjectManager::Internal {

// QmlProjectRunConfiguration

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
    m_qmlViewerAspect->setLabelText(tr("Override device QML viewer:"));
    m_qmlViewerAspect->setPlaceHolderText(commandLine().executable().toString());
    m_qmlViewerAspect->setDisplayStyle(StringAspect::PathChooserDisplay);
    m_qmlViewerAspect->setHistoryCompleter("QmlProjectManager.viewer.history");

    auto argumentAspect = addAspect<ArgumentsAspect>(macroExpander());
    argumentAspect->setSettingsKey(Constants::QML_VIEWER_ARGUMENTS_KEY);

    setCommandLineGetter([this, target] {
        const FilePath qmlRuntime = qmlRuntimeFilePath();
        CommandLine cmd(qmlRuntime);

        // arguments in .user file
        cmd.addArgs(aspect<ArgumentsAspect>()->arguments(), CommandLine::Raw);

        // arguments from .qmlproject file
        const QmlBuildSystem *bs = qobject_cast<QmlBuildSystem *>(target->buildSystem());
        const QStringList importPaths = QmlBuildSystem::makeAbsolute(bs->targetDirectory(),
                                                                     bs->customImportPaths());
        for (const QString &importPath : importPaths) {
            cmd.addArg("-I");
            cmd.addArg(importPath);
        }

        for (const QString &fileSelector : bs->customFileSelectors()) {
            cmd.addArg("-S");
            cmd.addArg(fileSelector);
        }

        if (qmlRuntime.osType() == OsTypeWindows && bs->forceFreeType()) {
            cmd.addArg("-platform");
            cmd.addArg("windows:fontengine=freetype");
        }

        if (bs->qt6Project() && bs->widgetApp()) {
            cmd.addArg("--apptype");
            cmd.addArg("widget");
        }

        const FilePath main = bs->targetFile(FilePath::fromString(mainScript()));
        if (!main.isEmpty())
            cmd.addArg(main.nativePath());

        return cmd;
    });

    m_qmlMainFileAspect = addAspect<QmlMainFileAspect>(target);
    connect(m_qmlMainFileAspect, &QmlMainFileAspect::changed, this, &RunConfiguration::update);

    createQtVersionAspect();

    connect(target, &Target::kitChanged, this, &RunConfiguration::update);

    m_multiLanguageAspect = addAspect<QmlMultiLanguageAspect>(target);
    auto buildSystem = qobject_cast<const QmlBuildSystem *>(activeBuildSystem());
    if (buildSystem)
        m_multiLanguageAspect->setValue(buildSystem->multilanguageSupport());

    auto envAspect = addAspect<EnvironmentAspect>();
    connect(m_multiLanguageAspect,
            &QmlMultiLanguageAspect::changed,
            envAspect,
            &EnvironmentAspect::environmentChanged);

    auto envModifier = [this](Environment env) {
        if (auto bs = qobject_cast<const QmlBuildSystem *>(activeBuildSystem()))
            env.modify(bs->environment());

        if (m_multiLanguageAspect && m_multiLanguageAspect->value()
            && !m_multiLanguageAspect->databaseFilePath().isEmpty()) {
            env.set("QT_MULTILANGUAGE_DATABASE",
                    m_multiLanguageAspect->databaseFilePath().toString());
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

    if (HostOsInfo::isAnyUnixHost())
        addAspect<X11ForwardingAspect>(macroExpander());

    setRunnableModifier([this](Runnable &r) {
        const QmlBuildSystem *bs = static_cast<QmlBuildSystem *>(activeBuildSystem());
        r.workingDirectory = bs->targetDirectory();
        if (const auto * const forwardingAspect = aspect<X11ForwardingAspect>())
            r.extraData.insert("Ssh.X11ForwardToDisplay", forwardingAspect->display());
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
    // Give precedence to the manual override in the run configuration.
    const FilePath qmlViewer = m_qmlViewerAspect->filePath();
    if (!qmlViewer.isEmpty())
        return qmlViewer;

    Kit *kit = target()->kit();

    // We might not have a full Qt version for building, but the device
    // might know what is good for running.
    if (IDevice::ConstPtr dev = DeviceKitAspect::device(kit)) {
        const FilePath qmlRuntime = dev->qmlRunCommand();
        if (!qmlRuntime.isEmpty())
            return qmlRuntime;
    }

    // The Qt version might know. That's the "build" Qt version,
    // i.e. not necessarily something the device can use, but the
    // device had its chance above.
    if (QtVersion *version = QtKitAspect::qtVersion(kit)) {
        const FilePath qmlRuntime = version->qmlRuntimeFilePath();
        if (!qmlRuntime.isEmpty())
            return qmlRuntime;
    }

    // If not given explicitly by run device, nor Qt, try to pick
    // it from $PATH on the run device.
    return "qml";
}

void QmlProjectRunConfiguration::createQtVersionAspect()
{
    if (!QmlProject::isQtDesignStudio())
        return;

    m_qtversionAspect = addAspect<SelectionAspect>();
    m_qtversionAspect->setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    m_qtversionAspect->setLabelText(tr("Qt Version:"));
    m_qtversionAspect->setSettingsKey("QmlProjectManager.kit");

    Kit *kit = target()->kit();
    QtVersion *version = QtKitAspect::qtVersion(kit);

    if (version) {
        const QmlBuildSystem *buildSystem = qobject_cast<QmlBuildSystem *>(target()->buildSystem());
        const bool isQt6Project = buildSystem && buildSystem->qt6Project();

        if (isQt6Project) {
            m_qtversionAspect->addOption(tr("Qt 6"));
            m_qtversionAspect->setReadOnly(true);
        } else { /* Only if this is not a Qt 6 project changing kits makes sense */
            m_qtversionAspect->addOption(tr("Qt 5"));
            m_qtversionAspect->addOption(tr("Qt 6"));

            const int valueForVersion = version->qtVersion().majorVersion() == 6 ? 1 : 0;

            m_qtversionAspect->setValue(valueForVersion);

            connect(m_qtversionAspect, &SelectionAspect::changed, this, [&]() {
                QTC_ASSERT(target(), return );
                auto project = target()->project();
                QTC_ASSERT(project, return );

                int oldValue = !m_qtversionAspect->value();
                const int preferedQtVersion = m_qtversionAspect->value() > 0 ? 6 : 5;
                Kit *currentKit = target()->kit();

                const QList<Kit *> kits = Utils::filtered(KitManager::kits(), [&](const Kit *k) {
                    QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
                    return (version && version->qtVersion().majorVersion() == preferedQtVersion)
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

} // QmlProjectManager::Internal
