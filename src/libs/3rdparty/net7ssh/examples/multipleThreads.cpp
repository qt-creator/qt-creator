/* An example of ne7ssh library usage. Please change the values in connectWithPassword 
   function before compiling.
   
   This will work with openssh server if default shell of authenticating user is bash.
   When using a different shell or custom prompt replace " $" string in waitFor() 
   method with a string corresponding with your shell prompt.
   
   If you are testing this with later openssh versions, make sure to add this
   option to your server's configuration file to enable password authentication:
   
   PasswordAuthentication yes
*/
#include <string.h>
#include <ne7ssh.h>

#if defined(WIN32) || defined (__MINGW32__)
#   include <windows.h>
typedef HANDLE pthread_t;
#endif

void* thread1_proc (void* initData);
void* thread2_proc (void* initData);
void* thread3_proc (void* initData);
void* thread4_proc (void* initData);
pthread_t thread1, thread2, thread3, thread4;

struct ssh_thrarg_t
{
    ne7ssh *ssh;
    int thrid;
};

int main(int argc,char *argv[])
{
    ne7ssh *_ssh = new ne7ssh();
    int status;

    // Set SSH connection options.
    _ssh->setOptions ("aes128-cbc", "hmac-md5");

    ssh_thrarg_t args[4];
    args[0].ssh = _ssh;
    args[1].ssh = _ssh;
    args[2].ssh = _ssh;
    args[3].ssh = _ssh;

    args[0].thrid = 1;
    args[1].thrid = 2;
    args[2].thrid = 3;
    args[3].thrid = 4;

#if !defined(WIN32) && !defined(__MINGW32__)
    status = pthread_create (&thread1, NULL, thread1_proc, (void*)&args[0]);
    status = pthread_create (&thread2, NULL, thread2_proc, (void*)&args[1]);
    status = pthread_create (&thread3, NULL, thread3_proc, (void*)&args[2]);
    status = pthread_create (&thread4, NULL, thread4_proc, (void*)&args[3]);
    pthread_join (thread1, NULL);
    pthread_join (thread2, NULL);
    pthread_join (thread3, NULL);
    pthread_join (thread4, NULL);

#else
    HANDLE h[3];
    if ((thread1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) thread1_proc, (LPVOID)&args[0], 0, NULL)) == NULL)
    {
        ne7ssh::errors()->push (-1, "Failure creating new thread.");
        return (EXIT_FAILURE);
    }
    if ((thread2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) thread2_proc, (LPVOID)&args[1], 0, NULL)) == NULL)
    {
        ne7ssh::errors()->push (-1, "Failure creating new thread.");
        return (EXIT_FAILURE);
    }
    if ((thread3 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) thread3_proc, (LPVOID)&args[2], 0, NULL)) == NULL)
    {
        ne7ssh::errors()->push (-1, "Failure creating new thread.");
        return (EXIT_FAILURE);
    }
    if ((thread4 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) thread4_proc, (LPVOID)&args[3], 0, NULL)) == NULL)
    {
        ne7ssh::errors()->push (-1, "Failure creating new thread.");
        return (EXIT_FAILURE);
    }
    h[0] = thread1;
    h[1] = thread2;
    h[2] = thread3;
    h[3] = thread4;
    WaitForMultipleObjects(4, h, TRUE, INFINITE);
#endif

    delete _ssh;
    return EXIT_SUCCESS;
}


void* thread1_proc (void* initData)
{
    int channel1, i;
    const char* result;
    ne7ssh *_ssh = ((ssh_thrarg_t*) initData)->ssh;
    int thrid = ((ssh_thrarg_t*) initData)->thrid;

    for (i = 0; i < 50; i++)
    {
        // Initiate a connection.
        channel1 = _ssh->connectWithPassword ("remoteHost", 22, "remoteUsr", "password", true, 30);
        if (channel1 < 0)
        {
            const char *errmsg = _ssh->errors()->pop();
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Thread1. Connection failed with last error: %s\n\n", errmsg);
            continue;
        }

        // Wait for bash prompt, or die in 5 seconds.
        if (!_ssh->waitFor (channel1, " $", 5))
        {
            const char *errmsg = _ssh->errors()->pop(channel1);
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Failed while waiting for remote shell wiht last error: %s\n\n", errmsg);
            _ssh->close(channel1);
            continue;
        }

        // Send "ls" command.
        if (!_ssh->send ("ls -al\n", channel1))
        {
            const char *errmsg = _ssh->errors()->pop(channel1);
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Could not send the command. Last error: %s\n\n", errmsg);
            _ssh->close(channel1);
            continue;
        }

        // Wait for bash prompt, or die in 5 seconds
        if (!_ssh->waitFor (channel1, " $", 5))
        {
            const char *errmsg = _ssh->errors()->pop(channel1);
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Timeout while waiting for remote site. Last error: %s\n\n", errmsg);
            _ssh->close(channel1);
            continue;
        }

        // Fetch recieved data.
        result = _ssh->read (channel1);
        printf ("Data Thread %i: %s\n\n", thrid, result);

        // Close the connection.
        _ssh->send ("exit\n", channel1);
    }
}

void* thread2_proc (void* initData)
{
    int channel1, i;
    const char* result;
    ne7ssh *_ssh = ((ssh_thrarg_t*) initData)->ssh;
    int thrid = ((ssh_thrarg_t*) initData)->thrid;

    for (i = 0; i < 50; i++)
    {
        // Initiate a connection.
        channel1 = _ssh->connectWithPassword ("remoteHost", 22, "remoteUsr", "password", true, 30);
        if (channel1 < 0)
        {
            const char *errmsg = _ssh->errors()->pop();
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Thread2. Connection failed with last error: %s\n\n", errmsg);
            continue;
        }

        // Wait for bash prompt, or die in 5 seconds.
        if (!_ssh->waitFor (channel1, " $", 5))
        {
            const char *errmsg = _ssh->errors()->pop(channel1);
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Failed while waiting for remote shell wiht last error: %s\n\n", errmsg);
            _ssh->close(channel1);
            continue;
        }
        result = _ssh->read (channel1);
        
        printf ("Data Count: %i, Thread %i: %s\n\n", i, thrid, result);

        // Send "ls" command.
        if (!_ssh->send ("ls\n", channel1))
        {
            const char *errmsg = _ssh->errors()->pop(channel1);
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Could not send the command. Last error: %s\n\n", errmsg);
            _ssh->close(channel1);
            continue;
        }

        // Wait for bash prompt, or die in 5 seconds
        if (!_ssh->waitFor (channel1, " $", 5))
        {
            const char *errmsg = _ssh->errors()->pop(channel1);
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Timeout while waiting for remote site. Last error: %s\n\n", errmsg);
            _ssh->close(channel1);
            continue;
        }

        // Fetch recieved data.
        result = _ssh->read (channel1);
        printf ("Data Count: %i, Thread %i: %s\n\n", i, thrid, result);

        // Close the connection.
        _ssh->send ("exit\n", channel1);
    }
}

void* thread3_proc (void* initData)
{
    int channel1, i;
    const char* result;
    ne7ssh *_ssh = ((ssh_thrarg_t*) initData)->ssh;
    int thrid = ((ssh_thrarg_t*) initData)->thrid;

    for (i = 0; i < 50; i++)
    {
        // Initiate a connection.
        channel1 = _ssh->connectWithPassword ("remoteHost", 22, "remoteUsr", "password", true, 30);
        if (channel1 < 0)
        {
            const char *errmsg = _ssh->errors()->pop();
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Thread3. Connection failed with last error: %s\n\n", errmsg);
            continue;
        }

        // Wait for bash prompt, or die in 5 seconds.
        if (!_ssh->waitFor (channel1, " $", 5))
        {
            const char *errmsg = _ssh->errors()->pop(channel1);
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Failed while waiting for remote shell wiht last error: %s\n\n", errmsg);
            _ssh->close(channel1);
            continue;
        }

        // Send "ls" command.
        if (!_ssh->send ("ls\n", channel1))
        {
            const char *errmsg = _ssh->errors()->pop(channel1);
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Could not send the command. Last error: %s\n\n", errmsg);
            _ssh->close(channel1);
            continue;
        }

        // Wait for bash prompt, or die in 5 seconds
        if (!_ssh->waitFor (channel1, " $", 5))
        {
            const char *errmsg = _ssh->errors()->pop(channel1);
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Timeout while waiting for remote site. Last error: %s\n\n", errmsg);
            _ssh->close(channel1);
            continue;
        }

        // Fetch recieved data.
        result = _ssh->read (channel1);
        printf ("Data Thread %i: %s\n\n", thrid, result);

        // Close the connection.
        _ssh->send ("exit\n", channel1);
    }
}

void* thread4_proc (void* initData)
{
    int channel1, i;
    const char* result;
    ne7ssh *_ssh = ((ssh_thrarg_t*) initData)->ssh;
    int thrid = ((ssh_thrarg_t*) initData)->thrid;

    for (i = 0; i < 50; i++)
    {
        // Initiate a connection.
        channel1 = _ssh->connectWithPassword ("remoteHost", 22, "remoteUsr", "password", true, 30);
        if (channel1 < 0)
        {
            const char *errmsg = _ssh->errors()->pop();
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Thread4. Connection failed with last error: %s\n\n", errmsg);
            continue;
        }

        // Wait for bash prompt, or die in 5 seconds.
        if (!_ssh->waitFor (channel1, " $", 5))
        {
            const char *errmsg = _ssh->errors()->pop(channel1);
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Failed while waiting for remote shell wiht last error: %s\n\n", errmsg);
            _ssh->close(channel1);
            continue;
        }

        // Send "ls" command.
        if (!_ssh->send ("ls\n", channel1))
        {
            const char *errmsg = _ssh->errors()->pop(channel1);
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Could not send the command. Last error: %s\n\n", errmsg);
            _ssh->close(channel1);
            continue;
        }

        // Wait for bash prompt, or die in 5 seconds
        if (!_ssh->waitFor (channel1, " $", 5))
        {
            const char *errmsg = _ssh->errors()->pop(channel1);
            if (errmsg == NULL)
                errmsg = "<null>";
            printf ("Timeout while waiting for remote site. Last error: %s\n\n", errmsg);
            _ssh->close(channel1);
            continue;
        }

        // Fetch recieved data.
        result = _ssh->read (channel1);
        printf ("Data Thread %i: %s\n\n", thrid, result);

        // Close the connection.
        _ssh->send ("exit\n", channel1);
    }
}
