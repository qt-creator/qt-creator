# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

firstStart = True

def startCreatorVerifyingClang(useClang):
    global firstStart
    try:
        # start Qt Creator with / without enabled ClangCodeModel plugin (without modifying settings)
        loadOrNoLoad = '-load' if useClang else '-noload'
        startQC([loadOrNoLoad, 'ClangCodeModel'], closeLinkToQt=firstStart, cancelTour=firstStart)
        firstStart = False
    except RuntimeError:
        t, v = sys.exc_info()[:2]
        strv = str(v)
        if strv == "startApplication() failed":
            test.fatal("%s. Creator built without ClangCodeModel?" % strv)
        else:
            test.fatal("Exception caught", "%s: %s" % (t.__name__, strv))
        return False
    if platform.system() not in ('Microsoft', 'Windows'): # only Win uses dialogs for this
        return startedWithoutPluginError()
    errorMsg = "{type='QMessageBox' unnamed='1' visible='1' windowTitle='Qt Creator'}"
    errorOK = "{text='OK' type='QPushButton' unnamed='1' visible='1' window=%s}" % errorMsg
    if not waitFor("object.exists(errorOK)", 5000):
        return startedWithoutPluginError()
    clickButton(errorOK) # Error message
    clickButton(errorOK) # Help message
    test.fatal("ClangCodeModel plugin not available.")
    return False

def __openCodeModelOptions__():
    invokeMenuItem("Edit", "Preferences...")
    mouseClick(waitForObjectItem(":Options_QListView", "C++"))
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Code Model")

def getCodeModelString(useClang):
    codeModelName = "built-in"
    if useClang:
        codeModelName = "Clang"
    return "Testing code model: %s" % codeModelName

def checkCodeModelSettings(useClang):
    __openCodeModelOptions__()
    test.log("Verifying whether 'Ignore pre-compiled headers' is unchecked by default.")
    verifyChecked("{text='Ignore precompiled headers' type='QCheckBox' visible='1'}", False)
    clickButton(waitForObject(":Options.OK_QPushButton"))
