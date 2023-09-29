// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baremetalconstants.h"

#include "baremetaltr.h"
#include "debugserverproviderchooser.h"
#include "debugserverprovidermanager.h"
#include "idebugserverprovider.h"

#include <coreplugin/icore.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>

namespace BareMetal::Internal {

// DebugServerProviderChooser

DebugServerProviderChooser::DebugServerProviderChooser(
        bool useManageButton, QWidget *parent)
    : QWidget(parent)
{
    m_chooser = new QComboBox(this);
    m_chooser->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_manageButton = new QPushButton(Tr::tr("Manage..."), this);
    m_manageButton->setEnabled(useManageButton);
    m_manageButton->setVisible(useManageButton);

    const auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chooser);
    layout->addWidget(m_manageButton);
    setFocusProxy(m_manageButton);

    connect(m_chooser, &QComboBox::currentIndexChanged,
            this, &DebugServerProviderChooser::currentIndexChanged);
    connect(m_manageButton, &QAbstractButton::clicked,
            this, &DebugServerProviderChooser::manageButtonClicked);
    connect(DebugServerProviderManager::instance(), &DebugServerProviderManager::providersChanged,
            this, &DebugServerProviderChooser::populate);
}

QString DebugServerProviderChooser::currentProviderId() const
{
    const int idx = m_chooser->currentIndex();
    return qvariant_cast<QString>(m_chooser->itemData(idx));
}

void DebugServerProviderChooser::setCurrentProviderId(const QString &id)
{
    for (int i = 0; i < m_chooser->count(); ++i) {
        if (id != qvariant_cast<QString>(m_chooser->itemData(i)))
            continue;
        m_chooser->setCurrentIndex(i);
    }
}

void DebugServerProviderChooser::manageButtonClicked()
{
    Core::ICore::showOptionsDialog(Constants::DEBUG_SERVER_PROVIDERS_SETTINGS_ID, this);
}

void DebugServerProviderChooser::currentIndexChanged(int index)
{
    Q_UNUSED(index)
    emit providerChanged();
}

bool DebugServerProviderChooser::providerMatches(const IDebugServerProvider *provider) const
{
    return provider->isValid();
}

QString DebugServerProviderChooser::providerText(const IDebugServerProvider *provider) const
{
    return provider->displayName();
}

void DebugServerProviderChooser::populate()
{
    const QSignalBlocker blocker(m_chooser);
    m_chooser->clear();
    m_chooser->addItem(Tr::tr("None"));

    for (const IDebugServerProvider *p : DebugServerProviderManager::providers()) {
        if (!providerMatches(p))
            continue;
        m_chooser->addItem(providerText(p), QVariant::fromValue(p->id()));
    }
}

} // BareMetal::Internal
