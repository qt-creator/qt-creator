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

#include "addnewbackenddialog.h"
#include "ui_addnewbackenddialog.h"

#include <QPushButton>

namespace QmlDesigner {

AddNewBackendDialog::AddNewBackendDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::AddNewBackendDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
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

void AddNewBackendDialog::setupPossibleTypes(const QList<CppTypeData> &types)
{
    bool block = blockSignals(true);
    m_typeData = types;
    for (const CppTypeData &typeData : types)
        m_ui->comboBox->addItem(typeData.typeName);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_ui->comboBox->count() > 0);
    invalidate();
    blockSignals(block);
}

QString AddNewBackendDialog::importString() const
{
    if (m_ui->comboBox->currentIndex() < 0)
        return QString();

    CppTypeData typeData = m_typeData.at(m_ui->comboBox->currentIndex());

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

    CppTypeData typeData = m_typeData.at(m_ui->comboBox->currentIndex());
    m_ui->importLabel->setText(importString());

    m_ui->checkBox->setChecked(false);
    if (typeData.isSingleton)
        m_ui->checkBox->setEnabled(false);

    m_isSingleton = typeData.isSingleton;
}

} // namespace QmlDesigner
