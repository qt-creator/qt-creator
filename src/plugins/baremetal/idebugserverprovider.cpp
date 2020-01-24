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
#include <QSpinBox>
#include <QUuid>

using namespace Debugger;
using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

const char idKeyC[] = "BareMetal.IDebugServerProvider.Id";
const char displayNameKeyC[] = "BareMetal.IDebugServerProvider.DisplayName";
const char engineTypeKeyC[] = "BareMetal.IDebugServerProvider.EngineType";

const char hostKeySuffixC[] = ".Host";
const char portKeySuffixC[] = ".Port";

static QString createId(const QString &id)
{
    QString newId = id.left(id.indexOf(':'));
    newId.append(':' + QUuid::createUuid().toString());
    return newId;
}

// IDebugServerProvider

IDebugServerProvider::IDebugServerProvider(const QString &id)
    : m_id(createId(id))
{
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

void IDebugServerProvider::setChannel(const QUrl &channel)
{
    m_channel = channel;
}

void IDebugServerProvider::setChannel(const QString &host, int port)
{
    m_channel.setHost(host);
    m_channel.setPort(port);
}

QUrl IDebugServerProvider::channel() const
{
    return m_channel;
}

QString IDebugServerProvider::channelString() const
{
    // Just return as "host:port" form.
    if (m_channel.port() <= 0)
        return m_channel.host();
    return m_channel.host() + ':' + QString::number(m_channel.port());
}

QString IDebugServerProvider::id() const
{
    return m_id;
}

QString IDebugServerProvider::typeDisplayName() const
{
    return m_typeDisplayName;
}

void IDebugServerProvider::setTypeDisplayName(const QString &typeDisplayName)
{
    m_typeDisplayName = typeDisplayName;
}

DebuggerEngineType IDebugServerProvider::engineType() const
{
    return m_engineType;
}

void IDebugServerProvider::setEngineType(DebuggerEngineType engineType)
{
    if (m_engineType == engineType)
        return;
    m_engineType = engineType;
    providerUpdated();
}

void IDebugServerProvider::setSettingsKeyBase(const QString &settingsBase)
{
    m_settingsBase = settingsBase;
}

bool IDebugServerProvider::operator==(const IDebugServerProvider &other) const
{
    if (this == &other)
        return true;

    const QString thisId = id().left(id().indexOf(':'));
    const QString otherId = other.id().left(other.id().indexOf(':'));

    // We ignore displayname
    return thisId == otherId
            && m_engineType == other.m_engineType
            && m_channel == other.m_channel;
}

IDebugServerProviderConfigWidget *IDebugServerProvider::configurationWidget() const
{
    QTC_ASSERT(m_configurationWidgetCreator, return nullptr);
    return m_configurationWidgetCreator();
}

QVariantMap IDebugServerProvider::toMap() const
{
    return {
        {idKeyC, m_id},
        {displayNameKeyC, m_displayName},
        {engineTypeKeyC, m_engineType},
        {m_settingsBase + hostKeySuffixC, m_channel.host()},
        {m_settingsBase + portKeySuffixC, m_channel.port()},
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
}

void IDebugServerProvider::resetId()
{
    m_id = createId(m_id);
}

bool IDebugServerProvider::fromMap(const QVariantMap &data)
{
    m_id = data.value(idKeyC).toString();
    m_displayName = data.value(displayNameKeyC).toString();
    m_engineType = static_cast<DebuggerEngineType>(
                data.value(engineTypeKeyC, NoEngineType).toInt());
    m_channel.setHost(data.value(m_settingsBase + hostKeySuffixC).toString());
    m_channel.setPort(data.value(m_settingsBase + portKeySuffixC).toInt());
    return true;
}

void IDebugServerProvider::setConfigurationWidgetCreator(const std::function<IDebugServerProviderConfigWidget *()> &configurationWidgetCreator)
{
    m_configurationWidgetCreator = configurationWidgetCreator;
}

// IDebugServerProviderFactory

IDebugServerProviderFactory::IDebugServerProviderFactory() = default;

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

IDebugServerProvider *IDebugServerProviderFactory::create() const
{
    return m_creator();
}

IDebugServerProvider *IDebugServerProviderFactory::restore(const QVariantMap &data) const
{
    IDebugServerProvider *p = m_creator();
    const auto updated = data;

    if (p->fromMap(updated))
        return p;
    delete p;
    return nullptr;
}

bool IDebugServerProviderFactory::canRestore(const QVariantMap &data) const
{
    const QString id = idFromMap(data);
    return id.startsWith(m_id + ':');
}

void IDebugServerProviderFactory::setDisplayName(const QString &name)
{
    m_displayName = name;
}

void IDebugServerProviderFactory::setCreator(const std::function<IDebugServerProvider *()> &creator)
{
    m_creator = creator;
}

QString IDebugServerProviderFactory::idFromMap(const QVariantMap &data)
{
    return data.value(idKeyC).toString();
}

void IDebugServerProviderFactory::idToMap(QVariantMap &data, const QString &id)
{
    data.insert(idKeyC, id);
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
        m_errorLabel->setStyleSheet("background-color: \"red\"");
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

// HostWidget

HostWidget::HostWidget(QWidget *parent)
    : QWidget(parent)
{
    m_hostLineEdit = new QLineEdit(this);
    m_hostLineEdit->setToolTip(tr("Enter TCP/IP hostname of the debug server, "
                                  "like \"localhost\" or \"192.0.2.1\"."));
    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setRange(0, 65535);
    m_portSpinBox->setToolTip(tr("Enter TCP/IP port which will be listened by "
                                 "the debug server."));
    const auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_hostLineEdit);
    layout->addWidget(m_portSpinBox);

    connect(m_hostLineEdit, &QLineEdit::textChanged,
            this, &HostWidget::dataChanged);
    connect(m_portSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &HostWidget::dataChanged);
}

void HostWidget::setChannel(const QUrl &channel)
{
    const QSignalBlocker blocker(this);
    m_hostLineEdit->setText(channel.host());
    m_portSpinBox->setValue(channel.port());
}

QUrl HostWidget::channel() const
{
    QUrl url;
    url.setHost(m_hostLineEdit->text());
    url.setPort(m_portSpinBox->value());
    return url;
}

} // namespace Internal
} // namespace BareMetal
