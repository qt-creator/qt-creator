// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "test-utilities.h"

#include <coreplugin/iwizardfactory.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>

#include "wizardfactories.h"

using namespace StudioWelcome;
using Core::IWizardFactory;
using ProjectExplorer::JsonWizardFactory;

namespace {

class MockWizardFactory : public JsonWizardFactory
{
public:
    MOCK_METHOD(Utils::Wizard *, runWizardImpl,
                (
                    const Utils::FilePath &,
                    QWidget *,
                    Utils::Id,
                    const QVariantMap &,
                    bool
                 ),
                (override));

    MOCK_METHOD((std::pair<int, QStringList>), screenSizeInfoFromPage, (const QString &), (const, override));

    MOCK_METHOD(bool, isAvailable, (Utils::Id), (const, override));
};

class QdsWizardFactories: public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldIconUnicodeFunc = WizardFactories::setIconUnicodeCallback(+[](const QString &) -> QString {
            return "whatever-icon unicode";
        });
    }

    void TearDown() override
    {
        WizardFactories::setIconUnicodeCallback(oldIconUnicodeFunc);
    }

    IWizardFactory *aWizardFactory(IWizardFactory::WizardKind kind,
                                   const QPair<QString, bool> &availableOnPlatform = {})
    {
        MockWizardFactory *factory = new MockWizardFactory;
        m_factories.push_back(std::unique_ptr<IWizardFactory>(factory));

        configureFactory(*factory, kind, availableOnPlatform);

        return factory;
    }

    // a good wizard factory is a wizard factory that is not filtered out, and which is available on
    // platform `this->platform`
    IWizardFactory *aGoodWizardFactory(const QString &name = "", const QString &id = "",
            const QString &categoryId = "", const std::pair<int, QStringList> &sizes = {})
    {
        MockWizardFactory *factory = new MockWizardFactory;
        m_factories.push_back(std::unique_ptr<IWizardFactory>(factory));

        configureFactory(*factory, IWizardFactory::ProjectWizard,
                         {platform, true}, sizes);

        if (!name.isEmpty())
            factory->setDisplayName(name);
        if (!id.isEmpty())
            factory->setId(Utils::Id::fromString(id));
        if (!categoryId.isEmpty())
            factory->setCategory(categoryId);

        return factory;
    }

    void configureFactory(MockWizardFactory &factory, IWizardFactory::WizardKind kind,
                          const QPair<QString, bool> &availableOnPlatform = {},
                          const std::pair<int, QStringList> &sizes = {})
    {
        if (kind == IWizardFactory::ProjectWizard) {
            QSet<Utils::Id> supported{Utils::Id{"QmlProjectManager.QmlProject"}};
            factory.setSupportedProjectTypes(supported);
            EXPECT_EQ(IWizardFactory::ProjectWizard, factory.kind())
                    << "Expected to create a Project Wizard factory";
        } else {
            factory.setSupportedProjectTypes({});
            EXPECT_EQ(IWizardFactory::FileWizard, factory.kind())
                    << "Expected to create a File Wizard factory";
        }

        if (!availableOnPlatform.first.isEmpty()) {
            const QString platform = availableOnPlatform.first;
            bool value = availableOnPlatform.second;
            Utils::Id platformId = Utils::Id::fromString(platform);

            EXPECT_CALL(factory, isAvailable(platformId))
                  .Times(AtLeast(1))
                  .WillRepeatedly(Return(value));
        }

        auto screenSizes = (sizes == std::pair<int, QStringList>{}
                                ? std::make_pair(0, QStringList({"640 x 480"}))
                                : sizes);

        EXPECT_CALL(factory, screenSizeInfoFromPage(QString("Fields")))
            .Times(AtLeast(0))
            .WillRepeatedly(Return(screenSizes));
    }

    WizardFactories makeWizardFactoriesHandler(QList<IWizardFactory *> source,
                                               const QString &platform = "")
    {
        return WizardFactories{source, nullptr, Utils::Id::fromString(platform)};
    }

protected:
    QString platform = "Desktop";

private:
    std::vector<std::unique_ptr<IWizardFactory>> m_factories;
    WizardFactories::GetIconUnicodeFunc oldIconUnicodeFunc;
};

QStringList presetNames(const WizardCategory &cat)
{
    QStringList result = Utils::transform<QStringList>(cat.items, &PresetItem::wizardName);
    return result;
}

QStringList screenSizes(const WizardCategory &cat)
{
    QStringList result = Utils::transform<QStringList>(cat.items, &PresetItem::screenSizeName);
    return result;
}

QStringList categoryNames(const std::map<QString, WizardCategory> &presets)
{
    const QMap<QString, WizardCategory> qmap{presets};
    return qmap.keys();
}

} // namespace

/******************* TESTS *******************/

TEST_F(QdsWizardFactories, haveEmptyListOfWizardFactories)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {/*wizard factories*/},
                /*get wizards supporting platform*/ "platform"
                );

    std::map<QString, WizardCategory> presets = wf.presetsGroupedByCategory();

    ASSERT_THAT(presets, IsEmpty());
}

TEST_F(QdsWizardFactories, filtersOutNonProjectWizardFactories)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {aWizardFactory(IWizardFactory::FileWizard, {platform, true})},
                /*get wizards supporting platform*/ platform
                );

    std::map<QString, WizardCategory> presets = wf.presetsGroupedByCategory();

    ASSERT_THAT(presets, IsEmpty());
}

TEST_F(QdsWizardFactories, filtersOutWizardFactoriesUnavailableForPlatform)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {aWizardFactory(IWizardFactory::ProjectWizard, {"Non-Desktop", false})},
                /*get wizards supporting platform*/ "Non-Desktop"
                );

    std::map<QString, WizardCategory> presets = wf.presetsGroupedByCategory();

    ASSERT_THAT(presets, IsEmpty());
}

TEST_F(QdsWizardFactories, doesNotFilterOutAGoodWizardFactory)
{
    WizardFactories wf = makeWizardFactoriesHandler({aGoodWizardFactory()}, platform);

    std::map<QString, WizardCategory> presets = wf.presetsGroupedByCategory();

    ASSERT_THAT(presets, Not(IsEmpty()));
}

TEST_F(QdsWizardFactories, buildsPresetItemWithCorrectSizeName)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {
                    aGoodWizardFactory("A", "A_id", "A.category", {1, {"size 0", "size 1"}}),
                },
                platform);

    std::map<QString, WizardCategory> presets = wf.presetsGroupedByCategory();

    ASSERT_THAT(categoryNames(presets), ElementsAreArray({"A.category"}));
    ASSERT_THAT(presetNames(presets["A.category"]), ElementsAreArray({"A"}));
    ASSERT_THAT(screenSizes(presets["A.category"]), ElementsAreArray({"size 1"}));
}

TEST_F(QdsWizardFactories, whenSizeInfoIsBadBuildsPresetItemWithEmptySizeName)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {
                    aGoodWizardFactory("A", "A_id", "A.category", {1, {/*empty*/}}),
                },
                platform);

    std::map<QString, WizardCategory> presets = wf.presetsGroupedByCategory();

    ASSERT_THAT(categoryNames(presets), ElementsAreArray({"A.category"}));
    ASSERT_THAT(presetNames(presets["A.category"]), ElementsAreArray({"A"}));
    ASSERT_THAT(screenSizes(presets["A.category"]), ElementsAreArray({""}));
}

TEST_F(QdsWizardFactories, sortsWizardFactoriesByCategory)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {
                    aGoodWizardFactory("B", "B_id", "Z.category"),
                    aGoodWizardFactory("X", "X_id", "A.category"),
                },
                platform);

    std::map<QString, WizardCategory> presets = wf.presetsGroupedByCategory();

    ASSERT_THAT(categoryNames(presets), ElementsAreArray({"A.category", "Z.category"}));
    ASSERT_THAT(presetNames(presets["A.category"]), ElementsAreArray({"X"}));
    ASSERT_THAT(presetNames(presets["Z.category"]), ElementsAreArray({"B"}));
}

TEST_F(QdsWizardFactories, sortsWizardFactoriesById)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {
                    aGoodWizardFactory("B", "Z_id", "category"),
                    aGoodWizardFactory("X", "A_id", "category"),
                },
                platform);

    std::map<QString, WizardCategory> presets = wf.presetsGroupedByCategory();

    ASSERT_THAT(categoryNames(presets), ElementsAreArray({"category"}));
    ASSERT_THAT(presetNames(presets["category"]), ElementsAreArray({"X", "B"}));
}

TEST_F(QdsWizardFactories, groupsWizardFactoriesByCategory)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {
                    aGoodWizardFactory("C", "C_id", "Z.category"),
                    aGoodWizardFactory("B", "B_id", "A.category"),
                    aGoodWizardFactory("A", "A_id", "A.category"),
                },
                platform);

    std::map<QString, WizardCategory> presets = wf.presetsGroupedByCategory();

    ASSERT_THAT(categoryNames(presets), ElementsAreArray({"A.category", "Z.category"}));
    ASSERT_THAT(presetNames(presets["A.category"]), ElementsAreArray({"A", "B"}));
    ASSERT_THAT(presetNames(presets["Z.category"]), ElementsAreArray({"C"}));
}

TEST_F(QdsWizardFactories, createsPresetItemAndCategoryCorrectlyFromWizardFactory)
{
    IWizardFactory *source = aGoodWizardFactory("myName", "myId", "myCategoryId");

    source->setDescription("myDescription");
    source->setDetailsPageQmlPath(":/my/qml/path");
    source->setFontIconName("myFontIconConstantName");
    source->setDisplayCategory("myDisplayCategory");

    WizardFactories::setIconUnicodeCallback(+[](const QString &name) -> QString {
        return (name == "myFontIconConstantName" ? "\uABCD" : "\0");
    });

    WizardFactories wf = makeWizardFactoriesHandler({source}, platform);

    std::map<QString, WizardCategory> presets = wf.presetsGroupedByCategory();

    ASSERT_THAT(categoryNames(presets), ElementsAreArray({"myCategoryId"}));
    ASSERT_THAT(presetNames(presets["myCategoryId"]), ElementsAreArray({"myName"}));

    auto category = presets["myCategoryId"];
    ASSERT_EQ("myCategoryId", category.id);
    ASSERT_EQ("myDisplayCategory", category.name);

    auto presetItem = presets["myCategoryId"].items[0];
    ASSERT_EQ("myName", presetItem->wizardName);
    ASSERT_EQ("myDescription", presetItem->description);
    ASSERT_EQ("qrc:/my/qml/path", presetItem->qmlPath.toString());
    ASSERT_EQ("\uABCD", presetItem->fontIconCode);
}

