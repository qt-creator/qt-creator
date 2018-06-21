#!/usr/bin/python

'''A simple wrapper around ninja that enables Qt Creator to parse compile errors

   This wrapper splits up ninja output in such a way that Qt Creator will be able
   to extract compile errors from the output of this script.

   Make sure that ninja is found in PATH and then override the make build step in
   Qt Creator with this script to use it.
'''

import os, subprocess, sys

def main():
    stdout = os.fdopen(sys.stdout.fileno(), 'wb')
    stderr = os.fdopen(sys.stderr.fileno(), 'wb')

    proc = subprocess.Popen(['ninja'] + sys.argv[1:], stdout=subprocess.PIPE, bufsize=256)

    for line in iter(proc.stdout.readline, b''):
        if (line.startswith(b'[') or line.startswith(b'ninja: no work to do')) :
            stdout.write(line)
            stdout.flush()
        else:
            stderr.write(line)
            stderr.flush()

    return proc.returncode

if __name__ == '__main__':
    sys.exit(main())
