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

#include "abstractsettings.h"
#include "configurationdialog.h"

#include <utils/layoutbuilder.h>

#include <QComboBox>
#include <QPushButton>

namespace Beautifier::Internal {

ConfigurationPanel::ConfigurationPanel(QWidget *parent)
    : QWidget(parent)
{
    resize(595, 23);

    m_configurations = new QComboBox;
    m_configurations->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_edit = new QPushButton(tr("Edit"));
    m_remove = new QPushButton(tr("Remove"));
    auto add = new QPushButton(tr("Add"));

    using namespace Utils::Layouting;

    Row {
        m_configurations,
        m_edit,
        m_remove,
        add
    }.attachTo(this, WithoutMargins);

    connect(add, &QPushButton::clicked, this, &ConfigurationPanel::add);
    connect(m_edit, &QPushButton::clicked, this, &ConfigurationPanel::edit);
    connect(m_remove, &QPushButton::clicked, this, &ConfigurationPanel::remove);
    connect(m_configurations, &QComboBox::currentIndexChanged,
            this, &ConfigurationPanel::updateButtons);
}

ConfigurationPanel::~ConfigurationPanel() = default;

void ConfigurationPanel::setSettings(AbstractSettings *settings)
{
    m_settings = settings;
    populateConfigurations();
}

void ConfigurationPanel::setCurrentConfiguration(const QString &text)
{
    const int textIndex = m_configurations->findText(text);
    if (textIndex != -1)
        m_configurations->setCurrentIndex(textIndex);
}

QString ConfigurationPanel::currentConfiguration() const
{
    return m_configurations->currentText();
}

void ConfigurationPanel::remove()
{
    m_settings->removeStyle(m_configurations->currentText());
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
    const QString key = m_configurations->currentText();
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
            m_configurations->setItemText(m_configurations->currentIndex(), newKey);
        }
    }
}

void ConfigurationPanel::populateConfigurations(const QString &key)
{
    QSignalBlocker blocker(m_configurations);
    const QString currentText = (!key.isEmpty()) ? key : m_configurations->currentText();
    m_configurations->clear();
    m_configurations->addItems(m_settings->styles());
    const int textIndex = m_configurations->findText(currentText);
    if (textIndex != -1)
        m_configurations->setCurrentIndex(textIndex);
    updateButtons();
}

void ConfigurationPanel::updateButtons()
{
    const bool enabled = (m_configurations->count() > 0)
            && !m_settings->styleIsReadOnly(m_configurations->currentText());
    m_remove->setEnabled(enabled);
    m_edit->setEnabled(enabled);
}

} // Beautifier::Internal
