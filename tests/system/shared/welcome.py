############################################################################
#
# Copyright (C) 2017 The Qt Company Ltd.
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

def __childrenOfType__(parentObject, typeName):
    return [child for child in object.children(parentObject) if className(child) == typeName]

def __getWelcomeScreenButtonHelper__(buttonLabel, widgetWithQFrames, isUrlButton = False):
    frames = __childrenOfType__(widgetWithQFrames, 'QWidget')
    for frame in frames:
        childCount = 1 # incorrect but okay for framed sidebar buttons
        if isUrlButton:
            childCount = len(__childrenOfType__(frame, 'QLabel'))
        for occurrence in range(1, childCount + 1):
            label = getChildByClass(frame, 'QLabel', occurrence)
            if label is None:
                continue
            if str(label.text) == buttonLabel:
                return frame, label
    return None, None

def getWelcomeScreenSideBarButton(buttonLabel, isUrlButton = False):
    sideBar = waitForObject("{type='Welcome::Internal::SideBar' unnamed='1' "
                            "window=':Qt Creator_Core::Internal::MainWindow'}")
    return __getWelcomeScreenButtonHelper__(buttonLabel, sideBar, isUrlButton)

def getWelcomeScreenMainButton(buttonLabel):
    stackedWidget = waitForObject(":Qt Creator.WelcomeScreenStackedWidget")
    currentStackWidget = stackedWidget.currentWidget()
    return __getWelcomeScreenButtonHelper__(buttonLabel, currentStackWidget)

def getWelcomeTreeView(treeViewLabel):
    try:
        return waitForObjectExists("{container=':Qt Creator.WelcomeScreenStackedWidget' "
                                   "name='%s' type='QTreeView' visible='1'}" % treeViewLabel)
    except:
        return None

def switchToSubMode(subModeLabel):
    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton(subModeLabel)
    frameAndLabelFound = all((wsButtonFrame, wsButtonLabel))
    if frameAndLabelFound:
        mouseClick(wsButtonLabel)
    return frameAndLabelFound

def findExampleOrTutorial(tableView, regex, verbose=False):
    children = __childrenOfType__(tableView, 'QModelIndex')
    for child in children:
        if re.match(regex, str(child.text)):
            if verbose:
                test.log("Returning matching example/tutorial '%s'." % str(child.text), regex)
            return child
    return None
