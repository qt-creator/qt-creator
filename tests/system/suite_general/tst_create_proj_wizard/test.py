#############################################################################
##
## Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

source("../../shared/qtcreator.py")

import re

def main():
    global tmpSettingsDir
    quickCombinations = ["1.1", "2.1", "2.2", "Controls 1.0", "Controls 1.1"]
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    kits = getConfiguredKits()
    test.log("Collecting potential project types...")
    availableProjectTypes = []
    invokeMenuItem("File", "New File or Project...")
    categoriesView = waitForObject(":New.templateCategoryView_QTreeView")
    catModel = categoriesView.model()
    projects = catModel.index(0, 0)
    test.compare("Projects", str(projects.data()))
    comboBox = findObject(":New.comboBox_QComboBox")
    targets = zip(*kits.values())[0]
    maddeTargets = Targets.getTargetsAsStrings([Targets.MAEMO5, Targets.HARMATTAN])
    maddeInTargets = len(set(targets) & set(maddeTargets)) > 0
    test.verify(comboBox.enabled, "Verifying whether combobox is enabled.")
    test.verify(not maddeInTargets, "Verify there are no leftovers of Madde")
    test.compare(comboBox.currentText, "Desktop Templates")
    selectFromCombo(comboBox, "All Templates")
    for category in [item.replace(".", "\\.") for item in dumpItems(catModel, projects)]:
        # skip non-configurable
        if "Import" in category:
            continue
        clickItem(categoriesView, "Projects." + category, 5, 5, 0, Qt.LeftButton)
        templatesView = waitForObject("{name='templatesView' type='QListView' visible='1'}")
        # needed because categoriesView and templatesView using same model
        for template in dumpItems(templatesView.model(), templatesView.rootIndex()):
            template = template.replace(".", "\\.")
            # skip non-configurable
            if (template != "Qt Quick UI" and "(CMake Build)" not in template
                and "(Qbs Build)" not in template):
                availableProjectTypes.append({category:template})
    clickButton(waitForObject("{text='Cancel' type='QPushButton' unnamed='1' visible='1'}"))
    for current in availableProjectTypes:
        category = current.keys()[0]
        template = current.values()[0]
        displayedPlatforms = __createProject__(category, template)
        if template == "Qt Quick Application":
            for counter, qComb in enumerate(quickCombinations):
                requiredQtVersion = __createProjectHandleQtQuickSelection__(qComb)
                __modifyAvailableTargets__(displayedPlatforms, requiredQtVersion, True)
                verifyKitCheckboxes(kits, displayedPlatforms)
                # FIXME: if QTBUG-35203 is fixed replace by triggering the shortcut for Back
                clickButton(waitForObject("{type='QPushButton' text='Cancel' visible='1'}"))
                # are there more Quick combinations - then recreate this project
                if counter < len(quickCombinations) - 1:
                    displayedPlatforms = __createProject__(category, template)
            continue
        try:
            waitForObject("{name='mainQmlFileGroupBox' title='Main HTML File' type='QGroupBox' visible='1'}", 1000)
            clickButton(waitForObject(":Next_QPushButton"))
        except LookupError:
            pass
        verifyKitCheckboxes(kits, displayedPlatforms)
        clickButton(waitForObject("{type='QPushButton' text='Cancel' visible='1'}"))
    invokeMenuItem("File", "Exit")

def verifyKitCheckboxes(kits, displayedPlatforms):
    waitForObject("{type='QLabel' unnamed='1' visible='1' text='Kit Selection'}")
    availableCheckboxes = filter(visibleCheckBoxExists, kits.keys())
    # verification whether expected, found and configured match
    for t in kits:
        if t in displayedPlatforms:
            if t in availableCheckboxes:
                test.passes("Found expected kit '%s' on 'Kit Selection' page." % t)
                availableCheckboxes.remove(t)
            else:
                test.fail("Expected kit '%s' missing on 'Kit Selection' page." % t)
        else:
            if t in availableCheckboxes:
                test.fail("Kit '%s' found on 'Kit Selection' page - but was not expected!" % t)
            else:
                test.passes("Irrelevant kit '%s' not found on 'Kit Selection' page." % t)
    if len(availableCheckboxes) != 0:
        test.fail("Found unexpected additional kit(s) %s on 'Kit Selection' page."
                  % str(availableCheckboxes))

def __createProject__(category, template):
    invokeMenuItem("File", "New File or Project...")
    selectFromCombo(waitForObject(":New.comboBox_QComboBox"), "All Templates")
    categoriesView = waitForObject(":New.templateCategoryView_QTreeView")
    clickItem(categoriesView, "Projects." + category, 5, 5, 0, Qt.LeftButton)
    templatesView = waitForObject("{name='templatesView' type='QListView' visible='1'}")
    test.log("Verifying '%s' -> '%s'" % (category.replace("\\.", "."), template.replace("\\.", ".")))
    textBrowser = findObject(":frame.templateDescription_QTextBrowser")
    origTxt = str(textBrowser.plainText)
    clickItem(templatesView, template, 5, 5, 0, Qt.LeftButton)
    waitFor("origTxt != str(textBrowser.plainText)", 2000)
    displayedPlatforms = __getSupportedPlatforms__(str(textBrowser.plainText), template, True)[0]
    clickButton(waitForObject("{text='Choose...' type='QPushButton' unnamed='1' visible='1'}"))
    # don't check because project could exist
    __createProjectSetNameAndPath__(os.path.expanduser("~"), 'untitled', False)
    return displayedPlatforms
