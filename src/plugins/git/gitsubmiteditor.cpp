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

enum { FileTypeRole = Qt::UserRole + 1 };
enum FileType { StagedFile , UnstagedFile, UntrackedFile };

/* The problem with git is that no diff can be obtained to for a random
 * multiselection of staged/unstaged files; it requires the --cached
 * option for staged files. So, we set the file list to
 * single selection and sort the files manual according to a type
 * flag we add to the model. */

GitSubmitEditor::GitSubmitEditor(const VCSBase::VCSBaseSubmitEditorParameters *parameters, QWidget *parent) :
    VCSBaseSubmitEditor(parameters, new GitSubmitEditorWidget(parent)),
    m_model(0)
{
    setDisplayName(tr("Git Commit"));
    setFileListSelectionMode(QAbstractItemView::SingleSelection);
    connect(this, SIGNAL(diffSelectedFiles(QStringList)), this, SLOT(slotDiffSelected(QStringList)));
}

GitSubmitEditorWidget *GitSubmitEditor::submitEditorWidget()
{
    return static_cast<GitSubmitEditorWidget *>(widget());
}

// Utility to add a list of state/file pairs to the model
// setting a file type.
static void addStateFileListToModel(const QList<CommitData::StateFilePair> &l,                               
                                    bool checked, FileType ft,
                                    VCSBase::SubmitFileModel *model)
{

    typedef QList<CommitData::StateFilePair>::const_iterator ConstIterator;
    if (!l.empty()) {
        const ConstIterator cend = l.constEnd();
        const QVariant fileTypeData(ft);
        for (ConstIterator it = l.constBegin(); it != cend; ++it)
            model->addFile(it->second, it->first, checked).front()->setData(fileTypeData, FileTypeRole);
    }
}

void GitSubmitEditor::setCommitData(const CommitData &d)
{
    submitEditorWidget()->setPanelData(d.panelData);
    submitEditorWidget()->setPanelInfo(d.panelInfo);

    m_model = new VCSBase::SubmitFileModel(this);
    addStateFileListToModel(d.stagedFiles,   true,  StagedFile,   m_model);
    addStateFileListToModel(d.unstagedFiles, false, UnstagedFile, m_model);
    if (!d.untrackedFiles.empty()) {
        const QString untrackedSpec = QLatin1String("untracked");
        const QVariant fileTypeData(UntrackedFile);
        const QStringList::const_iterator cend = d.untrackedFiles.constEnd();
        for (QStringList::const_iterator it = d.untrackedFiles.constBegin(); it != cend; ++it)
            m_model->addFile(*it, untrackedSpec, false).front()->setData(fileTypeData, FileTypeRole);
    }
    setFileModel(m_model);
}

void GitSubmitEditor::slotDiffSelected(const QStringList &files)
{
    QList<QStandardItem *> fileRow = m_model->findRow(files.front(), fileNameColumn());
    if (fileRow.empty())
        return;
    const FileType ft = static_cast<FileType>(fileRow.front()->data(FileTypeRole).toInt());
    switch (ft) {
        case StagedFile:
            emit diffStaged(files);
            break;
        case UnstagedFile:
            emit diffUnstaged(files);
            break;
        case UntrackedFile:
            break;
    }


}

GitSubmitEditorPanelData GitSubmitEditor::panelData() const
{
    return const_cast<GitSubmitEditor*>(this)->submitEditorWidget()->panelData();
}

} // namespace Internal
} // namespace Git
