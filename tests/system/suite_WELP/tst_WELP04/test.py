# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")


def __waitForListView__():
    listView = waitForObject("{container=':Qt Creator.WelcomeScreenStackedWidget' "
                             "type='QListView' name='AllItemsView' visible='1'}")
    return listView


def main():
    # open Qt Creator
    startQC()
    if not startedWithoutPluginError():
        return
    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton('Get Started')
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
    listView = __waitForListView__()
    waitFor('findExampleOrTutorial(listView, ".*") is None', 3000)
    tutorial = findExampleOrTutorial(listView, ".*", True)
    test.verify(tutorial is None,
                "Verifying: 'Tutorials' topic is opened and nothing is shown.")
    bnr = "Building and Running an Example"
    replaceEditorContent(searchTutorials, bnr.lower())
    listView = __waitForListView__()
    waitFor('findExampleOrTutorial(listView, "%s.*") is not None' % bnr, 3000)
    tutorial = findExampleOrTutorial(listView, "%s.*" % bnr, True)
    test.verify(tutorial is not None, "Verifying: Expected Text tutorial is shown.")
    # clicking before documentation was updated will open the tutorial in browser
    progressBarWait(warn=False)
    # select a text tutorial
    mouseClick(waitForObjectItem(listView, str(tutorial.text)))
    test.verify("Building and Running an Example" in
                str(waitForObject(":Help Widget_Help::Internal::HelpWidget").windowTitle),
                "Verifying: The tutorial is opened inside Help.")
    # close help widget again to avoid focus issues
    sendEvent("QCloseEvent", waitForObject(":Help Widget_Help::Internal::HelpWidget"))
    # check a demonstration video link
    mouseClick(searchTutorials)
    replaceEditorContent(searchTutorials, "embedded device")
    embeddedTutorial = "How to install and set up Qt for Device Creation.*"
    listView = __waitForListView__()
    waitFor('findExampleOrTutorial(listView, embeddedTutorial) is not None', 3000)
    tutorial = findExampleOrTutorial(listView, embeddedTutorial, True)
    test.verify(tutorial is not None,
                "Verifying: Link to the expected demonstration video exists.")
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
