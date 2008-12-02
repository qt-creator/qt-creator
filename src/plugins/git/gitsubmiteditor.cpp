/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "gitsubmiteditor.h"
#include "gitsubmiteditorwidget.h"
#include "gitconstants.h"
#include "commitdata.h"

#include <QtCore/QDebug>

namespace Git {
namespace Internal {

GitSubmitEditor::GitSubmitEditor(const VCSBase::VCSBaseSubmitEditorParameters *parameters, QWidget *parent) :
    VCSBaseSubmitEditor(parameters, new GitSubmitEditorWidget(parent))
{
    setDisplayName(tr("Git Commit"));
}

GitSubmitEditorWidget *GitSubmitEditor::submitEditorWidget()
{
    return static_cast<GitSubmitEditorWidget *>(widget());
}

QStringList GitSubmitEditor::vcsFileListToFileList(const QStringList &rawList) const
{
    QStringList rc;
    foreach (const QString &rf, rawList)
        rc.push_back(fileFromChangeLine(rf));
    return rc;
}

void GitSubmitEditor::setCommitData(const CommitData &d)
{
    submitEditorWidget()->setPanelData(d.panelData);
    submitEditorWidget()->setPanelInfo(d.panelInfo);

    // Commited: Checked, user cannot uncheck
    addFiles(d.commitFiles, true, false);
    // Not Updated: User can check
    addFiles(d.notUpdatedFiles, false, true);
    addFiles(d.untrackedFiles, false, true);
}

GitSubmitEditorPanelData GitSubmitEditor::panelData() const
{
    return const_cast<GitSubmitEditor*>(this)->submitEditorWidget()->panelData();
}

QString GitSubmitEditor::fileFromChangeLine(const QString &line)
{
    QString rc = line;
    // "modified: mainwindow.cpp"
    const int index = rc.indexOf(QLatin1Char(':'));
    if (index != -1)
        rc.remove(0, index + 1);
    const QChar blank(' ');
    while (rc.startsWith(blank))
        rc.remove(0, 1);
    return rc;
}

} // namespace Internal
} // namespace Git
