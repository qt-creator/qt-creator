/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
