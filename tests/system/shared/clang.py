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

firstStart = True

def startCreatorVerifyingClang(useClang):
    global firstStart
    try:
        # start Qt Creator with / without enabled ClangCodeModel plugin (without modifying settings)
        loadOrNoLoad = '-load' if useClang else '-noload'
        startQC([loadOrNoLoad, 'ClangCodeModel'], cancelTour=firstStart)
        firstStart = False
    except RuntimeError:
        t, v = sys.exc_info()[:2]
        strv = str(v)
        if strv == "startApplication() failed":
            test.fatal("%s. Creator built without ClangCodeModel?" % strv)
        else:
            test.fatal("Exception caught", "%s(%s)" % (str(t), strv))
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
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "C++")
    clickItem(":Options_QListView", "C++", 14, 15, 0, Qt.LeftButton)
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Code Model")

def getCodeModelString(useClang):
    codeModelName = "built-in"
    if useClang:
        codeModelName = "Clang"
    return "Testing code model: %s" % codeModelName

def checkCodeModelSettings(useClang):
    __openCodeModelOptions__()
    test.log("Verifying whether 'Ignore pre-compiled headers' is unchecked by default.")
    verifyChecked("{name='ignorePCHCheckBox' type='QCheckBox' visible='1'}", False)
    clickButton(waitForObject(":Options.OK_QPushButton"))
