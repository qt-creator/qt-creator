source("../../shared/qtcreator.py")

import re

def main():
    global textChanged
    sourceExample = os.path.abspath(sdkPath + "/Examples/4.7/declarative/text/textselection")
    qmlFile = os.path.join("qml", "textselection.qml")
    if not neededFilePresent(os.path.join(sourceExample, qmlFile)):
        return
    templateDir = prepareTemplate(sourceExample)
    startApplication("qtcreator" + SettingsPath)
    overrideInstallLazySignalHandler()
    installLazySignalHandler(":frame.templateDescription_QTextBrowser",
                             "textChanged()","__handleTextChanged__")
    kits = getConfiguredKits()
    test.log("Collecting potential project types...")
    availableProjectTypes = []
    invokeMenuItem("File", "New File or Project...")
    categoriesView = waitForObject(":New.templateCategoryView_QTreeView")
    catModel = categoriesView.model()
    projects = catModel.index(0, 0)
    test.compare("Projects", str(projects.data()))
    comboBox = waitForObject("{name='comboBox' type='QComboBox' visible='1' "
                             "window=':New_Core::Internal::NewDialog'}")
    test.compare(comboBox.currentText, "All Templates")
    for category in [item.replace(".", "\\.") for item in dumpItems(catModel, projects)]:
        # skip non-configurable
        if "Import" in category or "Non-Qt" in category:
            continue
        clickItem(categoriesView, "Projects." + category, 5, 5, 0, Qt.LeftButton)
        templatesView = waitForObject("{name='templatesView' type='QListView' visible='1'}")
        # needed because categoriesView and templatesView using same model
        for template in dumpItems(templatesView.model(), templatesView.rootIndex()):
            template = template.replace(".", "\\.")
            # skip non-configurable
            if "Qt Quick UI" in template or "Plain C" in template:
                continue
            availableProjectTypes.append({category:template})
    clickButton(waitForObject("{text='Cancel' type='QPushButton' unnamed='1' visible='1'}"))
    for current in availableProjectTypes:
        category = current.keys()[0]
        template = current.values()[0]
        invokeMenuItem("File", "New File or Project...")
        categoriesView = waitForObject(":New.templateCategoryView_QTreeView")
        clickItem(categoriesView, "Projects." + category, 5, 5, 0, Qt.LeftButton)
        templatesView = waitForObject("{name='templatesView' type='QListView' visible='1'}")
        test.log("Verifying '%s' -> '%s'" % (category.replace("\\.", "."), template.replace("\\.", ".")))
        textChanged = False
        clickItem(templatesView, template, 5, 5, 0, Qt.LeftButton)
        waitFor("textChanged", 2000)
        text = waitForObject(":frame.templateDescription_QTextBrowser").plainText
        displayedPlatforms, requiredVersion = __getSupportedPlatforms__(str(text), True)
        clickButton(waitForObject("{text='Choose...' type='QPushButton' unnamed='1' visible='1'}"))
        # don't check because project could exist
        __createProjectSetNameAndPath__(os.path.expanduser("~"), 'untitled', False)
        try:
            waitForObject("{name='mainQmlFileGroupBox' title='Main HTML File' type='QGroupBox' visible='1'}", 1000)
            clickButton(waitForObject(":Next_QPushButton"))
        except LookupError:
            try:
                waitForObject("{text='Select Existing QML file' type='QLabel' visible='1'}", 1000)
                baseLineEd = waitForObject("{type='Utils::BaseValidatingLineEdit' unnamed='1' visible='1'}")
                type(baseLineEd, os.path.join(templateDir, qmlFile))
                clickButton(waitForObject(":Next_QPushButton"))
            except LookupError:
                pass
        waitForObject("{type='QLabel' unnamed='1' visible='1' text='Kit Selection'}")
        availableCheckboxes = filter(visibleCheckBoxExists, kits.keys())
        # verification whether expected, found and configured match
        for t in kits:
            if requiredVersion:
                if kits[t][1] < requiredVersion:
                    if t in availableCheckboxes:
                        test.fail("Kit '%s' found as checkbox, but required Qt version (%s) is "
                                  "higher than configured Qt version (%s)!" % (t, requiredVersion,
                                                                               str(kits[t][1])))
                        availableCheckboxes.remove(t)
                    else:
                        test.passes("Irrelevant kit '%s' not found on 'Kit Selection' page - "
                                    "required Qt version is '%s', current Qt version is '%s'." %
                                    (t, requiredVersion, str(kits[t][1])))
                    continue
            found = False
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
        clickButton(waitForObject("{text='Cancel' type='QPushButton' unnamed='1' visible='1'}"))
    invokeMenuItem("File", "Exit")

def __handleTextChanged__(object):
    global textChanged
    textChanged = True
