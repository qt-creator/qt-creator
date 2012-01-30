#!/usr/bin/env python
################################################################################
# Copyright (c) 2011 Nokia Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#   * Neither the name of Nokia Corporation, nor the names of its contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
################################################################################

import os
import sys
import getopt
import subprocess
import tempfile
import re
import shutil

def usage():
    print "Usage: %s <creator_install_dir> <pattern>" %  os.path.basename(sys.argv[0])

def main():
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], 'h', ['help'])
    except:
        usage()
        sys.exit(2)
    for o, a in opts:
        if o in ('-h', '--help'):
            usage()
            sys.exit(0)

    if len(args) < 2:
            usage()
            sys.exit(2)

    sourcedir=args[0]
    pattern=args[1]

    try:
        temp_dir = tempfile.mkdtemp()
    except:
        print "Failed to create a temporary directory!"
        sys.exit(2)

    try:
        formatted_dirname = "qt-creator-%s" % pattern
        temp_fullpath = os.path.join(temp_dir, formatted_dirname)
        print "Copying files to temporary directory '%s' for packaging..." % temp_fullpath
        shutil.copytree(sourcedir, temp_fullpath, symlinks=True)
        print "7-zipping temporary directory..."
        output = subprocess.check_output(['7z', 'a', '-mx9', formatted_dirname+'.7z', temp_fullpath])
        print output
    except:
        print "Error occured, cleaning up..."
        shutil.rmtree(temp_dir)
        sys.exit(2)

    print "Cleaning up..."
    shutil.rmtree(temp_dir)
    print "Done."

if __name__ == "__main__":
    if sys.platform == 'darwin':
        print "Mac OS does not require this script!"
        sys.exit(2)
    else:
        main()
