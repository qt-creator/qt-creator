/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "configurationpanel.h"
#include "ui_configurationpanel.h"

#include "abstractsettings.h"
#include "configurationdialog.h"

namespace Beautifier {
namespace Internal {

ConfigurationPanel::ConfigurationPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConfigurationPanel)
{
    ui->setupUi(this);
    connect(ui->add, &QPushButton::clicked, this, &ConfigurationPanel::add);
    connect(ui->edit, &QPushButton::clicked, this, &ConfigurationPanel::edit);
    connect(ui->remove, &QPushButton::clicked, this, &ConfigurationPanel::remove);
    connect(ui->configurations, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ConfigurationPanel::updateButtons);
}

ConfigurationPanel::~ConfigurationPanel()
{
    delete ui;
}

void ConfigurationPanel::setSettings(AbstractSettings *settings)
{
    m_settings = settings;
    populateConfigurations();
}

void ConfigurationPanel::setCurrentConfiguration(const QString &text)
{
    const int textIndex = ui->configurations->findText(text);
    if (textIndex != -1)
        ui->configurations->setCurrentIndex(textIndex);
}

QString ConfigurationPanel::currentConfiguration() const
{
    return ui->configurations->currentText();
}

void ConfigurationPanel::remove()
{
    m_settings->removeStyle(ui->configurations->currentText());
    populateConfigurations();
}

void ConfigurationPanel::add()
{
    ConfigurationDialog dialog;
    dialog.setWindowTitle(tr("Add Configuration"));
    dialog.setSettings(m_settings);
    if (dialog.exec() == QDialog::Accepted) {
        const QString key = dialog.key();
        m_settings->setStyle(key, dialog.value());
        populateConfigurations(key);
    }
}

void ConfigurationPanel::edit()
{
    const QString key = ui->configurations->currentText();
    ConfigurationDialog dialog;
    dialog.setWindowTitle(tr("Edit Configuration"));
    dialog.setSettings(m_settings);
    dialog.setKey(key);
    if (dialog.exec() == QDialog::Accepted) {
        const QString newKey = dialog.key();
        if (newKey == key) {
            m_settings->setStyle(key, dialog.value());
        } else {
            m_settings->replaceStyle(key, newKey, dialog.value());
            ui->configurations->setItemText(ui->configurations->currentIndex(), newKey);
        }
    }
}

void ConfigurationPanel::populateConfigurations(const QString &key)
{
    const bool block = ui->configurations->blockSignals(true);
    const QString currentText = (!key.isEmpty()) ? key : ui->configurations->currentText();
    ui->configurations->clear();
    ui->configurations->addItems(m_settings->styles());
    const int textIndex = ui->configurations->findText(currentText);
    if (textIndex != -1)
        ui->configurations->setCurrentIndex(textIndex);
    updateButtons();
    ui->configurations->blockSignals(block);
}

void ConfigurationPanel::updateButtons()
{
    const bool enabled = (ui->configurations->count() > 0)
            && !m_settings->styleIsReadOnly(ui->configurations->currentText());
    ui->remove->setEnabled(enabled);
    ui->edit->setEnabled(enabled);
}

} // namespace Internal
} // namespace Beautifier
