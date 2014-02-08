/****************************************************************************
**
** Copyright (C) 2014 Orgad Shaneh <orgads@gmail.com>.
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

#include "branchcombobox.h"
#include "../gitplugin.h"
#include "../gitclient.h"

using namespace Git::Internal;
using namespace Gerrit::Internal;

BranchComboBox::BranchComboBox(QWidget *parent) :
    QComboBox(parent),
    m_detached(false)
{
    m_client = GitPlugin::instance()->gitClient();
}

void BranchComboBox::init(const QString &repository)
{
    m_repository = repository;
    QString currentBranch = m_client->synchronousCurrentLocalBranch(repository);
    if (currentBranch.isEmpty()) {
        m_detached = true;
        currentBranch = QLatin1String("HEAD");
        addItem(currentBranch);
    }
    QString output;
    const QString branchPrefix(QLatin1String("refs/heads/"));
    QStringList args;
    args << QLatin1String("--format=%(refname)") << branchPrefix;
    if (!m_client->synchronousForEachRefCmd(m_repository, args, &output))
        return;
    QStringList branches = output.trimmed().split(QLatin1Char('\n'));
    foreach (const QString &ref, branches) {
        const QString branch = ref.mid(branchPrefix.size());
        addItem(branch);
    }
    if (currentBranch.isEmpty())
        return;
    int index = findText(currentBranch);
    if (index != -1)
        setCurrentIndex(index);
}
