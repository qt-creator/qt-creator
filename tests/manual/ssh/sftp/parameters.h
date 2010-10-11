#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <coreplugin/ssh/sshconnection.h>

struct Parameters {
    Parameters() : sshParams(Core::SshConnectionParameters::DefaultProxy) {}

    Core::SshConnectionParameters sshParams;
    int smallFileCount;
    int bigFileSize;
};

#endif // PARAMETERS_H
