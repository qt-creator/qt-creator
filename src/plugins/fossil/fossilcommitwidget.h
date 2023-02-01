/****************************************************************************
**
** Copyright (c) 2018 Artur Shepilko
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
