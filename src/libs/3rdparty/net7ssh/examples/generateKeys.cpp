/* An example of ne7ssh library usage. This code will generate a DSA key pair
   and store private key in ./privKeyFile and public key in ./pubKeyFile.
   
   If you are testing this with later openssh versions, make sure to add this
   option to your server's configuration file to enable password authentication:
   
   PasswordAuthentication yes
*/

#include <ne7ssh.h>

int main(int argc,char *argv[])
{
   ne7ssh *_ssh = new ne7ssh();
 
    // Generating DSA keys
    if (!_ssh->generateKeyPair ("rsa", "test@test.com", "./privKeyFile", "./pubKeyFile", 2048))
    {
        const char *errmsg = _ssh->errors()->pop();

        if (errmsg == NULL)
            errmsg = "<null>";
        printf ("Key gneration failed with last error: %s.\n\n", errmsg);
    }

    delete _ssh; 
    return EXIT_SUCCESS;
}
