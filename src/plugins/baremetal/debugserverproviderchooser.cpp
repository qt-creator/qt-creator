/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "baremetalconstants.h"

#include "debugserverproviderchooser.h"
#include "debugserverprovidermanager.h"
#include "idebugserverprovider.h"

#include <coreplugin/icore.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>

namespace BareMetal {
namespace Internal {

// DebugServerProviderChooser

DebugServerProviderChooser::DebugServerProviderChooser(
        bool useManageButton, QWidget *parent)
    : QWidget(parent)
{
    m_chooser = new QComboBox(this);
    m_chooser->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_manageButton = new QPushButton(tr("Manage..."), this);
    m_manageButton->setEnabled(useManageButton);
    m_manageButton->setVisible(useManageButton);

    const auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chooser);
    layout->addWidget(m_manageButton);
    setFocusProxy(m_manageButton);

    connect(m_chooser, QOverload<int>::of(&QComboBox::currentIndexChanged),
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
    m_chooser->addItem(tr("None"));

    for (const IDebugServerProvider *p : DebugServerProviderManager::providers()) {
        if (!providerMatches(p))
            continue;
        m_chooser->addItem(providerText(p), QVariant::fromValue(p->id()));
    }
}

} // namespace Internal
} // namespace BareMetal
