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
#include "maemopublishinguploadsettingspagefremantlefree.h"
#include "ui_maemopublishinguploadsettingspagefremantlefree.h"

#include "maemopublisherfremantlefree.h"

#include <utils/pathchooser.h>

#include <QDir>

namespace Madde {
namespace Internal {

MaemoPublishingUploadSettingsPageFremantleFree::MaemoPublishingUploadSettingsPageFremantleFree(MaemoPublisherFremantleFree *publisher,
    QWidget *parent) :
    QWizardPage(parent),
    m_publisher(publisher),
    ui(new Ui::MaemoPublishingUploadSettingsPageFremantleFree)
{
    ui->setupUi(this);
    ui->serverAddressLabel->hide();
    ui->serverAddressLineEdit->hide();
    ui->targetDirectoryOnServerLabel->hide();
    ui->targetDirectoryOnServerLineEdit->hide();
    setTitle(tr("Publishing to Fremantle's \"Extras-devel/free\" Repository"));
    setSubTitle(tr("Upload options"));
    connect(ui->garageAccountLineEdit, SIGNAL(textChanged(QString)),
        SIGNAL(completeChanged()));
    connect(ui->privateKeyPathChooser, SIGNAL(changed(QString)),
        SIGNAL(completeChanged()));
    connect(ui->serverAddressLineEdit, SIGNAL(textChanged(QString)),
        SIGNAL(completeChanged()));
    connect(ui->targetDirectoryOnServerLineEdit, SIGNAL(textChanged(QString)),
        SIGNAL(completeChanged()));
}

MaemoPublishingUploadSettingsPageFremantleFree::~MaemoPublishingUploadSettingsPageFremantleFree()
{
    delete ui;
}

void MaemoPublishingUploadSettingsPageFremantleFree::initializePage()
{
    ui->garageAccountLineEdit->clear();
    ui->privateKeyPathChooser->setExpectedKind(Utils::PathChooser::File);
    ui->privateKeyPathChooser->setPromptDialogTitle(tr("Choose a private key file"));
    ui->privateKeyPathChooser->setPath(QDir::toNativeSeparators(QDir::homePath() + QLatin1String("/.ssh/id_rsa")));
    ui->serverAddressLineEdit->setText(QLatin1String("drop.maemo.org"));
    ui->targetDirectoryOnServerLineEdit->setText(QLatin1String("/var/www/extras-devel/incoming-builder/fremantle/"));
}

bool MaemoPublishingUploadSettingsPageFremantleFree::isComplete() const
{
    return !garageAccountName().isEmpty() && !privateKeyFilePath().isEmpty()
        && !serverName().isEmpty() && !directoryOnServer().isEmpty();
}

QString MaemoPublishingUploadSettingsPageFremantleFree::garageAccountName() const
{
    return ui->garageAccountLineEdit->text().trimmed();
}

QString MaemoPublishingUploadSettingsPageFremantleFree::privateKeyFilePath() const
{
    return ui->privateKeyPathChooser->path();
}

QString MaemoPublishingUploadSettingsPageFremantleFree::serverName() const
{
    return ui->serverAddressLineEdit->text().trimmed();
}

QString MaemoPublishingUploadSettingsPageFremantleFree::directoryOnServer() const
{
    return ui->targetDirectoryOnServerLineEdit->text().trimmed();
}

bool MaemoPublishingUploadSettingsPageFremantleFree::validatePage()
{
    m_publisher->setSshParams(serverName(), garageAccountName(),
        privateKeyFilePath(), directoryOnServer());
    return true;
}

} // namespace Internal
} // namespace Madde
