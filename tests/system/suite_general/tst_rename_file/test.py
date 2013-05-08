source("../../shared/qtcreator.py")

def main():
    # prepare example project
    projectName = "declarative-music-browser"
    sourceExample = os.path.join(sdkPath, "Examples", "QtMobility", projectName)
    proFile = projectName + ".pro"
    if not neededFilePresent(os.path.join(sourceExample, proFile)):
        return
    # copy example project to temp directory
    templateDir = prepareTemplate(sourceExample)

    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    usedProFile = os.path.join(templateDir, proFile)
    openQmakeProject(usedProFile)
    for filetype, filename in [["Headers", "utility.h"],
                               ["Sources", "main.cpp"],
                               ["Sources", "utility.cpp"],
                               ["Resources", "musicbrowser.qrc"],
                               ["QML", "musicbrowser.qml"]]:
        filenames = ["ABCD" + filename.upper(), "abcd" + filename.lower(), "test", "TEST", filename]
        if platform.system() == 'Darwin':
            # avoid QTCREATORBUG-9197
            filtered = [filenames[0]]
            for i in range(1, len(filenames)):
                if filenames[i].lower() != filtered[-1].lower():
                    filtered.append(filenames[i])
            filenames = filtered
        for i in range(len(filenames)):
            tempFiletype = filetype
            if filetype == "QML" and filenames[i - 1][-4:] != ".qml":
                tempFiletype = "Other files"
            renameFile(templateDir, usedProFile, projectName + "." + tempFiletype,
                       filenames[i - 1], filenames[i])
    invokeMenuItem("File", "Exit")

def renameFile(projectDir, proFile, branch, oldname, newname):
    oldFilePath = os.path.join(projectDir, oldname)
    newFilePath = os.path.join(projectDir, newname)
    oldFileText = readFile(oldFilePath)
    itemText = branch + "." + oldname.replace(".", "\\.")
    treeview = waitForObject(":Qt Creator_Utils::NavigationTreeView")
    if platform.system() == 'Darwin':
        JIRA.performWorkaroundForBug(8735, JIRA.Bug.CREATOR, treeview)
    try:
        openItemContextMenu(treeview, itemText, 5, 5, 0)
    except:
        openItemContextMenu(treeview, addBranchWildcardToRoot(itemText), 5, 5, 0)
    activateItem(waitForObjectItem(":Qt Creator.Project.Menu.File_QMenu", "Rename..."))
    type(waitForObject(":Qt Creator_Utils::NavigationTreeView::QExpandingLineEdit"), newname)
    type(waitForObject(":Qt Creator_Utils::NavigationTreeView::QExpandingLineEdit"), "<Return>")
    test.verify(waitFor("os.path.exists(newFilePath)", 1000),
                "Verify that file with new name exists: %s" % newFilePath)
    if not (oldname.lower() == newname.lower() and platform.system() in ('Windows', 'Microsoft')):
        test.verify(not os.path.exists(oldFilePath),
                    "Verify that file with old name does not exist: %s" % oldFilePath)
    test.compare(readFile(newFilePath), oldFileText,
                 "Comparing content of file before and after renaming")
    test.verify(waitFor("newname in safeReadFile(proFile)", 2000),
                "Verify that new filename '%s' was added to pro-file." % newname)
    if not oldname in newname:
        test.verify(not oldname in readFile(proFile),
                    "Verify that old filename '%s' was removed from pro-file." % oldname)

def safeReadFile(filename):
    text = ""
    while text == "":
        try:
            text = readFile(filename)
        except:
            pass
    return text
