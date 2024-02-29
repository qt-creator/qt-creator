# Copyright (C) 2017 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

def __childrenOfType__(parentObject, typeName):
    return [child for child in object.children(parentObject) if className(child) == typeName]


def getWelcomeScreenSideBarButton(buttonLabel):
    return ("{text='%s' type='QPushButton' unnamed='1' visible='1' "
            "window=':Qt Creator_Core::Internal::MainWindow'}" % buttonLabel)


def getWelcomeTreeView(treeViewLabel):
    try:
        return waitForObjectExists("{container=':Qt Creator.WelcomeScreenStackedWidget' "
                                   "name='%s' type='QTreeView' visible='1'}" % treeViewLabel)
    except:
        return None

def switchToSubMode(subModeLabel):
    wsButton = getWelcomeScreenSideBarButton(subModeLabel)
    buttonFound = object.exists(wsButton)
    if buttonFound:
        mouseClick(wsButton)
    return buttonFound

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
