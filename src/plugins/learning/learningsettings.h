// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Learning::Internal {

const char EXPERIENCE_PREFIX[] = "experience_";
const char EXPERIENCE_BASIC[] = "basic";
const char EXPERIENCE_ADVANCED[] = "advanced";

const char TARGET_PREFIX[] = "target_";
const char TARGET_DESKTOP[] = "desktop";
const char TARGET_ANDROID[] = "android";
const char TARGET_IOS[] = "ios";
const char TARGET_BOOT2QT[] = "boot2qt";
const char TARGET_QTFORMCUS[] = "qtformcus";

class LearningSettings final : public Utils::AspectContainer
{
public:
    LearningSettings();

    static QString defaultExperience();

    Utils::StringListAspect userFlags{this};
    Utils::BoolAspect showWizardOnStart{this};
};

LearningSettings &settings();

} // namespace Learning::Internal
