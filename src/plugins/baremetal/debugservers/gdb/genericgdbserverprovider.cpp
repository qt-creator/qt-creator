/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "genericgdbserverprovider.h"

#include <baremetal/baremetalconstants.h>
#include <baremetal/debugserverprovidermanager.h>

#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QFormLayout>
#include <QPlainTextEdit>

namespace BareMetal {
namespace Internal {

// GenericGdbServerProvider

GenericGdbServerProvider::GenericGdbServerProvider()
    : GdbServerProvider(Constants::GDBSERVER_GENERIC_PROVIDER_ID)
{
    setChannel("localhost", 3333);
    setTypeDisplayName(GdbServerProvider::tr("Generic"));
    setConfigurationWidgetCreator([this] { return new GenericGdbServerProviderConfigWidget(this); });
}

QSet<GdbServerProvider::StartupMode> GenericGdbServerProvider::supportedStartupModes() const
{
    return {StartupOnNetwork};
}

// GenericGdbServerProviderFactory

GenericGdbServerProviderFactory::GenericGdbServerProviderFactory()
{
    setId(Constants::GDBSERVER_GENERIC_PROVIDER_ID);
    setDisplayName(GdbServerProvider::tr("Generic"));
    setCreator([] { return new GenericGdbServerProvider; });
}

// GdbServerProviderConfigWidget

GenericGdbServerProviderConfigWidget::GenericGdbServerProviderConfigWidget(
        GenericGdbServerProvider *provider)
    : GdbServerProviderConfigWidget(provider)
{
    Q_ASSERT(provider);

    m_hostWidget = new HostWidget(this);
    m_mainLayout->addRow(tr("Host:"), m_hostWidget);

    m_useExtendedRemoteCheckBox = new QCheckBox(this);
    m_useExtendedRemoteCheckBox->setToolTip("Use GDB target extended-remote");
    m_mainLayout->addRow(tr("Extended mode:"), m_useExtendedRemoteCheckBox);
    m_initCommandsTextEdit = new QPlainTextEdit(this);
    m_initCommandsTextEdit->setToolTip(defaultInitCommandsTooltip());
    m_mainLayout->addRow(tr("Init commands:"), m_initCommandsTextEdit);
    m_resetCommandsTextEdit = new QPlainTextEdit(this);
    m_resetCommandsTextEdit->setToolTip(defaultResetCommandsTooltip());
    m_mainLayout->addRow(tr("Reset commands:"), m_resetCommandsTextEdit);

    addErrorLabel();
    setFromProvider();

    const auto chooser = new Utils::VariableChooser(this);
    chooser->addSupportedWidget(m_initCommandsTextEdit);
    chooser->addSupportedWidget(m_resetCommandsTextEdit);

    connect(m_hostWidget, &HostWidget::dataChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_useExtendedRemoteCheckBox, &QCheckBox::stateChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_initCommandsTextEdit, &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_resetCommandsTextEdit, &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
}

void GenericGdbServerProviderConfigWidget::apply()
{
    const auto p = static_cast<GenericGdbServerProvider *>(m_provider);
    Q_ASSERT(p);

    p->setChannel(m_hostWidget->channel());
    p->setUseExtendedRemote(m_useExtendedRemoteCheckBox->isChecked());
    p->setInitCommands(m_initCommandsTextEdit->toPlainText());
    p->setResetCommands(m_resetCommandsTextEdit->toPlainText());
    IDebugServerProviderConfigWidget::apply();
}

void GenericGdbServerProviderConfigWidget::discard()
{
    setFromProvider();
    IDebugServerProviderConfigWidget::discard();
}

void GenericGdbServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<GenericGdbServerProvider *>(m_provider);
    Q_ASSERT(p);

    const QSignalBlocker blocker(this);
    m_hostWidget->setChannel(p->channel());
    m_useExtendedRemoteCheckBox->setChecked(p->useExtendedRemote());
    m_initCommandsTextEdit->setPlainText(p->initCommands());
    m_resetCommandsTextEdit->setPlainText(p->resetCommands());
}

} // namespace Internal
} // namespace ProjectExplorer
