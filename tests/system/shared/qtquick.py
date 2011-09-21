processStarted = False
processExited = False

# for easier re-usage (because Python hasn't an enum type)
class QtQuickConstants:
    class Components:
        BUILTIN = 1
        SYMBIAN = 2
        MEEGO_HARMATTAN = 4
        EXISTING_QML = 8

    class Destinations:
        DESKTOP = 1
        SIMULATOR = 2
        SYMBIAN = 4
        MAEMO5 = 8
        HARMATTAN = 16

    @staticmethod
    def getStringForComponents(components):
            if components==QtQuickConstants.Components.BUILTIN:
                return "Built-in elements only (for all platforms)"
            elif components==QtQuickConstants.Components.SYMBIAN:
                return "Qt Quick Components for Symbian"
            elif components==QtQuickConstants.Components.MEEGO_HARMATTAN:
                return "Qt Quick Components for Meego/Harmattan"
            elif components==QtQuickConstants.Components.EXISTING_QML:
                return "Use an existing .qml file"
            else:
                return None

    @staticmethod
    def getStringForDestination(destination):
        if destination==QtQuickConstants.Destinations.DESKTOP:
            return "Desktop"
        elif destination==QtQuickConstants.Destinations.SYMBIAN:
            return "Symbian Device"
        elif destination==QtQuickConstants.Destinations.MAEMO5:
            return "Maemo5"
        elif destination==QtQuickConstants.Destinations.SIMULATOR:
            return "Qt Simulator"
        elif destination==QtQuickConstants.Destinations.HARMATTAN:
            return "Harmattan"
        else:
            return None

def __handleProcessStarted__(object):
    global processStarted
    processStarted = True

def __handleProcessExited__(object, exitCode):
    global processExited
    processExited = True

# parameter components can only be one of the Constants defined in QtQuickConstants.Components
def chooseComponents(components=QtQuickConstants.Components.BUILTIN):
    rbComponentToChoose = waitForObject("{type='QRadioButton' text='%s' visible='1'}"
                              % QtQuickConstants.getStringForComponents(components), 20000)
    if rbComponentToChoose.checked:
        test.passes("Selected QRadioButton is '%s'" % QtQuickConstants.getStringForComponents(components))
    else:
        clickButton(rbComponentToChoose)
        test.verify(rbComponentToChoose.checked, "Selected QRadioButton is '%s'"
                % QtQuickConstants.getStringForComponents(components))

# parameter destination can be an OR'd value of QtQuickConstants.Destinations
def chooseDestination(destination=QtQuickConstants.Destinations.DESKTOP):
     # DESKTOP should be always accessible
    destDesktop = waitForObject("{type='QCheckBox' text='%s' visible='1'}"
                                % QtQuickConstants.getStringForDestination(QtQuickConstants.Destinations.DESKTOP), 20000)
    mustCheck = destination & QtQuickConstants.Destinations.DESKTOP==QtQuickConstants.Destinations.DESKTOP
    if (mustCheck and not destDesktop.checked) or (not mustCheck and destDesktop.checked):
        clickButton(destDesktop)
    # following destinations depend on the build environment - added for further/later tests
    available = [QtQuickConstants.Destinations.SYMBIAN, QtQuickConstants.Destinations.MAEMO5,
                 QtQuickConstants.Destinations.SIMULATOR, QtQuickConstants.Destinations.HARMATTAN]
    for current in available:
        mustCheck = destination & current == current
        try:
            target = findObject("{type='QCheckBox' text='%s' visible='1'}" % QtQuickConstants.getStringForDestination(current))
            if (mustCheck and not target.checked) or (not mustCheck and target.checked):
                clickButton(target)
        except LookupError:
            if mustCheck:
                test.fail("Failed to check destination '%s'" % QtQuickConstants.getStringForDestination(current))

def runAndCloseApp():
    global processStarted, processExited
    processStarted = processExited = False
    installLazySignalHandler("{type='ProjectExplorer::ApplicationLaucher'}", "processStarted()", "__handleProcessStarted__")
    installLazySignalHandler("{type='ProjectExplorer::ApplicationLaucher'}", "processExited(int)", "__handleProcessExited__")
    runButton = waitForObject("{type='Core::Internal::FancyToolButton' text='Run' visible='1'}", 20000)
    clickButton(runButton)
    waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)", 300000)
    buildSucceeded = checkLastBuild()
    waitFor("processStarted==True", 10000)
    if not buildSucceeded:
        test.log("Build inside run wasn't successful - leaving test")
        invokeMenuItem("File", "Exit")
        return False
    if not processStarted:
        test.fatal("Couldn't start application - leaving test")
        invokeMenuItem("File", "Exit")
        return False
    # the following is currently a work-around for not using hooking into subprocesses
    if (waitForObject(":Qt Creator_Core::Internal::OutputPaneToggleButton").checked!=True):
        clickButton(":Qt Creator_Core::Internal::OutputPaneToggleButton")
    clickButton(":Qt Creator.Stop_QToolButton")
    return True

def runAndCloseQtQuickUI():
    global processStarted, processExited
    processStarted = processExited = False
    installLazySignalHandler("{type='ProjectExplorer::ApplicationLaucher'}", "processStarted()", "__handleProcessStarted__")
    installLazySignalHandler("{type='ProjectExplorer::ApplicationLaucher'}", "processExited(int)", "__handleProcessExited__")
    runButton = waitForObject("{type='Core::Internal::FancyToolButton' text='Run' visible='1'}", 20000)
    clickButton(runButton)
    waitFor("processStarted==True", 10000)
    # the following is currently a work-around for not using hooking into subprocesses
    if (waitForObject(":Qt Creator_Core::Internal::OutputPaneToggleButton").checked!=True):
        clickButton(":Qt Creator_Core::Internal::OutputPaneToggleButton")
    clickButton(":Qt Creator.Stop_QToolButton")
    return True

