// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gitsettings.h" // CommitType

#include <utils/filepath.h>

#include <QStringList>
#include <QPair>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

namespace Git::Internal {

// Read-only
class GitSubmitEditorPanelInfo
{
public:
    void clear();
    Utils::FilePath repository;
    QString branch;
};

enum PushAction {
    NoPush,
    NormalPush,
    PushToGerrit
};

class GitSubmitEditorPanelData
{
public:
    void clear();
    // Format as "John Doe <jdoe@foobar.com>"
    QString authorString() const;

    QString author;
    QString email;
    bool bypassHooks;
    PushAction pushAction;
    bool signOff;
};

enum FileState {
    EmptyFileState = 0x00,

    StagedFile   = 0x01,
    ModifiedFile = 0x02,
    AddedFile    = 0x04,
    DeletedFile  = 0x08,
    RenamedFile  = 0x10,
    CopiedFile   = 0x20,
    UnmergedFile = 0x40,
    TypeChangedFile = 0x80,

    UnmergedUs   = 0x100,
    UnmergedThem = 0x200,

    UntrackedFile = 0x400,
    UnknownFileState = 0x800
};
Q_DECLARE_FLAGS(FileStates, FileState)

class CommitData
{
public:
    CommitData(CommitType type = SimpleCommit);
    // A pair of state string/file name ('modified', 'file.cpp').
    using StateFilePair = QPair<FileStates, QString>;

    void clear();
    // Parse the files and the branch of panelInfo
    // from a git status output
    bool parseFilesFromStatus(const QString &output);

    // Convenience to retrieve the file names from
    // the specification list. Optionally filter for a certain state
    QStringList filterFiles(const FileStates &state) const;

    static QString stateDisplayName(const FileStates &state);

    CommitType commitType;
    QString amendSHA1;
    QTextCodec *commitEncoding = nullptr;
    GitSubmitEditorPanelInfo panelInfo;
    GitSubmitEditorPanelData panelData;
    bool enablePush = false;
    QChar commentChar;

    QList<StateFilePair> files;

private:
    bool checkLine(const QString &stateInfo, const QString &file);
};

} // Git::Internal

Q_DECLARE_OPERATORS_FOR_FLAGS(Git::Internal::FileStates)

namespace Git::Internal {

// Must appear after Q_DECLARE_OPERATORS_FOR_FLAGS
bool operator<(const CommitData::StateFilePair &a,
               const CommitData::StateFilePair &b);

} // Git::Internal
