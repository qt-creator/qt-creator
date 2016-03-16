#!/usr/bin/env python

############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

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
    print('Usage: %s [-v|--version-string=versionstring] [-d|--display-version=versionstring] [-i|--installer-path=/path/to/installerfw] [-a|--archive=archive.7z] [--debug] <outputname>' %  os.path.basename(sys.argv[0]))

def substitute_file(infile, outfile, substitutions):
    with open(infile, 'r') as f:
        template = f.read()
    with open(outfile, 'w') as f:
        f.write(template.format(**substitutions))

def ifw_template_dir():
    script_dir = os.path.dirname(inspect.getfile(inspect.currentframe()))
    source_dir = os.path.normpath(os.path.join(script_dir, '..'))
    return os.path.normpath(os.path.join(source_dir, 'dist', 'installer', 'ifw'))

def main():
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], 'hv:d:i:a:', ['help', 'version-string=', 'display-version=', 'installer-path=', 'archive', 'debug'])
    except:
        usage()
        sys.exit(2)

    if len(args) < 1:
        usage()
        sys.exit(2)

    version = ''
    display_version = ''
    ifw_location = ''
    archives = []
    debug = False
    for o, a in opts:
        if o in ['-h', '--help']:
            usage()
            sys.exit(0)
        if o in ['-v', '--version-string']:
            version = a
        if o in ['-d', '--display-version']:
            display_version = a
        if o in ['-i', '--installer-path']:
            ifw_location = a
        if o in ['-a', '--archive']:
            archives.append(a)
        if o in ['--debug']:
            debug = True

    if (version == ''):
        raise Exception('Version not specified (--version-string)!')

    if not display_version:
        display_version = version

    if (ifw_location == ''):
        raise Exception('Installer framework location not specified (--installer-path)!')

    if not archives:
        raise ValueError('No archive(s) specified (--archive)!')

    installer_name = args[0]
    config_postfix = ''
    if sys.platform == 'darwin':
        config_postfix = '-mac'
    if sys.platform.startswith('win'):
        config_postfix = '-windows'
    if sys.platform.startswith('linux'):
        config_postfix = '-linux'
        installer_name = installer_name + '.run'

    config_name = 'config' + config_postfix + '.xml'

    try:
        temp_dir = tempfile.mkdtemp()
    except:
        raise IOError('Failed to create a temporary directory!')

    if debug:
        print('Working directory: {0}'.format(temp_dir))
    try:
        substs = {}
        substs['version'] = version
        substs['display_version'] = display_version
        substs['date'] = datetime.date.today().isoformat()
        substs['archives'] = ','.join(archives)

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
        for archive in archives:
            shutil.copy(archive, data_path)

        ifw_call = [os.path.join(ifw_location, 'bin', 'binarycreator'), '-c', os.path.join(out_config_dir, config_name), '-p', out_packages_dir, installer_name, '--offline-only' ]
        if debug:
            ifw_call.append('-v')
        subprocess.check_call(ifw_call, stderr=subprocess.STDOUT)
    finally:
        if not debug:
            print('Cleaning up...')
            shutil.rmtree(temp_dir)
        print('Done.')

if __name__ == '__main__':
    main()
