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
    progressBarWait()
    for filetype, filename in [["Headers", "utility.h"],
                               ["Sources", "main.cpp"],
                               ["Sources", "utility.cpp"],
                               ["Resources", "musicbrowser.qrc"],
                               ["QML", "musicbrowser.qml"]]:
        filenames = ["ABCD" + filename.upper(), "abcd" + filename.lower(), "test", "TEST", filename]
        previous = filenames[-1]
        for filename in filenames:
            tempFiletype = filetype
            if previous in ("test", "TEST") or filetype == "QML" and previous[-4:] != ".qml":
                tempFiletype = "Other files"
            renameFile(templateDir, usedProFile, projectName + "." + tempFiletype,
                       previous, filename)
            # QTCREATORBUG-13176 does update the navigator async
            progressBarWait()
            if tempFiletype == "Headers":   # QTCREATORBUG-13204
                verifyRenamedIncludes(templateDir, "main.cpp", previous, filename)
                verifyRenamedIncludes(templateDir, "utility.cpp", previous, filename)
            previous = filename
    invokeMenuItem("File", "Exit")

def grep(pattern, text):
    return "\n".join(filter(lambda x: pattern in x, text.splitlines()))

def verifyRenamedIncludes(templateDir, file, oldname, newname):
    fileText = readFile(os.path.join(templateDir, file))
    if not (test.verify('#include "%s"' % oldname not in fileText,
                        'Verify that old filename is no longer included in %s' % file) and
            test.verify('#include "%s"' % newname in fileText,
                        'Verify that new filename is included in %s' % file)):
        test.log(grep("include", fileText))

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
        waitFor("macHackActivateContextMenuItem('Rename...')", 5000)
    else:
        if oldname.lower().endswith(".qrc"):
            menu = ":Qt Creator.Project.Menu.Folder_QMenu"
        else:
            menu = ":Qt Creator.Project.Menu.File_QMenu"
        activateItem(waitForObjectItem(menu, "Rename..."))
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
