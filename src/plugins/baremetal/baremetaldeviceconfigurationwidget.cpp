/****************************************************************************
**
** Copyright (C) 2014 Tim Sander <tim@krieglstein.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "baremetaldeviceconfigurationwidget.h"

#include "baremetaldevice.h"

#include <coreplugin/variablechooser.h>
#include <ssh/sshconnection.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPlainTextEdit>

using namespace Core;
using namespace QSsh;

namespace BareMetal {
namespace Internal {

BareMetalDeviceConfigurationWidget::BareMetalDeviceConfigurationWidget(
        const ProjectExplorer::IDevice::Ptr &deviceConfig, QWidget *parent)
    : IDeviceWidget(deviceConfig, parent)
{
    SshConnectionParameters sshParams = device()->sshParameters();
    QSharedPointer<BareMetalDevice> p = qSharedPointerCast<BareMetalDevice>(device());
    QTC_ASSERT(!p.isNull(), return);

    m_gdbHostLineEdit = new QLineEdit(this);
    m_gdbHostLineEdit->setText(sshParams.host);
    m_gdbHostLineEdit->setToolTip(BareMetalDevice::hostLineToolTip());

    m_gdbPortSpinBox = new QSpinBox(this);
    m_gdbPortSpinBox->setRange(1, 65535);
    m_gdbPortSpinBox->setValue(sshParams.port);

    m_gdbInitCommandsTextEdit = new QPlainTextEdit(this);
    m_gdbInitCommandsTextEdit->setPlainText(p->gdbInitCommands());
    m_gdbInitCommandsTextEdit->setToolTip(BareMetalDevice::initCommandToolTip());

    m_gdbResetCommandsTextEdit = new QPlainTextEdit(this);
    m_gdbResetCommandsTextEdit->setPlainText(p->gdbResetCommands());
    m_gdbResetCommandsTextEdit->setToolTip(BareMetalDevice::resetCommandToolTip());

    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->addRow(tr("GDB host:"), m_gdbHostLineEdit);
    formLayout->addRow(tr("GDB port:"), m_gdbPortSpinBox);
    formLayout->addRow(tr("Init commands:"), m_gdbInitCommandsTextEdit);
    formLayout->addRow(tr("Reset commands:"), m_gdbResetCommandsTextEdit);

    VariableChooser::addVariableSupport(m_gdbResetCommandsTextEdit);
    VariableChooser::addVariableSupport(m_gdbInitCommandsTextEdit);
    (void)new VariableChooser(this);

    connect(m_gdbHostLineEdit, SIGNAL(editingFinished()), SLOT(hostnameChanged()));
    connect(m_gdbPortSpinBox, SIGNAL(valueChanged(int)), SLOT(portChanged()));
    connect(m_gdbResetCommandsTextEdit, SIGNAL(textChanged()),SLOT(gdbResetCommandsChanged()));
    connect(m_gdbInitCommandsTextEdit, SIGNAL(textChanged()), SLOT(gdbInitCommandsChanged()));
}

void BareMetalDeviceConfigurationWidget::hostnameChanged()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.host = m_gdbHostLineEdit->text().trimmed();
    device()->setSshParameters(sshParams);
}

void BareMetalDeviceConfigurationWidget::portChanged()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.port = m_gdbPortSpinBox->value();
    device()->setSshParameters(sshParams);
}

void BareMetalDeviceConfigurationWidget::gdbResetCommandsChanged()
{
    QSharedPointer<BareMetalDevice> p = qSharedPointerCast<BareMetalDevice>(device());
    QTC_ASSERT(!p.isNull(), return);
    p->setGdbResetCommands(m_gdbResetCommandsTextEdit->toPlainText().trimmed());
}

void BareMetalDeviceConfigurationWidget::gdbInitCommandsChanged()
{
    QSharedPointer<BareMetalDevice> p = qSharedPointerCast<BareMetalDevice>(device());
    QTC_ASSERT(!p.isNull(), return);
    p->setGdbInitCommands(m_gdbInitCommandsTextEdit->toPlainText());
}

void BareMetalDeviceConfigurationWidget::updateDeviceFromUi()
{
    hostnameChanged();
    portChanged();
    gdbResetCommandsChanged();
    gdbInitCommandsChanged();
}

} // namespace Internal
} // namespace BareMetal
