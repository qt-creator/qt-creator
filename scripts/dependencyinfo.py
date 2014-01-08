#! /usr/bin/env python2
################################################################################
# Copyright (C) 2014 Digia Plc
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

import glob
import logging
import os
import re
import subprocess
import sys
import platform

class Library:
    def __init__(self, path):
        self.path = path
        self.name = ''
        self.exportedSymbols = {}

        self.name = re.sub('^(.*/)?lib', '', path)
        self.name = re.sub('\.so.*$', '', self.name)

        self._runNM(self.path)

    def isLibrary(self):
        return True

    def isPlugin(self):
        return False

    def debugDump(self):
        log.debug('Library "%s" exports %d symbols.', self.name, len(self.exportedSymbols))

    def _runNM(self, path):
        try:
            output = subprocess.check_output(['/usr/bin/nm', '--demangle', path], stderr=subprocess.STDOUT).splitlines()
        except:
            output = []
        for line in output:
            self._parseNMline(line)

    def _parseNMline(self, line):
        m = re.search('^[0-9a-fA-F]{8,16} [TD] (.*)$', line)
        if m:
            self.exportedSymbols[m.group(1)] = 1



class Plugin(Library):
    def __init__(self, spec):
        self.pluginSpec = spec
        self.specDependencies = {}
        self.symbolDependencies = {}
        self.name = ''
        self.importedSymbols = []
        self.path = self._parsePluginSpec(spec)
        Library.__init__(self, self.path)

        self.importedSymbols.sort()

    def isLibrary(self):
        return False

    def isPlugin(self):
        return True

    def debugDump(self):
        log.debug('Plugin "%s" imports %d symbols and exports %d symbols.', self.name, len(self.importedSymbols),
            len(self.exportedSymbols))
        for i in self.specDependencies:
            log.debug('    Spec declares dependency on "%s"', i)
        for i in self.symbolDependencies:
            tmp = 'plugin'
            if i.isLibrary():
                tmp = 'lib'
            log.debug('    Symbol dependency on %s "%s" (%d)', tmp, i.name, self.symbolDependencies[i])

    def _parsePluginSpec(self, spec):
        dirname = os.path.dirname(spec)
        with open(spec) as f:
            content = f.readlines()
        for line in content:
            m = re.search('(plugin|dependency)\s+name="([^"]+)"(?:.*\stype="([^"]+)")?', line)
            if not(m):
                continue
            if m.group(1) == 'plugin':
                if self.name != '':
                    log.critical('Plugin name already set to "%s"!', self.name)
                else:
                    self.name = m.group(2)
            else:
                kind = m.group(3)
                if not(kind):
                    kind = 'strong'
                self.specDependencies[m.group(2)] = kind

        if self.name == '':
            log.critical('Plugin name not set for spec "%s".', spec)

        return os.path.join(dirname, "lib%s.so" % self.name)

    def _parseNMline(self, line):
        m = re.search('^\s+ U (.*)$', line)
        if m:
            self.importedSymbols.append(m.group(1))
        else:
            Library._parseNMline(self, line)

    def addSymbolDependency(self, dep, symbol):
        if dep in self.symbolDependencies:
            self.symbolDependencies[dep]['total'] += 1
        else:
            self.symbolDependencies[dep] = {}
            self.symbolDependencies[dep]['total'] = 1

        self.symbolDependencies[dep][symbol] = 1



class SymbolResolver:
    def __init__(self, plugins, libraries):
        self.libraries = libraries
        self.libraries.extend(plugins)

        for i in plugins:
            self._resolve(i)

    def _resolve(self, plugin):
        print 'Resolving symbols for {}...'.format(plugin.name)
        for symbol in plugin.importedSymbols:
            lib = self._resolveSymbol(symbol)
            if lib:
                plugin.addSymbolDependency(lib, symbol)

    def _resolveSymbol(self, symbol):
        for i in self.libraries:
            if symbol in i.exportedSymbols:
                return i
        return None



class Reporter:
    def __init__(self, plugins):
        for i in plugins:
            self._reportPluginSpecIssues(i)

    def _reportPluginSpecIssues(self, plugin):
        print 'Plugin "{}" imports {} symbols and exports {} symbols.'.format(
            plugin.name, len(plugin.importedSymbols), len(plugin.exportedSymbols))

        spec = plugin.specDependencies
        symb = {}
        lib = {}
        for p in plugin.symbolDependencies:
            if p.isPlugin():
                symb[p.name] = plugin.symbolDependencies[p]
            else:
                lib[p.name] = plugin.symbolDependencies[p]

        for i in spec:
            if i in symb:
                total = symb[i]['total']
                print '    {}: OK ({} usages)'.format(i, total)

                self._printSome(symb[i])
                del symb[i]
            else:
                if spec[i] == 'optional':
                    print '    {}: OK (optional)'.format(i)
                else:
                    print '    {}: WARNING: unused'.format(i)
        for i in symb:
            total = symb[i]['total']
            print '    {}: ERROR: undeclared ({} usages)'.format(i, total)
            self._printSome(symb[i])
        for i in lib:
            total = lib[i]['total']
            print '    LIBRARY {} used ({} usages)'.format(i, total)

    def _printSome(self, data):
        keys = data.keys()
        if len(keys) <= 11:
            for i in keys:
                if i != 'total':
                    print '        {}'.format(i)



class BinaryDirExaminer:
    def __init__(self, path):
        self.libraries = []
        self.plugins = []
        self.binaryDir = path

        log.debug('Examining directory "%s".', path)

        self._findLibraries(path)
        self._findPlugins(path)

    def _findLibraries(self, path):
        libdir = glob.glob(os.path.join(path, "lib", "qtcreator", "lib*"))
        for l in libdir:
            if os.path.islink(l):
                continue
            log.debug('   Looking at library "%s".', l)
            self.libraries.append(Library(l))

    def _findPlugins(self, path):
        vendordirs = glob.glob(os.path.join(path, "lib", "qtcreator", "plugins", "*"))
        for dir in vendordirs:
            pluginspecs = glob.glob(os.path.join(dir, "*.pluginspec"))
            for spec in pluginspecs:
                log.debug('   Looking at plugin "%s".', spec)
                self.plugins.append(Plugin(spec))



if __name__ == '__main__':
    # Setup logging:
    log = logging.getLogger('log')
    log.setLevel(logging.DEBUG)
    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)
    log.addHandler(ch)

    # Make sure we are on linux:
    if platform.system() != 'Linux':
        log.critical("This check can only run on Linux")
        sys.exit(1)

    # Sanity check:
    if not(os.path.exists(os.path.join(os.getcwd(), "bin", "qtcreator"))):
        log.critical('Not a top level Qt Creator build directory.')
        sys.exit(1)

    binExaminer = BinaryDirExaminer(os.path.abspath(os.getcwd()))
    # Find symbol dependencies:
    resolver = SymbolResolver(binExaminer.plugins, binExaminer.libraries)
    reporter = Reporter(binExaminer.plugins)
