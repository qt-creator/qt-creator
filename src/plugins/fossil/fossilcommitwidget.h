// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_fossilcommitpanel.h"

#include <vcsbase/submiteditorwidget.h>

QT_BEGIN_NAMESPACE
class QValidator;
QT_END_NAMESPACE

namespace Utils { class FilePath; }

namespace Fossil {
namespace Internal {

class BranchInfo;

/*submit editor widget based on git SubmitEditor
  Some extra fields have been added to the standard SubmitEditorWidget,
  to help to conform to the commit style that is used by both git and Fossil*/

class FossilCommitWidget : public VcsBase::SubmitEditorWidget
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

protected:
    bool canSubmit(QString *whyNot = nullptr) const;

private slots:
    void branchChanged();

private:
    bool isValidBranch() const;

    QWidget *m_commitPanel;
    Ui::FossilCommitPanel m_commitPanelUi;
    QValidator *m_branchValidator;
};

} // namespace Internal
} // namespace Fossil
