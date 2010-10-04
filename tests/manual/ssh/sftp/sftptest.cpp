#include "sftptest.h"

SftpTest::SftpTest(const Parameters &params)
    : m_parameters(params), m_state(Inactive)
{

}

void SftpTest::run()
{
// connect to remote host
// create sftp connection
// create n 1KB files
// upload these files
// download these files
// compare the original to the downloaded files
// remove local files
// remove remote files
// then the same for a big N GB file
}
