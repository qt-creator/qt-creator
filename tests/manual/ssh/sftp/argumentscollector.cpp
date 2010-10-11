#include "argumentscollector.h"

#include <iostream>

using namespace std;
using namespace Core;

ArgumentsCollector::ArgumentsCollector(const QStringList &args)
    : m_arguments(args)
{
}

Parameters ArgumentsCollector::collect(bool &success) const
{
    Parameters parameters;
    try {
        bool authTypeGiven = false;
        bool portGiven = false;
        bool timeoutGiven = false;
        bool smallFileCountGiven = false;
        bool bigFileSizeGiven = false;
        bool proxySettingGiven = false;
        int pos;
        int port;
        for (pos = 1; pos < m_arguments.count() - 1; ++pos) {
            if (checkAndSetStringArg(pos, parameters.sshParams.host, "-h")
                || checkAndSetStringArg(pos, parameters.sshParams.uname, "-u"))
                continue;
            if (checkAndSetIntArg(pos, port, portGiven, "-p")
                || checkAndSetIntArg(pos, parameters.sshParams.timeout, timeoutGiven, "-t")
                || checkAndSetIntArg(pos, parameters.smallFileCount, smallFileCountGiven, "-c")
                || checkAndSetIntArg(pos, parameters.bigFileSize, bigFileSizeGiven, "-s"))
                continue;
            if (checkAndSetStringArg(pos, parameters.sshParams.pwd, "-pwd")) {
                if (!parameters.sshParams.privateKeyFile.isEmpty())
                    throw ArgumentErrorException(QLatin1String("-pwd and -k are mutually exclusive."));
                parameters.sshParams.authType
                    = SshConnectionParameters::AuthByPwd;
                authTypeGiven = true;
                continue;
            }
            if (checkAndSetStringArg(pos, parameters.sshParams.privateKeyFile, "-k")) {
                if (!parameters.sshParams.pwd.isEmpty())
                    throw ArgumentErrorException(QLatin1String("-pwd and -k are mutually exclusive."));
                parameters.sshParams.authType
                    = SshConnectionParameters::AuthByKey;
                authTypeGiven = true;
                continue;
            }
            if (!checkForNoProxy(pos, parameters.sshParams.proxyType, proxySettingGiven))
                throw ArgumentErrorException(QLatin1String("unknown option ") + m_arguments.at(pos));
        }

        Q_ASSERT(pos <= m_arguments.count());
        if (pos == m_arguments.count() - 1) {
            if (!checkForNoProxy(pos, parameters.sshParams.proxyType, proxySettingGiven))
                throw ArgumentErrorException(QLatin1String("unknown option ") + m_arguments.at(pos));
        }

        if (!authTypeGiven)
            throw ArgumentErrorException(QLatin1String("No authentication argument given."));
        if (parameters.sshParams.host.isEmpty())
            throw ArgumentErrorException(QLatin1String("No host given."));
        if (parameters.sshParams.uname.isEmpty())
            throw ArgumentErrorException(QLatin1String("No user name given."));

        parameters.sshParams.port = portGiven ? port : 22;
        if (!timeoutGiven)
            parameters.sshParams.timeout = 30;
        if (!smallFileCountGiven)
            parameters.smallFileCount = 1000;
        if (!bigFileSizeGiven)
            parameters.bigFileSize = 1024;
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
        << " -h <host> -u <user> "
        << "-pwd <password> | -k <private key file> [ -p <port> ] "
        << "[ -t <timeout> ] [ -c <small file count> ] "
        << "[ -s <big file size in MB> ] [ -no-proxy ]" << endl;
}

bool ArgumentsCollector::checkAndSetStringArg(int &pos, QString &arg, const char *opt) const
{
    if (m_arguments.at(pos) == QLatin1String(opt)) {
        if (!arg.isEmpty()) {
            throw ArgumentErrorException(QLatin1String("option ") + opt
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
            throw ArgumentErrorException(QLatin1String("option ") + opt
                + QLatin1String(" was given twice."));
        }
        bool isNumber;
        val = m_arguments.at(++pos).toInt(&isNumber);
        if (!isNumber) {
            throw ArgumentErrorException(QLatin1String("option ") + opt
                 + QLatin1String(" needs integer argument"));
        }
        alreadyGiven = true;
        return true;
    }
    return false;
}

bool ArgumentsCollector::checkForNoProxy(int &pos,
    SshConnectionParameters::ProxyType &type, bool &alreadyGiven) const
{
    if (m_arguments.at(pos) == QLatin1String("-no-proxy")) {
        if (alreadyGiven)
            throw ArgumentErrorException(QLatin1String("proxy setting given twice."));
        type = SshConnectionParameters::NoProxy;
        alreadyGiven = true;
        return true;
    }
    return false;
}
