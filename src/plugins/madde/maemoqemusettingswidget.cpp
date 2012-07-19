/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
