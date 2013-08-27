#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
source("../../shared/suites_qtta.py")

# test Qt Creator version information from file and dialog
def getQtCreatorVersionFromDialog():
    chk = re.search("(?<=Qt Creator)\s\d+.\d+.\d+",
                    str(waitForObject("{text?='*Qt Creator*' type='QLabel' unnamed='1' visible='1' "
                                      "window=':About Qt Creator_Core::Internal::VersionDialog'}").text))
    try:
        ver = chk.group(0).strip()
        return ver
    except:
        test.fail("Failed to get the exact version from Dialog")
        return ""

def getQtCreatorVersionFromFile():
    qtCreatorPriFileName = "../../../../qtcreator.pri"
    # open file <qtCreatorPriFileName> and read version
    fileText = readFile(qtCreatorPriFileName)
    chk = re.search("(?<=QTCREATOR_VERSION =)\s\d+.\d+.\d+", fileText)
    try:
        ver = chk.group(0).strip()
        return ver
    except:
        test.fail("Failed to get the exact version from File")
        return ""

def checkQtCreatorHelpVersion(expectedVersion):
    switchViewTo(ViewConstants.HELP)
    try:
        creatorManual = waitForObject("{column='0' container=':Qt Creator_QHelpContentWidget' "
                                      "text?='Qt Creator Manual*' type='QModelIndex'}", 5000)
        test.compare(str(creatorManual.text)[18:], expectedVersion,
                     "Verifying whether manual uses expected version, text is: %s" % creatorManual.text)
    except LookupError:
        test.fail("Missing Qt Creator Manual.")

def main():
    expectedVersion = getQtCreatorVersionFromFile()
    if not expectedVersion:
        test.fatal("Can't find version from file.")
        return
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    if platform.system() == "Darwin":
       invokeMenuItem("Help", "About Qt Creator")
    else:
       invokeMenuItem("Help", "About Qt Creator...")
    # verify qt creator version
    waitForObject(":About Qt Creator_Core::Internal::VersionDialog")
    actualVersion = getQtCreatorVersionFromDialog()
    test.verify(actualVersion == expectedVersion,
                "Verifying version. Current version is '%s', expected version is '%s'"
                % (actualVersion, expectedVersion))
    # close and verify about dialog closed
    clickButton(waitForObject("{text='Close' type='QPushButton' unnamed='1' visible='1' "
                              "window=':About Qt Creator_Core::Internal::VersionDialog'}"))
    test.verify(checkIfObjectExists(":About Qt Creator_Core::Internal::VersionDialog", False),
                "Verifying if About dialog closed.")
    checkQtCreatorHelpVersion(expectedVersion)
    # exit qt creator
    invokeMenuItem("File", "Exit")
    # verify if qt creator closed properly
    test.verify(checkIfObjectExists(":Qt Creator_Core::Internal::MainWindow", False),
                "Verifying if Qt Creator closed.")
