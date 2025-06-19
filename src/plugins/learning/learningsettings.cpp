// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "learningsettings.h"

using namespace Utils;

namespace Learning::Internal {

LearningSettings::LearningSettings()
{
    setSettingsGroup("Learning");

    userFlags.setSettingsKey("UserFlags");
    userFlags.setDefaultValue({
        defaultExperience(),
        QLatin1String(TARGET_PREFIX) + TARGET_DESKTOP,
    });

    showWizardOnStart.setSettingsKey("ShowWizardOnStart");
    showWizardOnStart.setDefaultValue(true);

    readSettings();
}

QString LearningSettings::defaultExperience()
{
    return QLatin1String(EXPERIENCE_PREFIX) + EXPERIENCE_BASIC;
}

LearningSettings &settings()
{
    static LearningSettings theSettings;
    return theSettings;
}

} // namespace Learning::Internal
