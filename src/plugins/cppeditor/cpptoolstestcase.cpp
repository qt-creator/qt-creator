// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpptoolstestcase.h"

#include "baseeditordocumentparser.h"
#include "baseeditordocumentprocessor.h"
#include "cppeditorwidget.h"
#include "cppmodelmanager.h"
#include "cppworkingcopy.h"
#include "editordocumenthandle.h"
#include "projectinfo.h"

#include <coreplugin/editormanager/editormanager.h>

#include <cplusplus/CppDocument.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/texteditor.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/codeassist/iassistproposalmodel.h>
#include <texteditor/storagesettings.h>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QtTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor::Internal::Tests {

CppTestDocument::CppTestDocument(const QByteArray &fileName, const QByteArray &source,
                                         char cursorMarker)
    : m_fileName(QString::fromUtf8(fileName))
    , m_source(QString::fromUtf8(source))
    , m_cursorMarker(cursorMarker)
    , m_targetCursorPosition(m_source.indexOf(QLatin1Char('$')))
    , m_selectionStartMarker(QLatin1Char(m_cursorMarker) + QLatin1String("{start}"))
    , m_selectionEndMarker(QLatin1Char(m_cursorMarker) + QLatin1String("{end}"))
{
    // Try to find selection markers
    const int selectionStartIndex = m_source.indexOf(m_selectionStartMarker);
    const int selectionEndIndex = m_source.indexOf(m_selectionEndMarker);
    const bool bothSelectionMarkersFound = selectionStartIndex != -1 && selectionEndIndex != -1;
    const bool noneSelectionMarkersFounds = selectionStartIndex == -1 && selectionEndIndex == -1;
    QTC_ASSERT(bothSelectionMarkersFound || noneSelectionMarkersFounds, return);

    if (selectionStartIndex != -1) {
        m_cursorPosition = selectionEndIndex;
        m_anchorPosition = selectionStartIndex;

    // No selection markers found, so check for simple cursorMarker
    } else {
        m_cursorPosition = m_source.indexOf(QLatin1Char(cursorMarker));
    }

    if (m_cursorPosition != -1 || m_targetCursorPosition != -1)
        QVERIFY(m_cursorPosition != m_targetCursorPosition);
}

TestDocumentPtr CppTestDocument::create(const QByteArray &source, const QByteArray &fileName)
{
    const TestDocumentPtr doc(new CppTestDocument(fileName, source));
    doc->removeMarkers();
    return doc;
}

TestDocumentPtr CppTestDocument::create(const QByteArray &fileName, const QByteArray &source,
                                            const QByteArray &expectedSource)
{
    const TestDocumentPtr doc(new CppTestDocument(fileName, source));
    doc->m_expectedSource = QString::fromUtf8(expectedSource);
    doc->removeMarkers();
    return doc;
}

FilePath CppTestDocument::filePath() const
{
    if (!m_baseDirectory.isEmpty())
        return FilePath::fromString(QDir::cleanPath(m_baseDirectory + '/' + m_fileName));

    if (!QFileInfo(m_fileName).isAbsolute())
        return FilePath::fromString(TemporaryDirectory::masterDirectoryPath() + '/' + m_fileName);

    return FilePath::fromString(m_fileName);
}

bool CppTestDocument::writeToDisk() const
{
    return CppEditor::Tests::TestCase::writeFile(filePath(), m_source.toUtf8());
}

void CppTestDocument::removeMarkers()
{
    // Remove selection markers
    if (m_anchorPosition != -1) {
        if (m_anchorPosition < m_cursorPosition) {
            m_source.remove(m_anchorPosition, m_selectionStartMarker.size());
            m_cursorPosition -= m_selectionStartMarker.size();
            m_source.remove(m_cursorPosition, m_selectionEndMarker.size());
        } else {
            m_source.remove(m_cursorPosition, m_selectionEndMarker.size());
            m_anchorPosition -= m_selectionEndMarker.size();
            m_source.remove(m_anchorPosition, m_selectionStartMarker.size());
        }

    // Remove simple cursor marker
    } else if (m_cursorPosition != -1 || m_targetCursorPosition != -1) {
        if (m_cursorPosition > m_targetCursorPosition) {
            m_source.remove(m_cursorPosition, 1);
            if (m_targetCursorPosition != -1) {
                m_source.remove(m_targetCursorPosition, 1);
                --m_cursorPosition;
            }
        } else {
            m_source.remove(m_targetCursorPosition, 1);
            if (m_cursorPosition != -1) {
                m_source.remove(m_cursorPosition, 1);
                --m_targetCursorPosition;
            }
        }
    }

    const int cursorPositionInExpectedSource
        = m_expectedSource.indexOf(QLatin1Char(m_cursorMarker));
    if (cursorPositionInExpectedSource > -1)
        m_expectedSource.remove(cursorPositionInExpectedSource, 1);
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
    RETURN_FALSE_IF_NOT(CppModelManager::projectInfos().isEmpty());
    RETURN_FALSE_IF_NOT(CppModelManager::headerPaths().isEmpty());
    RETURN_FALSE_IF_NOT(CppModelManager::definedMacros().isEmpty());
    RETURN_FALSE_IF_NOT(CppModelManager::projectFiles().isEmpty());
    if (!testOnlyForCleanedProjects) {
        RETURN_FALSE_IF_NOT(CppModelManager::snapshot().isEmpty());
        RETURN_FALSE_IF_NOT(CppModelManager::workingCopy().size() == 1);
        RETURN_FALSE_IF_NOT(CppModelManager::workingCopy()
            .get(CppModelManager::configurationFileName()));
    }
    return true;
}

#undef RETURN_FALSE_IF_NOT

} // namespace CppEditor::Internal::Tests

namespace CppEditor::Tests {

static bool closeEditorsWithoutGarbageCollectorInvocation(const QList<Core::IEditor *> &editors)
{
    CppModelManager::enableGarbageCollector(false);
    const bool closeEditorsSucceeded = Core::EditorManager::closeEditors(editors, false);
    CppModelManager::enableGarbageCollector(true);
    return closeEditorsSucceeded;
}

static bool snapshotContains(const CPlusPlus::Snapshot &snapshot, const QSet<FilePath> &filePaths)
{
    for (const FilePath &filePath : filePaths) {
        if (!snapshot.contains(filePath)) {
            qWarning() << "Missing file in snapshot:" << qPrintable(filePath.toString());
            return false;
        }
    }
    return true;
}

TestCase::TestCase(bool runGarbageCollector)
    : m_succeededSoFar(false)
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

bool TestCase::openCppEditor(const FilePath &filePath, TextEditor::BaseTextEditor **editor,
                             CppEditorWidget **editorWidget)
{
    if (const auto e = dynamic_cast<TextEditor::BaseTextEditor *>(
            Core::EditorManager::openEditor(filePath))) {
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
    return CppModelManager::snapshot();
}

bool TestCase::garbageCollectGlobalSnapshot()
{
    CppModelManager::GC();
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

bool TestCase::waitForProcessedEditorDocument(const FilePath &filePath, int timeOutInMs)
{
    auto *editorDocument = CppModelManager::cppEditorDocument(filePath);
    return waitForProcessedEditorDocument_internal(editorDocument, timeOutInMs);
}

CPlusPlus::Document::Ptr TestCase::waitForRehighlightedSemanticDocument(CppEditorWidget *editorWidget)
{
    while (!editorWidget->isSemanticInfoValid())
        QCoreApplication::processEvents();
    return editorWidget->semanticInfo().doc;
}

bool TestCase::parseFiles(const QSet<FilePath> &filePaths)
{
    CppModelManager::updateSourceFiles(filePaths).waitForFinished();
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
    return parseFiles({FilePath::fromString(filePath)});
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

CPlusPlus::Document::Ptr TestCase::waitForFileInGlobalSnapshot(const FilePath &filePath,
                                                               int timeOutInMs)
{
    const auto documents = waitForFilesInGlobalSnapshot({filePath}, timeOutInMs);
    return documents.isEmpty() ? CPlusPlus::Document::Ptr() : documents.first();
}

QList<CPlusPlus::Document::Ptr> TestCase::waitForFilesInGlobalSnapshot(const FilePaths &filePaths,
                                                                       int timeOutInMs)
{
    QElapsedTimer t;
    t.start();

    QList<CPlusPlus::Document::Ptr> result;
    for (const FilePath &filePath : filePaths) {
        forever {
            if (CPlusPlus::Document::Ptr document = globalSnapshot().document(filePath)) {
                result.append(document);
                break;
            }
            if (t.elapsed() > timeOutInMs)
                return {};
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
            return ProjectManager::startupBuildSystem()
                    && !ProjectManager::startupBuildSystem()->isParsing()
                    && CppModelManager::projectInfo(project);
        },
        timeOutInMs);
}

bool TestCase::writeFile(const FilePath &filePath, const QByteArray &contents)
{
    Utils::FileSaver saver(filePath);
    if (!saver.write(contents) || !saver.finalize()) {
        qWarning() << "Failed to write file to disk:" << qPrintable(filePath.toUserOutput());
        return false;
    }
    return true;
}

ProjectOpenerAndCloser::ProjectOpenerAndCloser()
{
    QVERIFY(!ProjectManager::hasProjects());
}

ProjectOpenerAndCloser::~ProjectOpenerAndCloser()
{
    if (m_openProjects.isEmpty())
        return;

    bool hasGcFinished = false;
    auto connection = QObject::connect(CppModelManager::instance(), &CppModelManager::gcFinished,
                                       [&hasGcFinished] { hasGcFinished = true; });

    for (Project *project : std::as_const(m_openProjects))
        ProjectExplorerPlugin::unloadProject(project);

    QElapsedTimer t;
    t.start();
    while (!hasGcFinished && t.elapsed() <= 30000)
        QCoreApplication::processEvents();

    QObject::disconnect(connection);
}

ProjectInfo::ConstPtr ProjectOpenerAndCloser::open(const FilePath &projectFile,
        bool configureAsExampleProject, Kit *kit)
{
    ProjectExplorerPlugin::OpenProjectResult result =
            ProjectExplorerPlugin::openProject(projectFile);
    if (!result) {
        qWarning() << result.errorMessage() << result.alreadyOpen();
        return {};
    }

    Project *project = result.project();
    if (configureAsExampleProject)
        project->configureAsExampleProject(kit);

    if (TestCase::waitUntilProjectIsFullyOpened(project)) {
        m_openProjects.append(project);
        return CppModelManager::projectInfo(project);
    }

    return {};
}

TemporaryDir::TemporaryDir()
    : m_temporaryDir("qtcreator-tests-XXXXXX")
    , m_isValid(m_temporaryDir.isValid())
{
}

FilePath TemporaryDir::createFile(const QByteArray &relativePath, const QByteArray &contents)
{
    const QString relativePathString = QString::fromUtf8(relativePath);
    if (relativePathString.isEmpty() || QFileInfo(relativePathString).isAbsolute())
        return {};

    const FilePath filePath = m_temporaryDir.filePath(relativePathString);
    if (!TestCase::writeFile(filePath, contents))
        return {};
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

FilePath TemporaryCopiedDir::absolutePath(const QString &relativePath) const
{
    return m_temporaryDir.filePath(relativePath);
}

int clangdIndexingTimeout()
{
    bool isConversionOk = false;
    const int intervalAsInt = qtcEnvironmentVariableIntValue("QTC_CLANGD_INDEXING_TIMEOUT",
                                                             &isConversionOk);
    if (!isConversionOk)
        return Utils::HostOsInfo::isWindowsHost() ? 20000 : 10000;
    return intervalAsInt;
}

} // namespace CppEditor::Tests
