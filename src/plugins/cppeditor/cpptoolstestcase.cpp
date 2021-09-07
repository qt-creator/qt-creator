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
#include "cppeditorwidget.h"
#include "cppmodelmanager.h"
#include "cppworkingcopy.h"
#include "editordocumenthandle.h"
#include "projectinfo.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <texteditor/texteditor.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/codeassist/iassistproposalmodel.h>
#include <texteditor/storagesettings.h>

#include <cplusplus/CppDocument.h>
#include <utils/executeondestruction.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/temporarydirectory.h>

#include <QtTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor::Tests {

static bool closeEditorsWithoutGarbageCollectorInvocation(const QList<Core::IEditor *> &editors)
{
    CppModelManager::instance()->enableGarbageCollector(false);
    const bool closeEditorsSucceeded = Core::EditorManager::closeEditors(editors, false);
    CppModelManager::instance()->enableGarbageCollector(true);
    return closeEditorsSucceeded;
}

static bool snapshotContains(const CPlusPlus::Snapshot &snapshot, const QSet<QString> &filePaths)
{
    for (const QString &filePath : filePaths) {
        if (!snapshot.contains(filePath)) {
            qWarning() << "Missing file in snapshot:" << qPrintable(filePath);
            return false;
        }
    }
    return true;
}

BaseCppTestDocument::BaseCppTestDocument(const QByteArray &fileName, const QByteArray &source, char cursorMarker)
    : m_fileName(QString::fromUtf8(fileName))
    , m_source(QString::fromUtf8(source))
    , m_cursorMarker(cursorMarker)
{}

QString BaseCppTestDocument::filePath() const
{
    if (!m_baseDirectory.isEmpty())
        return QDir::cleanPath(m_baseDirectory + QLatin1Char('/') + m_fileName);

    if (!QFileInfo(m_fileName).isAbsolute())
        return Utils::TemporaryDirectory::masterDirectoryPath() + '/' + m_fileName;

    return m_fileName;
}

bool BaseCppTestDocument::writeToDisk() const
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

bool TestCase::openCppEditor(const QString &fileName, TextEditor::BaseTextEditor **editor,
                             CppEditorWidget **editorWidget)
{
    if (const auto e = dynamic_cast<TextEditor::BaseTextEditor *>(Core::EditorManager::openEditor(fileName))) {
        if (editor) {
            *editor = e;
            TextEditor::StorageSettings s = e->textDocument()->storageSettings();
            s.m_addFinalNewLine = false;
            e->textDocument()->setStorageSettings(s);
        }
        if (editorWidget) {
            if (CppEditorWidget *w = dynamic_cast<CppEditorWidget *>(e->editorWidget())) {
                *editorWidget = w;
                return true;
            } else {
                return false; // no or wrong widget
            }
        } else {
            return true; // ok since no widget requested
        }
    } else {
        return false; // no or wrong editor
    }
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

    QElapsedTimer timer;
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

CPlusPlus::Document::Ptr TestCase::waitForRehighlightedSemanticDocument(CppEditorWidget *editorWidget)
{
    while (!editorWidget->isSemanticInfoValid())
        QCoreApplication::processEvents();
    return editorWidget->semanticInfo().doc;
}

bool TestCase::parseFiles(const QSet<QString> &filePaths)
{
    CppModelManager::instance()->updateSourceFiles(filePaths).waitForFinished();
    QCoreApplication::processEvents();
    const CPlusPlus::Snapshot snapshot = globalSnapshot();
    if (snapshot.isEmpty()) {
        qWarning("After parsing: snapshot is empty.");
        return false;
    }
    if (!snapshotContains(snapshot, filePaths)) {
        qWarning("After parsing: snapshot does not contain all expected files.");
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
    return closeEditorsWithoutGarbageCollectorInvocation({editor});
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
    QElapsedTimer t;
    t.start();

    QList<CPlusPlus::Document::Ptr> result;
    for (const QString &filePath : filePaths) {
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

bool TestCase::waitUntilProjectIsFullyOpened(Project *project, int timeOutInMs)
{
    if (!project)
        return false;

    return QTest::qWaitFor(
        [project]() {
            return SessionManager::startupBuildSystem()
                    && !SessionManager::startupBuildSystem()->isParsing()
                    && CppModelManager::instance()->projectInfo(project);
        },
        timeOutInMs);
}

bool TestCase::writeFile(const QString &filePath, const QByteArray &contents)
{
    Utils::FileSaver saver(Utils::FilePath::fromString(filePath));
    if (!saver.write(contents) || !saver.finalize()) {
        qWarning() << "Failed to write file to disk:" << qPrintable(filePath);
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

    QElapsedTimer t;
    t.start();
    while (!hasGcFinished && t.elapsed() <= 30000)
        QCoreApplication::processEvents();
}

ProjectInfo::ConstPtr ProjectOpenerAndCloser::open(const QString &projectFile,
        bool configureAsExampleProject, Kit *kit)
{
    ProjectExplorerPlugin::OpenProjectResult result =
            ProjectExplorerPlugin::openProject(FilePath::fromString(projectFile));
    if (!result) {
        qWarning() << result.errorMessage() << result.alreadyOpen();
        return {};
    }

    Project *project = result.project();
    if (configureAsExampleProject)
        project->configureAsExampleProject(kit);

    if (TestCase::waitUntilProjectIsFullyOpened(project)) {
        m_openProjects.append(project);
        return CppModelManager::instance()->projectInfo(project);
    }

    return {};
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

    const QString filePath = m_temporaryDir.filePath(relativePathString).path();
    if (!TestCase::writeFile(filePath, contents))
        return QString();
    return filePath;
}

static bool copyRecursively(const QString &sourceDirPath,
                            const QString &targetDirPath,
                            QString *error)
{
    auto copyHelper = [](const FilePath &sourcePath, const FilePath &targetPath, QString *error) -> bool {
        if (!sourcePath.copyFile(targetPath)) {
            if (error) {
                *error = QString::fromLatin1("copyRecursively() failed: \"%1\" to \"%2\".")
                            .arg(sourcePath.toUserOutput(), targetPath.toUserOutput());
            }
            return false;
        }

        // Copied files from Qt resources are read-only. Make them writable
        // so that their parent directory can be removed without warnings.
        return targetPath.setPermissions(targetPath.permissions() | QFile::WriteUser);
    };

    return Utils::FileUtils::copyRecursively(Utils::FilePath::fromString(sourceDirPath),
                                             Utils::FilePath::fromString(targetDirPath),
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
        qWarning() << qPrintable(errorMessage);
        m_isValid = false;
    }
}

QString TemporaryCopiedDir::absolutePath(const QByteArray &relativePath) const
{
    return m_temporaryDir.filePath(QString::fromUtf8(relativePath)).path();
}

FileWriterAndRemover::FileWriterAndRemover(const QString &filePath, const QByteArray &contents)
    : m_filePath(filePath)
{
    if (QFileInfo::exists(filePath)) {
        qWarning().nospace() << "Will not overwrite existing file: " << m_filePath << "."
            << " If this file is left over due to a(n) abort/crash, please remove manually.";
        m_writtenSuccessfully = false;
    } else {
        m_writtenSuccessfully = TestCase::writeFile(filePath, contents);
    }
}

FileWriterAndRemover::~FileWriterAndRemover()
{
    if (m_writtenSuccessfully && !QFile::remove(m_filePath))
        qWarning() << "Failed to remove file from disk:" << qPrintable(m_filePath);
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
    if (!testOnlyForCleanedProjects) {
        RETURN_FALSE_IF_NOT(mm->snapshot().isEmpty());
        RETURN_FALSE_IF_NOT(mm->workingCopy().size() == 1);
        RETURN_FALSE_IF_NOT(mm->workingCopy().contains(mm->configurationFileName()));
    }
    return true;
}

#undef RETURN_FALSE_IF_NOT

int clangdIndexingTimeout()
{
    const QByteArray timeoutAsByteArray = qgetenv("QTC_CLANGD_INDEXING_TIMEOUT");
    bool isConversionOk = false;
    const int intervalAsInt = timeoutAsByteArray.toInt(&isConversionOk);
    if (!isConversionOk)
        return Utils::HostOsInfo::isWindowsHost() ? 20000 : 10000;
    return intervalAsInt;
}

} // namespace CppEditor::Tests
