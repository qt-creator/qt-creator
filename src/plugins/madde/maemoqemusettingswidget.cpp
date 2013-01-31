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

#include "maemoqemusettingswidget.h"
#include "ui_maemoqemusettingswidget.h"

#include "maemoqemusettings.h"

namespace Madde {
namespace Internal {

MaemoQemuSettingsWidget::MaemoQemuSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MaemoQemuSettingsWidget)
{
    ui->setupUi(this);
    switch (MaemoQemuSettings::openGlMode()) {
    case MaemoQemuSettings::HardwareAcceleration:
        ui->hardwareAccelerationButton->setChecked(true);
        break;
    case MaemoQemuSettings::SoftwareRendering:
        ui->softwareRenderingButton->setChecked(true);
        break;
    case MaemoQemuSettings::AutoDetect:
        ui->autoDetectButton->setChecked(true);
        break;
    }
}

MaemoQemuSettingsWidget::~MaemoQemuSettingsWidget()
{
    delete ui;
}

QString MaemoQemuSettingsWidget::keywords() const
{
    const QChar space = QLatin1Char(' ');
    QString keywords = ui->groupBox->title() + space
        + ui->hardwareAccelerationButton->text() + space
        + ui->softwareRenderingButton->text() + space
        + ui->autoDetectButton->text();
    keywords.remove(QLatin1Char('&'));
    return keywords;
}

void MaemoQemuSettingsWidget::saveSettings()
{
    const MaemoQemuSettings::OpenGlMode openGlMode
        = ui->hardwareAccelerationButton->isChecked()
            ? MaemoQemuSettings::HardwareAcceleration
            : ui->softwareRenderingButton->isChecked()
                  ? MaemoQemuSettings::SoftwareRendering
                  : MaemoQemuSettings::AutoDetect;
    MaemoQemuSettings::setOpenGlMode(openGlMode);
}

} // namespace Internal
} // namespace Madde
