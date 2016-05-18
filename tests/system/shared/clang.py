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

def startCreatorTryingClang():
    try:
        # start Qt Creator with enabled ClangCodeModel plugin (without modifying settings)
        startApplication("qtcreator -load ClangCodeModel" + SettingsPath)
    except RuntimeError:
        t, v = sys.exc_info()[:2]
        strv = str(v)
        if strv == "startApplication() failed":
            test.fatal("%s. Creator built without ClangCodeModel?" % strv)
        else:
            test.fatal("Exception caught", "%s(%s)" % (str(t), strv))
        return False
    errorMsg = "{type='QMessageBox' unnamed='1' visible='1' windowTitle='Qt Creator'}"
    errorOK = "{text='OK' type='QPushButton' unnamed='1' visible='1' window=%s}" % errorMsg
    if not waitFor("object.exists(errorOK)", 5000):
        return True
    clickButton(errorOK) # Error message
    clickButton(errorOK) # Help message
    test.fatal("ClangCodeModel plugin not available.")
    return False

def startCreator(useClang):
    try:
        if useClang:
            if not startCreatorTryingClang():
                return False
        else:
            startApplication("qtcreator" + SettingsPath)
    finally:
        overrideStartApplication()
    return startedWithoutPluginError()

def __openCodeModelOptions__():
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "C++")
    clickItem(":Options_QListView", "C++", 14, 15, 0, Qt.LeftButton)
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Code Model")

def checkCodeModelSettings(useClang):
    codeModelName = "built-in"
    if useClang:
        codeModelName = "Clang"
    test.log("Testing code model: %s" % codeModelName)
    __openCodeModelOptions__()
    test.verify(verifyChecked("{name='ignorePCHCheckBox' type='QCheckBox' visible='1'}"),
                "Verifying whether 'Ignore pre-compiled headers' is checked by default.")
    clickButton(waitForObject(":Options.OK_QPushButton"))
