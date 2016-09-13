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

def main():
    # prepare example project
    projectName = "adding"
    sourceExample = os.path.join(Qt5Path.examplesPath(Targets.DESKTOP_561_DEFAULT),
                                 "qml", "referenceexamples", "adding")
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
    openDocument(projectName + "." + projectName + "\\.pro")
    typeLines(waitForObject(":Qt Creator_TextEditor::TextEditorWidget"),
              "OTHER_FILES += example.qml")
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Close All")
    progressBarWait()
    for filetype, filename in [["Headers", "person.h"],
                               ["Sources", "main.cpp"],
                               ["Sources", "person.cpp"],
                               ["Resources", "adding.qrc"],
                               ["QML", "example.qml"]]:
        filenames = ["ABCD" + filename.upper(), "abcd" + filename.lower(), "test", "TEST", filename]
        previous = filenames[-1]
        for filename in filenames:
            tempFiletype = filetype
            if (filetype == "Resources" and previous in ("test", "TEST")
                or filetype == "QML" and not previous.endswith(".qml")):
                tempFiletype = "Other files"
            elif filetype == "Sources" and previous in ("test", "TEST"):
                tempFiletype = "Headers"
            renameFile(templateDir, usedProFile, projectName + "." + tempFiletype,
                       previous, filename)
            # QTCREATORBUG-13176 does update the navigator async
            progressBarWait()
            if filetype == "Headers":
                verifyRenamedIncludes(templateDir, "main.cpp", previous, filename)
                verifyRenamedIncludes(templateDir, "person.cpp", previous, filename)
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
