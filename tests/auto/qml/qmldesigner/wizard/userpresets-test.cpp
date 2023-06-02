// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "test-utilities.h"
#include "userpresets.h"

#include <utils/filepath.h>
#include <utils/temporarydirectory.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

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

class FakeStoreIo : public StoreIo
{
public:
    QByteArray read() const override
    {
        return data.toUtf8();
    }

    void write(const QByteArray &bytes) override
    {
        data = bytes;
    }

    QString data;
};

class QdsUserPresets : public ::testing::Test
{
protected:
    void SetUp()
    {
        storeIo = std::make_unique<FakeStoreIo>();
    }

    UserPresetsStore anEmptyStore()
    {
        return UserPresetsStore{std::move(storeIo), StorePolicy::UniqueNames};
    }

    UserPresetsStore aStoreWithZeroItems()
    {
        storeIo->data = "[]";

        return UserPresetsStore{std::move(storeIo), StorePolicy::UniqueNames};
    }

    UserPresetsStore aStoreWithOne(const UserPresetData &preset,
                                   StorePolicy policy = StorePolicy::UniqueNames)
    {
        QJsonArray array({QJsonObject{{"categoryId", preset.categoryId},
                                      {"wizardName", preset.wizardName},
                                      {"name", preset.name},
                                      {"screenSize", preset.screenSize},
                                      {"useQtVirtualKeyboard", preset.useQtVirtualKeyboard},
                                      {"qtVersion", preset.qtVersion},
                                      {"styleName", preset.styleName}}});
        QJsonDocument doc{array};
        storeIo->data = doc.toJson();

        return UserPresetsStore{std::move(storeIo), policy};
    }

    UserPresetsStore aStoreWithPresets(const std::vector<UserPresetData> &presetItems)
    {
        QJsonArray array;

        for (const auto &preset : presetItems) {
            QJsonObject obj({{"categoryId", preset.categoryId},
                             {"wizardName", preset.wizardName},
                             {"name", preset.name},
                             {"screenSize", preset.screenSize},
                             {"useQtVirtualKeyboard", preset.useQtVirtualKeyboard},
                             {"qtVersion", preset.qtVersion},
                             {"styleName", preset.styleName}});

            array.append(QJsonValue{obj});
        }

        QJsonDocument doc{array};
        storeIo->data = doc.toJson();

        return UserPresetsStore{std::move(storeIo), StorePolicy::UniqueNames};
    }

    Utils::TemporaryDirectory tempDir{"userpresets-XXXXXX"};
    std::unique_ptr<FakeStoreIo> storeIo;

private:
    QString storeIoPath;
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

TEST_F(QdsUserPresets, cannotSavePresetWithSameNameForUniqueNamesPolicy)
{
    UserPresetData existing{"B.categ", "3D App", "Same Name", "400 x 20", true, "Qt 5", "Material Dark"};
    auto store = aStoreWithOne(existing, StorePolicy::UniqueNames);

    UserPresetData newPreset{"C.categ", "Empty", "Same Name", "100 x 30", false, "Qt 6", "Fusion"};
    bool saved = store.save(newPreset);

    ASSERT_FALSE(saved);
    ASSERT_THAT(store.fetchAll(), ElementsAreArray({existing}));
}

TEST_F(QdsUserPresets, canSavePresetWithSameNameForUniqueValuesPolicy)
{
    UserPresetData existing{"B.categ", "3D App", "Same Name", "400 x 20", true, "Qt 5", "Material Dark"};
    auto store = aStoreWithOne(existing, StorePolicy::UniqueValues);

    // NOTE: only Style is different
    UserPresetData newPreset{"B.categ", "3D App", "Same Name", "400 x 20", true, "Qt 5", "Fusion"};
    bool saved = store.save(newPreset);

    ASSERT_TRUE(saved);
    ASSERT_THAT(store.fetchAll(), ElementsAreArray({existing, newPreset}));
}

TEST_F(QdsUserPresets, cannotSaveExactCopyForUniqueValuesPolicy)
{
    UserPresetData existing{"B.categ", "3D App", "Same Name", "400 x 20", true, "Qt 5", "Material Dark"};
    auto store = aStoreWithOne(existing, StorePolicy::UniqueNames);

    bool saved = store.save(existing);

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

TEST_F(QdsUserPresets, canLimitPresetsToAMaximum)
{
    std::vector<UserPresetData> existing{
        {"A.categ", "AppA", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"},
        {"B.categ", "AppB", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"},
        {"C.categ", "AppC", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"},
    };
    auto store = aStoreWithPresets(existing);
    store.setMaximum(3);

    UserPresetData newPreset{"D.categ", "AppD", "Huawei", "100 x 30", true, "Qt 6", "Fusion"};
    store.save(newPreset);

    auto presets = store.fetchAll();
    ASSERT_THAT(presets, ElementsAre(existing[1], existing[2], newPreset));
}

TEST_F(QdsUserPresets, canLimitPresetsToAMaximumForReverseOrder)
{
    std::vector<UserPresetData> existing{
        {"A.categ", "AppA", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"},
        {"B.categ", "AppB", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"},
        {"C.categ", "AppC", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"},
    };
    auto store = aStoreWithPresets(existing);
    store.setMaximum(3);
    store.setReverseOrder();

    UserPresetData newPreset{"D.categ", "AppD", "Huawei", "100 x 30", true, "Qt 6", "Fusion"};
    store.save(newPreset);

    auto presets = store.fetchAll();
    ASSERT_THAT(presets, ElementsAre(newPreset, existing[0], existing[1]));
}

TEST_F(QdsUserPresets, canSavePresetsInReverseOrder)
{
    UserPresetData existing{"A.categ", "3D App", "iPhone7", "400 x 20", true, "Qt 5", "Material Dark"};
    auto store = aStoreWithOne(existing, StorePolicy::UniqueNames);
    store.setReverseOrder();

    UserPresetData newPreset{"A.categ", "Empty", "Huawei", "100 x 30", true, "Qt 6", "Fusion"};
    store.save(newPreset);

    auto presets = store.fetchAll();
    ASSERT_THAT(presets, ElementsAre(newPreset, existing));
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

