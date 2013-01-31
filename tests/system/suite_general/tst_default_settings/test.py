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
    clickTab(waitForObject(":Options.qt_tabwidget_tabbar_QTabBar"), "Compilers")
    compilerTV = waitForObject(":Kits_Or_Compilers_QTreeView")
    __iterateTree__(compilerTV, __compFunc__, foundCompilers, foundCompilerNames)
    test.verify(__compareCompilers__(foundCompilers, expectedCompilers),
                "Verifying found and expected compilers are equal.")
    # check Qt versions
    qmakePath = which("qmake")
    foundQt = []
    clickTab(waitForObject(":Options.qt_tabwidget_tabbar_QTabBar"), "Qt Versions")
    qtTW = waitForObject(":QtSupport__Internal__QtVersionManager.qtdirList_QTreeWidget")
    __iterateTree__(qtTW, __qtFunc__, foundQt, qmakePath)
    if foundQt:
        foundQt = foundQt[0]
    # check kits
    clickTab(waitForObject(":Options.qt_tabwidget_tabbar_QTabBar"), "Kits")
    kitsTV = waitForObject(":Kits_Or_Compilers_QTreeView")
    __iterateTree__(kitsTV, __kitFunc__, foundQt, foundCompilerNames)

def __iterateTree__(treeObj, additionalFunc, *additionalParameters):
    global currentSelectedTreeItem
    model = treeObj.model()
    # 1st row: Auto-detected, 2nd row: Manual
    for sect in dumpIndices(model):
        sObj = "%s container=%s}" % (objectMap.realName(sect)[:-1], objectMap.realName(treeObj))
        items = dumpIndices(model, sect)
        doneItems = []
        for it in items:
            indexName = str(it.data().toString())
            itObj = "%s container=%s}" % (objectMap.realName(it)[:-1], sObj)
            alreadyDone = doneItems.count(itObj)
            doneItems.append(itObj)
            if alreadyDone:
                itObj = "%s occurrence='%d'}" % (itObj[:-1], alreadyDone + 1)
            currentSelectedTreeItem = waitForObject(itObj, 3000)
            mouseClick(currentSelectedTreeItem, 5, 5, 0, Qt.LeftButton)
            additionalFunc(indexName, *additionalParameters)
            currentSelectedTreeItem = None

def __compFunc__(it, foundComp, foundCompNames):
    try:
        waitFor("object.exists(':CompilerPath.Utils_BaseValidatingLineEdit')", 1000)
        pathLineEdit = findObject(":CompilerPath.Utils_BaseValidatingLineEdit")
        foundComp.append(str(pathLineEdit.text))
    except:
        label = findObject("{buddy={container=':qt_tabwidget_stackedwidget_QWidget' "
                           "text='Initialization:' type='QLabel' unnamed='1' visible='1'} "
                           "type='QLabel' unnamed='1' visible='1'}")
        foundComp.append({it:str(label.text)})
    foundCompNames.append(it)

def __qtFunc__(it, foundQt, qmakePath):
    foundQt.append(it)
    qtPath = str(waitForObject(":QtSupport__Internal__QtVersionManager.qmake_QLabel").text)
    if platform.system() in ('Microsoft', 'Windows'):
        qtPath = qtPath.lower()
        qmakePath = qmakePath.lower()
    test.compare(qtPath, qmakePath, "Verifying found and expected Qt version are equal.")
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
    compilerCombo = waitForObject("{occurrence='3' type='QComboBox' unnamed='1' visible='1' "
                                  "window=':Qt Creator_Core::Internal::MainWindow'}")
    test.verify(str(compilerCombo.currentText) in foundCompNames,
                "Verifying if one of the found compilers had been set.")
    if currentSelectedTreeItem:
        foundWarningOrError = warningOrError.search(str(currentSelectedTreeItem.toolTip))
        if foundWarningOrError:
            details = str(foundWarningOrError.group(1)).replace("<br>", "\n")
            details = details.replace("<b>", "").replace("</b>", "")
            test.warning("Detected error and/or warning: %s" % details)

def __getExpectedCompilers__():
    expected = []
    if platform.system() in ('Microsoft', 'Windows'):
        expected.extend(__getWinCompilers__())
    compilers = ["g++"]
    if platform.system() in ('Linux', 'Darwin'):
        compilers.extend(["g++-4.0", "g++-4.2", "clang++"])
    for compiler in compilers:
        compilerPath = which(compiler)
        if compilerPath:
            if compiler == 'clang++':
                if subprocess.call(['clang++', '-dumpmachine']) != 0:
                    test.warning("clang found in PATH, but version is not supported.")
                    continue
            expected.append(compilerPath)
    return expected

def __getWinCompilers__():
    result = []
    winEnvVars = __getWinEnvVars__()
    for record in testData.dataset("win_compiler_paths.tsv"):
        envvar = winEnvVars.get(testData.field(record, "envvar"), "")
        compiler = os.path.abspath(os.path.join(envvar, testData.field(record, "path"),
                                                testData.field(record, "file")))
        if os.path.exists(compiler):
            parameters = testData.field(record, "displayedParameters").split(",")
            usedParameters = testData.field(record, "usedParameters").split(",")
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

# using os.getenv() or getOutputFromCmdline() do not work - they would return C:\Program Files (x86)
# for %ProgramFiles% as well as for %ProgramFiles(x86)% when using Python 32bit on 64bit machines
def __getWinEnvVars__():
    result = {}
    tmpF, tmpFPath = tempfile.mkstemp()
    envvars = subprocess.call('set', stdout=tmpF, shell=True)
    os.close(tmpF)
    tmpF = open(tmpFPath, "r")
    for line in tmpF:
        tmp = line.split("=")
        result[tmp[0]] = tmp[1]
    tmpF.close()
    os.remove(tmpFPath)
    return result


def __compareCompilers__(foundCompilers, expectedCompilers):
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

def __lowerStrs__(iterable):
    for it in iterable:
        if isinstance(it, (str, unicode)):
            yield it.lower()
        else:
            yield it

def __checkCreatedSettings__(settingsFolder):
    qtProj = os.path.join(settingsFolder, "QtProject")
    folders = []
    files = [{os.path.join(qtProj, "QtCreator.db"):0},
             {os.path.join(qtProj, "QtCreator.ini"):30}]
    folders.append(os.path.join(qtProj, "qtcreator"))
    files.extend([{os.path.join(folders[0], "devices.xml"):0},
                  {os.path.join(folders[0], "helpcollection.qhc"):0},
                  {os.path.join(folders[0], "profiles.xml"):0},
                  {os.path.join(folders[0], "qtversion.xml"):0},
                  {os.path.join(folders[0], "toolchains.xml"):0}])
    folders.extend([os.path.join(folders[0], "generic-highlighter"),
                    os.path.join(folders[0], "json"),
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
