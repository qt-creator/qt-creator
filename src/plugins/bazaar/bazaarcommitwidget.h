// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/submiteditorwidget.h>

namespace Bazaar::Internal {

class BranchInfo;
class BazaarCommitPanel;

/*submit editor widget based on git SubmitEditor
  Some extra fields have been added to the standard SubmitEditorWidget,
  to help to conform to the commit style that is used by both git and Bazaar*/

class BazaarCommitWidget : public VcsBase::SubmitEditorWidget
{
public:
    BazaarCommitWidget();

    void setFields(const BranchInfo &branch, const QString &userName, const QString &email);

    QString committer() const;
    QStringList fixedBugs() const;
    bool isLocalOptionEnabled() const;

private:
    BazaarCommitPanel *m_bazaarCommitPanel;
};

} // Bazaar::Insteral
