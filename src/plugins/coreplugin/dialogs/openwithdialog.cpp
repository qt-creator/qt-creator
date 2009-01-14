/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "openwithdialog.h"

#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtCore/QFileInfo>

using namespace Core;
using namespace Core::Internal;

OpenWithDialog::OpenWithDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    label->setText(tr("Open file '%1' with:").arg(QFileInfo(fileName).fileName()));
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()),
            this, SLOT(accept()));
    connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
            this, SLOT(reject()));
    connect(editorListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
        this, SLOT(accept()));
    connect(editorListWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(currentItemChanged(QListWidgetItem*,QListWidgetItem*)));

    setOkButtonEnabled(false);
}

void OpenWithDialog::setOkButtonEnabled(bool v)
{
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(v);
}

void OpenWithDialog::setEditors(const QStringList &editors)
{
    foreach (const QString &e, editors)
        editorListWidget->addItem(e);
}

QString OpenWithDialog::editor() const
{
    if (const QListWidgetItem *item = editorListWidget->currentItem())
        return item->text();
    return QString();
}

void OpenWithDialog::setCurrentEditor(int index)
{
    editorListWidget->setCurrentRow(index);
}

void OpenWithDialog::currentItemChanged(QListWidgetItem *current, QListWidgetItem *)
{
    setOkButtonEnabled(current);
}
