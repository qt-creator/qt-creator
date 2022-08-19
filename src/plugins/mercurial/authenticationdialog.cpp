// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "authenticationdialog.h"
#include "ui_authenticationdialog.h"

namespace Mercurial {
namespace Internal  {

AuthenticationDialog::AuthenticationDialog(const QString &username, const QString &password, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AuthenticationDialog)
{
    ui->setupUi(this);
    ui->username->setText(username);
    ui->password->setText(password);
}

AuthenticationDialog::~AuthenticationDialog()
{
    delete ui;
}

void AuthenticationDialog::setPasswordEnabled(bool enabled)
{
    ui->password->setEnabled(enabled);
}

QString AuthenticationDialog::getUserName()
{
    return ui->username->text();
}

QString AuthenticationDialog::getPassword()
{
    return ui->password->text();
}

} // namespace Internal
} // namespace Mercurial
