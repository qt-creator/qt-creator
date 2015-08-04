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
source("../../shared/suites_qtta.py")

if isQt54Build:
    welcomePage = ":WelcomePageStyledBar.WelcomePage_QQuickView"
else:
    welcomePage = ":Qt Creator.WelcomePage_QQuickWidget"
gettingStartedText = getQmlItem("Button", welcomePage, False,
                                "text='Get Started Now' id='gettingStartedButton'")

def clickItemVerifyHelpCombo(qmlItem, expectedHelpComboRegex, testDetails):
    global gettingStartedText
    mouseClick(waitForObject(qmlItem), 5, 5, 0, Qt.LeftButton)
    helpCombo = waitForObject(":Qt Creator_HelpSelector_QComboBox")
    if not test.verify(waitFor('re.match(expectedHelpComboRegex, str(helpCombo.currentText))',
                               5000), testDetails):
        test.log("Found %s" % str(helpCombo.currentText))
    # select "Welcome" page from left toolbar again
    switchViewTo(ViewConstants.WELCOME)
    test.verify(checkIfObjectExists(gettingStartedText),
                "Verifying: Get Started Now button is being displayed.")

def waitForButtonsState(projectsChecked, examplesChecked, tutorialsChecked, timeout=5000):
    projButton = waitForObject(getQmlItem("Button", welcomePage, False, "text='Projects'"))
    exmpButton = waitForObject(getQmlItem("Button", welcomePage, False, "text='Examples'"))
    tutoButton = waitForObject(getQmlItem("Button", welcomePage, False, "text='Tutorials'"))
    return waitFor('projButton.checked == projectsChecked '
                   'and exmpButton.checked == examplesChecked '
                   'and tutoButton.checked == tutorialsChecked', timeout)

def main():
    if not canTestEmbeddedQtQuick():
        test.log("Welcome mode is not scriptable with this Squish version")
        return
    global gettingStartedText
    # open Qt Creator
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return

    setAlwaysStartFullHelp()
    addCurrentCreatorDocumentation()

    buttonsAndState = {'Projects':True, 'Examples':False, 'Tutorials':False, 'New Project':False,
                       'Open Project':False}
    for button, state in buttonsAndState.items():
        qmlItem = getQmlItem("Button", welcomePage, False, "text='%s'" % button)
        if test.verify(checkIfObjectExists(qmlItem),
                       "Verifying whether '%s' button is shown." % button):
            buttonObj = findObject(qmlItem)
            test.compare(buttonObj.checked, state, "Verifying whether '%s' button is checked."
                         % button)

    test.verify(checkIfObjectExists(gettingStartedText),
                   "Verifying: Qt Creator displays Welcome Page with 'Get Started Now' button.")
    clickItemVerifyHelpCombo(gettingStartedText, "Qt Creator Manual",
                             "Verifying: Help with Creator Documentation is being opened.")
    textUrls = {'Online Community':'http://forum.qt.io',
                'Blogs':'http://planet.qt.io',
                'Qt Account':'https://account.qt.io',
                'Qt Cloud Services':'https://developer.qtcloudservices.com',
                'User Guide':'qthelp://org.qt-project.qtcreator/doc/index.html'
                }
    for text, url in textUrls.items():
        qmlItem = getQmlItem("LinkedText", welcomePage, False, "text='%s'" % text)
        if test.verify(checkIfObjectExists(qmlItem),
                       "Verifying: Link to %s exists." % text):
            itemObj = findObject(qmlItem)
            # some URLs might have varying parameters - limiting them to URL without a query
            if url.startswith("qthelp"):
                relevantUrlPart = str(itemObj.parent.openHelpUrl).split("?")[0]
            else:
                relevantUrlPart = str(itemObj.parent.openUrl).split("?")[0]
            test.compare(relevantUrlPart, url, "Verifying link.")

    mouseClick(gettingStartedText, 5, 5, 0, Qt.LeftButton)
    qcManualQModelIndexStr = getQModelIndexStr("text~='Qt Creator Manual [0-9.]+'",
                                               ":Qt Creator_QHelpContentWidget")
    if str(waitForObject(":Qt Creator_HelpSelector_QComboBox").currentText) == "(Untitled)":
        mouseClick(qcManualQModelIndexStr, 5, 5, 0, Qt.LeftButton)
        test.warning("Clicking 'Get Started Now' the second time showed blank page (Untitled)")

    # select "Welcome" page from left toolbar again
    switchViewTo(ViewConstants.WELCOME)
    test.verify(checkIfObjectExists(gettingStartedText),
                "Verifying: Getting Started topic is being displayed.")
    # select Examples and roughly check them
    mouseClick(waitForObject(getQmlItem("Button", welcomePage, False, "text='Examples'")),
               5, 5, 0, Qt.LeftButton)
    waitForButtonsState(False, True, False)
    expect = (("Rectangle", "id='rectangle1' radius='0'", "examples rectangle"),
              ("TextField", "id='lineEdit' placeholderText='Search in Examples...'",
               "examples search line edit"),
              ("ComboBox", "id='comboBox'", "Qt version combo box"),
              ("Delegate", "id='delegate' radius='0' caption~='.*Example'", "at least one example")
              )
    for (qType, prop, info) in expect:
        test.verify(checkIfObjectExists(getQmlItem(qType, welcomePage, None, prop)),
                    "Verifying whether %s is shown" % info)
    # select Tutorials and roughly check them
    mouseClick(waitForObject(getQmlItem("Button", welcomePage, False, "text='Tutorials'")),
               5, 5, 0, Qt.LeftButton)
    waitForButtonsState(False, False, True)
    expect = (("Rectangle", "id='rectangle1' radius='0'", "tutorials rectangle"),
              ("TextField", "id='lineEdit' placeholderText='Search in Tutorials...'",
               "tutorials search line edit"),
              ("Delegate", "id='delegate' radius='0' caption~='Creating.*'", "at least one tutorial")
              )
    for (qType, prop, info) in expect:
        test.verify(checkIfObjectExists(getQmlItem(qType, welcomePage, None, prop)),
                    "Verifying whether %s is shown" % info)
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
