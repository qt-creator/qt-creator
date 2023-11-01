// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprojectrunconfiguration.h"
#include "qmlmainfileaspect.h"
#include "qmlmultilanguageaspect.h"
#include "qmlproject.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlprojectmanagertr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qmldesignerbase/qmldesignerbaseplugin.h>
#include <qmldesignerbase/utils/qmlpuppetpaths.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/process.h>
#include <utils/processinterface.h>
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
public:
    QmlProjectRunConfiguration(Target *target, Id id);

private:
    QString disabledReason() const final;
    bool isEnabled() const final;

    FilePath mainScript() const;
    FilePath qmlRuntimeFilePath() const;
    void setupQtVersionAspect();

    FilePathAspect qmlViewer{this};
    ArgumentsAspect arguments{this};
    QmlMainFileAspect qmlMainFile{this};
    SelectionAspect qtversion{this};
    QmlMultiLanguageAspect multiLanguage{this};
    EnvironmentAspect environment{this};
    X11ForwardingAspect x11Forwarding{this};

    mutable bool usePuppetAsQmlRuntime = false;
};

QmlProjectRunConfiguration::QmlProjectRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
{
    qmlViewer.setSettingsKey(Constants::QML_VIEWER_KEY);
    qmlViewer.setLabelText(Tr::tr("Override device QML viewer:"));
    qmlViewer.setPlaceHolderText(qmlRuntimeFilePath().toUserOutput());
    qmlViewer.setHistoryCompleter("QmlProjectManager.viewer.history");

    arguments.setSettingsKey(Constants::QML_VIEWER_ARGUMENTS_KEY);
    arguments.setMacroExpander(macroExpander());

    setCommandLineGetter([this, target] {
        const FilePath qmlRuntime = qmlRuntimeFilePath();
        CommandLine cmd(qmlRuntime);
        if (usePuppetAsQmlRuntime)
            cmd.addArg("--qml-runtime");

        // arguments in .user file
        cmd.addArgs(arguments(), CommandLine::Raw);

        // arguments from .qmlproject file
        const QmlBuildSystem *bs = qobject_cast<QmlBuildSystem *>(target->buildSystem());
        for (const QString &importPath : bs->customImportPaths()) {
            cmd.addArg("-I");
            cmd.addArg(bs->targetDirectory().pathAppended(importPath).path());
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

        const FilePath main = bs->targetFile(mainScript());

        if (!main.isEmpty())
            cmd.addArg(main.path());

        return cmd;
    });

    qmlMainFile.setTarget(target);
    connect(&qmlMainFile, &BaseAspect::changed, this, &RunConfiguration::update);

    if (Core::ICore::isQtDesignStudio())
        setupQtVersionAspect();
    else
        qtversion.setVisible(false);

    connect(target, &Target::kitChanged, this, &RunConfiguration::update);

    multiLanguage.setTarget(target);
    auto buildSystem = qobject_cast<const QmlBuildSystem *>(activeBuildSystem());
    if (buildSystem)
        multiLanguage.setValue(buildSystem->multilanguageSupport());

    connect(&multiLanguage, &BaseAspect::changed,
            &environment, &EnvironmentAspect::environmentChanged);

    auto envModifier = [this](Environment env) {
        if (auto bs = qobject_cast<const QmlBuildSystem *>(activeBuildSystem()))
            env.modify(bs->environment());

        if (multiLanguage() && !multiLanguage.databaseFilePath().isEmpty()) {
            env.set("QT_MULTILANGUAGE_DATABASE", multiLanguage.databaseFilePath().path());
            env.set("QT_MULTILANGUAGE_LANGUAGE", multiLanguage.currentLocale());
        } else {
            env.unset("QT_MULTILANGUAGE_DATABASE");
            env.unset("QT_MULTILANGUAGE_LANGUAGE");
        }
        return env;
    };

    const Id deviceTypeId = DeviceTypeKitAspect::deviceTypeId(target->kit());
    if (deviceTypeId == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        environment.addPreferredBaseEnvironment(Tr::tr("System Environment"), [envModifier] {
            return envModifier(Environment::systemEnvironment());
        });
    }

    environment.addSupportedBaseEnvironment(Tr::tr("Clean Environment"), [envModifier] {
        Environment environment;
        return envModifier(environment);
    });

    x11Forwarding.setMacroExpander(macroExpander());

    setRunnableModifier([this](ProcessRunData &r) {
        const QmlBuildSystem *bs = static_cast<QmlBuildSystem *>(activeBuildSystem());
        r.workingDirectory = bs->targetDirectory();
    });

    setDisplayName(Tr::tr("QML Utility", "QMLRunConfiguration display name."));
    update();
}

QString QmlProjectRunConfiguration::disabledReason() const
{
    if (mainScript().isEmpty())
        return Tr::tr("No script file to execute.");

    const FilePath viewer = qmlRuntimeFilePath();
    if (DeviceTypeKitAspect::deviceTypeId(kit())
            == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE
            && !viewer.exists()) {
        return Tr::tr("No QML utility found.");
    }
    if (viewer.isEmpty())
        return Tr::tr("No QML utility specified for target device.");
    return RunConfiguration::disabledReason();
}

FilePath QmlProjectRunConfiguration::qmlRuntimeFilePath() const
{
    usePuppetAsQmlRuntime = false;
    // Give precedence to the manual override in the run configuration.
    if (!qmlViewer().isEmpty())
        return qmlViewer();

    Kit *kit = target()->kit();

    // We might not have a full Qt version for building, but the device
    // might know what is good for running.
    IDevice::ConstPtr dev = DeviceKitAspect::device(kit);
    if (dev) {
        const FilePath qmlRuntime = dev->qmlRunCommand();
        if (!qmlRuntime.isEmpty())
            return qmlRuntime;
    }
    auto hasDeployStep = [this]() {
        return target()->activeDeployConfiguration() &&
            !target()->activeDeployConfiguration()->stepList()->isEmpty();
    };

    // The Qt version might know, but we need to make sure
    // that the device can reach it.
    if (QtVersion *version = QtKitAspect::qtVersion(kit)) {
        // look for puppet as qmlruntime only in QtStudio Qt versions
        if (version->features().contains("QtStudio") &&
            version->qtVersion().majorVersion() > 5 && !hasDeployStep()) {

            auto [workingDirectoryPath, puppetPath] = QmlDesigner::QmlPuppetPaths::qmlPuppetPaths(
                        target(), QmlDesigner::QmlDesignerBasePlugin::settings());
            if (!puppetPath.isEmpty()) {
                usePuppetAsQmlRuntime = true;
                return puppetPath;
            }
        }
        const FilePath qmlRuntime = version->qmlRuntimeFilePath();
        if (!qmlRuntime.isEmpty() && (!dev || dev->ensureReachable(qmlRuntime)))
            return qmlRuntime;
    }

    // If not given explicitly by run device, nor Qt, try to pick
    // it from $PATH on the run device.
    return dev ? dev->filePath("qml").searchInPath() : "qml";
}

void QmlProjectRunConfiguration::setupQtVersionAspect()
{
    if (!Core::ICore::isQtDesignStudio())
        return;

    qtversion.setSettingsKey("QmlProjectManager.kit");
    qtversion.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    qtversion.setLabelText(Tr::tr("Qt Version:"));

    Kit *kit = target()->kit();
    QtVersion *version = QtKitAspect::qtVersion(kit);

    if (version) {
        const QmlBuildSystem *buildSystem = qobject_cast<QmlBuildSystem *>(target()->buildSystem());
        const bool isQt6Project = buildSystem && buildSystem->qt6Project();

        if (isQt6Project) {
            qtversion.addOption(Tr::tr("Qt 6"));
            qtversion.setReadOnly(true);
        } else { /* Only if this is not a Qt 6 project changing kits makes sense */
            qtversion.addOption(Tr::tr("Qt 5"));
            qtversion.addOption(Tr::tr("Qt 6"));

            const int valueForVersion = version->qtVersion().majorVersion() == 6 ? 1 : 0;

            qtversion.setValue(valueForVersion);

            connect(&qtversion, &BaseAspect::changed, this, [this] {
                QTC_ASSERT(target(), return );
                auto project = target()->project();
                QTC_ASSERT(project, return );

                int oldValue = !qtversion();
                const int preferedQtVersion = qtversion() > 0 ? 6 : 5;
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

                    project->setActiveTarget(newTarget, SetActive::Cascade);

                    /* Reset the aspect. We changed the target and this aspect should not change. */
                    // FIXME: That should use setValueSilently()
                    qtversion.blockSignals(true);
                    qtversion.setValue(oldValue);
                    qtversion.blockSignals(false);
                }
            });
        }
    }
}

bool QmlProjectRunConfiguration::isEnabled() const
{
    return const_cast<QmlProjectRunConfiguration *>(this)->qmlMainFile.isQmlFilePresent()
           && !commandLine().executable().isEmpty()
           && activeBuildSystem()->hasParsingData();
}

FilePath QmlProjectRunConfiguration::mainScript() const
{
    return qmlMainFile.mainScript();
}

// QmlProjectRunConfigurationFactory

QmlProjectRunConfigurationFactory::QmlProjectRunConfigurationFactory()
    : FixedRunConfigurationFactory(Tr::tr("QML Runtime"), false)
{
    registerRunConfiguration<QmlProjectRunConfiguration>
            ("QmlProjectManager.QmlRunConfiguration.Qml");
    addSupportedProjectType(QmlProjectManager::Constants::QML_PROJECT_ID);
}

} // QmlProjectManager::Internal
