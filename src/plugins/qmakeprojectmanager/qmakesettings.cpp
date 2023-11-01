// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmakesettings.h"
#include "qmakeprojectmanagertr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

using namespace Utils;

namespace QmakeProjectManager::Internal {

QmakeSettings &settings()
{
    static QmakeSettings theSettings;
    return theSettings;
}

QmakeSettings::QmakeSettings()
{
    setAutoApply(false);
    setSettingsGroup("QmakeProjectManager");

    warnAgainstUnalignedBuildDir.setSettingsKey("WarnAgainstUnalignedBuildDir");
    warnAgainstUnalignedBuildDir.setDefaultValue(HostOsInfo::isWindowsHost());
    warnAgainstUnalignedBuildDir.setLabelText(Tr::tr("Warn if a project's source and "
        "build directories are not at the same level"));
    warnAgainstUnalignedBuildDir.setToolTip(Tr::tr("Qmake has subtle bugs that "
        "can be triggered if source and build directory are not at the same level."));

    alwaysRunQmake.setSettingsKey("AlwaysRunQmake");
    alwaysRunQmake.setLabelText(Tr::tr("Run qmake on every build"));
    alwaysRunQmake.setToolTip(Tr::tr("This option can help to prevent failures on "
        "incremental builds, but might slow them down unnecessarily in the general case."));

    ignoreSystemFunction.setSettingsKey("RunSystemFunction");
    ignoreSystemFunction.setLabelText(Tr::tr("Ignore qmake's system() function when parsing a project"));
    ignoreSystemFunction.setToolTip(Tr::tr("Checking this option avoids unwanted side effects, "
         "but may result in inexact parsing results."));
    // The settings value has been stored with the opposite meaning for a while.
    // Avoid changing the stored value, but flip it on read/write:
    const auto invertBoolVariant = [](const QVariant &v) { return QVariant(!v.toBool()); };
    ignoreSystemFunction.setFromSettingsTransformation(invertBoolVariant);
    ignoreSystemFunction.setToSettingsTransformation(invertBoolVariant);

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            warnAgainstUnalignedBuildDir,
            alwaysRunQmake,
            ignoreSystemFunction,
            st
        };
    });

    readSettings();
}

class QmakeSettingsPage : public Core::IOptionsPage
{
public:
    QmakeSettingsPage()
    {
        setId("K.QmakeProjectManager.QmakeSettings");
        setDisplayName(Tr::tr("Qmake"));
        setCategory(ProjectExplorer::Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

static const QmakeSettingsPage settingsPage;

} // QmakeProjectManager::Internal
