source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

# test bookmark functionality
def renameBookmarkFolder(view, item, newName):
    openItemContextMenu(view, item, 5, 5, 0)
    activateItem(waitForObjectItem(":Add Bookmark.line_QMenu", "Rename Folder"))
    replaceEditorContent(waitForObject(":treeView_QExpandingLineEdit"), newName)
    type(waitForObject(":treeView_QExpandingLineEdit"), "<Return>")
    return

def main():
    startApplication("qtcreator" + SettingsPath)
    # goto help mode and click on topic
    switchViewTo(ViewConstants.HELP)
    doubleClick(":Qt Creator Manual QModelIndex", 5, 5, 0, Qt.LeftButton)
    doubleClick(":Qt Creator Manual*.Getting Started_QModelIndex", 5, 5, 0, Qt.LeftButton)
    mouseClick(waitForObject(":Getting Started.Building and Running an Example_QModelIndex"), 5, 5, 0, Qt.LeftButton)
    # open bookmarks window
    clickButton(waitForObject(":Qt Creator.Add Bookmark_QToolButton"))
    clickButton(waitForObject(":Add Bookmark.+_QToolButton"))
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
    mouseClick(":Qt Creator Manual QModelIndex", 5, 5, 0, Qt.LeftButton)
    type(waitForObject(":Qt Creator_QHelpContentWidget"), "<Down>")
    clickButton(waitForObject(":Qt Creator.Add Bookmark_QToolButton"))
    clickButton(waitForObject(":Add Bookmark.+_QToolButton"))
    # click on "Sample" and create new directory under it
    mouseClick(waitForObject(":treeView.Sample_QModelIndex"))
    clickButton(waitForObject(":Add Bookmark.New Folder_QPushButton"))
    clickButton(waitForObject(":Add Bookmark.OK_QPushButton"))
    # choose bookmarks
    mouseClick(waitForObjectItem(":Qt Creator_Core::Internal::CommandComboBox", "Bookmarks"))
    # verify if all folders are created and bookmarks present
    test.verify(checkIfObjectExists(":Sample_QModelIndex", verboseOnFail = True) and
                checkIfObjectExists(":Sample.Folder 1_QModelIndex", verboseOnFail = True) and
                checkIfObjectExists(":Folder 1.Folder 2_QModelIndex", verboseOnFail = True) and
                checkIfObjectExists(":Folder 2.Qt Creator : Building and Running an Example_QModelIndex", verboseOnFail = True) and
                checkIfObjectExists(":New Folder.Qt Creator : Qt Creator Manual_QModelIndex", verboseOnFail = True),
                "Verifying if all folders and bookmarks are present")
    mouseClick(waitForObject(":Qt Creator_TreeView"), 5, 5, 0, Qt.LeftButton)
    for i in range(6):
        type(waitForObject(":Qt Creator_TreeView"), "<Right>")
    type(waitForObject(":Qt Creator_TreeView"), "<Return>")
    test.verify("QtCreator : Building and Running an Example" in str(waitForObject(":Qt Creator_Help::Internal::HelpViewer").title),
                "Verifying if first bookmark is opened")
    mouseClick(waitForObject(":Folder 2.Qt Creator : Building and Running an Example_QModelIndex"))
    type(waitForObject(":Qt Creator_TreeView"), "<Down>")
    type(waitForObject(":Qt Creator_TreeView"), "<Right>")
    type(waitForObject(":Qt Creator_TreeView"), "<Down>")
    type(waitForObject(":Qt Creator_TreeView"), "<Return>")
    test.verify("QtCreator : Qt Creator Manual" in str(waitForObject(":Qt Creator_Help::Internal::HelpViewer").title),
                "Verifying if second bookmark is opened")
    # delete previously created directory
    clickButton(waitForObject(":Qt Creator.Add Bookmark_QToolButton"))
    clickButton(waitForObject(":Add Bookmark.+_QToolButton"))
    openItemContextMenu(waitForObject(":Add Bookmark.treeView_QTreeView"), "Sample.Folder 1", 5, 5, 0)
    activateItem(waitForObjectItem(":Add Bookmark.line_QMenu", "Delete Folder"))
    clickButton(waitForObject(":treeView.Yes_QPushButton"))
    # close bookmarks
    clickButton(waitForObject(":Add Bookmark.OK_QPushButton"))
    # choose bookmarks from command combobox (left top corner of qt creator)
    mouseClick(waitForObject(":Qt Creator_Core::Internal::CommandComboBox"))
    mouseClick(waitForObjectItem(":Qt Creator_Core::Internal::CommandComboBox", "Bookmarks"))
    # verify if folders and bookmark deleted
    test.verify(checkIfObjectExists(":Sample_QModelIndex", verboseOnFail = True) and
                checkIfObjectExists(":Sample.Folder 1_QModelIndex ", shouldExist = False, verboseOnFail = True) and
                checkIfObjectExists(":Folder 1.Folder 2_QModelIndex", shouldExist = False, verboseOnFail = True) and
                checkIfObjectExists(":Folder 2.Qt Creator : Building and Running an Example_QModelIndex", shouldExist = False, verboseOnFail = True) and
                checkIfObjectExists(":New Folder.Qt Creator : Qt Creator Manual_QModelIndex", verboseOnFail = True),
                "Verifying if folder 1 and folder 2 deleted including their bookmark")
    # exit
    invokeMenuItem("File", "Exit")
# no cleanup needed
