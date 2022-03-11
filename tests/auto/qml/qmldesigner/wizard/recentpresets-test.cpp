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

#include <QDir>
#include <QRandomGenerator>
#include <QTime>

#include "recentpresets.h"
#include "utils/filepath.h"
#include "utils/temporarydirectory.h"

using namespace StudioWelcome;

constexpr char GROUP_NAME[] = "RecentPresets";
constexpr char ITEMS[] = "Wizards";

namespace StudioWelcome {
void PrintTo(const RecentPresetData &recent, std::ostream *os)
{
    *os << "{categId: " << recent.category << ", name: " << recent.presetName
        << ", size: " << recent.sizeName << ", isUser: " << recent.isUserPreset;

    *os << "}";
}
} // namespace StudioWelcome

class QdsRecentPresets : public ::testing::Test
{
protected:
    RecentPresetsStore aStoreWithRecents(const QStringList &items)
    {
        settings.beginGroup(GROUP_NAME);
        settings.setValue(ITEMS, items);
        settings.endGroup();

        return RecentPresetsStore{&settings};
    }

    RecentPresetsStore aStoreWithOne(const QVariant &item)
    {
        settings.beginGroup(GROUP_NAME);
        settings.setValue(ITEMS, item);
        settings.endGroup();

        return RecentPresetsStore{&settings};
    }

protected:
    Utils::TemporaryDirectory tempDir{"recentpresets-XXXXXX"};
    QSettings settings{tempDir.filePath("test").toString(), QSettings::IniFormat};

private:
    QString settingsPath;
};

/******************* TESTS *******************/

TEST_F(QdsRecentPresets, readFromEmptyStore)
{
    RecentPresetsStore store{&settings};

    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents, IsEmpty());
}

TEST_F(QdsRecentPresets, readEmptyRecentPresets)
{
    RecentPresetsStore store = aStoreWithOne("");

    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents, IsEmpty());
}

TEST_F(QdsRecentPresets, readOneRecentPresetAsList)
{
    RecentPresetsStore store = aStoreWithRecents({"category/preset:640 x 480"});

    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents, ElementsAre(RecentPresetData("category", "preset", "640 x 480")));
}

TEST_F(QdsRecentPresets, readOneRecentPresetAsString)
{
    RecentPresetsStore store = aStoreWithOne("category/preset:200 x 300");

    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents, ElementsAre(RecentPresetData("category", "preset", "200 x 300")));
}

TEST_F(QdsRecentPresets, readOneRecentUserPresetAsString)
{
    RecentPresetsStore store = aStoreWithOne("category/[U]preset:200 x 300");

    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents, ElementsAre(RecentPresetData("category", "preset", "200 x 300", true)));
}

TEST_F(QdsRecentPresets, readBadRecentPresetAsString)
{
    RecentPresetsStore store = aStoreWithOne("no_category_only_preset");

    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents, IsEmpty());
}

TEST_F(QdsRecentPresets, readBadRecentPresetAsInt)
{
    RecentPresetsStore store = aStoreWithOne(32);

    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents, IsEmpty());
}

TEST_F(QdsRecentPresets, readBadRecentPresetsInList)
{
    RecentPresetsStore store = aStoreWithRecents({
        "bad1",                      // no category, no size
        "categ/name:800 x 600",      // good
        "categ/bad2",                //no size
        "categ/bad3:",               //no size
        "categ 1/bad4:200 x 300",    // category has space
        "categ/bad5: 400 x 300",     // size starts with space
        "categ/bad6:400",            // bad size
        "categ/[U]user:300 x 200",   // good
        "categ/[u]user2:300 x 200",  // small cap "U"
        "categ/[x]user3:300 x 200",  // must be letter "U"
        "categ/[U] user4:300 x 200", // space
    });

    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents,
                ElementsAre(RecentPresetData("categ", "name", "800 x 600", false),
                            RecentPresetData("categ", "user", "300 x 200", true)));
}

TEST_F(QdsRecentPresets, readTwoRecentPresets)
{
    RecentPresetsStore store = aStoreWithRecents(
        {"category_1/preset 1:640 x 480", "category_2/preset 2:320 x 200"});

    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents,
                ElementsAre(RecentPresetData("category_1", "preset 1", "640 x 480"),
                            RecentPresetData("category_2", "preset 2", "320 x 200")));
}

TEST_F(QdsRecentPresets, readRecentsToDifferentKindsOfPresets)
{
    RecentPresetsStore store = aStoreWithRecents(
        {"category_1/preset 1:640 x 480", "category_2/[U]preset 2:320 x 200"});

    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents,
                ElementsAre(RecentPresetData("category_1", "preset 1", "640 x 480", false),
                            RecentPresetData("category_2", "preset 2", "320 x 200", true)));
}

TEST_F(QdsRecentPresets, addFirstRecentPreset)
{
    RecentPresetsStore store{&settings};

    store.add("A.Category", "Normal Application", "400 x 600");
    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents, ElementsAre(RecentPresetData("A.Category", "Normal Application", "400 x 600")));
}

TEST_F(QdsRecentPresets, addFirstRecentUserPreset)
{
    RecentPresetsStore store{&settings};

    store.add("A.Category", "Normal Application", "400 x 600", /*user preset*/ true);
    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents,
                ElementsAre(RecentPresetData("A.Category", "Normal Application", "400 x 600", true)));
}

TEST_F(QdsRecentPresets, addExistingFirstRecentPreset)
{
    RecentPresetsStore store = aStoreWithRecents({"category/preset:200 x 300"});
    ASSERT_THAT(store.fetchAll(), ElementsAre(RecentPresetData("category", "preset", "200 x 300")));

    store.add("category", "preset", "200 x 300");
    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents, ElementsAre(RecentPresetData("category", "preset", "200 x 300")));
}

TEST_F(QdsRecentPresets, addRecentUserPresetWithSameNameAsExistingRecentNormalPreset)
{
    RecentPresetsStore store = aStoreWithRecents({"category/preset:200 x 300"});
    ASSERT_THAT(store.fetchAll(), ElementsAre(RecentPresetData("category", "preset", "200 x 300")));

    store.add("category", "preset", "200 x 300", /*user preset*/ true);
    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents,
                ElementsAre(RecentPresetData("category", "preset", "200 x 300", true),
                            RecentPresetData("category", "preset", "200 x 300", false)));
}

TEST_F(QdsRecentPresets, addSecondRecentPreset)
{
    RecentPresetsStore store = aStoreWithRecents({"A.Category/Preset 1:800 x 600"});

    store.add("A.Category", "Preset 2", "640 x 480");
    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents,
                ElementsAre(RecentPresetData("A.Category", "Preset 2", "640 x 480"),
                            RecentPresetData("A.Category", "Preset 1", "800 x 600")));
}

TEST_F(QdsRecentPresets, addSecondRecentPresetSameKindButDifferentSize)
{
    RecentPresetsStore store = aStoreWithRecents({"A.Category/Preset:800 x 600"});

    store.add("A.Category", "Preset", "640 x 480");
    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents,
                ElementsAre(RecentPresetData("A.Category", "Preset", "640 x 480"),
                            RecentPresetData("A.Category", "Preset", "800 x 600")));
}

TEST_F(QdsRecentPresets, fetchesRecentPresetsInTheReverseOrderTheyWereAdded)
{
    RecentPresetsStore store{&settings};

    store.add("A.Category", "Preset 1", "640 x 480");
    store.add("A.Category", "Preset 2", "640 x 480");
    store.add("A.Category", "Preset 3", "800 x 600");
    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents,
                ElementsAre(RecentPresetData("A.Category", "Preset 3", "800 x 600"),
                            RecentPresetData("A.Category", "Preset 2", "640 x 480"),
                            RecentPresetData("A.Category", "Preset 1", "640 x 480")));
}

TEST_F(QdsRecentPresets, addingAnExistingRecentPresetMakesItTheFirst)
{
    RecentPresetsStore store = aStoreWithRecents({"A.Category/Preset 1:200 x 300",
                                                  "A.Category/Preset 2:200 x 300",
                                                  "A.Category/Preset 3:640 x 480"});

    store.add("A.Category", "Preset 3", "640 x 480");
    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents,
                ElementsAre(RecentPresetData("A.Category", "Preset 3", "640 x 480"),
                            RecentPresetData("A.Category", "Preset 1", "200 x 300"),
                            RecentPresetData("A.Category", "Preset 2", "200 x 300")));
}

TEST_F(QdsRecentPresets, addingTooManyRecentPresetsRemovesTheOldestOne)
{
    RecentPresetsStore store = aStoreWithRecents(
        {"A.Category/Preset 2:200 x 300", "A.Category/Preset 1:200 x 300"});
    store.setMaximum(2);

    store.add("A.Category", "Preset 3", "200 x 300");
    std::vector<RecentPresetData> recents = store.fetchAll();

    ASSERT_THAT(recents,
                ElementsAre(RecentPresetData("A.Category", "Preset 3", "200 x 300"),
                            RecentPresetData("A.Category", "Preset 2", "200 x 300")));
}
