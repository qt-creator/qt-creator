/****************************************************************************
**
** Copyright (C) 2013 Petar Perisin.
** Contact: petar.perisin@gmail.com
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

#ifndef GERRITPUSHDIALOG_H
#define GERRITPUSHDIALOG_H

#include <QDialog>
#include <QMultiMap>

namespace Gerrit {
namespace Internal {

namespace Ui {
    class GerritPushDialog;
}

class GerritPushDialog : public QDialog
{
    Q_OBJECT

public:
    GerritPushDialog(const QString &workingDir, const QString &reviewerList, QWidget *parent);
    ~GerritPushDialog();

    QString selectedCommit() const;
    QString selectedRemoteName() const;
    QString selectedRemoteBranchName() const;
    QString selectedPushType() const;
    QString selectedTopic() const;
    QString reviewers() const;
    bool localChangesFound() const;
    bool valid() const;

private slots:
    void setChangeRange();
    void setRemoteBranches();

private:
    typedef QMultiMap<QString, QString> RemoteBranchesMap;

    QString calculateChangeRange();
    QString m_workingDir;
    QString m_suggestedRemoteName;
    QString m_suggestedRemoteBranch;
    Ui::GerritPushDialog *m_ui;
    RemoteBranchesMap m_remoteBranches;
    bool m_localChangesFound;
    bool m_valid;
};


} // namespace Internal
} // namespace Gerrit

#endif // GERRITPUSHDIALOG_H
