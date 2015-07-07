/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPTOOLSTESTCASE_H
#define CPPTOOLSTESTCASE_H

#include "cpptools_global.h"

#include <cplusplus/CppDocument.h>

#include <QStringList>
#include <QTemporaryDir>

namespace CPlusPlus {
class Document;
class Snapshot;
}

namespace Core { class IEditor; }
namespace ProjectExplorer { class Project; }

namespace TextEditor {
class BaseTextEditor;
class IAssistProposal;
}

namespace CppTools {
class CppModelManager;
class ProjectInfo;

namespace Tests {

class CPPTOOLS_EXPORT TestDocument
{
public:
    TestDocument(const QByteArray &fileName, const QByteArray &source, char cursorMarker = '@');

    QString baseDirectory() const { return m_baseDirectory; }
    void setBaseDirectory(const QString &baseDirectory) { m_baseDirectory = baseDirectory; }

    QString filePath() const;
    bool writeToDisk() const;

public:
    QString m_baseDirectory;
    QString m_fileName;
    QString m_source;
    char m_cursorMarker;
};

class CPPTOOLS_EXPORT TestCase
{
    Q_DISABLE_COPY(TestCase)

public:
    TestCase(bool runGarbageCollector = true);
    ~TestCase();

    bool succeededSoFar() const;
    bool openBaseTextEditor(const QString &fileName, TextEditor::BaseTextEditor **editor);
    void closeEditorAtEndOfTestCase(Core::IEditor *editor);

    static bool closeEditorWithoutGarbageCollectorInvocation(Core::IEditor *editor);

    static bool parseFiles(const QString &filePath);
    static bool parseFiles(const QSet<QString> &filePaths);

    static CPlusPlus::Snapshot globalSnapshot();
    static bool garbageCollectGlobalSnapshot();

    static bool waitForProcessedEditorDocument(const QString &filePath, int timeOutInMs = 5000);

    enum { defaultTimeOutInMs = 30 * 1000 /*= 30 secs*/ };
    static bool waitUntilCppModelManagerIsAwareOf(
            ProjectExplorer::Project *project,
            int timeOutInMs = defaultTimeOutInMs);
    static CPlusPlus::Document::Ptr waitForFileInGlobalSnapshot(
            const QString &filePath,
            int timeOutInMs = defaultTimeOutInMs);
    static QList<CPlusPlus::Document::Ptr> waitForFilesInGlobalSnapshot(
            const QStringList &filePaths,
            int timeOutInMs = defaultTimeOutInMs);

    static bool writeFile(const QString &filePath, const QByteArray &contents);

protected:
    CppModelManager *m_modelManager;
    bool m_succeededSoFar;

private:
    QList<Core::IEditor *> m_editorsToClose;
    bool m_runGarbageCollector;
};

class CPPTOOLS_EXPORT ProjectOpenerAndCloser
{
public:
    ProjectOpenerAndCloser();
    ~ProjectOpenerAndCloser(); // Closes opened projects

    ProjectInfo open(const QString &projectFile, bool configureAsExampleProject = false);

private:
    QList<ProjectExplorer::Project *> m_openProjects;
};

class CPPTOOLS_EXPORT TemporaryDir
{
    Q_DISABLE_COPY(TemporaryDir)

public:
    TemporaryDir();

    bool isValid() const { return m_isValid; }
    QString path() const { return m_temporaryDir.path(); }

    QString createFile(const QByteArray &relativePath, const QByteArray &contents);

protected:
    QTemporaryDir m_temporaryDir;
    bool m_isValid;
};

class CPPTOOLS_EXPORT TemporaryCopiedDir : public TemporaryDir
{
public:
    TemporaryCopiedDir(const QString &sourceDirPath);
    QString absolutePath(const QByteArray &relativePath) const;

private:
    TemporaryCopiedDir();
};

class CPPTOOLS_EXPORT VerifyCleanCppModelManager
{
public:
    VerifyCleanCppModelManager();
    ~VerifyCleanCppModelManager();
    static bool isClean(bool testCleanedProjects = true);
};

class FileWriterAndRemover
{
public:
    FileWriterAndRemover(const QString &filePath, const QByteArray &contents); // Writes file
    bool writtenSuccessfully() const { return m_writtenSuccessfully; }
    ~FileWriterAndRemover(); // Removes file

private:
    const QString m_filePath;
    bool m_writtenSuccessfully;
};

// Normally the proposal is deleted by the ProcessorRunner or the
// GenericProposalWidget, but in tests we usually don't make use of them.
class CPPTOOLS_EXPORT IAssistProposalScopedPointer
{
public:
    IAssistProposalScopedPointer(TextEditor::IAssistProposal *proposal);
    ~IAssistProposalScopedPointer();

    QScopedPointer<TextEditor::IAssistProposal> d;
};

} // namespace Tests
} // namespace CppTools

#endif // CPPTOOLSTESTCASE_H
