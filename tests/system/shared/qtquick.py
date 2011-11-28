processStarted = False
processExited = False

# for easier re-usage (because Python hasn't an enum type)
class QtQuickConstants:
    class Components:
        BUILTIN = 1
        SYMBIAN = 2
        MEEGO_HARMATTAN = 4
        EXISTING_QML = 8

    class Targets:
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
    def getStringForTarget(target):
        if target==QtQuickConstants.Targets.DESKTOP:
            return "Desktop"
        elif target==QtQuickConstants.Targets.SYMBIAN:
            return "Symbian Device"
        elif target==QtQuickConstants.Targets.MAEMO5:
            return "Maemo5"
        elif target==QtQuickConstants.Targets.SIMULATOR:
            return "Qt Simulator"
        elif target==QtQuickConstants.Targets.HARMATTAN:
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
def __chooseComponents__(components=QtQuickConstants.Components.BUILTIN):
    rbComponentToChoose = waitForObject("{type='QRadioButton' text='%s' visible='1'}"
                              % QtQuickConstants.getStringForComponents(components), 20000)
    if rbComponentToChoose.checked:
        test.passes("Selected QRadioButton is '%s'" % QtQuickConstants.getStringForComponents(components))
    else:
        clickButton(rbComponentToChoose)
        test.verify(rbComponentToChoose.checked, "Selected QRadioButton is '%s'"
                % QtQuickConstants.getStringForComponents(components))

# parameter target can be an OR'd value of QtQuickConstants.Targets
def __chooseTargets__(targets=QtQuickConstants.Targets.DESKTOP):
     # DESKTOP should be always accessible
    ensureChecked("{type='QCheckBox' text='%s' visible='1'}"
                  % QtQuickConstants.getStringForTarget(QtQuickConstants.Targets.DESKTOP),
                  targets & QtQuickConstants.Targets.DESKTOP)
    # following targets depend on the build environment - added for further/later tests
    available = [QtQuickConstants.Targets.MAEMO5,
                 QtQuickConstants.Targets.SIMULATOR, QtQuickConstants.Targets.HARMATTAN]
    if platform.system() in ('Windows', 'Microsoft'):
        available += [QtQuickConstants.Targets.SYMBIAN]
    for current in available:
        mustCheck = targets & current == current
        try:
            ensureChecked("{type='QCheckBox' text='%s' visible='1'}" % QtQuickConstants.getStringForTarget(current),
                          mustCheck)
        except LookupError:
            if mustCheck:
                test.fail("Failed to check target '%s'" % QtQuickConstants.getStringForTarget(current))

# run and close a Qt Quick application
# withHookInto - if set to True the function tries to attach to the sub-process instead of simply pressing Stop inside Creator
# executable - must be defined when using hook-into
# port - must be defined when using hook-into
# function - can be a string holding a function name or a reference to the function itself - this function will be called on
# the sub-process when hooking-into has been successful - if its missing simply closing the Qt Quick app will be done
# ATTENTION! Make sure this function won't fail and the sub-process will end when the function returns
def runAndCloseApp(withHookInto=False, executable=None, port=None, function=None):
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
    if withHookInto and not executable in ("", None):
        __closeSubprocessByHookingIntoQmlApplicationViewer__(executable, port, function)
    else:
        __closeSubprocessByPushingStop__()
    return True

def __closeSubprocessByPushingStop__():
    ensureChecked(":Qt Creator_Core::Internal::OutputPaneToggleButton")
    playButton = verifyEnabled(":Qt Creator.ReRun_QToolButton", False)
    stopButton = verifyEnabled(":Qt Creator.Stop_QToolButton")
    clickButton(stopButton)
    test.verify(playButton.enabled)
    test.compare(stopButton.enabled, False)

def __closeSubprocessByHookingIntoQmlApplicationViewer__(executable, port, function):
    global processExited
    ensureChecked(":Qt Creator_Core::Internal::OutputPaneToggleButton")
    output = waitForObject("{type='Core::OutputWindow' visible='1' windowTitle='Application Output Window'}", 20000)
    if port == None:
        test.warning("I need a port number or attaching might fail.")
    else:
        waitFor("'Listening on port %d for incoming connectionsdone' in str(output.plainText)" % port, 5000)
    attachToApplication(executable)
    if function == None:
        sendEvent("QCloseEvent", "{type='QmlApplicationViewer' unnamed='1' visible='1'}")
        setApplicationContext(applicationContext("qtcreator"))
    else:
        try:
            if isinstance(function, (str, unicode)):
                globals()[function]()
            else:
                function()
        except:
            test.fatal("Function to execute on sub-process could not be found.",
                       "Using fallback of pushing STOP inside Creator.")
            setApplicationContext(applicationContext("qtcreator"))
            __closeSubprocessByPushingStop__()
    waitFor("processExited==True", 10000)
    if not processExited:
        test.warning("Sub-process seems not to have closed properly.")
        try:
            setApplicationContext(applicationContext("qtcreator"))
            __closeSubprocessByPushingStop__()
        except:
            pass
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
    ensureChecked(":Qt Creator_Core::Internal::OutputPaneToggleButton")
    stop = findObject(":Qt Creator.Stop_QToolButton")
    waitFor("stop.enabled==True")
    clickButton(stop)
    if platform.system()=="Darwin":
        waitFor("stop.enabled==False")
        snooze(2)
        nativeType("<Escape>")
    return True

