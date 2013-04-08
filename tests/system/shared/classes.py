import operator

# for easier re-usage (because Python hasn't an enum type)
class Targets:
    DESKTOP_474_GCC = 1
    SIMULATOR = 2
    MAEMO5 = 4
    HARMATTAN = 8
    EMBEDDED_LINUX = 16
    DESKTOP_474_MSVC2008 = 32
    DESKTOP_501_DEFAULT = 64

    @staticmethod
    def desktopTargetClasses():
        desktopTargets = Targets.DESKTOP_474_GCC | Targets.DESKTOP_501_DEFAULT
        if platform.system() in ('Windows', 'Microsoft'):
            desktopTargets |= Targets.DESKTOP_474_MSVC2008
        return desktopTargets

    @staticmethod
    def getStringForTarget(target):
        if target == Targets.DESKTOP_474_GCC:
            return "Desktop 474 GCC"
        elif target == Targets.MAEMO5:
            return "Fremantle"
        elif target == Targets.SIMULATOR:
            return "Qt Simulator"
        elif target == Targets.HARMATTAN:
            return "Harmattan"
        elif target == Targets.EMBEDDED_LINUX:
            return "Embedded Linux"
        elif target == Targets.DESKTOP_474_MSVC2008:
            return "Desktop 474 MSVC2008"
        elif target == Targets.DESKTOP_501_DEFAULT:
            return "Desktop 501 default"
        else:
            return None

    @staticmethod
    def getTargetsAsStrings(targets):
        if not isinstance(targets, (tuple,list)):
            test.fatal("Wrong usage... This function handles only tuples or lists.")
            return None
        result = map(Targets.getStringForTarget, targets)
        if None in result:
            test.fatal("You've passed at least one unknown target!")
        return result

    @staticmethod
    def intToArray(targets):
        available = [Targets.DESKTOP_474_GCC, Targets.SIMULATOR, Targets.MAEMO5, Targets.HARMATTAN,
                     Targets.EMBEDDED_LINUX, Targets.DESKTOP_474_MSVC2008,
                     Targets.DESKTOP_501_DEFAULT]
        return filter(lambda x: x & targets == x, available)

    @staticmethod
    def arrayToInt(targetArr):
        return reduce(operator.or_, targetArr, 0)

# this class holds some constants for easier usage inside the Projects view
class ProjectSettings:
    BUILD = 1
    RUN = 2

# this class defines some constants for the views of the creator's MainWindow
class ViewConstants:
    WELCOME = 0
    EDIT = 1
    DESIGN = 2
    DEBUG = 3
    PROJECTS = 4
    ANALYZE = 5
    HELP = 6
    # always adjust the following to the highest value of the available ViewConstants when adding new
    LAST_AVAILABLE = HELP

    # this function returns a regex of the tooltip of the FancyTabBar elements
    # this is needed because the keyboard shortcut is OS specific
    # if the provided argument does not match any of the ViewConstants it returns None
    @staticmethod
    def getToolTipForViewTab(viewTab):
        if viewTab == ViewConstants.WELCOME:
            return ur'Switch to <b>Welcome</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)1</span>'
        elif viewTab == ViewConstants.EDIT:
            return ur'Switch to <b>Edit</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)2</span>'
        elif viewTab == ViewConstants.DESIGN:
            return ur'Switch to <b>Design</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)3</span>'
        elif viewTab == ViewConstants.DEBUG:
            return ur'Switch to <b>Debug</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)4</span>'
        elif viewTab == ViewConstants.PROJECTS:
            return ur'Switch to <b>Projects</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)5</span>'
        elif viewTab == ViewConstants.ANALYZE:
            return ur'Switch to <b>Analyze</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)6</span>'
        elif viewTab == ViewConstants.HELP:
            return ur'Switch to <b>Help</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)7</span>'
        else:
            return None

class SubprocessType:
    QT_WIDGET=0
    QT_QUICK_APPLICATION=1
    QT_QUICK_UI=2
    USER_DEFINED=3

    @staticmethod
    def getWindowType(subprocessType):
        if subprocessType == SubprocessType.QT_WIDGET:
            return "QMainWindow"
        if subprocessType == SubprocessType.QT_QUICK_APPLICATION:
            return "QmlApplicationViewer"
        if subprocessType == SubprocessType.QT_QUICK_UI:
            return "QDeclarativeViewer"
        if subprocessType == SubprocessType.USER_DEFINED:
            return "user-defined"
        test.fatal("Could not determine the WindowType for SubprocessType %s" % subprocessType)
        return None

class QtInformation:
    QT_VERSION = 0
    QT_BINPATH = 1
    QT_LIBPATH = 2
