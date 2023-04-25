// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "authenticationdialog.h"

#include "mercurialtr.h"

#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>
#include <QLineEdit>

namespace Mercurial::Internal {

AuthenticationDialog::AuthenticationDialog(const QString &username, const QString &password, QWidget *parent)
    : QDialog(parent)
{
    resize(312, 116);

    m_username = new QLineEdit(username);

    m_password = new QLineEdit(password);
    m_password->setEchoMode(QLineEdit::Password);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

    Column {
        Form {
            Tr::tr("Username:"), m_username, br,
            Tr::tr("Password:"), m_password
        },
        buttonBox
    }.attachTo(this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AuthenticationDialog::~AuthenticationDialog() = default;

void AuthenticationDialog::setPasswordEnabled(bool enabled)
{
    m_password->setEnabled(enabled);
}

QString AuthenticationDialog::getUserName()
{
    return m_username->text();
}

QString AuthenticationDialog::getPassword()
{
    return m_password->text();
}

} // Mercurial::Internal
