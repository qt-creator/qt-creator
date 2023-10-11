# Copyright (C) 2017 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    sideBar = waitForObject("{container={type='Welcome::Internal::SideArea' unnamed='1' "
                            "window=':Qt Creator_Core::Internal::MainWindow'} type='QWidget' "
                            "unnamed='1'}")
    return __getWelcomeScreenButtonHelper__(buttonLabel, sideBar, isUrlButton)

def getWelcomeScreenBottomButton(buttonLabel):
    bottomArea = waitForObject("{type='Welcome::Internal::BottomArea' unnamed='1' "
                               "window=':Qt Creator_Core::Internal::MainWindow'}")
    return __getWelcomeScreenButtonHelper__(buttonLabel, bottomArea, False)

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
    filterModel = __childrenOfType__(tableView, 'QSortFilterProxyModel')
    if len(filterModel) != 1:
        test.fatal("Something's wrong - could not find filter proxy model.")
        return None
    filterModel = filterModel[0]
    if filterModel.rowCount() == 0:
        return None

    children = dumpIndices(filterModel)
    for child in children:
        if re.match(regex, str(child.text)):
            if verbose:
                test.log("Returning matching example/tutorial '%s'." % str(child.text), regex)
            return child
    return None
