// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Learning::Internal {

inline constexpr char EXPERIENCE_PREFIX[] = "experience_";
inline constexpr char EXPERIENCE_BASIC[] = "basic";
inline constexpr char EXPERIENCE_ADVANCED[] = "advanced";

inline constexpr char TARGET_PREFIX[] = "target_";
inline constexpr char TARGET_DESKTOP[] = "desktop";
inline constexpr char TARGET_ANDROID[] = "android";
inline constexpr char TARGET_IOS[] = "ios";
inline constexpr char TARGET_BOOT2QT[] = "boot2qt";
inline constexpr char TARGET_QTFORMCUS[] = "qtformcus";

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
