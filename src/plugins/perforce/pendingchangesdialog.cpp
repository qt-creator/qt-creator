/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "pendingchangesdialog.h"

#include <QRegExp>


using namespace Perforce::Internal;

PendingChangesDialog::PendingChangesDialog(const QString &data, QWidget *parent) : QDialog(parent)
{
    m_ui.setupUi(this);
    if (!data.isEmpty()) {
        QRegExp r(QLatin1String("Change\\s(\\d+).*\\s\\*pending\\*\\s(.+)\n"));
        r.setMinimal(true);
        int pos = 0;
        QListWidgetItem *item;
        while ((pos = r.indexIn(data, pos)) != -1) {
            item = new QListWidgetItem(tr("Change %1: %2").arg(r.cap(1))
                .arg(r.cap(2).trimmed()), m_ui.listWidget);
            item->setData(234, r.cap(1).trimmed());
            ++pos;
        }
    }
    m_ui.listWidget->setSelectionMode(QListWidget::SingleSelection);
    if (m_ui.listWidget->count()) {
        m_ui.listWidget->setCurrentRow(0);
        m_ui.submitButton->setEnabled(true);
    } else {
        m_ui.submitButton->setEnabled(false);
    }
}

int PendingChangesDialog::changeNumber() const
{
    QListWidgetItem *item = m_ui.listWidget->item(m_ui.listWidget->currentRow());
    if (!item)
        return -1;
    bool ok = true;
    int i = item->data(234).toInt(&ok);
    return ok ? i : -1;
}
