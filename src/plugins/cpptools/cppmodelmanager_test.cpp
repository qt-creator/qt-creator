/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cpptoolsplugin.h"

#include "cpppreprocessor.h"
#include "modelmanagertesthelper.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QDebug>
#include <QFileInfo>
#include <QtTest>

using namespace CppTools::Internal;

typedef CPlusPlus::Document Document;
typedef CppTools::CppModelManagerInterface::ProjectInfo ProjectInfo;
typedef CppTools::ProjectPart ProjectPart;
typedef CppTools::ProjectFile ProjectFile;
typedef ProjectExplorer::Project Project;

namespace {

class TestDataDirectory
{
public:
    TestDataDirectory(const QString &testDataDirectory)
        : m_testDataDirectory(QLatin1String(SRCDIR "/../../../tests/cppmodelmanager/")
                              + testDataDirectory)
    {
        QFileInfo testDataDir(m_testDataDirectory);
        QVERIFY(testDataDir.exists());
        QVERIFY(testDataDir.isDir());
    }


    QString includeDir(bool cleaned = true) const
    {
        return testDataDir(QLatin1String("include"), cleaned);
    }

    QString frameworksDir(bool cleaned = true) const
    {
        return testDataDir(QLatin1String("frameworks"), cleaned);
    }

    QString fileFromSourcesDir(const QString &fileName) const
    {
        return testDataDir(QLatin1String("sources")) + fileName;
    }

    /// File from the test data directory (top leve)
    QString file(const QString &fileName) const
    {
        return testDataDir(QString()) + fileName;
    }

private:
    QString testDataDir(const QString& subdir, bool cleaned = true) const
    {
        QString path = m_testDataDirectory;
        if (!subdir.isEmpty())
            path += QLatin1String("/") + subdir;
        if (cleaned)
            return CppPreprocessor::cleanPath(path);
        else
            return path;
    }

private:
    const QString m_testDataDirectory;
};


// TODO: When possible, use this helper class in all tests
class ProjectCreator
{
public:
    ProjectCreator(ModelManagerTestHelper *modelManagerTestHelper)
        : modelManagerTestHelper(modelManagerTestHelper)
    {}

    /// 'files' is expected to be a list of file names that reside in 'dir'.
    void create(const QString &name, const QString &dir, const QStringList files)
    {
        const TestDataDirectory projectDir(dir);
        foreach (const QString &file, files)
            projectFiles << projectDir.file(file);

        Project *project = modelManagerTestHelper->createProject(name);
        projectInfo = CppModelManager::instance()->projectInfo(project);
        QCOMPARE(projectInfo.project().data(), project);

        ProjectPart::Ptr part(new ProjectPart);
        projectInfo.appendProjectPart(part);
        part->cxxVersion = ProjectPart::CXX98;
        part->qtVersion = ProjectPart::Qt5;
        foreach (const QString &file, projectFiles) {
            ProjectFile projectFile(file, ProjectFile::classify(file));
            part->files.append(projectFile);
        }
    }

    ModelManagerTestHelper *modelManagerTestHelper;
    ProjectInfo projectInfo;
    QStringList projectFiles;
};


} // anonymous namespace

void CppToolsPlugin::test_modelmanager_paths()
{
    ModelManagerTestHelper helper;
    CppModelManager *mm = CppModelManager::instance();

    const TestDataDirectory testDataDir(QLatin1String("testdata"));

    Project *project = helper.createProject(QLatin1String("test_modelmanager_paths"));
    ProjectInfo pi = mm->projectInfo(project);
    QCOMPARE(pi.project().data(), project);

    ProjectPart::Ptr part(new ProjectPart);
    pi.appendProjectPart(part);
    part->cxxVersion = ProjectPart::CXX98;
    part->qtVersion = ProjectPart::Qt5;
    part->defines = QByteArray("#define OH_BEHAVE -1\n");
    part->includePaths = QStringList() << testDataDir.includeDir(false);
    part->frameworkPaths = QStringList() << testDataDir.frameworksDir(false);

    mm->updateProjectInfo(pi);

    QStringList includePaths = mm->includePaths();
    QCOMPARE(includePaths.size(), 1);
    QVERIFY(includePaths.contains(testDataDir.includeDir()));

    QStringList frameworkPaths = mm->frameworkPaths();
    QCOMPARE(frameworkPaths.size(), 1);
    QVERIFY(frameworkPaths.contains(testDataDir.frameworksDir()));
}

void CppToolsPlugin::test_modelmanager_framework_headers()
{
    ModelManagerTestHelper helper;
    CppModelManager *mm = CppModelManager::instance();

    const TestDataDirectory testDataDir(QLatin1String("testdata"));

    Project *project = helper.createProject(QLatin1String("test_modelmanager_framework_headers"));
    ProjectInfo pi = mm->projectInfo(project);
    QCOMPARE(pi.project().data(), project);

    ProjectPart::Ptr part(new ProjectPart);
    pi.appendProjectPart(part);
    part->cxxVersion = ProjectPart::CXX98;
    part->qtVersion = ProjectPart::Qt5;
    part->defines = QByteArray("#define OH_BEHAVE -1\n");
    part->includePaths << testDataDir.includeDir();
    part->frameworkPaths << testDataDir.frameworksDir();
    const QString &source = testDataDir.fileFromSourcesDir(
        QLatin1String("test_modelmanager_framework_headers.cpp"));
    part->files << ProjectFile(source, ProjectFile::CXXSource);

    mm->updateProjectInfo(pi);
    mm->updateSourceFiles(QStringList(source)).waitForFinished();
    QCoreApplication::processEvents();

    QVERIFY(mm->snapshot().contains(source));
    Document::Ptr doc = mm->snapshot().document(source);
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
void CppToolsPlugin::test_modelmanager_refresh_1()
{
    ModelManagerTestHelper helper;
    CppModelManager *mm = CppModelManager::instance();

    const TestDataDirectory testDataDir(QLatin1String("testdata"));

    const QString testCpp(testDataDir.fileFromSourcesDir(
        QLatin1String("test_modelmanager_refresh.cpp")));
    const QString testHeader(testDataDir.fileFromSourcesDir(
        QLatin1String("test_modelmanager_refresh.h")));

    Project *project = helper.createProject(QLatin1String("test_modelmanager_refresh_1"));
    ProjectInfo pi = mm->projectInfo(project);
    QCOMPARE(pi.project().data(), project);

    ProjectPart::Ptr part(new ProjectPart);
    pi.appendProjectPart(part);
    part->cxxVersion = ProjectPart::CXX98;
    part->qtVersion = ProjectPart::Qt5;
    part->defines = QByteArray("#define OH_BEHAVE -1\n");
    part->includePaths = QStringList() << testDataDir.includeDir(false);
    part->files.append(ProjectFile(testCpp, ProjectFile::CXXSource));

    mm->updateProjectInfo(pi);
    mm->updateSourceFiles(QStringList() << testCpp);

    QStringList refreshedFiles = helper.waitForRefreshedSourceFiles();

    QCOMPARE(refreshedFiles.size(), 1);
    QVERIFY(refreshedFiles.contains(testCpp));
    CPlusPlus::Snapshot snapshot = mm->snapshot();
    QVERIFY(snapshot.contains(testHeader));
    QVERIFY(snapshot.contains(testCpp));

    part->defines = QByteArray();
    mm->updateProjectInfo(pi);
    snapshot = mm->snapshot();
    QVERIFY(!snapshot.contains(testHeader));
    QVERIFY(!snapshot.contains(testCpp));

    mm->updateSourceFiles(QStringList() << testCpp);
    refreshedFiles = helper.waitForRefreshedSourceFiles();

    QCOMPARE(refreshedFiles.size(), 1);
    QVERIFY(refreshedFiles.contains(testCpp));
    snapshot = mm->snapshot();
    QVERIFY(snapshot.contains(testHeader));
    QVERIFY(snapshot.contains(testCpp));
}

/// QTCREATORBUG-9205
void CppToolsPlugin::test_modelmanager_refresh_2()
{
    ModelManagerTestHelper helper;
    CppModelManager *mm = CppModelManager::instance();

    const TestDataDirectory testDataDir(QLatin1String("testdata_refresh"));

    const QString testHeader1(testDataDir.file(QLatin1String("defines.h")));
    const QString testHeader2(testDataDir.file(QLatin1String("header.h")));
    const QString testCpp(testDataDir.file(QLatin1String("source.cpp")));

    Project *project = helper.createProject(QLatin1String("test_modelmanager_refresh_2"));
    ProjectInfo pi = mm->projectInfo(project);
    QCOMPARE(pi.project().data(), project);

    ProjectPart::Ptr part(new ProjectPart);
    pi.appendProjectPart(part);
    part->cxxVersion = ProjectPart::CXX98;
    part->qtVersion = ProjectPart::Qt5;
    part->files.append(ProjectFile(testHeader1, ProjectFile::CXXHeader));
    part->files.append(ProjectFile(testHeader2, ProjectFile::CXXHeader));
    part->files.append(ProjectFile(testCpp, ProjectFile::CXXSource));

    mm->updateProjectInfo(pi);

    CPlusPlus::Snapshot snapshot;
    QStringList refreshedFiles;
    CPlusPlus::Document::Ptr document;

    for (int i = 0; i < 2; ++i) {
        mm->updateSourceFiles(QStringList() << testHeader1 << testHeader2 << testCpp);
        refreshedFiles = helper.waitForRefreshedSourceFiles();

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

void CppToolsPlugin::test_modelmanager_snapshot_after_two_projects()
{
    QStringList refreshedFiles;
    ModelManagerTestHelper helper;
    ProjectCreator project1(&helper);
    ProjectCreator project2(&helper);
    CppModelManager *mm = CppModelManager::instance();

    // Project 1
    project1.create(QLatin1String("snapshot_after_two_projects.1"),
                    QLatin1String("testdata_project1"),
                    QStringList() << QLatin1String("foo.h")
                                  << QLatin1String("foo.cpp")
                                  << QLatin1String("main.cpp"));

    mm->updateProjectInfo(project1.projectInfo);
    mm->updateSourceFiles(project1.projectFiles);
    refreshedFiles = helper.waitForRefreshedSourceFiles();
    QCOMPARE(refreshedFiles.toSet(), project1.projectFiles.toSet());
    const int snapshotSizeAfterProject1 = mm->snapshot().size();

    foreach (const QString &file, project1.projectFiles)
        QVERIFY(mm->snapshot().contains(file));

    // Project 2
    project2.create(QLatin1String("snapshot_after_two_projects.2"),
                    QLatin1String("testdata_project2"),
                    QStringList() << QLatin1String("bar.h")
                                  << QLatin1String("bar.cpp")
                                  << QLatin1String("main.cpp"));

    mm->updateProjectInfo(project2.projectInfo);
    mm->updateSourceFiles(project2.projectFiles);
    refreshedFiles = helper.waitForRefreshedSourceFiles();
    QCOMPARE(refreshedFiles.toSet(), project2.projectFiles.toSet());

    const int snapshotSizeAfterProject2 = mm->snapshot().size();
    QVERIFY(snapshotSizeAfterProject2 > snapshotSizeAfterProject1);
    QVERIFY(snapshotSizeAfterProject2 >= snapshotSizeAfterProject1 + project2.projectFiles.size());

    foreach (const QString &file, project1.projectFiles)
        QVERIFY(mm->snapshot().contains(file));
    foreach (const QString &file, project2.projectFiles)
        QVERIFY(mm->snapshot().contains(file));
}

void CppToolsPlugin::test_modelmanager_extraeditorsupport_uiFiles()
{
    TestDataDirectory testDataDirectory(QLatin1String("testdata_guiproject1"));
    const QString projectFile = testDataDirectory.file(QLatin1String("testdata_guiproject1.pro"));

    // Open project with *.ui file
    ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();
    QString errorOpeningProject;
    Project *project = pe->openProject(projectFile, &errorOpeningProject);
    QVERIFY(errorOpeningProject.isEmpty());
    project->configureAsExampleProject(QStringList());

    // Check working copy.
    // An AbstractEditorSupport object should have been added for the ui_* file.
    CppModelManagerInterface *mm = CppModelManagerInterface::instance();
    CppModelManagerInterface::WorkingCopy workingCopy = mm->workingCopy();

    QCOMPARE(workingCopy.size(), 2); // mm->configurationFileName() and "ui_*.h"

    QStringList fileNamesInWorkinCopy;
    QHashIterator<QString, QPair<QString, unsigned> > it = workingCopy.iterator();
    while (it.hasNext()) {
        it.next();
        fileNamesInWorkinCopy << QFileInfo(it.key()).fileName();
    }
    fileNamesInWorkinCopy.sort();
    const QString expectedUiHeaderFileName = QLatin1String("ui_mainwindow.h");
    QCOMPARE(fileNamesInWorkinCopy.at(0), mm->configurationFileName());
    QCOMPARE(fileNamesInWorkinCopy.at(1), expectedUiHeaderFileName);

    // Check CppPreprocessor / includes.
    // The CppPreprocessor is expected to find the ui_* file in the working copy.
    const QString fileIncludingTheUiFile = testDataDirectory.file(QLatin1String("mainwindow.cpp"));
    while (!mm->snapshot().document(fileIncludingTheUiFile))
        QCoreApplication::processEvents();

    const CPlusPlus::Snapshot snapshot = mm->snapshot();
    const Document::Ptr document = snapshot.document(fileIncludingTheUiFile);
    QVERIFY(document);
    const QStringList includedFiles = document->includedFiles();
    QCOMPARE(includedFiles.size(), 2);
    QCOMPARE(QFileInfo(includedFiles.at(0)).fileName(), QLatin1String("mainwindow.h"));
    QCOMPARE(QFileInfo(includedFiles.at(1)).fileName(), QLatin1String("ui_mainwindow.h"));

    // Close Project
    ProjectExplorer::SessionManager *sm = pe->session();
    sm->removeProject(project);
    ModelManagerTestHelper::verifyClean();
}

/// QTCREATORBUG-9828: Locator shows symbols of closed files
/// Check: The garbage collector should be run if the last CppEditor is closed.
void CppToolsPlugin::test_modelmanager_gc_if_last_cppeditor_closed()
{
    TestDataDirectory testDataDirectory(QLatin1String("testdata_guiproject1"));
    const QString file = testDataDirectory.file(QLatin1String("main.cpp"));

    Core::EditorManager *em = Core::EditorManager::instance();
    CppModelManager *mm = CppModelManager::instance();

    // Open a file in the editor
    QCOMPARE(Core::EditorManager::documentModel()->openedDocuments().size(), 0);
    Core::IEditor *editor = em->openEditor(file);
    QVERIFY(editor);
    QCOMPARE(Core::EditorManager::documentModel()->openedDocuments().size(), 1);
    QVERIFY(mm->isCppEditor(editor));
    QVERIFY(mm->workingCopy().contains(file));

    // Check: File is in the snapshot
    QVERIFY(mm->snapshot().contains(file));

    // Close file/editor
    const QList<Core::IEditor*> editorsToClose = QList<Core::IEditor*>() << editor;
    em->closeEditors(editorsToClose, /*askAboutModifiedEditors=*/ false);

    // Check: File is removed from the snapshpt
    QVERIFY(!mm->workingCopy().contains(file));
    QVERIFY(!mm->snapshot().contains(file));
}
