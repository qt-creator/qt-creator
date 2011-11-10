/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "branchinfo.h"
#include "commiteditor.h"
#include "bazaarcommitwidget.h"

#include <vcsbase/submitfilemodel.h>

#include <QtCore/QDebug>

#include <QDir> //TODO REMOVE WHEN BASE FILE CHANGES ARE PULLED

using namespace Bazaar::Internal;

CommitEditor::CommitEditor(const VCSBase::VCSBaseSubmitEditorParameters *parameters, QWidget *parent)
    : VCSBase::VCSBaseSubmitEditor(parameters, new BazaarCommitWidget(parent)),
      m_fileModel(0)
{
    setDisplayName(tr("Commit Editor"));
}

const BazaarCommitWidget *CommitEditor::commitWidget() const
{
    CommitEditor *nonConstThis = const_cast<CommitEditor *>(this);
    return static_cast<const BazaarCommitWidget *>(nonConstThis->widget());
}

BazaarCommitWidget *CommitEditor::commitWidget()
{
    return static_cast<BazaarCommitWidget *>(widget());
}

void CommitEditor::setFields(const BranchInfo &branch,
                             const QString &userName, const QString &email,
                             const QList<VCSBase::VCSBaseClient::StatusItem> &repoStatus)
{
    BazaarCommitWidget *bazaarWidget = commitWidget();
    if (!bazaarWidget)
        return;

    bazaarWidget->setFields(branch, userName, email);

    m_fileModel = new VCSBase::SubmitFileModel(this);
    foreach (const VCSBase::VCSBaseClient::StatusItem &item, repoStatus)
        if (item.flags != QLatin1String("Unknown"))
            m_fileModel->addFile(item.file, item.flags, true);
    setFileModel(m_fileModel);
}
