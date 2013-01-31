#!/usr/bin/env python
################################################################################
# Copyright (C) 2013 Digia Plc
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
#   * Neither the name of Digia Plc, nor the names of its contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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
import datetime
import getopt
import subprocess
import fnmatch
import tempfile
import shutil
import inspect

def usage():
    print 'Usage: %s [-v|--version-string=versionstring] [-i|--installer-path=/path/to/installerfw] [-a|--archive=archive.7z] <outputname>' %  os.path.basename(sys.argv[0])

def substitute_file(infile, outfile, substitutions):
    with open(infile, 'r') as f:
      template = f.read()
    with open(outfile, 'w') as f:
      f.write(template.format(**substitutions))

def ifw_template_dir():
    script_dir = os.path.dirname(inspect.getfile(inspect.currentframe()))
    source_dir = os.path.normpath(os.path.join(script_dir, '..'));
    return os.path.normpath(os.path.join(source_dir, 'dist', 'installer', 'ifw'))

def main():
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], 'hv:i:a:', ['help', 'version-string=', 'installer-path=', 'archive'])
    except:
        usage()
        sys.exit(2)

    if len(args) < 1:
        usage()
        sys.exit(2)

    version = ''
    ifw_location = ''
    archive = ''
    for o, a in opts:
        if o in ('-h', '--help'):
            usage()
            sys.exit(0)
        if o in ('-v', '--version-string'):
            version = a
        if o in ('-i', '--installer-path'):
            ifw_location = a
        if o in ('-a', '--archive'):
            archive = a

    if (version == ''):
      raise Exception('Version not specified (--version-string)!')

    if (ifw_location == ''):
      raise Exception('Installer framework location not specified (--installer-path)!')

    if (archive == ''):
      raise Exception('Archive not specified (--archive)!')

    installer_name = args[0]
    config_postfix = ''
    if sys.platform == 'darwin':
        installer_name = installer_name + '.dmg'
    if sys.platform.startswith('win'):
        config_postfix = '-windows'
    if sys.platform.startswith('linux'):
        config_postfix = '-linux'
        installer_name = installer_name + '.bin'

    config_name = 'config' + config_postfix + '.xml'

    try:
        temp_dir = tempfile.mkdtemp()
    except:
        raise Exception('Failed to create a temporary directory!')

    try:
        substs = {}
        substs['version'] = version
        substs['date'] = datetime.date.today().isoformat()

        template_dir = ifw_template_dir()
        out_config_dir = os.path.join(temp_dir,'config')
        out_packages_dir = os.path.join(temp_dir, 'packages')

        shutil.copytree(os.path.join(template_dir, 'packages'), os.path.join(temp_dir, 'packages'))
        shutil.copytree(os.path.join(template_dir, 'config'), os.path.join(temp_dir, 'config'))

        for root, dirnames, filenames in os.walk(out_packages_dir):
            for template in fnmatch.filter(filenames, '*.in'):
                substitute_file(os.path.join(root, template), os.path.join(root, template[:-3]), substs)
                os.remove(os.path.join(root, template))

        for root, dirnames, filenames in os.walk(out_config_dir):
            for template in fnmatch.filter(filenames, '*.in'):
                substitute_file(os.path.join(root, template), os.path.join(root, template[:-3]), substs)
                os.remove(os.path.join(root, template))

        data_path = os.path.join(out_packages_dir, 'org.qtproject.qtcreator.application', 'data')
        if not os.path.exists(data_path):
            os.makedirs(data_path)
        shutil.copy(archive, data_path)

        ifw_call = [os.path.join(ifw_location, 'bin', 'binarycreator'), '-c', os.path.join(out_config_dir, config_name), '-p', out_packages_dir, installer_name, '--offline-only' ]
        subprocess.check_call(ifw_call, stderr=subprocess.STDOUT)
    finally:
        print 'Cleaning up...'
        shutil.rmtree(temp_dir)
        print 'Done.'

if __name__ == '__main__':
    main()
