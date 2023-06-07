// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/google-using-declarations.h"
#include "../utils/googletest.h" // IWYU pragma: keep

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

TEST_F(QmlProjectItem, get_with_qds_prefix_main_file_project)
{
    auto mainFile = projectItemWithQdsPrefix->mainFile();

    ASSERT_THAT(mainFile, Eq("content/App.qml"));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_main_ui_file_project)
{
    auto mainUiFile = projectItemWithQdsPrefix->mainUiFile();

    ASSERT_THAT(mainUiFile, Eq("Screen01.ui.qml"));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_mcu_project)
{
    auto isMcuProject = projectItemWithQdsPrefix->isQt4McuProject();

    ASSERT_TRUE(isMcuProject);
}

TEST_F(QmlProjectItem, get_with_qds_prefix_qt_version)
{
    auto qtVersion = projectItemWithQdsPrefix->versionQt();

    ASSERT_THAT(qtVersion, Eq("6"));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_qt_quick_version)
{
    auto qtQuickVersion = projectItemWithQdsPrefix->versionQtQuick();

    ASSERT_THAT(qtQuickVersion, Eq("6.2"));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_design_studio_version)
{
    auto designStudioVersion = projectItemWithQdsPrefix->versionDesignStudio();

    ASSERT_THAT(designStudioVersion, Eq("3.9"));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_source_directory)
{
    auto sourceDirectory = projectItemWithQdsPrefix->sourceDirectory().path();

    auto expectedSourceDir = localTestDataDir + "/getter-setter";

    ASSERT_THAT(sourceDirectory, Eq(expectedSourceDir));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_tar_get_with_qds_prefix_directory)
{
    auto targetDirectory = projectItemWithQdsPrefix->targetDirectory();

    ASSERT_THAT(targetDirectory, Eq("/opt/targetDirectory"));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_import_paths)
{
    auto importPaths = projectItemWithQdsPrefix->importPaths();

    ASSERT_THAT(importPaths, UnorderedElementsAre("imports", "asset_imports"));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_file_selectors)
{
    auto fileSelectors = projectItemWithQdsPrefix->fileSelectors();

    ASSERT_THAT(fileSelectors, UnorderedElementsAre("WXGA", "darkTheme", "ShowIndicator"));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_multi_language_support)
{
    auto multilanguageSupport = projectItemWithQdsPrefix->multilanguageSupport();

    ASSERT_TRUE(multilanguageSupport);
}

TEST_F(QmlProjectItem, get_with_qds_prefix_supported_languages)
{
    auto supportedLanguages = projectItemWithQdsPrefix->supportedLanguages();

    ASSERT_THAT(supportedLanguages, UnorderedElementsAre("en", "fr"));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_primary_language)
{
    auto primaryLanguage = projectItemWithQdsPrefix->primaryLanguage();
    ;

    ASSERT_THAT(primaryLanguage, Eq("en"));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_widget_app)
{
    auto widgetApp = projectItemWithQdsPrefix->widgetApp();

    ASSERT_TRUE(widgetApp);
}

TEST_F(QmlProjectItem, get_with_qds_prefix_file_list)
{
    QStringList fileList;
    for (const auto &file : projectItemWithQdsPrefix->files()) {
        fileList.append(file.path());
    }

    auto expectedFileList = localTestDataDir + "/getter-setter/qtquickcontrols2.conf";

    ASSERT_THAT(fileList, UnorderedElementsAre(expectedFileList));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_shader_tool_args)
{
    auto shaderToolArgs = projectItemWithQdsPrefix->shaderToolArgs();

    ASSERT_THAT(
        shaderToolArgs,
        UnorderedElementsAre("-s", "--glsl", "\"100 es,120,150\"", "--hlsl", "50", "--msl", "12"));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_shader_tool_files)
{
    auto shaderToolFiles = projectItemWithQdsPrefix->shaderToolFiles();

    ASSERT_THAT(shaderToolFiles, UnorderedElementsAre("content/shaders/*"));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_environment)
{
    auto env = projectItemWithQdsPrefix->environment();

    ASSERT_THAT(env,
                UnorderedElementsAre(
                    Utils::EnvironmentItem("QT_QUICK_CONTROLS_CONF", "qtquickcontrols2.conf")));
}

TEST_F(QmlProjectItem, get_with_qds_prefix_force_free_type)
{
    auto forceFreeType = projectItemWithQdsPrefix->forceFreeType();

    ASSERT_TRUE(forceFreeType);
}

TEST_F(QmlProjectItem, get_without_qds_prefix_main_file_project)
{
    auto mainFile = projectItemWithoutQdsPrefix->mainFile();

    ASSERT_THAT(mainFile, Eq("content/App.qml"));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_main_ui_file_project)
{
    auto mainUiFile = projectItemWithoutQdsPrefix->mainUiFile();

    ASSERT_THAT(mainUiFile, Eq("Screen01.ui.qml"));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_mcu_project)
{
    auto isMcuProject = projectItemWithoutQdsPrefix->isQt4McuProject();

    ASSERT_TRUE(isMcuProject);
}

TEST_F(QmlProjectItem, get_without_qds_prefix_qt_version)
{
    auto qtVersion = projectItemWithoutQdsPrefix->versionQt();

    ASSERT_THAT(qtVersion, Eq("6"));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_qt_quick_version)
{
    auto qtQuickVersion = projectItemWithoutQdsPrefix->versionQtQuick();

    ASSERT_THAT(qtQuickVersion, Eq("6.2"));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_design_studio_version)
{
    auto designStudioVersion = projectItemWithoutQdsPrefix->versionDesignStudio();

    ASSERT_THAT(designStudioVersion, Eq("3.9"));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_source_directory)
{
    auto sourceDirectory = projectItemWithoutQdsPrefix->sourceDirectory().path();

    auto expectedSourceDir = localTestDataDir + "/getter-setter";

    ASSERT_THAT(sourceDirectory, Eq(expectedSourceDir));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_tar_get_without_qds_prefix_directory)
{
    auto targetDirectory = projectItemWithoutQdsPrefix->targetDirectory();

    ASSERT_THAT(targetDirectory, Eq("/opt/targetDirectory"));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_import_paths)
{
    auto importPaths = projectItemWithoutQdsPrefix->importPaths();

    ASSERT_THAT(importPaths, UnorderedElementsAre("imports", "asset_imports"));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_file_selectors)
{
    auto fileSelectors = projectItemWithoutQdsPrefix->fileSelectors();

    ASSERT_THAT(fileSelectors, UnorderedElementsAre("WXGA", "darkTheme", "ShowIndicator"));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_multi_language_support)
{
    auto multilanguageSupport = projectItemWithoutQdsPrefix->multilanguageSupport();

    ASSERT_TRUE(multilanguageSupport);
}

TEST_F(QmlProjectItem, get_without_qds_prefix_supported_languages)
{
    auto supportedLanguages = projectItemWithoutQdsPrefix->supportedLanguages();

    ASSERT_THAT(supportedLanguages, UnorderedElementsAre("en", "fr"));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_primary_language)
{
    auto primaryLanguage = projectItemWithoutQdsPrefix->primaryLanguage();
    ;

    ASSERT_THAT(primaryLanguage, Eq("en"));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_widget_app)
{
    auto widgetApp = projectItemWithoutQdsPrefix->widgetApp();

    ASSERT_TRUE(widgetApp);
}

TEST_F(QmlProjectItem, get_without_qds_prefix_file_list)
{
    QStringList fileList;
    for (const auto &file : projectItemWithoutQdsPrefix->files()) {
        fileList.append(file.path());
    }

    auto expectedFileList = localTestDataDir + "/getter-setter/qtquickcontrols2.conf";

    ASSERT_THAT(fileList, UnorderedElementsAre(expectedFileList));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_shader_tool_args)
{
    auto shaderToolArgs = projectItemWithoutQdsPrefix->shaderToolArgs();

    ASSERT_THAT(
        shaderToolArgs,
        UnorderedElementsAre("-s", "--glsl", "\"100 es,120,150\"", "--hlsl", "50", "--msl", "12"));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_shader_tool_files)
{
    auto shaderToolFiles = projectItemWithoutQdsPrefix->shaderToolFiles();

    ASSERT_THAT(shaderToolFiles, UnorderedElementsAre("content/shaders/*"));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_environment)
{
    auto env = projectItemWithoutQdsPrefix->environment();

    ASSERT_THAT(env,
                UnorderedElementsAre(
                    Utils::EnvironmentItem("QT_QUICK_CONTROLS_CONF", "qtquickcontrols2.conf")));
}

TEST_F(QmlProjectItem, get_without_qds_prefix_force_free_type)
{
    auto forceFreeType = projectItemWithoutQdsPrefix->forceFreeType();

    ASSERT_TRUE(forceFreeType);
}

TEST_F(QmlProjectItem, get_empty_main_file_project)
{
    auto mainFile = projectItemEmpty->mainFile();

    ASSERT_THAT(mainFile, IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_main_ui_file_project)
{
    auto mainUiFile = projectItemEmpty->mainUiFile();

    ASSERT_THAT(mainUiFile, IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_mcu_project)
{
    auto isMcuProject = projectItemEmpty->isQt4McuProject();

    ASSERT_FALSE(isMcuProject);
}

TEST_F(QmlProjectItem, get_empty_qt_version)
{
    auto qtVersion = projectItemEmpty->versionQt();

    // default Qt Version is "5" for Design Studio projects
    ASSERT_THAT(qtVersion, Eq("5"));
}

TEST_F(QmlProjectItem, get_empty_qt_quick_version)
{
    auto qtQuickVersion = projectItemEmpty->versionQtQuick();

    ASSERT_THAT(projectItemEmpty->versionQtQuick(), IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_design_studio_version)
{
    auto designStudioVersion = projectItemEmpty->versionDesignStudio();

    ASSERT_THAT(projectItemEmpty->versionDesignStudio(), IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_source_directory)
{
    auto sourceDirectory = projectItemEmpty->sourceDirectory().path();

    auto expectedSourceDir = localTestDataDir + "/getter-setter";

    // default source directory is the project directory
    ASSERT_THAT(sourceDirectory, Eq(expectedSourceDir));
}

TEST_F(QmlProjectItem, get_empty_tar_get_empty_directory)
{
    auto targetDirectory = projectItemEmpty->targetDirectory();

    ASSERT_THAT(targetDirectory, IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_import_paths)
{
    auto importPaths = projectItemEmpty->importPaths();

    ASSERT_THAT(importPaths, IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_file_selectors)
{
    auto fileSelectors = projectItemEmpty->fileSelectors();

    ASSERT_THAT(fileSelectors, IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_multi_language_support)
{
    auto multilanguageSupport = projectItemEmpty->multilanguageSupport();

    ASSERT_FALSE(multilanguageSupport);
}

TEST_F(QmlProjectItem, get_empty_supported_languages)
{
    auto supportedLanguages = projectItemEmpty->supportedLanguages();

    ASSERT_THAT(supportedLanguages, IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_primary_language)
{
    auto primaryLanguage = projectItemEmpty->primaryLanguage();

    ASSERT_THAT(primaryLanguage, IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_widget_app)
{
    auto widgetApp = projectItemEmpty->widgetApp();

    ASSERT_FALSE(widgetApp);
}

TEST_F(QmlProjectItem, get_empty_file_list)
{
    auto fileList = projectItemEmpty->files();

    ASSERT_THAT(fileList, IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_shader_tool_args)
{
    auto shaderToolArgs = projectItemEmpty->shaderToolArgs();

    ASSERT_THAT(shaderToolArgs, IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_shader_tool_files)
{
    auto shaderToolFiles = projectItemEmpty->shaderToolFiles();

    ASSERT_THAT(shaderToolFiles, IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_environment)
{
    auto env = projectItemEmpty->environment();

    ASSERT_THAT(env, IsEmpty());
}

TEST_F(QmlProjectItem, get_empty_force_free_type)
{
    auto forceFreeType = projectItemEmpty->forceFreeType();

    ASSERT_FALSE(forceFreeType);
}

TEST_F(QmlProjectItem, set_main_file_project)
{
    projectItemSetters->setMainFile("testing");

    auto mainFile = projectItemSetters->mainFile();

    ASSERT_THAT(mainFile, Eq("testing"));
}

TEST_F(QmlProjectItem, set_main_ui_file_project)
{
    projectItemSetters->setMainUiFile("testing");

    auto mainUiFile = projectItemSetters->mainUiFile();

    ASSERT_THAT(mainUiFile, Eq("testing"));
}

TEST_F(QmlProjectItem, set_import_paths)
{
    projectItemSetters->setImportPaths({"testing"});

    auto importPaths = projectItemSetters->importPaths();

    ASSERT_THAT(importPaths, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, add_import_paths)
{
    projectItemSetters->setImportPaths({});
    projectItemSetters->addImportPath("testing");

    auto importPaths = projectItemSetters->importPaths();

    ASSERT_THAT(importPaths, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, set_file_selectors)
{
    projectItemSetters->setFileSelectors({"testing"});

    auto fileSelectors = projectItemSetters->fileSelectors();

    ASSERT_THAT(fileSelectors, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, add_file_selectors)
{
    projectItemSetters->setFileSelectors({});
    projectItemSetters->addFileSelector("testing");

    auto fileSelectors = projectItemSetters->fileSelectors();

    ASSERT_THAT(fileSelectors, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, set_multi_language_support)
{
    projectItemSetters->setMultilanguageSupport(true);

    auto multilanguageSupport = projectItemSetters->multilanguageSupport();

    ASSERT_TRUE(multilanguageSupport);
}

TEST_F(QmlProjectItem, set_supported_languages)
{
    projectItemSetters->setSupportedLanguages({"testing"});

    auto supportedLanguages = projectItemSetters->supportedLanguages();

    ASSERT_THAT(supportedLanguages, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, add_supported_languages)
{
    projectItemSetters->setSupportedLanguages({});
    projectItemSetters->addSupportedLanguage("testing");

    auto supportedLanguages = projectItemSetters->supportedLanguages();

    ASSERT_THAT(supportedLanguages, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, set_primary_language)
{
    projectItemSetters->setPrimaryLanguage("testing");

    auto primaryLanguage = projectItemSetters->primaryLanguage();
    ;

    ASSERT_THAT(primaryLanguage, Eq("testing"));
}

TEST_F(QmlProjectItem, set_widget_app)
{
    projectItemSetters->setWidgetApp(true);

    auto widgetApp = projectItemSetters->widgetApp();

    ASSERT_TRUE(widgetApp);
}

TEST_F(QmlProjectItem, set_shader_tool_args)
{
    projectItemSetters->setShaderToolArgs({"testing"});

    auto shaderToolArgs = projectItemSetters->shaderToolArgs();

    ASSERT_THAT(shaderToolArgs, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, add_shader_tool_args)
{
    projectItemSetters->setShaderToolArgs({});
    projectItemSetters->addShaderToolArg("testing");

    auto shaderToolArgs = projectItemSetters->shaderToolArgs();

    ASSERT_THAT(shaderToolArgs, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, set_shader_tool_files)
{
    projectItemSetters->setShaderToolFiles({"testing"});

    auto shaderToolFiles = projectItemSetters->shaderToolFiles();

    ASSERT_THAT(shaderToolFiles, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, add_shader_tool_files)
{
    projectItemSetters->setShaderToolFiles({});
    projectItemSetters->addShaderToolFile("testing");

    auto shaderToolFiles = projectItemSetters->shaderToolFiles();

    ASSERT_THAT(shaderToolFiles, UnorderedElementsAre("testing"));
}

TEST_F(QmlProjectItem, add_environment)
{
    projectItemSetters->addToEnviroment("testing", "testing");
    auto envs = projectItemSetters->environment();

    Utils::EnvironmentItems expectedEnvs;
    expectedEnvs.push_back({"testing", "testing"});

    ASSERT_EQ(envs, expectedEnvs);
}

TEST_F(QmlProjectItem, set_force_free_type_true)
{
    projectItemSetters->setForceFreeType(true);

    ASSERT_EQ(projectItemSetters->forceFreeType(), true);
}

TEST_F(QmlProjectItem, set_force_free_type_false)
{
    projectItemSetters->setForceFreeType(false);

    ASSERT_EQ(projectItemSetters->forceFreeType(), false);
}

TEST_F(QmlProjectItem, set_qt_version)
{
    projectItemSetters->setVersionQt("6");

    ASSERT_EQ(projectItemSetters->versionQt().toStdString(), "6");
}

TEST_F(QmlProjectItem, set_qt_quick_version)
{
    projectItemSetters->setVersionQtQuick("6");

    ASSERT_EQ(projectItemSetters->versionQtQuick(), "6");
}

TEST_F(QmlProjectItem, set_design_studio_version)
{
    projectItemSetters->setVersionDesignStudio("6");

    ASSERT_EQ(projectItemSetters->versionDesignStudio(), "6");
}

// TODO: We should move these 2 tests into the integration tests
TEST_F(QmlProjectItem, test_file_filters)
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

TEST_F(QmlProjectItem, matches_file)
{
    // GIVEN
    auto fileSearched = localTestDataDir + "/file-filters/content/MaterialNames.qml";

    // WHEN
    auto fileFound = projectItemFileFilters->matchesFile(fileSearched);

    // THEN
    ASSERT_TRUE(fileFound);
}

TEST_F(QmlProjectItem, not_matches_file)
{
    // GIVEN
    auto fileSearched = localTestDataDir + "/file-filters/content/non-existing-file.qwerty";

    // WHEN
    auto fileFound = projectItemFileFilters->matchesFile(fileSearched);

    // THEN
    ASSERT_FALSE(fileFound);
}

} // namespace
