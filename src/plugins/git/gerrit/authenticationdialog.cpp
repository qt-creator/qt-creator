// Copyright (C) 2017 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "authenticationdialog.h"

#include "gerritserver.h"

#include "../gittr.h"

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

#include <QClipboard>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextStream>
#include <QTimer>

using namespace Utils;

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
    return {};
}

static bool replaceEntry(QString &line, const QString &type, const QString &value)
{
    const QRegularExpressionMatch match = entryMatch(line, type);
    if (!match.hasMatch())
        return false;
    line.replace(match.capturedStart(1), match.capturedLength(1), value);
    return true;
}

AuthenticationDialog::AuthenticationDialog(GerritServer *server)
        : m_server(server)
{
    setWindowTitle(Git::Tr::tr("Authentication"));
    resize(400, 334);

    // FIXME: Take html out of this translatable string.
    const QString desc = Git::Tr::tr(
        "<html><head/><body><p>Gerrit server with HTTP was detected, but you need "
        "to set up credentials for it.</p><p>To get your password, "
        "<a href=\"LINK_PLACEHOLDER\"><span style=\" text-decoration: "
        "underline; color:#007af4;\">click here</span></a> (sign in if needed). "
        "Click Generate Password if the password is blank, and copy the user name "
        "and password to this form.</p><p>Choose Anonymous if you do not want "
        "authentication for this server. In this case, changes that require "
        "authentication (like draft changes or private projects) will not be displayed."
        "</p></body></html>")
        .replace("LINK_PLACEHOLDER", server->url() + "/#/settings/#HTTPCredentials");

    auto descriptionLabel = new QLabel(desc);
    descriptionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    descriptionLabel->setTextFormat(Qt::RichText);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setOpenExternalLinks(true);

    m_userLineEdit = new QLineEdit(server->user.userName);

    m_passwordLineEdit = new QLineEdit;

    auto serverLineEdit = new QLineEdit(server->host);
    serverLineEdit->setEnabled(false);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    m_netrcFileName = QDir::homePath() + '/' +
            QLatin1String(Utils::HostOsInfo::isWindowsHost() ? "_netrc" : ".netrc");

    using namespace Layouting;
    Column {
        descriptionLabel,
        Form {
            Git::Tr::tr("Server:"), serverLineEdit, br,
            Git::Tr::tr("&User:"), m_userLineEdit, br,
            Git::Tr::tr("&Password:"), m_passwordLineEdit, br,
        },
        m_buttonBox,
    }.attachTo(this);

    readExistingConf();

    QPushButton *anonymous = m_buttonBox->addButton(Git::Tr::tr("Anonymous"), QDialogButtonBox::AcceptRole);
    connect(m_buttonBox, &QDialogButtonBox::clicked,
            this, [this, anonymous](QAbstractButton *button) {
        if (button == anonymous)
            m_authenticated = false;
    });
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(m_passwordLineEdit, &QLineEdit::editingFinished,
            this, &AuthenticationDialog::checkCredentials);
    m_checkTimer = new QTimer(this);
    m_checkTimer->setSingleShot(true);
    connect(m_checkTimer, &QTimer::timeout, this, &AuthenticationDialog::checkCredentials);
    connect(m_passwordLineEdit, &QLineEdit::textChanged, this, [this] {
        if (QGuiApplication::clipboard()->text() == m_passwordLineEdit->text()) {
            checkCredentials();
            return;
        }

        m_checkTimer->start(2000);
    });
    if (!m_userLineEdit->text().isEmpty())
        m_passwordLineEdit->setFocus();

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AuthenticationDialog::~AuthenticationDialog() = default;

void AuthenticationDialog::readExistingConf()
{
    QFile netrcFile(m_netrcFileName);
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
                m_userLineEdit->setText(login);
            if (!password.isEmpty())
                m_passwordLineEdit->setText(password);
        }
    }
    netrcFile.close();
}

bool AuthenticationDialog::setupCredentials()
{
    QString netrcContents;
    QTextStream out(&netrcContents);
    bool found = false;
    const QString user = m_userLineEdit->text().trimmed();
    const QString password = m_passwordLineEdit->text().trimmed();

    if (user.isEmpty() || password.isEmpty())
        return false;

    m_server->user.userName = user;
    for (QString &line : m_allMachines) {
        const QString machine = findEntry(line, "machine");
        if (machine == m_server->host) {
            found = true;
            replaceEntry(line, "login", user);
            replaceEntry(line, "password", password);
        }
        out << line << '\n';
    }
    if (!found)
        out << "machine " << m_server->host << " login " << user << " password " << password << '\n';
    Utils::FileSaver saver(Utils::FilePath::fromString(m_netrcFileName),
                           QFile::WriteOnly | QFile::Truncate | QFile::Text);
    saver.write(netrcContents.toUtf8());
    return saver.finalize();
}

void AuthenticationDialog::checkCredentials()
{
    int result = 400;
    if (setupCredentials())
        result = m_server->testConnection();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(result == 200);
}

} // Internal
} // Gerrit
