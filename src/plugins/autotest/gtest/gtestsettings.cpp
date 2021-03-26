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
#include "gtestconstants.h"
#include "../autotestconstants.h"
#include "../testframeworkmanager.h"
#include "../testtreemodel.h"

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Autotest {
namespace Internal {

GTestSettings::GTestSettings()
{
    setSettingsGroups("Autotest", "GTest");
    setAutoApply(false);

    registerAspect(&iterations);
    iterations.setSettingsKey("Iterations");
    iterations.setDefaultValue(1);
    iterations.setEnabled(false);
    iterations.setLabelText(tr("Iterations:"));
    iterations.setEnabler(&repeat);

    registerAspect(&seed);
    seed.setSettingsKey("Seed");
    seed.setSpecialValueText(QString());
    seed.setEnabled(false);
    seed.setLabelText(tr("Seed:"));
    seed.setToolTip(tr("A seed of 0 generates a seed based on the current timestamp."));
    seed.setEnabler(&shuffle);

    registerAspect(&runDisabled);
    runDisabled.setSettingsKey("RunDisabled");
    runDisabled.setLabelText(tr("Run disabled tests"));
    runDisabled.setToolTip(tr("Executes disabled tests when performing a test run."));

    registerAspect(&shuffle);
    shuffle.setSettingsKey("Shuffle");
    shuffle.setLabelText(tr("Shuffle tests"));
    shuffle.setToolTip(tr("Shuffles tests automatically on every iteration by the given seed."));

    registerAspect(&repeat);
    repeat.setSettingsKey("Repeat");
    repeat.setLabelText(tr("Repeat tests"));
    repeat.setToolTip(tr("Repeats a test run (you might be required to increase the timeout to avoid canceling the tests)."));

    registerAspect(&throwOnFailure);
    throwOnFailure.setSettingsKey("ThrowOnFailure");
    throwOnFailure.setLabelText(tr("Throw on failure"));
    throwOnFailure.setToolTip(tr("Turns assertion failures into C++ exceptions."));

    registerAspect(&breakOnFailure);
    breakOnFailure.setSettingsKey("BreakOnFailure");
    breakOnFailure.setDefaultValue(true);
    breakOnFailure.setLabelText(tr("Break on failure while debugging"));
    breakOnFailure.setToolTip(tr("Turns failures into debugger breakpoints."));

    registerAspect(&groupMode);
    groupMode.setSettingsKey("GroupMode");
    groupMode.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    groupMode.setFromSettingsTransformation([this](const QVariant &savedValue) -> QVariant {
        // avoid problems if user messes around with the settings file
        bool ok = false;
        const int tmp = savedValue.toInt(&ok);
        return ok ? groupMode.indexForItemValue(tmp) : GTest::Constants::Directory;
    });
    groupMode.setToSettingsTransformation([this](const QVariant &value) {
        return groupMode.itemValueForIndex(value.toInt());
    });
    groupMode.addOption({tr("Directory"), {}, GTest::Constants::Directory});
    groupMode.addOption({tr("GTest Filter"), {}, GTest::Constants::GTestFilter});
    groupMode.setDefaultValue(GTest::Constants::Directory);
    groupMode.setLabelText(tr("Group mode:"));
    groupMode.setToolTip(tr("Select on what grouping the tests should be based."));

    registerAspect(&gtestFilter);
    gtestFilter.setSettingsKey("GTestFilter");
    gtestFilter.setDisplayStyle(StringAspect::LineEditDisplay);
    gtestFilter.setDefaultValue(GTest::Constants::DEFAULT_FILTER);
    gtestFilter.setFromSettingsTransformation([](const QVariant &savedValue) -> QVariant {
        // avoid problems if user messes around with the settings file
        const QString tmp = savedValue.toString();
        if (GTestUtils::isValidGTestFilter(tmp))
            return tmp;
        return GTest::Constants::DEFAULT_FILTER;
    });
    gtestFilter.setEnabled(false);
    gtestFilter.setLabelText(tr("Active filter:"));
    gtestFilter.setToolTip(tr("Set the GTest filter to be used for grouping.\n"
        "See Google Test documentation for further information on GTest filters."));

    gtestFilter.setValidationFunction([](FancyLineEdit *edit, QString * /*error*/) {
        return edit && GTestUtils::isValidGTestFilter(edit->text());
    });

    QObject::connect(&groupMode, &SelectionAspect::volatileValueChanged,
                     &gtestFilter, [this](int val) {
        gtestFilter.setEnabled(groupMode.itemValueForIndex(val) == GTest::Constants::GTestFilter);
    });
}

GTestSettingsPage::GTestSettingsPage(GTestSettings *settings, Utils::Id settingsId)
{
    setId(settingsId);
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayName(QCoreApplication::translate("GTestFramework",
                                               GTest::Constants::FRAMEWORK_SETTINGS_CATEGORY));
    setSettings(settings);
    QObject::connect(settings, &AspectContainer::applied, this, [] {
        Id id = Id(Constants::FRAMEWORK_PREFIX).withSuffix(GTest::Constants::FRAMEWORK_NAME);
        TestTreeModel::instance()->rebuild({id});
    });

    setLayouter([settings](QWidget *widget) {
        GTestSettings &s = *settings;
        using namespace Layouting;
        const Break nl;

        Grid grid {
            s.runDisabled, nl,
            s.breakOnFailure, nl,
            s.repeat, s.iterations, nl,
            s.shuffle, s.seed
        };

        Form form {
            s.groupMode,
            s.gtestFilter
        };

        Column { Row { Column { grid, form, Stretch() }, Stretch() } }.attachTo(widget);
    });
}

} // namespace Internal
} // namespace Autotest
