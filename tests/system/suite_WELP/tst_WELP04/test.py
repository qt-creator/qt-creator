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
    # open Qt Creator
    startQC()
    if not startedWithoutPluginError():
        return
    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton('Get Started Now')
    if not test.verify(all((wsButtonFrame, wsButtonLabel)),
                       "Verifying: Qt Creator displays Welcome Page with Getting Started."):
        test.fatal("Something's wrong - leaving test.")
        invokeMenuItem("File", "Exit")
        return
    # select "Tutorials"
    if not switchToSubMode('Tutorials'):
        test.fatal("Could not find Tutorials button - leaving test")
        invokeMenuItem("File", "Exit")
        return
    searchTutorials = waitForObject("{type='QLineEdit' placeholderText='Search in Tutorials...'}")
    mouseClick(searchTutorials)
    replaceEditorContent(searchTutorials, "qwerty")
    tableView = waitForObject("{type='QTableView' unnamed='1' visible='1' "
                              "window=':Qt Creator_Core::Internal::MainWindow'}")
    waitFor('findExampleOrTutorial(tableView, ".*") is None', 3000)
    tutorial = findExampleOrTutorial(tableView, ".*", True)
    test.verify(tutorial is None,
                "Verifying: 'Tutorials' topic is opened and nothing is shown.")
    bnr = "Help: Building and Running an Example"
    replaceEditorContent(searchTutorials, bnr.lower())
    waitFor('findExampleOrTutorial(tableView, "%s.*") is not None' % bnr, 3000)
    tutorial = findExampleOrTutorial(tableView, "%s.*" % bnr, True)
    test.verify(tutorial is not None, "Verifying: Expected Text tutorial is shown.")
    # clicking before documentation was updated will open the tutorial in browser
    progressBarWait(warn=False)
    # select a text tutorial
    mouseClick(tutorial)
    test.verify("Building and Running an Example" in
                str(waitForObject(":Help Widget_Help::Internal::HelpWidget").windowTitle),
                "Verifying: The tutorial is opened inside Help.")
    # close help widget again to avoid focus issues
    sendEvent("QCloseEvent", waitForObject(":Help Widget_Help::Internal::HelpWidget"))
    # check a demonstration video link
    mouseClick(searchTutorials)
    replaceEditorContent(searchTutorials, "embedded device")
    waitFor('findExampleOrTutorial(tableView, "Online: Qt for Device Creation.*") is not None', 3000)
    tutorial = findExampleOrTutorial(tableView, "Online: Qt for Device Creation.*", True)
    test.verify(tutorial is not None,
                "Verifying: Link to the expected demonstration video exists.")
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
