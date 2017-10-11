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

source("../../shared/qtcreator.py")

import re
import tempfile
import __builtin__

currentSelectedTreeItem = None
warningOrError = re.compile('<p><b>((Error|Warning).*?)</p>')

def main():
    emptySettings = tempDir()
    __createMinimumIni__(emptySettings)
    SettingsPath = ' -settingspath "%s"' % emptySettings
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    invokeMenuItem("Tools", "Options...")
    __checkBuildAndRun__()
    clickButton(waitForObject(":Options.Cancel_QPushButton"))
    invokeMenuItem("File", "Exit")
    __checkCreatedSettings__(emptySettings)

def __createMinimumIni__(emptyParent):
    qtProjDir = os.path.join(emptyParent, "QtProject")
    os.mkdir(qtProjDir)
    iniFile = open(os.path.join(qtProjDir, "QtCreator.ini"), "w")
    iniFile.write("[%General]\n")
    iniFile.write("OverrideLanguage=C\n")
    iniFile.close()

def __checkBuildAndRun__():
    waitForObjectItem(":Options_QListView", "Build & Run")
    clickItem(":Options_QListView", "Build & Run", 14, 15, 0, Qt.LeftButton)
    # check compilers
    expectedCompilers = __getExpectedCompilers__()
    foundCompilers = []
    foundCompilerNames = []
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Compilers")
    __iterateTree__(":BuildAndRun_QTreeView", __compFunc__, foundCompilers, foundCompilerNames)
    test.verify(__compareCompilers__(foundCompilers, expectedCompilers),
                "Verifying found and expected compilers are equal.")
    # check debugger
    expectedDebuggers = __getExpectedDebuggers__()
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Debuggers")
    foundDebugger = []
    __iterateTree__(":BuildAndRun_QTreeView", __dbgFunc__, foundDebugger)
    test.verify(__compareDebuggers__(foundDebugger, expectedDebuggers),
                "Verifying found and expected debuggers are equal.")
    # check Qt versions
    qmakePath = which("qmake")
    foundQt = []
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Qt Versions")
    __iterateTree__(":qtdirList_QTreeView", __qtFunc__, foundQt, qmakePath)
    test.verify(not qmakePath or len(foundQt) == 1,
                "Was qmake from %s autodetected? Found %s" % (qmakePath, foundQt))
    if foundQt:
        foundQt = foundQt[0]    # qmake from "which" should be used in kits
    # check kits
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Kits")
    __iterateTree__(":BuildAndRun_QTreeView", __kitFunc__, foundQt, foundCompilerNames)

def __processSubItems__(treeObjStr, section, parModelIndexStr, doneItems,
                        additionalFunc, *additionalParameters):
    global currentSelectedTreeItem
    tree = waitForObject(treeObjStr)
    model = tree.model()
    items = dumpIndices(model, section)
    for it in items:
        indexName = str(it.data().toString())
        itObj = "%s container=%s}" % (objectMap.realName(it)[:-1], parModelIndexStr)
        alreadyDone = doneItems.count(itObj)
        doneItems.append(itObj)
        if alreadyDone:
            itObj = "%s occurrence='%d'}" % (itObj[:-1], alreadyDone + 1)
        currentSelectedTreeItem = waitForObject(itObj, 3000)
        tree.scrollTo(it)
        mouseClick(currentSelectedTreeItem, 5, 5, 0, Qt.LeftButton)
        additionalFunc(indexName, *additionalParameters)
        currentSelectedTreeItem = None
        if model.rowCount(it) > 0:
            __processSubItems__(treeObjStr, it, itObj, doneItems,
                                additionalFunc, *additionalParameters)

def __iterateTree__(treeObjStr, additionalFunc, *additionalParameters):
    global currentSelectedTreeItem
    model = waitForObject(treeObjStr).model()
    # 1st row: Auto-detected, 2nd row: Manual
    for sect in dumpIndices(model):
        doneItems = []
        parentModelIndex = "%s container='%s'}" % (objectMap.realName(sect)[:-1], treeObjStr)
        __processSubItems__(treeObjStr, sect, parentModelIndex, doneItems,
                            additionalFunc, *additionalParameters)

def __compFunc__(it, foundComp, foundCompNames):
    # skip sub section items (will continue on its children)
    if str(it) == "C" or str(it) == "C++":
        return
    try:
        waitFor("object.exists(':Path.Utils_BaseValidatingLineEdit')", 1000)
        pathLineEdit = findObject(":Path.Utils_BaseValidatingLineEdit")
        foundComp.append(str(pathLineEdit.text))
    except:
        label = findObject("{buddy={container=':qt_tabwidget_stackedwidget_QWidget' "
                           "text='Initialization:' type='QLabel' unnamed='1' visible='1'} "
                           "type='QLabel' unnamed='1' visible='1'}")
        foundComp.append({it:str(label.text)})
    foundCompNames.append(it)

def __dbgFunc__(it, foundDbg):
    waitFor("object.exists(':Path.Utils_BaseValidatingLineEdit')", 2000)
    pathLineEdit = findObject(":Path.Utils_BaseValidatingLineEdit")
    foundDbg.append(str(pathLineEdit.text))

def __qtFunc__(it, foundQt, qmakePath):
    qtPath = str(waitForObject(":QtSupport__Internal__QtVersionManager.qmake_QLabel").text)
    if platform.system() in ('Microsoft', 'Windows'):
        qtPath = qtPath.lower()
        qmakePath = qmakePath.lower()
    test.verify(os.path.isfile(qtPath) and os.access(qtPath, os.X_OK),
                "Verifying found Qt (%s) is executable." % qtPath)
    # Two Qt versions will be found when using qtchooser: QTCREATORBUG-14697
    # Only add qmake from "which" to list
    if qtPath == qmakePath:
        foundQt.append(it)
    try:
        errorLabel = findObject(":QtSupport__Internal__QtVersionManager.errorLabel.QLabel")
        test.warning("Detected error or warning: '%s'" % errorLabel.text)
    except:
        pass

def __kitFunc__(it, foundQt, foundCompNames):
    global currentSelectedTreeItem, warningOrError
    qtVersionStr = str(waitForObject(":Kits_QtVersion_QComboBox").currentText)
    test.compare(it, "Desktop (default)", "Verifying whether default Desktop kit has been created.")
    if foundQt:
        test.compare(qtVersionStr, foundQt, "Verifying if Qt versions match.")
    cCompilerCombo = findObject(":CCompiler:_QComboBox")
    test.compare(cCompilerCombo.enabled, cCompilerCombo.count > 1,
                 "Verifying whether C compiler combo is enabled/disabled correctly.")
    cppCompilerCombo = findObject(":CppCompiler:_QComboBox")
    test.compare(cppCompilerCombo.enabled, cppCompilerCombo.count > 1,
                 "Verifying whether C++ compiler combo is enabled/disabled correctly.")

    test.verify(str(cCompilerCombo.currentText) in foundCompNames,
                "Verifying if one of the found C compilers had been set.")
    test.verify(str(cppCompilerCombo.currentText) in foundCompNames,
                "Verifying if one of the found C++ compilers had been set.")
    if currentSelectedTreeItem:
        foundWarningOrError = warningOrError.search(str(currentSelectedTreeItem.toolTip))
        if foundWarningOrError:
            details = str(foundWarningOrError.group(1)).replace("<br>", "\n")
            details = details.replace("<b>", "").replace("</b>", "")
            test.warning("Detected error and/or warning: %s" % details)

def __getExpectedCompilers__():
    # TODO: enhance this to distinguish between C and C++ compilers
    expected = []
    if platform.system() in ('Microsoft', 'Windows'):
        expected.extend(__getWinCompilers__())
    compilers = ["g++", "gcc"]
    if platform.system() in ('Linux', 'Darwin'):
        compilers.extend(["clang++", "clang"])
        compilers.extend(findAllFilesInPATH("*g++*"))
        compilers.extend(findAllFilesInPATH("*gcc*"))
    if platform.system() == 'Darwin':
        xcodeClang = getOutputFromCmdline(["xcrun", "--find", "clang++"]).strip("\n")
        if xcodeClang and os.path.exists(xcodeClang) and xcodeClang not in expected:
            expected.append(xcodeClang)
    for compiler in compilers:
        compilerPath = which(compiler)
        if compilerPath:
            if compiler.endswith('clang++') or compiler.endswith('clang'):
                if subprocess.call([compiler, '-dumpmachine']) != 0:
                    test.warning("clang found in PATH, but version is not supported.")
                    continue
            expected.append(compilerPath)
    return expected

def __getWinCompilers__():
    result = []
    for record in testData.dataset("win_compiler_paths.tsv"):
        envvar = os.getenv(testData.field(record, "envvar"))
        if not envvar:
            continue
        compiler = os.path.abspath(os.path.join(envvar, testData.field(record, "path"),
                                                testData.field(record, "file")))
        if os.path.exists(compiler):
            parameters = testData.field(record, "displayedParameters").split(",")
            usedParameters = testData.field(record, "usedParameters").split(",")
            idePath = testData.field(record, "IDEPath")
            if len(idePath):
                if not os.path.exists(os.path.abspath(os.path.join(envvar, idePath))):
                    continue
            if testData.field(record, "isSDK") == "true":
                for para, used in zip(parameters, usedParameters):
                    result.append(
                                  {"%s \(.*?\) \(%s\)" % (testData.field(record, 'displayName'),
                                                          para)
                                   :"%s %s" % (compiler, used)})
            else:
                for para, used in zip(parameters, usedParameters):
                    result.append({"%s (%s)" % (testData.field(record, 'displayName'), para)
                                   :"%s %s" % (compiler, used)})
    return result

def __getExpectedDebuggers__():
    exeSuffix = ""
    result = []
    if platform.system() in ('Microsoft', 'Windows'):
        result.extend(__getCDB__())
        exeSuffix = ".exe"
    for debugger in ["gdb", "lldb"]:
        result.extend(findAllFilesInPATH(debugger + exeSuffix))
    if platform.system() == 'Linux':
        result.extend(filter(lambda s: not ("lldb-platform" in s or "lldb-gdbserver" in s),
                             findAllFilesInPATH("lldb-*")))
    if platform.system() == 'Darwin':
        xcodeLLDB = getOutputFromCmdline(["xcrun", "--find", "lldb"]).strip("\n")
        if xcodeLLDB and os.path.exists(xcodeLLDB) and xcodeLLDB not in result:
            result.append(xcodeLLDB)
    return result

def __getCDB__():
    result = []
    possibleLocations = ["C:\\Program Files\\Debugging Tools for Windows (x64)",
                         "C:\\Program Files (x86)\\Debugging Tools for Windows (x86)",
                         "C:\\Program Files (x86)\\Windows Kits\\8.0\\Debuggers\\x86",
                         "C:\\Program Files (x86)\\Windows Kits\\8.0\\Debuggers\\x64",
                         "C:\\Program Files\\Windows Kits\\8.0\\Debuggers\\x86",
                         "C:\\Program Files\\Windows Kits\\8.0\\Debuggers\\x64",
                         "C:\\Program Files (x86)\\Windows Kits\\8.1\\Debuggers\\x86",
                         "C:\\Program Files (x86)\\Windows Kits\\8.1\\Debuggers\\x64",
                         "C:\\Program Files\\Windows Kits\\8.1\\Debuggers\\x86",
                         "C:\\Program Files\\Windows Kits\\8.1\\Debuggers\\x64",
                         "C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x86",
                         "C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64"]
    for cdbPath in possibleLocations:
        cdb = os.path.join(cdbPath, "cdb.exe")
        if os.path.exists(cdb):
            result.append(cdb)
    return result

def __compareCompilers__(foundCompilers, expectedCompilers):
    # TODO: Check if all expected compilers were found
    equal = True
    flags = 0
    isWin = platform.system() in ('Microsoft', 'Windows')
    if isWin:
        flags = re.IGNORECASE
    for currentFound in foundCompilers:
        if isinstance(currentFound, dict):
            foundExp = False
            for currentExp in expectedCompilers:
                if isinstance(currentExp, (str, unicode)):
                    continue
                key = currentExp.keys()[0]
                # the regex .*? is used for the different possible version strings of the WinSDK
                # if it's present a regex will be validated otherwise simple string comparison
                if (((".*?" in key and re.match(key, currentFound.keys()[0], flags))
                    or currentFound.keys() == currentExp.keys())):
                    if ((isWin and os.path.abspath(currentFound.values()[0].lower())
                         == os.path.abspath(currentExp.values()[0].lower()))
                        or currentFound.values() == currentExp.values()):
                        foundExp = True
                        break
            equal = foundExp
        else:
            if isWin:
                equal = currentFound.lower() in __lowerStrs__(expectedCompilers)
            else:
                equal = currentFound in expectedCompilers
        if not equal:
            test.fail("Found '%s' but was not expected." % str(currentFound),
                      str(expectedCompilers))
            break
    return equal

def __compareDebuggers__(foundDebuggers, expectedDebuggers):
    if not len(foundDebuggers) == len(expectedDebuggers):
        test.log("Number of found and expected debuggers do not match.",
                 "Found: %s\nExpected: %s" % (str(foundDebuggers), str(expectedDebuggers)))
        return False
    if platform.system() in ('Microsoft', 'Windows'):
        foundSet = set(__lowerStrs__(foundDebuggers))
        expectedSet = set(__lowerStrs__(expectedDebuggers))
    else:
        foundSet = set(foundDebuggers)
        expectedSet = set(expectedDebuggers)
    return test.compare(foundSet, expectedSet,
                        "Verifying expected and found debuggers match.")

def __lowerStrs__(iterable):
    for it in iterable:
        if isinstance(it, (str, unicode)):
            yield it.lower()
        else:
            yield it

def __checkCreatedSettings__(settingsFolder):
    waitForCleanShutdown()
    qtProj = os.path.join(settingsFolder, "QtProject")
    folders = []
    files = [{os.path.join(qtProj, "QtCreator.db"):0},
             {os.path.join(qtProj, "QtCreator.ini"):30}]
    folders.append(os.path.join(qtProj, "qtcreator"))
    files.extend([{os.path.join(folders[0], "debuggers.xml"):0},
                  {os.path.join(folders[0], "devices.xml"):0},
                  {os.path.join(folders[0], "helpcollection.qhc"):0},
                  {os.path.join(folders[0], "profiles.xml"):0},
                  {os.path.join(folders[0], "qtversion.xml"):0},
                  {os.path.join(folders[0], "toolchains.xml"):0}])
    folders.extend([os.path.join(folders[0], "generic-highlighter"),
                    os.path.join(folders[0], "macros")])
    for f in folders:
        test.verify(os.path.isdir(f),
                    "Verifying whether folder '%s' has been created." % os.path.basename(f))
    for f in files:
        fName = f.keys()[0]
        fMinSize = f.values()[0]
        text = "created non-empty"
        if fMinSize > 0:
            text = "modified"
        test.verify(os.path.isfile(fName) and os.path.getsize(fName) > fMinSize,
                    "Verifying whether file '%s' has been %s." % (os.path.basename(fName), text))

def findAllFilesInPATH(programGlob):
    result = []
    for path in os.environ["PATH"].split(os.pathsep):
        for curr in glob.glob(os.path.join(path, programGlob)):
            if os.path.isfile(curr) and os.access(curr, os.X_OK):
                result.append(curr)
    return result
