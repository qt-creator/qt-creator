#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

def startCreatorTryingClang():
    try:
        # start Qt Creator with enabled ClangCodeModel plugin (without modifying settings)
        startApplication("qtcreator -load ClangCodeModel" + SettingsPath)
        errorMsg = "{type='QMessageBox' unnamed='1' visible='1' windowTitle='Qt Creator'}"
        errorOK = "{text='OK' type='QPushButton' unnamed='1' visible='1' window=%s}" % errorMsg
        if waitFor("object.exists(errorOK)", 5000):
            clickButton(errorOK) # Error message
            clickButton(errorOK) # Help message
            raise Exception("ClangCodeModel not found.")
        return True
    except:
        # ClangCodeModel plugin has not been built - start without it
        test.warning("ClangCodeModel plugin not available - performing test without.")
        startApplication("qtcreator" + SettingsPath)
        return False

def __openCodeModelOptions__():
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "C++")
    clickItem(":Options_QListView", "C++", 14, 15, 0, Qt.LeftButton)
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Code Model")

def iterateAvailableCodeModels():
    __openCodeModelOptions__()
    cppChooser = findObject("{type='QComboBox' name='cppChooser' visible='1'}")
    models = [str(cppChooser.currentText)] # Make sure default is first in list
    if cppChooser.count > 1:
        furtherModels = dumpItems(cppChooser.model())
        furtherModels.remove(models[0])
        models.extend(furtherModels)
    clickButton(waitForObject(":Options.OK_QPushButton"))
    return models

def selectCodeModel(codeModel):
    __openCodeModelOptions__()
    expectedObjNames = ['cChooser', 'cppChooser', 'objcChooser', 'objcppChooser', 'hChooser']
    for exp in expectedObjNames:
        test.verify(checkIfObjectExists("{type='QComboBox' name='%s' visible='1'}" % exp),
                    "Verifying whether combobox '%s' exists." % exp)
        combo = findObject("{type='QComboBox' name='%s' visible='1'}" % exp)
        try:
            selectFromCombo(combo, codeModel)
        except:
            test.fatal("Could not find code model '%s'. Canceling dialog." % codeModel)
            clickButton(waitForObject(":Options.Cancel_QPushButton"))
            return
    test.verify(verifyChecked("{name='ignorePCHCheckBox' type='QCheckBox' visible='1'}"),
                "Verifying whether 'Ignore pre-compiled headers' is checked by default.")
    clickButton(waitForObject(":Options.OK_QPushButton"))
