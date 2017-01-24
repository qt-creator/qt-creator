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

#include "cpptoolstestcase.h"

#include "baseeditordocumentparser.h"
#include "baseeditordocumentprocessor.h"
#include "editordocumenthandle.h"
#include "cppmodelmanager.h"
#include "cppworkingcopy.h"
#include "projectinfo.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <texteditor/texteditor.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/codeassist/iassistproposalmodel.h>

#include <cplusplus/CppDocument.h>
#include <utils/executeondestruction.h>
#include <utils/fileutils.h>
#include <utils/temporarydirectory.h>

#include <QtTest>

using namespace ProjectExplorer;

static bool closeEditorsWithoutGarbageCollectorInvocation(const QList<Core::IEditor *> &editors)
{
    CppTools::CppModelManager::instance()->enableGarbageCollector(false);
    const bool closeEditorsSucceeded = Core::EditorManager::closeEditors(editors, false);
    CppTools::CppModelManager::instance()->enableGarbageCollector(true);
    return closeEditorsSucceeded;
}

static bool snapshotContains(const CPlusPlus::Snapshot &snapshot, const QSet<QString> &filePaths)
{
    foreach (const QString &filePath, filePaths) {
        if (!snapshot.contains(filePath)) {
            const QString warning = QLatin1String("Missing file in snapshot: ") + filePath;
            QWARN(qPrintable(warning));
            return false;
        }
    }
    return true;
}

namespace CppTools {
namespace Tests {

TestDocument::TestDocument(const QByteArray &fileName, const QByteArray &source, char cursorMarker)
    : m_fileName(QString::fromUtf8(fileName))
    , m_source(QString::fromUtf8(source))
    , m_cursorMarker(cursorMarker)
{}

QString TestDocument::filePath() const
{
    if (!m_baseDirectory.isEmpty())
        return QDir::cleanPath(m_baseDirectory + QLatin1Char('/') + m_fileName);

    if (!QFileInfo(m_fileName).isAbsolute())
        return Utils::TemporaryDirectory::masterDirectoryPath() + '/' + m_fileName;

    return m_fileName;
}

bool TestDocument::writeToDisk() const
{
    return TestCase::writeFile(filePath(), m_source.toUtf8());
}

TestCase::TestCase(bool runGarbageCollector)
    : m_modelManager(CppModelManager::instance())
    , m_succeededSoFar(false)
    , m_runGarbageCollector(runGarbageCollector)
{
    if (m_runGarbageCollector)
        QVERIFY(garbageCollectGlobalSnapshot());
    m_succeededSoFar = true;
}

TestCase::~TestCase()
{
    QVERIFY(closeEditorsWithoutGarbageCollectorInvocation(m_editorsToClose));
    QCoreApplication::processEvents();

    if (m_runGarbageCollector)
        QVERIFY(garbageCollectGlobalSnapshot());
}

bool TestCase::succeededSoFar() const
{
    return m_succeededSoFar;
}

bool TestCase::openBaseTextEditor(const QString &fileName, TextEditor::BaseTextEditor **editor)
{
    typedef TextEditor::BaseTextEditor BTEditor;
    if (BTEditor *e = qobject_cast<BTEditor *>(Core::EditorManager::openEditor(fileName))) {
        if (editor) {
            *editor = e;
            return true;
        }
    }
    return false;
}

CPlusPlus::Snapshot TestCase::globalSnapshot()
{
    return CppModelManager::instance()->snapshot();
}

bool TestCase::garbageCollectGlobalSnapshot()
{
    CppModelManager::instance()->GC();
    return globalSnapshot().isEmpty();
}

static bool waitForProcessedEditorDocument_internal(CppEditorDocumentHandle *editorDocument,
                                                    int timeOutInMs)
{
    QTC_ASSERT(editorDocument, return false);

    QTime timer;
    timer.start();

    forever {
        if (!editorDocument->processor()->isParserRunning())
            return true;
        if (timer.elapsed() > timeOutInMs)
            return false;

        QCoreApplication::processEvents();
        QThread::msleep(20);
    }
}

bool TestCase::waitForProcessedEditorDocument(const QString &filePath, int timeOutInMs)
{
    auto *editorDocument = CppModelManager::instance()->cppEditorDocument(filePath);
    return waitForProcessedEditorDocument_internal(editorDocument, timeOutInMs);
}

bool TestCase::parseFiles(const QSet<QString> &filePaths)
{
    CppModelManager::instance()->updateSourceFiles(filePaths).waitForFinished();
    QCoreApplication::processEvents();
    const CPlusPlus::Snapshot snapshot = globalSnapshot();
    if (snapshot.isEmpty()) {
        QWARN("After parsing: snapshot is empty.");
        return false;
    }
    if (!snapshotContains(snapshot, filePaths)) {
        QWARN("After parsing: snapshot does not contain all expected files.");
        return false;
    }
    return true;
}

bool TestCase::parseFiles(const QString &filePath)
{
    return parseFiles(QSet<QString>() << filePath);
}

void TestCase::closeEditorAtEndOfTestCase(Core::IEditor *editor)
{
    if (editor && !m_editorsToClose.contains(editor))
        m_editorsToClose.append(editor);
}

bool TestCase::closeEditorWithoutGarbageCollectorInvocation(Core::IEditor *editor)
{
    return closeEditorsWithoutGarbageCollectorInvocation(QList<Core::IEditor *>() << editor);
}

CPlusPlus::Document::Ptr TestCase::waitForFileInGlobalSnapshot(const QString &filePath,
                                                               int timeOutInMs)
{
    const auto documents = waitForFilesInGlobalSnapshot(QStringList(filePath), timeOutInMs);
    return documents.isEmpty() ? CPlusPlus::Document::Ptr() : documents.first();
}

QList<CPlusPlus::Document::Ptr> TestCase::waitForFilesInGlobalSnapshot(const QStringList &filePaths,
                                                                       int timeOutInMs)
{
    QTime t;
    t.start();

    QList<CPlusPlus::Document::Ptr> result;
    foreach (const QString &filePath, filePaths) {
        forever {
            if (CPlusPlus::Document::Ptr document = globalSnapshot().document(filePath)) {
                result.append(document);
                break;
            }
            if (t.elapsed() > timeOutInMs)
                return QList<CPlusPlus::Document::Ptr>();
            QCoreApplication::processEvents();
        }
    }
    return result;
}

bool TestCase::waitUntilCppModelManagerIsAwareOf(Project *project, int timeOutInMs)
{
    if (!project)
        return false;

    QTime t;
    t.start();

    CppModelManager *modelManager = CppModelManager::instance();
    forever {
        if (modelManager->projectInfo(project).isValid())
            return true;
        if (t.elapsed() > timeOutInMs)
            return false;
        QCoreApplication::processEvents();
    }
}

bool TestCase::writeFile(const QString &filePath, const QByteArray &contents)
{
    Utils::FileSaver saver(filePath);
    if (!saver.write(contents) || !saver.finalize()) {
        const QString warning = QLatin1String("Failed to write file to disk: ") + filePath;
        QWARN(qPrintable(warning));
        return false;
    }
    return true;
}

ProjectOpenerAndCloser::ProjectOpenerAndCloser()
{
    QVERIFY(!SessionManager::hasProjects());
}

ProjectOpenerAndCloser::~ProjectOpenerAndCloser()
{
    if (m_openProjects.isEmpty())
        return;

    bool hasGcFinished = false;
    QMetaObject::Connection connection;
    Utils::ExecuteOnDestruction disconnect([&]() { QObject::disconnect(connection); });
    connection = QObject::connect(CppModelManager::instance(), &CppModelManager::gcFinished, [&]() {
        hasGcFinished = true;
    });

    foreach (Project *project, m_openProjects)
        ProjectExplorerPlugin::unloadProject(project);

    QTime t;
    t.start();
    while (!hasGcFinished && t.elapsed() <= 30000)
        QCoreApplication::processEvents();
}

ProjectInfo ProjectOpenerAndCloser::open(const QString &projectFile, bool configureAsExampleProject)
{
    ProjectExplorerPlugin::OpenProjectResult result = ProjectExplorerPlugin::openProject(projectFile);
    if (!result) {
        qWarning() << result.errorMessage() << result.alreadyOpen();
        return ProjectInfo();
    }

    Project *project = result.project();
    if (configureAsExampleProject)
        project->configureAsExampleProject({ });

    if (TestCase::waitUntilCppModelManagerIsAwareOf(project)) {
        m_openProjects.append(project);
        return CppModelManager::instance()->projectInfo(project);
    }

    return ProjectInfo();
}

TemporaryDir::TemporaryDir()
    : m_temporaryDir("qtcreator-tests-XXXXXX")
    , m_isValid(m_temporaryDir.isValid())
{
}

QString TemporaryDir::createFile(const QByteArray &relativePath, const QByteArray &contents)
{
    const QString relativePathString = QString::fromUtf8(relativePath);
    if (relativePathString.isEmpty() || QFileInfo(relativePathString).isAbsolute())
        return QString();

    const QString filePath = m_temporaryDir.path() + QLatin1Char('/') + relativePathString;
    if (!TestCase::writeFile(filePath, contents))
        return QString();
    return filePath;
}

static bool copyRecursively(const QString &sourceDirPath,
                            const QString &targetDirPath,
                            QString *error)
{
    auto copyHelper = [](QFileInfo sourceInfo, QFileInfo targetInfo, QString *error) -> bool {
        const QString sourcePath = sourceInfo.absoluteFilePath();
        const QString targetPath = targetInfo.absoluteFilePath();
        if (!QFile::copy(sourcePath, targetPath)) {
            if (error) {
                *error = QString::fromLatin1("copyRecursively() failed: \"%1\" to \"%2\".")
                            .arg(sourcePath, targetPath);
            }
            return false;
        }

        // Copied files from Qt resources are read-only. Make them writable
        // so that their parent directory can be removed without warnings.
        QFile file(targetPath);
        return file.setPermissions(file.permissions() | QFile::WriteUser);
    };

    return Utils::FileUtils::copyRecursively(Utils::FileName::fromString(sourceDirPath),
                                             Utils::FileName::fromString(targetDirPath),
                                             error,
                                             copyHelper);
}

TemporaryCopiedDir::TemporaryCopiedDir(const QString &sourceDirPath)
{
    if (!m_isValid)
        return;

    if (sourceDirPath.isEmpty())
        return;

    QFileInfo fi(sourceDirPath);
    if (!fi.exists() || !fi.isReadable()) {
        m_isValid = false;
        return;
    }

    QString errorMessage;
    if (!copyRecursively(sourceDirPath, path(), &errorMessage)) {
        QWARN(qPrintable(errorMessage));
        m_isValid = false;
    }
}

QString TemporaryCopiedDir::absolutePath(const QByteArray &relativePath) const
{
    return m_temporaryDir.path() + QLatin1Char('/') + QString::fromUtf8(relativePath);
}

FileWriterAndRemover::FileWriterAndRemover(const QString &filePath, const QByteArray &contents)
    : m_filePath(filePath)
{
    if (QFileInfo::exists(filePath)) {
        const QString warning = QString::fromLatin1(
            "Will not overwrite existing file: \"%1\"."
            " If this file is left over due to a(n) abort/crash, please remove manually.")
                .arg(m_filePath);
        QWARN(qPrintable(warning));
        m_writtenSuccessfully = false;
    } else {
        m_writtenSuccessfully = TestCase::writeFile(filePath, contents);
    }
}

FileWriterAndRemover::~FileWriterAndRemover()
{
    if (m_writtenSuccessfully && !QFile::remove(m_filePath)) {
        const QString warning = QLatin1String("Failed to remove file from disk: ") + m_filePath;
        QWARN(qPrintable(warning));
    }
}

IAssistProposalScopedPointer::IAssistProposalScopedPointer(TextEditor::IAssistProposal *proposal)
    : d(proposal)
{}

IAssistProposalScopedPointer::~IAssistProposalScopedPointer()
{
    if (d && d->model())
        delete d->model();
}

VerifyCleanCppModelManager::VerifyCleanCppModelManager()
{
    QVERIFY(isClean());
}

VerifyCleanCppModelManager::~VerifyCleanCppModelManager() {
    QVERIFY(isClean());
}

#define RETURN_FALSE_IF_NOT(check) if (!(check)) return false;

bool VerifyCleanCppModelManager::isClean(bool testOnlyForCleanedProjects)
{
    CppModelManager *mm = CppModelManager::instance();
    RETURN_FALSE_IF_NOT(mm->projectInfos().isEmpty());
    RETURN_FALSE_IF_NOT(mm->headerPaths().isEmpty());
    RETURN_FALSE_IF_NOT(mm->definedMacros().isEmpty());
    RETURN_FALSE_IF_NOT(mm->projectFiles().isEmpty());
    if (!testOnlyForCleanedProjects)
        RETURN_FALSE_IF_NOT(mm->snapshot().isEmpty());
    return true;
}

#undef RETURN_FALSE_IF_NOT

} // namespace Tests
} // namespace CppTools
