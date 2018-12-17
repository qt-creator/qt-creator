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
    try {
        bool authTypeGiven = false;
        bool portGiven = false;
        bool timeoutGiven = false;
        int pos;
        int port = 22;

        for (pos = 1; pos < m_arguments.count() - 1; ++pos) {
            QString str;
            if (checkAndSetStringArg(pos, str, "-h")) {
                parameters.setHost(str);
            } else if (checkAndSetStringArg(pos, str, "-u")) {
                parameters.setUserName(str);
            } else if (checkAndSetIntArg(pos, port, portGiven, "-p")
                       || checkAndSetIntArg(pos, parameters.timeout, timeoutGiven, "-t")) {
                continue;
            } else  if (checkAndSetStringArg(pos, parameters.privateKeyFile, "-k")) {
                parameters.authenticationType
                        = SshConnectionParameters::AuthenticationTypeSpecificKey;
                authTypeGiven = true;
            }
        }

        Q_ASSERT(pos <= m_arguments.count());

        if (!authTypeGiven)
            parameters.authenticationType = SshConnectionParameters::AuthenticationTypeAll;

        if (parameters.userName().isEmpty())
            parameters.setUserName(QProcessEnvironment::systemEnvironment().value("USER"));
        if (parameters.userName().isEmpty())
            throw ArgumentErrorException(QLatin1String("No user name given."));

        if (parameters.host().isEmpty())
            throw ArgumentErrorException(QLatin1String("No host given."));

        parameters.setPort(portGiven ? port : 22);
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
        << " -h <host> [ -u <user> ] "
        << "[ -k <private key file> ] [ -p <port> ] "
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
