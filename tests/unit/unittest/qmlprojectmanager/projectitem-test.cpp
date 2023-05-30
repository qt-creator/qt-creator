// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "google-using-declarations.h"
#include "googletest.h" // IWYU pragma: keep

#include <qmlprojectmanager/buildsystem/projectitem/qmlprojectitem.h>

#include <utils/algorithm.h>
namespace {

constexpr QLatin1String localTestDataDir{UNITTEST_DIR "/qmlprojectmanager/data"};

class QmlProjectItem : public testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        projectItemEmpty = std::make_unique<const QmlProjectManager::QmlProjectItem>(
            Utils::FilePath::fromString(localTestDataDir + "/getter-setter/empty.qmlproject"), true);

        projectItemWithQdsPrefix = std::make_unique<const QmlProjectManager::QmlProjectItem>(
            Utils::FilePath::fromString(localTestDataDir
                                        + "/getter-setter/with_qds_prefix.qmlproject"),
            true);

        projectItemWithoutQdsPrefix = std::make_unique<const QmlProjectManager::QmlProjectItem>(
            Utils::FilePath::fromString(localTestDataDir
                                        + "/getter-setter/without_qds_prefix.qmlproject"),
            true);

        projectItemFileFilters = std::make_unique<const QmlProjectManager::QmlProjectItem>(
            Utils::FilePath::fromString(localTestDataDir
                                        + "/file-filters/MaterialBundle.qmlproject"),
            true);
    }

    static void TearDownTestSuite()
    {
        projectItemEmpty.reset();
        projectItemWithQdsPrefix.reset();
        projectItemWithoutQdsPrefix.reset();
        projectItemFileFilters.reset();
    }

protected:
    static inline std::unique_ptr<const QmlProjectManager::QmlProjectItem> projectItemEmpty;
    static inline std::unique_ptr<const QmlProjectManager::QmlProjectItem> projectItemWithQdsPrefix;
    static inline std::unique_ptr<const QmlProjectManager::QmlProjectItem>
        projectItemWithoutQdsPrefix;
    std::unique_ptr<QmlProjectManager::QmlProjectItem> projectItemSetters = std::make_unique<
        QmlProjectManager::QmlProjectItem>(Utils::FilePath::fromString(
                                               localTestDataDir + "/getter-setter/empty.qmlproject"),
                                           true);
    static inline std::unique_ptr<const QmlProjectManager::QmlProjectItem> projectItemFileFilters;
};

auto createAbsoluteFilePaths(const QStringList &fileList)
{
    return Utils::transform(fileList, [](const QString &fileName) {
        return Utils::FilePath::fromString(localTestDataDir + "/file-filters").pathAppended(fileName);
    });
}

TEST_F(QmlProjectItem, GetWithQdsPrefixMainFileProject)
{
    auto mainFile = projectItemWithQdsPrefix->mainFile();

    ASSERT_THAT(mainFile, Eq("content/App.qml"));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixMainUIFileProject)
{
    auto mainUiFile = projectItemWithQdsPrefix->mainUiFile();

    ASSERT_THAT(mainUiFile, Eq("Screen01.ui.qml"));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixMcuProject)
{
    auto isMcuProject = projectItemWithQdsPrefix->isQt4McuProject();

    ASSERT_TRUE(isMcuProject);
}

TEST_F(QmlProjectItem, GetWithQdsPrefixQtVersion)
{
    auto qtVersion = projectItemWithQdsPrefix->versionQt();

    ASSERT_THAT(qtVersion, Eq("6"));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixQtQuickVersion)
{
    auto qtQuickVersion = projectItemWithQdsPrefix->versionQtQuick();

    ASSERT_THAT(qtQuickVersion, Eq("6.2"));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixDesignStudioVersion)
{
    auto designStudioVersion = projectItemWithQdsPrefix->versionDesignStudio();

    ASSERT_THAT(designStudioVersion, Eq("3.9"));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixSourceDirectory)
{
    auto sourceDirectory = projectItemWithQdsPrefix->sourceDirectory().path();

    auto expectedSourceDir = localTestDataDir + "/getter-setter";

    ASSERT_THAT(sourceDirectory, Eq(expectedSourceDir));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixTarGetWithQdsPrefixDirectory)
{
    auto targetDirectory = projectItemWithQdsPrefix->targetDirectory();

    ASSERT_THAT(targetDirectory, Eq("/opt/targetDirectory"));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixImportPaths)
{
    auto importPaths = projectItemWithQdsPrefix->importPaths();

    ASSERT_THAT(importPaths, UnorderedElementsAre("imports", "asset_imports"));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixFileSelectors)
{
    auto fileSelectors = projectItemWithQdsPrefix->fileSelectors();

    ASSERT_THAT(fileSelectors, UnorderedElementsAre("WXGA", "darkTheme", "ShowIndicator"));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixMultiLanguageSupport)
{
    auto multilanguageSupport = projectItemWithQdsPrefix->multilanguageSupport();

    ASSERT_TRUE(multilanguageSupport);
}

TEST_F(QmlProjectItem, GetWithQdsPrefixSupportedLanguages)
{
    auto supportedLanguages = projectItemWithQdsPrefix->supportedLanguages();

    ASSERT_THAT(supportedLanguages, UnorderedElementsAre("en", "fr"));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixPrimaryLanguage)
{
    auto primaryLanguage = projectItemWithQdsPrefix->primaryLanguage();
    ;

    ASSERT_THAT(primaryLanguage, Eq("en"));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixWidgetApp)
{
    auto widgetApp = projectItemWithQdsPrefix->widgetApp();

    ASSERT_TRUE(widgetApp);
}

TEST_F(QmlProjectItem, GetWithQdsPrefixFileList)
{
    QStringList fileList;
    for (const auto &file : projectItemWithQdsPrefix->files()) {
        fileList.append(file.path());
    }

    auto expectedFileList = localTestDataDir + "/getter-setter/qtquickcontrols2.conf";

    ASSERT_THAT(fileList, UnorderedElementsAre(expectedFileList));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixShaderToolArgs)
{
    auto shaderToolArgs = projectItemWithQdsPrefix->shaderToolArgs();

    ASSERT_THAT(
        shaderToolArgs,
        UnorderedElementsAre("-s", "--glsl", "\"100 es,120,150\"", "--hlsl", "50", "--msl", "12"));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixShaderToolFiles)
{
    auto shaderToolFiles = projectItemWithQdsPrefix->shaderToolFiles();

    ASSERT_THAT(shaderToolFiles, UnorderedElementsAre("content/shaders/*"));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixEnvironment)
{
    auto env = projectItemWithQdsPrefix->environment();

    ASSERT_THAT(env,
                UnorderedElementsAre(
                    Utils::EnvironmentItem("QT_QUICK_CONTROLS_CONF", "qtquickcontrols2.conf")));
}

TEST_F(QmlProjectItem, GetWithQdsPrefixForceFreeType)
{
    auto forceFreeType = projectItemWithQdsPrefix->forceFreeType();

    ASSERT_TRUE(forceFreeType);
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixMainFileProject)
{
    auto mainFile = projectItemWithoutQdsPrefix->mainFile();

    ASSERT_THAT(mainFile, Eq("content/App.qml"));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixMainUIFileProject)
{
    auto mainUiFile = projectItemWithoutQdsPrefix->mainUiFile();

    ASSERT_THAT(mainUiFile, Eq("Screen01.ui.qml"));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixMcuProject)
{
    auto isMcuProject = projectItemWithoutQdsPrefix->isQt4McuProject();

    ASSERT_TRUE(isMcuProject);
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixQtVersion)
{
    auto qtVersion = projectItemWithoutQdsPrefix->versionQt();

    ASSERT_THAT(qtVersion, Eq("6"));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixQtQuickVersion)
{
    auto qtQuickVersion = projectItemWithoutQdsPrefix->versionQtQuick();

    ASSERT_THAT(qtQuickVersion, Eq("6.2"));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixDesignStudioVersion)
{
    auto designStudioVersion = projectItemWithoutQdsPrefix->versionDesignStudio();

    ASSERT_THAT(designStudioVersion, Eq("3.9"));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixSourceDirectory)
{
    auto sourceDirectory = projectItemWithoutQdsPrefix->sourceDirectory().path();

    auto expectedSourceDir = localTestDataDir + "/getter-setter";

    ASSERT_THAT(sourceDirectory, Eq(expectedSourceDir));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixTarGetWithoutQdsPrefixDirectory)
{
    auto targetDirectory = projectItemWithoutQdsPrefix->targetDirectory();

    ASSERT_THAT(targetDirectory, Eq("/opt/targetDirectory"));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixImportPaths)
{
    auto importPaths = projectItemWithoutQdsPrefix->importPaths();

    ASSERT_THAT(importPaths, UnorderedElementsAre("imports", "asset_imports"));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixFileSelectors)
{
    auto fileSelectors = projectItemWithoutQdsPrefix->fileSelectors();

    ASSERT_THAT(fileSelectors, UnorderedElementsAre("WXGA", "darkTheme", "ShowIndicator"));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixMultiLanguageSupport)
{
    auto multilanguageSupport = projectItemWithoutQdsPrefix->multilanguageSupport();

    ASSERT_TRUE(multilanguageSupport);
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixSupportedLanguages)
{
    auto supportedLanguages = projectItemWithoutQdsPrefix->supportedLanguages();

    ASSERT_THAT(supportedLanguages, UnorderedElementsAre("en", "fr"));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixPrimaryLanguage)
{
    auto primaryLanguage = projectItemWithoutQdsPrefix->primaryLanguage();
    ;

    ASSERT_THAT(primaryLanguage, Eq("en"));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixWidgetApp)
{
    auto widgetApp = projectItemWithoutQdsPrefix->widgetApp();

    ASSERT_TRUE(widgetApp);
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixFileList)
{
    QStringList fileList;
    for (const auto &file : projectItemWithoutQdsPrefix->files()) {
        fileList.append(file.path());
    }

    auto expectedFileList = localTestDataDir + "/getter-setter/qtquickcontrols2.conf";

    ASSERT_THAT(fileList, UnorderedElementsAre(expectedFileList));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixShaderToolArgs)
{
    auto shaderToolArgs = projectItemWithoutQdsPrefix->shaderToolArgs();

    ASSERT_THAT(
        shaderToolArgs,
        UnorderedElementsAre("-s", "--glsl", "\"100 es,120,150\"", "--hlsl", "50", "--msl", "12"));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixShaderToolFiles)
{
    auto shaderToolFiles = projectItemWithoutQdsPrefix->shaderToolFiles();

    ASSERT_THAT(shaderToolFiles, UnorderedElementsAre("content/shaders/*"));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixEnvironment)
{
    auto env = projectItemWithoutQdsPrefix->environment();

    ASSERT_THAT(env,
                UnorderedElementsAre(
                    Utils::EnvironmentItem("QT_QUICK_CONTROLS_CONF", "qtquickcontrols2.conf")));
}

TEST_F(QmlProjectItem, GetWithoutQdsPrefixForceFreeType)
{
    auto forceFreeType = projectItemWithoutQdsPrefix->forceFreeType();

    ASSERT_TRUE(forceFreeType);
}

TEST_F(QmlProjectItem, GetEmptyMainFileProject)
{
    auto mainFile = projectItemEmpty->mainFile();

    ASSERT_THAT(mainFile, IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptyMainUIFileProject)
{
    auto mainUiFile = projectItemEmpty->mainUiFile();

    ASSERT_THAT(mainUiFile, IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptyMcuProject)
{
    auto isMcuProject = projectItemEmpty->isQt4McuProject();

    ASSERT_FALSE(isMcuProject);
}

TEST_F(QmlProjectItem, GetEmptyQtVersion)
{
    auto qtVersion = projectItemEmpty->versionQt();

    // default Qt Version is "5" for Design Studio projects
    ASSERT_THAT(qtVersion, Eq("5"));
}

TEST_F(QmlProjectItem, GetEmptyQtQuickVersion)
{
    auto qtQuickVersion = projectItemEmpty->versionQtQuick();

    ASSERT_THAT(projectItemEmpty->versionQtQuick(), IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptyDesignStudioVersion)
{
    auto designStudioVersion = projectItemEmpty->versionDesignStudio();

    ASSERT_THAT(projectItemEmpty->versionDesignStudio(), IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptySourceDirectory)
{
    auto sourceDirectory = projectItemEmpty->sourceDirectory().path();

    auto expectedSourceDir = localTestDataDir + "/getter-setter";

    // default source directory is the project directory
    ASSERT_THAT(sourceDirectory, Eq(expectedSourceDir));
}

TEST_F(QmlProjectItem, GetEmptyTarGetEmptyDirectory)
{
    auto targetDirectory = projectItemEmpty->targetDirectory();

    ASSERT_THAT(targetDirectory, IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptyImportPaths)
{
    auto importPaths = projectItemEmpty->importPaths();

    ASSERT_THAT(importPaths, IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptyFileSelectors)
{
    auto fileSelectors = projectItemEmpty->fileSelectors();

    ASSERT_THAT(fileSelectors, IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptyMultiLanguageSupport)
{
    auto multilanguageSupport = projectItemEmpty->multilanguageSupport();

    ASSERT_FALSE(multilanguageSupport);
}

TEST_F(QmlProjectItem, GetEmptySupportedLanguages)
{
    auto supportedLanguages = projectItemEmpty->supportedLanguages();

    ASSERT_THAT(supportedLanguages, IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptyPrimaryLanguage)
{
    auto primaryLanguage = projectItemEmpty->primaryLanguage();

    ASSERT_THAT(primaryLanguage, IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptyWidgetApp)
{
    auto widgetApp = projectItemEmpty->widgetApp();

    ASSERT_FALSE(widgetApp);
}

TEST_F(QmlProjectItem, GetEmptyFileList)
{
    auto fileList = projectItemEmpty->files();

    ASSERT_THAT(fileList, IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptyShaderToolArgs)
{
    auto shaderToolArgs = projectItemEmpty->shaderToolArgs();

    ASSERT_THAT(shaderToolArgs, IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptyShaderToolFiles)
{
    auto shaderToolFiles = projectItemEmpty->shaderToolFiles();

    ASSERT_THAT(shaderToolFiles, IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptyEnvironment)
{
    auto env = projectItemEmpty->environment();

    ASSERT_THAT(env, IsEmpty());
}

TEST_F(QmlProjectItem, GetEmptyForceFreeType)
{
    auto forceFreeType = projectItemEmpty->forceFreeType();

    ASSERT_FALSE(forceFreeType);
}

TEST_F(QmlProjectItem, SetMainFileProject)
{
    projectItemSetters->setMainFile("testing");

    auto mainFile = projectItemSetters->mainFile();

    ASSERT_THAT(mainFile, Eq("testing"));
}

TEST_F(QmlProjectItem, SetMainUIFileProject)
{
    projectItemSetters->setMainUiFile("testing");

    auto mainUiFile = projectItemSetters->mainUiFile();

    ASSERT_THAT(mainUiFile, Eq("testing"));
}

TEST_F(QmlProjectItem, SetImportPaths)
{
    projectItemSetters->setImportPaths({"testing"});

    auto importPaths = projectItemSetters->importPaths();

    ASSERT_THAT(importPaths, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, AddImportPaths)
{
    projectItemSetters->setImportPaths({});
    projectItemSetters->addImportPath("testing");

    auto importPaths = projectItemSetters->importPaths();

    ASSERT_THAT(importPaths, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, SetFileSelectors)
{
    projectItemSetters->setFileSelectors({"testing"});

    auto fileSelectors = projectItemSetters->fileSelectors();

    ASSERT_THAT(fileSelectors, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, AddFileSelectors)
{
    projectItemSetters->setFileSelectors({});
    projectItemSetters->addFileSelector("testing");

    auto fileSelectors = projectItemSetters->fileSelectors();

    ASSERT_THAT(fileSelectors, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, SetMultiLanguageSupport)
{
    projectItemSetters->setMultilanguageSupport(true);

    auto multilanguageSupport = projectItemSetters->multilanguageSupport();

    ASSERT_TRUE(multilanguageSupport);
}

TEST_F(QmlProjectItem, SetSupportedLanguages)
{
    projectItemSetters->setSupportedLanguages({"testing"});

    auto supportedLanguages = projectItemSetters->supportedLanguages();

    ASSERT_THAT(supportedLanguages, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, AddSupportedLanguages)
{
    projectItemSetters->setSupportedLanguages({});
    projectItemSetters->addSupportedLanguage("testing");

    auto supportedLanguages = projectItemSetters->supportedLanguages();

    ASSERT_THAT(supportedLanguages, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, SetPrimaryLanguage)
{
    projectItemSetters->setPrimaryLanguage("testing");

    auto primaryLanguage = projectItemSetters->primaryLanguage();
    ;

    ASSERT_THAT(primaryLanguage, Eq("testing"));
}

TEST_F(QmlProjectItem, SetWidgetApp)
{
    projectItemSetters->setWidgetApp(true);

    auto widgetApp = projectItemSetters->widgetApp();

    ASSERT_TRUE(widgetApp);
}

TEST_F(QmlProjectItem, SetShaderToolArgs)
{
    projectItemSetters->setShaderToolArgs({"testing"});

    auto shaderToolArgs = projectItemSetters->shaderToolArgs();

    ASSERT_THAT(shaderToolArgs, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, AddShaderToolArgs)
{
    projectItemSetters->setShaderToolArgs({});
    projectItemSetters->addShaderToolArg("testing");

    auto shaderToolArgs = projectItemSetters->shaderToolArgs();

    ASSERT_THAT(shaderToolArgs, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, SetShaderToolFiles)
{
    projectItemSetters->setShaderToolFiles({"testing"});

    auto shaderToolFiles = projectItemSetters->shaderToolFiles();

    ASSERT_THAT(shaderToolFiles, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, AddShaderToolFiles)
{
    projectItemSetters->setShaderToolFiles({});
    projectItemSetters->addShaderToolFile("testing");

    auto shaderToolFiles = projectItemSetters->shaderToolFiles();

    ASSERT_THAT(shaderToolFiles, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, AddEnvironment)
{
    projectItemSetters->addToEnviroment("testing", "testing");
    auto envs = projectItemSetters->environment();

    Utils::EnvironmentItems expectedEnvs;
    expectedEnvs.push_back({"testing", "testing"});

    ASSERT_EQ(envs, expectedEnvs);
}

TEST_F(QmlProjectItem, SetForceFreeTypeTrue)
{
    projectItemSetters->setForceFreeType(true);

    ASSERT_EQ(projectItemSetters->forceFreeType(), true);
}

TEST_F(QmlProjectItem, SetForceFreeTypeFalse)
{
    projectItemSetters->setForceFreeType(false);

    ASSERT_EQ(projectItemSetters->forceFreeType(), false);
}

TEST_F(QmlProjectItem, SetQtVersion)
{
    projectItemSetters->setVersionQt("6");

    ASSERT_EQ(projectItemSetters->versionQt().toStdString(), "6");
}

TEST_F(QmlProjectItem, SetQtQuickVersion)
{
    projectItemSetters->setVersionQtQuick("6");

    ASSERT_EQ(projectItemSetters->versionQtQuick(), "6");
}

TEST_F(QmlProjectItem, SetDesignStudioVersion)
{
    projectItemSetters->setVersionDesignStudio("6");

    ASSERT_EQ(projectItemSetters->versionDesignStudio(), "6");
}

// TODO: We should move these 2 tests into the integration tests
TEST_F(QmlProjectItem, TestFileFilters)
{
    // GIVEN
    auto fileListPath = Utils::FilePath::fromString(localTestDataDir + "/file-filters/filelist.txt");
    QStringList fileNameList
        = QString::fromUtf8(fileListPath.fileContents().value()).replace("\r\n", "\n").split("\n");
    auto expectedAbsoluteFilePaths = createAbsoluteFilePaths(fileNameList);

    // WHEN
    auto filePaths = projectItemFileFilters->files();

    // THEN
    ASSERT_THAT(filePaths, UnorderedElementsAreArray(expectedAbsoluteFilePaths));
}

TEST_F(QmlProjectItem, MatchesFile)
{
    // GIVEN
    auto fileSearched = localTestDataDir + "/file-filters/content/MaterialNames.qml";

    // WHEN
    auto fileFound = projectItemFileFilters->matchesFile(fileSearched);

    // THEN
    ASSERT_TRUE(fileFound);
}

TEST_F(QmlProjectItem, NotMatchesFile)
{
    // GIVEN
    auto fileSearched = localTestDataDir + "/file-filters/content/non-existing-file.qwerty";

    // WHEN
    auto fileFound = projectItemFileFilters->matchesFile(fileSearched);

    // THEN
    ASSERT_FALSE(fileFound);
}

} // namespace
