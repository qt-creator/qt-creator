// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "proxycredentialsdialog.h"
#include "ui_proxycredentialsdialog.h"

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

