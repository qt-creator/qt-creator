/****************************************************************************
**
** Copyright (C) 2017 Orgad Shaneh <orgads@gmail.com>.
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

#ifndef AUTHENTICATIONDIALOG_H
#define AUTHENTICATIONDIALOG_H

#include <QCoreApplication>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace Gerrit {
namespace Internal {

class GerritServer;

namespace Ui { class AuthenticationDialog; }

class AuthenticationDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(Gerrit::Internal::AuthenticationDialog)

public:
    AuthenticationDialog(GerritServer *server);
    ~AuthenticationDialog();
    bool isAuthenticated() const { return m_authenticated; }

private:
    void readExistingConf();
    bool setupCredentials();
    void checkCredentials();
    Ui::AuthenticationDialog *ui = nullptr;
    GerritServer *m_server = nullptr;
    QString m_netrcFileName;
    QStringList m_allMachines;
    bool m_authenticated = true;
    QTimer *m_checkTimer = nullptr;
};

} // Internal
} // Gerrit

#endif // AUTHENTICATIONDIALOG_H
