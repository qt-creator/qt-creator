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

source("../../shared/qtcreator.py")

def handleInsertVirtualFunctions(expected):
    treeView = waitForObject("{container={title='Functions to insert:' type='QGroupBox' unnamed='1'"
                             " visible='1'} type='QTreeView' unnamed='1' visible='1'}")

    model = treeView.model()
    classIndices = dumpIndices(model, treeView.rootIndex())
    found = set()
    isChecked = lambda ch: model.data(ch, Qt.CheckStateRole).toInt() == Qt.Checked

    for classIndex in classIndices:
        if model.hasChildren(classIndex):
            for child in dumpIndices(model, classIndex):
                for curr in expected:
                    if str(child.text).startswith(curr):
                        if test.verify(isChecked(child), "Verifying: '%s' is checked." % curr):
                            found.add(curr)
                        else:
                            item = "%s.%s" % (str(classIndex.text), str(child.text))
                            test.log("Checking '%s'." % item)
                            clickItem(treeView, item.replace("_", "\\_"), 5, 5, 0, Qt.LeftButton)
                            waitFor("isChecked(child)", 1000)

    test.verify(len(set(expected).difference(found)) == 0,
                "Verifying whether all expected functions have been found.")

    selectFromCombo("{container={title='Insertion options:' type='QGroupBox' unnamed='1' "
                    " visible='1'} occurrence='2' type='QComboBox' unnamed='1' visible='1'}",
                    "Insert definitions in implementation file")
    clickButton("{text='OK' type='QPushButton' unnamed='1' visible='1'}")

def checkSimpleCppLib(projectName, static):
    checkedTargets, projectName, className = createNewCPPLib(tempDir(), projectName, "MyClass",
                                                             target=Targets.desktopTargetClasses(),
                                                             isStatic=static)
    for kit, config in iterateBuildConfigs(len(checkedTargets), "Release"):
        verifyBuildConfig(len(checkedTargets), kit, config, False, True)
        invokeMenuItem('Build', 'Build Project "%s"' % projectName)
        waitForCompile(10000)
        checkCompile()

def addReturn(editor, toFunction, returnValue):
    placeCursorToLine(editor, toFunction, True)
    type(editor, "<Down>")
    type(editor, "<Return>")
    type(editor, "return %s;" % returnValue)

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return

    checkSimpleCppLib("SampleApp1", False)
    checkSimpleCppLib("SampleApp2", True)

    # Qt Plugin needs Qt4.8 for QGenericPlugin which is tested by default
    targets = Targets.desktopTargetClasses() & ~Targets.DESKTOP_474_GCC
    checkedTargets, projectName, className = createNewQtPlugin(tempDir(), "SampleApp3", "MyPlugin",
                                                               target=targets)
    virtualFunctionsAdded = False
    for kit, config in iterateBuildConfigs(len(checkedTargets), "Debug"):
        verifyBuildConfig(len(checkedTargets), kit, config, True, True)
        if (virtualFunctionsAdded and platform.system() in ('Microsoft', 'Windows')
            and "480" in Targets.getStringForTarget(checkedTargets[kit])):
            test.warning("Skipping building of Qt4.8 targets because of QTCREATORBUG-12251.")
            continue
        invokeMenuItem('Build', 'Build Project "%s"' % projectName)
        waitForCompile(10000)
        if not virtualFunctionsAdded:
            checkLastBuild(True, False)
            if not openDocument("%s.Headers.%s\.h" % (projectName, className.lower())):
                test.fail("Could not open %s.h - continuing." % className.lower())
                continue
            editor = getEditorForFileSuffix("%s.h" % className.lower())
            oldContent = str(editor.plainText)
            placeCursorToLine(editor, "class %s.*" % className, True)
            snooze(1) # avoid timing issue with the parser
            invokeContextMenuItem(editor, "Refactor", "Insert Virtual Functions of Base Classes")
            handleInsertVirtualFunctions(["keys() const = 0 : QStringList",
                                          "create(const QString &, const QString &) = 0 : QObject *"])
            waitFor("'keys' in str(editor.plainText)", 2000)
            modifiedContent = str(editor.plainText)
            test.verify(re.search("QStringList keys.*;", modifiedContent, re.MULTILINE),
                        "Verifying whether keys() declaration has been added to the header.")
            test.verify(re.search("QObject \*create.*;", modifiedContent, re.MULTILINE),
                        "Verifying whether create() declaration has been added to the header.")

            if not openDocument("%s.Sources.%s\.cpp" % (projectName, className.lower())):
                test.fail("Could not open %s.cpp - continuing." % className.lower())
                continue
            editor = getEditorForFileSuffix("%s.cpp" % className.lower())
            modifiedContent = str(editor.plainText)
            test.verify("QStringList %s::keys(" % className in modifiedContent,
                        "Verifying whether keys() definition has been added to the source file.")
            test.verify("QObject *%s::create(" % className in modifiedContent,
                        "Verifying whether create() definition has been added to the source file.")
            # add return to not run into build issues of missing return values
            addReturn(editor, "QStringList %s::keys.*" % className, "QStringList()")
            addReturn(editor, "QObject \*%s::create.*" % className, "0")
            virtualFunctionsAdded = True
            invokeMenuItem('File', 'Save All')
            if (platform.system() in ('Microsoft', 'Windows')  # QTCREATORBUG-12251
                and "480" in Targets.getStringForTarget(checkedTargets[kit])):
                test.warning("Skipping building of Qt4.8 targets because of QTCREATORBUG-12251.")
                continue
            invokeMenuItem('Build', 'Rebuild Project "%s"' % projectName)
            waitForCompile(10000)
        checkCompile()

    invokeMenuItem("File", "Exit")
