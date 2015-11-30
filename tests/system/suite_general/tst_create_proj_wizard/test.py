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

def main():
    global tmpSettingsDir
    qtVersionsForQuick = ["5.3"]
    availableBuildSystems = ["qmake", "Qbs"]
    if platform.system() != 'Darwin':
        qtVersionsForQuick.append("5.4")
    if which("cmake"):
        availableBuildSystems.append("CMake")
    else:
        test.warning("Could not find cmake in PATH - several tests won't run without.")

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
    test.verify(comboBox.enabled, "Verifying whether combobox is enabled.")
    test.compare(comboBox.currentText, "All Templates")
    try:
        selectFromCombo(comboBox, "All Templates")
    except:
        test.warning("Could not explicitly select 'All Templates' from combobox.")
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
            if not template in ["Qt Quick UI", "Qt Quick Controls UI", "Qt Canvas 3D Application"]:
                availableProjectTypes.append({category:template})
    safeClickButton("Cancel")
    for current in availableProjectTypes:
        category = current.keys()[0]
        template = current.values()[0]
        displayedPlatforms = __createProject__(category, template)
        if template == "Qt Quick Application" or template == "Qt Quick Controls Application":
            for counter, qtVersion in enumerate(qtVersionsForQuick):
                requiredQtVersion = __createProjectHandleQtQuickSelection__(qtVersion)
                __modifyAvailableTargets__(displayedPlatforms, requiredQtVersion, True)
                verifyKitCheckboxes(kits, displayedPlatforms)
                # FIXME: if QTBUG-35203 is fixed replace by triggering the shortcut for Back
                safeClickButton("Cancel")
                # are there more Quick combinations - then recreate this project
                if counter < len(qtVersionsForQuick) - 1:
                    displayedPlatforms = __createProject__(category, template)
            continue
        elif template.startswith("Plain C"):
            for counter, buildSystem in enumerate(availableBuildSystems):
                combo = "{name='BuildSystem' type='Utils::TextFieldComboBox' visible='1'}"
                selectFromCombo(combo, buildSystem)
                clickButton(waitForObject(":Next_QPushButton"))
                verifyKitCheckboxes(kits, displayedPlatforms)
                # FIXME: if QTBUG-35203 is fixed replace by triggering the shortcut for Back
                safeClickButton("Cancel")
                if counter < len(availableBuildSystems) - 1:
                    displayedPlatforms = __createProject__(category, template)
            continue

        verifyKitCheckboxes(kits, displayedPlatforms)
        safeClickButton("Cancel")

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
    def safeGetTextBrowserText():
        try:
            return str(waitForObject(":frame.templateDescription_QTextBrowser", 500).plainText)
        except:
            return ""

    invokeMenuItem("File", "New File or Project...")
    selectFromCombo(waitForObject(":New.comboBox_QComboBox"), "All Templates")
    categoriesView = waitForObject(":New.templateCategoryView_QTreeView")
    clickItem(categoriesView, "Projects." + category, 5, 5, 0, Qt.LeftButton)
    templatesView = waitForObject("{name='templatesView' type='QListView' visible='1'}")

    test.log("Verifying '%s' -> '%s'" % (category.replace("\\.", "."), template.replace("\\.", ".")))
    origTxt = safeGetTextBrowserText()
    clickItem(templatesView, template, 5, 5, 0, Qt.LeftButton)
    waitFor("origTxt != safeGetTextBrowserText() != ''", 2000)
    displayedPlatforms = __getSupportedPlatforms__(safeGetTextBrowserText(), template, True)[0]
    safeClickButton("Choose...")
    # don't check because project could exist
    __createProjectSetNameAndPath__(os.path.expanduser("~"), 'untitled', False)
    return displayedPlatforms

def safeClickButton(buttonLabel):
    buttonString = "{type='QPushButton' text='%s' visible='1' unnamed='1'}"
    for _ in range(5):
        try:
            clickButton(buttonString % buttonLabel)
            return
        except:
            if buttonLabel == "Cancel":
                try:
                    clickButton("{name='qt_wizard_cancel' type='QPushButton' text='Cancel' "
                                "visible='1'}")
                    return
                except:
                    pass
            snooze(1)
    test.fatal("Even safeClickButton failed...")
