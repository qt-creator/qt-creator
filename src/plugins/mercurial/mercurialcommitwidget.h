// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_mercurialcommitpanel.h"

#include <vcsbase/submiteditorwidget.h>

namespace Mercurial {
namespace Internal {

/*submit editor widget based on git SubmitEditor
  Some extra fields have been added to the standard SubmitEditorWidget,
  to help to conform to the commit style that is used by both git and Mercurial*/

class MercurialCommitWidget : public VcsBase::SubmitEditorWidget
{
public:
    MercurialCommitWidget();

    void setFields(const QString &repositoryRoot, const QString &branch,
                   const QString &userName, const QString &email);

    QString committer();
    QString repoRoot();

protected:
    QString cleanupDescription(const QString &input) const override;

private:
    QWidget *mercurialCommitPanel;
    Ui::MercurialCommitPanel mercurialCommitPanelUi;
};

} // namespace Internal
} // namespace Mercurial
