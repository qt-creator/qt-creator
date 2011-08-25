
def invokeMenuItem(menu, item):
    menuObject = waitForObjectItem("{type='QMenuBar' visible='true'}", menu)
    activateItem(menuObject)
    activateItem(waitForObjectItem(menuObject, item))

def openProject(projectPath):
    invokeMenuItem("File", "Open File or Project...")
    waitForObject("{name='QFileDialog' type='QFileDialog' visible='1' windowTitle='Open File'}")
    type(findObject("{name='fileNameEdit' type='QLineEdit'}"), projectPath)
    clickButton(findObject("{text='Open' type='QPushButton'}"))
    waitForObject("{type='Qt4ProjectManager::Internal::ProjectLoadWizard' visible='1' windowTitle='Project Setup'}")
    clickButton(findObject("{text='Finish' type='QPushButton'}"))
