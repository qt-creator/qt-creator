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

#ifndef COMMITDATA_H
#define COMMITDATA_H

#include "gitsettings.h" // CommitType

#include <QStringList>
#include <QPair>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

// Read-only
struct GitSubmitEditorPanelInfo
{
    void clear();
    QString repository;
    QString branch;
};

QDebug operator<<(QDebug d, const GitSubmitEditorPanelInfo &);

enum PushAction {
    NoPush,
    NormalPush,
    PushToGerrit
};

struct GitSubmitEditorPanelData
{
    void clear();
    // Format as "John Doe <jdoe@foobar.com>"
    QString authorString() const;

    QString author;
    QString email;
    bool bypassHooks;
    PushAction pushAction;
};

QDebug operator<<(QDebug d, const GitSubmitEditorPanelData &);

enum FileState {
    EmptyFileState = 0x00,

    StagedFile   = 0x01,
    ModifiedFile = 0x02,
    AddedFile    = 0x04,
    DeletedFile  = 0x08,
    RenamedFile  = 0x10,
    CopiedFile   = 0x20,
    UnmergedFile = 0x40,

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

#endif // COMMITDATA_H
