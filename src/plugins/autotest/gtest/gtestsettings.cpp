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

#include "gtestsettings.h"
#include "gtest_utils.h"

namespace Autotest {
namespace Internal {

GTestSettings::GTestSettings()
{
    setSettingsGroups("Autotest", "GTest");
    setAutoApply(false);

    registerAspect(&iterations);
    iterations.setSettingsKey("Iterations");
    iterations.setDefaultValue(1);

    registerAspect(&seed);
    seed.setSettingsKey("Seed");

    registerAspect(&runDisabled);
    runDisabled.setSettingsKey("RunDisabled");

    registerAspect(&shuffle);
    shuffle.setSettingsKey("Shuffle");

    registerAspect(&repeat);
    repeat.setSettingsKey("Repeat");

    registerAspect(&throwOnFailure);
    throwOnFailure.setSettingsKey("ThrowOnFailure");

    registerAspect(&breakOnFailure);
    breakOnFailure.setSettingsKey("BreakOnFailure");
    breakOnFailure.setDefaultValue(true);

    registerAspect(&groupMode);
    groupMode.setDefaultValue(GTest::Constants::Directory);
    groupMode.setSettingsKey("GroupMode");
    groupMode.setFromSettingsTransformation([](const QVariant &savedValue) -> QVariant {
        // avoid problems if user messes around with the settings file
        bool ok = false;
        const int tmp = savedValue.toInt(&ok);
        return ok ? static_cast<GTest::Constants::GroupMode>(tmp) : GTest::Constants::Directory;
    });

    registerAspect(&gtestFilter);
    gtestFilter.setSettingsKey("GTestFilter");
    gtestFilter.setDefaultValue(GTest::Constants::DEFAULT_FILTER);
    gtestFilter.setFromSettingsTransformation([](const QVariant &savedValue) -> QVariant {
        // avoid problems if user messes around with the settings file
        const QString tmp = savedValue.toString();
        if (GTestUtils::isValidGTestFilter(tmp))
            return tmp;
        return GTest::Constants::DEFAULT_FILTER;
    });
}

} // namespace Internal
} // namespace Autotest
