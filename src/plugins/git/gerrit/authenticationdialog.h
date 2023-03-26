// Copyright (C) 2017 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QCoreApplication>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QLineEdit;
class QTimer;
QT_END_NAMESPACE

namespace Gerrit {
namespace Internal {

class GerritServer;

class AuthenticationDialog : public QDialog
{
public:
    AuthenticationDialog(GerritServer *server);
    ~AuthenticationDialog() override;
    bool isAuthenticated() const { return m_authenticated; }

private:
    void readExistingConf();
    bool setupCredentials();
    void checkCredentials();

    GerritServer *m_server = nullptr;
    QString m_netrcFileName;
    QStringList m_allMachines;
    bool m_authenticated = true;
    QTimer *m_checkTimer = nullptr;
    QLineEdit *m_userLineEdit;
    QLineEdit *m_passwordLineEdit;
    QDialogButtonBox *m_buttonBox;
};

} // Internal
} // Gerrit
