/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidrunconfiguration.h"

#include "androidconstants.h"
#include "androidglobal.h"
#include "androidtoolchain.h"
#include "androidmanager.h"

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
    envAspect->addSupportedBaseEnvironment(tr("Clean Environment"), {});

    auto extraAppArgsAspect = addAspect<ArgumentsAspect>();

    connect(extraAppArgsAspect, &BaseAspect::changed,
            this, [target, extraAppArgsAspect]() {
        if (target->buildConfigurations().first()->buildType() == BuildConfiguration::BuildType::Release) {
            const QString buildKey = target->activeBuildKey();
            target->buildSystem()->setExtraData(buildKey,
                                            Android::Constants::AndroidApplicationArgs,
                                            extraAppArgsAspect->arguments(target->macroExpander()));
        }
    });

    auto amStartArgsAspect = addAspect<StringAspect>();
    amStartArgsAspect->setId(Constants::ANDROID_AM_START_ARGS);
    amStartArgsAspect->setSettingsKey("Android.AmStartArgsKey");
    amStartArgsAspect->setLabelText(tr("Activity manager start arguments:"));
    amStartArgsAspect->setDisplayStyle(StringAspect::LineEditDisplay);
    amStartArgsAspect->setHistoryCompleter("Android.AmStartArgs.History");

    auto preStartShellCmdAspect = addAspect<BaseStringListAspect>();
    preStartShellCmdAspect->setDisplayStyle(StringAspect::TextEditDisplay);
    preStartShellCmdAspect->setId(Constants::ANDROID_PRESTARTSHELLCMDLIST);
    preStartShellCmdAspect->setSettingsKey("Android.PreStartShellCmdListKey");
    preStartShellCmdAspect->setLabelText(tr("Pre-launch on-device shell commands:"));

    auto postStartShellCmdAspect = addAspect<BaseStringListAspect>();
    postStartShellCmdAspect->setDisplayStyle(StringAspect::TextEditDisplay);
    postStartShellCmdAspect->setId(Constants::ANDROID_POSTFINISHSHELLCMDLIST);
    postStartShellCmdAspect->setSettingsKey("Android.PostStartShellCmdListKey");
    postStartShellCmdAspect->setLabelText(tr("Post-quit on-device shell commands:"));

    setUpdater([this, target] {
        const BuildTargetInfo bti = buildTargetInfo();
        setDisplayName(bti.displayName);
        setDefaultDisplayName(bti.displayName);
        AndroidManager::updateGradleProperties(target, buildKey());
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
}

} // namespace Android
