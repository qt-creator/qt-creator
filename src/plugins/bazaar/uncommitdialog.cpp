/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
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

#include "uncommitdialog.h"

#include "ui_uncommitdialog.h"
#include "bazaarclient.h"
#include "bazaarplugin.h"
#include <utils/qtcassert.h>

#include <QPushButton>

namespace Bazaar {
namespace Internal {

UnCommitDialog::UnCommitDialog(QWidget *parent) : QDialog(parent),
    m_ui(new Ui::UnCommitDialog)
{
    m_ui->setupUi(this);

    auto dryRunBtn = new QPushButton(tr("Dry Run"));
    dryRunBtn->setToolTip(tr("Test the outcome of removing the last committed revision, without actually removing anything."));
    m_ui->buttonBox->addButton(dryRunBtn, QDialogButtonBox::ApplyRole);
    connect(dryRunBtn, &QPushButton::clicked, this, &UnCommitDialog::dryRun);
}

UnCommitDialog::~UnCommitDialog()
{
    delete m_ui;
}

QStringList UnCommitDialog::extraOptions() const
{
    QStringList opts;
    if (m_ui->keepTagsCheckBox->isChecked())
        opts += QLatin1String("--keep-tags");
    if (m_ui->localCheckBox->isChecked())
        opts += QLatin1String("--local");
    return opts;
}

QString UnCommitDialog::revision() const
{
    return m_ui->revisionLineEdit->text().trimmed();
}

void UnCommitDialog::dryRun()
{
    BazaarPlugin* bzrPlugin = BazaarPlugin::instance();
    QTC_ASSERT(bzrPlugin->currentState().hasTopLevel(), return);
    bzrPlugin->client()->synchronousUncommit(bzrPlugin->currentState().topLevel(),
                                             revision(),
                                             extraOptions() << QLatin1String("--dry-run"));
}

} // namespace Internal
} // namespace Bazaar
