/* An example of ne7ssh library usage. Please change the values in connectWitKeys 
   function before compiling.
   
   This will work with openssh server if default shell of authenticating user is bash.
   When using a different shell or custom prompt replace " $" string in waitFor() 
   method with a string corresponding with your shell prompt.
   
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

    // Set SSH connection options.
    _ssh->setOptions ("aes128-cbc", "hmac-sha1");

    // Initiate connection.
    channel1 = _ssh->connectWithKey ("remoteHost", 22, "remoteUsr", "/local/path/to/privKeyFile");
    if (channel1 < 0)
    {
        const char *errmsg = _ssh->errors()->pop();
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Connection failed with last error: %s\n\n", errmsg);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Wait for bash prompt, or die in 5 seconds.
    if (!_ssh->waitFor (channel1, "$", 5))
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Failed while waiting for remote shell wiht last error: %s\n\n", errmsg);
        _ssh->close(channel1);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Send "ls" command.
    if (!_ssh->send ("ls\n", channel1))
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Could not send the command. Last error: %s\n\n", errmsg);
        _ssh->close(channel1);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Wait for bash prompt, or die in 5 seconds
    if (!_ssh->waitFor (channel1, "$", 5))
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Timeout while waiting for remote site.Last error: %s\n\n", errmsg);
        _ssh->close(channel1);
        delete _ssh;
        return EXIT_FAILURE;
    }

    // Fetch recieved data.
    result = _ssh->read (channel1);

    if (!result)
    {
        const char *errmsg = _ssh->errors()->pop(channel1);
        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("No data received. Last error: %s\n\n", errmsg);
    }
    else
        printf ("Received data:\n %s\n", result);

    // Terminate connection by sending "exit" command.
    _ssh->send ("exit\n", channel1);

    // Destroy the instance.
    delete _ssh;
    return EXIT_SUCCESS;
}
