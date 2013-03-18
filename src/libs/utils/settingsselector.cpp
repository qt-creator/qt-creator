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

    connect(m_addButton, SIGNAL(clicked()), this, SIGNAL(add()));
    connect(m_removeButton, SIGNAL(clicked()), this, SLOT(removeButtonClicked()));
    connect(m_renameButton, SIGNAL(clicked()), this, SLOT(renameButtonClicked()));
    connect(m_configurationCombo, SIGNAL(currentIndexChanged(int)),
            this, SIGNAL(currentChanged(int)));
}

SettingsSelector::~SettingsSelector()
{ }

void SettingsSelector::setConfigurationModel(QAbstractItemModel *model)
{
    if (m_configurationCombo->model()) {
        disconnect(m_configurationCombo->model(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(updateButtonState()));
        disconnect(m_configurationCombo->model(), SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(updateButtonState()));
    }
    m_configurationCombo->setModel(model);
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(updateButtonState()));
    connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(updateButtonState()));

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
