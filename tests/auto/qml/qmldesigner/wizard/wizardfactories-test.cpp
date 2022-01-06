/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <coreplugin/iwizardfactory.h>

#include "wizardfactories.h"

using namespace StudioWelcome;
using Core::IWizardFactory;

namespace {

class MockWizardFactory : public IWizardFactory
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

    IWizardFactory *aWizardFactory(IWizardFactory::WizardKind kind, bool requiresQtStudio = true,
                                   const QPair<QString, bool> &availableOnPlatform = {})
    {
        MockWizardFactory *factory = new MockWizardFactory;
        m_factories.push_back(std::unique_ptr<IWizardFactory>(factory));

        configureFactory(*factory, kind, requiresQtStudio, availableOnPlatform);

        return factory;
    }

    // a good wizard factory is a wizard factory that is not filtered out, and which is available on
    // platform `this->platform`
    IWizardFactory *aGoodWizardFactory(const QString &name = "", const QString &id = "", const QString &categoryId = "")
    {
        MockWizardFactory *factory = new MockWizardFactory;
        m_factories.push_back(std::unique_ptr<IWizardFactory>(factory));

        configureFactory(*factory, IWizardFactory::ProjectWizard, /*req QtStudio*/true, {platform, true});

        if (!name.isEmpty())
            factory->setDisplayName(name);
        if (!id.isEmpty())
            factory->setId(Utils::Id::fromString(id));
        if (!categoryId.isEmpty())
            factory->setCategory(categoryId);

        return factory;
    }

    void configureFactory(MockWizardFactory &factory, IWizardFactory::WizardKind kind,
                          bool requiresQtStudio = true,
                          const QPair<QString, bool> &availableOnPlatform = {})
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

        if (requiresQtStudio) {
            QSet<Utils::Id> features{"QtStudio"};
            factory.setRequiredFeatures(features);
        } else {
            QSet<Utils::Id> features{"some", "other", "features"};
            factory.setRequiredFeatures(features);
        }

        if (!availableOnPlatform.first.isEmpty()) {
            const QString platform = availableOnPlatform.first;
            bool value = availableOnPlatform.second;
            Utils::Id platformId = Utils::Id::fromString(platform);

            EXPECT_CALL(factory, isAvailable(platformId))
                  .Times(AtLeast(1))
                  .WillRepeatedly(Return(value));
        }
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

inline QStringList projectNames(const ProjectCategory &cat)
{
    QStringList result = Utils::transform<QStringList>(cat.items, &ProjectItem::name);
    return result;
}

inline QStringList categoryNames(const std::map<QString, ProjectCategory> &projects)
{
    QMap<QString, ProjectCategory> qmap{projects};
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

    std::map<QString, ProjectCategory> projects = wf.projectsGroupedByCategory();

    ASSERT_THAT(projects, IsEmpty());
}

TEST_F(QdsWizardFactories, filtersOutNonProjectWizardFactories)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {aWizardFactory(IWizardFactory::FileWizard, /*req QtStudio*/ true, {platform, true})},
                /*get wizards supporting platform*/ platform
                );

    std::map<QString, ProjectCategory> projects = wf.projectsGroupedByCategory();

    ASSERT_THAT(projects, IsEmpty());
}

TEST_F(QdsWizardFactories, filtersOutWizardFactoriesUnavailableForPlatform)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {aWizardFactory(IWizardFactory::ProjectWizard, /*req QtStudio*/ true, {"Non-Desktop", false})},
                /*get wizards supporting platform*/ "Non-Desktop"
                );

    std::map<QString, ProjectCategory> projects = wf.projectsGroupedByCategory();

    ASSERT_THAT(projects, IsEmpty());
}

TEST_F(QdsWizardFactories, filtersOutWizardFactoriesThatDontRequireQtStudio)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {
                    aWizardFactory(IWizardFactory::FileWizard, /*require QtStudio*/ false, {platform, true})
                },
                /*get wizards supporting platform*/ platform);

    std::map<QString, ProjectCategory> projects = wf.projectsGroupedByCategory();

    ASSERT_THAT(projects, IsEmpty());
}

TEST_F(QdsWizardFactories, doesNotFilterOutAGoodWizardFactory)
{
    WizardFactories wf = makeWizardFactoriesHandler({aGoodWizardFactory()}, platform);

    std::map<QString, ProjectCategory> projects = wf.projectsGroupedByCategory();

    ASSERT_THAT(projects, Not(IsEmpty()));
}

TEST_F(QdsWizardFactories, sortsWizardFactoriesByCategory)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {
                    aGoodWizardFactory("B", "B_id", "Z.category"),
                    aGoodWizardFactory("X", "X_id", "A.category"),
                },
                platform);

    std::map<QString, ProjectCategory> projects = wf.projectsGroupedByCategory();

    ASSERT_THAT(categoryNames(projects), ElementsAreArray({"A.category", "Z.category"}));
    ASSERT_THAT(projectNames(projects["A.category"]), ElementsAreArray({"X"}));
    ASSERT_THAT(projectNames(projects["Z.category"]), ElementsAreArray({"B"}));
}

TEST_F(QdsWizardFactories, sortsWizardFactoriesById)
{
    WizardFactories wf = makeWizardFactoriesHandler(
                {
                    aGoodWizardFactory("B", "Z_id", "category"),
                    aGoodWizardFactory("X", "A_id", "category"),
                },
                platform);

    std::map<QString, ProjectCategory> projects = wf.projectsGroupedByCategory();

    ASSERT_THAT(categoryNames(projects), ElementsAreArray({"category"}));
    ASSERT_THAT(projectNames(projects["category"]), ElementsAreArray({"X", "B"}));
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

    std::map<QString, ProjectCategory> projects = wf.projectsGroupedByCategory();

    ASSERT_THAT(categoryNames(projects), ElementsAreArray({"A.category", "Z.category"}));
    ASSERT_THAT(projectNames(projects["A.category"]), ElementsAreArray({"A", "B"}));
    ASSERT_THAT(projectNames(projects["Z.category"]), ElementsAreArray({"C"}));
}

TEST_F(QdsWizardFactories, createsProjectItemAndCategoryCorrectlyFromWizardFactory)
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

    std::map<QString, ProjectCategory> projects = wf.projectsGroupedByCategory();

    ASSERT_THAT(categoryNames(projects), ElementsAreArray({"myCategoryId"}));
    ASSERT_THAT(projectNames(projects["myCategoryId"]), ElementsAreArray({"myName"}));

    auto category = projects["myCategoryId"];
    ASSERT_EQ("myCategoryId", category.id);
    ASSERT_EQ("myDisplayCategory", category.name);

    auto projectItem = projects["myCategoryId"].items[0];
    ASSERT_EQ("myName", projectItem.name);
    ASSERT_EQ("myDescription", projectItem.description);
    ASSERT_EQ("qrc:/my/qml/path", projectItem.qmlPath.toString());
    ASSERT_EQ("\uABCD", projectItem.fontIconCode);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
