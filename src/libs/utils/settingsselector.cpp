/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "settingsselector.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

namespace Utils {

// --------------------------------------------------------------------------
// SettingsSelector
// --------------------------------------------------------------------------

SettingsSelector::SettingsSelector(QWidget *parent) :
    QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    m_configurationCombo = new QComboBox(this);
    m_configurationCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_configurationCombo->setMinimumContentsLength(80);

    m_addButton = new QPushButton(tr("Add"), this);
    m_removeButton = new QPushButton(tr("Remove"), this);
    m_renameButton = new QPushButton(tr("Rename"), this);

    m_label = new QLabel(this);
    m_label->setMinimumWidth(200);
    m_label->setBuddy(m_configurationCombo);

    layout->addWidget(m_label);
    layout->addWidget(m_configurationCombo);
    layout->addWidget(m_addButton);
    layout->addWidget(m_removeButton);
    layout->addWidget(m_renameButton);

    layout->addSpacerItem(new QSpacerItem(0, 0));

    updateButtonState();

    connect(m_addButton, &QAbstractButton::clicked, this, &SettingsSelector::add);
    connect(m_removeButton, &QAbstractButton::clicked,
            this, &SettingsSelector::removeButtonClicked);
    connect(m_renameButton, &QAbstractButton::clicked,
            this, &SettingsSelector::renameButtonClicked);
    connect(m_configurationCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &SettingsSelector::currentChanged);
}

SettingsSelector::~SettingsSelector()
{ }

void SettingsSelector::setConfigurationModel(QAbstractItemModel *model)
{
    if (m_configurationCombo->model()) {
        disconnect(m_configurationCombo->model(), &QAbstractItemModel::rowsInserted,
                   this, &SettingsSelector::updateButtonState);
        disconnect(m_configurationCombo->model(), &QAbstractItemModel::rowsRemoved,
                   this, &SettingsSelector::updateButtonState);
    }
    m_configurationCombo->setModel(model);
    connect(model, &QAbstractItemModel::rowsInserted, this, &SettingsSelector::updateButtonState);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &SettingsSelector::updateButtonState);

    updateButtonState();
}

QAbstractItemModel *SettingsSelector::configurationModel() const
{
    return m_configurationCombo->model();
}

void SettingsSelector::setLabelText(const QString &text)
{
    m_label->setText(text);
}

QString SettingsSelector::labelText() const
{
    return m_label->text();
}

void SettingsSelector::setCurrentIndex(int index)
{
    m_configurationCombo->setCurrentIndex(index);
}

void SettingsSelector::setAddMenu(QMenu *menu)
{
    m_addButton->setMenu(menu);
}

QMenu *SettingsSelector::addMenu() const
{
    return m_addButton->menu();
}

int SettingsSelector::currentIndex() const
{
    return m_configurationCombo->currentIndex();
}

void SettingsSelector::removeButtonClicked()
{
    int pos = currentIndex();
    if (pos < 0)
        return;
    const QString title = tr("Remove");
    const QString message = tr("Do you really want to delete the configuration <b>%1</b>?")
                                .arg(m_configurationCombo->currentText());
    QMessageBox msgBox(QMessageBox::Question, title, message, QMessageBox::Yes|QMessageBox::No, this);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setEscapeButton(QMessageBox::No);
    if (msgBox.exec() == QMessageBox::No)
        return;

    emit remove(pos);
}

void SettingsSelector::renameButtonClicked()
{
    int pos = currentIndex();
    if (pos < 0)
        return;

    QAbstractItemModel *model = m_configurationCombo->model();
    int row = m_configurationCombo->currentIndex();
    QModelIndex idx = model->index(row, 0);
    QString baseName = model->data(idx, Qt::EditRole).toString();

    bool ok;
    const QString message = tr("New name for configuration <b>%1</b>:").arg(baseName);

    QString name = QInputDialog::getText(this, tr("Rename..."), message,
                                         QLineEdit::Normal, baseName, &ok);
    if (!ok)
        return;

    emit rename(pos, name);
}

void SettingsSelector::updateButtonState()
{
    bool haveItems = m_configurationCombo->count() > 0;
    m_addButton->setEnabled(true);
    m_removeButton->setEnabled(haveItems);
    m_renameButton->setEnabled(haveItems);
}

} // namespace Utils
