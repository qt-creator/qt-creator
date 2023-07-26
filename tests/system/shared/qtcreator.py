# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import platform;
import re;
import shutil;
import os;
import glob;
import atexit;
import codecs;
import subprocess;
import sys
import errno;
from datetime import datetime,timedelta;
if sys.version_info.major > 2:
    import builtins as __builtin__
else:
    import __builtin__


srcPath = ''
SettingsPath = []
tmpSettingsDir = ''
testSettings.logScreenshotOnFail = True
testSettings.logScreenshotOnError = True

source("../../shared/classes.py")
source("../../shared/utils.py")
source("../../shared/fs_utils.py")
source("../../shared/build_utils.py")
source("../../shared/project.py")
source("../../shared/editor_utils.py")
source("../../shared/project_explorer.py")
source("../../shared/debugger.py")
source("../../shared/clang.py")
source("../../shared/welcome.py")
source("../../shared/workarounds.py") # include this at last

settingsPathsWithExplicitlyEnabledClangd = set()

def __closeInfoBarEntry__(leftButtonText):
    toolButton = ("text='%s' type='QToolButton' unnamed='1' visible='1' "
                  "window=':Qt Creator_Core::Internal::MainWindow'")
    doNotShowAgain = toolButton % "Do Not Show Again"
    leftWidget = "leftWidget={%s}" % (toolButton % leftButtonText)
    test.log("closing %s" % leftButtonText)
    buttonObjStr = "{%s %s}" % (doNotShowAgain, leftWidget)
    checkIfObjectExists(buttonObjStr, verboseOnFail=True)
    clickButton(waitForObject(buttonObjStr))
    checkIfObjectExists(buttonObjStr, False, 2000, True)

# additionalParameters must be a list or tuple of strings or None
def startQC(additionalParameters=None, withPreparedSettingsPath=True, closeLinkToQt=True, cancelTour=True):
    global SettingsPath
    global settingsPathsWithExplicitlyEnabledClangd
    appWithOptions = ['"Qt Creator"' if platform.system() == 'Darwin' else "qtcreator"]
    if withPreparedSettingsPath:
        appWithOptions.extend(SettingsPath)
    if additionalParameters is not None:
        appWithOptions.extend(additionalParameters)
    if platform.system() in ('Microsoft', 'Windows'): # for hooking into native file dialog
        appWithOptions.extend(('-platform', 'windows:dialogs=none'))
    test.log("Starting now: %s" % ' '.join(appWithOptions))
    appContext = startApplication(' '.join(appWithOptions))
    if (closeLinkToQt or cancelTour or
        str(SettingsPath) not in settingsPathsWithExplicitlyEnabledClangd):
        progressBarWait(3000)  # wait for the "Updating documentation" progress bar
    if str(SettingsPath) not in settingsPathsWithExplicitlyEnabledClangd:
        # This block will incorrectly be skipped when a test calls startQC multiple times in a row
        # passing different settings paths in "additionalParameters". Currently we don't have such
        # a test. Even if we did, it would only make a difference if the test relied on clangd
        # being active and ran on a machine with insufficient memory.
        try:
            mouseClick(waitForObject("{text='Enable Anyway' type='QToolButton' "
                                     "unnamed='1' visible='1' "
                                     "window=':Qt Creator_Core::Internal::MainWindow'}", 500))
            settingsPathsWithExplicitlyEnabledClangd.add(str(SettingsPath))
        except:
            pass
    if closeLinkToQt or cancelTour:
        if closeLinkToQt:
            __closeInfoBarEntry__("Link with Qt")
        if cancelTour:
            __closeInfoBarEntry__("Take UI Tour")
    return appContext;

def startedWithoutPluginError():
    try:
        loaderErrorWidgetName = ("{name='ExtensionSystem__Internal__PluginErrorOverview' "
                                 "type='ExtensionSystem::PluginErrorOverview' visible='1' "
                                 "windowTitle='Plugin Loader Messages'}")
        waitForObject(loaderErrorWidgetName, 1000)
        test.fatal("Could not perform clean start of Qt Creator - Plugin error occurred.",
                   str(waitForObject("{name='pluginError' type='QTextEdit' visible='1' window=%s}"
                                     % loaderErrorWidgetName, 1000).plainText))
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
        checkForStillRunningQmlExecutable(['qmlscene.exe', 'qml.exe'])
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
            except OSError as err:
                if err.errno == errno.EPERM or err.errno == errno.ESRCH:
                    shutdownDone=True
            if not shutdownDone and datetime.utcnow() > endtime:
                break

def checkForStillRunningQmlExecutable(possibleNames):
    for qmlHelper in possibleNames:
        output = getOutputFromCmdline(["tasklist", "/FI", "IMAGENAME eq %s" % qmlHelper])
        if "INFO: No tasks are running which match the specified criteria." in output:
            continue
        else:
            if subprocess.call(["taskkill", "/F", "/FI", "IMAGENAME eq %s" % qmlHelper]) == 0:
                print("Killed still running %s" % qmlHelper)
            else:
                print("%s is still running - failed to kill it" % qmlHelper)


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


def substituteTildeWithinQtVersion(settingsDir):
    toolchains = os.path.join(settingsDir, "QtProject", 'qtcreator', 'qtversion.xml')
    home = os.path.expanduser("~")
    __substitute__(toolchains, "~", home)
    test.log("Substituted all tildes with '%s' inside qtversion.xml..." % home)


def substituteOnlineInstallerPath(settingsDir):
    qtversions = os.path.join(settingsDir, "QtProject", 'qtcreator', 'qtversion.xml')
    dflt = "C:/Qt" if platform.system() in ('Microsoft', 'Windows') else os.path.expanduser("~/Qt")
    replacement = str(os.getenv("SYSTEST_QTOI_BASEPATH", dflt)).replace('\\', '/')
    while replacement.endswith('/'):
        replacement = replacement[:-1]
    __substitute__(qtversions, "SQUISH_QTOI_BASEPATH", replacement)
    test.log("Substituted online installer base path (%s) inside qtversions.xml." % replacement)


def substituteDefaultCompiler(settingsDir):
    compiler = None
    if platform.system() == 'Darwin':
        compiler = "clang_64"
    elif platform.system() == 'Linux':
        compiler = "gcc_64"
    else:
        test.warning("Called substituteDefaultCompiler() on wrong platform.",
                     "This is a script error.")
    if compiler:
        qtversion = os.path.join(settingsDir, "QtProject", 'qtcreator', 'qtversion.xml')
        __substitute__(qtversion, "SQUISH_DEFAULT_COMPILER", compiler)
        test.log("Injected default compiler '%s' to qtversion.xml..." % compiler)

def substituteCdb(settingsDir):
    def canUse64bitCdb():
        try:
            serverIni = readFile(os.path.join(os.getenv("APPDATA"), "froglogic",
                                              "Squish", "ver1", "server.ini"))
            autLine = next(iter(filter(lambda line: "AUT/qtcreator" in line,
                                       serverIni.splitlines())))
            autPath = autLine.split("\"")[1]
            return os.path.exists(os.path.join(autPath, "..", "lib", "qtcreatorcdbext64"))
        except:
            test.fatal("Something went wrong when determining debugger bitness",
                       "Did Squish's file structure change? Guessing 32-bit cdb can be used...")
            return True

    if canUse64bitCdb():
        architecture = "x64"
        bitness = "64"
    else:
        architecture = "x86"
        bitness = "32"
    debuggers = os.path.join(settingsDir, "QtProject", 'qtcreator', 'debuggers.xml')
    __substitute__(debuggers, "SQUISH_DEBUGGER_ARCHITECTURE", architecture)
    __substitute__(debuggers, "SQUISH_DEBUGGER_BITNESS", bitness)
    test.log("Injected architecture '%s' and bitness '%s' in cdb path..." % (architecture, bitness))


def substituteMsvcPaths(settingsDir, version, targetBitness=64):
    if not version in ['2017', '2019']:
        test.fatal('Unexpected MSVC version - "%s" not implemented yet.' % version)
        return

    hostArch = "Hostx64" if targetBitness == 64 else "Hostx86"
    targetArch = "x64" if targetBitness == 64 else "x86"
    for msvcFlavor in ["Community", "BuildTools"]:
        try:
            msvcPath = os.path.join("C:\\Program Files (x86)", "Microsoft Visual Studio",
                                    version, msvcFlavor, "VC", "Tools", "MSVC")
            foundVersions = os.listdir(msvcPath) # undetermined order
            foundVersions.sort(reverse=True) # we explicitly want the latest and greatest
            msvcPath = os.path.join(msvcPath, foundVersions[0], "bin", hostArch, targetArch)
            __substitute__(os.path.join(settingsDir, "QtProject", 'qtcreator', 'toolchains.xml'),
                           "SQUISH_MSVC%s_%d_PATH" % (version, targetBitness), msvcPath)
            return
        except:
            continue
    test.warning("PATH variable for MSVC%s could not be set, some tests will fail." % version,
                 "Please make sure that MSVC%s is installed correctly." % version)


def prependWindowsKit(settingsDir, targetBitness=64):
    targetArch = "x64" if targetBitness == 64 else "x86"
    profilesPath = os.path.join(settingsDir, 'QtProject', 'qtcreator', 'profiles.xml')
    winkits = os.path.join("C:\\Program Files (x86)", "Windows Kits", "10")
    if not os.path.exists(winkits):
        __substitute__(profilesPath, "SQUISH_ENV_MODIFICATION", "")
        return
    possibleVersions = os.listdir(os.path.join(winkits, 'bin'))
    possibleVersions.reverse() # prefer higher versions
    for version in possibleVersions:
        if not version.startswith("10"):
            continue
        toolsPath = os.path.join(winkits, 'bin', version, targetArch)
        if os.path.exists(os.path.join(toolsPath, 'rc.exe')):
            __substitute__(profilesPath, "SQUISH_ENV_MODIFICATION", "PATH=+%s" % toolsPath)
            return
    test.warning("Windows Kit path could not be added, some tests mail fail.")
    __substitute__(profilesPath, "SQUISH_ENV_MODIFICATION", "")


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
            if ff not in omitFiles:
                shutil.copy(os.path.join(r, ff), currentPath)
    if platform.system() in ('Linux', 'Darwin'):
        substituteTildeWithinToolchains(tmpSettingsDir)
        substituteTildeWithinQtVersion(tmpSettingsDir)
        substituteDefaultCompiler(tmpSettingsDir)
    elif platform.system() in ('Windows', 'Microsoft'):
        substituteCdb(tmpSettingsDir)
        substituteMsvcPaths(tmpSettingsDir, '2017', 64)
        substituteMsvcPaths(tmpSettingsDir, '2017', 32)
        substituteMsvcPaths(tmpSettingsDir, '2019', 64)
        prependWindowsKit(tmpSettingsDir, 32)
    substituteOnlineInstallerPath(tmpSettingsDir)
    SettingsPath = ['-settingspath', '"%s"' % tmpSettingsDir]

test.log("Test is running on Python %s" % sys.version)
# current dir is directory holding qtcreator.py
origSettingsDir = os.path.abspath(os.path.join(os.getcwd(), "..", "..", "settings"))

if platform.system() in ('Windows', 'Microsoft'):
    origSettingsDir = os.path.join(origSettingsDir, "windows")
elif platform.system() == 'Darwin':
    origSettingsDir = os.path.join(origSettingsDir, "mac")
else:
    origSettingsDir = os.path.join(origSettingsDir, "unix")

srcPath = os.getenv("SYSTEST_SRCPATH", os.path.expanduser(os.path.join("~", "squish-data")))

# the following only doesn't work if the test ends in an exception
if os.getenv("SYSTEST_NOSETTINGSPATH") != "1":
    copySettingsToTmpDir()
    atexit.register(__removeTestingDir__)

if os.getenv("SYSTEST_WRITE_RESULTS") == "1" and os.getenv("SYSTEST_RESULTS_FOLDER") != None:
    atexit.register(writeTestResults, os.getenv("SYSTEST_RESULTS_FOLDER"))
