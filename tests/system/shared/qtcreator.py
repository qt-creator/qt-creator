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

import platform;
import shutil;
import os;
import glob;
import atexit;
import codecs;
import subprocess;
import sys
import errno;
from datetime import datetime,timedelta;
import __builtin__

srcPath = ''
SettingsPath = ''
tmpSettingsDir = ''
testSettings.logScreenshotOnFail = True
testSettings.logScreenshotOnError = True
__origStartApplication__ = None

source("../../shared/classes.py")
source("../../shared/utils.py")
source("../../shared/fs_utils.py")
source("../../shared/build_utils.py")
source("../../shared/project.py")
source("../../shared/editor_utils.py")
source("../../shared/project_explorer.py")
source("../../shared/hook_utils.py")
source("../../shared/debugger.py")
source("../../shared/clang.py")
source("../../shared/welcome.py")
source("../../shared/workarounds.py") # include this at last

# ATTENTION: if a test case calls startApplication("qtcreator...") for several times this
# function must be called BEFORE any call except the first (which is done always automatically)
def overrideStartApplication():
    global startApplication, __origStartApplication__
    if (platform.system() == "Linux"):
        return
    if (__origStartApplication__ == None):
        __origStartApplication__ = startApplication
    def startApplication(*args):
        args = list(args)
        if str(args[0]).startswith('qtcreator'):
            if platform.system() == 'Darwin':
                args[0] = args[0].replace('qtcreator', '"Qt Creator"', 1)
                test.log("Using workaround for MacOS (different AUT name)")
            else:
                args[0] = args[0] + ' -platform windows:dialogs=none'
                test.log("Using workaround for Windows (failing to hook into native FileDialog)")
        return __origStartApplication__(*args)

def startedWithoutPluginError():
    try:
        loaderErrorWidgetName = ("{name='ExtensionSystem__Internal__PluginErrorOverview' "
                                 "type='ExtensionSystem::PluginErrorOverview' visible='1' "
                                 "windowTitle='Qt Creator - Plugin loader messages'}")
        loaderError = waitForObject(loaderErrorWidgetName, 1000)
        test.fatal("Could not perform clean start of Qt Creator - Plugin error occurred.",
                   waitForObject("{name='pluginError' type='QTextEdit' visible='1' window=%s}"
                                 % loaderErrorWidgetName, 1000).plainText)
        clickButton("{text~='(Next.*|Continue)' type='QPushButton' visible='1'}")
        invokeMenuItem("File", "Exit")
        return False
    except:
        return True

def waitForCleanShutdown(timeOut=10):
    appCtxt = currentApplicationContext()
    shutdownDone = (str(appCtxt)=="")
    if platform.system() in ('Windows','Microsoft'):
        # cleaning helper for running on the build machines
        checkForStillRunningQmlExecutable(['qmlviewer.exe', 'qmlscene.exe'])
        endtime = datetime.utcnow() + timedelta(seconds=timeOut)
        while not shutdownDone:
            # following work-around because os.kill() works for win not until python 2.7
            if appCtxt.pid==-1:
                break
            output = getOutputFromCmdline(["tasklist", "/FI", "PID eq %d" % appCtxt.pid],
                                          acceptedError=1)
            if (output=="INFO: No tasks are running which match the specified criteria."
                or output=="" or output.find("ERROR")==0):
                shutdownDone=True
            if not shutdownDone and datetime.utcnow() > endtime:
                break
    else:
        endtime = datetime.utcnow() + timedelta(seconds=timeOut)
        while not shutdownDone:
            try:
                os.kill(appCtxt.pid,0)
            except OSError, err:
                if err.errno == errno.EPERM or err.errno == errno.ESRCH:
                    shutdownDone=True
            if not shutdownDone and datetime.utcnow() > endtime:
                break
        if platform.system() == 'Linux' and JIRA.isBugStillOpen(15749):
            pgrepOutput = getOutputFromCmdline(["pgrep", "-f", "qtcreator_process_stub"],
                                               acceptedError=1)
            pids = pgrepOutput.splitlines()
            if len(pids):
                print("Killing %d qtcreator_process_stub instances" % len(pids))
            for pid in pids:
                try:
                    os.kill(__builtin__.int(pid), 9)
                except OSError: # we might kill the parent before the current pid
                    pass

def checkForStillRunningQmlExecutable(possibleNames):
    for qmlHelper in possibleNames:
        output = getOutputFromCmdline(["tasklist", "/FI", "IMAGENAME eq %s" % qmlHelper])
        if "INFO: No tasks are running which match the specified criteria." in output:
            continue
        else:
            if subprocess.call(["taskkill", "/F", "/FI", "IMAGENAME eq %s" % qmlHelper]) == 0:
                print "Killed still running %s" % qmlHelper
            else:
                print "%s is still running - failed to kill it" % qmlHelper

def __removeTestingDir__():
    def __removeIt__(directory):
        deleteDirIfExists(directory)
        return not os.path.exists(directory)

    devicesXML = os.path.join(tmpSettingsDir, "QtProject", "qtcreator", "devices.xml")
    lastMTime = os.path.getmtime(devicesXML)
    testingDir = os.path.dirname(tmpSettingsDir)
    waitForCleanShutdown()
    waitFor('os.path.getmtime(devicesXML) > lastMTime', 5000)
    waitFor('__removeIt__(testingDir)', 2000)

def __substitute__(fileName, search, replace):
    origFileName = fileName + "_orig"
    os.rename(fileName, origFileName)
    origFile = open(origFileName, "r")
    modifiedFile = open(fileName, "w")
    for line in origFile:
        modifiedFile.write(line.replace(search, replace))
    origFile.close()
    modifiedFile.close()
    os.remove(origFileName)

def substituteTildeWithinToolchains(settingsDir):
    toolchains = os.path.join(settingsDir, "QtProject", 'qtcreator', 'toolchains.xml')
    home = os.path.expanduser("~")
    __substitute__(toolchains, "~", home)
    test.log("Substituted all tildes with '%s' inside toolchains.xml..." % home)

def substituteDefaultCompiler(settingsDir):
    compiler = None
    if platform.system() == 'Darwin':
        compiler = "clang_64"
    elif platform.system() == 'Linux':
        if __is64BitOS__():
            compiler = "gcc_64"
        else:
            compiler = "gcc"
    else:
        test.warning("Called substituteDefaultCompiler() on wrong platform.",
                     "This is a script error.")
    if compiler:
        qtversion = os.path.join(settingsDir, "QtProject", 'qtcreator', 'qtversion.xml')
        __substitute__(qtversion, "SQUISH_DEFAULT_COMPILER", compiler)
        test.log("Injected default compiler '%s' to qtversion.xml..." % compiler)

def substituteCdb(settingsDir):
    def canUse32bitCdb():
        try:
            serverIni = readFile(os.path.join(os.getenv("APPDATA"), "froglogic",
                                              "Squish", "ver1", "server.ini"))
            autLine = filter(lambda line: "AUT/qtcreator" in line, serverIni.splitlines())[0]
            autPath = autLine.split("\"")[1]
            return os.path.exists(os.path.join(autPath, "..", "lib", "qtcreatorcdbext32"))
        except:
            test.fatal("Something went wrong when determining debugger bitness",
                       "Did Squish's file structure change? Guessing 32-bit cdb can be used...")
            return True

    if canUse32bitCdb():
        architecture = "x86"
        bitness = "32"
    else:
        architecture = "x64"
        bitness = "64"
    debuggers = os.path.join(settingsDir, "QtProject", 'qtcreator', 'debuggers.xml')
    __substitute__(debuggers, "SQUISH_DEBUGGER_ARCHITECTURE", architecture)
    __substitute__(debuggers, "SQUISH_DEBUGGER_BITNESS", bitness)
    test.log("Injected architecture '%s' and bitness '%s' in cdb path..." % (architecture, bitness))

def __guessABI__(supportedABIs, use64Bit):
    if platform.system() == 'Linux':
        supportedABIs = filter(lambda x: 'linux' in x, supportedABIs)
    elif platform.system() == 'Darwin':
        supportedABIs = filter(lambda x: 'macos' in x, supportedABIs)
    if use64Bit:
        searchFor = "64bit"
    else:
        searchFor = "32bit"
    for abi in supportedABIs:
        if searchFor in abi:
            return abi
    if use64Bit:
        test.log("Supported ABIs do not include an ABI supporting 64bit - trying 32bit now")
        return __guessABI__(supportedABIs, False)
    test.fatal('Could not guess ABI!',
               'Given ABIs: %s' % str(supportedABIs))
    return ''

def __is64BitOS__():
    if platform.system() in ('Microsoft', 'Windows'):
        machine = os.getenv("PROCESSOR_ARCHITEW6432", os.getenv("PROCESSOR_ARCHITECTURE"))
    else:
        machine = platform.machine()
    if machine:
        return '64' in machine
    else:
        return False

def substituteUnchosenTargetABIs(settingsDir):
    class ReadState:
        NONE = 0
        READING = 1
        CLOSED = 2

    on64Bit = __is64BitOS__()
    toolchains = os.path.join(settingsDir, "QtProject", 'qtcreator', 'toolchains.xml')
    origToolchains = toolchains + "_orig"
    os.rename(toolchains, origToolchains)
    origFile = open(origToolchains, "r")
    modifiedFile = open(toolchains, "w")
    supported = []
    readState = ReadState.NONE
    for line in origFile:
        if readState == ReadState.NONE:
            if "SupportedAbis" in line:
                supported = []
                readState = ReadState.READING
        elif readState == ReadState.READING:
            if "</valuelist>" in line:
                readState = ReadState.CLOSED
            else:
                supported.append(line.split(">", 1)[1].rsplit("<", 1)[0])
        elif readState == ReadState.CLOSED:
            if "SupportedAbis" in line:
                supported = []
                readState = ReadState.READING
            elif "SET_BY_SQUISH" in line:
                line = line.replace("SET_BY_SQUISH", __guessABI__(supported, on64Bit))
        modifiedFile.write(line)
    origFile.close()
    modifiedFile.close()
    os.remove(origToolchains)
    test.log("Substituted unchosen ABIs inside toolchains.xml...")

def copySettingsToTmpDir(destination=None, omitFiles=[]):
    global tmpSettingsDir, SettingsPath, origSettingsDir
    if destination:
        destination = os.path.abspath(destination)
        if not os.path.exists(destination):
            os.makedirs(destination)
        elif os.path.isfile(destination):
                test.warning("Provided destination for settings exists as file.",
                             "Creating another folder for being able to execute tests.")
                destination = tempDir()
    else:
        destination = tempDir()
    tmpSettingsDir = destination
    pathLen = len(origSettingsDir) + 1
    for r,d,f in os.walk(origSettingsDir):
        currentPath = os.path.join(tmpSettingsDir, r[pathLen:])
        for dd in d:
            folder = os.path.join(currentPath, dd)
            if not os.path.exists(folder):
                os.makedirs(folder)
        for ff in f:
            if not ff in omitFiles:
                shutil.copy(os.path.join(r, ff), currentPath)
    if platform.system() in ('Linux', 'Darwin'):
        substituteTildeWithinToolchains(tmpSettingsDir)
        substituteDefaultCompiler(tmpSettingsDir)
    elif platform.system() in ('Windows', 'Microsoft'):
        substituteCdb(tmpSettingsDir)
    substituteUnchosenTargetABIs(tmpSettingsDir)
    SettingsPath = ' -settingspath "%s"' % tmpSettingsDir

# current dir is directory holding qtcreator.py
origSettingsDir = os.path.abspath(os.path.join(os.getcwd(), "..", "..", "settings"))
qt4Path = os.path.expanduser("~/Qt4.8.7")

if platform.system() in ('Windows', 'Microsoft'):
    qt4Path = "C:\\Qt\\Qt4.8.7"
    origSettingsDir = os.path.join(origSettingsDir, "windows")
elif platform.system() == 'Darwin':
    origSettingsDir = os.path.join(origSettingsDir, "mac")
else:
    origSettingsDir = os.path.join(origSettingsDir, "unix")

qt4examplePath = os.path.join(qt4Path, "examples")
srcPath = os.getenv("SYSTEST_SRCPATH", os.path.expanduser(os.path.join("~", "squish-data")))

overrideStartApplication()

# the following only doesn't work if the test ends in an exception
if os.getenv("SYSTEST_NOSETTINGSPATH") != "1":
    copySettingsToTmpDir()
    atexit.register(__removeTestingDir__)

if os.getenv("SYSTEST_WRITE_RESULTS") == "1" and os.getenv("SYSTEST_RESULTS_FOLDER") != None:
    atexit.register(writeTestResults, os.getenv("SYSTEST_RESULTS_FOLDER"))
