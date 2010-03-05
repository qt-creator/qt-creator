/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "gitsubmiteditor.h"
#include "gitsubmiteditorwidget.h"
#include "gitconstants.h"
#include "commitdata.h"

#include <vcsbase/submitfilemodel.h>

#include <QtCore/QDebug>
#include <QtCore/QStringList>

namespace Git {
namespace Internal {

enum { FileTypeRole = Qt::UserRole + 1 };
enum FileType { StagedFile , UnstagedFile, UntrackedFile };

/* The problem with git is that no diff can be obtained to for a random
 * multiselection of staged/unstaged files; it requires the --cached
 * option for staged files. So, we sort apart the diff file lists
 * according to a type flag we add to the model. */

GitSubmitEditor::GitSubmitEditor(const VCSBase::VCSBaseSubmitEditorParameters *parameters, QWidget *parent) :
    VCSBaseSubmitEditor(parameters, new GitSubmitEditorWidget(parent)),
    m_model(0)
{
    setDisplayName(tr("Git Commit"));
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
    // Sort it apart into staged/unstaged files
    QStringList unstagedFiles;
    QStringList stagedFiles;
    const int fileColumn = fileNameColumn();
    const int rowCount = m_model->rowCount();
    for (int r = 0; r < rowCount; r++) {
        const QString fileName = m_model->item(r, fileColumn)->text();
        if (files.contains(fileName)) {
            const FileType ft = static_cast<FileType>(m_model->item(r, 0)->data(FileTypeRole).toInt());
            switch (ft) {
            case StagedFile:
                stagedFiles.push_back(fileName);
                break;
            case UnstagedFile:
                unstagedFiles.push_back(fileName);
                break;
            case UntrackedFile:
                break;
            }
        }
    }
    if (!unstagedFiles.empty() || !stagedFiles.empty())
        emit diff(unstagedFiles, stagedFiles);
}

QString GitSubmitEditor::fileContents() const
{
    // We need to manually purge out comment lines starting with
    // hash '#' since git does not do that when using -F.
    const QChar newLine = QLatin1Char('\n');
    const QChar hash = QLatin1Char('#');
    QString message = VCSBase::VCSBaseSubmitEditor::fileContents();
    for (int pos = 0; pos < message.size(); ) {
        const int newLinePos = message.indexOf(newLine, pos);
        const int startOfNextLine = newLinePos == -1 ? message.size() : newLinePos + 1;
        if (message.at(pos) == hash) {
            message.remove(pos, startOfNextLine - pos);
        } else {
            pos = startOfNextLine;
        }
    }
    return message;
}

GitSubmitEditorPanelData GitSubmitEditor::panelData() const
{
    return const_cast<GitSubmitEditor*>(this)->submitEditorWidget()->panelData();
}

} // namespace Internal
} // namespace Git
