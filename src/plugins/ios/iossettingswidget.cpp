/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
      m_ui(new Ui_IosSettingsWidget),
      m_iosConfig(IosConfigurations::instance().config()),
      m_saveSettingsRequested(false)
{
    initGui();
}

IosSettingsWidget::~IosSettingsWidget()
{
    if (m_saveSettingsRequested)
        IosConfigurations::instance().setConfig(m_iosConfig);
    delete m_ui;
}

QString IosSettingsWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui->developerPathLabel->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

void IosSettingsWidget::initGui()
{
    m_ui->setupUi(this);
    m_ui->developerPathLineEdit->setText(m_iosConfig.developerPath.toUserOutput());
    m_ui->deviceAskCheckBox->setChecked(!m_iosConfig.ignoreAllDevices);
    connect(m_ui->developerPathLineEdit, SIGNAL(editingFinished()),
            SLOT(developerPathEditingFinished()));
    connect(m_ui->developerPathPushButton, SIGNAL(clicked()),
            SLOT(browseDeveloperPath()));
    connect(m_ui->deviceAskCheckBox, SIGNAL(toggled(bool)),
            SLOT(deviceAskToggled(bool)));
}

void IosSettingsWidget::deviceAskToggled(bool checkboxValue)
{
    m_iosConfig.ignoreAllDevices = !checkboxValue;
    saveSettings(true);
}

void IosSettingsWidget::saveSettings(bool saveNow)
{
    // We must defer this step because of a stupid bug on MacOS. See QTCREATORBUG-1675.
    if (saveNow) {
        IosConfigurations::instance().setConfig(m_iosConfig);
        m_saveSettingsRequested = false;
    } else {
        m_saveSettingsRequested = true;
    }
}

void IosSettingsWidget::developerPathEditingFinished()
{
    Utils::FileName basePath = Utils::FileName::fromUserInput(m_ui->developerPathLineEdit->text());
    // auto extend Contents/Developer if required
    Utils::FileName devDir = basePath;
    devDir.appendPath(QLatin1String("Contents/Developer"));
    if (devDir.toFileInfo().isDir())
        m_iosConfig.developerPath = devDir;
    else
        m_iosConfig.developerPath = basePath;
    m_ui->developerPathLineEdit->setText(m_iosConfig.developerPath.toUserOutput());
    saveSettings(true);
}

void IosSettingsWidget::browseDeveloperPath()
{
    Utils::FileName dir = Utils::FileName::fromString(
                QFileDialog::getOpenFileName(this,
                                             tr("Select Xcode application"),
                                             QLatin1String("/Applications"), QLatin1String("*.app")));
    m_ui->developerPathLineEdit->setText(dir.toUserOutput());
    developerPathEditingFinished();
}

} // namespace Internal
} // namespace Ios
