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

# test bookmark functionality
def renameBookmarkFolder(view, item, newName):
    invokeContextMenuItemOnBookmarkFolder(view, item, "Rename Folder")
    replaceEditorContent(waitForObject(":Add Bookmark.treeView_QExpandingLineEdit"), newName)
    type(waitForObject(":Add Bookmark.treeView_QExpandingLineEdit"), "<Return>")
    return

def invokeContextMenuItemOnBookmarkFolder(view, item, menuItem):
    aboveWidget = "{name='line' type='QFrame' visible='1' window=':Add Bookmark_BookmarkDialog'}"
    mouseClick(waitForObjectItem(view, item), 5, 5, 0, Qt.LeftButton)
    openItemContextMenu(view, item, 5, 5, 0)
    activateItem(waitForObject("{aboveWidget=%s type='QMenu' unnamed='1' visible='1' "
                               "window=':Add Bookmark_BookmarkDialog'}" % aboveWidget), menuItem)

def textForQtVersion(text):
    suffix = "Qt Creator Manual"
    if text != suffix:
        text += " | " + suffix
    return text

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    # goto help mode and click on topic
    switchViewTo(ViewConstants.HELP)
    manualQModelIndex = getQModelIndexStr("text?='Qt Creator Manual *'",
                                          ":Qt Creator_QHelpContentWidget")
    doubleClick(waitForObject(manualQModelIndex), 5, 5, 0, Qt.LeftButton)
    mouseClick(waitForObject(getQModelIndexStr("text='Building and Running an Example'",
                                               manualQModelIndex)), 5, 5, 0, Qt.LeftButton)
    helpSelector = waitForObject(":Qt Creator_HelpSelector_QComboBox")
    waitFor("str(helpSelector.currentText).startswith('Building and Running an Example')", 10000)
    # open bookmarks window
    clickButton(waitForObject(":Qt Creator.Add Bookmark_QToolButton"))
    clickButton(waitForObject(":Add Bookmark.ExpandBookmarksList_QToolButton"))
    # create root bookmark directory
    clickButton(waitForObject(":Add Bookmark.New Folder_QPushButton"))
    # rename root bookmark directory
    bookmarkView = waitForObject(":Add Bookmark.treeView_QTreeView")
    renameBookmarkFolder(bookmarkView, "New Folder*", "Sample")
    # create two more subfolders
    clickButton(waitForObject(":Add Bookmark.New Folder_QPushButton"))
    renameBookmarkFolder(bookmarkView, "Sample.New Folder*", "Folder 1")
    clickButton(waitForObject(":Add Bookmark.New Folder_QPushButton"))
    renameBookmarkFolder(bookmarkView, "Sample.Folder 1.New Folder*", "Folder 2")
    clickButton(waitForObject(":Add Bookmark.OK_QPushButton"))
    mouseClick(manualQModelIndex, 5, 5, 0, Qt.LeftButton)
    type(waitForObject(":Qt Creator_QHelpContentWidget"), "<Down>")
    clickButton(waitForObject(":Qt Creator.Add Bookmark_QToolButton"))
    clickButton(waitForObject(":Add Bookmark.ExpandBookmarksList_QToolButton"))
    # click on "Sample" and create new directory under it
    mouseClick(waitForObject(getQModelIndexStr("text='Sample'", ":Add Bookmark.treeView_QTreeView")))
    clickButton(waitForObject(":Add Bookmark.New Folder_QPushButton"))
    clickButton(waitForObject(":Add Bookmark.OK_QPushButton"))
    # choose bookmarks
    mouseClick(waitForObjectItem(":Qt Creator_Core::Internal::CommandComboBox", "Bookmarks"))
    # verify if all folders are created and bookmarks present
    sampleQModelIndex = getQModelIndexStr("text='Sample'", ":Qt Creator_Bookmarks_TreeView")
    folder1QModelIndex = getQModelIndexStr("text='Folder 1'", sampleQModelIndex)
    folder2QModelIndex = getQModelIndexStr("text='Folder 2'", folder1QModelIndex)
    bldRunQModelIndex = getQModelIndexStr("text?='%s'" % textForQtVersion("Building and Running an Example*"),
                                          folder2QModelIndex)
    newFolderQModelIndex = getQModelIndexStr("text='New Folder'", sampleQModelIndex)
    manualQModelIndex = getQModelIndexStr("text='%s'" % textForQtVersion("Qt Creator Manual"),
                                             newFolderQModelIndex)
    test.verify(checkIfObjectExists(sampleQModelIndex, verboseOnFail = True) and
                checkIfObjectExists(folder1QModelIndex, verboseOnFail = True) and
                checkIfObjectExists(folder2QModelIndex, verboseOnFail = True) and
                checkIfObjectExists(bldRunQModelIndex, verboseOnFail = True) and
                checkIfObjectExists(manualQModelIndex, verboseOnFail = True),
                "Verifying if all folders and bookmarks are present")
    mouseClick(waitForObject(":Qt Creator_Bookmarks_TreeView"), 5, 5, 0, Qt.LeftButton)
    for i in range(6):
        type(waitForObject(":Qt Creator_Bookmarks_TreeView"), "<Right>")
    type(waitForObject(":Qt Creator_Bookmarks_TreeView"), "<Return>")
    test.verify(textForQtVersion("Building and Running an Example") in getHelpTitle(),
                "Verifying if first bookmark is opened")
    mouseClick(waitForObject(bldRunQModelIndex))
    type(waitForObject(":Qt Creator_Bookmarks_TreeView"), "<Down>")
    type(waitForObject(":Qt Creator_Bookmarks_TreeView"), "<Right>")
    type(waitForObject(":Qt Creator_Bookmarks_TreeView"), "<Down>")
    type(waitForObject(":Qt Creator_Bookmarks_TreeView"), "<Return>")
    test.verify(textForQtVersion("Qt Creator Manual") in getHelpTitle(),
                "Verifying if second bookmark is opened")
    # delete previously created directory
    clickButton(waitForObject(":Qt Creator.Add Bookmark_QToolButton"))
    clickButton(waitForObject(":Add Bookmark.ExpandBookmarksList_QToolButton"))
    invokeContextMenuItemOnBookmarkFolder(":Add Bookmark.treeView_QTreeView", "Sample.Folder 1",
                                          "Delete Folder")
    clickButton(waitForObject("{container=':Add Bookmark.treeView_QTreeView' text='Yes' "
                              "type='QPushButton' unnamed='1' visible='1'}"))
    # close bookmarks
    clickButton(waitForObject(":Add Bookmark.OK_QPushButton"))
    # choose bookmarks from command combobox
    mouseClick(waitForObject(":Qt Creator_Core::Internal::CommandComboBox"))
    mouseClick(waitForObjectItem(":Qt Creator_Core::Internal::CommandComboBox", "Bookmarks"))
    # verify if folders and bookmark deleted
    test.verify(checkIfObjectExists(sampleQModelIndex, verboseOnFail = True) and
                checkIfObjectExists(folder1QModelIndex, shouldExist = False, verboseOnFail = True) and
                checkIfObjectExists(folder2QModelIndex, shouldExist = False, verboseOnFail = True) and
                checkIfObjectExists(bldRunQModelIndex, shouldExist = False, verboseOnFail = True) and
                checkIfObjectExists(manualQModelIndex, verboseOnFail = True),
                "Verifying if folder 1 and folder 2 deleted including their bookmark")
    # exit
    invokeMenuItem("File", "Exit")
