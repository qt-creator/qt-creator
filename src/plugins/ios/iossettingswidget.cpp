/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "iossettingswidget.h"

#include "ui_iossettingswidget.h"

#include "iosconfigurations.h"
#include "iosconstants.h"

#include <utils/hostosinfo.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <QFile>
#include <QTextStream>
#include <QProcess>

#include <QFileDialog>
#include <QMessageBox>
#include <QModelIndex>

namespace Ios {
namespace Internal {

IosSettingsWidget::IosSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_ui(new Ui::IosSettingsWidget),
      m_saveSettingsRequested(false)
{
    initGui();
}

IosSettingsWidget::~IosSettingsWidget()
{
    delete m_ui;
}

void IosSettingsWidget::initGui()
{
    m_ui->setupUi(this);
    m_ui->deviceAskCheckBox->setChecked(!IosConfigurations::ignoreAllDevices());
}

void IosSettingsWidget::saveSettings()
{
    IosConfigurations::setIgnoreAllDevices(!m_ui->deviceAskCheckBox->isChecked());
}

} // namespace Internal
} // namespace Ios
