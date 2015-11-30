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

#include "commitdata.h"
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>

namespace Git {
namespace Internal {

void GitSubmitEditorPanelInfo::clear()
{
    repository.clear();
    branch.clear();
}

QDebug operator<<(QDebug d, const GitSubmitEditorPanelInfo &data)
{
    d.nospace() << "Rep: " << data.repository << " branch: " << data.branch;
    return d;
}

void GitSubmitEditorPanelData::clear()
{
    author.clear();
    email.clear();
    bypassHooks = false;
    pushAction = NoPush;
}

QString GitSubmitEditorPanelData::authorString() const
{
    QString rc;
    rc += author;

    if (email.isEmpty())
        return rc;

    rc += QLatin1String(" <");
    rc += email;
    rc += QLatin1Char('>');
    return rc;
}

QDebug operator<<(QDebug d, const GitSubmitEditorPanelData &data)
{
    d.nospace() << " author:" << data.author << " email: " << data.email
                << " bypass hooks: " << data.bypassHooks
                << " action after commit " << data.pushAction;
    return d;
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

    if (stateInfo == QLatin1String("??")) {
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
                newFile = file.mid(file.indexOf(QLatin1String(" -> ")) + 4);

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
    const QStringList lines = output.split(QLatin1Char('\n'));

    foreach (const QString &line, lines) {
        if (line.isEmpty())
            continue;

        if (line.startsWith(QLatin1String("## "))) {
            // Branch indication:
            panelInfo.branch = line.mid(3);
            continue;
        }
        QTC_ASSERT(line.at(2) == QLatin1Char(' '), continue);
        QString file = line.mid(3);
        if (file.startsWith(QLatin1Char('"')))
            file.remove(0, 1).chop(1);
        if (!checkLine(line.mid(0, 2), file))
            return false;
    }

    return true;
}

QStringList CommitData::filterFiles(const FileStates &state) const
{
    QStringList result;
    foreach (const StateFilePair &p, files) {
        if (state == (p.first & ~(UnmergedFile | UnmergedUs | UnmergedThem)))
            result.append(p.second);
    }
    return result;
}

QString CommitData::stateDisplayName(const FileStates &state)
{
    QString resultState;
    if (state == UntrackedFile)
        return QCoreApplication::translate("Git::Internal::CommitData", "untracked");

    if (state & StagedFile)
        resultState = QCoreApplication::translate("Git::Internal::CommitData", "staged + ");
    if (state & ModifiedFile)
        resultState.append(QCoreApplication::translate("Git::Internal::CommitData", "modified"));
    else if (state & AddedFile)
        resultState.append(QCoreApplication::translate("Git::Internal::CommitData", "added"));
    else if (state & DeletedFile)
        resultState.append(QCoreApplication::translate("Git::Internal::CommitData", "deleted"));
    else if (state & RenamedFile)
        resultState.append(QCoreApplication::translate("Git::Internal::CommitData", "renamed"));
    else if (state & CopiedFile)
        resultState.append(QCoreApplication::translate("Git::Internal::CommitData", "copied"));
    if (state & UnmergedUs) {
        if (state & UnmergedThem)
            resultState.append(QCoreApplication::translate("Git::Internal::CommitData", " by both"));
        else
            resultState.append(QCoreApplication::translate("Git::Internal::CommitData", " by us"));
    } else if (state & UnmergedThem) {
        resultState.append(QCoreApplication::translate("Git::Internal::CommitData", " by them"));
    }
    return resultState;
}

} // namespace Internal
} // namespace Git
