// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "configurationpanel.h"

#include "beautifiertool.h"
#include "beautifiertr.h"
#include "configurationdialog.h"

#include <utils/layoutbuilder.h>

#include <QComboBox>
#include <QPushButton>

namespace Beautifier::Internal {

ConfigurationPanel::ConfigurationPanel(QWidget *parent)
    : QWidget(parent)
{
    m_configurations = new QComboBox;
    m_configurations->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_edit = new QPushButton(Tr::tr("Edit"));
    m_remove = new QPushButton(Tr::tr("Remove"));
    auto add = new QPushButton(Tr::tr("Add"));

    using namespace Layouting;

    Row {
        m_configurations,
        m_edit,
        m_remove,
        add,
        noMargin,
    }.attachTo(this);

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
    dialog.setWindowTitle(Tr::tr("Add Configuration"));
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
    dialog.setWindowTitle(Tr::tr("Edit Configuration"));
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
