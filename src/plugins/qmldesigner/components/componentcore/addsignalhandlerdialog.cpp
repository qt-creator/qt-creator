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

#include "addsignalhandlerdialog.h"
#include "ui_addsignalhandlerdialog.h"

AddSignalHandlerDialog::AddSignalHandlerDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::AddSignalHandlerDialog)
{
    m_ui->setupUi(this);
    setModal(true);

    connect(m_ui->all, &QRadioButton::toggled, this, &AddSignalHandlerDialog::updateComboBox);
    connect(m_ui->properties, &QRadioButton::toggled, this, &AddSignalHandlerDialog::updateComboBox);
    connect(m_ui->frequent, &QRadioButton::toggled, this, &AddSignalHandlerDialog::updateComboBox);
    connect(this, &QDialog::accepted, this, &AddSignalHandlerDialog::handleAccepted);
}

AddSignalHandlerDialog::~AddSignalHandlerDialog()
{
    delete m_ui;
}

void AddSignalHandlerDialog::setSignals(const QStringList &_signals)
{
    m_signals = _signals;
    updateComboBox();
}

QString AddSignalHandlerDialog::signal() const
{
    return m_signal;
}

bool checkForPropertyChanges(const QString &signal)
{
    static const QStringList importantProperties = {"pressed","position","value",
                                             "checked","currentIndex","index",
                                             "text","currentText", "currentItem"};
    if (!signal.endsWith("Changed"))
        return true;

    QString property = signal;
    property.chop(7);

    /* Some important property changes we keep */
    return importantProperties.contains(property);
}

void AddSignalHandlerDialog::updateComboBox()
{
    m_ui->comboBox->clear();
    foreach (const QString &signal, m_signals) {
        if (m_ui->all->isChecked()) {
            m_ui->comboBox->addItem(signal);
        } else if (m_ui->properties->isChecked()) {
            if (signal.endsWith("Changed"))
                m_ui->comboBox->addItem(signal);
        } else if (checkForPropertyChanges(signal))
            m_ui->comboBox->addItem(signal);
    }
}

void AddSignalHandlerDialog::handleAccepted()
{
    m_signal = m_ui->comboBox->currentText();
    emit signalSelected();
}
