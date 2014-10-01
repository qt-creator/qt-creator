/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrydebugtokenpinsdialog.h"
#include "ui_blackberrydebugtokenpinsdialog.h"
#include "blackberrydebugtokenreader.h"
#include "blackberryconfigurationmanager.h"
#include "blackberrysigningutils.h"

#include <QStandardItemModel>
#include <QMessageBox>
#include <QLineEdit>

namespace Qnx {
namespace Internal {

BlackBerryDebugTokenPinsDialog::BlackBerryDebugTokenPinsDialog(const QString &debugToken, QWidget *parent) :
    QDialog(parent),
    ui(new Ui_BlackBerryDebugTokenPinsDialog),
    m_model(new QStandardItemModel(this)),
    m_debugTokenPath(debugToken),
    m_updated(false)
{
    ui->setupUi(this);
    setWindowTitle(tr("Debug Token PINs"));
    ui->pins->setModel(m_model);
    ui->pathLabel->setText(debugToken);
    BlackBerryDebugTokenReader reader(debugToken);
    if (reader.isValid()) {
        QStringList pins = reader.pins().split(QLatin1Char(','));
        foreach (const QString &pin, pins)
            m_model->appendRow(new QStandardItem(pin));
    }

    m_okButton = ui->buttonBox->button(QDialogButtonBox::Ok);

    ui->editButton->setEnabled(false);
    ui->removeButton->setEnabled(false);

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addPin()));
    connect(ui->editButton, SIGNAL(clicked()), this, SLOT(editPin()));
    connect(ui->removeButton, SIGNAL(clicked()), this, SLOT(removePin()));
    connect(m_okButton, SIGNAL(clicked()), this, SLOT(emitUpdatedPins()));
    connect(ui->pins, SIGNAL(pressed(QModelIndex)), this, SLOT(updateUi(QModelIndex)));
}

BlackBerryDebugTokenPinsDialog::~BlackBerryDebugTokenPinsDialog()
{
    delete ui;
}

void BlackBerryDebugTokenPinsDialog::addPin()
{
    bool ok;
    const QString pin = promptPIN(QString(), &ok);
    if (ok && !pin.isEmpty()) {
        m_model->appendRow(new QStandardItem(pin));
        m_updated = true;
    }
}

void BlackBerryDebugTokenPinsDialog::editPin()
{
    const QModelIndex index = ui->pins->currentIndex();
    if (!index.isValid())
        return;

    bool ok;
    QString pin = m_model->item(index.row(), 0)->text();
    QString newPin = promptPIN(pin, &ok);
    if (ok && newPin != pin) {
        m_model->item(index.row(), 0)->setText(newPin);
        m_updated = true;
    }
}

void BlackBerryDebugTokenPinsDialog::removePin()
{
    const QModelIndex index = ui->pins->currentIndex();
    if (!index.isValid())
        return;

    const QString pin = m_model->item(index.row(), 0)->text();
    const int result = QMessageBox::question(this, tr("Confirmation"),
            tr("Are you sure you want to remove PIN: %1?")
            .arg(pin), QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_model->removeRow(index.row());
        m_updated = true;
    }
}

void BlackBerryDebugTokenPinsDialog::updateUi(const QModelIndex& index)
{
    ui->editButton->setEnabled(index.isValid());
    ui->removeButton->setEnabled(index.isValid());
}

void BlackBerryDebugTokenPinsDialog::emitUpdatedPins()
{
    if (!m_updated)
        return;

    QStringList pins;
    for (int i = 0; i < m_model->rowCount(); i++)
        pins << m_model->item(i)->text();

    emit pinsUpdated(pins);
}

QString BlackBerryDebugTokenPinsDialog::promptPIN(const QString &value, bool *ok)
{
    QDialog dialog(this);
    QVBoxLayout *layout = new QVBoxLayout;
    QLineEdit *lineEdit = new QLineEdit;
    QDialogButtonBox *buttonBox = new QDialogButtonBox;

    lineEdit->setMaxLength(8);
    lineEdit->setText(value);

    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    layout->addWidget(lineEdit);
    layout->addWidget(buttonBox);

    dialog.setWindowTitle(tr("Debug Token PIN"));
    dialog.setLayout(layout);

    const bool rejected = dialog.exec() == QDialog::Rejected;
     if (ok)
         *ok = !rejected;

     if (rejected)
         return QString();

     return lineEdit->text();
}

} // Internal
} // Qnx
