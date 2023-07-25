// Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "branchcombobox.h"
#include "../gitclient.h"

using namespace Git::Internal;
using namespace Gerrit::Internal;
using namespace Utils;

BranchComboBox::BranchComboBox(QWidget *parent) : QComboBox(parent)
{ }

void BranchComboBox::init(const FilePath &repository)
{
    m_repository = repository;
    QString currentBranch = gitClient().synchronousCurrentLocalBranch(repository);
    if (currentBranch.isEmpty()) {
        m_detached = true;
        currentBranch = "HEAD";
        addItem(currentBranch);
    }
    QString output;
    const QString branchPrefix("refs/heads/");
    if (!gitClient().synchronousForEachRefCmd(
                m_repository, {"--format=%(refname)", branchPrefix}, &output)) {
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
