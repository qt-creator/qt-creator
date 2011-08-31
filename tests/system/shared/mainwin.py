
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
    selectFromCombo(":scrollArea.Create Build Configurations:_QComboBox", "per Qt Version a Debug and Release")
    clickButton(findObject("{text='Finish' type='QPushButton'}"))

def openCmakeProject(projectPath):
    invokeMenuItem("File", "Open File or Project...")
    waitForObject("{name='QFileDialog' type='QFileDialog' visible='1' windowTitle='Open File'}")
    type(findObject("{name='fileNameEdit' type='QLineEdit'}"), projectPath)
    clickButton(findObject("{text='Open' type='QPushButton'}"))
    clickButton(waitForObject(":CMake Wizard.Next_QPushButton", 20000))
    clickButton(waitForObject(":CMake Wizard.Run CMake_QPushButton", 20000))
    clickButton(waitForObject(":CMake Wizard.Finish_QPushButton", 60000))
