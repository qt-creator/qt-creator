// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "projectinfo.h"

#include <cplusplus/CppDocument.h>
#include <utils/temporarydirectory.h>

#include <QEventLoop>
#include <QStringList>
#include <QTimer>

namespace CPlusPlus {
class Document;
class Snapshot;
}

namespace Core { class IEditor; }
namespace ProjectExplorer {
class Kit;
class Project;
}

namespace TextEditor {
class BaseTextEditor;
class IAssistProposal;
}

namespace CppEditor {
class CppEditorWidget;

namespace Internal::Tests {

class CppTestDocument;
typedef QSharedPointer<CppTestDocument> TestDocumentPtr;

/**
 * Represents a test document.
 *
 * The source can contain special characters:
 *   - A '@' character denotes the initial text cursor position
 *   - A '$' character denotes the target text cursor position (for "follow symbol" type of tests)
 *   - For selections, the markers "@{start}" and "@{end}" can be used.
 */
class CppTestDocument
{
public:
    CppTestDocument(const QByteArray &fileName, const QByteArray &source,
                    char cursorMarker = '@');

    static TestDocumentPtr create(const QByteArray &source, const QByteArray &fileName);
    static TestDocumentPtr create(const QByteArray &fileName, const QByteArray &source,
                                  const QByteArray &expectedSource);

    QString baseDirectory() const { return m_baseDirectory; }
    void setBaseDirectory(const QString &baseDirectory) { m_baseDirectory = baseDirectory; }

    Utils::FilePath filePath() const;
    bool writeToDisk() const;

    bool hasCursorMarker() const { return m_cursorPosition != -1; }
    bool hasAnchorMarker() const { return m_anchorPosition != -1; }
    bool hasTargetCursorMarker() const { return m_targetCursorPosition != -1; }

    void removeMarkers();

    QString m_baseDirectory;
    QString m_fileName;
    QString m_source;
    char m_cursorMarker;
    int m_targetCursorPosition;
    int m_cursorPosition = -1;
    int m_anchorPosition = -1;
    QString m_selectionStartMarker;
    QString m_selectionEndMarker;
    QString m_expectedSource;
    TextEditor::BaseTextEditor *m_editor = nullptr;
    CppEditorWidget *m_editorWidget = nullptr;
};

using TestDocuments = QVector<CppTestDocument>;

class VerifyCleanCppModelManager
{
public:
    VerifyCleanCppModelManager();
    ~VerifyCleanCppModelManager();
    static bool isClean(bool testCleanedProjects = true);
};

} // namespace Internal::Tests

namespace Tests {

int CPPEDITOR_EXPORT clangdIndexingTimeout();

template <typename Signal> inline bool waitForSignalOrTimeout(
        const typename QtPrivate::FunctionPointer<Signal>::Object *sender, Signal signal,
        int timeoutInMs)
{
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutInMs);
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(sender, signal, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();
    return timer.isActive();
}

class CPPEDITOR_EXPORT TestCase
{
    Q_DISABLE_COPY(TestCase)

public:
    explicit TestCase(bool runGarbageCollector = true);
    ~TestCase();

    bool succeededSoFar() const;
    static bool openCppEditor(const Utils::FilePath &filePath, TextEditor::BaseTextEditor **editor,
                              CppEditorWidget **editorWidget = nullptr);
    void closeEditorAtEndOfTestCase(Core::IEditor *editor);

    static bool closeEditorWithoutGarbageCollectorInvocation(Core::IEditor *editor);

    static bool parseFiles(const QString &filePath);
    static bool parseFiles(const QSet<Utils::FilePath> &filePaths);

    static CPlusPlus::Snapshot globalSnapshot();
    static bool garbageCollectGlobalSnapshot();

    static bool waitForProcessedEditorDocument(
            const Utils::FilePath &filePath, int timeOutInMs = 5000);

    static CPlusPlus::Document::Ptr waitForRehighlightedSemanticDocument(
            CppEditorWidget *editorWidget);

    enum { defaultTimeOutInMs = 30 * 1000 /*= 30 secs*/ };
    static bool waitUntilProjectIsFullyOpened(ProjectExplorer::Project *project,
                                              int timeOutInMs = defaultTimeOutInMs);
    static CPlusPlus::Document::Ptr waitForFileInGlobalSnapshot(const Utils::FilePath &filePath,
            int timeOutInMs = defaultTimeOutInMs);
    static QList<CPlusPlus::Document::Ptr> waitForFilesInGlobalSnapshot(
            const Utils::FilePaths &filePaths, int timeOutInMs = defaultTimeOutInMs);

    static bool writeFile(const Utils::FilePath &filePath, const QByteArray &contents);

protected:
    bool m_succeededSoFar;

private:
    QList<Core::IEditor *> m_editorsToClose;
    bool m_runGarbageCollector;
};

class CPPEDITOR_EXPORT ProjectOpenerAndCloser
{
public:
    ProjectOpenerAndCloser();
    ~ProjectOpenerAndCloser(); // Closes opened projects

    ProjectInfo::ConstPtr open(
            const Utils::FilePath &projectFile,
            bool configureAsExampleProject = false,
            ProjectExplorer::Kit *kit = nullptr);

private:
    QList<ProjectExplorer::Project *> m_openProjects;
};

class CPPEDITOR_EXPORT TemporaryDir
{
    Q_DISABLE_COPY(TemporaryDir)

public:
    TemporaryDir();

    bool isValid() const { return m_isValid; }
    QString path() const { return m_temporaryDir.path().path(); }
    Utils::FilePath filePath() const { return m_temporaryDir.path(); }

    Utils::FilePath createFile(const QByteArray &relativePath, const QByteArray &contents);

protected:
    Utils::TemporaryDirectory m_temporaryDir;
    bool m_isValid;
};

class CPPEDITOR_EXPORT TemporaryCopiedDir : public TemporaryDir
{
public:
    explicit TemporaryCopiedDir(const QString &sourceDirPath);
    Utils::FilePath absolutePath(const QString &relativePath) const;

private:
    TemporaryCopiedDir();
};

} // namespace Tests
} // namespace CppEditor
