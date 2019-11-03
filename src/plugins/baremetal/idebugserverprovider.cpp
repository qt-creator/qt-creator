/****************************************************************************
**
** Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "baremetaldevice.h"
#include "debugserverprovidermanager.h"
#include "idebugserverprovider.h"

#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QUuid>

namespace BareMetal {
namespace Internal {

const char idKeyC[] = "BareMetal.IDebugServerProvider.Id";
const char displayNameKeyC[] = "BareMetal.IDebugServerProvider.DisplayName";

static QString createId(const QString &id)
{
    QString newId = id.left(id.indexOf(QLatin1Char(':')));
    newId.append(QLatin1Char(':') + QUuid::createUuid().toString());
    return newId;
}

// IDebugServerProvider

IDebugServerProvider::IDebugServerProvider(const QString &id)
    : m_id(createId(id))
{
}

IDebugServerProvider::IDebugServerProvider(const IDebugServerProvider &provider)
    : m_id(createId(provider.m_id))
{
    m_displayName = QCoreApplication::translate(
                "BareMetal::IDebugServerProvider", "Clone of %1")
            .arg(provider.displayName());
}

IDebugServerProvider::~IDebugServerProvider()
{
    const QSet<BareMetalDevice *> devices = m_devices;
    for (BareMetalDevice *device : devices)
        device->unregisterDebugServerProvider(this);
}

QString IDebugServerProvider::displayName() const
{
    if (m_displayName.isEmpty())
        return typeDisplayName();
    return m_displayName;
}

void IDebugServerProvider::setDisplayName(const QString &name)
{
    if (m_displayName == name)
        return;

    m_displayName = name;
    providerUpdated();
}

QString IDebugServerProvider::id() const
{
    return m_id;
}

QString IDebugServerProvider::typeDisplayName() const
{
    return m_typeDisplayName;
}

bool IDebugServerProvider::operator==(const IDebugServerProvider &other) const
{
    if (this == &other)
        return true;

    const QString thisId = id().left(id().indexOf(QLatin1Char(':')));
    const QString otherId = other.id().left(other.id().indexOf(QLatin1Char(':')));

    // We ignore displayname
    return thisId == otherId;
}

QVariantMap IDebugServerProvider::toMap() const
{
    return {
        {QLatin1String(idKeyC), m_id},
        {QLatin1String(displayNameKeyC), m_displayName},
    };
}

void IDebugServerProvider::registerDevice(BareMetalDevice *device)
{
    m_devices.insert(device);
}

void IDebugServerProvider::unregisterDevice(BareMetalDevice *device)
{
    m_devices.remove(device);
}

void IDebugServerProvider::providerUpdated()
{
    DebugServerProviderManager::notifyAboutUpdate(this);
    for (BareMetalDevice *device : qAsConst(m_devices))
        device->debugServerProviderUpdated(this);
}

bool IDebugServerProvider::fromMap(const QVariantMap &data)
{
    m_id = data.value(QLatin1String(idKeyC)).toString();
    m_displayName = data.value(QLatin1String(displayNameKeyC)).toString();
    return true;
}

void IDebugServerProvider::setTypeDisplayName(const QString &typeDisplayName)
{
    m_typeDisplayName = typeDisplayName;
}

// IDebugServerProviderFactory

QString IDebugServerProviderFactory::id() const
{
    return m_id;
}

void IDebugServerProviderFactory::setId(const QString &id)
{
    m_id = id;
}

QString IDebugServerProviderFactory::displayName() const
{
    return m_displayName;
}

void IDebugServerProviderFactory::setDisplayName(const QString &name)
{
    m_displayName = name;
}

QString IDebugServerProviderFactory::idFromMap(const QVariantMap &data)
{
    return data.value(QLatin1String(idKeyC)).toString();
}

void IDebugServerProviderFactory::idToMap(QVariantMap &data, const QString &id)
{
    data.insert(QLatin1String(idKeyC), id);
}

// IDebugServerProviderConfigWidget

IDebugServerProviderConfigWidget::IDebugServerProviderConfigWidget(
        IDebugServerProvider *provider)
    : m_provider(provider)
{
    Q_ASSERT(provider);

    m_mainLayout = new QFormLayout(this);
    m_mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_nameLineEdit = new QLineEdit(this);
    m_nameLineEdit->setToolTip(tr("Enter the name of the debugger server provider."));
    m_mainLayout->addRow(tr("Name:"), m_nameLineEdit);

    setFromProvider();

    connect(m_nameLineEdit, &QLineEdit::textChanged,
            this, &IDebugServerProviderConfigWidget::dirty);
}

void IDebugServerProviderConfigWidget::apply()
{
    m_provider->setDisplayName(m_nameLineEdit->text());
}

void IDebugServerProviderConfigWidget::discard()
{
    setFromProvider();
}

void IDebugServerProviderConfigWidget::addErrorLabel()
{
    if (!m_errorLabel) {
        m_errorLabel = new QLabel;
        m_errorLabel->setVisible(false);
    }
    m_mainLayout->addRow(m_errorLabel);
}

void IDebugServerProviderConfigWidget::setErrorMessage(const QString &m)
{
    QTC_ASSERT(m_errorLabel, return);
    if (m.isEmpty()) {
        clearErrorMessage();
    } else {
        m_errorLabel->setText(m);
        m_errorLabel->setStyleSheet(QLatin1String("background-color: \"red\""));
        m_errorLabel->setVisible(true);
    }
}

void IDebugServerProviderConfigWidget::clearErrorMessage()
{
    QTC_ASSERT(m_errorLabel, return);
    m_errorLabel->clear();
    m_errorLabel->setStyleSheet(QString());
    m_errorLabel->setVisible(false);
}

void IDebugServerProviderConfigWidget::setFromProvider()
{
    const QSignalBlocker blocker(this);
    m_nameLineEdit->setText(m_provider->displayName());
}

} // namespace Internal
} // namespace BareMetal
