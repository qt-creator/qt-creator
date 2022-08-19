// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_bazaarcommitpanel.h"

#include <vcsbase/submiteditorwidget.h>

namespace Bazaar {
namespace Internal {

class BranchInfo;

/*submit editor widget based on git SubmitEditor
  Some extra fields have been added to the standard SubmitEditorWidget,
  to help to conform to the commit style that is used by both git and Bazaar*/

class BazaarCommitWidget : public VcsBase::SubmitEditorWidget
{
public:
    BazaarCommitWidget();

    void setFields(const BranchInfo &branch,
                   const QString &userName, const QString &email);

    QString committer() const;
    QStringList fixedBugs() const;
    bool isLocalOptionEnabled() const;

private:
    QWidget *m_bazaarCommitPanel;
    Ui::BazaarCommitPanel m_bazaarCommitPanelUi;
};

} // namespace Internal
} // namespace Bazaar
