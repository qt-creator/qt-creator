// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h" // IWYU pragma: keep

#include <qmlprojectmanager/qmldirtoqmlproject/mcumoduleprojectitem.h>

#include <QJsonDocument>

namespace {

constexpr QLatin1String localTestDataDir{UNITTEST_DIR "/qmlprojectmanager/data"};

constexpr QLatin1String jsonProject{R"(
{
    "moduleUri": "test.module",
    "qmlFiles": [
        "TestSingleton.qml",
        "File1.qml",
        "File2.qml",
        "Internal/File3.qml"
    ],
    "qmlProjectPath": "%1"
}
)"};

class McuModuleProjectItem : public testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        existingQmlProject = std::make_unique<const QmlProjectManager::McuModuleProjectItem>(
            Utils::FilePath::fromString(
                localTestDataDir + "/qmldirtoqmlproject/existing_qmlproject/test_module.qmlproject"));

        auto fromQmldir = QmlProjectManager::McuModuleProjectItem::fromQmldirModule(
            Utils::FilePath::fromString(localTestDataDir
                                        + "/qmldirtoqmlproject/missing_qmlproject/qmldir"));
        missingQmlProject = std::make_unique<const QmlProjectManager::McuModuleProjectItem>(
            fromQmldir ? *fromQmldir : QmlProjectManager::McuModuleProjectItem{QJsonObject{}});

        invalidQmlProject = std::make_unique<const QmlProjectManager::McuModuleProjectItem>(
            Utils::FilePath::fromString(
                localTestDataDir + "/qmldirtoqmlproject/invalid_qmlproject/test_module.qmlproject"));

        fromJsonObject = std::make_unique<const QmlProjectManager::McuModuleProjectItem>(
            QJsonDocument::fromJson(
                jsonProject.arg(localTestDataDir + "/qmldirtoqmlproject/test_module.qmlproject").toUtf8())
                .object());

        fromIncompleteJsonObject = std::make_unique<const QmlProjectManager::McuModuleProjectItem>(
            QJsonDocument::fromJson(jsonProject.toString().toUtf8()).object());

        createFromEmpty = std::make_unique<QmlProjectManager::McuModuleProjectItem>(QJsonObject{});
    }

    static void TearDownTestSuite()
    {
        existingQmlProject.reset();
        missingQmlProject->qmlProjectPath().removeFile();
        missingQmlProject.reset();
        invalidQmlProject.reset();
        fromJsonObject.reset();
        fromIncompleteJsonObject.reset();
        createFromEmpty->qmlProjectPath().removeFile();
        createFromEmpty.reset();
    }

protected:
    inline static std::unique_ptr<const QmlProjectManager::McuModuleProjectItem> existingQmlProject;
    inline static std::unique_ptr<const QmlProjectManager::McuModuleProjectItem> missingQmlProject;
    inline static std::unique_ptr<const QmlProjectManager::McuModuleProjectItem> invalidQmlProject;
    inline static std::unique_ptr<const QmlProjectManager::McuModuleProjectItem> fromJsonObject;
    inline static std::unique_ptr<const QmlProjectManager::McuModuleProjectItem> fromIncompleteJsonObject;
    inline static std::unique_ptr<QmlProjectManager::McuModuleProjectItem> createFromEmpty;
};
} // namespace

TEST_F(McuModuleProjectItem, is_valid_existing_qmlproject)
{
    auto isValid = existingQmlProject->isValid();

    ASSERT_TRUE(isValid);
}

TEST_F(McuModuleProjectItem, get_uri_existing_qmlproject)
{
    auto uri = existingQmlProject->uri();

    ASSERT_THAT(uri, Eq("test.module"));
}

TEST_F(McuModuleProjectItem, get_qml_files_existing_qmlproject)
{
    auto files = existingQmlProject->qmlFiles();

    ASSERT_THAT(files,
                UnorderedElementsAre("Internal/File3.qml", "File2.qml", "File1.qml", "TestSingleton.qml"));
}

TEST_F(McuModuleProjectItem, get_qmlproject_path_existing_qmlproject)
{
    auto path = existingQmlProject->qmlProjectPath();
    auto expectedPath = Utils::FilePath::fromString(
        localTestDataDir + "/qmldirtoqmlproject/existing_qmlproject/test_module.qmlproject");

    ASSERT_THAT(path, Eq(expectedPath));
}

TEST_F(McuModuleProjectItem, get_project_existing_qmlproject)
{
    auto project = existingQmlProject->project();
    auto expectedJsonProject = QJsonDocument::fromJson(
                                   jsonProject
                                       .arg(localTestDataDir + "/qmldirtoqmlproject/existing_qmlproject/test_module.qmlproject")
                                       .toUtf8())
                                   .object();

    ASSERT_THAT(project, Eq(expectedJsonProject));
}

TEST_F(McuModuleProjectItem, save_qmlproject_file_existing_qmlproject)
{
    bool saved = existingQmlProject->saveQmlProjectFile();

    ASSERT_FALSE(saved);
}

TEST_F(McuModuleProjectItem, is_valid_missing_qmlproject)
{
    auto isValid = missingQmlProject->isValid();

    ASSERT_TRUE(isValid);
}

TEST_F(McuModuleProjectItem, get_uri_missing_qmlproject)
{
    auto uri = missingQmlProject->uri();

    ASSERT_THAT(uri, Eq("test.module"));
}

TEST_F(McuModuleProjectItem, get_qml_files_missing_qmlproject)
{
    auto files = missingQmlProject->qmlFiles();

    ASSERT_THAT(files,
                UnorderedElementsAre("Internal/File3.qml", "File2.qml", "File1.qml", "TestSingleton.qml"));
}

TEST_F(McuModuleProjectItem, get_qmlproject_path_missing_qmlproject)
{
    auto path = missingQmlProject->qmlProjectPath();
    auto expectedPath = Utils::FilePath::fromString(
        localTestDataDir + "/qmldirtoqmlproject/missing_qmlproject/test_module.qmlproject");

    ASSERT_THAT(path, Eq(expectedPath));
}

TEST_F(McuModuleProjectItem, check_saved_qmlproject_file_missing_qmlproject)
{
    auto projectPath = Utils::FilePath::fromString(
        localTestDataDir + "/qmldirtoqmlproject/missing_qmlproject/test_module.qmlproject");
    missingQmlProject->saveQmlProjectFile();

    QmlProjectManager::McuModuleProjectItem savedQmlProject(projectPath);

    ASSERT_THAT(*missingQmlProject, Eq(savedQmlProject));
}

TEST_F(McuModuleProjectItem, is_valid_invalid_qmlproject)
{
    auto isValid = invalidQmlProject->isValid();

    ASSERT_FALSE(isValid);
}

TEST_F(McuModuleProjectItem, get_uri_invalid_qmlproject)
{
    auto uri = invalidQmlProject->uri();

    ASSERT_THAT(uri, IsEmpty());
}

TEST_F(McuModuleProjectItem, get_qml_files_invalid_qmlproject)
{
    auto files = invalidQmlProject->qmlFiles();

    ASSERT_THAT(files, IsEmpty());
}

TEST_F(McuModuleProjectItem, get_qmlproject_path_invalid_qmlproject)
{
    auto path = invalidQmlProject->qmlProjectPath();
    auto expectedPath = Utils::FilePath::fromString(
        localTestDataDir + "/qmldirtoqmlproject/invalid_qmlproject/test_module.qmlproject");

    ASSERT_THAT(path, Eq(expectedPath));
}

TEST_F(McuModuleProjectItem, save_qmlproject_file_invalid_qmlproject)
{
    bool saved = invalidQmlProject->saveQmlProjectFile();

    ASSERT_FALSE(saved);
}

TEST_F(McuModuleProjectItem, is_valid_from_json_object)
{
    auto isValid = fromJsonObject->isValid();

    ASSERT_TRUE(isValid);
}

TEST_F(McuModuleProjectItem, get_uri_from_json_object)
{
    auto uri = fromJsonObject->uri();

    ASSERT_THAT(uri, Eq("test.module"));
}

TEST_F(McuModuleProjectItem, get_qml_files_from_json_object)
{
    auto files = fromJsonObject->qmlFiles();

    ASSERT_THAT(files,
                UnorderedElementsAre("Internal/File3.qml", "File2.qml", "File1.qml", "TestSingleton.qml"));
}

TEST_F(McuModuleProjectItem, get_qmlproject_path_from_json_object)
{
    auto path = fromJsonObject->qmlProjectPath();
    auto expectedPath = Utils::FilePath::fromString(localTestDataDir
                                                    + "/qmldirtoqmlproject/test_module.qmlproject");

    ASSERT_THAT(path, Eq(expectedPath));
}

TEST_F(McuModuleProjectItem, get_project_from_json_object)
{
    auto project = fromJsonObject->project();
    auto expectedJsonProject = QJsonDocument::fromJson(
                                   jsonProject
                                       .arg(localTestDataDir
                                            + "/qmldirtoqmlproject/test_module.qmlproject")
                                       .toUtf8())
                                   .object();

    ASSERT_THAT(project, Eq(expectedJsonProject));
}

TEST_F(McuModuleProjectItem, is_valid_from_incomplete_json_object)
{
    auto isValid = fromIncompleteJsonObject->isValid();

    ASSERT_FALSE(isValid);
}

TEST_F(McuModuleProjectItem, get_uri_from_incomplete_json_object)
{
    auto uri = fromIncompleteJsonObject->uri();

    ASSERT_THAT(uri, Eq("test.module"));
}

TEST_F(McuModuleProjectItem, get_qml_files_from_incomplete_json_object)
{
    auto files = fromIncompleteJsonObject->qmlFiles();

    ASSERT_THAT(files,
                UnorderedElementsAre("Internal/File3.qml", "File2.qml", "File1.qml", "TestSingleton.qml"));
}

TEST_F(McuModuleProjectItem, get_qmlproject_path_from_incomplete_json_object)
{
    auto path = fromIncompleteJsonObject->qmlProjectPath();

    ASSERT_FALSE(path.endsWith(".qmlproject"));
}

TEST_F(McuModuleProjectItem, save_qmlproject_from_incomplete_json_object)
{
    bool saved = fromIncompleteJsonObject->saveQmlProjectFile();

    ASSERT_FALSE(saved);
}

TEST_F(McuModuleProjectItem, set_uri_create_from_empty)
{
    createFromEmpty->setUri("test.module");

    ASSERT_THAT(createFromEmpty->uri(), Eq("test.module"));
}

TEST_F(McuModuleProjectItem, set_qml_files_create_from_empty)
{
    createFromEmpty->setQmlFiles({"File1.qml", "File2.qml"});

    ASSERT_THAT(createFromEmpty->qmlFiles(), UnorderedElementsAre("File1.qml", "File2.qml"));
}

TEST_F(McuModuleProjectItem, set_qmlproject_path_create_from_empty)
{
    auto projectPath = Utils::FilePath::fromString(localTestDataDir
                                                   + "/qmldirtoqmlproject/test_module.qmlproject");

    createFromEmpty->setQmlProjectPath(projectPath);

    ASSERT_THAT(createFromEmpty->qmlProjectPath(), Eq(projectPath));
}

TEST_F(McuModuleProjectItem, is_valid_create_from_empty)
{
    bool isValid = createFromEmpty->isValid();

    ASSERT_TRUE(isValid);
}

TEST_F(McuModuleProjectItem, check_saved_qmlproject_create_from_empty)
{
    auto projectPath = Utils::FilePath::fromString(localTestDataDir
                                                   + "/qmldirtoqmlproject/test_module.qmlproject");
    createFromEmpty->saveQmlProjectFile();

    QmlProjectManager::McuModuleProjectItem savedQmlProject(projectPath);

    ASSERT_THAT(*createFromEmpty, Eq(savedQmlProject));
}

TEST_F(McuModuleProjectItem, create_from_nonexisting_file)
{
    auto projectPath = Utils::FilePath::fromString(localTestDataDir
                                                   + "/qmldirtoqmlproject/nonexisting");

    auto projectItem = QmlProjectManager::McuModuleProjectItem::fromQmldirModule(projectPath);

    ASSERT_FALSE(projectItem);
}

TEST_F(McuModuleProjectItem, create_from_missing_module_name_qmldir)
{
    auto projectPath = Utils::FilePath::fromString(
        localTestDataDir + "/qmldirtoqmlproject/missing_module_name_qmldir/qmldir");

    auto projectItem = QmlProjectManager::McuModuleProjectItem::fromQmldirModule(projectPath);

    ASSERT_FALSE(projectItem);
}

TEST_F(McuModuleProjectItem, create_from_incorrect_module_name_qmldir)
{
    auto projectPath = Utils::FilePath::fromString(
        localTestDataDir + "/qmldirtoqmlproject/incorrect_module_name_qmldir/qmldir");

    auto projectItem = QmlProjectManager::McuModuleProjectItem::fromQmldirModule(projectPath);

    ASSERT_FALSE(projectItem);
}

TEST_F(McuModuleProjectItem, create_from_missing_qml_files_qmldir)
{
    auto projectPath = Utils::FilePath::fromString(
        localTestDataDir + "/qmldirtoqmlproject/missing_qml_files_qmldir/qmldir");

    auto projectItem = QmlProjectManager::McuModuleProjectItem::fromQmldirModule(projectPath);

    ASSERT_FALSE(projectItem);
}
