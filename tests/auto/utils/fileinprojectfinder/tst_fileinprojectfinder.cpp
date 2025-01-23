// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>

#include <utils/fileinprojectfinder.h>
#include <utils/hostosinfo.h>

using namespace Utils;

class tst_fileinprojectfinder : public QObject
{
    Q_OBJECT

private:
    Utils::FilePath makePath(const Utils::FilePath &path)
    {
        auto created = path.ensureWritableDir();
        QTC_CHECK(created);
        return path;
    }
    Utils::FilePath makeFile(const Utils::FilePath &filePath)
    {
        makePath(filePath.parentDir());
        QFile file(filePath.path());
        file.open(QIODevice::WriteOnly);
        file.close();
        return filePath;
    }
    Utils::FilePath makeTempPath()
    {
        return makePath(FilePath::fromString(QDir::tempPath()) / "FileInProjectFinder_test");
    }

    Utils::FileInProjectFinder finder;
    Utils::FilePath tempPath;

private slots:
    void cleanup()
    {
        finder.setProjectDirectory({});
        finder.setProjectFiles({});
        tempPath.removeRecursively();
        tempPath = makeTempPath();
    }

    void initTestCase()
    {
        tempPath = makeTempPath();
    }

    void find_existing_file_in_project_directory()
    {
        FilePath testFile = makeFile(tempPath.pathAppended("testfile.txt"));
        finder.setProjectDirectory(tempPath);
        finder.setProjectFiles({testFile});
        QUrl testFileUrl = testFile.toUrl();
        bool success = false;

        auto result = finder.findFile(testFileUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{testFile});
    }

    void find_file_in_sysroot_when_not_in_project()
    {
        FilePath sysrootPath = makePath(tempPath.pathAppended("sysroot"));
        FilePath fileInSysroot = makeFile(sysrootPath.pathAppended("file.txt"));
        finder.setProjectDirectory(tempPath);
        finder.setProjectFiles({fileInSysroot});
        finder.setSysroot(sysrootPath);
        QUrl fileInSysrootUrl = fileInSysroot.toUrl();
        bool success = false;

        auto result = finder.findFile(fileInSysrootUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{fileInSysroot});
    }

    void map_shadow_build_file_to_original_source()
    {
        FilePath projectFilePath = tempPath.pathAppended("qml");
        FilePath originalFilePath = makeFile(projectFilePath.pathAppended("main.qml"));
        FilePath shadowBuildFilePath = makeFile(tempPath.pathAppended("shadow-build/qml/main.qml"));
        finder.setProjectDirectory(projectFilePath);
        finder.setProjectFiles({originalFilePath});
        QUrl shadowBuildMainQmlUrl = shadowBuildFilePath.toUrl();
        bool success = false;

        auto result = finder.findFile(shadowBuildMainQmlUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{originalFilePath});
    }

    void find_file_when_project_directory_is_empty()
    {
        FilePath projectFile = makeFile(tempPath.pathAppended("file.txt"));
        finder.setProjectDirectory({});
        finder.setProjectFiles({projectFile});
        QUrl projectUrl = projectFile.toUrl();
        bool success = false;

        auto result = finder.findFile(projectUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{projectFile});
    }

    void find_file_in_added_search_directories()
    {
        FilePath additionalSearchPath = makePath(tempPath.pathAppended("extra/search/path"));
        FilePath fileInSearchPath = makeFile(additionalSearchPath.pathAppended("file.txt"));
        finder.setAdditionalSearchDirectories({additionalSearchPath});
        QUrl searchPathUrl = fileInSearchPath.toUrl();
        bool success = false;

        auto result = finder.findFile(searchPathUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{fileInSearchPath});
    }

    void find_file_using_relative_path()
    {
        FilePath relativeFilePath = makeFile(tempPath.pathAppended("subdir/file.txt"));
        finder.setProjectDirectory(tempPath);
        finder.setProjectFiles({relativeFilePath});
        QUrl relativeUrl = QUrl::fromLocalFile("subdir/file.txt");
        bool success = false;

        auto result = finder.findFile(relativeUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{relativeFilePath});
    }

    void prioritize_project_files_over_sysroot()
    {
        FilePath projectFile = makeFile(tempPath.pathAppended("project/testfile.txt"));
        FilePath sysrootFile = makeFile(tempPath.pathAppended("sysroot/testfile.txt"));
        finder.setProjectDirectory(tempPath.pathAppended("project"));
        finder.setProjectFiles({projectFile});
        finder.setSysroot(tempPath.pathAppended("sysroot"));
        QUrl testFileUrl = projectFile.toUrl();
        bool success = false;

        auto result = finder.findFile(testFileUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{projectFile});
    }

    void find_relative_file_in_additional_search_paths()
    {
        FilePath additionalSearchPath = makePath(tempPath.pathAppended("extra/search"));
        FilePath fileInSearchPath = makeFile(
            additionalSearchPath.pathAppended("relative/testfile.txt"));
        finder.setAdditionalSearchDirectories({additionalSearchPath});
        QUrl relativeUrl = QUrl::fromLocalFile("relative/testfile.txt");
        bool success = false;

        auto result = finder.findFile(relativeUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{fileInSearchPath});
    }

    void resolve_nested_mapped_paths()
    {
        FilePath localFile = makeFile(tempPath.pathAppended("local/nested/path/file.qml"));
        finder.addMappedPath(localFile, "remote/nested/path/file.qml");
        QUrl remoteUrl("remote/nested/path/file.qml");
        bool success = false;

        auto result = finder.findFile(remoteUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{localFile});
    }


    void find_file_using_single_mapped_path()
    {
        FilePath localFile = makeFile(tempPath.pathAppended("local/testfile.txt"));
        finder.addMappedPath(localFile, "remote/testfile.txt");
        QUrl testFileUrl("remote/testfile.txt");
        bool success = false;

        auto result = finder.findFile(testFileUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{localFile});
    }

    void find_file_in_nested_mapped_paths()
    {
        FilePath mappedFile1 = makeFile(tempPath.pathAppended("local/path1/deep/file.qml"));
        FilePath mappedFile2 = makeFile(tempPath.pathAppended("local/path2/deeper/file.qml"));
        finder.addMappedPath(mappedFile1, "remote/path1/file.qml");
        finder.addMappedPath(mappedFile2, "remote/path2/file.qml");
        QUrl remoteUrl("remote/path2/file.qml");
        bool success = false;

        auto result = finder.findFile(remoteUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{mappedFile2});
    }

    void prioritize_project_files_over_search_and_sysroot()
    {
        FilePath projectFile = makeFile(tempPath.pathAppended("project/file.qml"));
        FilePath sysrootFile = makeFile(tempPath.pathAppended("sysroot/file.qml"));
        FilePath deploymentFile = makeFile(tempPath.pathAppended("deployment/file.qml"));
        finder.setProjectDirectory(tempPath.pathAppended("project"));
        finder.setProjectFiles({projectFile});
        finder.setSysroot(tempPath.pathAppended("sysroot"));
        finder.setAdditionalSearchDirectories({tempPath.pathAppended("deployment")});
        QUrl searchUrl = QUrl::fromLocalFile("file.qml");
        bool success = false;

        auto result = finder.findFile(searchUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{projectFile});
    }

    void select_file_with_most_matching_path_segments()
    {
        FilePath file1 = makeFile(tempPath.pathAppended("qml/1/view.qml"));
        FilePath file2 = makeFile(tempPath.pathAppended("qml/2/view.qml"));
        FilePath searchPath = FilePath::fromString("/somewhere/else/qml/2/view.qml");
        finder.setProjectDirectory(tempPath);
        finder.setProjectFiles({file1, file2});
        QUrl searchPathUrl = searchPath.toUrl();
        bool success = false;

        auto result = finder.findFile(searchPathUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{file2});
    }

    void avoid_speculative_qmldir_file_matches()
    {
        FilePath exactQmldir = makeFile(tempPath.pathAppended("qml/2/qmldir"));
        FilePath speculativeQmldir = makeFile(tempPath.pathAppended("resources/qml/1/qmldir"));

        finder.setProjectDirectory(tempPath);
        finder.setProjectFiles({exactQmldir, speculativeQmldir});

        QUrl searchUrl = QUrl::fromLocalFile("/external/qml/2/qmldir");
        bool success = false;

        auto result = finder.findFile(searchUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{exactQmldir});
    }

    void map_qrc_url_to_project_file()
    {
        FilePath qrcFilePath = makeFile(tempPath.pathAppended("qml/main.qml"));
        finder.setProjectDirectory(tempPath);
        finder.setProjectFiles({qrcFilePath});
        QUrl qrcUrl("qrc:/qml/main.qml");
        bool success = false;

        auto result = finder.findFile(qrcUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{qrcFilePath});
    }

    void fallback_to_filepath_on_invalid_qrc_path()
    {
        QUrl invalidQrcUrl("qrc:/invalid/path/file.qml");
        bool success = false;

        auto result = finder.findFile(invalidQrcUrl, &success);

        QVERIFY(!success);
        QCOMPARE(result.size(), 1); // Fallback
        QCOMPARE(result, FilePaths{FilePath::fromUrl(invalidQrcUrl.path())});
    }

    void return_original_path_when_file_not_found()
    {
        FilePath noneExistentFilePath = tempPath.pathAppended("nonexistent.txt");
        QUrl noneExistingUrl = noneExistentFilePath.toUrl();
        bool success = false;

        auto result = finder.findFile(noneExistingUrl, &success);

        QVERIFY(!success);
        QCOMPARE(result.size(), 1); // Fallback
        QCOMPARE(result, FilePaths{noneExistentFilePath});
    }

    void ignore_unsupported_protocols_in_file_search()
    {
        QUrl unsupportedUrl("ftp://example.com/testfile.txt");
        bool success = false;

        auto result = finder.findFile(unsupportedUrl, &success);

        QVERIFY(!success);
        QCOMPARE(result.size(), 1); // Fallback
        QCOMPARE(result, FilePaths{FilePath::fromString("/testfile.txt")});
    }

    void testMacAppBundleIgnore()
    {
        FilePath projectPath = makePath(tempPath.pathAppended("project"));
        FilePath appBundleFile = makeFile(projectPath.pathAppended(".build/MyApp.app/Contents/Resources/qml/ignored.qml"));
        finder.setProjectDirectory(projectPath);
        QUrl appBundleUrl = appBundleFile.toUrl();
        bool success = false;

        auto result = finder.findFile(appBundleUrl, &success);

        if (Utils::HostOsInfo::isMacHost()) {
            QVERIFY(!success);
            QCOMPARE(result.size(), 1); // Fallback
            QVERIFY(result.contains(appBundleFile));
        } else {
            QVERIFY(success);
            QVERIFY(result.contains(appBundleFile));
        }
    }

    void testMacAppBundleVsProjectFile()
    {
        FilePath projectPath = makePath(tempPath.pathAppended("project"));
        FilePath appBundleFile = makeFile(projectPath.pathAppended("MyApp.app/Contents/Resources/qml/same.qml"));
        FilePath deeperFile = makeFile(projectPath.pathAppended("subdir/subdir/subdir/subdir/subdir/subdir/same.qml"));
        finder.setProjectDirectory(projectPath);
        finder.setProjectFiles({deeperFile});
        QUrl appBundleUrl = appBundleFile.toUrl();
        bool success = false;

        auto result = finder.findFile(appBundleUrl, &success);

        if (Utils::HostOsInfo::isMacHost()) {
            QVERIFY(success);
            QVERIFY(result.contains(deeperFile));
            QVERIFY(!result.contains(appBundleFile));
        } else {
            QVERIFY(success);
            QVERIFY(!result.contains(deeperFile));
            QVERIFY(result.contains(appBundleFile));
        }
    }


    void search_deployment_data_for_paths()
    {
        FilePath deploymentPath = makePath(tempPath.pathAppended("deployment/data"));
        FilePath deployedFile = makeFile(deploymentPath.pathAppended("deployed.qml"));
        finder.setAdditionalSearchDirectories({deploymentPath});
        QUrl deploymentUrl = deployedFile.toUrl();
        bool success = false;

        auto result = finder.findFile(deploymentUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{deployedFile});
    }

    void strip_directories_in_project_directory_search()
    {
        FilePath projectDir = makePath(tempPath.pathAppended("project"));
        FilePath deepFile = makeFile(projectDir.pathAppended("nested/subdir/file.qml"));
        finder.setProjectDirectory(projectDir);
        finder.setProjectFiles({deepFile});
        QUrl searchUrl = QUrl::fromLocalFile("subdir/file.qml");
        bool success = false;

        auto result = finder.findFile(searchUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{deepFile});
    }

    void call_file_handler_when_searching_for_file()
    {
        FilePath projectDir = makePath(tempPath.pathAppended("project"));
        FilePath filePath = makeFile(projectDir.pathAppended("file.qml"));

        finder.setProjectDirectory(projectDir);
        finder.setProjectFiles({filePath});

        int callCount = 0;
        auto fileHandler = [&](const FilePath &fn, int) {
            QCOMPARE(fn, filePath);
            ++callCount;
        };

        bool success = finder.findFileOrDirectory(filePath, fileHandler, nullptr);

        QVERIFY(success);
        QCOMPARE(callCount, 1);
    }

    void call_directory_handler_when_searching_for_directory()
    {
        FilePath projectDir = makePath(tempPath.pathAppended("project"));
        FilePath directoryPath = makePath(projectDir.pathAppended("subdir"));
        makeFile(directoryPath.pathAppended("file.qml"));

        finder.setProjectDirectory(projectDir);

        int callCount = 0;
        auto directoryHandler = [&](const QStringList &entries, int) {
            QVERIFY(entries.contains("file.qml"));
            ++callCount;
        };

        bool success = finder.findFileOrDirectory(directoryPath, nullptr, directoryHandler);

        QVERIFY(success);
        QCOMPARE(callCount, 1);
    }

    void directory_handler_receives_correct_entries()
    {
        finder.setProjectDirectory(tempPath);
        FilePath directoryPath = makePath(tempPath.pathAppended("subdir"));

        int callCount = 0;
        auto directoryHandler = [&](const QStringList &entries, int) {
            QVERIFY(entries.contains("."));
            QVERIFY(entries.contains(".."));
            QCOMPARE(entries.size(), 2);
            ++callCount;
        };

        finder.findFileOrDirectory(directoryPath, nullptr, directoryHandler);
        QCOMPARE(callCount, 1);
    }

    void return_multiple_candidates_for_relative_path()
    {
        FilePath file1 = makeFile(tempPath.pathAppended("dir1/shared_file.qml"));
        FilePath file2 = makeFile(tempPath.pathAppended("dir2/shared_file.qml"));
        finder.setProjectDirectory(tempPath);
        finder.setProjectFiles({file1, file2});
        QUrl relativeUrl("shared_file.qml");
        bool success = false;

        auto result = finder.findFile(relativeUrl, &success);

        QVERIFY(success);
        QVERIFY(result.contains(file1));
        QVERIFY(result.contains(file2));
        QCOMPARE(result.length(), 2);
    }

    void qrc_single_match()
    {
        FilePath qrcFile = makeFile(tempPath.pathAppended("test_single.qrc"));
        qrcFile.writeFileContents(R"(<RCC><qresource prefix="/"><file>qml/single.qml</file></qresource></RCC>)");
        FilePath qmlFile = makeFile(tempPath.pathAppended("qml/single.qml"));
        finder.setProjectFiles({qrcFile});
        QUrl qrcUrl("qrc:/qml/single.qml");
        bool success = false;

        auto result = finder.findFile(qrcUrl, &success);

        QVERIFY(success);
        QVERIFY(!result.isEmpty());
        QVERIFY(result.contains(qmlFile));
    }

    void qrc_no_match()
    {
        FilePath qrcFile = makeFile(tempPath.pathAppended("test_none.qrc"));
        qrcFile.writeFileContents(R"(<RCC><qresource prefix="/"><file>qml/empty.qml</file></qresource></RCC>)");
        finder.setProjectFiles({qrcFile});
        QUrl notExistingInQrcUrl("qrc:/qml/missing.qml");
        // removing the scheme even it could not find anything, feels wired but this is how it was implemented
        FilePath fallBackResult = FilePath::fromString(notExistingInQrcUrl.path());
        bool success = false;

        auto result = finder.findFile(notExistingInQrcUrl, &success);

        QVERIFY(!success);
        QVERIFY(result.contains(fallBackResult));
    }

    void multiple_qrc()
    {
        FilePath qrcFile1 = makeFile(tempPath.pathAppended("test_multi1.qrc"));
        qrcFile1.writeFileContents(R"(<RCC><qresource prefix="/"><file>qml/first.qml</file></qresource></RCC>)");
        FilePath qrcFile2 = makeFile(tempPath.pathAppended("test_multi2.qrc"));
        qrcFile2.writeFileContents(R"(<RCC><qresource prefix="/"><file>qml/second.qml</file></qresource></RCC>)");
        FilePath second = tempPath.pathAppended("qml/second.qml");
        finder.setProjectFiles({qrcFile1, qrcFile2});
        bool success = false;

        auto result = finder.findFile(QUrl("qrc:/qml/second.qml"), &success);

        QVERIFY(success);
        QCOMPARE(result.size(), 1);
        QVERIFY(result.contains(second));
    }

    void trigger_directoryHandler_for_mapped_path()
    {
        FilePath mappedDir = tempPath.pathAppended("maps/dir");
        // localFilePath empty => triggers directoryHandler
        finder.addMappedPath({}, "remote/dir");
        int callCount = 0;
        auto dirHandler = [&](const QStringList &entries, int) {
            Q_UNUSED(entries)
            ++callCount;
        };

        bool found = finder.findFileOrDirectory(FilePath::fromString("remote/dir"), nullptr, dirHandler);

        QVERIFY(found);
        QCOMPARE(callCount, 1);
    }

    void use_cached_result_for_second_search()
    {
        FilePath existing = makeFile(tempPath.pathAppended("cached_item.txt"));
        finder.setProjectDirectory(tempPath);
        finder.setProjectFiles({existing});
        QUrl url = existing.toUrl();

        // 1st search -> fills cache
        bool success1 = false;
        auto firstResult = finder.findFile(url, &success1);
        QVERIFY(success1);
        QVERIFY(firstResult.contains(existing));

        // 2nd search -> triggers the "checking cache..." debug line
        bool success2 = false;
        auto secondResult = finder.findFile(url, &success2);
        QVERIFY(success2);
        QVERIFY(secondResult.contains(existing));
    }

    void only_in_sysroot_returns_from_line_302()
    {
        // Make sure it doesn't find it in the project or search paths, but finds it in sysroot.
        FilePath sysRoot = makePath(tempPath.pathAppended("mySysroot"));
        FilePath inSysroot = makeFile(sysRoot.pathAppended("sysroot_only.qml"));
        FilePath projectDir = makePath(tempPath.pathAppended("fakeProject"));
        finder.setProjectDirectory(projectDir);
        finder.setProjectFiles({});
        finder.setAdditionalSearchDirectories({});
        finder.setSysroot(sysRoot);
        QUrl searchUrl(QStringLiteral("sysroot_only.qml"));
        bool success = false;

        auto result = finder.findFile(searchUrl, &success);

        QVERIFY(success);
        QCOMPARE(result, FilePaths{inSysroot});
    }

    void directory_handler_matches_searchPath_itself()
    {
        // Additional search directory is named "theDir", and we search for "theDir" as a relative path:
        FilePath theDir = makePath(tempPath.pathAppended("some/theDir"));
        finder.setAdditionalSearchDirectories({theDir});
        int callCount = 0;
        auto directoryHandler = [&](const QStringList &entries, int matchLength) {
            Q_UNUSED(matchLength)
            ++callCount;
            QVERIFY(entries.contains("."));
            QVERIFY(entries.contains(".."));
        };

        bool found = finder.findFileOrDirectory(FilePath::fromString("theDir"), nullptr, directoryHandler);

        QVERIFY(found);
        QCOMPARE(callCount, 1);
    }

    void pathSegmentsWithSameName_multiple_directories()
    {
        // We want to confirm a non-empty result.
        FilePath projectDir = makePath(tempPath.pathAppended("projectSegments"));
        FilePath c1 = makePath(projectDir.pathAppended("some/commonDir"));
        FilePath c2 = makePath(projectDir.pathAppended("another/commonDir"));
        makeFile(c1.pathAppended("fileA.txt"));
        makeFile(c2.pathAppended("fileB.txt"));
        finder.setProjectDirectory(projectDir);
        finder.setProjectFiles({c1.pathAppended("fileA.txt"), c2.pathAppended("fileB.txt")});
        int dirCount = 0;
        auto directoryHandler = [&](const QStringList &entries, int) {
            if (entries.contains("fileA.txt"))
                ++dirCount;
            else if (entries.contains("fileB.txt"))
                ++dirCount;
        };

        bool success = finder.findFileOrDirectory(FilePath::fromString("commonDir"), nullptr, directoryHandler);

        QVERIFY(success);
        QCOMPARE(dirCount, 2);
    }


    void qrcUrlFinder_cache_hit()
    {
        FilePath qrcFile = makeFile(tempPath.pathAppended("cache_hit.qrc"));
        qrcFile.writeFileContents(R"(<RCC><qresource prefix="/"><file>qml/cached.qml</file></qresource></RCC>)");
        FilePath qmlFile = makeFile(tempPath.pathAppended("qml/cached.qml"));
        finder.setProjectFiles({qrcFile});
        // create cache entry
        QUrl qrcUrl("qrc:/qml/cached.qml");
        bool successFirst = false;
        auto firstResult = finder.findFile(qrcUrl, &successFirst);
        QVERIFY(successFirst);
        QVERIFY(!firstResult.isEmpty());
        QVERIFY(firstResult.contains(qmlFile));
        bool successSecond = false;

        auto secondResult = finder.findFile(qrcUrl, &successSecond);

        QVERIFY(successSecond);
        QVERIFY(!secondResult.isEmpty());
        QVERIFY(secondResult.contains(qmlFile));
    }

    void cached_path_invalidation_removes_partial_and_full()
    {
        FilePath projectDir = makePath(tempPath.pathAppended("ProjectForInvalidation"));
        FilePath fileA = makeFile(projectDir.pathAppended("subdirA/multi.qml"));
        FilePath fileB = makeFile(projectDir.pathAppended("subdirB/multi.qml"));
        finder.setProjectDirectory(projectDir);
        finder.setProjectFiles({fileA, fileB});
        QUrl searchUrl("multi.qml");
        bool success1 = false;
        auto result1 = finder.findFile(searchUrl, &success1);
        QVERIFY(success1);
        QCOMPARE(result1.size(), 2);
        QVERIFY(result1.contains(fileA));
        QVERIFY(result1.contains(fileB));

        // second run with missing file
        fileA.removeFile();
        bool success2 = false;
        auto result2 = finder.findFile(searchUrl, &success2);
        QVERIFY(success2);
        QCOMPARE(result2.size(), 1);
        QVERIFY(!result2.contains(fileA));
        QVERIFY(result2.contains(fileB));

        // third remove all files so even the path is not valid
        fileB.removeFile();
        bool success3 = false;

        auto result3 = finder.findFile(searchUrl, &success3);

        QVERIFY(!success3);
        QCOMPARE(result3.size(), 1); // Fallback
        QCOMPARE(result3.front().path(), QStringLiteral("multi.qml"));
    }

    void check_root_directory_content()
    {
        FilePath testProjectDirectory = makePath(tempPath.pathAppended("UntitledProject68"));
        FilePath subdir = makePath(testProjectDirectory.pathAppended("UntitledProject68"));
        finder.setProjectDirectory(testProjectDirectory);
        finder.setProjectFiles({
            makeFile(testProjectDirectory.pathAppended("InParent.qml")),
            makeFile(subdir.pathAppended("InSubdir.qml"))
        });

        bool parentFound = finder.findFileOrDirectory(
            testProjectDirectory, nullptr, [](const QStringList &entries, int) {
                qDebug() << "entries: " << entries;
                //QDEBUG : tst_fileinprojectfinder::check_root_directory_content() entries:  QList(".", "..", "InSubdir.qml")
                QEXPECT_FAIL("", "Known issue: Parent directory contains subdir's files.", Continue);
                QVERIFY(entries.contains("InParent.qml"));
                //FAIL!  : tst_fileinprojectfinder::check_root_directory_content() 'entries.contains("InParent.qml")' returned FALSE. ()
            });

        QVERIFY(parentFound);
    }

};

QTEST_GUILESS_MAIN(tst_fileinprojectfinder)

#include "tst_fileinprojectfinder.moc"
