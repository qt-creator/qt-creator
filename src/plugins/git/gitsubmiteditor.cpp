/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gitsubmiteditor.h"
#include "gitsubmiteditorwidget.h"
#include "gitconstants.h"
#include "commitdata.h"

#include <utils/qtcassert.h>
#include <vcsbase/submitfilemodel.h>

#include <QDebug>
#include <QStringList>
#include <QTextCodec>

namespace Git {
namespace Internal {

/* The problem with git is that no diff can be obtained to for a random
 * multiselection of staged/unstaged files; it requires the --cached
 * option for staged files. So, we sort apart the diff file lists
 * according to a type flag we add to the model. */

GitSubmitEditor::GitSubmitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters, QWidget *parent) :
    VcsBaseSubmitEditor(parameters, new GitSubmitEditorWidget(parent)),
    m_model(0)
{
    connect(this, SIGNAL(diffSelectedFiles(QStringList)), this, SLOT(slotDiffSelected(QStringList)));
}

GitSubmitEditorWidget *GitSubmitEditor::submitEditorWidget()
{
    return static_cast<GitSubmitEditorWidget *>(widget());
}

void GitSubmitEditor::setCommitData(const CommitData &d)
{
    GitSubmitEditorWidget *w = submitEditorWidget();
    w->setPanelData(d.panelData);
    w->setPanelInfo(d.panelInfo);
    w->setHasUnmerged(false);

    m_commitEncoding = d.commitEncoding;

    m_model = new VcsBase::SubmitFileModel(this);
    if (!d.files.isEmpty()) {
        for (QList<CommitData::StateFilePair>::const_iterator it = d.files.constBegin();
             it != d.files.constEnd(); ++it) {
            const FileStates state = it->first;
            const QString file = it->second;
            VcsBase::CheckMode checkMode;
            if (state & UnmergedFile) {
                checkMode = VcsBase::Uncheckable;
                w->setHasUnmerged(true);
            } else if (state & StagedFile) {
                checkMode = VcsBase::Checked;
            } else {
                checkMode = VcsBase::Unchecked;
            }
            m_model->addFile(file, CommitData::stateDisplayName(state), checkMode,
                             QVariant(static_cast<int>(state)));
        }
    }
    setFileModel(m_model, d.panelInfo.repository);
}

void GitSubmitEditor::slotDiffSelected(const QStringList &files)
{
    // Sort it apart into unmerged/staged/unstaged files
    QStringList unmergedFiles;
    QStringList unstagedFiles;
    QStringList stagedFiles;
    const int fileColumn = fileNameColumn();
    const int rowCount = m_model->rowCount();
    for (int r = 0; r < rowCount; r++) {
        const QString fileName = m_model->item(r, fileColumn)->text();
        if (files.contains(fileName)) {
            const FileStates state = static_cast<FileStates>(m_model->extraData(r).toInt());
            if (state & UnmergedFile)
                unmergedFiles.push_back(fileName);
            else if (state & StagedFile)
                stagedFiles.push_back(fileName);
            else if (state != UntrackedFile)
                unstagedFiles.push_back(fileName);
        }
    }
    if (!unstagedFiles.empty() || !stagedFiles.empty())
        emit diff(unstagedFiles, stagedFiles);
    if (!unmergedFiles.empty())
        emit merge(unmergedFiles);
}

GitSubmitEditorPanelData GitSubmitEditor::panelData() const
{
    return const_cast<GitSubmitEditor*>(this)->submitEditorWidget()->panelData();
}

QByteArray GitSubmitEditor::fileContents() const
{
    const QString& text = const_cast<GitSubmitEditor*>(this)->submitEditorWidget()->descriptionText();

    if (!m_commitEncoding.isEmpty()) {
        // Do the encoding convert, When use user-defined encoding
        // e.g. git config --global i18n.commitencoding utf-8
        QTextCodec *codec = QTextCodec::codecForName(m_commitEncoding.toLocal8Bit());
        if (codec)
            return codec->fromUnicode(text);
    }

    // Using utf-8 as the default encoding
    return text.toUtf8();
}

} // namespace Internal
} // namespace Git
