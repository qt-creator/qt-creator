# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

currentSelectedTreeItem = None
sectionInProgress = None
genericDebuggers = []
warningOrError = re.compile('<p><b>((Error|Warning).*?)</p>')

def main():
    global appContext
    emptySettings = tempDir()
    __createMinimumIni__(emptySettings)
    appContext = startQC(['-settingspath', '"%s"' % emptySettings], False)
    if not startedWithoutPluginError():
        return
    invokeMenuItem("Edit", "Preferences...")
    __checkKits__()
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

def __checkKits__():
    global genericDebuggers
    mouseClick(waitForObjectItem(":Options_QListView", "Kits"))
    # check compilers
    expectedCompilers = __getExpectedCompilers__()
    llvmForBuild = os.getenv("SYSTEST_LLVM_FROM_BUILD", None)
    if llvmForBuild is not None:
        internalClangExe = os.path.join(llvmForBuild, "bin", "clang")
        if platform.system() in ("Microsoft", "Windows"):
            internalClangExe += ".exe"
        internalClangExe = os.path.realpath(internalClangExe) # clean symlinks
        if os.path.exists(internalClangExe):
            expectedCompilers.append(internalClangExe)
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
    if not test.compare(len(genericDebuggers), 2, "Verifying generic debugger count."):
        test.log(str(genericDebuggers))
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
    global currentSelectedTreeItem, sectionInProgress
    model = waitForObject(treeObjStr).model()
    # 1st row: Auto-detected, 2nd row: Manual (Debugger has additional section Generic prepended)
    for sect in dumpIndices(model):
        sectionInProgress = str(sect.text)
        doneItems = []
        parentModelIndex = "%s container='%s'}" % (objectMap.realName(sect)[:-1], treeObjStr)
        __processSubItems__(treeObjStr, sect, parentModelIndex, doneItems,
                            additionalFunc, *additionalParameters)
    sectionInProgress = None

def __compFunc__(it, foundComp, foundCompNames):
    # skip sub section items (will continue on its children)
    if str(it) == "C" or str(it) == "C++":
        return
    try:
        waitFor("object.exists(':Path.Utils_BaseValidatingLineEdit')", 1000)
        pathLineEdit = findObject(":Path.Utils_BaseValidatingLineEdit")
        foundComp.append(str(pathLineEdit.text))
    except:
        varsBatComboStr = "{name='varsBatCombo' type='QComboBox' visible='1'}"
        varsBatCombo = waitForObjectExists(varsBatComboStr)
        parameterComboStr = "{type='QComboBox' visible='1' unnamed='1' leftWidget=%s}" % varsBatComboStr
        try:
            parameterCombo = findObject(parameterComboStr)
            parameter = ' ' + str(parameterCombo.currentText)
        except:
            parameter = ''
        foundComp.append({it:str(varsBatCombo.currentText) + parameter})

    foundCompNames.append(it)

def __dbgFunc__(it, foundDbg):
    global sectionInProgress, genericDebuggers
    waitFor("object.exists(':Path.Utils_BaseValidatingLineEdit')", 2000)
    pathLineEdit = findObject(":Path.Utils_BaseValidatingLineEdit")
    if sectionInProgress == 'Generic':
        debugger = str(pathLineEdit.text)
        test.verify(debugger == 'gdb' or debugger == 'lldb',
                    'Verifying generic debugger is GDB or LLDB.')
        genericDebuggers.append(debugger)
    else:
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
    qtVersionStr = str(waitForObjectExists(":Kits_QtVersion_QComboBox").currentText)
    # The following may fail if Creator doesn't find a Qt version in PATH. It will then create one
    # Qt-less kit for each available toolchain instead of just one default Desktop kit.
    # Since Qt usually is in PATH on the test machines anyway, we consider this too much of a
    # corner case to add error handling code or make Qt in PATH a hard requirement for the tests.
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

def __LLVMInRegistry__():
    # following only works on Win64 (Win32 has different registry keys)
    try:
        output = subprocess.check_output(['reg', 'query',
                                          r'HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\LLVM\LLVM'],
                                          shell=True)
        for line in output.splitlines():
            if '(Default)' in line:
                path = line.split(' REG_SZ ')[1].strip()
                if os.path.exists(os.path.join(path, 'bin', 'clang-cl.exe')):
                    return True
    except subprocess.CalledProcessError:
        pass
    return False

def __getExpectedCompilers__():
    # TODO: enhance this to distinguish between C and C++ compilers
    expected = []
    if platform.system() in ('Microsoft', 'Windows'):
        expected.extend(__getWinCompilers__())
    compilers = ["g++", "gcc"]
    if platform.system() in ('Linux', 'Darwin'):
        for c in ('clang++', 'clang', 'afl-clang',
                  'clang-[0-9]', 'clang-[0-9].[0-9]', 'clang-1[0-9]', 'clang-1[0-9].[0-9]',
                  '*g++*', '*gcc*'):
            filesInPath = set(findAllFilesInPATH(c))
            compilers.extend(filesInPath | set(map(os.path.realpath, filesInPath)))
    if platform.system() == 'Darwin':
        for compilerExe in ('clang++', 'clang'):
            xcodeClang = getOutputFromCmdline(["xcrun", "--find", compilerExe]).strip("\n")
            if xcodeClang and os.path.exists(xcodeClang) and xcodeClang not in expected:
                expected.append(xcodeClang)

    if platform.system() in ('Microsoft', 'Windows'):
        clangClInPath = len(findAllFilesInPATH('clang-cl.exe'))
        if clangClInPath > 0 or __LLVMInRegistry__():
            expected.append({'^LLVM \d{2} bit based on MSVC\d{4}$' : ''})

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
            displayName = testData.field(record, 'displayName')
            if len(idePath):
                if not os.path.exists(os.path.abspath(os.path.join(envvar, idePath))):
                    continue
            if testData.field(record, "isSDK") == "true":
                for para, used in zip(parameters, usedParameters):
                    result.append(
                                  {"%s \(.*?\) \(%s\)" % (displayName, para)
                                   : "%s %s" % (compiler, used)})
            else:
                for para, used in zip(parameters, usedParameters):
                    if "[.0-9]+" in displayName:
                        result.append({"%s \(%s\)" % (displayName, para)
                                       : "%s %s" % (compiler, used)})
                    else:
                        result.append({"%s (%s)" % (displayName, para)
                                       : "%s %s" % (compiler, used)})
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
        explicitlyOmitted = ("lldb-platform", "lldb-gdbserver", "lldb-instr", "lldb-argdumper",
                             "lldb-server", "lldb-vscode")
        result.extend(filter(lambda s: not (any(omitted in s for omitted in explicitlyOmitted)),
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
                         "C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64",
                         "C:\\Program Files\\Windows Kits\\10\\Debuggers\\x86",
                         "C:\\Program Files\\Windows Kits\\10\\Debuggers\\x64"]
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
                if isString(currentExp):
                    continue
                key = next(iter(currentExp.keys()))
                # the regex .*? is used for the different possible version strings of the WinSDK
                # if it's present a regex will be validated otherwise simple string comparison
                # same applies for [.0-9]+ which is used for minor/patch versions
                isRegex = ".*?" in key or "[.0-9]+" in key
                if (((isRegex and re.match(key, next(iter(currentFound.keys())), flags)))
                    or currentFound.keys() == currentExp.keys()):
                    if ((isWin and os.path.abspath(next(iter(currentFound.values())).lower())
                         == os.path.abspath(next(iter(currentExp.values())).lower()))
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
        if isString(it):
            yield it.lower()
        else:
            yield it

def __checkCreatedSettings__(settingsFolder):
    waitForCleanShutdown()
    qtProj = os.path.join(settingsFolder, "QtProject")
    creatorFolder = os.path.join(qtProj, "qtcreator")
    folders = [creatorFolder,
               os.path.join(creatorFolder, "generic-highlighter"),
               os.path.join(creatorFolder, "macros")]
    files = {os.path.join(qtProj, "QtCreator.db"):0,
             os.path.join(qtProj, "QtCreator.ini"):30,
             os.path.join(creatorFolder, "debuggers.xml"):0,
             os.path.join(creatorFolder, "devices.xml"):0,
             os.path.join(creatorFolder, "helpcollection.qhc"):0,
             os.path.join(creatorFolder, "profiles.xml"):0,
             os.path.join(creatorFolder, "qtversion.xml"):0,
             os.path.join(creatorFolder, "toolchains.xml"):0}
    for f in folders:
        test.verify(os.path.isdir(f),
                    "Verifying whether folder '%s' has been created." % os.path.basename(f))
    for fName, fMinSize in files.items():
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
