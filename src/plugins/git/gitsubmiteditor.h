// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "commitdata.h"

#include <utils/filepath.h>

#include <vcsbase/vcsbasesubmiteditor.h>

#include <QFutureWatcher>
#include <QStringList>

namespace VcsBase { class SubmitFileModel; }

namespace Git::Internal {

class GitSubmitEditorWidget;
class GitSubmitEditorPanelData;

class CommitDataFetchResult
{
public:
    static CommitDataFetchResult fetch(CommitType commitType, const Utils::FilePath &workingDirectory);

    QString errorMessage;
    CommitData commitData;
    bool success;
};

class GitSubmitEditor : public VcsBase::VcsBaseSubmitEditor
{
    Q_OBJECT

public:
    GitSubmitEditor();
    ~GitSubmitEditor() override;

    void setCommitData(const CommitData &);
    GitSubmitEditorPanelData panelData() const;
    CommitType commitType() const { return m_commitType; }
    QString amendSHA1() const;
    void updateFileModel() override;

protected:
    QByteArray fileContents() const override;
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
    Utils::FilePath m_workingDirectory;
    bool m_firstUpdate = true;
    QFutureWatcher<CommitDataFetchResult> m_fetchWatcher;
};

} // Git::Internal
