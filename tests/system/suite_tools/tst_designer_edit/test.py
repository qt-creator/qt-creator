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

def performEditMenu():
    test.log("Editing menu")
    previewMenuBar = waitForObject(":FormEditorStack.menuBar_QDesignerMenuBar")
    # add File menu
    doubleClick(previewMenuBar, 30, 7, 0, Qt.LeftButton)
    passiveLineEdit = waitForObject(":FormEditorStack.__qt__passive_editor_QLineEdit")
    replaceEditorContent(passiveLineEdit, "SquishTestFile")
    type(passiveLineEdit, "<Return>")
    # this "special" QDesignerMenu will be hidden and unusable on OSX
    menuStr = ("{name='menuSquishTestFile' title='SquishTestFile' type='QDesignerMenu' "
               "window=':Qt Creator_Core::Internal::MainWindow'}")
    try:
        menu = waitForObject(menuStr, 5000)
    except:
        if platform.system() == 'Darwin':
            # we need some information of the menu, so find at least the 'hidden' one
            menu = findObject(menuStr)
        else:
            raise
    menuHeight = menu.height
    itemHeight = menuHeight / 2 # actually only 'Type Here' and 'Add Separator' are shown
    itemHalf = itemHeight / 2
    # add Open menu item
    if platform.system() == 'Darwin':
        # double clicking is not possible on hidden objects
        nativeType("<Return>")
    else:
        doubleClick(menu, 15, itemHalf, 0, Qt.LeftButton)
    passiveLineEdit = waitForObject(":FormEditorStack.__qt__passive_editor_QLineEdit")
    replaceEditorContent(passiveLineEdit, "Open")
    type(passiveLineEdit, "<Return>")
    waitFor("menu.height > menuHeight", 2000)
    menuHeight = menu.height
    # add a separator
    if platform.system() == 'Darwin':
        nativeType("<Down>")
        nativeType("<Return>")
    else:
        doubleClick(menu, 15, menu.height - itemHalf, 0, Qt.LeftButton)
    waitFor("menu.height > menuHeight", 2000)
    separatorHeight = menu.height - menuHeight
    menuHeight = menu.height
    # add Shutdown menu item (Quit/Exit do not work because Squish/Qt5 problems with menus)
    if platform.system() == 'Darwin':
        nativeType("<Return>")
    else:
        doubleClick(menu, 30, itemHeight + separatorHeight + itemHalf, 0, Qt.LeftButton)
    passiveLineEdit = waitForObject(":FormEditorStack.__qt__passive_editor_QLineEdit")
    replaceEditorContent(passiveLineEdit, "Shutdown")
    type(passiveLineEdit, "<Return>")
    waitFor("menu.height > menuHeight", 2000)
    # close menu in case it overlaps the combo box
    mouseClick(waitForObject(":FormEditorStack_qdesigner_internal::FormWindow"),
               100, 100, 0, Qt.LeftButton)
    # verify Action Editor and Object Inspector
    actionTV = waitForObject("{container={name='ActionEditorDockWidget' type='QDockWidget' "
                             "visible='1' window=':Qt Creator_Core::Internal::MainWindow'} "
                             "type='qdesigner_internal::ActionTreeView' unnamed='1' visible='1'}")
    test.compare(dumpItems(actionTV.model()), ["actionOpen", "actionShutdown"],
                 "Verify whether respective actions have been added to Action Editor.")
    objInspTV = waitForObject("{container={name='ObjectInspectorDockWidget' type='QDockWidget' "
                              "visible='1' window=':Qt Creator_Core::Internal::MainWindow'} "
                              "type='QTreeView' unnamed='1' visible='1'}")
    tree = __iterateChildren__(objInspTV.model(), None)
    expectedMenuSequence = [["menuSquishTestFile", 2], ["actionOpen", 3], ["separator", 3],
                            ["actionShutdown", 3]]
    seqStart = tree.index(expectedMenuSequence[0])
    test.verify(seqStart != -1 and tree[seqStart:seqStart + 4] == expectedMenuSequence,
                "Verify Object Inspector contains expected menu inclusive children.")
    return [["Open", False], ["", True], ["Shutdown", False]]

def performEditCombo():
    edComboWin = "{type='qdesigner_internal::ListWidgetEditor' unnamed='1' visible='1'}"
    test.log("Editing combo box")
    comboBox = waitForObject(":FormEditorStack.comboBox_QComboBox")
    doubleClick(comboBox, 5, 5, 0, Qt.LeftButton)
    # make sure properties of items are shown
    propertyEdView = "{type='QtPropertyEditorView' unnamed='1' visible='1' window=%s}" % edComboWin
    if test.verify(not object.exists(propertyEdView), "Verifying property editor is not visible."):
        clickButton(waitForObject("{name='showPropertiesButton' type='QPushButton' visible='1' "
                                  "window=%s}" % edComboWin))
    for i in range(5):
        clickButton(waitForObject("{name='newListItemButton' type='QToolButton' visible='1' "
                                  "window=%s}" % edComboWin))
        doubleClick(waitForObject("{column='1' container=%s text='New Item' type='QModelIndex'}"
                                  % propertyEdView))
        lineEd = waitForObject("{container=%s type='qdesigner_internal::PropertyLineEdit' "
                               "unnamed='1' visible='1'}" % propertyEdView)
        type(lineEd, "Combo Item %d" % i)
        type(lineEd, "<Return>")
    itemListWidget = waitForObject("{name='listWidget' type='QListWidget' visible='1' window=%s}"
                                   % edComboWin)
    expectedItems = ["Combo Item %d" % i for i in range(5)]
    test.compare(dumpItems(itemListWidget.model()), expectedItems,
                 "Verifying all items have been added.")
    # move last item to top (assume last item is still selected)
    upButton = waitForObject("{name='moveListItemUpButton' type='QToolButton' visible='1' "
                             "window=%s}" % edComboWin)
    downButton = findObject("{name='moveListItemDownButton' type='QToolButton' visible='1' "
                            "window=%s}" % edComboWin)
    test.verify(upButton.enabled, "Verifying whether Up button is enabled")
    test.verify(not downButton.enabled, "Verifying whether Down button is disabled")
    for _ in range(4):
        clickButton(upButton)
    test.verify(waitFor("not upButton.enabled", 1000), "Verifying whether Up button is disabled")
    test.verify(downButton.enabled, "Verifying whether Down button is enabled")
    expectedItems.insert(0, expectedItems.pop())
    test.compare(dumpItems(itemListWidget.model()), expectedItems,
                 "Verifying last item has moved to top of the list.")
    # remove the "Combo Item 1" item from the list
    clickItem(itemListWidget, "Combo Item 1", 5, 5, 0, Qt.LeftButton)
    clickButton("{name='deleteListItemButton' type='QToolButton' visible='1' window=%s}"
                % edComboWin)
    waitFor("itemListWidget.model().rowCount() == len(expectedItems) - 1", 2000)
    expectedItems.remove("Combo Item 1")
    test.compare(dumpItems(itemListWidget.model()), expectedItems,
                 "Verifying 'Combo Item 1' has been removed.")
    clickButton("{text='OK' type='QPushButton' unnamed='1' visible='1' window=%s}" % edComboWin)
    test.compare(dumpItems(comboBox.model()), expectedItems,
                 "Verifying combo box inside Designer has been updated")
    return expectedItems

def verifyPreview(menuItems, comboItems):
    test.log("Verifying preview")
    prev = "{name='MainWindow' type='QMainWindow' visible='1' windowTitle='MainWindow - [Preview]'}"
    invokeMenuItem("Tools", "Form Editor", "Preview...")
    # verify menu
    menuBar = waitForObject("{name='menuBar' type='QMenuBar' visible='1' window=%s}" % prev)
    menu = None
    activateItem(menuBar, "SquishTestFile")
    # known issue for Squish using Qt5 on Mac
    if platform.system() == 'Darwin':
        for obj in object.topLevelObjects():
            try:
                if className(obj) == 'QMenu' and str(obj.objectName) == 'menuSquishTestFile':
                    menu = obj
                    break
            except:
                pass
    else:
        try:
            menu = waitForObject("{name='menuSquishTestFile' title='SquishTestFile' "
                                 "type='QMenu' visible='1' window=%s}" % prev)
        except:
            pass
    if menu:
        actions = menu.actions()
        for position, (text, isSep) in enumerate(menuItems):
            action = actions.at(position)
            test.verify(action.isSeparator() == isSep and str(action.text) == text,
                        "Verifying menu item '%s' (separator: %s)" % (text, str(isSep)))
    else:
        test.warning("Failed to get menu...")
    activateItem(menuBar, "SquishTestFile")
    # verify combo
    combo = waitForObject("{name='comboBox' type='QComboBox' visible='1' window=%s}" % prev)
    test.compare(dumpItems(combo.model()), comboItems,
                 "Verifying combo box contains expected items.")
    sendEvent("QCloseEvent", waitForObject(prev))

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    createProject_Qt_GUI(tempDir(), "DesignerTestApp", False)
    selectFromLocator("mainwindow.ui")
    replaceEditorContent(waitForObject("{container=':*Qt Creator.Widget Box_QDockWidget' "
                                       "type='QLineEdit' visible='1'}"), "combo")
    categoryView = ("{container=':Widget Box_qdesigner_internal::WidgetBoxTreeWidget' "
                    "type='qdesigner_internal::WidgetBoxCategoryListView' unnamed='1' visible='1'}")
    dragAndDrop(waitForObject("{container=%s text='Combo Box' type='QModelIndex'}" % categoryView),
                5, 5, ":FormEditorStack_qdesigner_internal::FormWindow", 20, 50, Qt.CopyAction)
    menuItems = performEditMenu()
    comboItems = performEditCombo()
    verifyPreview(menuItems, comboItems)
    invokeMenuItem("File", "Save All")
    invokeMenuItem("Build", "Build All")
    waitForCompile()
    checkCompile()
    invokeMenuItem("File", "Exit")
