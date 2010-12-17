/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "maemopublishinguploadsettingspagefremantlefree.h"
#include "ui_maemopublishinguploadsettingspagefremantlefree.h"

#include "maemopublisherfremantlefree.h"

#include <utils/pathchooser.h>

#include <QtCore/QDir>

namespace Qt4ProjectManager {
namespace Internal {

MaemoPublishingUploadSettingsPageFremantleFree::MaemoPublishingUploadSettingsPageFremantleFree(MaemoPublisherFremantleFree *publisher,
    QWidget *parent) :
    QWizardPage(parent),
    m_publisher(publisher),
    ui(new Ui::MaemoPublishingUploadSettingsPageFremantleFree)
{
    ui->setupUi(this);
    setTitle(tr("Publishing to Fremantle's \"Extras-devel/free\" Repository"));
    setSubTitle(tr("Upload options"));
    ui->privateKeyPathChooser->setExpectedKind(Utils::PathChooser::File);
    ui->privateKeyPathChooser->setPromptDialogTitle(tr("Choose a private key file"));
    ui->privateKeyPathChooser->setPath(QDir::toNativeSeparators(QDir::homePath() + QLatin1String("/.ssh/id_rsa")));
    ui->serverAddressLineEdit->setText(QLatin1String("drop.maemo.org"));
    ui->targetDirectoryOnServerLineEdit->setText(QLatin1String("/var/www/extras-devel/incoming-builder/fremantle/"));
}

MaemoPublishingUploadSettingsPageFremantleFree::~MaemoPublishingUploadSettingsPageFremantleFree()
{
    delete ui;
}

bool MaemoPublishingUploadSettingsPageFremantleFree::validatePage()
{
    m_publisher->setSshParams(ui->serverAddressLineEdit->text(),
        ui->garageAccountLineEdit->text(), ui->privateKeyPathChooser->path(),
        ui->targetDirectoryOnServerLineEdit->text());
    return true;
}

} // namespace Internal
} // namespace Qt4ProjectManager
