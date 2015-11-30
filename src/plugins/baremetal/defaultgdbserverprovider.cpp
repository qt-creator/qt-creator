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

#include "defaultgdbserverprovider.h"
#include "baremetalconstants.h"
#include "gdbserverprovidermanager.h"

#include <utils/qtcassert.h>

#include <coreplugin/variablechooser.h>

#include <QFormLayout>
#include <QPlainTextEdit>

namespace BareMetal {
namespace Internal {

static const char hostKeyC[] = "BareMetal.DefaultGdbServerProvider.Host";
static const char portKeyC[] = "BareMetal.DefaultGdbServerProvider.Port";

DefaultGdbServerProvider::DefaultGdbServerProvider()
    : GdbServerProvider(QLatin1String(Constants::DEFAULT_PROVIDER_ID))
    , m_host(QLatin1String("localhost"))
    , m_port(3333)
{
}

DefaultGdbServerProvider::DefaultGdbServerProvider(const DefaultGdbServerProvider &other)
    : GdbServerProvider(other)
    , m_host(other.m_host)
    , m_port(other.m_port)
{
}

quint16 DefaultGdbServerProvider::port() const
{
    return m_port;
}

void DefaultGdbServerProvider::setPort(const quint16 &port)
{
    m_port = port;
}

QString DefaultGdbServerProvider::host() const
{
    return m_host;
}

void DefaultGdbServerProvider::setHost(const QString &host)
{
    if (m_host == host)
        return;
    m_host = host;
    providerUpdated();
}

QString DefaultGdbServerProvider::typeDisplayName() const
{
    return DefaultGdbServerProviderFactory::tr("Default");
}

QString DefaultGdbServerProvider::channel() const
{
    // Just return as "host:port" form.
    if (m_port == 0)
        return m_host;
    return m_host + QLatin1Char(':') + QString::number(m_port);
}

bool DefaultGdbServerProvider::isValid() const
{
    if (!GdbServerProvider::isValid())
        return false;

    if (m_host.isEmpty())
        return false;
    return true;
}

GdbServerProvider *DefaultGdbServerProvider::clone() const
{
    return new DefaultGdbServerProvider(*this);
}

QVariantMap DefaultGdbServerProvider::toMap() const
{
    auto data = GdbServerProvider::toMap();
    data.insert(QLatin1String(hostKeyC), m_host);
    data.insert(QLatin1String(portKeyC), m_port);
    return data;
}

bool DefaultGdbServerProvider::fromMap(const QVariantMap &data)
{
    if (!GdbServerProvider::fromMap(data))
        return false;

    m_host = data.value(QLatin1String(hostKeyC)).toString();
    m_port = data.value(QLatin1String(portKeyC)).toInt();
    return true;
}

bool DefaultGdbServerProvider::operator==(const GdbServerProvider &other) const
{
    if (!GdbServerProvider::operator==(other))
        return false;

    const auto p = static_cast<const DefaultGdbServerProvider *>(&other);
    return m_host == p->m_host && m_port == p->m_port;
}

GdbServerProviderConfigWidget *DefaultGdbServerProvider::configurationWidget()
{
    return new DefaultGdbServerProviderConfigWidget(this);
}

DefaultGdbServerProviderFactory::DefaultGdbServerProviderFactory()
{
    setId(QLatin1String(Constants::DEFAULT_PROVIDER_ID));
    setDisplayName(tr("Default"));
}

GdbServerProvider *DefaultGdbServerProviderFactory::create()
{
    return new DefaultGdbServerProvider;
}

bool DefaultGdbServerProviderFactory::canRestore(const QVariantMap &data)
{
    const auto id = idFromMap(data);
    return id.startsWith(QLatin1String(Constants::DEFAULT_PROVIDER_ID)
                         + QLatin1Char(':'));
}

GdbServerProvider *DefaultGdbServerProviderFactory::restore(const QVariantMap &data)
{
    auto p = new DefaultGdbServerProvider;
    auto updated = data;
    if (p->fromMap(updated))
        return p;
    delete p;
    return 0;
}

DefaultGdbServerProviderConfigWidget::DefaultGdbServerProviderConfigWidget(
        DefaultGdbServerProvider *provider)
    : GdbServerProviderConfigWidget(provider)
{
    Q_ASSERT(provider);

    m_hostWidget = new HostWidget(this);
    m_mainLayout->addRow(tr("Host:"), m_hostWidget);

    m_initCommandsTextEdit = new QPlainTextEdit(this);
    m_initCommandsTextEdit->setToolTip(defaultInitCommandsTooltip());
    m_mainLayout->addRow(tr("Init commands:"), m_initCommandsTextEdit);
    m_resetCommandsTextEdit = new QPlainTextEdit(this);
    m_resetCommandsTextEdit->setToolTip(defaultResetCommandsTooltip());
    m_mainLayout->addRow(tr("Reset commands:"), m_resetCommandsTextEdit);

    addErrorLabel();
    setFromProvider();

    auto chooser = new Core::VariableChooser(this);
    chooser->addSupportedWidget(m_initCommandsTextEdit.data());
    chooser->addSupportedWidget(m_resetCommandsTextEdit.data());

    connect(m_hostWidget.data(), &HostWidget::dataChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_initCommandsTextEdit.data(), &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_resetCommandsTextEdit.data(), &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
}

void DefaultGdbServerProviderConfigWidget::applyImpl()
{
    auto p = static_cast<DefaultGdbServerProvider *>(provider());
    Q_ASSERT(p);

    p->setHost(m_hostWidget->host());
    p->setPort(m_hostWidget->port());
    p->setInitCommands(m_initCommandsTextEdit->toPlainText());
    p->setResetCommands(m_resetCommandsTextEdit->toPlainText());
}

void DefaultGdbServerProviderConfigWidget::discardImpl()
{
    setFromProvider();
}

void DefaultGdbServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<DefaultGdbServerProvider *>(provider());
    Q_ASSERT(p);

    const auto b = blockSignals(true);
    m_hostWidget->setHost(p->m_host);
    m_hostWidget->setPort(p->m_port);
    m_initCommandsTextEdit->setPlainText(p->initCommands());
    m_resetCommandsTextEdit->setPlainText(p->resetCommands());
    blockSignals(b);
}

} // namespace Internal
} // namespace ProjectExplorer
