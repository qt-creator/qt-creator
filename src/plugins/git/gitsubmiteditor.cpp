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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "gitsubmiteditor.h"
#include "gitsubmiteditorwidget.h"
#include "gitconstants.h"
#include "commitdata.h"

#include <vcsbase/submitfilemodel.h>

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

static void addStateFileListToModel(const QList<CommitData::StateFilePair> &l,
                                    VCSBase::SubmitFileModel *model,
                                    bool checked)
{
    typedef QList<CommitData::StateFilePair>::const_iterator ConstIterator;
    const ConstIterator cend = l.constEnd();
    for (ConstIterator it = l.constBegin(); it != cend; ++it)
        model->addFile(it->second, it->first, checked);
}

void GitSubmitEditor::setCommitData(const CommitData &d)
{
    submitEditorWidget()->setPanelData(d.panelData);
    submitEditorWidget()->setPanelInfo(d.panelInfo);

    VCSBase::SubmitFileModel *model = new VCSBase::SubmitFileModel(this);
    addStateFileListToModel(d.stagedFiles, model, true);
    addStateFileListToModel(d.unstagedFiles, model, false);
    if (!d.untrackedFiles.empty()) {
        const QString untrackedSpec = QLatin1String("untracked");
        const QStringList::const_iterator cend = d.untrackedFiles.constEnd();
        for (QStringList::const_iterator it = d.untrackedFiles.constBegin(); it != cend; ++it)
            model->addFile(*it, untrackedSpec, false);
    }
    setFileModel(model);
}

GitSubmitEditorPanelData GitSubmitEditor::panelData() const
{
    return const_cast<GitSubmitEditor*>(this)->submitEditorWidget()->panelData();
}

} // namespace Internal
} // namespace Git
