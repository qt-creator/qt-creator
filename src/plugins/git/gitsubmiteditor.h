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

#pragma once

#include "commitdata.h"

#include <vcsbase/vcsbasesubmiteditor.h>

#include <QFutureWatcher>
#include <QStringList>

namespace VcsBase { class SubmitFileModel; }

namespace Git {
namespace Internal {

class GitClient;
class GitSubmitEditorWidget;
class GitSubmitEditorPanelData;

class CommitDataFetchResult
{
public:
    static CommitDataFetchResult fetch(CommitType commitType, const QString &workingDirectory);

    QString errorMessage;
    CommitData commitData;
    bool success;
};

class GitSubmitEditor : public VcsBase::VcsBaseSubmitEditor
{
    Q_OBJECT

public:
    explicit GitSubmitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters);
    ~GitSubmitEditor() override;

    void setCommitData(const CommitData &);
    GitSubmitEditorPanelData panelData() const;
    CommitType commitType() const { return m_commitType; }
    QString amendSHA1() const;

protected:
    QByteArray fileContents() const override;
    void updateFileModel() override;
    void forceUpdateFileModel();

private:
    void slotDiffSelected(const QList<int> &rows);
    void showCommit(const QString &commit);
    void commitDataRetrieved();

    inline GitSubmitEditorWidget *submitEditorWidget();
    inline const GitSubmitEditorWidget *submitEditorWidget() const;

    VcsBase::SubmitFileModel *m_model = nullptr;
    QTextCodec *m_commitEncoding = nullptr;
    CommitType m_commitType = SimpleCommit;
    QString m_amendSHA1;
    QString m_workingDirectory;
    bool m_firstUpdate = true;
    QFutureWatcher<CommitDataFetchResult> m_fetchWatcher;
};

} // namespace Internal
} // namespace Git
