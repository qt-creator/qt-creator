/**************************************************************************
**
** Copyright (c) 2013 Hugues Delorme
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef BAZAARCOMMITWIDGET_H
#define BAZAARCOMMITWIDGET_H

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
    explicit BazaarCommitWidget(QWidget *parent = 0);

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

#endif // BAZAARCOMMITWIDGET_H
