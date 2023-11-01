// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconstants.h"
#include "androidglobal.h"
#include "androidrunconfiguration.h"
#include "androidtoolchain.h"
#include "androidtr.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/detailswidget.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android {

class BaseStringListAspect final : public Utils::StringAspect
{
public:
    explicit BaseStringListAspect(AspectContainer *container)
        : StringAspect(container)
    {}

    void fromMap(const Store &map) final
    {
        // Pre Qt Creator 5.0 hack: Reads QStringList as QString
        setValue(map.value(settingsKey()).toStringList().join('\n'));
    }

    void toMap(Store &map) const final
    {
        // Pre Qt Creator 5.0 hack: Writes QString as QStringList
        map.insert(settingsKey(), value().split('\n'));
    }
};

class AndroidRunConfiguration : public RunConfiguration
{
public:
    AndroidRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        environment.addSupportedBaseEnvironment(Tr::tr("Clean Environment"), {});

        extraAppArgs.setMacroExpander(macroExpander());

        connect(&extraAppArgs, &BaseAspect::changed, this, [this, target] {
            if (target->buildConfigurations().first()->buildType() == BuildConfiguration::BuildType::Release) {
                const QString buildKey = target->activeBuildKey();
                target->buildSystem()->setExtraData(buildKey,
                                                    Android::Constants::AndroidApplicationArgs,
                                                    extraAppArgs());
            }
        });

        amStartArgs.setId(Constants::ANDROID_AM_START_ARGS);
        amStartArgs.setSettingsKey("Android.AmStartArgsKey");
        amStartArgs.setLabelText(Tr::tr("Activity manager start arguments:"));
        amStartArgs.setDisplayStyle(StringAspect::LineEditDisplay);
        amStartArgs.setHistoryCompleter("Android.AmStartArgs.History");

        preStartShellCmd.setDisplayStyle(StringAspect::TextEditDisplay);
        preStartShellCmd.setId(Constants::ANDROID_PRESTARTSHELLCMDLIST);
        preStartShellCmd.setSettingsKey("Android.PreStartShellCmdListKey");
        preStartShellCmd.setLabelText(Tr::tr("Pre-launch on-device shell commands:"));

        postStartShellCmd.setDisplayStyle(StringAspect::TextEditDisplay);
        postStartShellCmd.setId(Constants::ANDROID_POSTFINISHSHELLCMDLIST);
        postStartShellCmd.setSettingsKey("Android.PostStartShellCmdListKey");
        postStartShellCmd.setLabelText(Tr::tr("Post-quit on-device shell commands:"));

        setUpdater([this] {
            const BuildTargetInfo bti = buildTargetInfo();
            setDisplayName(bti.displayName);
            setDefaultDisplayName(bti.displayName);
        });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    }

    EnvironmentAspect environment{this};
    ArgumentsAspect extraAppArgs{this};
    StringAspect amStartArgs{this};
    BaseStringListAspect preStartShellCmd{this};
    BaseStringListAspect postStartShellCmd{this};
};

AndroidRunConfigurationFactory::AndroidRunConfigurationFactory()
{
    registerRunConfiguration<AndroidRunConfiguration>("Qt4ProjectManager.AndroidRunConfiguration:");
    addSupportedTargetDeviceType(Android::Constants::ANDROID_DEVICE_TYPE);
}

} // namespace Android
