/****************************************************************************
**
** Copyright (C) 2013 Tim Sander <tim@krieglstein.org>
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

#include "ui_baremetaldeviceconfigurationwidget.h"

#include <ssh/sshconnection.h>
#include <QLabel>

using namespace QSsh;

namespace BareMetal {

BareMetalDeviceConfigurationWidget::BareMetalDeviceConfigurationWidget(
        const ProjectExplorer::IDevice::Ptr &deviceConfig, QWidget *parent) :
   IDeviceWidget(deviceConfig, parent),
   m_ui(new Ui::BareMetalDeviceConfigurationWidget)
{
    m_ui->setupUi(this);
    connect(m_ui->gdbHostLineEdit, SIGNAL(editingFinished()), SLOT(hostnameChanged()));
    connect(m_ui->gdbPortSpinBox, SIGNAL(valueChanged(int)), SLOT(portChanged()));
    connect(m_ui->gdbCommandsTextEdit, SIGNAL(textChanged()), SLOT(gdbInitCommandsChanged()));
    initGui();
}

BareMetalDeviceConfigurationWidget::~BareMetalDeviceConfigurationWidget()
{
    delete m_ui;
}

/* using sshParams fields is ugly but otherwise i would have needed to write my own fromMap
 * toMap functions */

void BareMetalDeviceConfigurationWidget::hostnameChanged()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.host = m_ui->gdbHostLineEdit->text().trimmed();
    device()->setSshParameters(sshParams);
}

void BareMetalDeviceConfigurationWidget::portChanged()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.port = m_ui->gdbPortSpinBox->value();
    device()->setSshParameters(sshParams);
}

void BareMetalDeviceConfigurationWidget::gdbInitCommandsChanged()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.userName = m_ui->gdbCommandsTextEdit->toPlainText();
    device()->setSshParameters(sshParams);
}

void BareMetalDeviceConfigurationWidget::updateDeviceFromUi() {
    hostnameChanged();
    portChanged();
    gdbInitCommandsChanged();
}

void BareMetalDeviceConfigurationWidget::initGui()
{
    //FIXME reusing SshConnectionParameters is kind of ugly?
    SshConnectionParameters sshParams = device()->sshParameters();
    m_ui->gdbHostLineEdit->setText(sshParams.host);
    m_ui->gdbPortSpinBox->setValue(sshParams.port);
    m_ui->gdbCommandsTextEdit->setPlainText(sshParams.userName);
}

} //namespace BareMetal
