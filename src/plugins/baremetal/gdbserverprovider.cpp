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

#include "gdbserverprovider.h"
#include "gdbserverprovidermanager.h"
#include "baremetaldevice.h"

#include <utils/asconst.h>
#include <utils/qtcassert.h>
#include <utils/environment.h>

#include <QCoreApplication>
#include <QUuid>

#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QPlainTextEdit>

namespace BareMetal {
namespace Internal {

const char idKeyC[] = "BareMetal.GdbServerProvider.Id";
const char displayNameKeyC[] = "BareMetal.GdbServerProvider.DisplayName";
const char startupModeKeyC[] = "BareMetal.GdbServerProvider.Mode";
const char initCommandsKeyC[] = "BareMetal.GdbServerProvider.InitCommands";
const char resetCommandsKeyC[] = "BareMetal.GdbServerProvider.ResetCommands";

static QString createId(const QString &id)
{
    QString newId = id.left(id.indexOf(QLatin1Char(':')));
    newId.append(QLatin1Char(':') + QUuid::createUuid().toString());
    return newId;
}

GdbServerProvider::GdbServerProvider(const QString &id)
    : m_id(createId(id))
    , m_startupMode(NoStartup)
{
}

GdbServerProvider::GdbServerProvider(const GdbServerProvider &other)
    : m_id(createId(other.m_id))
    , m_startupMode(other.m_startupMode)
    , m_initCommands(other.m_initCommands)
    , m_resetCommands(other.m_resetCommands)
{
    m_displayName = QCoreApplication::translate(
                "BareMetal::GdbServerProvider", "Clone of %1")
            .arg(other.displayName());
}

GdbServerProvider::~GdbServerProvider()
{
    const QSet<BareMetalDevice *> devices = m_devices;
    for (BareMetalDevice *device : devices)
        device->unregisterProvider(this);
}

QString GdbServerProvider::displayName() const
{
    if (m_displayName.isEmpty())
        return typeDisplayName();
    return m_displayName;
}

void GdbServerProvider::setDisplayName(const QString &name)
{
    if (m_displayName == name)
        return;

    m_displayName = name;
    providerUpdated();
}

QString GdbServerProvider::id() const
{
    return m_id;
}

GdbServerProvider::StartupMode GdbServerProvider::startupMode() const
{
    return m_startupMode;
}

void GdbServerProvider::setStartupMode(StartupMode m)
{
    m_startupMode = m;
}

QString GdbServerProvider::initCommands() const
{
    return m_initCommands;
}

void GdbServerProvider::setInitCommands(const QString &cmds)
{
    m_initCommands = cmds;
}

QString GdbServerProvider::resetCommands() const
{
    return m_resetCommands;
}

void GdbServerProvider::setResetCommands(const QString &cmds)
{
    m_resetCommands = cmds;
}

QString GdbServerProvider::executable() const
{
    return QString();
}

QStringList GdbServerProvider::arguments() const
{
    return QStringList();
}

bool GdbServerProvider::operator==(const GdbServerProvider &other) const
{
    if (this == &other)
        return true;

    const QString thisId = id().left(id().indexOf(QLatin1Char(':')));
    const QString otherId = other.id().left(other.id().indexOf(QLatin1Char(':')));

    // We ignore displayname
    return thisId == otherId
            && m_startupMode == other.m_startupMode
            && m_initCommands == other.m_initCommands
            && m_resetCommands == other.m_resetCommands;
}

QVariantMap GdbServerProvider::toMap() const
{
    return {
        {QLatin1String(idKeyC), m_id},
        {QLatin1String(displayNameKeyC), m_displayName},
        {QLatin1String(startupModeKeyC), m_startupMode},
        {QLatin1String(initCommandsKeyC), m_initCommands},
        {QLatin1String(resetCommandsKeyC), m_resetCommands}
    };
}

bool GdbServerProvider::isValid() const
{
    return !channel().isNull();
}

bool GdbServerProvider::canStartupMode(StartupMode m) const
{
    return m == NoStartup;
}

void GdbServerProvider::registerDevice(BareMetalDevice *device)
{
    m_devices.insert(device);
}

void GdbServerProvider::unregisterDevice(BareMetalDevice *device)
{
    m_devices.remove(device);
}

void GdbServerProvider::providerUpdated()
{
    GdbServerProviderManager::notifyAboutUpdate(this);
    for (BareMetalDevice *device : Utils::asConst(m_devices))
        device->providerUpdated(this);
}

bool GdbServerProvider::fromMap(const QVariantMap &data)
{
    m_id = data.value(QLatin1String(idKeyC)).toString();
    m_displayName = data.value(QLatin1String(displayNameKeyC)).toString();
    m_startupMode = static_cast<StartupMode>(data.value(QLatin1String(startupModeKeyC)).toInt());
    m_initCommands = data.value(QLatin1String(initCommandsKeyC)).toString();
    m_resetCommands = data.value(QLatin1String(resetCommandsKeyC)).toString();
    return true;
}

QString GdbServerProviderFactory::id() const
{
    return m_id;
}

void GdbServerProviderFactory::setId(const QString &id)
{
    m_id = id;
}

QString GdbServerProviderFactory::displayName() const
{
    return m_displayName;
}

void GdbServerProviderFactory::setDisplayName(const QString &name)
{
    m_displayName = name;
}

QString GdbServerProviderFactory::idFromMap(const QVariantMap &data)
{
    return data.value(QLatin1String(idKeyC)).toString();
}

void GdbServerProviderFactory::idToMap(QVariantMap &data, const QString id)
{
    data.insert(QLatin1String(idKeyC), id);
}

GdbServerProviderConfigWidget::GdbServerProviderConfigWidget(
        GdbServerProvider *provider)
    : m_provider(provider)
{
    Q_ASSERT(provider);

    m_mainLayout = new QFormLayout(this);
    m_mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_nameLineEdit = new QLineEdit(this);
    m_nameLineEdit->setToolTip(tr("Enter the name of the GDB server provider."));
    m_mainLayout->addRow(tr("Name:"), m_nameLineEdit);

    m_startupModeComboBox = new QComboBox(this);
    m_startupModeComboBox->setToolTip(tr("Choose the desired startup mode "
                                         "of the GDB server provider."));
    m_mainLayout->addRow(tr("Startup mode:"), m_startupModeComboBox);

    populateStartupModes();
    setFromProvider();

    connect(m_nameLineEdit, &QLineEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_startupModeComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &GdbServerProviderConfigWidget::dirty);
}

void GdbServerProviderConfigWidget::apply()
{
    m_provider->setDisplayName(m_nameLineEdit->text());
    m_provider->setStartupMode(startupMode());

    applyImpl();
}

void GdbServerProviderConfigWidget::discard()
{
    m_nameLineEdit->setText(m_provider->displayName());
    discardImpl();
}

GdbServerProvider *GdbServerProviderConfigWidget::provider() const
{
    return m_provider;
}

void GdbServerProviderConfigWidget::addErrorLabel()
{
    if (!m_errorLabel) {
        m_errorLabel = new QLabel;
        m_errorLabel->setVisible(false);
    }
    m_mainLayout->addRow(m_errorLabel);
}

GdbServerProvider::StartupMode GdbServerProviderConfigWidget::startupModeFromIndex(
        int idx) const
{
    return static_cast<GdbServerProvider::StartupMode>(
                m_startupModeComboBox->itemData(idx).toInt());
}

GdbServerProvider::StartupMode GdbServerProviderConfigWidget::startupMode() const
{
    const int idx = m_startupModeComboBox->currentIndex();
    return startupModeFromIndex(idx);
}

void GdbServerProviderConfigWidget::setStartupMode(GdbServerProvider::StartupMode m)
{
    for (int idx = 0; idx < m_startupModeComboBox->count(); ++idx) {
        if (m == startupModeFromIndex(idx)) {
            m_startupModeComboBox->setCurrentIndex(idx);
            break;
        }
    }
}

void GdbServerProviderConfigWidget::populateStartupModes()
{
    for (int i = 0; i < GdbServerProvider::StartupModesCount; ++i) {
        const auto m = static_cast<GdbServerProvider::StartupMode>(i);
        if (!m_provider->canStartupMode(m))
            continue;

        const int idx = m_startupModeComboBox->count();
        m_startupModeComboBox->insertItem(
                    idx,
                    (m == GdbServerProvider::NoStartup)
                    ? tr("No Startup")
                    : ((m == GdbServerProvider::StartupOnNetwork)
                       ? tr("Startup in TCP/IP Mode")
                       : tr("Startup in Pipe Mode")),
                    m);
    }
}

void GdbServerProviderConfigWidget::setErrorMessage(const QString &m)
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

void GdbServerProviderConfigWidget::clearErrorMessage()
{
    QTC_ASSERT(m_errorLabel, return);
    m_errorLabel->clear();
    m_errorLabel->setStyleSheet(QString());
    m_errorLabel->setVisible(false);
}

void GdbServerProviderConfigWidget::setFromProvider()
{
    QSignalBlocker blocker(this);
    m_nameLineEdit->setText(m_provider->displayName());
    setStartupMode(m_provider->startupMode());
}

QString GdbServerProviderConfigWidget::defaultInitCommandsTooltip()
{
    return QCoreApplication::translate("BareMetal",
                                       "Enter GDB commands to reset the board "
                                       "and to write the nonvolatile memory.");
}

QString GdbServerProviderConfigWidget::defaultResetCommandsTooltip()
{
    return QCoreApplication::translate("BareMetal",
                                       "Enter GDB commands to reset the hardware. "
                                       "The MCU should be halted after these commands.");
}

HostWidget::HostWidget(QWidget *parent)
    : QWidget(parent)
{
    m_hostLineEdit = new QLineEdit(this);
    m_hostLineEdit->setToolTip(tr("Enter TCP/IP hostname of the GDB server provider, "
                                  "like \"localhost\" or \"192.0.2.1\"."));
    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setRange(0, 65535);
    m_portSpinBox->setToolTip(tr("Enter TCP/IP port which will be listened by "
                                 "the GDB server provider."));
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_hostLineEdit);
    layout->addWidget(m_portSpinBox);

    connect(m_hostLineEdit, &QLineEdit::textChanged,
            this, &HostWidget::dataChanged);
    connect(m_portSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &HostWidget::dataChanged);
}

void HostWidget::setHost(const QString &host)
{
    QSignalBlocker blocker(this);
    m_hostLineEdit->setText(host);
}

QString HostWidget::host() const
{
    return m_hostLineEdit->text();
}

void HostWidget::setPort(const quint16 &port)
{
    QSignalBlocker blocker(this);
    m_portSpinBox->setValue(port);
}

quint16 HostWidget::port() const
{
    return m_portSpinBox->value();
}

} // namespace Internal
} // namespace BareMetal
