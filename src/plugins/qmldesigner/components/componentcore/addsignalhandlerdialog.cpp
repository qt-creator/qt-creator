// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    for (const QString &signal : std::as_const(m_signals)) {
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
