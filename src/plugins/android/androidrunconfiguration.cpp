// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconstants.h"
#include "androidglobal.h"
#include "androidrunconfiguration.h"
#include "androidtoolchain.h"
#include "androidtr.h"

#include <app/app_version.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/detailswidget.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android {

class BaseStringListAspect final : public Utils::StringAspect
{
public:
    explicit BaseStringListAspect() = default;
    ~BaseStringListAspect() final = default;

    void fromMap(const QVariantMap &map) final
    {
        // Pre Qt Creator 5.0 hack: Reads QStringList as QString
        setValue(map.value(settingsKey()).toStringList().join('\n'));
    }

    void toMap(QVariantMap &map) const final
    {
        // Pre Qt Creator 5.0 hack: Writes QString as QStringList
        map.insert(settingsKey(), value().split('\n'));
    }
};

AndroidRunConfiguration::AndroidRunConfiguration(Target *target, Utils::Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<EnvironmentAspect>();
    envAspect->addSupportedBaseEnvironment(Tr::tr("Clean Environment"), {});

    auto extraAppArgsAspect = addAspect<ArgumentsAspect>(macroExpander());

    connect(extraAppArgsAspect, &BaseAspect::changed, this, [target, extraAppArgsAspect] {
        if (target->buildConfigurations().first()->buildType() == BuildConfiguration::BuildType::Release) {
            const QString buildKey = target->activeBuildKey();
            target->buildSystem()->setExtraData(buildKey,
                                            Android::Constants::AndroidApplicationArgs,
                                            extraAppArgsAspect->arguments());
        }
    });

    auto amStartArgsAspect = addAspect<StringAspect>();
    amStartArgsAspect->setId(Constants::ANDROID_AM_START_ARGS);
    amStartArgsAspect->setSettingsKey("Android.AmStartArgsKey");
    amStartArgsAspect->setLabelText(Tr::tr("Activity manager start arguments:"));
    amStartArgsAspect->setDisplayStyle(StringAspect::LineEditDisplay);
    amStartArgsAspect->setHistoryCompleter("Android.AmStartArgs.History");

    auto preStartShellCmdAspect = addAspect<BaseStringListAspect>();
    preStartShellCmdAspect->setDisplayStyle(StringAspect::TextEditDisplay);
    preStartShellCmdAspect->setId(Constants::ANDROID_PRESTARTSHELLCMDLIST);
    preStartShellCmdAspect->setSettingsKey("Android.PreStartShellCmdListKey");
    preStartShellCmdAspect->setLabelText(Tr::tr("Pre-launch on-device shell commands:"));

    auto postStartShellCmdAspect = addAspect<BaseStringListAspect>();
    postStartShellCmdAspect->setDisplayStyle(StringAspect::TextEditDisplay);
    postStartShellCmdAspect->setId(Constants::ANDROID_POSTFINISHSHELLCMDLIST);
    postStartShellCmdAspect->setSettingsKey("Android.PostStartShellCmdListKey");
    postStartShellCmdAspect->setLabelText(Tr::tr("Post-quit on-device shell commands:"));

    setUpdater([this] {
        const BuildTargetInfo bti = buildTargetInfo();
        setDisplayName(bti.displayName);
        setDefaultDisplayName(bti.displayName);
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
}

} // namespace Android
