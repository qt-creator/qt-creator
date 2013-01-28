/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QRegExp>

#include "pendingchangesdialog.h"

using namespace Perforce::Internal;

PendingChangesDialog::PendingChangesDialog(const QString &data, QWidget *parent)
    : QDialog(parent)
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
