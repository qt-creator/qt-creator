/****************************************************************************
**
** Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
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

#include "branchcombobox.h"
#include "../gitplugin.h"
#include "../gitclient.h"

using namespace Git::Internal;
using namespace Gerrit::Internal;

BranchComboBox::BranchComboBox(QWidget *parent) : QComboBox(parent)
{ }

void BranchComboBox::init(const QString &repository)
{
    m_repository = repository;
    QString currentBranch = GitPlugin::client()->synchronousCurrentLocalBranch(repository);
    if (currentBranch.isEmpty()) {
        m_detached = true;
        currentBranch = "HEAD";
        addItem(currentBranch);
    }
    QString output;
    const QString branchPrefix("refs/heads/");
    if (!GitPlugin::client()->synchronousForEachRefCmd(
                m_repository, { "--format=%(refname)", branchPrefix }, &output)) {
        return;
    }
    const QStringList branches = output.trimmed().split('\n');
    for (const QString &ref : branches) {
        const QString branch = ref.mid(branchPrefix.size());
        addItem(branch);
    }
    if (currentBranch.isEmpty())
        return;
    int index = findText(currentBranch);
    if (index != -1)
        setCurrentIndex(index);
}
