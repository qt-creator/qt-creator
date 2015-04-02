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
source("Tree.py")

def main():
    if os.getenv("SYSTEST_OPENGL_MISSING") == "1":
        test.xverify(False, "This test needs OpenGL - skipping...")
        return
    projName = "simpleQuickUI2.qmlproject"
    projFolder = os.path.dirname(findFile("testdata", "simpleQuickUI2/%s" % projName))
    if not neededFilePresent(os.path.join(projFolder, projName)):
        return
    qmlProjDir = prepareTemplate(projFolder)
    if qmlProjDir == None:
        test.fatal("Could not prepare test files - leaving test")
        return
    qmlProjFile = os.path.join(qmlProjDir, projName)
    # start Creator by passing a .qmlproject file
    startApplication('qtcreator -load QmlProjectManager' + SettingsPath + ' "%s"' % qmlProjFile)
    if not startedWithoutPluginError():
        return
    waitFor('object.exists(":Qt Creator_Utils::NavigationTreeView")', 10000)
    fancyConfButton = findObject(":*Qt Creator_Core::Internal::FancyToolButton")
    fancyRunButton = findObject(":*Qt Creator.Run_Core::Internal::FancyToolButton")
    fancyDebugButton = findObject(":*Qt Creator.Start Debugging_Core::Internal::FancyToolButton")
    exe, target = getExecutableAndTargetFromToolTip(str(waitForObject(fancyConfButton).toolTip))
    if not (test.verify(fancyRunButton.enabled and fancyDebugButton.enabled,
                        "Verifying Run and Debug are enabled (Qt5 is available).")
            and test.compare(target, Targets.getStringForTarget(Targets.getDefaultKit()),
                             "Verifying selected Target is Qt5.")
            and test.compare(exe, "QML Scene", "Verifying selected executable is QML Scene.")):
        earlyExit("Something went wrong opening Qml project - probably missing Qt5.")
        return
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(1, 0, ProjectSettings.RUN, True)
    ensureChecked("{container=':Qt Creator.scrollArea_QScrollArea' text='Enable QML' "
                  "type='QCheckBox' unnamed='1' visible='1'}")
    switchViewTo(ViewConstants.EDIT)
    if platform.system() in ('Microsoft', 'Windows'):
        qmake = getQtInformationForQmlProject()[3]
        if qmake == None:
            earlyExit("Could not figure out which qmake is used.")
            return
        qmlScenePath = os.path.abspath(os.path.dirname(qmake))
        qmlScene = "qmlscene.exe"
        allowAppThroughWinFW(qmlScenePath, qmlScene, None)
    clickButton(fancyDebugButton)
    locAndExprTV = waitForObject(":Locals and Expressions_Debugger::Internal::WatchTreeView")
    # Locals and Expressions populates treeview only on demand - so the tree must be expanded
    __unfoldTree__()
    items = fetchItems(QModelIndex(), QModelIndex(), locAndExprTV)
    # reduce items to Locals (invisible object)
    items = items.getChild("Inspector")
    if items == None:
        earlyExit("Could not find expected Inspector tree inside Locals and Expressions.")
        return
    # reduce items to outer Rectangle object
    items = items.getChild("Rectangle")
    if items == None:
        earlyExit("Could not find expected Rectangle tree inside Locals and Expressions.")
        return
    checkForEmptyRows(items)
    check = [[None, 0, {"Properties":1, "Rectangle":2, "Text":1}, {"width":"360", "height":"360"}],
             ["Text", 1, {"Properties":1}, {"text":"Check"}],
             ["Rectangle", 1, {"Properties":1}, {"width":"50", "height":"50", "color":"#008000"}],
             ["Rectangle", 2, {"Properties":1}, {"width":"100", "height":"100", "color":"#ff0000"}]
             ]
    for current in check:
        if current[0]:
            subItem = items.getChild(current[0], current[1])
        else:
            subItem = items
        checkForExpectedValues(subItem, current[2], current[3])
    clickButton(waitForObject(':Debugger Toolbar.Exit Debugger_QToolButton', 5000))
    if platform.system() in ('Microsoft', 'Windows'):
        deleteAppFromWinFW(qmlScenePath, qmlScene)
    invokeMenuItem("File", "Exit")

def __unfoldTree__():
    rootIndex = getQModelIndexStr("text='Rectangle'",
                                  ':Locals and Expressions_Debugger::Internal::WatchTreeView')
    if JIRA.isBugStillOpen(14210):
        doubleClick(waitForObject(rootIndex))
    else:
        test.warning("QTCREATORBUG-14210 is not open anymore. Can the workaround be removed?")
    unfoldQModelIndexIncludingProperties(rootIndex)
    if JIRA.isBugStillOpen(14210):
        for item in ["text='Rectangle' occurrence='2'", "text='Rectangle' occurrence='2'", "text='Text'"]:
            # both Rectangles will be clicked because they change their order
            doubleClick(waitForObject(getQModelIndexStr(item, rootIndex)))
            snooze(1)
    subItems = ["text='Rectangle' occurrence='2'", "text='Rectangle'", "text='Text'"]
    for item in subItems:
        unfoldQModelIndexIncludingProperties(getQModelIndexStr(item, rootIndex))

def unfoldQModelIndexIncludingProperties(indexStr):
    doubleClick(waitForObject(indexStr))
    propIndex = getQModelIndexStr("text='Properties'", indexStr)
    doubleClick(waitForObject(propIndex))

def fetchItems(index, valIndex, treeView):
    tree = Tree()
    model = treeView.model()
    if index.isValid():
            name = str(model.data(index).toString())
            value = str(model.data(valIndex).toString())
            tree.setName(name)
            tree.setValue(value)
    for row in range(model.rowCount(index)):
         tree.addChild(fetchItems(model.index(row, 0, index), model.index(row, 1, index), treeView))
    return tree

def checkForEmptyRows(items, isRootCheck=True):
    # check for QTCREATORBUG-9069
    noEmptyRowsFound = True
    if items.getName().strip() == "":
        noEmptyRowsFound = False
        test.fail('Found empty row inside Locals and Expressions', '%s' % items)
    if items.childrenCount():
        for item in items.getChildren():
            noEmptyRowsFound &= checkForEmptyRows(item, False)
    if isRootCheck and noEmptyRowsFound:
        test.passes("No empty rows inside Locals and Expressions found.")
    return noEmptyRowsFound

def checkForExpectedValues(items, expectedChildren, expectedProperties):
    if items == None:
        test.fatal("Got a None object to inspect")
        return
    for subItemName in expectedChildren.keys():
        test.compare(items.countChildOccurrences(subItemName), expectedChildren[subItemName],
                     "Verify number of children named %s for %s" % (subItemName, items.getName()))
    properties = items.getChild("Properties")
    if properties:
        children = properties.getChildren()
        for property,value in expectedProperties.iteritems():
            foundProperty = getProperty(property, children)
            if foundProperty:
                test.compare(foundProperty.getValue(), value, "Verifying value for %s" % property)
            else:
                test.fail("Could not find property %s for object %s" % (property, items.getName()))
    else:
        test.fail("Missing properties for %s" % items.getName())

def getProperty(property, propertyList):
    for prop in propertyList:
        if prop.getName() == property:
            return prop
    return None
