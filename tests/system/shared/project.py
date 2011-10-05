def shadowBuildDir(path, project, qtVersion, debugVersion):
    qtVersion = qtVersion.replace(" ", "_")
    qtVersion = qtVersion.replace(".", "_")
    qtVersion = qtVersion.replace("(", "_")
    qtVersion = qtVersion.replace(")", "_")
    buildDir = path + os.sep + project + "-build-desktop-" + qtVersion
    if debugVersion:
        return buildDir + "_Debug"
    else:
        return buildDir + "_Release"

def createProject_Qt_GUI(path, projectName, qtVersion, checks):
    invokeMenuItem("File", "New File or Project...")
    waitForObjectItem(":New.templateCategoryView_QTreeView", "Projects.Qt Widget Project")
    clickItem(":New.templateCategoryView_QTreeView", "Projects.Qt Widget Project", 125, 16, 0, Qt.LeftButton)
    waitForObjectItem(":New.templatesView_QListView", "Qt Gui Application")
    clickItem(":New.templatesView_QListView", "Qt Gui Application", 35, 12, 0, Qt.LeftButton)
    clickButton(waitForObject(":New.Choose..._QPushButton"))
    directoryEdit = waitForObject(":frame_Utils::BaseValidatingLineEdit")
    replaceEditorContent(directoryEdit, path)
    projectNameEdit = waitForObject(":frame.nameLineEdit_Utils::ProjectNameValidatingLineEdit")
    replaceEditorContent(projectNameEdit, projectName)
    clickButton(waitForObject(":Qt Gui Application.Next_QPushButton"))

    desktopCheckbox = waitForObject(":scrollArea.Desktop_QCheckBox", 20000)
    if checks:
        test.compare(desktopCheckbox.text, "Desktop")
    if not desktopCheckbox.checked:
        clickButton(desktopCheckbox)
    selectFromCombo(":scrollArea.Create Build Configurations:_QComboBox_2", "For One Qt Version One Debug And One Release")
    selectFromCombo(":scrollArea.Qt Version:_QComboBox", qtVersion.replace(".", "\\."))
    if checks:
        if platform.system() in ('Windows', 'Microsoft'):
            path = os.path.abspath(path)
        verifyChecked(":scrollArea.Qt 4 for Desktop - (Qt SDK) debug_QCheckBox")
        verifyChecked(":scrollArea.Qt 4 for Desktop - (Qt SDK) release_QCheckBox")
    clickButton(waitForObject(":Qt Gui Application.Next_QPushButton"))

    if checks:
        exp_filename = "mainwindow"
        h_file = exp_filename + ".h"
        cpp_file = exp_filename + ".cpp"
        ui_file = exp_filename + ".ui"
        pro_file = projectName + ".pro"

        waitFor("object.exists(':headerFileLineEdit_Utils::FileNameValidatingLineEdit')", 20000)
        waitFor("object.exists(':sourceFileLineEdit_Utils::FileNameValidatingLineEdit')", 20000)
        waitFor("object.exists(':formFileLineEdit_Utils::FileNameValidatingLineEdit')", 20000)

        test.compare(findObject(":headerFileLineEdit_Utils::FileNameValidatingLineEdit").text, h_file)
        test.compare(findObject(":sourceFileLineEdit_Utils::FileNameValidatingLineEdit").text, cpp_file)
        test.compare(findObject(":formFileLineEdit_Utils::FileNameValidatingLineEdit").text, ui_file)

    clickButton(verifyEnabled(":Qt Gui Application.Next_QPushButton"))

    if checks:
        summary = str(waitForObject(":scrollArea.Files to be added").text)

        path_found = summary.find(os.path.join(path, projectName))
        cpp_found = summary.find(cpp_file)
        h_found = summary.find(h_file)
        ui_found = summary.find(ui_file)
        pro_found = summary.find(pro_file)

        test.verify(path_found > 0, "'" + path + "' found at index " + str(path_found))
        test.verify(cpp_found > path_found, "'" + cpp_file + "' found at index " + str(cpp_found))
        test.verify(h_found > cpp_found, "'" + h_file + "' found at index " + str(h_found))
        test.verify(ui_found > cpp_found, "'" + ui_file + "' found at index " + str(ui_found))
        test.verify(pro_found > ui_found, "'" + pro_file + "' found at index " + str(pro_found))

    clickButton(waitForObject(":Qt Gui Application.Finish_QPushButton"))

    if checks:
        waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}", "sourceFilesRefreshed(QStringList)", 20000)

        path = os.path.join(path, projectName)
        cpp_path = os.path.join(path, cpp_file)
        h_path = os.path.join(path, h_file)
        ui_path = os.path.join(path, ui_file)
        pro_path = os.path.join(path, pro_file)

        test.verify(os.path.exists(cpp_path), "Checking if '" + cpp_path + "' was created")
        test.verify(os.path.exists(h_path), "Checking if '" + h_path + "' was created")
        test.verify(os.path.exists(ui_path), "Checking if '" + ui_path + "' was created")
        test.verify(os.path.exists(pro_path), "Checking if '" + pro_path + "' was created")
