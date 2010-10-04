#ifndef SFTPTEST_H
#define SFTPTEST_H

#include "parameters.h"

#include <QtCore/QObject>

class SftpTest : public QObject
{
    Q_OBJECT
public:
    SftpTest(const Parameters &params);
    void run();

private:
    enum State { Inactive, Connecting, UploadingSmall, DownloadingSmall,
        RemovingSmall, UploadingBig, DownloadingBig, RemovingBig
    };
    const Parameters m_parameters;
    State m_state;
};


#endif // SFTPTEST_H
