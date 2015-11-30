/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MERCURIALCOMMITWIDGET_H
#define MERCURIALCOMMITWIDGET_H

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
    QString cleanupDescription(const QString &input) const;

private:
    QWidget *mercurialCommitPanel;
    Ui::MercurialCommitPanel mercurialCommitPanelUi;
};

} // namespace Internal
} // namespace Mercurial

#endif // MERCURIALCOMMITWIDGET_H
