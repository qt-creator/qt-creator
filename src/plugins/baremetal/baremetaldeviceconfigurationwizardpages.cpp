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

#include "baremetaldeviceconfigurationwizardpages.h"
#include "baremetaldevice.h"

#include <coreplugin/variablechooser.h>
#include <projectexplorer/devicesupport/idevice.h>

#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>

using namespace Core;

namespace BareMetal {
namespace Internal {

BareMetalDeviceConfigurationWizardSetupPage::BareMetalDeviceConfigurationWizardSetupPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Set up GDB Server or Hardware Debugger"));

    m_nameLineEdit = new QLineEdit(this);

    m_hostNameLineEdit = new QLineEdit(this);
    m_hostNameLineEdit->setToolTip(BareMetalDevice::hostLineToolTip());
    m_hostNameLineEdit->setText(QLatin1String(
        "|openocd -c \"gdb_port pipe\" -c \"log_output openocd.log;\" "
        "-f board/stm3241g_eval_stlink.cfg"));

    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setRange(1, 65535);
    m_portSpinBox->setValue(3333);

    m_gdbInitCommandsPlainTextEdit = new QPlainTextEdit(this);
    m_gdbInitCommandsPlainTextEdit->setToolTip(BareMetalDevice::initCommandToolTip());
    m_gdbInitCommandsPlainTextEdit->setPlainText(QLatin1String(
        "set remote hardware-breakpoint-limit 6\n"
        "set remote hardware-watchpoint-limit 4\n"
        "monitor reset halt\n"
        "load\n"
        "monitor reset halt"));

    m_gdbResetCommandsTextEdit = new QPlainTextEdit(this);
    m_gdbResetCommandsTextEdit->setToolTip(BareMetalDevice::resetCommandToolTip());
    m_gdbResetCommandsTextEdit->setPlainText(QLatin1String("monitor reset halt"));

    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(tr("Name:"), m_nameLineEdit);
    formLayout->addRow(tr("GDB host:"), m_hostNameLineEdit);
    formLayout->addRow(tr("GDB port:"), m_portSpinBox);
    formLayout->addRow(tr("Init commands:"), m_gdbInitCommandsPlainTextEdit);
    formLayout->addRow(tr("Reset commands:"), m_gdbResetCommandsTextEdit);

    connect(m_nameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(m_hostNameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(m_portSpinBox, SIGNAL(valueChanged(int)), SIGNAL(completeChanged()));
    connect(m_gdbResetCommandsTextEdit, SIGNAL(textChanged()), SIGNAL(completeChanged()));
    connect(m_gdbInitCommandsPlainTextEdit, SIGNAL(textChanged()), SIGNAL(completeChanged()));

    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(m_gdbResetCommandsTextEdit);
    chooser->addSupportedWidget(m_gdbInitCommandsPlainTextEdit);
}

void BareMetalDeviceConfigurationWizardSetupPage::initializePage()
{
    m_nameLineEdit->setText(defaultConfigurationName());
}

bool BareMetalDeviceConfigurationWizardSetupPage::isComplete() const
{
    return !configurationName().isEmpty();
}

QString BareMetalDeviceConfigurationWizardSetupPage::configurationName() const
{
    return m_nameLineEdit->text().trimmed();
}

QString BareMetalDeviceConfigurationWizardSetupPage::gdbHostname() const
{
    return m_hostNameLineEdit->text().trimmed();
}

quint16 BareMetalDeviceConfigurationWizardSetupPage::gdbPort() const
{
    return quint16(m_portSpinBox->value());
}

QString BareMetalDeviceConfigurationWizardSetupPage::gdbResetCommands() const
{
    return m_gdbResetCommandsTextEdit->toPlainText().trimmed();
}

QString BareMetalDeviceConfigurationWizardSetupPage::gdbInitCommands() const
{
    return m_gdbInitCommandsPlainTextEdit->toPlainText().trimmed();
}

QString BareMetalDeviceConfigurationWizardSetupPage::defaultConfigurationName() const
{
    return tr("Bare Metal Device");
}

} // namespace Internal
} // namespace BareMetal
