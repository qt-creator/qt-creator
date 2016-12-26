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

#pragma once

#include "gitsettings.h" // CommitType

#include <QCoreApplication>
#include <QStringList>
#include <QPair>

namespace Git {
namespace Internal {

// Read-only
class GitSubmitEditorPanelInfo
{
public:
    void clear();
    QString repository;
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
    Q_DECLARE_TR_FUNCTIONS(Git::Internal::CommitData)

public:
    CommitData(CommitType type = SimpleCommit);
    // A pair of state string/file name ('modified', 'file.cpp').
    typedef QPair<FileStates, QString> StateFilePair;

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
    QTextCodec *commitEncoding;
    GitSubmitEditorPanelInfo panelInfo;
    GitSubmitEditorPanelData panelData;
    bool enablePush;

    QList<StateFilePair> files;

private:
    bool checkLine(const QString &stateInfo, const QString &file);
};

} // namespace Internal
} // namespace Git

Q_DECLARE_OPERATORS_FOR_FLAGS(Git::Internal::FileStates)

namespace Git {
namespace Internal {

// Must appear after Q_DECLARE_OPERATORS_FOR_FLAGS
bool operator<(const CommitData::StateFilePair &a,
               const CommitData::StateFilePair &b);

} // namespace Internal
} // namespace Git
