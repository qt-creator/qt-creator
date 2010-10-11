#ifndef ARGUMENTSCOLLECTOR_H
#define ARGUMENTSCOLLECTOR_H

#include "parameters.h"

#include <QtCore/QStringList>

class ArgumentsCollector
{
public:
    ArgumentsCollector(const QStringList &args);
    Parameters collect(bool &success) const;
private:
    struct ArgumentErrorException
    {
        ArgumentErrorException(const QString &error) : error(error) {}
        const QString error;
    };

    void printUsage() const;
    bool checkAndSetStringArg(int &pos, QString &arg, const char *opt) const;
    bool checkAndSetIntArg(int &pos, int &val, bool &alreadyGiven,
        const char *opt) const;
    bool checkForNoProxy(int &pos,
        Core::SshConnectionParameters::ProxyType &type,
        bool &alreadyGiven) const;

    const QStringList m_arguments;
};

#endif // ARGUMENTSCOLLECTOR_H
