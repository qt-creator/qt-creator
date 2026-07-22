// Copyright (C) 2023 Andre Hartmann (aha_1980@gmx.de)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gitclient.h"

#include <texteditor/textmark.h>

#include <utils/filepath.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

QT_BEGIN_NAMESPACE
class QLayout;
class QTimer;
QT_END_NAMESPACE

namespace Git::Internal {

class CommitInfo {
public:
    QString hash;
    QString shortAuthor;
    QString author;
    QString authorMail;
    QDateTime authorDate;
    QString subject;
    QStringList oldLines;     ///< the previous line contents
    QString newLine;          ///< the new line contents
    Utils::FilePath filePath; ///< absolute file path for current file
    QString originalFileName; ///< relative file path from project root for the original file
    int line = -1;            ///< current line number in current file
    int originalLine = -1;    ///< original line number in the original file
    bool modified = false;    ///< line is locally modified (uncommitted)
    QString viewRevision;     ///< revision shown in the blamed view, empty for the working tree
    QString viewFileName;     ///< relative file path at viewRevision
};

class BlameMark : public TextEditor::TextMark
{
public:
    BlameMark(const Utils::FilePath &fileName, int lineNumber, const CommitInfo &info);
    // for documents without a file path, e.g. the baseline view of the
    // inline diff editor
    BlameMark(TextEditor::TextDocument *document, int lineNumber, const CommitInfo &info);
    bool addToolTipContent(QLayout *target) const final;
    QString toolTipText(const CommitInfo &info) const;
    void addOldLine(const QString &oldLine);
    void addNewLine(const QString &newLine);

private:
    void initialize();

    CommitInfo m_info;
};

// Shows blame annotations at a fixed revision for the line under the cursor
// of an editor widget whose document contains that revision of the file, for
// example the baseline view of the inline diff editor.
class BaselineBlame : public QObject
{
    Q_OBJECT

public:
    BaselineBlame(TextEditor::TextEditorWidget *widget,
                  const Utils::FilePath &topLevel,
                  const QString &ref,
                  const QString &relativeFile,
                  const Utils::FilePath &workingFilePath);

private:
    void perform();

    TextEditor::TextEditorWidget *m_widget = nullptr;
    const Utils::FilePath m_topLevel;
    const QString m_ref;
    const QString m_relativeFile;
    const Utils::FilePath m_workingFilePath;
    Utils::TextEncoding m_encoding;
    int m_lastLine = -1;
    QTimer *m_cursorTimer = nullptr;
    QtTaskTree::QSingleTaskTreeRunner m_taskTreeRunner;
    std::unique_ptr<BlameMark> m_blameMark;
};

class InstantBlame : public QObject
{
    Q_OBJECT

public:
    InstantBlame();

    void setup();
    void once();

private:
    void scheduleInstantBlame();
    void stop();
    void perform();
    void refreshWorkingDirectory();
    void slotDocumentChanged();

    Utils::FilePath m_workingDirectory;
    Utils::TextEncoding m_encoding;
    Author m_author;
    int m_lastVisitedEditorLine = -1;
    Core::IDocument *m_document = nullptr;
    bool m_modified = false;
    QTimer *m_cursorPositionChangedTimer = nullptr;
    QTimer *m_scheduleTimer = nullptr;
    QtTaskTree::QSingleTaskTreeRunner m_taskTreeRunner;
    std::unique_ptr<BlameMark> m_blameMark;
    QMetaObject::Connection m_blameCursorPosConn;
    QMetaObject::Connection m_documentChangedConn;
};

} // Git::Internal
