// Copyright (C) 2016 Petar Perisin.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QDialog>
#include <QMultiMap>
#include <QDate>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
QT_END_NAMESPACE

namespace Git::Internal { class LogChangeWidget; }

namespace Gerrit {
namespace Internal {

class BranchComboBox;
class GerritParameters;
class GerritRemoteChooser;

class GerritPushDialog : public QDialog
{
    Q_OBJECT

public:
    GerritPushDialog(const Utils::FilePath &workingDir, const QString &reviewerList,
                     QSharedPointer<GerritParameters> parameters, QWidget *parent);

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

    BranchComboBox *m_localBranchComboBox;
    Gerrit::Internal::GerritRemoteChooser *m_remoteComboBox;
    QComboBox *m_targetBranchComboBox;
    Git::Internal::LogChangeWidget *m_commitView;
    QLabel *m_infoLabel;
    QLineEdit *m_topicLineEdit;
    QCheckBox *m_draftCheckBox;
    QCheckBox *m_wipCheckBox;
    QLineEdit *m_reviewersLineEdit;
    QDialogButtonBox *m_buttonBox;

    Utils::FilePath m_workingDir;
    QString m_suggestedRemoteBranch;
    QString m_initErrorMessage;
    RemoteBranchesMap m_remoteBranches;
    bool m_hasLocalCommits = false;
    bool m_currentSupportsWip = false;
};


} // namespace Internal
} // namespace Gerrit
