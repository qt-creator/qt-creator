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

def __getWelcomeScreenButtonHelper__(buttonLabel, widgetWithQFrames):
    frames = [child for child in object.children(widgetWithQFrames) if className(child) == 'QWidget']
    for frame in frames:
        label = getChildByClass(frame, 'QLabel')
        if str(label.text) == buttonLabel:
            return frame, label
    return None, None

def getWelcomeScreenSideBarButton(buttonLabel):
    sideBar = waitForObject("{type='Welcome::Internal::SideBar' unnamed='1' "
                            "window=':Qt Creator_Core::Internal::MainWindow'}")
    return __getWelcomeScreenButtonHelper__(buttonLabel, sideBar)

def getWelcomeScreenMainButton(buttonLabel):
    stackedWidget = waitForObject("{type='QWidget' unnamed='1' visible='1' "
                                  "leftWidget={type='QWidget' unnamed='1' visible='1' "
                                  "leftWidget={type='Welcome::Internal::SideBar' unnamed='1' "
                                  "window=':Qt Creator_Core::Internal::MainWindow'}}}")
    currentStackWidget = stackedWidget.currentWidget()
    return __getWelcomeScreenButtonHelper__(buttonLabel, currentStackWidget)

def getWelcomeTreeView(treeViewLabel):
    try:
        return waitForObject("{aboveWidget={text='%s' type='QLabel' unnamed='1' visible='1' "
                             "window=':Qt Creator_Core::Internal::MainWindow'} "
                             "type='QTreeView' unnamed='1' visible='1'}" % treeViewLabel)
    except:
        return None

def findExampleOrTutorial(tableView, regex, verbose=False):
    model = tableView.model()
    children = [ch for ch in object.children(tableView) if className(ch) == 'QModelIndex']
    for child in children:
        if re.match(regex, str(child.text)):
            if verbose:
                test.log("Returning matching example/tutorial '%s'." % str(child.text), regex)
            return child
    return None
