// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addnewbackenddialog.h"
#include "ui_addnewbackenddialog.h"

#include <QPushButton>

namespace QmlDesigner {

AddNewBackendDialog::AddNewBackendDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::AddNewBackendDialog)
{
    m_ui->setupUi(this);

    connect(m_ui->comboBox, &QComboBox::currentTextChanged, this, &AddNewBackendDialog::invalidate);

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        m_applied = true;
        close();
    });

    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &AddNewBackendDialog::close);
}

AddNewBackendDialog::~AddNewBackendDialog()
{
    delete m_ui;
}

void AddNewBackendDialog::setupPossibleTypes(const QList<QmlTypeData> &types)
{
    QSignalBlocker blocker(this);
    m_typeData = types;
    for (const QmlTypeData &typeData : types)
        m_ui->comboBox->addItem(typeData.typeName);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_ui->comboBox->count() > 0);
    invalidate();
}

QString AddNewBackendDialog::importString() const
{
    if (m_ui->comboBox->currentIndex() < 0)
        return QString();

    QmlTypeData typeData = m_typeData.at(m_ui->comboBox->currentIndex());

    return typeData.importUrl + " " + typeData.versionString;
}

QString AddNewBackendDialog::type() const
{
    if (m_ui->comboBox->currentIndex() < 0)
        return QString();

    return m_typeData.at(m_ui->comboBox->currentIndex()).typeName;
}

bool AddNewBackendDialog::applied() const
{
    return m_applied;
}

bool AddNewBackendDialog::localDefinition() const
{
    return m_ui->checkBox->isChecked();
}

bool AddNewBackendDialog::isSingleton() const
{
    return m_isSingleton;
}

void AddNewBackendDialog::invalidate()
{
    if (m_ui->comboBox->currentIndex() < 0)
        return;

    QmlTypeData typeData = m_typeData.at(m_ui->comboBox->currentIndex());
    m_ui->importLabel->setText(importString());

    m_ui->checkBox->setChecked(false);
    if (typeData.isSingleton)
        m_ui->checkBox->setEnabled(false);

    m_isSingleton = typeData.isSingleton;
}

} // namespace QmlDesigner
