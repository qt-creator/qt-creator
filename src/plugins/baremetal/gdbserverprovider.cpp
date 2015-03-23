/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gdbserverprovider.h"
#include "gdbserverprovidermanager.h"

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
    : m_id(other.m_id)
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
    if (m == m_startupMode)
        return;
    m_startupMode = m;
}

QString GdbServerProvider::initCommands() const
{
    return m_initCommands;
}

void GdbServerProvider::setInitCommands(const QString &cmds)
{
    if (cmds == m_initCommands)
        return;
    m_initCommands = cmds;
}

QString GdbServerProvider::resetCommands() const
{
    return m_resetCommands;
}

void GdbServerProvider::setResetCommands(const QString &cmds)
{
    if (cmds == m_resetCommands)
        return;
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
            && m_resetCommands == other.m_resetCommands
            ;
}

QVariantMap GdbServerProvider::toMap() const
{
    QVariantMap result;
    result.insert(QLatin1String(idKeyC), m_id);
    result.insert(QLatin1String(displayNameKeyC), m_displayName);
    result.insert(QLatin1String(startupModeKeyC), m_startupMode);
    result.insert(QLatin1String(initCommandsKeyC), m_initCommands);
    result.insert(QLatin1String(resetCommandsKeyC), m_resetCommands);
    return result;
}

bool GdbServerProvider::isValid() const
{
    return !channel().isNull();
}

bool GdbServerProvider::canStartupMode(StartupMode m) const
{
    return m == NoStartup;
}

void GdbServerProvider::providerUpdated()
{
    GdbServerProviderManager::instance()->notifyAboutUpdate(this);
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

    connect(m_nameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(dirty()));
    connect(m_startupModeComboBox, SIGNAL(currentIndexChanged(int)), SIGNAL(dirty()));
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
    const bool b = blockSignals(true);
    m_nameLineEdit->setText(m_provider->displayName());
    setStartupMode(m_provider->startupMode());
    blockSignals(b);
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

    connect(m_hostLineEdit, SIGNAL(textChanged(QString)), SIGNAL(dataChanged()));
    connect(m_portSpinBox, SIGNAL(valueChanged(int)), SIGNAL(dataChanged()));
}

void HostWidget::setHost(const QString &host)
{
    const bool b = blockSignals(true);
    m_hostLineEdit->setText(host);
    blockSignals(b);
}

QString HostWidget::host() const
{
    return m_hostLineEdit->text();
}

void HostWidget::setPort(const quint16 &port)
{
    const bool b = blockSignals(true);
    m_portSpinBox->setValue(port);
    blockSignals(b);
}

quint16 HostWidget::port() const
{
    return m_portSpinBox->value();
}

} // namespace Internal
} // namespace BareMetal
