// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/submiteditorwidget.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLabel;
class QLineEdit;
class QValidator;
QT_END_NAMESPACE

namespace Utils {
class FilePath;
class InfoLabel;
}

namespace Fossil {
namespace Internal {

class BranchInfo;

/*submit editor widget based on git SubmitEditor
  Some extra fields have been added to the standard SubmitEditorWidget,
  to help to conform to the commit style that is used by both git and Fossil*/

class FossilCommitWidget final : public VcsBase::SubmitEditorWidget
{
    Q_OBJECT

public:
    FossilCommitWidget();

    void setFields(const Utils::FilePath &repoPath, const BranchInfo &newBranch,
                   const QStringList &tags, const QString &userName);

    QString newBranch() const;
    QStringList tags() const;
    QString committer() const;
    bool isPrivateOptionEnabled() const;

private:
    bool canSubmit(QString *whyNot = nullptr) const final;

    void branchChanged();
    bool isValidBranch() const;

    QWidget *m_commitPanel;
    QValidator *m_branchValidator;

    QLineEdit *m_localRootLineEdit;
    QLineEdit *m_currentBranchLineEdit;
    QLineEdit *m_currentTagsLineEdit;
    QLineEdit *m_branchLineEdit;
    Utils::InfoLabel *m_invalidBranchLabel;
    QCheckBox *m_isPrivateCheckBox;
    QLineEdit *m_tagsLineEdit;
    QLineEdit *m_authorLineEdit;
};

} // namespace Internal
} // namespace Fossil
