/* An example of ne7ssh library usage. Please change the values in connectWithPassword
function before compiling.

This will work with on a unix/linux server with cat utility.

If you are testing this with later openssh versions, make sure to add this
option to your server's configuration file to enable password authentication:

PasswordAuthentication yes
*/

#include <ne7ssh.h>

int main(int argc,char *argv[])
{
    int channel1;
    const char* result;
    ne7ssh *_ssh = new ne7ssh();
    int filesize = 0;
    FILE *testFi;

    // Set SSH connection options.
    _ssh->setOptions ("aes256-cbc", "hmac-md5");

    // Initiate connection without starting a remote shell.
    channel1 = _ssh->connectWithPassword ("remote-server", 22, "remoteUser", "password", 0);
    if (channel1 < 0)
    {
        const char *errmsg = _ssh->errors()->pop();
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Connection failed with last error: %s\n\n", errmsg);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // cat the remote file, works only on Unix systems. You may need to sepcifiy full path to cat.
    // Timeout after 100 seconds.

    if (!_ssh->sendCmd ("cat ~/test.bin", channel1, 100))
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Command failed with last error: %s\n\n", errmsg);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Determine the size of received file.
    filesize = _ssh->getReceivedSize(channel1);

    // Open a local file.
    testFi = fopen ("./test.bin", "wb");

    // Write binary data from the receive buffer to the opened file.
    if (!fwrite (_ssh->readBinary(channel1), (size_t) filesize, 1, testFi))
    {
        printf ("Error Writting to file\n\n");
        return EXIT_FAILURE;
    }

    // Close the files.
    fclose (testFi);

    // Destroy the instance.
    delete _ssh;

    return EXIT_SUCCESS;
}
