// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace Mercurial {
namespace Internal {

namespace Ui { class AuthenticationDialog; }

class AuthenticationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AuthenticationDialog(const QString &username, const QString &password,
                                  QWidget *parent = nullptr);
    ~AuthenticationDialog() override;
    void setPasswordEnabled(bool enabled);
    QString getUserName();
    QString getPassword();

private:
    Ui::AuthenticationDialog *ui;
};

} // namespace Internal
} // namespace Mercurial
