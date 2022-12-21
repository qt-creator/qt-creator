// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "test-utilities.h"

#include "presetmodel.h"

using namespace StudioWelcome;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::PrintToString;

namespace StudioWelcome {

void PrintTo(const UserPresetItem &item, std::ostream *os);

void PrintTo(const PresetItem &item, std::ostream *os)
{
    if (typeid(item) == typeid(UserPresetItem)) {
        PrintTo((UserPresetItem &) item, os);
        return;
    }

    *os << "{categId: " << item.categoryId << ", "
        << "name: " << item.wizardName;

    if (!item.screenSizeName.isEmpty())
        *os << ", size: " << item.screenSizeName;

    *os << "}";
}

void PrintTo(const UserPresetItem &item, std::ostream *os)
{
    *os << "{categId: " << item.categoryId << ", "
        << "name: " << item.wizardName << ", "
        << "user name: " << item.userName;

    if (!item.screenSizeName.isEmpty())
        *os << ", size: " << item.screenSizeName;

    *os << "}";
}

void PrintTo(const std::shared_ptr<PresetItem> &p, std::ostream *os)
{
    if (p)
        PrintTo(*p, os);
    else
        *os << "{null}";
}

} // namespace StudioWelcome

namespace {
std::pair<QString, WizardCategory> aCategory(const QString &categId,
                                             const QString &categName,
                                             const std::vector<QString> &names)
{
    std::vector<std::shared_ptr<PresetItem>> items
        = Utils::transform(names, [&categId](const QString &name) {
              std::shared_ptr<PresetItem> item{new PresetItem};
              item->wizardName = name;
              item->categoryId = categId;

              return item;
          });
    return std::make_pair(categId, WizardCategory{categId, categName, items});
}

UserPresetData aUserPreset(const QString &categId, const QString &wizardName, const QString &userName)
{
    UserPresetData preset;
    preset.categoryId = categId;
    preset.wizardName = wizardName;
    preset.name = userName;

    return preset;
}

UserPresetData aRecentPreset(const QString &categId, const QString &wizardName, const QString &screenSizeName)
{
    UserPresetData preset = aUserPreset(categId, wizardName, wizardName);
    preset.screenSize = screenSizeName;

    return preset;
}

MATCHER_P2(PresetIs, category, name, PrintToString(PresetItem{name, category}))
{
    return arg->categoryId == category && arg->wizardName == name;
}

MATCHER_P3(UserPresetIs,
           category,
           wizardName,
           userName,
           PrintToString(UserPresetItem{wizardName, userName, category}))
{
    auto userPreset = dynamic_cast<UserPresetItem *>(arg.get());

    return userPreset->categoryId == category && userPreset->wizardName == wizardName
           && userPreset->userName == userName;
}

MATCHER_P3(PresetIs, category, name, size, PrintToString(PresetItem{name, category, size}))
{
    return arg->categoryId == category && arg->wizardName == name && size == arg->screenSizeName;
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
        /*wizard presets*/
        {
            aCategory("A.categ", "A", {"item a", "item b"}),
            aCategory("B.categ", "B", {"item c", "item d"}),
        },
        {/*user presets*/},
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
        /*wizard presets*/
        {
            aCategory("A.categ", "A", {"item a", "item b"}),
            aCategory("B.categ", "B", {"item c", "item d"}),
        },
        {/*user presets*/},
        {/*recents*/});

    // Then
    ASSERT_THAT(data.categories(), ElementsAre("A", "B"));
    ASSERT_THAT(data.presets()[0],
                ElementsAre(PresetIs("A.categ", "item a"), PresetIs("A.categ", "item b")));
    ASSERT_THAT(data.presets()[1],
                ElementsAre(PresetIs("B.categ", "item c"), PresetIs("B.categ", "item d")));
}

TEST(QdsPresetModel, whenHaveUserPresetsButNoWizardPresetsReturnEmpty)
{
    // Given
    PresetData data;

    // When
    data.setData({/*builtin presets*/},
                 /*user presets*/
                 {
                     aUserPreset("A.Mobile", "Scroll", "iPhone5"),
                     aUserPreset("B.Desktop", "Launcher", "MacBook"),
                 },
                 {/*recents*/});

    // Then
    ASSERT_THAT(data.categories(), IsEmpty());
    ASSERT_THAT(data.presets(), IsEmpty());
}

TEST(QdsPresetModel, haveRecentsNoWizardPresets)
{
    PresetData data;

    data.setData({/*wizardPresets*/},
                 {/*user presets*/},
                 /*recent presets*/
                 {
                     aRecentPreset("A.categ", "Desktop", "640 x 480"),
                     aRecentPreset("B.categ", "Mobile", "800 x 600"),
                 });

    ASSERT_THAT(data.categories(), IsEmpty());
    ASSERT_THAT(data.presets(), IsEmpty());
}

TEST(QdsPresetModel, recentsAddedWithWizardPresets)
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
        {/*user presets*/},
        /*recents*/
        {
            aRecentPreset("A.categ", "Desktop", "800 x 600"),
            aRecentPreset("B.categ", "Mobile", "640 x 480"),
        });

    // Then
    ASSERT_THAT(data.categories(), ElementsAre("Recents", "A", "B"));

    ASSERT_THAT(data.presets(),
                ElementsAreArray(
                    {ElementsAre(PresetIs("A.categ", "Desktop"), PresetIs("B.categ", "Mobile")),

                     ElementsAre(PresetIs("A.categ", "Desktop"), PresetIs("A.categ", "item b")),
                     ElementsAre(PresetIs("B.categ", "item c"), PresetIs("B.categ", "Mobile"))}));
}

TEST(QdsPresetModel, userPresetsAddedWithWizardPresets)
{
    // Given
    PresetData data;

    // When
    data.setData(
        /*wizard presets*/
        {
            aCategory("A.categ", "A", {"Desktop", "item b"}),
            aCategory("B.categ", "B", {"Mobile"}),
        },
        /*user presets*/
        {
            aUserPreset("A.categ", "Desktop", "Windows10"),
        },
        {/*recents*/});

    // Then
    ASSERT_THAT(data.categories(), ElementsAre("A", "B", "Custom"));
    ASSERT_THAT(data.presets(),
                ElementsAre(ElementsAre(PresetIs("A.categ", "Desktop"),
                                        PresetIs("A.categ", "item b")),
                            ElementsAre(PresetIs("B.categ", "Mobile")),
                            ElementsAre(UserPresetIs("A.categ", "Desktop", "Windows10"))));
}

TEST(QdsPresetModel, doesNotAddUserPresetsOfNonExistingCategory)
{
    // Given
    PresetData data;

    // When
    data.setData(
        /*wizard presets*/
        {
            aCategory("A.categ", "A", {"Desktop"}), // Only category "A.categ" exists
        },
        /*user presets*/
        {
            aUserPreset("Bad.Categ", "Desktop", "Windows8"), // Bad.Categ does not exist
        },
        {/*recents*/});

    // Then
    ASSERT_THAT(data.categories(), ElementsAre("A"));
    ASSERT_THAT(data.presets(), ElementsAre(ElementsAre(PresetIs("A.categ", "Desktop"))));
}

TEST(QdsPresetModel, doesNotAddUserPresetIfWizardPresetItRefersToDoesNotExist)
{
    // Given
    PresetData data;

    // When
    data.setData(
        /*wizard presets*/
        {
            aCategory("A.categ", "A", {"Desktop"}),
        },
        /*user presets*/
        {
            aUserPreset("B.categ", "BadWizard", "Tablet"),   // BadWizard referenced does not exist
        },
        {/*recents*/});

    // Then
    ASSERT_THAT(data.categories(), ElementsAre("A"));
    ASSERT_THAT(data.presets(), ElementsAre(ElementsAre(PresetIs("A.categ", "Desktop"))));
}

TEST(QdsPresetModel, userPresetWithSameNameAsWizardPreset)
{
    // Given
    PresetData data;

    // When
    data.setData(
        /*wizard presets*/
        {
            aCategory("A.categ", "A", {"Desktop"}),
        },
        /*user presets*/
        {
            aUserPreset("A.categ", "Desktop", "Desktop"),
        },
        {/*recents*/});

    // Then
    ASSERT_THAT(data.categories(), ElementsAre("A", "Custom"));
    ASSERT_THAT(data.presets(),
                ElementsAre(ElementsAre(PresetIs("A.categ", "Desktop")),
                            ElementsAre(UserPresetIs("A.categ", "Desktop", "Desktop"))));
}

TEST(QdsPresetModel, recentOfNonExistentWizardPreset)
{
    // Given
    PresetData data;

    // When
    data.setData(
        /*wizard presets*/
        {
            aCategory("A.categ", "A", {"Desktop"}),
        },
        {/*user presets*/},
        /*recents*/
        {
            aRecentPreset("A.categ", "Windows 7", "200 x 300")
        });

    // Then
    ASSERT_THAT(data.categories(), ElementsAre("A"));
    ASSERT_THAT(data.presets(), ElementsAre(ElementsAre(PresetIs("A.categ", "Desktop"))));
}

TEST(QdsPresetModel, recentsShouldNotBeSorted)
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
        {/*user presets*/},
        /*recents*/
        {
            aRecentPreset("Z.categ", "Z.desktop", "200 x 300"),
            aRecentPreset("B.categ", "Mobile", "200 x 300"),
            aRecentPreset("A.categ", "Desktop", "200 x 300"),
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
        {/*user presets*/},
        /*recents*/
        {
            aRecentPreset("B.categ", "Mobile", "400 x 400"),
            aRecentPreset("B.categ", "Mobile", "200 x 300"),
            aRecentPreset("A.categ", "Desktop", "640 x 480"),
        });

    // Then
    ASSERT_THAT(data.presets()[0],
                ElementsAre(PresetIs("B.categ", "Mobile", "400 x 400"),
                            PresetIs("B.categ", "Mobile", "200 x 300"),
                            PresetIs("A.categ", "Desktop", "640 x 480")));
}

TEST(QdsPresetModel, allowRecentsWithTheSameName)
{
    // Given
    PresetData data;

    // When
    data.setData(
        /*wizard presets*/
        {
            aCategory("A.categ", "A", {"Desktop"}),
        },
        {/*user presets*/},
        /*recents*/
        {
            /* NOTE: it is assumed recents with the same name and size have other fields that
             * distinguishes them from one another. It is the responsibility of the caller, who
             * calls data.setData() to make sure that the recents do not contain duplicates. */
            aRecentPreset("A.categ", "Desktop", "200 x 300"),
            aRecentPreset("A.categ", "Desktop", "200 x 300"),
            aRecentPreset("A.categ", "Desktop", "200 x 300"),
        });

    // Then
    ASSERT_THAT(data.presets()[0],
                ElementsAre(PresetIs("A.categ", "Desktop"),
                            PresetIs("A.categ", "Desktop"),
                            PresetIs("A.categ", "Desktop")));
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
        {/*user presets*/},
        /*recents*/
        {
            aRecentPreset("B.categ", "NoLongerExists", "400 x 400"),
            aRecentPreset("A.categ", "Desktop", "640 x 480"),
        });

    // Then
    ASSERT_THAT(data.presets()[0], ElementsAre(PresetIs("A.categ", "Desktop", "640 x 480")));
}
