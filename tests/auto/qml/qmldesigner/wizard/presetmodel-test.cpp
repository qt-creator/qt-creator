/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "test-utilities.h"

#include "presetmodel.h"

using namespace StudioWelcome;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::PrintToString;

namespace StudioWelcome {
void PrintTo(const PresetItem &item, std::ostream *os)
{
    *os << "{categId: " << item.categoryId << ", "
        << "name: " << item.name;

    if (!item.screenSizeName.isEmpty())
        *os << ", size: " << item.screenSizeName;

    *os << "}";
}

} // namespace StudioWelcome

namespace {
std::pair<QString, WizardCategory> aCategory(const QString &categId,
                                             const QString &categName,
                                             const std::vector<QString> &names)
{
    std::vector<PresetItem> items = Utils::transform(names, [&categId](const QString &name) {
        return PresetItem{name, categId};
    });
    return std::make_pair(categId, WizardCategory{categId, categName, items});
}

MATCHER_P2(PresetIs, category, name, PrintToString(PresetItem{name, category}))
{
    return arg.categoryId == category && arg.name == name;
}

MATCHER_P3(PresetIs, category, name, size, PrintToString(PresetItem{name, category, size}))
{
    return arg.categoryId == category && arg.name == name && size == arg.screenSizeName;
}

} // namespace

/******************* TESTS *******************/

TEST(QdsPresetModel, whenHaveNoPresetsNoRecentsReturnEmpty)
{
    PresetData data;

    ASSERT_THAT(data.presets(), SizeIs(0));
    ASSERT_THAT(data.categories(), SizeIs(0));
}

TEST(QdsPresetModel, haveSameArraySizeForPresetsAndCategories)
{
    PresetData data;

    data.setData(
        {
            aCategory("A.categ", "A", {"item a", "item b"}),
            aCategory("B.categ", "B", {"item c", "item d"}),
        },
        {/*recents*/});

    ASSERT_THAT(data.presets(), SizeIs(2));
    ASSERT_THAT(data.categories(), SizeIs(2));
}

TEST(QdsPresetModel, haveWizardPresetsNoRecents)
{
    // Given
    PresetData data;

    // When
    data.setData(
        {
            aCategory("A.categ", "A", {"item a", "item b"}),
            aCategory("B.categ", "B", {"item c", "item d"}),
        },
        {/*recents*/});

    // Then
    ASSERT_THAT(data.categories(), ElementsAre("A", "B"));
    ASSERT_THAT(data.presets()[0],
                ElementsAre(PresetIs("A.categ", "item a"), PresetIs("A.categ", "item b")));
    ASSERT_THAT(data.presets()[1],
                ElementsAre(PresetIs("B.categ", "item c"), PresetIs("B.categ", "item d")));
}

TEST(QdsPresetModel, haveRecentsNoWizardPresets)
{
    PresetData data;

    data.setData({/*wizardPresets*/},
                 {
                     {"A.categ", "Desktop", "640 x 480"},
                     {"B.categ", "Mobile", "800 x 600"},
                 });

    ASSERT_THAT(data.categories(), IsEmpty());
    ASSERT_THAT(data.presets(), IsEmpty());
}

TEST(QdsPresetModel, recentsAddedBeforeWizardPresets)
{
    // Given
    PresetData data;

    // When
    data.setData(
        /*wizard presets*/
        {
            aCategory("A.categ", "A", {"Desktop", "item b"}),
            aCategory("B.categ", "B", {"item c", "Mobile"}),
        },
        /*recents*/
        {
            {"A.categ", "Desktop", "800 x 600"},
            {"B.categ", "Mobile", "640 x 480"},
        });

    // Then
    ASSERT_THAT(data.categories(), ElementsAre("Recents", "A", "B"));

    ASSERT_THAT(data.presets(),
                ElementsAreArray(
                    {ElementsAre(PresetIs("A.categ", "Desktop"), PresetIs("B.categ", "Mobile")),

                     ElementsAre(PresetIs("A.categ", "Desktop"), PresetIs("A.categ", "item b")),
                     ElementsAre(PresetIs("B.categ", "item c"), PresetIs("B.categ", "Mobile"))}));
}

TEST(QdsPresetModel, recentsShouldNotSorted)
{
    // Given
    PresetData data;

    // When
    data.setData(
        /*wizard presets*/
        {
            aCategory("A.categ", "A", {"Desktop", "item b"}),
            aCategory("B.categ", "B", {"item c", "Mobile"}),
            aCategory("Z.categ", "Z", {"Z.desktop"}),
        },
        /*recents*/
        {
            {"Z.categ", "Z.desktop", "200 x 300"},
            {"B.categ", "Mobile", "200 x 300"},
            {"A.categ", "Desktop", "200 x 300"},
        });

    // Then
    ASSERT_THAT(data.presets()[0],
                ElementsAre(PresetIs("Z.categ", "Z.desktop"),
                            PresetIs("B.categ", "Mobile"),
                            PresetIs("A.categ", "Desktop")));
}

TEST(QdsPresetModel, recentsOfSameWizardProjectButDifferentSizesAreRecognizedAsDifferentPresets)
{
    // Given
    PresetData data;

    // When
    data.setData(
        /*wizard presets*/
        {
            aCategory("A.categ", "A", {"Desktop"}),
            aCategory("B.categ", "B", {"Mobile"}),
        },
        /*recents*/
        {
            {"B.categ", "Mobile", "400 x 400"},
            {"B.categ", "Mobile", "200 x 300"},
            {"A.categ", "Desktop", "640 x 480"},
        });

    // Then
    ASSERT_THAT(data.presets()[0],
                ElementsAre(PresetIs("B.categ", "Mobile", "400 x 400"),
                            PresetIs("B.categ", "Mobile", "200 x 300"),
                            PresetIs("A.categ", "Desktop", "640 x 480")));
}

TEST(QdsPresetModel, outdatedRecentsAreNotShown)
{
    // Given
    PresetData data;

    // When
    data.setData(
        /*wizard presets*/
        {
            aCategory("A.categ", "A", {"Desktop"}),
            aCategory("B.categ", "B", {"Mobile"}),
        },
        /*recents*/
        {
            {"B.categ", "NoLongerExists", "400 x 400"},
            {"A.categ", "Desktop", "640 x 480"},
        });

    // Then
    ASSERT_THAT(data.presets()[0], ElementsAre(PresetIs("A.categ", "Desktop", "640 x 480")));
}
