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

import argparse
import os
import shutil
import subprocess
import tempfile

def archive(repository, target_file, prefix='', crlf=False):
    crlf_args = (['-c', 'core.autocrlf=true', '-c', 'core.eol=crlf'] if crlf
                 else [])
    subprocess.check_call(['git'] + crlf_args +
                          ['archive', '--format=tar', '--prefix=' + prefix,
                           '-o', target_file, 'HEAD'],
                          cwd=repository)

def extract_combined(archive_list, target_dir):
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)
    for a in archive_list:
        subprocess.check_call(['tar', 'xf', a], cwd=target_dir)

def createTarGz(source_dir, target_file):
    (cwd, path) = os.path.split(source_dir)
    subprocess.check_call(['tar', 'czf', target_file, path], cwd=cwd)

def createTarXz(source_dir, target_file):
    (cwd, path) = os.path.split(source_dir)
    subprocess.check_call(['tar', 'cJf', target_file, path], cwd=cwd)

def createZip(source_dir, target_file):
    (cwd, path) = os.path.split(source_dir)
    subprocess.check_call(['zip', '-9qr', target_file, path], cwd=cwd)

def package_repos(repos, combined_prefix, target_file_base):
    workdir = tempfile.mkdtemp(suffix="-createQtcSource")
    def crlf_postfix(crlf):
        return '_win' if crlf else ''
    def tar_name(name, crlf):
        sanitized_name = name.replace('/', '_').replace('\\', '_')
        return os.path.join(workdir, sanitized_name + crlf_postfix(crlf) + '.tar')
    def archive_path(crlf=False):
        return os.path.join(workdir, 'src' + crlf_postfix(crlf), combined_prefix)
    print('Working in "' + workdir + '"')
    print('Pre-packaging archives...')
    for (name, repo, prefix) in repos:
        print('  ' + name + '...')
        for crlf in [False, True]:
            archive(repo, tar_name(name, crlf), prefix, crlf=crlf)
    print('Preparing for packaging...')
    for crlf in [False, True]:
        archive_list = [tar_name(name, crlf) for (name, _, _) in repos]
        extract_combined(archive_list, archive_path(crlf))
    print('Creating .tar.gz...')
    createTarGz(archive_path(crlf=False), target_file_base + '.tar.gz')
    print('Creating .tar.xz...')
    createTarXz(archive_path(crlf=False), target_file_base + '.tar.xz')
    print('Creating .zip with CRLF...')
    createZip(archive_path(crlf=True), target_file_base + '.zip')
    print('Removing temporary directory...')
    shutil.rmtree(workdir)

def parse_arguments():
    script_path = os.path.dirname(os.path.realpath(__file__))
    qtcreator_repo = os.path.join(script_path, '..')
    parser = argparse.ArgumentParser(description="Create Qt Creator source packages")
    parser.add_argument('-p', default=qtcreator_repo, dest='repo', help='path to repository')
    parser.add_argument('-n', default='', dest='name', help='name of plugin')
    parser.add_argument('-s', action='append', default=[], dest='modules', help='submodule to add')
    parser.add_argument('version', help='full version including tag, e.g. 4.2.0-beta1')
    parser.add_argument('edition', help='(opensource | enterprise)')
    return parser.parse_args()

def main():
    args = parse_arguments()
    base_repo_name = args.name if args.name else "qtcreator"
    if not args.name and not args.modules: # default Qt Creator repository
        args.modules = [os.path.join('src', 'shared', 'qbs')]
    repos = [(base_repo_name, args.repo, '')]
    for module in args.modules:
        repos += [(module, os.path.join(args.repo, module), module + os.sep)]
    name_part = '-' + args.name if args.name else ''
    prefix = 'qt-creator-' + args.edition + name_part + '-src-' + args.version
    package_repos(repos, prefix, os.path.join(os.getcwd(), prefix))

if __name__ == "__main__":
    main()
