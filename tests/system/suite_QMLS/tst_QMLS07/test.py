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

source("../shared/qmls.py")

def main():
    editorArea = startQtCreatorWithNewAppAtQMLEditor(tempDir(), "SampleApp", "Window {")
    if not editorArea:
        return
    type(editorArea, "<Return>")
    type(editorArea, "Color")
    for i in range(3):
        type(editorArea, "<Left>")
    invokeMenuItem("File", "Save All")
    # invoke Refactoring - Add a message suppression comment.
    numLinesExpected = len(str(editorArea.plainText).splitlines()) + 1
    try:
        invokeContextMenuItem(editorArea, "Refactoring", "Add a Comment to Suppress This Message")
    except:
        # If menu item is disabled it needs to reopen the menu for updating
        invokeContextMenuItem(editorArea, "Refactoring", "Add a Comment to Suppress This Message")
    # wait until refactoring ended
    waitFor("len(str(editorArea.plainText).splitlines()) >= numLinesExpected", 5000)
    # verify if refactoring was properly applied
    type(editorArea, "<Up>")
    test.compare(str(lineUnderCursor(editorArea)).strip(), "// @disable-check M16",
                 "Verifying 'Add comment to suppress message' refactoring")
    # save and exit
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
