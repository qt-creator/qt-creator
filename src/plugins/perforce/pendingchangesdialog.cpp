// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "pendingchangesdialog.h"

#include <QRegularExpression>


using namespace Perforce::Internal;

PendingChangesDialog::PendingChangesDialog(const QString &data, QWidget *parent) : QDialog(parent)
{
    m_ui.setupUi(this);
    if (!data.isEmpty()) {
        const QRegularExpression r(QLatin1String("Change\\s(\\d+?).*?\\s\\*?pending\\*?\\s(.+?)\n"));
        QListWidgetItem *item;
        QRegularExpressionMatchIterator it = r.globalMatch(data);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            item = new QListWidgetItem(tr("Change %1: %2").arg(match.captured(1),
                                                               match.captured(2).trimmed()),
                                       m_ui.listWidget);
            item->setData(234, match.captured(1).trimmed());
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
