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

def main():
    if not canTestEmbeddedQtQuick():
        test.log("Welcome mode is not scriptable with this Squish version")
        return
    if isQt54Build:
        welcomePage = ":WelcomePageStyledBar.WelcomePage_QQuickView"
    else:
        welcomePage = ":Qt Creator.WelcomePage_QQuickWidget"
    # open Qt Creator
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    getStarted = getQmlItem("Button", welcomePage, False,
                            "text='Get Started Now' id='gettingStartedButton'")
    if not test.verify(checkIfObjectExists(getStarted),
                       "Verifying: Qt Creator displays Welcome Page with Get Started Now button."):
        test.fatal("Something's wrong here - leaving test.")
        invokeMenuItem("File", "Exit")
        return
    # select "Tutorials"
    mouseClick(waitForObject(getQmlItem("Button", welcomePage, False, "text='Tutorials'")),
               5, 5, 0, Qt.LeftButton)
    searchTut = getQmlItem("TextField", welcomePage, False,
                           "placeholderText='Search in Tutorials...' id='lineEdit'")
    mouseClick(waitForObject(searchTut), 5, 5, 0, Qt.LeftButton)
    replaceEditorContent(waitForObject(searchTut), "qwerty")
    test.verify(checkIfObjectExists(getQmlItem("Text", welcomePage,
                                               False, "text='Tutorials'")) and
                checkIfObjectExists(getQmlItem("Delegate", welcomePage,
                                               False, "id='delegate' radius='0' caption~='.*'"),
                                    False),
                "Verifying: 'Tutorials' topic is opened and nothing is shown.")
    replaceEditorContent(waitForObject(searchTut), "building and running an example application")
    bldRunExmpl = getQmlItem("Delegate", welcomePage, False,
                             "caption='Building and Running an Example Application' "
                             "id='delegate' radius='0'")
    test.verify(checkIfObjectExists(bldRunExmpl), "Verifying: Expected Text tutorial is shown.")
    # select a text tutorial
    mouseClick(waitForObject(bldRunExmpl), 5, 5, 0, Qt.LeftButton)
    test.verify("Building and Running an Example" in
                str(waitForObject(":Help Widget_Help::Internal::HelpWidget").windowTitle),
                "Verifying: The tutorial is opened inside Help.")
    # close help widget again to avoid focus issues
    sendEvent("QCloseEvent", waitForObject(":Help Widget_Help::Internal::HelpWidget"))
    # check a demonstration video link
    searchTutWidget = waitForObject(searchTut)
    mouseClick(searchTutWidget)
    replaceEditorContent(searchTutWidget, "embedded device")
    test.verify(checkIfObjectExists(getQmlItem("Delegate", welcomePage,
                                               False, "id='delegate' radius='0' caption="
                                               "'Device Creation with Qt'")),
                "Verifying: Link to the expected demonstration video exists.")
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
