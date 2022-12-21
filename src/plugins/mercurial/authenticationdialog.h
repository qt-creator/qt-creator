// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace Mercurial::Internal {

class AuthenticationDialog : public QDialog
{
public:
    explicit AuthenticationDialog(const QString &username, const QString &password,
                                  QWidget *parent = nullptr);
    ~AuthenticationDialog() override;

    void setPasswordEnabled(bool enabled);
    QString getUserName();
    QString getPassword();

private:
    QLineEdit *m_username;
    QLineEdit *m_password;
};

} // Mercurial::Internal
