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

#ifdef WITH_TESTS
namespace ExtensionSystem { class IPlugin; }
#endif

namespace Git::Internal {

class BlameController;

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
    Utils::FilePath topLevel; ///< repository top level path
    Utils::FilePath filePath; ///< absolute file path for current file
    QString originalFileName; ///< relative file path from project root for the original file
    QString previousFileName; ///< relative file path in the parent commit, if it differs (renames)
    int line = -1;            ///< current line number in current file
    int originalLine = -1;    ///< original line number in the original file
    bool modified = false;    ///< line is locally modified (uncommitted)
};

class BlameMark : public TextEditor::TextMark
{
public:
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
    BlameController *m_controller = nullptr;
};

class InstantBlame : public QObject
{
    Q_OBJECT

public:
    InstantBlame();

    void setup();
    void repeat();
    void once();

private:
    void setupForCurrentEditor();
    void scheduleInstantBlame();
    void stop();
    void slotDocumentChanged();

    BlameController *m_controller = nullptr;
    QPointer<TextEditor::TextDocument> m_document;
    bool m_modified = false;
    QMetaObject::Connection m_blameCursorPosConn;
    QMetaObject::Connection m_documentChangedConn;
};

#ifdef WITH_TESTS
void registerInstantBlameTests(ExtensionSystem::IPlugin *plugin);
#endif

} // Git::Internal
