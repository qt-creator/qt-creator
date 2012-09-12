source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

# test bookmark functionality
def renameBookmarkFolder(view, item, newName):
    openItemContextMenu(view, item, 5, 5, 0)
    activateItem(waitForObjectItem("{type='QMenu' unnamed='1' visible='1' "
                                   "window=':Add Bookmark_BookmarkDialog'}", "Rename Folder"))
    replaceEditorContent(waitForObject(":Add Bookmark.treeView_QExpandingLineEdit"), newName)
    type(waitForObject(":Add Bookmark.treeView_QExpandingLineEdit"), "<Return>")
    return

def getQModelIndexStr(textProperty, container):
    if (container.startswith(":")):
        container = "'%s'" % container
    return ("{column='0' container=%s %s type='QModelIndex'}"
            % (container, textProperty))

def main():
    startApplication("qtcreator" + SettingsPath)
    # goto help mode and click on topic
    switchViewTo(ViewConstants.HELP)
    manualQModelIndex = getQModelIndexStr("text?='Qt Creator Manual *'",
                                          ":Qt Creator_QHelpContentWidget")
    doubleClick(manualQModelIndex, 5, 5, 0, Qt.LeftButton)
    gettingStartedQModelIndex = getQModelIndexStr("text='Getting Started'", manualQModelIndex)
    doubleClick(gettingStartedQModelIndex, 5, 5, 0, Qt.LeftButton)
    mouseClick(waitForObject(getQModelIndexStr("text='Building and Running an Example'",
                                               gettingStartedQModelIndex)), 5, 5, 0, Qt.LeftButton)
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
    bldRunQModelIndex = getQModelIndexStr("text?='QtCreator : Building and Running an Example*'",
                                          folder2QModelIndex)
    newFolderQModelIndex = getQModelIndexStr("text='New Folder'", sampleQModelIndex)
    manualQModelIndex = getQModelIndexStr("text='QtCreator : Qt Creator Manual'",
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
    test.verify("QtCreator : Building and Running an Example" in str(waitForObject(":Qt Creator_Help::Internal::HelpViewer").title),
                "Verifying if first bookmark is opened")
    mouseClick(waitForObject(bldRunQModelIndex))
    type(waitForObject(":Qt Creator_Bookmarks_TreeView"), "<Down>")
    type(waitForObject(":Qt Creator_Bookmarks_TreeView"), "<Right>")
    type(waitForObject(":Qt Creator_Bookmarks_TreeView"), "<Down>")
    type(waitForObject(":Qt Creator_Bookmarks_TreeView"), "<Return>")
    test.verify("QtCreator : Qt Creator Manual" in str(waitForObject(":Qt Creator_Help::Internal::HelpViewer").title),
                "Verifying if second bookmark is opened")
    # delete previously created directory
    clickButton(waitForObject(":Qt Creator.Add Bookmark_QToolButton"))
    clickButton(waitForObject(":Add Bookmark.ExpandBookmarksList_QToolButton"))
    openItemContextMenu(waitForObject(":Add Bookmark.treeView_QTreeView"), "Sample.Folder 1", 5, 5, 0)
    activateItem(waitForObjectItem("{type='QMenu' unnamed='1' visible='1' "
                                   "window=':Add Bookmark_BookmarkDialog'}", "Delete Folder"))
    clickButton(waitForObject("{container=':Add Bookmark.treeView_QTreeView' text='Yes' "
                              "type='QPushButton' unnamed='1' visible='1'}"))#:treeView.Yes_QPushButton"))
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
