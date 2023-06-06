// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commitdata.h"

#include "gittr.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace Git::Internal {

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
    QTC_ASSERT(stateInfo.size() == 2, return false);

    if (stateInfo == "??") {
        files.push_back({FileStates(UntrackedFile), file});
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
            files.push_back({xState | UnmergedFile | UnmergedUs | UnmergedThem, file});
        } else if (xState == UnmergedFile) {
            files.push_back({yState | UnmergedFile | UnmergedThem, file});
        } else {
            files.push_back({xState | UnmergedFile | UnmergedUs, file});
        }
    } else {
        if (xState != EmptyFileState)
            files.push_back({xState | StagedFile, file});

        if (yState != EmptyFileState) {
            QString newFile = file;
            if (xState & (RenamedFile | CopiedFile))
                newFile = file.mid(file.indexOf(" -> ") + 4);

            files.push_back({yState, newFile});
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
        return Tr::tr("untracked");

    if (state & StagedFile)
        resultState = Tr::tr("staged + ");
    if (state & ModifiedFile)
        resultState.append(Tr::tr("modified"));
    else if (state & AddedFile)
        resultState.append(Tr::tr("added"));
    else if (state & DeletedFile)
        resultState.append(Tr::tr("deleted"));
    else if (state & RenamedFile)
        resultState.append(Tr::tr("renamed"));
    else if (state & CopiedFile)
        resultState.append(Tr::tr("copied"));
    else if (state & TypeChangedFile)
        resultState.append(Tr::tr("typechange"));
    if (state & UnmergedUs) {
        if (state & UnmergedThem)
            resultState.append(Tr::tr(" by both"));
        else
            resultState.append(Tr::tr(" by us"));
    } else if (state & UnmergedThem) {
        resultState.append(Tr::tr(" by them"));
    }
    return resultState;
}

} // Git::Internal
