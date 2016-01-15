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

#include "proxycredentialsdialog.h"
#include "ui_proxycredentialsdialog.h"

#include <utils/networkaccessmanager.h>
#include <QNetworkProxy>

using namespace Utils;

/*!
    \class Utils::ProxyCredentialsDialog

    Dialog for asking the user about proxy credentials (username, password).
*/

ProxyCredentialsDialog::ProxyCredentialsDialog(const QNetworkProxy &proxy, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProxyCredentialsDialog)
{
    ui->setupUi(this);

    setUserName(proxy.user());
    setPassword(proxy.password());

    const QString proxyString = QString::fromLatin1("%1:%2").arg(proxy.hostName()).arg(proxy.port());
    ui->infotext->setText(ui->infotext->text().arg(proxyString));
}

ProxyCredentialsDialog::~ProxyCredentialsDialog()
{
    delete ui;
}

QString ProxyCredentialsDialog::userName() const
{
    return ui->usernameLineEdit->text();
}

void ProxyCredentialsDialog::setUserName(const QString &username)
{
    ui->usernameLineEdit->setText(username);
}

QString ProxyCredentialsDialog::password() const
{
    return ui->passwordLineEdit->text();
}

void ProxyCredentialsDialog::setPassword(const QString &passwd)
{
    ui->passwordLineEdit->setText(passwd);
}

