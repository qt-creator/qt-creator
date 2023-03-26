// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/submiteditorwidget.h>

namespace Mercurial::Internal {

class MercurialCommitPanel;

/*submit editor widget based on git SubmitEditor
  Some extra fields have been added to the standard SubmitEditorWidget,
  to help to conform to the commit style that is used by both git and Mercurial*/

class MercurialCommitWidget : public VcsBase::SubmitEditorWidget
{
public:
    MercurialCommitWidget();

    void setFields(const QString &repositoryRoot, const QString &branch,
                   const QString &userName, const QString &email);

    QString committer() const;
    QString repoRoot() const;

protected:
    QString cleanupDescription(const QString &input) const override;

    MercurialCommitPanel *mercurialCommitPanel;
};

} // Mercurial::Internal
