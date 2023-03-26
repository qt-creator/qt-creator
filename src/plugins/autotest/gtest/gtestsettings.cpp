// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gtestsettings.h"

#include "gtest_utils.h"
#include "gtestconstants.h"
#include "../autotestconstants.h"
#include "../autotesttr.h"
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
    iterations.setLabelText(Tr::tr("Iterations:"));
    iterations.setEnabler(&repeat);

    registerAspect(&seed);
    seed.setSettingsKey("Seed");
    seed.setSpecialValueText({});
    seed.setEnabled(false);
    seed.setLabelText(Tr::tr("Seed:"));
    seed.setToolTip(Tr::tr("A seed of 0 generates a seed based on the current timestamp."));
    seed.setEnabler(&shuffle);

    registerAspect(&runDisabled);
    runDisabled.setSettingsKey("RunDisabled");
    runDisabled.setLabelText(Tr::tr("Run disabled tests"));
    runDisabled.setToolTip(Tr::tr("Executes disabled tests when performing a test run."));

    registerAspect(&shuffle);
    shuffle.setSettingsKey("Shuffle");
    shuffle.setLabelText(Tr::tr("Shuffle tests"));
    shuffle.setToolTip(Tr::tr("Shuffles tests automatically on every iteration by the given seed."));

    registerAspect(&repeat);
    repeat.setSettingsKey("Repeat");
    repeat.setLabelText(Tr::tr("Repeat tests"));
    repeat.setToolTip(Tr::tr("Repeats a test run (you might be required to increase the timeout to avoid canceling the tests)."));

    registerAspect(&throwOnFailure);
    throwOnFailure.setSettingsKey("ThrowOnFailure");
    throwOnFailure.setLabelText(Tr::tr("Throw on failure"));
    throwOnFailure.setToolTip(Tr::tr("Turns assertion failures into C++ exceptions."));

    registerAspect(&breakOnFailure);
    breakOnFailure.setSettingsKey("BreakOnFailure");
    breakOnFailure.setDefaultValue(true);
    breakOnFailure.setLabelText(Tr::tr("Break on failure while debugging"));
    breakOnFailure.setToolTip(Tr::tr("Turns failures into debugger breakpoints."));

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
    groupMode.addOption({Tr::tr("Directory"), {}, GTest::Constants::Directory});
    groupMode.addOption({Tr::tr("GTest Filter"), {}, GTest::Constants::GTestFilter});
    groupMode.setDefaultValue(GTest::Constants::Directory);
    groupMode.setLabelText(Tr::tr("Group mode:"));
    groupMode.setToolTip(Tr::tr("Select on what grouping the tests should be based."));

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
    gtestFilter.setLabelText(Tr::tr("Active filter:"));
    gtestFilter.setToolTip(Tr::tr("Set the GTest filter to be used for grouping.\n"
        "See Google Test documentation for further information on GTest filters."));

    gtestFilter.setValidationFunction([](FancyLineEdit *edit, QString * /*error*/) {
        return edit && GTestUtils::isValidGTestFilter(edit->text());
    });

    QObject::connect(&groupMode, &SelectionAspect::volatileValueChanged,
                     &gtestFilter, [this](int val) {
        gtestFilter.setEnabled(groupMode.itemValueForIndex(val) == GTest::Constants::GTestFilter);
    });
}

GTestSettingsPage::GTestSettingsPage(GTestSettings *settings, Id settingsId)
{
    setId(settingsId);
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayName(Tr::tr(GTest::Constants::FRAMEWORK_SETTINGS_CATEGORY));
    setSettings(settings);

    QObject::connect(settings, &AspectContainer::applied, this, [] {
        Id id = Id(Constants::FRAMEWORK_PREFIX).withSuffix(GTest::Constants::FRAMEWORK_NAME);
        TestTreeModel::instance()->rebuild({id});
    });

    setLayouter([settings](QWidget *widget) {
        GTestSettings &s = *settings;
        using namespace Layouting;

        Grid grid {
            s.runDisabled, br,
            s.breakOnFailure, br,
            s.repeat, s.iterations, br,
            s.shuffle, s.seed
        };

        Form form {
            s.groupMode,
            s.gtestFilter
        };

        Column { Row { Column { grid, form, st }, st } }.attachTo(widget);
    });
}

} // namespace Internal
} // namespace Autotest
