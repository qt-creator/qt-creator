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

#include "authenticationdialog.h"
#include "ui_authenticationdialog.h"
#include "gerritparameters.h"

#include <utils/asconst.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextStream>

namespace Gerrit {
namespace Internal {

static QRegularExpressionMatch entryMatch(const QString &line, const QString &type)
{
    const QRegularExpression regexp("(?:^|\\s)" + type + "\\s(\\S+)");
    return regexp.match(line);
}

static QString findEntry(const QString &line, const QString &type)
{
    const QRegularExpressionMatch match = entryMatch(line, type);
    if (match.hasMatch())
        return match.captured(1);
    return QString();
}

static bool replaceEntry(QString &line, const QString &type, const QString &value)
{
    const QRegularExpressionMatch match = entryMatch(line, type);
    if (!match.hasMatch())
        return false;
    line.replace(match.capturedStart(1), match.capturedLength(1), value);
    return true;
}

AuthenticationDialog::AuthenticationDialog(GerritServer *server) :
    ui(new Ui::AuthenticationDialog),
    m_server(server)
{
    ui->setupUi(this);
    ui->descriptionLabel->setText(ui->descriptionLabel->text().replace(
                                      "LINK_PLACEHOLDER", server->url() + "/#/settings/http-password"));
    ui->descriptionLabel->setOpenExternalLinks(true);
    ui->serverLineEdit->setText(server->host);
    ui->userLineEdit->setText(server->user.userName);
    m_netrcFileName = QLatin1String(Utils::HostOsInfo::isWindowsHost() ? "_netrc" : ".netrc");
    readExistingConf();

    QPushButton *anonymous = ui->buttonBox->addButton(tr("Anonymous"), QDialogButtonBox::AcceptRole);
    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, [this, anonymous](QAbstractButton *button) {
        if (button == anonymous)
            m_authenticated = false;
    });
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false);
    connect(ui->passwordLineEdit, &QLineEdit::editingFinished, this, [this, server, okButton] {
        setupCredentials();
        const int result = server->testConnection();
        okButton->setEnabled(result == 200);
    });
}

AuthenticationDialog::~AuthenticationDialog()
{
    delete ui;
}

void AuthenticationDialog::readExistingConf()
{
    QFile netrcFile(QDir::homePath() + '/' + m_netrcFileName);
    if (!netrcFile.open(QFile::ReadOnly | QFile::Text))
        return;

    QTextStream stream(&netrcFile);
    QString line;
    while (stream.readLineInto(&line)) {
        m_allMachines << line;
        const QString machine = findEntry(line, "machine");
        if (machine == m_server->host) {
            const QString login = findEntry(line, "login");
            const QString password = findEntry(line, "password");
            if (!login.isEmpty())
                ui->userLineEdit->setText(login);
            if (!password.isEmpty())
                ui->passwordLineEdit->setText(password);
        }
    }
    netrcFile.close();
}

bool AuthenticationDialog::setupCredentials()
{
    QString netrcContents;
    QTextStream out(&netrcContents);
    bool found = false;
    const QString user = ui->userLineEdit->text().trimmed();
    const QString password = ui->passwordLineEdit->text().trimmed();
    for (QString &line : m_allMachines) {
        const QString machine = findEntry(line, "machine");
        if (machine == m_server->host) {
            found = true;
            replaceEntry(line, "login", user);
            replaceEntry(line, "password", password);
        }
        out << line << endl;
    }
    if (!found)
        out << "machine " << m_server->host << " login " << user << " password " << password;
    Utils::FileSaver saver(m_netrcFileName, QFile::WriteOnly | QFile::Truncate | QFile::Text);
    saver.write(netrcContents.toUtf8());
    return saver.finalize();
}

} // Internal
} // Gerrit
