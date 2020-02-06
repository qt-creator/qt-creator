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

def main():
    global tmpSettingsDir, availableBuildSystems
    availableBuildSystems = ["qmake", "Qbs"]
    if which("cmake"):
        availableBuildSystems.append("CMake")
    else:
        test.warning("Could not find cmake in PATH - several tests won't run without.")

    startQC()
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
        mouseClick(waitForObjectItem(categoriesView, "Projects." + category))
        templatesView = waitForObject("{name='templatesView' type='QListView' visible='1'}")
        # needed because categoriesView and templatesView using same model
        for template in dumpItems(templatesView.model(), templatesView.rootIndex()):
            template = template.replace(".", "\\.")
            # skip non-configurable
            if (template not in ["Qt Quick UI Prototype", "Auto Test Project", "Qt Creator Plugin"]
                and "Qt for Python - " not in template): # FIXME
                availableProjectTypes.append({category:template})
    safeClickButton("Cancel")
    for current in availableProjectTypes:
        category = current.keys()[0]
        template = current.values()[0]
        with TestSection("Testing project template %s -> %s" % (category, template)):
            displayedPlatforms = __createProject__(category, template)
            if template.startswith("Qt Quick Application - "):
                qtVersionsForQuick = ["5.10", "5.14"]
                for counter, qtVersion in enumerate(qtVersionsForQuick):
                    def additionalFunc(displayedPlatforms, qtVersion):
                        requiredQtVersion = __createProjectHandleQtQuickSelection__(qtVersion)
                        __modifyAvailableTargets__(displayedPlatforms, requiredQtVersion, True)
                    handleBuildSystemVerifyKits(category, template, kits, displayedPlatforms,
                                                additionalFunc, qtVersion)
                    # are there more Quick combinations - then recreate this project
                    if counter < len(qtVersionsForQuick) - 1:
                        displayedPlatforms = __createProject__(category, template)
            elif template in ("Qt Widgets Application", "Qt Quick 2 Extension Plugin", "C++ Library"):
                def skipDetails(_):
                    clickButton(waitForObject(":Next_QPushButton"))
                handleBuildSystemVerifyKits(category, template, kits,
                                            displayedPlatforms, skipDetails)
            else:
                handleBuildSystemVerifyKits(category, template, kits, displayedPlatforms)

    invokeMenuItem("File", "Exit")

def verifyKitCheckboxes(kits, displayedPlatforms):
    waitForObject("{type='QLabel' unnamed='1' visible='1' text='Kit Selection'}")
    availableCheckboxes = frozenset(filter(enabledCheckBoxExists, kits.keys()))
    # verification whether expected, found and configured match

    expectedShownKits = availableCheckboxes.intersection(displayedPlatforms)
    unexpectedShownKits = availableCheckboxes.difference(displayedPlatforms)
    missingKits = displayedPlatforms.difference(availableCheckboxes)

    test.log("Expected kits shown on 'Kit Selection' page:\n%s" % "\n".join(expectedShownKits))
    if len(unexpectedShownKits):
        test.fail("Kits found on 'Kit Selection' page but not expected:\n%s"
                  % "\n".join(unexpectedShownKits))
    if len(missingKits):
        test.fail("Expected kits missing on 'Kit Selection' page:\n%s"
                  % "\n".join(missingKits))

def handleBuildSystemVerifyKits(category, template, kits, displayedPlatforms,
                                specialHandlingFunc = None, *args):
    global availableBuildSystems
    combo = "{name='BuildSystem' type='QComboBox' visible='1'}"
    try:
        waitForObject(combo, 2000)
        skipBuildsystemChooser = False
    except:
        skipBuildsystemChooser = True

    if skipBuildsystemChooser:
        test.log("Wizard without build system support.")
        if specialHandlingFunc:
            specialHandlingFunc(displayedPlatforms, *args)
        verifyKitCheckboxes(kits, displayedPlatforms)
        safeClickButton("Cancel")
        return

    for counter, buildSystem in enumerate(availableBuildSystems):
        test.log("Using build system '%s'" % buildSystem)
        selectFromCombo(combo, buildSystem)
        clickButton(waitForObject(":Next_QPushButton"))
        if specialHandlingFunc:
            specialHandlingFunc(displayedPlatforms, *args)
        if not ('Plain C' in template):
            __createProjectHandleTranslationSelection__()
        verifyKitCheckboxes(kits, displayedPlatforms)
        safeClickButton("Cancel")
        if counter < len(availableBuildSystems) - 1:
            displayedPlatforms = __createProject__(category, template)

def __createProject__(category, template):
    def safeGetTextBrowserText():
        try:
            return str(waitForObject(":frame.templateDescription_QTextBrowser", 500).plainText)
        except:
            return ""

    invokeMenuItem("File", "New File or Project...")
    selectFromCombo(waitForObject(":New.comboBox_QComboBox"), "All Templates")
    categoriesView = waitForObject(":New.templateCategoryView_QTreeView")
    mouseClick(waitForObjectItem(categoriesView, "Projects." + category))
    templatesView = waitForObject("{name='templatesView' type='QListView' visible='1'}")

    test.log("Verifying '%s' -> '%s'" % (category.replace("\\.", "."), template.replace("\\.", ".")))
    origTxt = safeGetTextBrowserText()
    mouseClick(waitForObjectItem(templatesView, template))
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
