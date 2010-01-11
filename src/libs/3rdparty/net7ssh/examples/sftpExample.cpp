/* An example of ne7ssh library usage. Please change the values in connectWithPassword 
   function before compiling.
   
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
    Ne7SftpSubsystem _sftp;
    const char* dirList;

    // Set SSH connection options.
    _ssh->setOptions ("aes256-cbc", "hmac-md5");

    // Initiate connection without starting a remote shell.
    channel1 = _ssh->connectWithPassword ("remoteHost", 22, "remoteUsr", "password", 0, 20);
    if (channel1 < 0)
    {
        const char *errmsg = _ssh->errors()->pop();
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Connection failed with last error: %s\n\n", errmsg);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Initiate SFTP subsystem.
    if (!_ssh->initSftp (_sftp, channel1))
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Command failed with last error: %s\n\n", errmsg);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Set a timeout for all SFTP communications.
    _sftp.setTimeout (30);

    // Check remote file permissions.
    Ne7SftpSubsystem::fileAttrs attrs;
    if (_sftp.getFileAttrs (attrs, "test.bin", true))
        printf ("Permissions: %o\n", (attrs.permissions & 0777));

    // Create a local file.
    testFi = fopen ("test.bin", "wb+");
    if (!testFi)
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Could not open local file: %s\n\n", errmsg);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Download a file.
    if (!_sftp.get ("test.bin", testFi))
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Could not get File: %s\n\n", errmsg);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Change directory.
    if (!_sftp.cd ("testing"))
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Could not change Directory: %s\n\n", errmsg);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Upload the file.
    if (!_sftp.put (testFi, "test2.bin"))
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Could not put File: %s\n\n", errmsg);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Create a new directory.
    if (!_sftp.mkdir ("testing3"))
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Could not create Directory: %s\n\n", errmsg);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Get listing.
    dirList = _sftp.ls (".", true);
    if (!dirList)
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Could not list Directory: %s\n\n", errmsg);
        delete _ssh;
        return EXIT_FAILURE;
    }
    else
        printf ("Directory Listing:\n\n %s\n", dirList);

    // Change permisions on newly uploaded file.
    if (!_sftp.chmod ("test2.bin", "755"))
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Could not chmod: %s\n\n", errmsg);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Destroy the instance.
    delete _ssh;

    return EXIT_SUCCESS;
}
