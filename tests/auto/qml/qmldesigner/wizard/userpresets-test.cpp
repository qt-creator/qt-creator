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
#include "userpresets.h"

#include <utils/filepath.h>
#include <utils/temporarydirectory.h>

namespace StudioWelcome {

void PrintTo(const UserPresetData &preset, std::ostream *os)
{
    *os << "UserPresetData{category = " << preset.categoryId;
    *os << "; wizardName = " << preset.wizardName;
    *os << "; name = " << preset.name;
    *os << "; screenSize = " << preset.screenSize;
    *os << "; keyboard = " << preset.useQtVirtualKeyboard;
    *os << "; qt = " << preset.qtVersion;
    *os << "; style = " << preset.styleName;
    *os << "}";
}

void PrintTo(const std::vector<UserPresetData> &presets, std::ostream *os)
{
    if (presets.size() == 0) {
        *os << "{}" << std::endl;
    } else {
        *os << "{" << std::endl;
        for (size_t i = 0; i < presets.size(); ++i) {
            *os << "#" << i << ": ";
            PrintTo(presets[i], os);
            *os << std::endl;
        }
        *os << "}" << std::endl;
    }
}

} // namespace StudioWelcome

using namespace StudioWelcome;

constexpr char ARRAY_NAME[] = "UserPresets";

class QdsUserPresets : public ::testing::Test
{
protected:
    void SetUp()
    {
        settings = std::make_unique<QSettings>(tempDir.filePath("test").toString(),
                                               QSettings::IniFormat);
    }

    UserPresetsStore anEmptyStore() { return UserPresetsStore{std::move(settings)}; }

    UserPresetsStore aStoreWithZeroItems()
    {
        settings->beginWriteArray(ARRAY_NAME, 0);
        settings->endArray();

        return UserPresetsStore{std::move(settings)};
    }

    UserPresetsStore aStoreWithOne(const UserPresetData &preset)
    {
        settings->beginWriteArray(ARRAY_NAME, 1);
        settings->setArrayIndex(0);

        settings->setValue("categoryId", preset.categoryId);
        settings->setValue("wizardName", preset.wizardName);
        settings->setValue("name", preset.name);
        settings->setValue("screenSize", preset.screenSize);
        settings->setValue("useQtVirtualKeyboard", preset.useQtVirtualKeyboard);
        settings->setValue("qtVersion", preset.qtVersion);
        settings->setValue("styleName", preset.styleName);

        settings->endArray();

        return UserPresetsStore{std::move(settings)};
    }

    UserPresetsStore aStoreWithPresets(const std::vector<UserPresetData> &presets)
    {
        settings->beginWriteArray(ARRAY_NAME, presets.size());

        for (size_t i = 0; i < presets.size(); ++i) {
            settings->setArrayIndex(i);
            const auto &preset = presets[i];

            settings->setValue("categoryId", preset.categoryId);
            settings->setValue("wizardName", preset.wizardName);
            settings->setValue("name", preset.name);
            settings->setValue("screenSize", preset.screenSize);
            settings->setValue("useQtVirtualKeyboard", preset.useQtVirtualKeyboard);
            settings->setValue("qtVersion", preset.qtVersion);
            settings->setValue("styleName", preset.styleName);
        }
        settings->endArray();

        return UserPresetsStore{std::move(settings)};
    }

    Utils::TemporaryDirectory tempDir{"userpresets-XXXXXX"};
    std::unique_ptr<QSettings> settings;

private:
    QString settingsPath;
};

/******************* TESTS *******************/

TEST_F(QdsUserPresets, readEmptyUserPresets)
{
    auto store = anEmptyStore();

    auto presets = store.fetchAll();

    ASSERT_THAT(presets, IsEmpty());
}

TEST_F(QdsUserPresets, readEmptyArrayOfUserPresets)
{
    auto store = aStoreWithZeroItems();

    auto presets = store.fetchAll();

    ASSERT_THAT(presets, IsEmpty());
}

TEST_F(QdsUserPresets, readOneUserPreset)
{
    UserPresetData preset{"A.categ", "3D App", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"};
    auto store = aStoreWithOne(preset);

    auto presets = store.fetchAll();

    ASSERT_THAT(presets, ElementsAreArray({preset}));
}

TEST_F(QdsUserPresets, readOneIncompleteUserPreset)
{
    // A preset entry that has the required entries, but not the others.
    UserPresetData preset{"A.categ", "3D App", "iPhone7", "", false, "", ""};
    auto store = aStoreWithOne(preset);

    auto presets = store.fetchAll();

    ASSERT_THAT(presets, ElementsAreArray({preset}));
}

TEST_F(QdsUserPresets, doesNotReadPresetsThatLackRequiredEntries)
{
    // Required entries are: Category id, wizard name, preset name.
    auto presetsInStore = std::vector<UserPresetData>{
        UserPresetData{"", "3D App", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"},
        UserPresetData{"A.categ", "", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"},
        UserPresetData{"A.categ", "3D App", "", "400 x 20", true, "Qt 5", "Material Dark"},
    };
    auto store = aStoreWithPresets(presetsInStore);

    auto presets = store.fetchAll();

    ASSERT_THAT(presets, IsEmpty());
}

TEST_F(QdsUserPresets, readSeveralUserPresets)
{
    auto presetsInStore = std::vector<UserPresetData>{
        UserPresetData{"A.categ", "3D App", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"},
        UserPresetData{"B.categ", "2D App", "iPhone6", "200 x 50", false, "Qt 6", "Fusion"},
        UserPresetData{"C.categ", "Empty", "Some Other", "60 x 30", true, "Qt 7", "Material Light"},
    };
    auto store = aStoreWithPresets(presetsInStore);

    auto presets = store.fetchAll();

    ASSERT_THAT(presets, ElementsAreArray(presetsInStore));
}

TEST_F(QdsUserPresets, cannotSaveInvalidPreset)
{
    // an invalid preset is a preset that lacks required fields.
    UserPresetData invalidPreset{"", "3D App", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"};
    auto store = anEmptyStore();

    bool saved = store.save(invalidPreset);

    auto presets = store.fetchAll();
    ASSERT_THAT(presets, IsEmpty());
    ASSERT_FALSE(saved);
}

TEST_F(QdsUserPresets, savePresetInEmptyStore)
{
    UserPresetData preset{"B.categ", "3D App", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"};
    auto store = anEmptyStore();

    store.save(preset);

    auto presets = store.fetchAll();
    ASSERT_THAT(presets, ElementsAre(preset));
}

TEST_F(QdsUserPresets, saveIncompletePreset)
{
    UserPresetData preset{"C.categ", "2D App", "Android", "", false, "", ""};
    auto store = anEmptyStore();

    store.save(preset);

    auto presets = store.fetchAll();
    ASSERT_THAT(presets, ElementsAre(preset));
}

TEST_F(QdsUserPresets, cannotSavePresetWithSameName)
{
    UserPresetData existing{"B.categ", "3D App", "Same Name", "400 x 20", true, "Qt 5", "Material Dark"};
    auto store = aStoreWithOne(existing);

    UserPresetData newPreset{"C.categ", "Empty", "Same Name", "100 x 30", false, "Qt 6", "Fusion"};
    bool saved = store.save(newPreset);

    ASSERT_FALSE(saved);
    ASSERT_THAT(store.fetchAll(), ElementsAreArray({existing}));
}

TEST_F(QdsUserPresets, saveNewPreset)
{
    UserPresetData existing{"A.categ", "3D App", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"};
    auto store = aStoreWithOne(existing);

    UserPresetData newPreset{"A.categ", "Empty", "Huawei", "100 x 30", true, "Qt 6", "Fusion"};
    store.save(newPreset);

    auto presets = store.fetchAll();
    ASSERT_THAT(presets, ElementsAre(existing, newPreset));
}

TEST_F(QdsUserPresets, removeUserPresetFromEmptyStore)
{
    UserPresetData preset{"C.categ", "2D App", "Android", "", false, "", ""};
    auto store = anEmptyStore();

    store.remove("A.categ", "User preset name");

    auto presets = store.fetchAll();
    ASSERT_THAT(presets, IsEmpty());
}

TEST_F(QdsUserPresets, removeExistingUserPreset)
{
    UserPresetData existing{"A.categ", "3D App", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"};
    auto store = aStoreWithOne(existing);

    store.remove("A.categ", "iPhone7");

    auto presets = store.fetchAll();
    ASSERT_THAT(presets, IsEmpty());
}

TEST_F(QdsUserPresets, removeNonExistingUserPreset)
{
    UserPresetData existing{"A.categ", "3D App", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"};
    auto store = aStoreWithOne(existing);

    store.remove("A.categ", "Android");

    auto presets = store.fetchAll();
    ASSERT_THAT(presets, ElementsAre(existing));
}

TEST_F(QdsUserPresets, removeExistingUserPresetInStoreWithManyPresets)
{
    auto presetsInStore = std::vector<UserPresetData>{
        UserPresetData{"A.categ", "3D App", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"},
        UserPresetData{"B.categ", "2D App", "iPhone6", "200 x 50", false, "Qt 6", "Fusion"},
        UserPresetData{"C.categ", "Empty", "Some Other", "60 x 30", true, "Qt 7", "Material Light"},
    };
    auto store = aStoreWithPresets(presetsInStore);

    store.remove("B.categ", "iPhone6");

    auto presets = store.fetchAll();
    ASSERT_THAT(presets, ElementsAre(presetsInStore[0], presetsInStore[2]));
}

