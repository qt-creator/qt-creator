# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

try:
    import __builtin__                  # Python 2
except ImportError:
    import builtins as __builtin__      # Python 3

# for easier re-usage (because Python hasn't an enum type)
class Targets:
    ALL_TARGETS = tuple(range(4))

    (DESKTOP_5_4_1_GCC,
     DESKTOP_5_10_1_DEFAULT,
     DESKTOP_5_14_1_DEFAULT,
     DESKTOP_6_2_4) = ALL_TARGETS

    __TARGET_NAME_DICT__ = dict(zip(ALL_TARGETS,
                                    ["Desktop 5.4.1 GCC",
                                     "Desktop 5.10.1 default",
                                     "Desktop 5.14.1 default",
                                     "Desktop 6.2.4"]))

    @staticmethod
    def isOnlineInstaller(target):
        return target == Targets.DESKTOP_6_2_4

    @staticmethod
    def availableTargetClasses(ignoreValidity=False):
        availableTargets = set(Targets.ALL_TARGETS)
        if platform.system() == 'Darwin':
            availableTargets.remove(Targets.DESKTOP_5_4_1_GCC)
        return availableTargets

    @staticmethod
    def desktopTargetClasses():
        return Targets.availableTargetClasses()

    @staticmethod
    def getStringForTarget(target):
        return Targets.__TARGET_NAME_DICT__[target]

    @staticmethod
    def getTargetsAsStrings(targets):
        return set(map(Targets.getStringForTarget, targets))

    @staticmethod
    def getIdForTargetName(targetName):
        return {v:k for k, v in Targets.__TARGET_NAME_DICT__.items()}[targetName]

    @staticmethod
    def getDefaultKit():
        return Targets.DESKTOP_5_14_1_DEFAULT

# this class holds some constants for easier usage inside the Projects view
class ProjectSettings:
    BUILD = 1
    RUN = 2

# this class defines some constants for the views of the creator's MainWindow
class ViewConstants:
    WELCOME, EDIT, DESIGN, DEBUG, PROJECTS, HELP = range(6)
    FIRST_AVAILABLE = 0
    # always adjust the following to the highest value of the available ViewConstants when adding new
    LAST_AVAILABLE = HELP

class LibType:
    SHARED = 0
    STATIC = 1
    QT_PLUGIN = 2

    @staticmethod
    def getStringForLib(libType):
        if libType == LibType.SHARED:
            return "Shared Library"
        if libType == LibType.STATIC:
            return "Statically Linked Library"
        if libType == LibType.QT_PLUGIN:
            return "Qt Plugin"
        return None

class QtPath:
    DOCS = 0
    EXAMPLES = 1

    @staticmethod
    def getPaths(pathSpec):
        qtTargets = [Targets.DESKTOP_5_10_1_DEFAULT, Targets.DESKTOP_5_14_1_DEFAULT,
                     Targets.DESKTOP_6_2_4]
        if platform.system() != 'Darwin':
            qtTargets.append(Targets.DESKTOP_5_4_1_GCC)
        if pathSpec == QtPath.DOCS:
            return map(lambda target: QtPath.docsPath(target), qtTargets)
        elif pathSpec == QtPath.EXAMPLES:
            return map(lambda target: QtPath.examplesPath(target), qtTargets)
        else:
            test.fatal("Unknown pathSpec given: %s" % str(pathSpec))
            return []

    @staticmethod
    def __preCheckAndExtractQtVersionStr__(target):
        if target not in Targets.ALL_TARGETS:
            raise Exception("Unexpected target '%s'" % str(target))

        matcher = re.match("^Desktop ([56]\.\\d{1,2}\.\\d{1,2}).*$", Targets.getStringForTarget(target))
        if matcher is None:
            raise Exception("Currently this is supported for Desktop Qt5/Qt6 only, got target '%s'"
                            % str(Targets.getStringForTarget(target)))
        return matcher.group(1)

    @staticmethod
    def __createPlatformQtPath__(qt5Minor):
        if platform.system() in ('Microsoft', 'Windows'):
            return "C:/Qt/Qt5.%d.1" % qt5Minor
        else:
            return os.path.expanduser("~/Qt5.%d.1" % qt5Minor)

    @staticmethod
    def __createQtOnlineInstallerPath__():
        qtBasePath = os.getenv('SYSTEST_QTOI_BASEPATH', None)
        if qtBasePath is None:
            qtBasePath = 'C:/Qt' if platform.system() in ('Microsoft', 'Windows') else '~/Qt'
        qtBasePath = os.path.expanduser(qtBasePath)
        if not os.path.exists(qtBasePath):
            test.fatal("Unexpected Qt install path '%s'" % qtBasePath)
            return ""
        return qtBasePath

    @staticmethod
    def toVersionTuple(versionString):
        return tuple(map(__builtin__.int, versionString.split(".")))

    @staticmethod
    def getQtVersion(target):
        qtVersionStr = QtPath.__preCheckAndExtractQtVersionStr__(target)
        versionTuple = QtPath.toVersionTuple(qtVersionStr)
        return versionTuple

    @staticmethod
    def examplesPath(target):
        qtMajorVersion, qtMinorVersion, qtPatchVersion = QtPath.getQtVersion(target)
        if qtMajorVersion == 5 and qtMinorVersion < 10:
            path = "Examples/Qt-%d.%d" % (qtMajorVersion,  qtMinorVersion)
        else:
            path = "Examples/Qt-%d.%d.%d" % (qtMajorVersion, qtMinorVersion, qtPatchVersion)

        if Targets.isOnlineInstaller(target):
            return os.path.join(QtPath.__createQtOnlineInstallerPath__(), path)
        return os.path.join(QtPath.__createPlatformQtPath__(qtMinorVersion), path)

    @staticmethod
    def docsPath(target):
        qtMajorVersion, qtMinorVersion, qtPatchVersion = QtPath.getQtVersion(target)
        if qtMajorVersion == 5 and qtMinorVersion < 10:
            path = "Docs/Qt-%d.%d" % (qtMajorVersion, qtMinorVersion)
        else:
            path = "Docs/Qt-%d.%d.%d" % (qtMajorVersion, qtMinorVersion, qtPatchVersion)

        if Targets.isOnlineInstaller(target):
            return os.path.join(QtPath.__createQtOnlineInstallerPath__(), path)
        return os.path.join(QtPath.__createPlatformQtPath__(qtMinorVersion), path)

class TestSection:
    def __init__(self, description):
        self.description = description

    def __enter__(self):
        test.startSection(self.description)

    def __exit__(self, exc_type, exc_value, traceback):
        test.endSection()
