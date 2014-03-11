#############################################################################
##
## Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

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
        if isQt4Build and platform.system() == 'Darwin':
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
            # following is necessary due to QTCREATORBUG-10179
            # will be fixed when Qt5's MIME type database can be used
            if ((filenames[-1] in ("main.cpp", "utility.cpp") and filenames[i - 1][-4:] != ".cpp")
                or (filenames[-1] == "utility.h" and filenames[i - 1][-2:].lower() != ".h")
                or (filetype == "Resources" and filenames[i - 1][-4:] != ".qrc")):
                tempFiletype = "Other files"
            # end of handling QTCREATORBUG-10179
            renameFile(templateDir, usedProFile, projectName + "." + tempFiletype,
                       filenames[i - 1], filenames[i])
    invokeMenuItem("File", "Exit")

def renameFile(projectDir, proFile, branch, oldname, newname):
    oldFilePath = os.path.join(projectDir, oldname)
    newFilePath = os.path.join(projectDir, newname)
    oldFileText = readFile(oldFilePath)
    itemText = branch + "." + oldname.replace(".", "\\.")
    treeview = waitForObject(":Qt Creator_Utils::NavigationTreeView")
    try:
        openItemContextMenu(treeview, itemText, 5, 5, 0)
    except:
        openItemContextMenu(treeview, addBranchWildcardToRoot(itemText), 5, 5, 0)
    # hack for Squish5/Qt5.2 problems of handling menus on Mac - remove asap
    if platform.system() == 'Darwin':
        item = "Rename..."
        if oldname.endswith(".qrc"):
            item = "Rename File"
        waitFor("macHackActivateContextMenuItem(item)", 5000)
    else:
        if oldname.endswith(".qrc"):
            activateItem(waitForObjectItem(":Qt Creator.Project.Menu.Folder_QMenu", "Rename File"))
        else:
            activateItem(waitForObjectItem(":Qt Creator.Project.Menu.File_QMenu", "Rename..."))
    type(waitForObject(":Qt Creator_Utils::NavigationTreeView::QExpandingLineEdit"), newname)
    type(waitForObject(":Qt Creator_Utils::NavigationTreeView::QExpandingLineEdit"), "<Return>")
    test.verify(waitFor("os.path.exists(newFilePath)", 1000),
                "Verify that file with new name exists: %s" % newFilePath)
    test.compare(readFile(newFilePath), oldFileText,
                 "Comparing content of file before and after renaming")
    test.verify(waitFor("newname in safeReadFile(proFile)", 2000),
                "Verify that new filename '%s' was added to pro-file." % newname)
    if not oldname in newname:
        test.verify(not oldname in readFile(proFile),
                    "Verify that old filename '%s' was removed from pro-file." % oldname)
    if not (oldname.lower() == newname.lower() and platform.system() in ('Windows', 'Microsoft')):
        test.verify(not oldname in os.listdir(projectDir),
                    "Verify that file with old name does not exist: %s" % oldFilePath)

def safeReadFile(filename):
    text = ""
    while text == "":
        try:
            text = readFile(filename)
        except:
            pass
    return text
