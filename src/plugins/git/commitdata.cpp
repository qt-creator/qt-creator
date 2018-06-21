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

#include "commitdata.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>

namespace Git {
namespace Internal {

void GitSubmitEditorPanelInfo::clear()
{
    repository.clear();
    branch.clear();
}

void GitSubmitEditorPanelData::clear()
{
    author.clear();
    email.clear();
    bypassHooks = false;
    pushAction = NoPush;
    signOff = false;
}

QString GitSubmitEditorPanelData::authorString() const
{
    QString rc;
    rc += author;

    if (email.isEmpty())
        return rc;

    rc += " <";
    rc += email;
    rc += '>';
    return rc;
}

CommitData::CommitData(CommitType type)
    : commitType(type)
    , commitEncoding(0)
    , enablePush(false)
{
}

void CommitData::clear()
{
    panelInfo.clear();
    panelData.clear();
    amendSHA1.clear();
    enablePush = false;

    files.clear();
}

static FileStates stateFor(const QChar &c)
{
    switch (c.unicode()) {
    case ' ':
        return EmptyFileState;
    case 'M':
        return ModifiedFile;
    case 'A':
        return AddedFile;
    case 'D':
        return DeletedFile;
    case 'R':
        return RenamedFile;
    case 'C':
        return CopiedFile;
    case 'U':
        return UnmergedFile;
    case 'T':
        return TypeChangedFile;
    case '?':
        return UntrackedFile;
    default:
        return UnknownFileState;
    }
}

bool operator<(const CommitData::StateFilePair &a, const CommitData::StateFilePair &b)
{
    if ((a.first & UnmergedFile) && !(b.first & UnmergedFile))
        return false;
    if ((b.first & UnmergedFile) && !(a.first & UnmergedFile))
        return true;
    return a.second < b.second;
}

bool CommitData::checkLine(const QString &stateInfo, const QString &file)
{
    QTC_ASSERT(stateInfo.count() == 2, return false);

    if (stateInfo == "??") {
        files.append(qMakePair(FileStates(UntrackedFile), file));
        return true;
    }

    FileStates xState = stateFor(stateInfo.at(0));
    FileStates yState = stateFor(stateInfo.at(1));
    if (xState == UnknownFileState || yState == UnknownFileState)
        return false;

    bool isMerge = (xState == UnmergedFile || yState == UnmergedFile
                    || ((xState == yState) && (xState == AddedFile || xState == DeletedFile)));
    if (isMerge) {
        if (xState == yState) {
            if (xState == UnmergedFile)
                xState = ModifiedFile;
            files.append(qMakePair(xState | UnmergedFile | UnmergedUs | UnmergedThem, file));
        } else if (xState == UnmergedFile) {
            files.append(qMakePair(yState | UnmergedFile | UnmergedThem, file));
        } else {
            files.append(qMakePair(xState | UnmergedFile | UnmergedUs, file));
        }
    } else {
        if (xState != EmptyFileState)
            files.append(qMakePair(xState | StagedFile, file));

        if (yState != EmptyFileState) {
            QString newFile = file;
            if (xState & (RenamedFile | CopiedFile))
                newFile = file.mid(file.indexOf(" -> ") + 4);

            files.append(qMakePair(yState, newFile));
        }
    }
    Utils::sort(files);
    return true;
}

/* Parse a git status file list:
 * \code
    ## branch_name
    XY file
    \endcode */
bool CommitData::parseFilesFromStatus(const QString &output)
{
    const QStringList lines = output.split('\n');

    for (const QString &line : lines) {
        if (line.isEmpty())
            continue;

        if (line.startsWith("## ")) {
            // Branch indication:
            panelInfo.branch = line.mid(3);
            continue;
        }
        QTC_ASSERT(line.at(2) == ' ', continue);
        QString file = line.mid(3);
        if (file.startsWith('"'))
            file.remove(0, 1).chop(1);
        if (!checkLine(line.mid(0, 2), file))
            return false;
    }

    return true;
}

QStringList CommitData::filterFiles(const FileStates &state) const
{
    QStringList result;
    for (const StateFilePair &p : files) {
        if (state == (p.first & ~(UnmergedFile | UnmergedUs | UnmergedThem)))
            result.append(p.second);
    }
    return result;
}

QString CommitData::stateDisplayName(const FileStates &state)
{
    QString resultState;
    if (state == UntrackedFile)
        return tr("untracked");

    if (state & StagedFile)
        resultState = tr("staged + ");
    if (state & ModifiedFile)
        resultState.append(tr("modified"));
    else if (state & AddedFile)
        resultState.append(tr("added"));
    else if (state & DeletedFile)
        resultState.append(tr("deleted"));
    else if (state & RenamedFile)
        resultState.append(tr("renamed"));
    else if (state & CopiedFile)
        resultState.append(tr("copied"));
    else if (state & TypeChangedFile)
        resultState.append(tr("typechange"));
    if (state & UnmergedUs) {
        if (state & UnmergedThem)
            resultState.append(tr(" by both"));
        else
            resultState.append(tr(" by us"));
    } else if (state & UnmergedThem) {
        resultState.append(tr(" by them"));
    }
    return resultState;
}

} // namespace Internal
} // namespace Git
