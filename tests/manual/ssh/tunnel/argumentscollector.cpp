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

#include "argumentscollector.h"

#include <QDir>
#include <QProcessEnvironment>

#include <iostream>

using namespace QSsh;

using namespace std;

ArgumentsCollector::ArgumentsCollector(const QStringList &args)
    : m_arguments(args)
{
}

SshConnectionParameters ArgumentsCollector::collect(bool &success) const
{
    SshConnectionParameters parameters;
    parameters.options &= ~SshIgnoreDefaultProxy;
    parameters.host = QLatin1String("localhost");

    try {
        bool authTypeGiven = false;
        bool portGiven = false;
        bool timeoutGiven = false;
        bool proxySettingGiven = false;
        int pos;
        int port;

        for (pos = 1; pos < m_arguments.count() - 1; ++pos) {
            if (checkAndSetStringArg(pos, parameters.userName, "-u"))
                continue;
            if (checkAndSetIntArg(pos, port, portGiven, "-p")
                || checkAndSetIntArg(pos, parameters.timeout, timeoutGiven, "-t"))
                continue;
            if (checkAndSetStringArg(pos, parameters.password, "-pwd")) {
                if (!parameters.privateKeyFile.isEmpty())
                    throw ArgumentErrorException(QLatin1String("-pwd and -k are mutually exclusive."));
                parameters.authenticationType
                    = SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods;
                authTypeGiven = true;
                continue;
            }
            if (checkAndSetStringArg(pos, parameters.privateKeyFile, "-k")) {
                if (!parameters.password.isEmpty())
                    throw ArgumentErrorException(QLatin1String("-pwd and -k are mutually exclusive."));
                parameters.authenticationType
                    = SshConnectionParameters::AuthenticationTypePublicKey;
                authTypeGiven = true;
                continue;
            }
            if (!checkForNoProxy(pos, parameters.options, proxySettingGiven))
                throw ArgumentErrorException(QLatin1String("unknown option ") + m_arguments.at(pos));
        }

        Q_ASSERT(pos <= m_arguments.count());
        if (pos == m_arguments.count() - 1) {
            if (!checkForNoProxy(pos, parameters.options, proxySettingGiven))
                throw ArgumentErrorException(QLatin1String("unknown option ") + m_arguments.at(pos));
        }

        if (!authTypeGiven) {
            parameters.authenticationType = SshConnectionParameters::AuthenticationTypePublicKey;
            parameters.privateKeyFile = QDir::homePath() + QLatin1String("/.ssh/id_rsa");
        }

        if (parameters.userName.isEmpty()) {
            parameters.userName
                = QProcessEnvironment::systemEnvironment().value(QLatin1String("USER"));
        }
        if (parameters.userName.isEmpty())
            throw ArgumentErrorException(QLatin1String("No user name given."));

        if (parameters.host.isEmpty())
            throw ArgumentErrorException(QLatin1String("No host given."));

        parameters.port = portGiven ? port : 22;
        if (!timeoutGiven)
            parameters.timeout = 30;
        success = true;
    } catch (ArgumentErrorException &ex) {
        cerr << "Error: " << qPrintable(ex.error) << endl;
        printUsage();
        success = false;
    }
    return parameters;
}

void ArgumentsCollector::printUsage() const
{
    cerr << "Usage: " << qPrintable(m_arguments.first())
        << "[ -u <user> ] "
        << "[ -pwd <password> | -k <private key file> ] [ -p <port> ] "
        << "[ -t <timeout> ] [ -no-proxy ]" << endl;
}

bool ArgumentsCollector::checkAndSetStringArg(int &pos, QString &arg, const char *opt) const
{
    if (m_arguments.at(pos) == QLatin1String(opt)) {
        if (!arg.isEmpty()) {
            throw ArgumentErrorException(QLatin1String("option ") + QLatin1String(opt)
                + QLatin1String(" was given twice."));
        }
        arg = m_arguments.at(++pos);
        if (arg.isEmpty() && QLatin1String(opt) != QLatin1String("-pwd"))
            throw ArgumentErrorException(QLatin1String("empty argument not allowed here."));
        return true;
    }
    return false;
}

bool ArgumentsCollector::checkAndSetIntArg(int &pos, int &val,
    bool &alreadyGiven, const char *opt) const
{
    if (m_arguments.at(pos) == QLatin1String(opt)) {
        if (alreadyGiven) {
            throw ArgumentErrorException(QLatin1String("option ") + QLatin1String(opt)
                + QLatin1String(" was given twice."));
        }
        bool isNumber;
        val = m_arguments.at(++pos).toInt(&isNumber);
        if (!isNumber) {
            throw ArgumentErrorException(QLatin1String("option ") + QLatin1String(opt)
                 + QLatin1String(" needs integer argument"));
        }
        alreadyGiven = true;
        return true;
    }
    return false;
}

bool ArgumentsCollector::checkForNoProxy(int &pos, SshConnectionOptions &options,
                                         bool &alreadyGiven) const
{
    if (m_arguments.at(pos) == QLatin1String("-no-proxy")) {
        if (alreadyGiven)
            throw ArgumentErrorException(QLatin1String("proxy setting given twice."));
        options |= SshIgnoreDefaultProxy;
        alreadyGiven = true;
        return true;
    }
    return false;
}
