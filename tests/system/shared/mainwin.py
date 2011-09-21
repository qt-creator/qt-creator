
def invokeMenuItem(menu, item):
    menuObject = waitForObjectItem("{type='QMenuBar' visible='true'}", menu)
    activateItem(menuObject)
    activateItem(waitForObjectItem(menuObject, item))

def openQmakeProject(projectPath):
    invokeMenuItem("File", "Open File or Project...")
    waitForObject("{name='QFileDialog' type='QFileDialog' visible='1' windowTitle='Open File'}")
    type(findObject("{name='fileNameEdit' type='QLineEdit'}"), projectPath)
    clickButton(findObject("{text='Open' type='QPushButton'}"))
    waitForObject("{type='Qt4ProjectManager::Internal::ProjectLoadWizard' visible='1' windowTitle='Project Setup'}")
    selectFromCombo(":scrollArea.Create Build Configurations:_QComboBox", "For Each Qt Version One Debug And One Release")
    clickButton(findObject("{text='Finish' type='QPushButton'}"))

def openCmakeProject(projectPath):
    invokeMenuItem("File", "Open File or Project...")
    waitForObject("{name='QFileDialog' type='QFileDialog' visible='1' windowTitle='Open File'}")
    type(findObject("{name='fileNameEdit' type='QLineEdit'}"), projectPath)
    clickButton(findObject("{text='Open' type='QPushButton'}"))
    clickButton(waitForObject(":CMake Wizard.Next_QPushButton", 20000))
    generatorCombo = waitForObject(":Generator:_QComboBox")
    index = generatorCombo.findText("MinGW Generator (MinGW from SDK)")
    if index == -1:
        index = generatorCombo.findText("NMake Generator (Microsoft Visual C++ Compiler 9.0 (x86))")
    if index != -1:
        generatorCombo.setCurrentIndex(index)
    clickButton(waitForObject(":CMake Wizard.Run CMake_QPushButton", 20000))
    clickButton(waitForObject(":CMake Wizard.Finish_QPushButton", 60000))

def logApplicationOutput():
    # make sure application output is shown
    toggleAppOutput = waitForObject("{type='Core::Internal::OutputPaneToggleButton' unnamed='1' visible='1' "
                                      "window=':Qt Creator_Core::Internal::MainWindow' occurrence='3'}", 20000)
    if not toggleAppOutput.checked:
        clickButton(toggleAppOutput)
    output = waitForObject("{type='Core::OutputWindow' visible='1' windowTitle='Application Output Window'}", 20000)
    test.log("Application Output:\n%s" % output.plainText)

