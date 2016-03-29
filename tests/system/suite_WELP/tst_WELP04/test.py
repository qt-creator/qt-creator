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
source("../../shared/suites_qtta.py")

def main():
    if not canTestEmbeddedQtQuick():
        test.log("Welcome mode is not scriptable with this Squish version")
        return
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
                                               "'Qt for Device Creation'")),
                "Verifying: Link to the expected demonstration video exists.")
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
