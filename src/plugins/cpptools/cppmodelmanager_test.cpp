/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "baseeditordocumentprocessor.h"
#include "builtineditordocumentparser.h"
#include "cppsourceprocessor.h"
#include "cpptoolsplugin.h"
#include "cpptoolstestcase.h"
#include "editordocumenthandle.h"
#include "modelmanagertesthelper.h"
#include "projectinfo.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/testdatadir.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <cplusplus/LookupContext.h>
#include <utils/executeondestruction.h>
#include <utils/hostosinfo.h>

#include <QDebug>
#include <QtTest>

#define VERIFY_DOCUMENT_REVISION(document, expectedRevision) \
    QVERIFY(document); \
    QCOMPARE(document->revision(), expectedRevision);

using namespace CppTools;
using namespace CppTools::Internal;
using namespace CppTools::Tests;
using namespace ProjectExplorer;

typedef CPlusPlus::Document Document;

Q_DECLARE_METATYPE(ProjectFile)

namespace {

inline QString _(const QByteArray &ba) { return QString::fromLatin1(ba, ba.size()); }

class MyTestDataDir : public Core::Tests::TestDataDir
{
public:
    MyTestDataDir(const QString &dir)
        : TestDataDir(_(SRCDIR "/../../../tests/cppmodelmanager/") + dir)
    {}

    QString includeDir(bool cleaned = true) const
    { return directory(_("include"), cleaned); }

    QString frameworksDir(bool cleaned = true) const
    { return directory(_("frameworks"), cleaned); }

    QString fileFromSourcesDir(const QString &fileName) const
    { return directory(_("sources")) + QLatin1Char('/') + fileName; }
};

QStringList toAbsolutePaths(const QStringList &relativePathList,
                            const Tests::TemporaryCopiedDir &temporaryDir)
{
    QStringList result;
    foreach (const QString &file, relativePathList)
        result << temporaryDir.absolutePath(file.toUtf8());
    return result;
}

// TODO: When possible, use this helper class in all tests
class ProjectCreator
{
public:
    ProjectCreator(ModelManagerTestHelper *modelManagerTestHelper)
        : modelManagerTestHelper(modelManagerTestHelper)
    {}

    /// 'files' is expected to be a list of file names that reside in 'dir'.
    void create(const QString &name, const QString &dir, const QStringList &files)
    {
        const MyTestDataDir projectDir(dir);
        foreach (const QString &file, files)
            projectFiles << projectDir.file(file);

        Project *project = modelManagerTestHelper->createProject(name);
        projectInfo = ProjectInfo(project);

        ProjectPart::Ptr part(new ProjectPart);
        part->qtVersion = ProjectPart::Qt5;
        foreach (const QString &file, projectFiles) {
            ProjectFile projectFile(file, ProjectFile::classify(file));
            part->files.append(projectFile);
        }
        projectInfo.appendProjectPart(part);
    }

    ModelManagerTestHelper *modelManagerTestHelper;
    ProjectInfo projectInfo;
    QStringList projectFiles;
};

/// Changes a file on the disk and restores its original contents on destruction
class FileChangerAndRestorer
{
public:
    FileChangerAndRestorer(const QString &filePath)
        : m_filePath(filePath)
    {
    }

    ~FileChangerAndRestorer()
    {
        restoreContents();
    }

    /// Saves the contents also internally so it can be restored on destruction
    bool readContents(QByteArray *contents)
    {
        Utils::FileReader fileReader;
        const bool isFetchOk = fileReader.fetch(m_filePath);
        if (isFetchOk) {
            m_originalFileContents = fileReader.data();
            if (contents)
                *contents = m_originalFileContents;
        }
        return isFetchOk;
    }

    bool writeContents(const QByteArray &contents) const
    {
        return TestCase::writeFile(m_filePath, contents);
    }

private:
    void restoreContents() const
    {
        TestCase::writeFile(m_filePath, m_originalFileContents);
    }

    QByteArray m_originalFileContents;
    const QString &m_filePath;
};

ProjectPart::Ptr projectPartOfEditorDocument(const QString &filePath)
{
    auto *editorDocument = CppModelManager::instance()->cppEditorDocument(filePath);
    QTC_ASSERT(editorDocument, return ProjectPart::Ptr());
    return editorDocument->processor()->parser()->projectPartInfo().projectPart;
}

} // anonymous namespace

/// Check: The preprocessor cleans include and framework paths.
void CppToolsPlugin::test_modelmanager_paths_are_clean()
{
    ModelManagerTestHelper helper;
    CppModelManager *mm = CppModelManager::instance();

    const MyTestDataDir testDataDir(_("testdata"));

    Project *project = helper.createProject(_("test_modelmanager_paths_are_clean"));
    ProjectInfo pi = ProjectInfo(project);

    typedef ProjectPartHeaderPath HeaderPath;

    ProjectPart::Ptr part(new ProjectPart);
    part->qtVersion = ProjectPart::Qt5;
    part->projectDefines = QByteArray("#define OH_BEHAVE -1\n");
    part->headerPaths = { HeaderPath(testDataDir.includeDir(false), HeaderPath::IncludePath),
                          HeaderPath(testDataDir.frameworksDir(false), HeaderPath::FrameworkPath) };
    pi.appendProjectPart(part);

    mm->updateProjectInfo(pi);

    ProjectPartHeaderPaths headerPaths = mm->headerPaths();
    QCOMPARE(headerPaths.size(), 2);
    QVERIFY(headerPaths.contains(HeaderPath(testDataDir.includeDir(), HeaderPath::IncludePath)));
    QVERIFY(headerPaths.contains(HeaderPath(testDataDir.frameworksDir(),
                                            HeaderPath::FrameworkPath)));
}

/// Check: Frameworks headers are resolved.
void CppToolsPlugin::test_modelmanager_framework_headers()
{
    if (Utils::HostOsInfo::isWindowsHost())
        QSKIP("Can't resolve framework soft links on Windows.");

    ModelManagerTestHelper helper;
    CppModelManager *mm = CppModelManager::instance();

    const MyTestDataDir testDataDir(_("testdata"));

    Project *project = helper.createProject(_("test_modelmanager_framework_headers"));
    ProjectInfo pi = ProjectInfo(project);

    typedef ProjectPartHeaderPath HeaderPath;

    ProjectPart::Ptr part(new ProjectPart);
    part->qtVersion = ProjectPart::Qt5;
    part->projectDefines = QByteArray("#define OH_BEHAVE -1\n");
    part->headerPaths = { HeaderPath(testDataDir.includeDir(false), HeaderPath::IncludePath),
                          HeaderPath(testDataDir.frameworksDir(false), HeaderPath::FrameworkPath) };
    const QString &source = testDataDir.fileFromSourcesDir(
        _("test_modelmanager_framework_headers.cpp"));
    part->files << ProjectFile(source, ProjectFile::CXXSource);
    pi.appendProjectPart(part);

    mm->updateProjectInfo(pi).waitForFinished();
    QCoreApplication::processEvents();

    QVERIFY(mm->snapshot().contains(source));
    Document::Ptr doc = mm->document(source);
    QVERIFY(!doc.isNull());
    CPlusPlus::Namespace *ns = doc->globalNamespace();
    QVERIFY(ns);
    QVERIFY(ns->memberCount() > 0);
    for (unsigned i = 0, ei = ns->memberCount(); i < ei; ++i) {
        CPlusPlus::Symbol *s = ns->memberAt(i);
        QVERIFY(s);
        QVERIFY(s->name());
        const CPlusPlus::Identifier *id = s->name()->asNameId();
        QVERIFY(id);
        QByteArray chars = id->chars();
        QVERIFY(chars.startsWith("success"));
    }
}

/// QTCREATORBUG-9056
/// Check: If the project configuration changes, all project files and their
///        includes have to be reparsed.
void CppToolsPlugin::test_modelmanager_refresh_also_includes_of_project_files()
{
    ModelManagerTestHelper helper;
    CppModelManager *mm = CppModelManager::instance();

    const MyTestDataDir testDataDir(_("testdata"));

    const QString testCpp(testDataDir.fileFromSourcesDir(_("test_modelmanager_refresh.cpp")));
    const QString testHeader(testDataDir.fileFromSourcesDir( _("test_modelmanager_refresh.h")));

    Project *project = helper.createProject(
                _("test_modelmanager_refresh_also_includes_of_project_files"));
    ProjectInfo pi = ProjectInfo(project);

    typedef ProjectPartHeaderPath HeaderPath;

    ProjectPart::Ptr part(new ProjectPart);
    part->qtVersion = ProjectPart::Qt5;
    part->projectDefines = QByteArray("#define OH_BEHAVE -1\n");
    part->headerPaths = { HeaderPath(testDataDir.includeDir(false), HeaderPath::IncludePath) };
    part->files.append(ProjectFile(testCpp, ProjectFile::CXXSource));
    pi.appendProjectPart(part);

    QSet<QString> refreshedFiles = helper.updateProjectInfo(pi);
    QCOMPARE(refreshedFiles.size(), 1);
    QVERIFY(refreshedFiles.contains(testCpp));
    CPlusPlus::Snapshot snapshot = mm->snapshot();
    QVERIFY(snapshot.contains(testHeader));
    QVERIFY(snapshot.contains(testCpp));

    Document::Ptr headerDocumentBefore = snapshot.document(testHeader);
    const QList<CPlusPlus::Macro> macrosInHeaderBefore = headerDocumentBefore->definedMacros();
    QCOMPARE(macrosInHeaderBefore.size(), 1);
    QVERIFY(macrosInHeaderBefore.first().name() == "test_modelmanager_refresh_h");

    // Introduce a define that will enable another define once the document is reparsed.
    part->projectDefines = QByteArray("#define TEST_DEFINE 1\n");
    pi = ProjectInfo(project);
    pi.appendProjectPart(part);

    refreshedFiles = helper.updateProjectInfo(pi);

    QCOMPARE(refreshedFiles.size(), 1);
    QVERIFY(refreshedFiles.contains(testCpp));
    snapshot = mm->snapshot();
    QVERIFY(snapshot.contains(testHeader));
    QVERIFY(snapshot.contains(testCpp));

    Document::Ptr headerDocumentAfter = snapshot.document(testHeader);
    const QList<CPlusPlus::Macro> macrosInHeaderAfter = headerDocumentAfter->definedMacros();
    QCOMPARE(macrosInHeaderAfter.size(), 2);
    QVERIFY(macrosInHeaderAfter.at(0).name() == "test_modelmanager_refresh_h");
    QVERIFY(macrosInHeaderAfter.at(1).name() == "TEST_DEFINE_DEFINED");
}

/// QTCREATORBUG-9205
/// Check: When reparsing the same files again, no errors occur
///        (The CppSourceProcessor's already seen files are properly cleared!).
void CppToolsPlugin::test_modelmanager_refresh_several_times()
{
    ModelManagerTestHelper helper;
    CppModelManager *mm = CppModelManager::instance();

    const MyTestDataDir testDataDir(_("testdata_refresh"));

    const QString testHeader1(testDataDir.file(_("defines.h")));
    const QString testHeader2(testDataDir.file(_("header.h")));
    const QString testCpp(testDataDir.file(_("source.cpp")));

    Project *project = helper.createProject(_("test_modelmanager_refresh_several_times"));
    ProjectInfo pi = ProjectInfo(project);

    ProjectPart::Ptr part(new ProjectPart);
    part->qtVersion = ProjectPart::Qt5;
    part->files.append(ProjectFile(testHeader1, ProjectFile::CXXHeader));
    part->files.append(ProjectFile(testHeader2, ProjectFile::CXXHeader));
    part->files.append(ProjectFile(testCpp, ProjectFile::CXXSource));
    pi.appendProjectPart(part);
    mm->updateProjectInfo(pi);

    CPlusPlus::Snapshot snapshot;
    QSet<QString> refreshedFiles;
    CPlusPlus::Document::Ptr document;

    QByteArray defines = "#define FIRST_DEFINE";
    for (int i = 0; i < 2; ++i) {
        pi = ProjectInfo(project);
        ProjectPart::Ptr part(new ProjectPart);
        // Simulate project configuration change by having different defines each time.
        defines += "\n#define ANOTHER_DEFINE";
        part->projectDefines = defines;
        part->qtVersion = ProjectPart::Qt5;
        part->files.append(ProjectFile(testHeader1, ProjectFile::CXXHeader));
        part->files.append(ProjectFile(testHeader2, ProjectFile::CXXHeader));
        part->files.append(ProjectFile(testCpp, ProjectFile::CXXSource));
        pi.appendProjectPart(part);

        refreshedFiles = helper.updateProjectInfo(pi);
        QCOMPARE(refreshedFiles.size(), 3);

        QVERIFY(refreshedFiles.contains(testHeader1));
        QVERIFY(refreshedFiles.contains(testHeader2));
        QVERIFY(refreshedFiles.contains(testCpp));

        snapshot = mm->snapshot();
        QVERIFY(snapshot.contains(testHeader1));
        QVERIFY(snapshot.contains(testHeader2));
        QVERIFY(snapshot.contains(testCpp));

        // No diagnostic messages expected
        document = snapshot.document(testHeader1);
        QVERIFY(document->diagnosticMessages().isEmpty());

        document = snapshot.document(testHeader2);
        QVERIFY(document->diagnosticMessages().isEmpty());

        document = snapshot.document(testCpp);
        QVERIFY(document->diagnosticMessages().isEmpty());
    }
}

/// QTCREATORBUG-9581
/// Check: If nothing has changes, nothing should be reindexed.
void CppToolsPlugin::test_modelmanager_refresh_test_for_changes()
{
    ModelManagerTestHelper helper;
    CppModelManager *mm = CppModelManager::instance();

    const MyTestDataDir testDataDir(_("testdata_refresh"));
    const QString testCpp(testDataDir.file(_("source.cpp")));

    Project *project = helper.createProject(_("test_modelmanager_refresh_2"));
    ProjectInfo pi = ProjectInfo(project);

    ProjectPart::Ptr part(new ProjectPart);
    part->qtVersion = ProjectPart::Qt5;
    part->files.append(ProjectFile(testCpp, ProjectFile::CXXSource));
    pi.appendProjectPart(part);

    // Reindexing triggers a reparsing thread
    helper.resetRefreshedSourceFiles();
    QFuture<void> firstFuture = mm->updateProjectInfo(pi);
    QVERIFY(firstFuture.isStarted() || firstFuture.isRunning());
    firstFuture.waitForFinished();
    const QSet<QString> refreshedFiles = helper.waitForRefreshedSourceFiles();
    QCOMPARE(refreshedFiles.size(), 1);
    QVERIFY(refreshedFiles.contains(testCpp));

    // No reindexing since nothing has changed
    QFuture<void> subsequentFuture = mm->updateProjectInfo(pi);
    QVERIFY(subsequentFuture.isCanceled() && subsequentFuture.isFinished());
}

/// Check: (1) Added project files are recognized and parsed.
/// Check: (2) Removed project files are recognized and purged from the snapshot.
void CppToolsPlugin::test_modelmanager_refresh_added_and_purge_removed()
{
    ModelManagerTestHelper helper;
    CppModelManager *mm = CppModelManager::instance();

    const MyTestDataDir testDataDir(_("testdata_refresh"));

    const QString testHeader1(testDataDir.file(_("header.h")));
    const QString testHeader2(testDataDir.file(_("defines.h")));
    const QString testCpp(testDataDir.file(_("source.cpp")));

    Project *project = helper.createProject(_("test_modelmanager_refresh_3"));
    ProjectInfo pi = ProjectInfo(project);

    ProjectPart::Ptr part(new ProjectPart);
    part->qtVersion = ProjectPart::Qt5;
    part->files.append(ProjectFile(testCpp, ProjectFile::CXXSource));
    part->files.append(ProjectFile(testHeader1, ProjectFile::CXXHeader));
    pi.appendProjectPart(part);

    CPlusPlus::Snapshot snapshot;
    QSet<QString> refreshedFiles;

    refreshedFiles = helper.updateProjectInfo(pi);

    QCOMPARE(refreshedFiles.size(), 2);
    QVERIFY(refreshedFiles.contains(testHeader1));
    QVERIFY(refreshedFiles.contains(testCpp));

    snapshot = mm->snapshot();
    QVERIFY(snapshot.contains(testHeader1));
    QVERIFY(snapshot.contains(testCpp));

    // Now add testHeader2 and remove testHeader1
    pi = ProjectInfo(project);
    ProjectPart::Ptr newPart(new ProjectPart);
    newPart->qtVersion = ProjectPart::Qt5;
    newPart->files.append(ProjectFile(testCpp, ProjectFile::CXXSource));
    newPart->files.append(ProjectFile(testHeader2, ProjectFile::CXXHeader));
    pi.appendProjectPart(newPart);

    refreshedFiles = helper.updateProjectInfo(pi);

    // Only the added project file was reparsed
    QCOMPARE(refreshedFiles.size(), 1);
    QVERIFY(refreshedFiles.contains(testHeader2));

    snapshot = mm->snapshot();
    QVERIFY(snapshot.contains(testHeader2));
    QVERIFY(snapshot.contains(testCpp));
    // The removed project file is not anymore in the snapshot
    QVERIFY(!snapshot.contains(testHeader1));
}

/// Check: Timestamp modified files are reparsed if project files are added or removed
///        while the project configuration stays the same
void CppToolsPlugin::test_modelmanager_refresh_timeStampModified_if_sourcefiles_change()
{
    QFETCH(QString, fileToChange);
    QFETCH(QStringList, initialProjectFiles);
    QFETCH(QStringList, finalProjectFiles);

    Tests::TemporaryCopiedDir temporaryDir(
                MyTestDataDir(QLatin1String("testdata_refresh2")).path());
    fileToChange = temporaryDir.absolutePath(fileToChange.toUtf8());
    initialProjectFiles = toAbsolutePaths(initialProjectFiles, temporaryDir);
    finalProjectFiles = toAbsolutePaths(finalProjectFiles, temporaryDir);

    ModelManagerTestHelper helper;
    CppModelManager *mm = CppModelManager::instance();

    Project *project = helper.createProject(_("test_modelmanager_refresh_timeStampModified"));
    ProjectInfo pi = ProjectInfo(project);

    ProjectPart::Ptr part(new ProjectPart);
    part->qtVersion = ProjectPart::Qt5;
    foreach (const QString &file, initialProjectFiles)
        part->files.append(ProjectFile(file, ProjectFile::CXXSource));
    pi = ProjectInfo(project);
    pi.appendProjectPart(part);

    Document::Ptr document;
    CPlusPlus::Snapshot snapshot;
    QSet<QString> refreshedFiles;

    refreshedFiles = helper.updateProjectInfo(pi);

    QCOMPARE(refreshedFiles.size(), initialProjectFiles.size());
    snapshot = mm->snapshot();
    foreach (const QString &file, initialProjectFiles) {
        QVERIFY(refreshedFiles.contains(file));
        QVERIFY(snapshot.contains(file));
    }

    document = snapshot.document(fileToChange);
    const QDateTime lastModifiedBefore = document->lastModified();
    QCOMPARE(document->globalSymbolCount(), 1U);
    QCOMPARE(document->globalSymbolAt(0)->name()->identifier()->chars(), "someGlobal");

    // Modify the file
    QTest::qSleep(1000); // Make sure the timestamp is different
    FileChangerAndRestorer fileChangerAndRestorer(fileToChange);
    QByteArray originalContents;
    QVERIFY(fileChangerAndRestorer.readContents(&originalContents));
    const QByteArray newFileContentes = originalContents + "\nint addedOtherGlobal;";
    QVERIFY(fileChangerAndRestorer.writeContents(newFileContentes));

    // Add or remove source file. The configuration stays the same.
    part->files.clear();
    foreach (const QString &file, finalProjectFiles)
        part->files.append(ProjectFile(file, ProjectFile::CXXSource));
    pi = ProjectInfo(project);
    pi.appendProjectPart(part);

    refreshedFiles = helper.updateProjectInfo(pi);

    QCOMPARE(refreshedFiles.size(), finalProjectFiles.size());
    snapshot = mm->snapshot();
    foreach (const QString &file, finalProjectFiles) {
        QVERIFY(refreshedFiles.contains(file));
        QVERIFY(snapshot.contains(file));
    }
    document = snapshot.document(fileToChange);
    const QDateTime lastModifiedAfter = document->lastModified();
    QVERIFY(lastModifiedAfter > lastModifiedBefore);
    QCOMPARE(document->globalSymbolCount(), 2U);
    QCOMPARE(document->globalSymbolAt(0)->name()->identifier()->chars(), "someGlobal");
    QCOMPARE(document->globalSymbolAt(1)->name()->identifier()->chars(), "addedOtherGlobal");
}

void CppToolsPlugin::test_modelmanager_refresh_timeStampModified_if_sourcefiles_change_data()
{
    QTest::addColumn<QString>("fileToChange");
    QTest::addColumn<QStringList>("initialProjectFiles");
    QTest::addColumn<QStringList>("finalProjectFiles");

    const QString testCpp = QLatin1String("source.cpp");
    const QString testCpp2 = QLatin1String("source2.cpp");

    const QString fileToChange = testCpp;
    const QStringList projectFiles1 = { testCpp };
    const QStringList projectFiles2 = { testCpp, testCpp2 };

    // Add a file
    QTest::newRow("case: add project file") << fileToChange << projectFiles1 << projectFiles2;

    // Remove a file
    QTest::newRow("case: remove project file") << fileToChange << projectFiles2 << projectFiles1;
}

/// Check: If a second project is opened, the code model is still aware of
///        files of the first project.
void CppToolsPlugin::test_modelmanager_snapshot_after_two_projects()
{
    QSet<QString> refreshedFiles;
    ModelManagerTestHelper helper;
    ProjectCreator project1(&helper);
    ProjectCreator project2(&helper);
    CppModelManager *mm = CppModelManager::instance();

    // Project 1
    project1.create(_("test_modelmanager_snapshot_after_two_projects.1"),
                    _("testdata_project1"),
                    { "foo.h", "foo.cpp",  "main.cpp" });

    refreshedFiles = helper.updateProjectInfo(project1.projectInfo);
    QCOMPARE(refreshedFiles, project1.projectFiles.toSet());
    const int snapshotSizeAfterProject1 = mm->snapshot().size();

    foreach (const QString &file, project1.projectFiles)
        QVERIFY(mm->snapshot().contains(file));

    // Project 2
    project2.create(_("test_modelmanager_snapshot_after_two_projects.2"),
                    _("testdata_project2"),
                    { "bar.h", "bar.cpp",  "main.cpp" });

    refreshedFiles = helper.updateProjectInfo(project2.projectInfo);
    QCOMPARE(refreshedFiles, project2.projectFiles.toSet());

    const int snapshotSizeAfterProject2 = mm->snapshot().size();
    QVERIFY(snapshotSizeAfterProject2 > snapshotSizeAfterProject1);
    QVERIFY(snapshotSizeAfterProject2 >= snapshotSizeAfterProject1 + project2.projectFiles.size());

    foreach (const QString &file, project1.projectFiles)
        QVERIFY(mm->snapshot().contains(file));
    foreach (const QString &file, project2.projectFiles)
        QVERIFY(mm->snapshot().contains(file));
}

/// Check: (1) For a project with a *.ui file an AbstractEditorSupport object
///            is added for the ui_* file.
/// Check: (2) The CppSourceProcessor can successfully resolve the ui_* file
///            though it might not be actually generated in the build dir.
///

void CppToolsPlugin::test_modelmanager_extraeditorsupport_uiFiles()
{
    VerifyCleanCppModelManager verify;

    TemporaryCopiedDir temporaryDir(MyTestDataDir(QLatin1String("testdata_guiproject1")).path());
    QVERIFY(temporaryDir.isValid());
    const QString projectFile = temporaryDir.absolutePath("testdata_guiproject1.pro");

    ProjectOpenerAndCloser projects;
    ProjectInfo projectInfo = projects.open(projectFile, /*configureAsExampleProject=*/ true);
    QVERIFY(projectInfo.isValid());

    // Check working copy.
    // An AbstractEditorSupport object should have been added for the ui_* file.
    CppModelManager *mm = CppModelManager::instance();
    WorkingCopy workingCopy = mm->workingCopy();

    QCOMPARE(workingCopy.size(), 1);

    QStringList fileNamesInWorkinCopy;
    QHashIterator<Utils::FileName, QPair<QByteArray, unsigned> > it = workingCopy.iterator();
    while (it.hasNext()) {
        it.next();
        fileNamesInWorkinCopy << Utils::FileName::fromString(it.key().toString()).fileName();
    }
    fileNamesInWorkinCopy.sort();
    const QString expectedUiHeaderFileName = _("ui_mainwindow.h");
    QCOMPARE(fileNamesInWorkinCopy.at(0), expectedUiHeaderFileName);

    // Check CppSourceProcessor / includes.
    // The CppSourceProcessor is expected to find the ui_* file in the working copy.
    const QString fileIncludingTheUiFile = temporaryDir.absolutePath("mainwindow.cpp");
    while (!mm->snapshot().document(fileIncludingTheUiFile))
        QCoreApplication::processEvents();

    const CPlusPlus::Snapshot snapshot = mm->snapshot();
    const Document::Ptr document = snapshot.document(fileIncludingTheUiFile);
    QVERIFY(document);
    const QStringList includedFiles = document->includedFiles();
    QCOMPARE(includedFiles.size(), 2);
    QCOMPARE(Utils::FileName::fromString(includedFiles.at(0)).fileName(), _("mainwindow.h"));
    QCOMPARE(Utils::FileName::fromString(includedFiles.at(1)).fileName(), _("ui_mainwindow.h"));
}

/// QTCREATORBUG-9828: Locator shows symbols of closed files
/// Check: The garbage collector should be run if the last CppEditor is closed.
void CppToolsPlugin::test_modelmanager_gc_if_last_cppeditor_closed()
{
    ModelManagerTestHelper helper;

    MyTestDataDir testDataDirectory(_("testdata_guiproject1"));
    const QString file = testDataDirectory.file(_("main.cpp"));

    CppModelManager *mm = CppModelManager::instance();
    helper.resetRefreshedSourceFiles();

    // Open a file in the editor
    QCOMPARE(Core::DocumentModel::openedDocuments().size(), 0);
    Core::IEditor *editor = Core::EditorManager::openEditor(file);
    QVERIFY(editor);
    QCOMPARE(Core::DocumentModel::openedDocuments().size(), 1);
    QVERIFY(mm->isCppEditor(editor));
    QVERIFY(mm->workingCopy().contains(file));

    // Wait until the file is refreshed
    helper.waitForRefreshedSourceFiles();

    // Close file/editor
    Core::EditorManager::closeDocument(editor->document(), /*askAboutModifiedEditors=*/ false);
    helper.waitForFinishedGc();

    // Check: File is removed from the snapshpt
    QVERIFY(!mm->workingCopy().contains(file));
    QVERIFY(!mm->snapshot().contains(file));
}

/// Check: Files that are open in the editor are not garbage collected.
void CppToolsPlugin::test_modelmanager_dont_gc_opened_files()
{
    ModelManagerTestHelper helper;

    MyTestDataDir testDataDirectory(_("testdata_guiproject1"));
    const QString file = testDataDirectory.file(_("main.cpp"));

    CppModelManager *mm = CppModelManager::instance();
    helper.resetRefreshedSourceFiles();

    // Open a file in the editor
    QCOMPARE(Core::DocumentModel::openedDocuments().size(), 0);
    Core::IEditor *editor = Core::EditorManager::openEditor(file);
    QVERIFY(editor);
    QCOMPARE(Core::DocumentModel::openedDocuments().size(), 1);
    QVERIFY(mm->isCppEditor(editor));

    // Wait until the file is refreshed and check whether it is in the working copy
    helper.waitForRefreshedSourceFiles();

    QVERIFY(mm->workingCopy().contains(file));

    // Run the garbage collector
    mm->GC();

    // Check: File is still there
    QVERIFY(mm->workingCopy().contains(file));
    QVERIFY(mm->snapshot().contains(file));

    // Close editor
    Core::EditorManager::closeDocument(editor->document());
    helper.waitForFinishedGc();
    QVERIFY(mm->snapshot().isEmpty());
}

namespace {
struct EditorCloser {
    Core::IEditor *editor;
    EditorCloser(Core::IEditor *editor): editor(editor) {}
    ~EditorCloser()
    {
        using namespace CppTools;
        if (editor)
            QVERIFY(Tests::TestCase::closeEditorWithoutGarbageCollectorInvocation(editor));
    }
};

QString nameOfFirstDeclaration(const Document::Ptr &doc)
{
    if (doc && doc->globalNamespace()) {
        if (CPlusPlus::Symbol *s = doc->globalSymbolAt(0)) {
            if (CPlusPlus::Declaration *decl = s->asDeclaration()) {
                if (const CPlusPlus::Name *name = decl->name()) {
                    if (const CPlusPlus::Identifier *identifier = name->identifier())
                        return QString::fromUtf8(identifier->chars(), identifier->size());
                }
            }
        }
    }
    return QString();
}
}

void CppToolsPlugin::test_modelmanager_defines_per_project()
{
    ModelManagerTestHelper helper;

    MyTestDataDir testDataDirectory(_("testdata_defines"));
    const QString main1File = testDataDirectory.file(_("main1.cpp"));
    const QString main2File = testDataDirectory.file(_("main2.cpp"));
    const QString header = testDataDirectory.file(_("header.h"));

    CppModelManager *mm = CppModelManager::instance();

    Project *project = helper.createProject(_("test_modelmanager_defines_per_project"));

    typedef ProjectPartHeaderPath HeaderPath;

    ProjectPart::Ptr part1(new ProjectPart);
    part1->projectFile = QLatin1String("project1.projectfile");
    part1->files.append(ProjectFile(main1File, ProjectFile::CXXSource));
    part1->files.append(ProjectFile(header, ProjectFile::CXXHeader));
    part1->qtVersion = ProjectPart::NoQt;
    part1->projectDefines = QByteArray("#define SUB1\n");
    part1->headerPaths = { HeaderPath(testDataDirectory.includeDir(false), HeaderPath::IncludePath) };

    ProjectPart::Ptr part2(new ProjectPart);
    part2->projectFile = QLatin1String("project1.projectfile");
    part2->files.append(ProjectFile(main2File, ProjectFile::CXXSource));
    part2->files.append(ProjectFile(header, ProjectFile::CXXHeader));
    part2->qtVersion = ProjectPart::NoQt;
    part2->projectDefines = QByteArray("#define SUB2\n");
    part2->headerPaths = { HeaderPath(testDataDirectory.includeDir(false), HeaderPath::IncludePath) };

    ProjectInfo pi = ProjectInfo(project);
    pi.appendProjectPart(part1);
    pi.appendProjectPart(part2);

    helper.updateProjectInfo(pi);
    QCOMPARE(mm->snapshot().size(), 3);

    // Open a file in the editor
    QCOMPARE(Core::DocumentModel::openedDocuments().size(), 0);

    struct Data {
        QString firstDeclarationName;
        QString fileName;
    } d[] = {
        { _("one"), main1File },
        { _("two"), main2File }
    };
    const int size = sizeof(d) / sizeof(d[0]);
    for (int i = 0; i < size; ++i) {
        const QString firstDeclarationName = d[i].firstDeclarationName;
        const QString fileName = d[i].fileName;

        Core::IEditor *editor = Core::EditorManager::openEditor(fileName);
        EditorCloser closer(editor);
        QVERIFY(editor);
        QCOMPARE(Core::DocumentModel::openedDocuments().size(), 1);
        QVERIFY(mm->isCppEditor(editor));

        Document::Ptr doc = mm->document(fileName);
        QCOMPARE(nameOfFirstDeclaration(doc), firstDeclarationName);
    }
}

void CppToolsPlugin::test_modelmanager_precompiled_headers()
{
    ModelManagerTestHelper helper;

    MyTestDataDir testDataDirectory(_("testdata_defines"));
    const QString main1File = testDataDirectory.file(_("main1.cpp"));
    const QString main2File = testDataDirectory.file(_("main2.cpp"));
    const QString header = testDataDirectory.file(_("header.h"));
    const QString pch1File = testDataDirectory.file(_("pch1.h"));
    const QString pch2File = testDataDirectory.file(_("pch2.h"));

    CppModelManager *mm = CppModelManager::instance();

    Project *project = helper.createProject(_("test_modelmanager_defines_per_project_pch"));

    typedef ProjectPartHeaderPath HeaderPath;

    ProjectPart::Ptr part1(new ProjectPart);
    part1->projectFile = QLatin1String("project1.projectfile");
    part1->files.append(ProjectFile(main1File, ProjectFile::CXXSource));
    part1->files.append(ProjectFile(header, ProjectFile::CXXHeader));
    part1->qtVersion = ProjectPart::NoQt;
    part1->precompiledHeaders.append(pch1File);
    part1->headerPaths = { HeaderPath(testDataDirectory.includeDir(false), HeaderPath::IncludePath) };
    part1->updateLanguageFeatures();

    ProjectPart::Ptr part2(new ProjectPart);
    part2->projectFile = QLatin1String("project2.projectfile");
    part2->files.append(ProjectFile(main2File, ProjectFile::CXXSource));
    part2->files.append(ProjectFile(header, ProjectFile::CXXHeader));
    part2->qtVersion = ProjectPart::NoQt;
    part2->precompiledHeaders.append(pch2File);
    part2->headerPaths = { HeaderPath(testDataDirectory.includeDir(false), HeaderPath::IncludePath) };
    part2->updateLanguageFeatures();

    ProjectInfo pi = ProjectInfo(project);
    pi.appendProjectPart(part1);
    pi.appendProjectPart(part2);

    helper.updateProjectInfo(pi);
    QCOMPARE(mm->snapshot().size(), 3);

    // Open a file in the editor
    QCOMPARE(Core::DocumentModel::openedDocuments().size(), 0);

    struct Data {
        QString firstDeclarationName;
        QString firstClassInPchFile;
        QString fileName;
    } d[] = {
        { _("one"), _("ClassInPch1"), main1File },
        { _("two"), _("ClassInPch2"), main2File }
    };
    const int size = sizeof(d) / sizeof(d[0]);
    for (int i = 0; i < size; ++i) {
        const QString firstDeclarationName = d[i].firstDeclarationName;
        const QByteArray firstClassInPchFile = d[i].firstClassInPchFile.toUtf8();
        const QString fileName = d[i].fileName;

        Core::IEditor *editor = Core::EditorManager::openEditor(fileName);
        EditorCloser closer(editor);
        QVERIFY(editor);
        QCOMPARE(Core::DocumentModel::openedDocuments().size(), 1);
        QVERIFY(mm->isCppEditor(editor));

        auto parser = BuiltinEditorDocumentParser::get(fileName);
        QVERIFY(parser);
        BaseEditorDocumentParser::Configuration config = parser->configuration();
        config.usePrecompiledHeaders = true;
        parser->setConfiguration(config);
        parser->update({CppModelManager::instance()->workingCopy(), nullptr, Language::Cxx, false});

        // Check if defines from pch are considered
        Document::Ptr document = mm->document(fileName);
        QCOMPARE(nameOfFirstDeclaration(document), firstDeclarationName);

        // Check if declarations from pch are considered
        CPlusPlus::LookupContext context(document, parser->snapshot());
        const CPlusPlus::Identifier *identifier
            = document->control()->identifier(firstClassInPchFile.data());
        const QList<CPlusPlus::LookupItem> results = context.lookup(identifier,
                                                                    document->globalNamespace());
        QVERIFY(!results.isEmpty());
        QVERIFY(results.first().declaration()->type()->asClassType());
    }
}

void CppToolsPlugin::test_modelmanager_defines_per_editor()
{
    ModelManagerTestHelper helper;

    MyTestDataDir testDataDirectory(_("testdata_defines"));
    const QString main1File = testDataDirectory.file(_("main1.cpp"));
    const QString main2File = testDataDirectory.file(_("main2.cpp"));
    const QString header = testDataDirectory.file(_("header.h"));

    CppModelManager *mm = CppModelManager::instance();

    Project *project = helper.createProject(_("test_modelmanager_defines_per_editor"));

    typedef ProjectPartHeaderPath HeaderPath;

    ProjectPart::Ptr part1(new ProjectPart);
    part1->files.append(ProjectFile(main1File, ProjectFile::CXXSource));
    part1->files.append(ProjectFile(header, ProjectFile::CXXHeader));
    part1->qtVersion = ProjectPart::NoQt;
    part1->headerPaths = { HeaderPath(testDataDirectory.includeDir(false), HeaderPath::IncludePath) };

    ProjectPart::Ptr part2(new ProjectPart);
    part2->files.append(ProjectFile(main2File, ProjectFile::CXXSource));
    part2->files.append(ProjectFile(header, ProjectFile::CXXHeader));
    part2->qtVersion = ProjectPart::NoQt;
    part2->headerPaths = { HeaderPath(testDataDirectory.includeDir(false), HeaderPath::IncludePath) };

    ProjectInfo pi = ProjectInfo(project);
    pi.appendProjectPart(part1);
    pi.appendProjectPart(part2);

    helper.updateProjectInfo(pi);

    QCOMPARE(mm->snapshot().size(), 3);

    // Open a file in the editor
    QCOMPARE(Core::DocumentModel::openedDocuments().size(), 0);

    struct Data {
        QString editorDefines;
        QString firstDeclarationName;
    } d[] = {
        { _("#define SUB1\n"), _("one") },
        { _("#define SUB2\n"), _("two") }
    };
    const int size = sizeof(d) / sizeof(d[0]);
    for (int i = 0; i < size; ++i) {
        const QString editorDefines = d[i].editorDefines;
        const QString firstDeclarationName = d[i].firstDeclarationName;

        Core::IEditor *editor = Core::EditorManager::openEditor(main1File);
        EditorCloser closer(editor);
        QVERIFY(editor);
        QCOMPARE(Core::DocumentModel::openedDocuments().size(), 1);
        QVERIFY(mm->isCppEditor(editor));

        const QString filePath = editor->document()->filePath().toString();
        const auto parser = BaseEditorDocumentParser::get(filePath);
        BaseEditorDocumentParser::Configuration config = parser->configuration();
        config.editorDefines = editorDefines.toUtf8();
        parser->setConfiguration(config);
        parser->update({CppModelManager::instance()->workingCopy(), nullptr, Language::Cxx, false});

        Document::Ptr doc = mm->document(main1File);
        QCOMPARE(nameOfFirstDeclaration(doc), firstDeclarationName);
    }
}

void CppToolsPlugin::test_modelmanager_updateEditorsAfterProjectUpdate()
{
    ModelManagerTestHelper helper;

    MyTestDataDir testDataDirectory(_("testdata_defines"));
    const QString fileA = testDataDirectory.file(_("main1.cpp")); // content not relevant
    const QString fileB = testDataDirectory.file(_("main2.cpp")); // content not relevant

    // Open file A in editor
    Core::IEditor *editorA = Core::EditorManager::openEditor(fileA);
    QVERIFY(editorA);
    EditorCloser closerA(editorA);
    QCOMPARE(Core::DocumentModel::openedDocuments().size(), 1);
    QVERIFY(TestCase::waitForProcessedEditorDocument(fileA));
    ProjectPart::Ptr documentAProjectPart = projectPartOfEditorDocument(fileA);
    QVERIFY(!documentAProjectPart->project);

    // Open file B in editor
    Core::IEditor *editorB = Core::EditorManager::openEditor(fileB);
    QVERIFY(editorB);
    EditorCloser closerB(editorB);
    QCOMPARE(Core::DocumentModel::openedDocuments().size(), 2);
    QVERIFY(TestCase::waitForProcessedEditorDocument(fileB));
    ProjectPart::Ptr documentBProjectPart = projectPartOfEditorDocument(fileB);
    QVERIFY(!documentBProjectPart->project);

    // Switch back to document A
    Core::EditorManager::activateEditor(editorA);

    // Open/update related project
    Project *project = helper.createProject(_("test_modelmanager_updateEditorsAfterProjectUpdate"));

    ProjectPart::Ptr part(new ProjectPart);
    part->project = project;
    part->files.append(ProjectFile(fileA, ProjectFile::CXXSource));
    part->files.append(ProjectFile(fileB, ProjectFile::CXXSource));
    part->qtVersion = ProjectPart::NoQt;

    ProjectInfo pi = ProjectInfo(project);
    pi.appendProjectPart(part);
    helper.updateProjectInfo(pi);

    // ... and check for updated editor document A
    QVERIFY(TestCase::waitForProcessedEditorDocument(fileA));
    documentAProjectPart = projectPartOfEditorDocument(fileA);
    QCOMPARE(documentAProjectPart->project, project);

    // Switch back to document B and check if that's updated, too
    Core::EditorManager::activateEditor(editorB);
    QVERIFY(TestCase::waitForProcessedEditorDocument(fileB));
    documentBProjectPart = projectPartOfEditorDocument(fileB);
    QCOMPARE(documentBProjectPart->project, project);
}

void CppToolsPlugin::test_modelmanager_renameIncludes()
{
    struct ModelManagerGCHelper {
        ~ModelManagerGCHelper() { CppModelManager::instance()->GC(); }
    } GCHelper;
    Q_UNUSED(GCHelper); // do not warn about being unused

    TemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QDir workingDir(tmpDir.path());
    const QStringList fileNames = { "foo.h", "foo.cpp", "main.cpp" };
    const QString oldHeader(workingDir.filePath(_("foo.h")));
    const QString newHeader(workingDir.filePath(_("bar.h")));
    CppModelManager *modelManager = CppModelManager::instance();
    const MyTestDataDir testDir(_("testdata_project1"));

    // Copy test files to a temporary directory
    QSet<QString> sourceFiles;
    foreach (const QString &fileName, fileNames) {
        const QString &file = workingDir.filePath(fileName);
        QVERIFY(QFile::copy(testDir.file(fileName), file));
        // Saving source file names for the model manager update,
        // so we can update just the relevant files.
        if (ProjectFile::classify(file) == ProjectFile::CXXSource)
            sourceFiles.insert(file);
    }

    // Update the c++ model manager and check for the old includes
    modelManager->updateSourceFiles(sourceFiles).waitForFinished();
    QCoreApplication::processEvents();
    CPlusPlus::Snapshot snapshot = modelManager->snapshot();
    foreach (const QString &sourceFile, sourceFiles)
        QCOMPARE(snapshot.allIncludesForDocument(sourceFile), QSet<QString>() << oldHeader);

    // Renaming the header
    QVERIFY(Core::FileUtils::renameFile(oldHeader, newHeader));

    // Update the c++ model manager again and check for the new includes
    modelManager->updateSourceFiles(sourceFiles).waitForFinished();
    QCoreApplication::processEvents();
    snapshot = modelManager->snapshot();
    foreach (const QString &sourceFile, sourceFiles)
        QCOMPARE(snapshot.allIncludesForDocument(sourceFile), QSet<QString>() << newHeader);
}

void CppToolsPlugin::test_modelmanager_renameIncludesInEditor()
{
    struct ModelManagerGCHelper {
        ~ModelManagerGCHelper() { CppModelManager::instance()->GC(); }
    } GCHelper;
    Q_UNUSED(GCHelper); // do not warn about being unused

    TemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QDir workingDir(tmpDir.path());
    const QStringList fileNames = { "foo.h", "foo.cpp", "main.cpp" };
    const QString oldHeader(workingDir.filePath(_("foo.h")));
    const QString newHeader(workingDir.filePath(_("bar.h")));
    const QString mainFile(workingDir.filePath(_("main.cpp")));
    CppModelManager *modelManager = CppModelManager::instance();
    const MyTestDataDir testDir(_("testdata_project1"));

    ModelManagerTestHelper helper;
    helper.resetRefreshedSourceFiles();

    // Copy test files to a temporary directory
    QSet<QString> sourceFiles;
    foreach (const QString &fileName, fileNames) {
        const QString &file = workingDir.filePath(fileName);
        QVERIFY(QFile::copy(testDir.file(fileName), file));
        // Saving source file names for the model manager update,
        // so we can update just the relevant files.
        if (ProjectFile::classify(file) == ProjectFile::CXXSource)
            sourceFiles.insert(file);
    }

    // Update the c++ model manager and check for the old includes
    modelManager->updateSourceFiles(sourceFiles).waitForFinished();
    QCoreApplication::processEvents();
    CPlusPlus::Snapshot snapshot = modelManager->snapshot();
    foreach (const QString &sourceFile, sourceFiles)
        QCOMPARE(snapshot.allIncludesForDocument(sourceFile), QSet<QString>() << oldHeader);

    // Open a file in the editor
    QCOMPARE(Core::DocumentModel::openedDocuments().size(), 0);
    Core::IEditor *editor = Core::EditorManager::openEditor(mainFile);
    QVERIFY(editor);
    EditorCloser editorCloser(editor);
    Utils::ExecuteOnDestruction saveAllFiles([](){
        Core::DocumentManager::saveAllModifiedDocumentsSilently();
    });
    QCOMPARE(Core::DocumentModel::openedDocuments().size(), 1);
    QVERIFY(modelManager->isCppEditor(editor));
    QVERIFY(modelManager->workingCopy().contains(mainFile));

    // Renaming the header
    QVERIFY(Core::FileUtils::renameFile(oldHeader, newHeader));

    // Update the c++ model manager again and check for the new includes
    TestCase::waitForProcessedEditorDocument(mainFile);
    modelManager->updateSourceFiles(sourceFiles).waitForFinished();
    QCoreApplication::processEvents();
    snapshot = modelManager->snapshot();
    foreach (const QString &sourceFile, sourceFiles)
        QCOMPARE(snapshot.allIncludesForDocument(sourceFile), QSet<QString>() << newHeader);
}

void CppToolsPlugin::test_modelmanager_documentsAndRevisions()
{
    TestCase helper;

    // Index two files
    const MyTestDataDir testDir(_("testdata_project1"));
    const QString filePath1 = testDir.file(QLatin1String("foo.h"));
    const QString filePath2 = testDir.file(QLatin1String("foo.cpp"));
    const QSet<QString> filesToIndex = QSet<QString>() << filePath1 << filePath2;
    QVERIFY(TestCase::parseFiles(filesToIndex));

    CppModelManager *modelManager = CppModelManager::instance();
    VERIFY_DOCUMENT_REVISION(modelManager->document(filePath1), 1U);
    VERIFY_DOCUMENT_REVISION(modelManager->document(filePath2), 1U);

    // Open editor for file 1
    TextEditor::BaseTextEditor *editor1;
    QVERIFY(helper.openBaseTextEditor(filePath1, &editor1));
    helper.closeEditorAtEndOfTestCase(editor1);
    QVERIFY(TestCase::waitForProcessedEditorDocument(filePath1));
    VERIFY_DOCUMENT_REVISION(modelManager->document(filePath1), 2U);
    VERIFY_DOCUMENT_REVISION(modelManager->document(filePath2), 1U);

    // Index again
    QVERIFY(TestCase::parseFiles(filesToIndex));
    VERIFY_DOCUMENT_REVISION(modelManager->document(filePath1), 3U);
    VERIFY_DOCUMENT_REVISION(modelManager->document(filePath2), 2U);

    // Open editor for file 2
    TextEditor::BaseTextEditor *editor2;
    QVERIFY(helper.openBaseTextEditor(filePath2, &editor2));
    helper.closeEditorAtEndOfTestCase(editor2);
    QVERIFY(TestCase::waitForProcessedEditorDocument(filePath2));
    VERIFY_DOCUMENT_REVISION(modelManager->document(filePath1), 3U);
    VERIFY_DOCUMENT_REVISION(modelManager->document(filePath2), 3U);

    // Index again
    QVERIFY(TestCase::parseFiles(filesToIndex));
    VERIFY_DOCUMENT_REVISION(modelManager->document(filePath1), 4U);
    VERIFY_DOCUMENT_REVISION(modelManager->document(filePath2), 4U);
}
