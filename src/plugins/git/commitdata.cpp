/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "commitdata.h"
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QRegExp>

namespace Git {
namespace Internal {

void GitSubmitEditorPanelInfo::clear()
{
    repository.clear();
    description.clear();
    branch.clear();
}

QDebug operator<<(QDebug d, const GitSubmitEditorPanelInfo &data)
{
    d.nospace() << "Rep: " << data.repository << " Descr: " << data.description
        << " branch: " << data.branch;
    return d;
}

void GitSubmitEditorPanelData::clear()
{
    author.clear();
    email.clear();
    bypassHooks = false;
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
                << " bypass hooks: " << data.bypassHooks;
    return d;
}

void CommitData::clear()
{
    panelInfo.clear();
    panelData.clear();
    amendSHA1.clear();

    files.clear();
}

static CommitData::FileState stateFor(const QChar &c)
{
    switch (c.unicode()) {
    case ' ':
        return CommitData::UntrackedFile;
    case 'M':
        return CommitData::ModifiedFile;
    case 'A':
        return CommitData::AddedFile;
    case 'D':
        return CommitData::DeletedFile;
    case 'R':
        return CommitData::RenamedFile;
    case 'C':
        return CommitData::CopiedFile;
    case 'U':
        return CommitData::UpdatedFile;
    default:
        return CommitData::UnknownFileState;
    }
}

static bool checkLine(const QString &stateInfo, const QString &file, QList<CommitData::StateFilePair> *files)
{
    QTC_ASSERT(stateInfo.count() == 2, return false);
    QTC_ASSERT(files, return false);

    if (stateInfo == QLatin1String("??")) {
        files->append(qMakePair(CommitData::UntrackedFile, file));
        return true;
    }

    CommitData::FileState stagedState = stateFor(stateInfo.at(0));
    if (stagedState == CommitData::UnknownFileState)
        return false;

    stagedState = static_cast<CommitData::FileState>(stagedState | CommitData::StagedFile);
    if (stagedState != CommitData::StagedFile)
        files->append(qMakePair(stagedState, file));

    CommitData::FileState state = stateFor(stateInfo.at(1));
    if (state == CommitData::UnknownFileState)
        return false;

    if (state != CommitData::UntrackedFile) {
        QString newFile = file;
        if (stagedState == CommitData::RenamedStagedFile || stagedState == CommitData::CopiedStagedFile)
            newFile = file.mid(file.indexOf(QLatin1String(" -> ")) + 4);

        files->append(qMakePair(state, newFile));
    }

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
        if (!checkLine(line.mid(0, 2), file, &files))
            return false;
    }

    return true;
}

QStringList CommitData::filterFiles(const CommitData::FileState &state) const
{
    QStringList result;
    foreach (const StateFilePair &p, files) {
        if (state == AllStates || state == p.first)
            result.append(p.second);
    }
    return result;
}

QString CommitData::stateDisplayName(const FileState &state)
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
    else if (state & UpdatedFile)
        resultState.append(QCoreApplication::translate("Git::Internal::CommitData", "updated"));
    return resultState;
}

} // namespace Internal
} // namespace Git
