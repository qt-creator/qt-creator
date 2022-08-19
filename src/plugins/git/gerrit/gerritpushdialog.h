// Copyright (C) 2016 Petar Perisin.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QDialog>
#include <QMultiMap>
#include <QDate>
#include <QSharedPointer>

namespace Git {
namespace Internal { class GitClient; }
}

namespace Gerrit {
namespace Internal {

class GerritParameters;

namespace Ui { class GerritPushDialog; }

class GerritPushDialog : public QDialog
{
    Q_OBJECT

public:
    GerritPushDialog(const Utils::FilePath &workingDir, const QString &reviewerList,
                     QSharedPointer<GerritParameters> parameters, QWidget *parent);
    ~GerritPushDialog() override;

    QString selectedCommit() const;
    QString selectedRemoteName() const;
    QString selectedRemoteBranchName() const;
    QString selectedTopic() const;
    QString reviewers() const;
    QString initErrorMessage() const;
    QString pushTarget() const;
    void storeTopic();

private:
    void setChangeRange();
    void onRemoteChanged(bool force = false);
    void setRemoteBranches(bool includeOld = false);
    void updateCommits(int index);
    void validate();

    using BranchDate = QPair<QString, QDate>;
    using RemoteBranchesMap = QMultiMap<QString, BranchDate>;

    QString determineRemoteBranch(const QString &localBranch);
    void initRemoteBranches();
    QString calculateChangeRange(const QString &branch);
    Utils::FilePath m_workingDir;
    QString m_suggestedRemoteBranch;
    QString m_initErrorMessage;
    Ui::GerritPushDialog *m_ui;
    RemoteBranchesMap m_remoteBranches;
    bool m_hasLocalCommits = false;
    bool m_currentSupportsWip = false;
};


} // namespace Internal
} // namespace Gerrit
